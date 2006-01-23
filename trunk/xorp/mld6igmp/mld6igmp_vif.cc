// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2005 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.

#ident "$XORP: xorp/mld6igmp/mld6igmp_vif.cc,v 1.43 2005/08/25 19:01:32 pavlin Exp $"


//
// MLD6IGMP virtual interfaces implementation.
//


#include "mld6igmp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "mrt/inet_cksum.h"

#include "mld6igmp_vif.hh"


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//

//
// Local variables
//

//
// Local functions prototypes
//


/**
 * Mld6igmpVif::Mld6igmpVif:
 * @mld6igmp_node: The MLD6IGMP node this interface belongs to.
 * @vif: The generic Vif interface that contains various information.
 * 
 * MLD6IGMP protocol vif constructor.
 **/
Mld6igmpVif::Mld6igmpVif(Mld6igmpNode& mld6igmp_node, const Vif& vif)
    : ProtoUnit(mld6igmp_node.family(), mld6igmp_node.module_id()),
      Vif(vif),
      _mld6igmp_node(mld6igmp_node),
      _primary_addr(IPvX::ZERO(mld6igmp_node.family())),
      _querier_addr(IPvX::ZERO(mld6igmp_node.family())),
      _ip_router_alert_option_check(false),
      _query_interval(TimeVal(0, 0)),
      _query_last_member_interval(TimeVal(0, 0)),
      _query_response_interval(TimeVal(0, 0)),
      _robust_count(0),
      _dummy_flag(false)
{
    XLOG_ASSERT(proto_is_igmp() || proto_is_mld6());
    
    //
    // TODO: when more things become classes, most of this init should go away
    _buffer_send = BUFFER_MALLOC(BUF_SIZE_DEFAULT);
    _proto_flags = 0;
    _startup_query_count = 0;

    // Set the protocol version
    if (proto_is_igmp()) {
	set_proto_version_default(IGMP_VERSION_DEFAULT);
	_query_interval.set(TimeVal(IGMP_QUERY_INTERVAL, 0));
	_query_last_member_interval.set(
	    TimeVal(IGMP_LAST_MEMBER_QUERY_INTERVAL, 0));
	_query_response_interval.set(TimeVal(IGMP_QUERY_RESPONSE_INTERVAL, 0));
	_robust_count = IGMP_ROBUSTNESS_VARIABLE;
    }
#ifdef HAVE_IPV6_MULTICAST_ROUTING
    if (proto_is_mld6()) {
	set_proto_version_default(MLD_VERSION_DEFAULT);
	_query_interval.set(TimeVal(MLD_QUERY_INTERVAL, 0));
	_query_last_member_interval.set(
	    TimeVal(MLD_LAST_LISTENER_QUERY_INTERVAL, 0));
	_query_response_interval.set(TimeVal(MLD_QUERY_RESPONSE_INTERVAL, 0));
	_robust_count = MLD_ROBUSTNESS_VARIABLE;
    }
#endif
    set_proto_version(proto_version_default());
}

/**
 * Mld6igmpVif::~Mld6igmpVif:
 * @: 
 * 
 * MLD6IGMP protocol vif destructor.
 * 
 **/
Mld6igmpVif::~Mld6igmpVif()
{
    string error_msg;

    stop(error_msg);
    
    // Remove all members entries
    list<MemberQuery *>::iterator iter;
    for (iter = _members.begin(); iter != _members.end(); ++iter) {
	MemberQuery *member_query = *iter;
	join_prune_notify_routing(member_query->source(),
				  member_query->group(), ACTION_PRUNE);
	delete member_query;
    }
    _members.clear();
    
    BUFFER_FREE(_buffer_send);
}

