// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/mld6igmp/mld6igmp_vif.cc,v 1.16 2003/09/13 02:56:48 pavlin Exp $"


//
// MLD6IGMP virtual interfaces implementation.
//


#include "mld6igmp_module.h"
#include "mld6igmp_private.hh"
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
      _querier_addr(mld6igmp_node.family()),		// XXX: ANY
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
    }
#ifdef HAVE_IPV6_MULTICAST_ROUTING
    if (proto_is_mld6()) {
	set_proto_version_default(MLD_VERSION_DEFAULT);
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
    stop();
    
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
 * @: 
 * 
 * Start MLD or IGMP on a single virtual interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::start()
{
    if (! is_underlying_vif_up())
	return (XORP_ERROR);
    if (is_loopback())
	return (XORP_ERROR);
    if (! is_multicast_capable())
	return (XORP_ERROR);
    if (addr_ptr() == NULL)
	return (XORP_ERROR);
    if (! addr_ptr()->is_unicast())
	return (XORP_ERROR);
    
    if (ProtoUnit::start() < 0)
	return (XORP_ERROR);
    
    // On startup, assume I am the MLD6IGMP Querier
    _querier_addr = *addr_ptr(); // XXX: addr_ptr() is valid (tested above)
    _proto_flags |= MLD6IGMP_VIF_QUERIER;
    
    //
    // Start the vif with the kernel
    //
    if (mld6igmp_node().start_protocol_kernel_vif(vif_index()) != XORP_OK) {
	XLOG_ERROR("Error starting protocol vif %s with the kernel",
		   name().c_str());
	return (XORP_ERROR);
    }
    
    XLOG_INFO("STARTING %s%s",
	      this->str().c_str(), flags_string().c_str());
    
    //
    // Join the appropriate multicast groups: ALL-SYSTEMS and ALL-ROUTERS
    //
    const IPvX group1 = IPvX::MULTICAST_ALL_SYSTEMS(family());
    const IPvX group2 = IPvX::MULTICAST_ALL_ROUTERS(family());
    if (mld6igmp_node().join_multicast_group(vif_index(), group1) != XORP_OK) {
	XLOG_ERROR("Error joining group %s on vif %s",
		   cstring(group1), name().c_str());
	return (XORP_ERROR);
    }
    if (mld6igmp_node().join_multicast_group(vif_index(), group2) != XORP_OK) {
	XLOG_ERROR("Error joining group %s on vif %s",
		   cstring(group2), name().c_str());
	return (XORP_ERROR);
    }
    
    //
    // Query all members on startup
    //
    if (proto_is_igmp()) {
	mld6igmp_send(IPvX::MULTICAST_ALL_SYSTEMS(family()),
		      IGMP_MEMBERSHIP_QUERY,
		      is_igmpv1_mode() ?
		      0
		      : (IGMP_QUERY_RESPONSE_INTERVAL * IGMP_TIMER_SCALE),
		      IPvX::ZERO(family()));
	_startup_query_count = MAX(IGMP_ROBUSTNESS_VARIABLE - 1, 0);
	_query_timer =
	    mld6igmp_node().eventloop().new_oneoff_after(
		TimeVal(IGMP_STARTUP_QUERY_INTERVAL, 0),
		callback(this, &Mld6igmpVif::query_timer_timeout));
    }
#ifdef HAVE_IPV6_MULTICAST_ROUTING
    if (proto_is_mld6()) {
	mld6igmp_send(IPvX::MULTICAST_ALL_SYSTEMS(family()),
		      MLD_LISTENER_QUERY,
		      (MLD_QUERY_RESPONSE_INTERVAL * MLD_TIMER_SCALE),
		      IPvX::ZERO(family()));
	_startup_query_count = MAX(MLD_ROBUSTNESS_VARIABLE - 1, 0);
	_query_timer =
	    mld6igmp_node().eventloop().new_oneoff_after(
		TimeVal(MLD_STARTUP_QUERY_INTERVAL, 0),
		callback(this, &Mld6igmpVif::query_timer_timeout));
    }
#endif // HAVE_IPV6_MULTICAST_ROUTING
    
    return (XORP_OK);
}

/**
 * Mld6igmpVif::stop:
 * @: 
 * 
 * Stop MLD or IGMP on a single virtual interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::stop()
{
    int ret_value = XORP_OK;
    
    if (! (is_up() || is_pending_up() || is_pending_down()))
	return (XORP_ERROR);
    
    //
    // Leave the appropriate multicast groups: ALL-SYSTEMS and ALL-ROUTERS
    //
    const IPvX group1 = IPvX::MULTICAST_ALL_SYSTEMS(family());
    const IPvX group2 = IPvX::MULTICAST_ALL_ROUTERS(family());
    if (mld6igmp_node().leave_multicast_group(vif_index(), group1) != XORP_OK) {
	XLOG_ERROR("Error leaving group %s on vif %s",
		   cstring(group1), name().c_str());
    }
    if (mld6igmp_node().leave_multicast_group(vif_index(), group2) != XORP_OK) {
	XLOG_ERROR("Error leaving group %s on vif %s",
		   cstring(group2), name().c_str());
    }
    
    if (ProtoUnit::stop() < 0)
	ret_value = XORP_ERROR;
    
    _proto_flags &= ~MLD6IGMP_VIF_QUERIER;
    _querier_addr = IPvX(family());			// XXX: ANY
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
	XLOG_ERROR("Error stopping protocol vif %s with the kernel",
		   name().c_str());
	return (XORP_ERROR);
    }
    
    XLOG_INFO("STOPPED %s%s",
	      this->str().c_str(), flags_string().c_str());
    
    return (ret_value);
}

/**
 * Mld6igmpVif::mld6igmp_send:
 * @dst: The message destination address.
 * @message_type: The MLD or IGMP type of the message.
 * @max_resp_time: The "Maximum Response Delay" or "Max Resp Time"
 * field in the MLD or IGMP headers respectively (in the particular protocol
 * resolution).
 * @group_address: The "Multicast Address" or "Group Address" field
 * in the MLD or IGMP headers respectively.
 * 
 * Send MLD or IGMP message.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::mld6igmp_send(const IPvX& dst,
			   uint8_t message_type,
			   int max_resp_time,
			   const IPvX& group_address)
{
    uint16_t cksum;
    buffer_t *buffer;
    int ret_value;
    
    if (! (is_up() || is_pending_down()))
	return (XORP_ERROR);
    
    XLOG_ASSERT(addr_ptr() != NULL);
    
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
	
	addr_ptr()->copy_out(pseudo_header.in6_src);
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
	       cstring(*addr_ptr()),
	       cstring(dst));
    
    //
    // Send the message
    //
    ret_value = mld6igmp_node().mld6igmp_send(vif_index(), *addr_ptr(), dst,
					      MINTTL, -1, true, buffer);
    
    return (ret_value);
    
 buflen_error:
    XLOG_UNREACHABLE();
    XLOG_ERROR("TX %s from %s to %s: "
	       "packet cannot fit into sending buffer",
	       proto_message_type2ascii(message_type),
	       cstring(*addr_ptr()),
	       cstring(dst));
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
 * @router_alert_bool: True if the received IP packet had the Router Alert
 * IP option set.
 * @buffer: The buffer with the received message.
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
			   bool router_alert_bool,
			   buffer_t *buffer)
{
    int ret_value = XORP_ERROR;
    
    if (! is_up())
	return (XORP_ERROR);
    
    do {
	if (proto_is_igmp()) {
	    ret_value = igmp_process(src, dst, ip_ttl, ip_tos,
				     router_alert_bool, buffer);
	    break;
	}
#ifdef HAVE_IPV6_MULTICAST_ROUTING
	if (proto_is_mld6()) {
	    ret_value = mld6_process(src, dst, ip_ttl, ip_tos,
				     router_alert_bool, buffer);
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
    if (proto_is_igmp())
	return (IGMPTYPE2ASCII(message_type));
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