/**
 * Mld6igmpVif::set_proto_version:
 * @proto_version: The protocol version to set.
 * 
 * Set protocol version.
 * 
 * Return value: %XORP_OK is @proto_version is valid, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::set_proto_version(int proto_version)
{
    if (proto_is_igmp()) {
	if ((proto_version < IGMP_VERSION_MIN)
	    || (proto_version > IGMP_VERSION_MAX))
	return (XORP_ERROR);
    }
    
#ifdef HAVE_IPV6_MULTICAST_ROUTING
    if (proto_is_mld6()) {
	if ((proto_version < MLD_VERSION_MIN)
	    || (proto_version > MLD_VERSION_MAX))
	return (XORP_ERROR);
    }
#endif // HAVE_IPV6_MULTICAST_ROUTING
    
    ProtoUnit::set_proto_version(proto_version);
    
    return (XORP_OK);
}

/**
 * Mld6igmpVif::proto_is_ssm:
 * @: 
 * 
 * Test if the interface is running a source-specific multicast capable
 * protocol version (e.g. IGMPv3 or MLDv2).
 * 
 * Return value: @true if the protocol version is source-specific multicast
 * capable, otherwise @fa.se
 **/
bool
Mld6igmpVif::proto_is_ssm() const
{
    if (proto_is_igmp())
	return (proto_version() >= 3);
    if (proto_is_mld6())
	return (proto_version() >= 2);
    
    return (false);
}

/**
 * Mld6igmpVif::start:
 * @error_msg: The error message (if error).
 * 
 * Start MLD or IGMP on a single virtual interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::start(string& error_msg)
{
    string dummy_error_msg;
    
    UNUSED(dummy_error_msg);

    if (! is_enabled())
	return (XORP_OK);

    if (is_up() || is_pending_up())
	return (XORP_OK);

    if (! is_underlying_vif_up()) {
	error_msg = "underlying vif is not UP";
	return (XORP_ERROR);
    }

    //
    // Start the vif only if it is of the appropriate type:
    // multicast-capable (loopback excluded).
    //
    if (! (is_multicast_capable() && (! is_loopback()))) {
	error_msg = "the interface is not multicast capable";
	return (XORP_ERROR);
    }

    if (update_primary_address(error_msg) < 0)
	return (XORP_ERROR);

    if (ProtoUnit::start() < 0) {
	error_msg = "internal error";
	return (XORP_ERROR);
    }

    // On startup, assume I am the MLD6IGMP Querier
    set_querier_addr(primary_addr());
    _proto_flags |= MLD6IGMP_VIF_QUERIER;
    
    //
    // Start the vif with the kernel
    //
    if (mld6igmp_node().start_protocol_kernel_vif(vif_index()) != XORP_OK) {
	error_msg = c_format("cannot start protocol vif %s with the kernel",
			     name().c_str());
	return (XORP_ERROR);
    }
    
    //
    // Join the appropriate multicast groups: ALL-SYSTEMS and ALL-ROUTERS
    //
    const IPvX group1 = IPvX::MULTICAST_ALL_SYSTEMS(family());
    const IPvX group2 = IPvX::MULTICAST_ALL_ROUTERS(family());
    if (mld6igmp_node().join_multicast_group(vif_index(), group1) != XORP_OK) {
	error_msg = c_format("cannot join group %s on vif %s",
			     cstring(group1), name().c_str());
	return (XORP_ERROR);
    }
    if (mld6igmp_node().join_multicast_group(vif_index(), group2) != XORP_OK) {
	error_msg = c_format("cannot join group %s on vif %s",
			     cstring(group2), name().c_str());
	return (XORP_ERROR);
    }
    
    //
    // Query all members on startup
    //
#ifdef HAVE_IPV4_MULTICAST_ROUTING
    if (proto_is_igmp()) {
	TimeVal scaled_max_resp_time =
	    (query_response_interval().get() * IGMP_TIMER_SCALE);
	mld6igmp_send(primary_addr(),
		      IPvX::MULTICAST_ALL_SYSTEMS(family()),
		      IGMP_MEMBERSHIP_QUERY,
		      is_igmpv1_mode() ? 0 : scaled_max_resp_time.sec(),
		      IPvX::ZERO(family()),
		      dummy_error_msg);
	_startup_query_count = MAX(robust_count().get() - 1, 0);
	TimeVal startup_query_interval = query_interval().get() / 4;
	_query_timer =
	    mld6igmp_node().eventloop().new_oneoff_after(
		startup_query_interval,
		callback(this, &Mld6igmpVif::query_timer_timeout));
    }
#endif // HAVE_IPV4_MULTICAST_ROUTING

#ifdef HAVE_IPV6_MULTICAST_ROUTING
    if (proto_is_mld6()) {
	TimeVal scaled_max_resp_time =
	    (query_response_interval().get() * MLD_TIMER_SCALE);
	mld6igmp_send(primary_addr(),
		      IPvX::MULTICAST_ALL_SYSTEMS(family()),
		      MLD_LISTENER_QUERY,
		      scaled_max_resp_time.sec(),
		      IPvX::ZERO(family()),
		      dummy_error_msg);
	_startup_query_count = MAX(robust_count().get() - 1, 0);
	TimeVal startup_query_interval = query_interval().get() / 4;
	_query_timer =
	    mld6igmp_node().eventloop().new_oneoff_after(
		startup_query_interval,
		callback(this, &Mld6igmpVif::query_timer_timeout));
    }
#endif // HAVE_IPV6_MULTICAST_ROUTING

    XLOG_INFO("Interface started: %s%s",
	      this->str().c_str(), flags_string().c_str());
    
    return (XORP_OK);
}

/**
 * Mld6igmpVif::stop:
 * @error_msg: The error message (if error).
 * 
 * Stop MLD or IGMP on a single virtual interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::stop(string& error_msg)
{
    int ret_value = XORP_OK;

    if (is_down())
	return (XORP_OK);

    if (! (is_up() || is_pending_up() || is_pending_down())) {
	error_msg = "the vif state is not UP or PENDING_UP or PENDING_DOWN";
	return (XORP_ERROR);
    }

    if (ProtoUnit::pending_stop() < 0) {
	error_msg = "internal error";
	ret_value = XORP_ERROR;
    }
    
    //
    // XXX: we don't have to explicitly leave the multicast groups
    // we have joined on that interface, because this will happen
    // automatically when we stop the vif through the MFEA.
    //

    if (ProtoUnit::stop() < 0) {
	error_msg = "internal error";
	ret_value = XORP_ERROR;
    }
    
    _proto_flags &= ~MLD6IGMP_VIF_QUERIER;
    set_querier_addr(IPvX::ZERO(family()));		// XXX: ANY
    _other_querier_timer.unschedule();
    _query_timer.unschedule();
    _igmpv1_router_present_timer.unschedule();
    _startup_query_count = 0;
    
    // Remove all members entries
    list<MemberQuery *>::iterator iter;
    for (iter = _members.begin(); iter != _members.end(); ++iter) {
	MemberQuery *member_query = *iter;
	join_prune_notify_routing(member_query->source(),
				  member_query->group(), ACTION_PRUNE);
	delete member_query;
    }
    _members.clear();
    
    //
    // Stop the vif with the kernel
    //
    if (mld6igmp_node().stop_protocol_kernel_vif(vif_index()) != XORP_OK) {
	XLOG_ERROR("Cannot stop protocol vif %s with the kernel",
		   name().c_str());
	ret_value = XORP_ERROR;
    }
    
    XLOG_INFO("Interface stopped: %s%s",
	      this->str().c_str(), flags_string().c_str());

    //
    // Inform the node that the vif has completed the shutdown
    //
    mld6igmp_node().vif_shutdown_completed(name());

    return (ret_value);
}

/**
 * Enable MLD/IGMP on a single virtual interface.
 * 
 * If an unit is not enabled, it cannot be start, or pending-start.
 */
void
Mld6igmpVif::enable()
{
    ProtoUnit::enable();

    XLOG_INFO("Interface enabled: %s%s",
	      this->str().c_str(), flags_string().c_str());
}

/**
 * Disable MLD/IGMP on a single virtual interface.
 * 
 * If an unit is disabled, it cannot be start or pending-start.
 * If the unit was runnning, it will be stop first.
 */
void
Mld6igmpVif::disable()
{
    string error_msg;

    stop(error_msg);
    ProtoUnit::disable();

    XLOG_INFO("Interface disabled: %s%s",
	      this->str().c_str(), flags_string().c_str());
}

/**
 * Mld6igmpVif::mld6igmp_send:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The MLD or IGMP type of the message.
 * @max_resp_time: The "Maximum Response Delay" or "Max Resp Time"
 * field in the MLD or IGMP headers respectively (in the particular protocol
 * resolution).
 * @group_address: The "Multicast Address" or "Group Address" field
 * in the MLD or IGMP headers respectively.
 * @error_msg: The error message (if error).
 * 
 * Send MLD or IGMP message.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::mld6igmp_send(const IPvX& src,
			   const IPvX& dst,
			   uint8_t message_type,
			   int max_resp_time,
			   const IPvX& group_address,
			   string& error_msg)
{
    uint16_t cksum;
    buffer_t *buffer;
    int ret_value;
    
    if (! (is_up() || is_pending_down())) {
	error_msg = c_format("vif %s is not UP", name().c_str());
	return (XORP_ERROR);
    }
    
    XLOG_ASSERT(src != IPvX::ZERO(family()));
    
    //
    // Prepare the MLD or IGMP header.
    //
    buffer = buffer_send_prepare();
    BUFFER_PUT_OCTET(message_type, buffer);	// The message type
    if (proto_is_igmp())
	BUFFER_PUT_OCTET(max_resp_time, buffer);
    else
	BUFFER_PUT_OCTET(0, buffer);		// XXX: Always 0 for MLD
    BUFFER_PUT_HOST_16(0, buffer);		// Zero the checksum field
    
    if (proto_is_mld6()) {
	BUFFER_PUT_HOST_16(max_resp_time, buffer);
	BUFFER_PUT_HOST_16(0, buffer);		// Reserved
    }
    BUFFER_PUT_IPVX(group_address, buffer);

    //
    // Compute the checksum
    //
    cksum = INET_CKSUM(BUFFER_DATA_HEAD(buffer), BUFFER_DATA_SIZE(buffer));
#ifdef HAVE_IPV6_MULTICAST_ROUTING
    // Add the checksum for the IPv6 pseudo-header
    if (proto_is_mld6()) {
	struct pseudo_header {
	    struct in6_addr in6_src;
	    struct in6_addr in6_dst;
	    uint32_t pkt_len;
	    uint32_t next_header;
	} pseudo_header;
	
	src.copy_out(pseudo_header.in6_src);
	dst.copy_out(pseudo_header.in6_dst);
	pseudo_header.pkt_len = ntohl(BUFFER_DATA_SIZE(buffer));
	pseudo_header.next_header = htonl(IPPROTO_ICMPV6);
	
	uint16_t cksum2 = INET_CKSUM(&pseudo_header, sizeof(pseudo_header));
	cksum = INET_CKSUM_ADD(cksum, cksum2);
    }
#endif // HAVE_IPV6_MULTICAST_ROUTING
    BUFFER_COPYPUT_INET_CKSUM(cksum, buffer, 2);	// XXX: the ckecksum
    
    XLOG_TRACE(mld6igmp_node().is_log_trace(), "TX %s from %s to %s",
	       proto_message_type2ascii(message_type),
	       cstring(src),
	       cstring(dst));
    
    //
    // Send the message
    //
    ret_value = mld6igmp_node().mld6igmp_send(vif_index(), src, dst,
					      MINTTL, -1, true, buffer,
					      error_msg);
    
    return (ret_value);
    
 buflen_error:
    XLOG_UNREACHABLE();
    error_msg = c_format("TX %s from %s to %s: "
			 "packet cannot fit into sending buffer",
			 proto_message_type2ascii(message_type),
			 cstring(src),
			 cstring(dst));
    XLOG_ERROR("%s", error_msg.c_str());
    return (XORP_ERROR);
}

/**
 * Mld6igmpVif::mld6igmp_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @ip_ttl: The IP TTL of the message. If it has a negative value,
 * it should be ignored.
 * @ip_tos: The IP TOS of the message. If it has a negative value,
 * it should be ignored.
 * @is_router_alert: True if the received IP packet had the Router Alert
 * IP option set.
 * @buffer: The buffer with the received message.
 * @error_msg: The error message (if error).
 * 
 * Receive MLD or IGMP message and pass it for processing.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::mld6igmp_recv(const IPvX& src,
			   const IPvX& dst,
			   int ip_ttl,
			   int ip_tos,
			   bool is_router_alert,
			   buffer_t *buffer,
			   string& error_msg)
{
    int ret_value = XORP_ERROR;
    
    if (! is_up()) {
	error_msg = c_format("vif %s is not UP", name().c_str());
	return (XORP_ERROR);
    }
    
    do {
	if (proto_is_igmp()) {
	    ret_value = igmp_process(src, dst, ip_ttl, ip_tos,
				     is_router_alert, buffer, error_msg);
	    break;
	}
#ifdef HAVE_IPV6_MULTICAST_ROUTING
	if (proto_is_mld6()) {
	    ret_value = mld6_process(src, dst, ip_ttl, ip_tos,
				     is_router_alert, buffer, error_msg);
	    break;
	}
#endif
	XLOG_UNREACHABLE();
	ret_value = XORP_ERROR;
	break;
    } while (false);
    
    return (ret_value);
}

/**
 * Mld6igmpVif::update_primary_address:
 * @error_msg: The error message (if error).
 * 
 * Update the primary address.
 * 
 * The primary address should be a link-local unicast address, and
 * is used for transmitting the multicast control packets on the LAN.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::update_primary_address(string& error_msg)
{
    bool i_was_querier = false;
    IPvX primary_a(IPvX::ZERO(family()));
    IPvX domain_wide_a(IPvX::ZERO(family()));

    //
    // Reset the primary address if it is not valid anymore.
    //
    if (Vif::find_address(primary_addr()) == NULL) {
	if (primary_addr() == querier_addr()) {
	    // Reset the querier address
	    set_querier_addr(IPvX::ZERO(family()));
	    _proto_flags &= ~MLD6IGMP_VIF_QUERIER;
	    i_was_querier = true;
	}
	set_primary_addr(IPvX::ZERO(family()));
    }

    list<VifAddr>::const_iterator iter;
    for (iter = addr_list().begin(); iter != addr_list().end(); ++iter) {
	const VifAddr& vif_addr = *iter;
	const IPvX& addr = vif_addr.addr();
	if (! addr.is_unicast())
	    continue;
	if (addr.is_linklocal_unicast()) {
	    if (primary_a.is_zero())
		primary_a = addr;
	    continue;
	}
	//
	// XXX: assume that everything else can be a domain-wide reachable
	// address.
	if (domain_wide_a.is_zero())
	    domain_wide_a = addr;
    }

    //
    // XXX: In case of IPv6 if there is no link-local address we may try
    // to use the the domain-wide address as a primary address,
    // but the MLD spec is clear that the MLD messages are to be originated
    // from a link-local address.
    // Hence, only in case of IPv4 we assign the domain-wide address
    // to the primary address.
    //
    if (is_ipv4()) {
	if (primary_a.is_zero())
	    primary_a = domain_wide_a;
    }

    //
    // Check that the interface has a primary address.
    //
    if (primary_addr().is_zero() && primary_a.is_zero()) {
	error_msg = "invalid primary address";
	return (XORP_ERROR);
    }
    
    if (primary_addr().is_zero())
	set_primary_addr(primary_a);

    if (i_was_querier) {
	// Assume again that I am the MLD6IGMP Querier
	set_querier_addr(primary_addr());
	_proto_flags |= MLD6IGMP_VIF_QUERIER;
    }

    return (XORP_OK);
}

/**
 * Mld6igmpVif::add_protocol:
 * @module_id: The #xorp_module_id of the protocol to add.
 * @module_instance_name: The module instance name of the protocol to add.
 * 
 * Add a protocol to the list of entries that would be notified if there
 * is membership change on this interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::add_protocol(xorp_module_id module_id,
			  const string& module_instance_name)
{
    if (find(_notify_routing_protocols.begin(),
	     _notify_routing_protocols.end(),
	     pair<xorp_module_id, string>(module_id, module_instance_name))
	!= _notify_routing_protocols.end()) {
	return (XORP_ERROR);		// Already added
    }
    
    _notify_routing_protocols.push_back(
	pair<xorp_module_id, string>(module_id, module_instance_name));
    
    return (XORP_OK);
}

/**
 * Mld6igmpVif::delete_protocol:
 * @module_id: The #xorp_module_id of the protocol to delete.
 * @module_instance_name: The module instance name of the protocol to delete.
 * 
 * Delete a protocol from the list of entries that would be notified if there
 * is membership change on this interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::delete_protocol(xorp_module_id module_id,
			     const string& module_instance_name)
{
    vector<pair<xorp_module_id, string> >::iterator iter;
    
    iter = find(_notify_routing_protocols.begin(),
		_notify_routing_protocols.end(),
		pair<xorp_module_id, string>(module_id, module_instance_name));
    
    if (iter == _notify_routing_protocols.end())
	return (XORP_ERROR);		// Not on the list
    
    _notify_routing_protocols.erase(iter);
    
    return (XORP_OK);
}

/**
 * Mld6igmpVif::is_igmpv1_mode:
 * @: 
 * 
 * Tests if the interface is running in IGMPv1 mode.
 * XXX: applies only to IGMP, and not to MLD.
 * 
 * Return value: true if the interface is running in IGMPv1 mode,
 * otherwise false.
 **/
bool
Mld6igmpVif::is_igmpv1_mode() const
{
    return (proto_is_igmp()
	    && ((proto_version() == IGMP_V1)
		|| _igmpv1_router_present_timer.scheduled()));
}

/**
 * Mld6igmpVif::proto_message_type2ascii:
 * @message_type: The protocol message type.
 * 
 * Return the ASCII text description of the protocol message.
 * 
 * Return value: The ASCII text descrpition of the protocol message.
 **/
const char *
Mld6igmpVif::proto_message_type2ascii(uint8_t message_type) const
{

    UNUSED(message_type);

#ifdef HAVE_IPV4_MULTICAST_ROUTING
    if (proto_is_igmp())
	return (IGMPTYPE2ASCII(message_type));
#endif

#ifdef HAVE_IPV6_MULTICAST_ROUTING
    if (proto_is_mld6())
	return (MLDTYPE2ASCII(message_type));
#endif
    
    return ("Unknown protocol message");
}

/**
 * Mld6igmpVif::buffer_send_prepare:
 * @: 
 * 
 * Reset and prepare the buffer for sending data.
 * 
 * Return value: The prepared buffer.
 **/
buffer_t *
Mld6igmpVif::buffer_send_prepare()
{
    BUFFER_RESET(_buffer_send);
    
    return (_buffer_send);
}

/**
 * Mld6igmpVif::join_prune_notify_routing:
 * @source: The source address of the (S,G) entry that has changed.
 * In case of group-specific membership, it could be NULL.
 * @group: The group address of the (S,G) entry that has changed.
 * @action_jp: The membership change: %ACTION_JOIN or %ACTION_PRUNE.
 * 
 * Notify the interested parties that there is membership change among
 * the local members.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::join_prune_notify_routing(const IPvX& source,
				       const IPvX& group,
				       action_jp_t action_jp) const
{
    vector<pair<xorp_module_id, string> >::const_iterator iter;
    for (iter = _notify_routing_protocols.begin();
	 iter != _notify_routing_protocols.end();
	 ++iter) {
	pair<xorp_module_id, string> my_pair = *iter;
	xorp_module_id module_id = my_pair.first;
	string module_instance_name = my_pair.second;

	if (mld6igmp_node().join_prune_notify_routing(module_instance_name,
						      module_id,
						      vif_index(),
						      source,
						      group,
						      action_jp) < 0) {
	    //
	    // TODO: remove <module_id, module_instance_name> ??
	    //
	}
    }
    
    return (XORP_OK);
}

// TODO: temporary here. Should go to the Vif class after the Vif
// class starts using the Proto class
string
Mld6igmpVif::flags_string() const
{
    string flags;
    
    if (is_up())
	flags += " UP";
    if (is_down())
	flags += " DOWN";
    if (is_pending_up())
	flags += " PENDING_UP";
    if (is_pending_down())
	flags += " PENDING_DOWN";
    if (is_ipv4())
	flags += " IPv4";
    if (is_ipv6())
	flags += " IPv6";
    if (is_enabled())
	flags += " ENABLED";
    if (is_disabled())
	flags += " DISABLED";
    
    return (flags);
}
