// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net




//
// MLD6IGMP virtual interfaces implementation.
//


#include "mld6igmp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "libproto/checksum.h"

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
      _proto_flags(0),
      _primary_addr(IPvX::ZERO(mld6igmp_node.family())),
      _querier_addr(IPvX::ZERO(mld6igmp_node.family())),
      _startup_query_count(0),
      _group_records(*this),
      _ip_router_alert_option_check(false),
      _configured_query_interval(
	  TimeVal(0, 0),
	  callback(this, &Mld6igmpVif::set_configured_query_interval_cb)),
      _effective_query_interval(TimeVal(0, 0)),
      _query_last_member_interval(
	  TimeVal(0, 0),
	  callback(this, &Mld6igmpVif::set_query_last_member_interval_cb)),
      _query_response_interval(
	  TimeVal(0, 0),
	  callback(this, &Mld6igmpVif::set_query_response_interval_cb)),
      _configured_robust_count(
	  0,
	  callback(this, &Mld6igmpVif::set_configured_robust_count_cb)),
      _effective_robustness_variable(0),
      _last_member_query_count(0),
      _group_membership_interval(TimeVal(0, 0)),
      _last_member_query_time(TimeVal(0, 0)),
      _older_version_host_present_interval(TimeVal(0, 0)),
      _dummy_flag(false)
{
    XLOG_ASSERT(proto_is_igmp() || proto_is_mld6());
    
    //
    // TODO: when more things become classes, most of this init should go away
    //
    _buffer_send = BUFFER_MALLOC(BUF_SIZE_DEFAULT);

    //
    // Set the protocol version
    //
    if (proto_is_igmp()) {
	set_proto_version_default(IGMP_VERSION_DEFAULT);
	_configured_query_interval.set(TimeVal(IGMP_QUERY_INTERVAL, 0));
	_query_last_member_interval.set(
	    TimeVal(IGMP_LAST_MEMBER_QUERY_INTERVAL, 0));
	_query_response_interval.set(TimeVal(IGMP_QUERY_RESPONSE_INTERVAL, 0));
	_configured_robust_count.set(IGMP_ROBUSTNESS_VARIABLE);
    }

    if (proto_is_mld6()) {
	set_proto_version_default(MLD_VERSION_DEFAULT);
	_configured_query_interval.set(TimeVal(MLD_QUERY_INTERVAL, 0));
	_query_last_member_interval.set(
	    TimeVal(MLD_LAST_LISTENER_QUERY_INTERVAL, 0));
	_query_response_interval.set(TimeVal(MLD_QUERY_RESPONSE_INTERVAL, 0));
	_configured_robust_count.set(MLD_ROBUSTNESS_VARIABLE);
    }

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
    _group_records.delete_payload_and_clear();
    
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
	    || (proto_version > IGMP_VERSION_MAX)) {
	    return (XORP_ERROR);
	}
	if (proto_version < IGMP_V3) {
	    //
	    // XXX: Restore the variables that might have been adopted from
	    // the Querier.
	    //
	    restore_effective_variables();
	}
    }
    
    if (proto_is_mld6()) {
	if ((proto_version < MLD_VERSION_MIN)
	    || (proto_version > MLD_VERSION_MAX)) {
	    return (XORP_ERROR);
	}
	if (proto_version < IGMP_V3) {
	    //
	    // XXX: Restore the variables that might have been adopted from
	    // the Querier.
	    //
	    restore_effective_variables();
	}
    }
    
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
	return (proto_version() >= IGMP_V3);
    if (proto_is_mld6())
	return (proto_version() >= MLD_V2);
    
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

    if (update_primary_address(error_msg) != XORP_OK)
	return (XORP_ERROR);

    if (ProtoUnit::start() != XORP_OK) {
	error_msg = "internal error";
	return (XORP_ERROR);
    }

    // On startup, assume I am the MLD6IGMP Querier
    set_querier_addr(primary_addr());
    set_i_am_querier(true);
    
    //
    // Register as a receiver with the kernel
    //
    if (mld6igmp_node().register_receiver(name(),
					  name(),
					  mld6igmp_node().ip_protocol_number(),
					  true)
	!= XORP_OK) {
	error_msg = c_format("cannot register as a receiver on vif %s "
			     "with the kernel",
			     name().c_str());
	return (XORP_ERROR);
    }
    
    //
    // Join the appropriate multicast groups: ALL-SYSTEMS, ALL-ROUTERS,
    // and SSM-ROUTERS.
    //
    list<IPvX> groups;
    list<IPvX>::iterator groups_iter;
    groups.push_back(IPvX::MULTICAST_ALL_SYSTEMS(family()));
    groups.push_back(IPvX::MULTICAST_ALL_ROUTERS(family()));
    groups.push_back(IPvX::SSM_ROUTERS(family()));
    for (groups_iter = groups.begin();
	 groups_iter != groups.end();
	 ++groups_iter) {
	const IPvX& group = *groups_iter;
	if (mld6igmp_node().join_multicast_group(name(),
						 name(),
						 mld6igmp_node().ip_protocol_number(),
						 group)
	    != XORP_OK) {
	    error_msg = c_format("cannot join group %s on vif %s",
				 cstring(group), name().c_str());
	    return (XORP_ERROR);
	}
    }
    
    //
    // Query all members on startup
    //
    TimeVal max_resp_time = query_response_interval().get();
    set<IPvX> no_sources;		// XXX: empty set
    mld6igmp_query_send(primary_addr(),
			IPvX::MULTICAST_ALL_SYSTEMS(family()),
			max_resp_time,
			IPvX::ZERO(family()),
			no_sources,
			false,
			dummy_error_msg);
    _startup_query_count = effective_robustness_variable();
    if (_startup_query_count > 0)
	_startup_query_count--;
    TimeVal startup_query_interval = effective_query_interval() / 4;
    _query_timer = mld6igmp_node().eventloop().new_oneoff_after(
	startup_query_interval,
	callback(this, &Mld6igmpVif::query_timer_timeout));

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

    if (ProtoUnit::pending_stop() != XORP_OK) {
	error_msg = "internal error";
	ret_value = XORP_ERROR;
    }
    
    //
    // XXX: we don't have to explicitly leave the multicast groups
    // we have joined on that interface, because this will happen
    // automatically when we stop the vif through the MFEA.
    //

    if (ProtoUnit::stop() != XORP_OK) {
	error_msg = "internal error";
	ret_value = XORP_ERROR;
    }
    
    set_i_am_querier(false);
    set_querier_addr(IPvX::ZERO(family()));		// XXX: ANY
    _other_querier_timer.unschedule();
    _query_timer.unschedule();
    _startup_query_count = 0;
    
    // Notify routing and remove all group records
    Mld6igmpGroupSet::const_iterator group_iter;
    for (group_iter = _group_records.begin();
	 group_iter != _group_records.end(); ++group_iter) {
	const Mld6igmpGroupRecord *group_record = group_iter->second;
	Mld6igmpSourceSet::const_iterator source_iter;
	// Clear the state for all included sources
	for (source_iter = group_record->do_forward_sources().begin();
	     source_iter != group_record->do_forward_sources().end();
	     ++source_iter) {
	    const Mld6igmpSourceRecord *source_record = source_iter->second;
	    join_prune_notify_routing(source_record->source(),
				      group_record->group(),
				      ACTION_PRUNE);
	}
	// Clear the state for all excluded sources
	for (source_iter = group_record->dont_forward_sources().begin();
	     source_iter != group_record->dont_forward_sources().end();
	     ++source_iter) {
	    const Mld6igmpSourceRecord *source_record = source_iter->second;
	    join_prune_notify_routing(source_record->source(),
				      group_record->group(),
				      ACTION_JOIN);
	}
	if (group_record->is_exclude_mode()) {
	    join_prune_notify_routing(IPvX::ZERO(family()),
				      group_record->group(), ACTION_PRUNE);
	}
    }
    _group_records.delete_payload_and_clear();
    
    //
    // Unregister as a receiver with the kernel
    //
    if (mld6igmp_node().unregister_receiver(name(),
					    name(),
					    mld6igmp_node().ip_protocol_number())
	!= XORP_OK) {
	XLOG_ERROR("Cannot unregister as a receiver on vif %s with the kernel",
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
 * @max_resp_code: The "Maximum Response Code" or "Max Resp Code"
 * field in the MLD or IGMP headers respectively (in the particular
 * protocol resolution).
 * @group_address: The "Multicast Address" or "Group Address" field
 * in the MLD or IGMP headers respectively.
 * @buffer: The buffer with the rest of the message.
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
			   uint16_t max_resp_code,
			   const IPvX& group_address,
			   buffer_t *buffer,
			   string& error_msg)
{
    uint16_t cksum;
    int ret_value;
    size_t datalen;
    bool ip_router_alert = true;	// XXX: always use Router Alert option
    bool ip_internet_control = true;	// XXX: always true
    
    if (! (is_up() || is_pending_down())) {
	error_msg = c_format("vif %s is not UP", name().c_str());
	return (XORP_ERROR);
    }
    
    XLOG_ASSERT(src != IPvX::ZERO(family()));
    
    //
    // Prepare the MLD or IGMP header.
    //
    // Point the buffer to the protocol header
    datalen = BUFFER_DATA_SIZE(buffer);
    if (datalen > 0) {
	BUFFER_RESET_TAIL(buffer);
    }
    if (proto_is_igmp()) {
	BUFFER_PUT_OCTET(message_type, buffer);	// The message type
	BUFFER_PUT_OCTET(max_resp_code, buffer);
	BUFFER_PUT_HOST_16(0, buffer);		// Zero the checksum field
	BUFFER_PUT_IPVX(group_address, buffer);
    }
    if (proto_is_mld6()) {
	BUFFER_PUT_OCTET(message_type, buffer);	// The message type
	BUFFER_PUT_OCTET(0, buffer);		// XXX: Always 0 for MLD
	BUFFER_PUT_HOST_16(0, buffer);		// Zero the checksum field
	BUFFER_PUT_HOST_16(max_resp_code, buffer);
	BUFFER_PUT_HOST_16(0, buffer);		// Reserved
	BUFFER_PUT_IPVX(group_address, buffer);
    }
    // Restore the buffer to include the data
    if (datalen > 0) {
	BUFFER_RESET_TAIL(buffer);
	BUFFER_PUT_SKIP(datalen, buffer);
    }

    //
    // Compute the checksum
    //
    cksum = inet_checksum(BUFFER_DATA_HEAD(buffer), BUFFER_DATA_SIZE(buffer));
#ifdef HAVE_IPV6
    // Add the checksum for the IPv6 pseudo-header
    if (proto_is_mld6()) {
	uint16_t cksum2;
	size_t ph_len = BUFFER_DATA_SIZE(buffer);
	cksum2 = calculate_ipv6_pseudo_header_checksum(src, dst, ph_len,
						       IPPROTO_ICMPV6);
	cksum = inet_checksum_add(cksum, cksum2);
    }
#endif // HAVE_IPV6
    BUFFER_COPYPUT_INET_CKSUM(cksum, buffer, 2);	// XXX: the checksum
    
    XLOG_TRACE(mld6igmp_node().is_log_trace(), "TX %s from %s to %s",
	       proto_message_type2ascii(message_type),
	       cstring(src),
	       cstring(dst));
    
    //
    // Send the message
    //
    ret_value = mld6igmp_node().mld6igmp_send(name(), name(),
					      src, dst,
					      mld6igmp_node().ip_protocol_number(),
					      MINTTL, -1,
					      ip_router_alert,
					      ip_internet_control,
					      buffer, error_msg);
    
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
 * Send Group-Specific Query message.
 *
 * @param group_address the "Multicast Address" or "Group Address" field
 * in the MLD or IGMP headers respectively.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 **/
int
Mld6igmpVif::mld6igmp_group_query_send(const IPvX& group_address,
				       string& error_msg)
{
    const IPvX& src = primary_addr();
    const IPvX& dst = group_address;
    const TimeVal& max_resp_time = query_last_member_interval().get();
    Mld6igmpGroupRecord* group_record = NULL;
    set<IPvX> no_sources;		// XXX: empty set
    int ret_value;

    //
    // Only the Querier should originate Query messages
    //
    if (! i_am_querier())
	return (XORP_OK);

    // Find the group record
    group_record = _group_records.find_group_record(group_address);
    if (group_record == NULL)
	return (XORP_ERROR);		// No such group

    //
    // Lower the group timer
    //
    _group_records.lower_group_timer(group_address, last_member_query_time());

    //
    // Send the message
    //
    ret_value = mld6igmp_query_send(src, dst, max_resp_time, group_address,
				    no_sources,
				    false,	// XXX: reset the s_flag 
				    error_msg);

    //
    // Schedule the periodic Group-Specific Query
    //
    if (ret_value == XORP_OK)
	group_record->schedule_periodic_group_query(no_sources);

    //
    // Print the error message if there was an error
    //
    if (ret_value != XORP_OK) {
	XLOG_ERROR("Error sending Group-Specific query for %s: %s",
		   cstring(group_address), error_msg.c_str());
    }

    return (ret_value);
}

/**
 * Send MLDv2 or IGMPv3 Group-and-Source-Specific Query message.
 *
 * @param group_address the "Multicast Address" or "Group Address" field
 * in the MLD or IGMP headers respectively.
 * @param sources the set of source addresses.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 **/
int
Mld6igmpVif::mld6igmp_group_source_query_send(const IPvX& group_address,
					      const set<IPvX>& sources,
					      string& error_msg)
{
    const IPvX& src = primary_addr();
    const IPvX& dst = group_address;
    const TimeVal& max_resp_time = query_last_member_interval().get();
    Mld6igmpGroupRecord* group_record = NULL;
    set<IPvX> selected_sources;
    set<IPvX>::const_iterator source_iter;
    int ret_value;

    //
    // Only the Querier should originate Query messages
    //
    if (! i_am_querier())
	return (XORP_OK);

    if (sources.empty())
	return (XORP_OK);		// No sources to query

    // Find the group record
    group_record = _group_records.find_group_record(group_address);
    if (group_record == NULL)
	return (XORP_ERROR);		// No such group

    //
    // Select only the sources with source timer larger than the
    // Last Member Query Time.
    //
    for (source_iter = sources.begin();
	 source_iter != sources.end();
	 ++source_iter) {
	const IPvX& ipvx = *source_iter;
	Mld6igmpSourceRecord* source_record;
	source_record = group_record->find_do_forward_source(ipvx);
	if (source_record == NULL)
	    continue;

	TimeVal timeval_remaining;
	source_record->source_timer().time_remaining(timeval_remaining);
	if (timeval_remaining <= last_member_query_time())
	    continue;
	selected_sources.insert(ipvx);
    }
    if (selected_sources.empty())
	return (XORP_OK);		// No selected sources to query

    //
    // Lower the source timer
    //
    group_record->lower_source_timer(selected_sources,
				     last_member_query_time());

    //
    // Send the message
    //
    ret_value = mld6igmp_query_send(src, dst, max_resp_time, group_address,
				    selected_sources,
				    false,	// XXX: reset the s_flag 
				    error_msg);

    //
    // Schedule the periodic Group-and-Source-Specific Query
    //
    if (ret_value == XORP_OK)
	group_record->schedule_periodic_group_query(selected_sources);

    //
    // Print the error message if there was an error
    //
    if (ret_value != XORP_OK) {
	XLOG_ERROR("Error sending Group-and-Source-Specific query for %s: %s",
		   cstring(group_address), error_msg.c_str());
    }

    return (ret_value);

}

/**
 * Send MLD or IGMP Query message.
 *
 * @param src the message source address.
 * @param dst the message destination address.
 * @param max_resp_time the maximum response time.
 * @param group_address the "Multicast Address" or "Group Address" field
 * in the MLD or IGMP headers respectively.
 * @param sources the set of source addresses (for IGMPv3 or MLDv2 only).
 * @param s_flag the "Suppress Router-Side Processing" bit (for IGMPv3
 * or MLDv2 only; in all other cases it should be set to false).
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 **/
int
Mld6igmpVif::mld6igmp_query_send(const IPvX& src,
				 const IPvX& dst,
				 const TimeVal& max_resp_time,
				 const IPvX& group_address,
				 const set<IPvX>& sources,
				 bool s_flag,
				 string& error_msg)
{
    buffer_t *buffer;
    uint32_t timer_scale = mld6igmp_constant_timer_scale();
    TimeVal scaled_max_resp_time = max_resp_time * timer_scale;
    set<IPvX>::const_iterator source_iter;
    uint8_t qrv, qqic;
    size_t max_sources_n;
    size_t max_payload = 0;
    Mld6igmpGroupRecord* group_record = NULL;

    //
    // Only the Querier should originate Query messages
    //
    if (! i_am_querier())
	return (XORP_OK);

    // Find the group record
    group_record = _group_records.find_group_record(group_address);

    //
    // Check protocol version and Query message matching
    //
    do {
	if (sources.empty())
	    break;

	// IGMPv3/MLDv2 Query(G, A) message
	if (is_igmpv3_mode(group_record) || is_mldv2_mode(group_record))
	    break;

	// XXX: Query(G, A) messages are not allowed in this mode
	return (XORP_ERROR);	
    } while (false);

    //
    // Lower the group and source timers (if necessary)
    //
    if (! s_flag) {
	if (sources.empty()) {
	    // XXX: Q(G) Query
	    _group_records.lower_group_timer(group_address,
					     last_member_query_time());
	} else {
	    // XXX: Q(G, A) Query
	    _group_records.lower_source_timer(group_address, sources,
					      last_member_query_time());
	}
    }

    //
    // Prepare the data after the Query header
    //
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // | Resv  |S| QRV |     QQIC      |     Number of Sources (N)     |
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    qrv = 0;
    if (effective_robustness_variable() <= 0x7)
	qrv = effective_robustness_variable();
    if (s_flag)
	qrv |= 0x8;
    qqic = 0;
    encode_exp_time_code8(effective_query_interval(), qqic, 1);

    //
    // Calculate the maximum number of sources
    //
    max_sources_n = sources.size();
    if (proto_is_igmp()) {
	max_payload = mtu()		// The MTU of the vif
	    - (0xf << 2)		// IPv4 max header size
	    - 4				// IPv4 Router Alert option
	    - IGMP_V3_QUERY_MINLEN;	// IGMPv3 Query pre-source fields
    }
    if (proto_is_mld6()) {
	max_payload = mtu()		// The MTU of the vif
	    - 8		// IPv6 Hop-by-hop Ext. Header with Router Alert option
	    - MLD_V2_QUERY_MINLEN;	// MLDv2 Query pre-source fields
    }
    max_sources_n = min(max_sources_n, max_payload / IPvX::addr_bytelen(family()));

    //
    // XXX: According to RFC 3810 (MLDv2), Section 8.3.2, the Querier
    // continues to send MLDv2 queries, regardless of its Multicast Address
    // Compatibility Mode.
    //
    // Interestingly, RFC 3376 (IGMPv3) does not include this statement.
    // According to the following email, the need for this statement has been
    // discovered too late to be included in RFC 3376:
    // http://www1.ietf.org/mail-archive/web/ssm/current/msg00084.html
    //

    //
    // Prepare the buffer
    //
    buffer = buffer_send_prepare();
    BUFFER_PUT_SKIP(mld6igmp_constant_minlen(), buffer);

    //
    // Insert the data (for IGMPv3 and MLDv2 only)
    //
    if (is_igmpv3_mode() || is_mldv2_mode()) {
	//
	// XXX: Note that we consider only the interface mode, but ignore
	// the Multicast Address Compatibility Mode.
	//
	BUFFER_PUT_OCTET(qrv, buffer);
	BUFFER_PUT_OCTET(qqic, buffer);
	BUFFER_PUT_HOST_16(max_sources_n, buffer);
	source_iter = sources.begin();
	while (max_sources_n > 0) {
	    const IPvX& ipvx = *source_iter;
	    BUFFER_PUT_IPVX(ipvx, buffer);
	    ++source_iter;
	    max_sources_n--;
	}
    } else {
	//
	// If IGMPv1 Multicast Address Compatibility Mode, then set the
	// Max Response Time to zero.
	//
	if (is_igmpv1_mode(group_record))
	    scaled_max_resp_time = TimeVal::ZERO();
    }

    //
    // Send the message
    //
    return (mld6igmp_send(src, dst,
			  mld6igmp_constant_membership_query(),
			  scaled_max_resp_time.sec(),
			  group_address, buffer, error_msg));

 buflen_error:
    XLOG_UNREACHABLE();
    XLOG_ERROR("INTERNAL mld6igmp_query_send() ERROR: "
	       "buffer size too small");

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
 * @ip_router_alert: True if the received IP packet had the Router Alert
 * IP option set.
 * @ip_internet_control: If true, then this is IP control traffic.
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
			   bool ip_router_alert,
			   bool ip_internet_control,
			   buffer_t *buffer,
			   string& error_msg)
{
    int ret_value = XORP_ERROR;
    
    if (! is_up()) {
	error_msg = c_format("vif %s is not UP", name().c_str());
	return (XORP_ERROR);
    }
    
    ret_value = mld6igmp_process(src, dst, ip_ttl, ip_tos, ip_router_alert,
				 ip_internet_control, buffer, error_msg);
    
    return (ret_value);
}

/**
 * Mld6igmpVif::mld6igmp_process:
 * @src: The message source address.
 * @dst: The message destination address.
 * @ip_ttl: The IP TTL of the message. If it has a negative value,
 * it should be ignored.
 * @ip_tos: The IP TOS of the message. If it has a negative value,
 * it should be ignored.
 * @ip_router_alert: True if the received IP packet had the Router Alert
 * IP option set.
 * @ip_internet_control: If true, then this is IP control traffic.
 * @buffer: The buffer with the message.
 * @error_msg: The error message (if error).
 * 
 * Process MLD or IGMP message and pass the control to the type-specific
 * functions.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::mld6igmp_process(const IPvX& src,
			      const IPvX& dst,
			      int ip_ttl,
			      int ip_tos,
			      bool ip_router_alert,
			      bool ip_internet_control,
			      buffer_t *buffer,
			      string& error_msg)
{
    uint8_t message_type = 0;
    uint16_t max_resp_code = 0;
    IPvX group_address(family());
    uint16_t cksum;
    bool check_router_alert_option = false;
    bool check_src_linklocal_unicast = false;
    bool allow_src_zero_address = false;
    bool check_dst_multicast = false;
    bool check_group_interfacelocal_multicast = false;
    bool decode_extra_fields = false;
    
    //
    // Message length check.
    //
    if (BUFFER_DATA_SIZE(buffer) < mld6igmp_constant_minlen()) {
	error_msg = c_format("RX packet from %s to %s on vif %s: "
			     "too short data field (%u octets)",
			     cstring(src), cstring(dst),
			     name().c_str(),
			     XORP_UINT_CAST(BUFFER_DATA_SIZE(buffer)));
	XLOG_WARNING("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    //
    // Checksum verification.
    //
    cksum = inet_checksum(BUFFER_DATA_HEAD(buffer), BUFFER_DATA_SIZE(buffer));
#ifdef HAVE_IPV6
    // Add the checksum for the IPv6 pseudo-header
    if (proto_is_mld6()) {
	uint16_t cksum2;
	size_t ph_len = BUFFER_DATA_SIZE(buffer);
	cksum2 = calculate_ipv6_pseudo_header_checksum(src, dst, ph_len,
						       IPPROTO_ICMPV6);
	cksum = inet_checksum_add(cksum, cksum2);
    }
#endif // HAVE_IPV6
    if (cksum) {
	error_msg = c_format("RX packet from %s to %s on vif %s: "
			     "checksum error",
			     cstring(src), cstring(dst),
			     name().c_str());
	XLOG_WARNING("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    //
    // Protocol version check.
    //
    // XXX: MLD and IGMP messages do not have an explicit field for protocol
    // version. Protocol version check is performed later, per (some) message
    // type.
    //
    
    //
    // Get the message type and the max. resp. time (in case of IGMP).
    //
    // Note that in case of IGMP the max. resp. time is the `igmp_code' field
    // in `struct igmp'.
    //
    if (proto_is_igmp()) {
	BUFFER_GET_OCTET(message_type, buffer);
	BUFFER_GET_OCTET(max_resp_code, buffer);
	BUFFER_GET_SKIP(2, buffer);		// The checksum
    }
    if (proto_is_mld6()) {
	BUFFER_GET_OCTET(message_type, buffer);
	BUFFER_GET_SKIP(1, buffer);		// The `Code' field: unused
	BUFFER_GET_SKIP(2, buffer);		// The `Checksum' field
    }

    XLOG_TRACE(mld6igmp_node().is_log_trace(),
	       "RX %s from %s to %s on vif %s",
	       proto_message_type2ascii(message_type),
	       cstring(src), cstring(dst),
	       name().c_str());

    //
    // Ignore messages that are not recognized by older protocol version.
    //
    // XXX: Unrecognized message types MUST be silently ignored.
    //
    if (proto_is_igmp()) {
	switch (message_type) {
	case IGMP_MEMBERSHIP_QUERY:
	    // Recognized by IGMPv1, IGMPv2, IGMPv3
	    break;
	case IGMP_V1_MEMBERSHIP_REPORT:
	    // Recognized by IGMPv1, IGMPv2, IGMPv3
	    break;
	case IGMP_V2_MEMBERSHIP_REPORT:
	    // Recognized by IGMPv2, IGMPv3
	    if (is_igmpv1_mode())
		return (XORP_ERROR);
	    break;
	case IGMP_V2_LEAVE_GROUP:
	    // Recognized by IGMPv2, IGMPv3
	    if (is_igmpv1_mode())
		return (XORP_ERROR);
	    break;
	case IGMP_V3_MEMBERSHIP_REPORT:
	    // Recognized by IGMPv3
	    if (is_igmpv1_mode() || is_igmpv2_mode())
		return (XORP_ERROR);
	    break;
	case IGMP_DVMRP:
	case IGMP_MTRACE:
	    break;
	default:
	    // Unrecognized message
	    return (XORP_ERROR);
	}
    }
    if (proto_is_mld6()) {
	switch (message_type) {
	case MLD_LISTENER_QUERY:
	    // Recognized by MLDv1, MLDv2
	    break;
	case MLD_LISTENER_REPORT:
	    // Recognized by MLDv1, MLDv2
	    break;
	case MLD_LISTENER_DONE:
	    // Recognized by MLDv1, MLDv2
	    break;
	case MLDV2_LISTENER_REPORT:
	    // Recognized by MLDv2
	    if (is_mldv1_mode())
		return (XORP_ERROR);
	    break;
	case MLD_MTRACE:
	    break;
	default:
	    // Unrecognized message
	    return (XORP_ERROR);
	}
    }

    //
    // Assign various flags what needs to be checked, based on the
    // message type:
    //  - check_router_alert_option
    //  - check_src_linklocal_unicast
    //  - allow_src_zero_address
    //  - check_dst_multicast
    //  - check_group_interfacelocal_multicast
    //  - decode_extra_fields
    //
    if (proto_is_igmp()) {
	switch (message_type) {
	case IGMP_MEMBERSHIP_QUERY:
	case IGMP_V1_MEMBERSHIP_REPORT:
	case IGMP_V2_MEMBERSHIP_REPORT:
	case IGMP_V2_LEAVE_GROUP:
	case IGMP_V3_MEMBERSHIP_REPORT:
	    if (_ip_router_alert_option_check.get())
		check_router_alert_option = true;
	    check_src_linklocal_unicast = false;	// Not needed for IPv4
	    if (is_igmpv3_mode()) {
		if ((message_type == IGMP_V1_MEMBERSHIP_REPORT)
		    || (message_type == IGMP_V2_MEMBERSHIP_REPORT)
		    || (message_type == IGMP_V3_MEMBERSHIP_REPORT)) {
		    allow_src_zero_address = true;	// True only for IGMPv3
		}
	    }
	    check_dst_multicast = true;
	    if (is_igmpv3_mode())
		check_dst_multicast = false;		// XXX: disable
	    check_group_interfacelocal_multicast = false;// Not needed for IPv4
	    decode_extra_fields = true;
	    if (message_type == IGMP_V3_MEMBERSHIP_REPORT)
		decode_extra_fields = false;
	    break;
	case IGMP_DVMRP:
	case IGMP_MTRACE:
	    // TODO: Assign the flags as appropriate
	    break;
	default:
	    break;
	}
    }

    if (proto_is_mld6()) {
	switch (message_type) {
	case MLD_LISTENER_QUERY:
	case MLD_LISTENER_REPORT:
	case MLD_LISTENER_DONE:
	case MLDV2_LISTENER_REPORT:
	    check_router_alert_option = true;
	    check_src_linklocal_unicast = true;
	    allow_src_zero_address = false;		// Always false for MLD
	    check_dst_multicast = true;
	    if (is_mldv2_mode())
		check_dst_multicast = false;		// XXX: disable
	    check_group_interfacelocal_multicast = true;
	    decode_extra_fields = true;
	    if (message_type == MLDV2_LISTENER_REPORT)
		decode_extra_fields = false;
	    break;
	case MLD_MTRACE:
	    // TODO: Assign the flags as appropriate
	    break;
	default:
	    break;
	}
    }

    //
    // Decode the extra fields: the max. resp. time (in case of MLD),
    // and the group address.
    //
    if (decode_extra_fields) {
	if (proto_is_igmp()) {
	    BUFFER_GET_IPVX(family(), group_address, buffer);
	}
	if (proto_is_mld6()) {
	    BUFFER_GET_HOST_16(max_resp_code, buffer);
	    BUFFER_GET_SKIP(2, buffer);		// The `Reserved' field
	    BUFFER_GET_IPVX(family(), group_address, buffer);
	}
    }

    //
    // IP Router Alert option check.
    //
    if (check_router_alert_option && (! ip_router_alert)) {
	error_msg = c_format("RX %s from %s to %s on vif %s: "
			     "missing IP Router Alert option",
			     proto_message_type2ascii(message_type),
			     cstring(src), cstring(dst),
			     name().c_str());
	XLOG_WARNING("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    //
    // TODO: check the TTL, TOS and ip_internet_control flag if we are
    // running in secure mode.
    //
    UNUSED(ip_ttl);
    UNUSED(ip_tos);
    UNUSED(ip_internet_control);
#if 0
    if (ip_ttl != MINTTL) {
	error_msg = c_format("RX %s from %s to %s on vif %s: "
			     "ip_ttl = %d instead of %d",
			     proto_message_type2ascii(message_type),
			     cstring(src), cstring(dst),
			     name().c_str(),
			     ip_ttl, MINTTL);
	XLOG_WARNING("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
#endif // 0
    
    //
    // Source address check.
    //
    if (! (src.is_unicast() || (allow_src_zero_address && src.is_zero()))) {
	//
	// Source address must always be unicast.
	// The kernel should have checked that, but just in case...
	//
	error_msg = c_format("RX %s from %s to %s on vif %s: "
			     "source must be unicast",
			     proto_message_type2ascii(message_type),
			     cstring(src), cstring(dst),
			     name().c_str());
	XLOG_WARNING("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    if (src.af() != family()) {
	// Invalid source address family
	XLOG_WARNING("RX %s from %s to %s on vif %s: "
		     "invalid source address family "
		     "(received %d expected %d)",
		     proto_message_type2ascii(message_type),
		     cstring(src), cstring(dst),
		     name().c_str(),
		     src.af(), family());
    }
    // Source address must be directly connected
    if (! (mld6igmp_node().is_directly_connected(*this, src)
	   || (allow_src_zero_address && src.is_zero()))) {
	error_msg = c_format("RX %s from %s to %s on vif %s: "
			     "source must be directly connected",
			     proto_message_type2ascii(message_type),
			     cstring(src), cstring(dst),
			     name().c_str());
	XLOG_WARNING("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    if (check_src_linklocal_unicast) {
	if (src.is_linklocal_unicast()
	    || (allow_src_zero_address && src.is_zero())) {
	    // The source address is link-local or (allowed) zero address
	} else {
	    // The source address is not link-local
	    error_msg = c_format("RX %s from %s to %s on vif %s: "
				 "source is not a link-local address",
				 proto_message_type2ascii(message_type),
				 cstring(src), cstring(dst),
				 name().c_str());
	    XLOG_WARNING("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
    }

    //
    // Destination address check.
    //
    if (dst.af() != family()) {
	// Invalid destination address family
	XLOG_WARNING("RX %s from %s to %s on vif %s: "
		     "invalid destination address family "
		     "(received %d expected %d)",
		     proto_message_type2ascii(message_type),
		     cstring(src), cstring(dst),
		     name().c_str(),
		     dst.af(), family());
    }
    if (check_dst_multicast && (! dst.is_multicast())) {
	// The destination address is not multicast
	error_msg = c_format("RX %s from %s to %s on vif %s: "
			     "destination must be multicast. "
			     "Packet ignored.",
			     proto_message_type2ascii(message_type),
			     cstring(src), cstring(dst),
			     name().c_str());
	XLOG_WARNING("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    //
    // Inner multicast address scope check.
    //
    if (check_group_interfacelocal_multicast
	&& group_address.is_interfacelocal_multicast()) {
	error_msg = c_format("RX %s from %s to %s on vif %s: "
			     "invalid interface-local scope of inner "
			     "multicast address: %s",
			     proto_message_type2ascii(message_type),
			     cstring(src), cstring(dst),
			     name().c_str(),
			     cstring(group_address));
	XLOG_WARNING("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    //
    // Origin router neighbor check.
    //
    // XXX: in IGMP and MLD we don't need such check
    
    //
    // Process each message, based on its type.
    //
    if (proto_is_igmp()) {
	switch (message_type) {
	case IGMP_MEMBERSHIP_QUERY:
	    mld6igmp_membership_query_recv(src, dst,
					   message_type, max_resp_code,
					   group_address, buffer);
	    break;
	case IGMP_V1_MEMBERSHIP_REPORT:
	case IGMP_V2_MEMBERSHIP_REPORT:
	    mld6igmp_membership_report_recv(src, dst,
					    message_type, max_resp_code,
					    group_address, buffer);
	    break;
	case IGMP_V2_LEAVE_GROUP:
	    mld6igmp_leave_group_recv(src, dst,
				      message_type, max_resp_code,
				      group_address, buffer);
	    break;
	case IGMP_V3_MEMBERSHIP_REPORT:
	    mld6igmp_ssm_membership_report_recv(src, dst, message_type,
						buffer);
	    break;
	case IGMP_DVMRP:
	{
	    //
	    // XXX: We care only about the DVMRP messages that are used
	    // by mrinfo.
	    //
	    // XXX: the older purpose of the 'igmp_code' field
	    uint16_t igmp_code = max_resp_code;
	    switch (igmp_code) {
	    case DVMRP_ASK_NEIGHBORS:
		// Some old DVMRP messages from mrinfo(?).
		// TODO: not implemented yet.
		// TODO: do we really need this message implemented?
		break;
	    case DVMRP_ASK_NEIGHBORS2:
		// Used for mrinfo support.
		// XXX: not implemented yet.
		break;
	    case DVMRP_INFO_REQUEST:
		// Information request (TODO: used by mrinfo?)
		// TODO: not implemented yet.
		break;
	    default:
		// XXX: We don't care about the rest of the DVMRP_* messages
		break;
	    }
	}
	case IGMP_MTRACE:
	    // TODO: is this the new message sent by 'mtrace'?
	    // TODO: not implemented yet.
	    break;
	default:
	    // XXX: Unrecognized message types MUST be silently ignored.
	    break;
	}
    }

    if (proto_is_mld6()) {
	switch (message_type) {
	case MLD_LISTENER_QUERY:
	    mld6igmp_membership_query_recv(src, dst,
					   message_type, max_resp_code,
					   group_address, buffer);
	    break;
	case MLD_LISTENER_REPORT:
	    mld6igmp_membership_report_recv(src, dst,
					    message_type, max_resp_code,
					    group_address, buffer);
	    break;
	case MLD_LISTENER_DONE:
	    mld6igmp_leave_group_recv(src, dst,
				      message_type, max_resp_code,
				      group_address, buffer);
	    break;
	case MLDV2_LISTENER_REPORT:
	    mld6igmp_ssm_membership_report_recv(src, dst, message_type,
						buffer);
	    break;
	case MLD_MTRACE:
	    // TODO: is this the new message sent by 'mtrace'?
	    // TODO: not implemented yet.
	    break;
	default:
	    // XXX: Unrecognized message types MUST be silently ignored.
	    break;
	}
    }

    return (XORP_OK);

 rcvlen_error:
    XLOG_UNREACHABLE();
    error_msg = c_format("RX packet from %s to %s on vif %s: "
			 "some fields are too short",
			 cstring(src), cstring(dst),
			 name().c_str());
    XLOG_WARNING("%s", error_msg.c_str());
    return (XORP_ERROR);
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
	    set_i_am_querier(false);
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
	set_i_am_querier(true);
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
 * Test if the interface is running in IGMPv1 mode.
 *
 * @return true if the interface is running in IGMPv1 mode, otherwise false.
 */
bool
Mld6igmpVif::is_igmpv1_mode() const
{
    return (proto_is_igmp() && (proto_version() == IGMP_V1));
}

/**
 * Test if the interface is running in IGMPv2 mode.
 *
 * @return true if the interface is running in IGMPv2 mode, otherwise false.
 */
bool
Mld6igmpVif::is_igmpv2_mode() const
{
    return (proto_is_igmp() && (proto_version() == IGMP_V2));
}

/**
 * Test if the interface is running in IGMPv3 mode.
 *
 * @return true if the interface is running in IGMPv3 mode, otherwise false.
 */
bool
Mld6igmpVif::is_igmpv3_mode() const
{
    return (proto_is_igmp() && (proto_version() == IGMP_V3));
}

/**
 * Test if the interface is running in MLDv1 mode.
 *
 * @return true if the interface is running in MLDv1 mode, otherwise false.
 */
bool
Mld6igmpVif::is_mldv1_mode() const
{
    return (proto_is_mld6() && (proto_version() == MLD_V1));
}

/**
 * Test if the interface is running in MLDv2 mode.
 *
 * @return true if the interface is running in MLDv2 mode, otherwise false.
 */
bool
Mld6igmpVif::is_mldv2_mode() const
{
    return (proto_is_mld6() && (proto_version() == MLD_V2));
}

/**
 * Test if a group is running in IGMPv1 mode.
 *
 * Note that if @ref group_record is NULL, then we test whether the
 * interface itself is running in IGMPv1 mode.
 * @param group_record the group record to test.
 * @return true if the group is running in IGMPv1 mode,
 * otherwise false.
 */
bool
Mld6igmpVif::is_igmpv1_mode(const Mld6igmpGroupRecord* group_record) const
{
    if (group_record != NULL)
	return (group_record->is_igmpv1_mode());

    return (is_igmpv1_mode());
}

/**
 * Test if a group is running in IGMPv2 mode.
 *
 * Note that if @ref group_record is NULL, then we test whether the
 * interface itself is running in IGMPv2 mode.
 * @param group_record the group record to test.
 * @return true if the group is running in IGMPv2 mode,
 * otherwise false.
 */
bool
Mld6igmpVif::is_igmpv2_mode(const Mld6igmpGroupRecord* group_record) const
{
    if (group_record != NULL)
	return (group_record->is_igmpv2_mode());

    return (is_igmpv2_mode());
}

/**
 * Test if a group is running in IGMPv3 mode.
 *
 * Note that if @ref group_record is NULL, then we test whether the
 * interface itself is running in IGMPv3 mode.
 * @param group_record the group record to test.
 * @return true if the group is running in IGMPv3 mode,
 * otherwise false.
 */
bool
Mld6igmpVif::is_igmpv3_mode(const Mld6igmpGroupRecord* group_record) const
{
    if (group_record != NULL)
	return (group_record->is_igmpv3_mode());

    return (is_igmpv3_mode());
}

/**
 * Test if a group is running in MLDv1 mode.
 *
 * Note that if @ref group_record is NULL, then we test whether the
 * interface itself is running in MLDv1 mode.
 * @param group_record the group record to test.
 * @return true if the group is running in MLDv1 mode,
 * otherwise false.
 */
bool
Mld6igmpVif::is_mldv1_mode(const Mld6igmpGroupRecord* group_record) const
{
    if (group_record != NULL)
	return (group_record->is_mldv1_mode());

    return (is_mldv1_mode());
}

/**
 * Test if a group is running in MLDv2 mode.
 *
 * Note that if @ref group_record is NULL, then we test whether the
 * interface itself is running in MLDv2 mode.
 * @param group_record the group record to test.
 * @return true if the group is running in MLDv2 mode,
 * otherwise false.
 */
bool
Mld6igmpVif::is_mldv2_mode(const Mld6igmpGroupRecord* group_record) const
{
    if (group_record != NULL)
	return (group_record->is_mldv2_mode());

    return (is_mldv2_mode());
}

/**
 * Return the ASCII text description of the protocol message.
 *
 * @param message_type the protocol message type.
 * @return the ASCII text descrpition of the protocol message.
 */
const char *
Mld6igmpVif::proto_message_type2ascii(uint8_t message_type) const
{
    if (proto_is_igmp())
	return (IGMPTYPE2ASCII(message_type));

    if (proto_is_mld6())
	return (MLDTYPE2ASCII(message_type));
    
    return ("Unknown protocol message");
}

/**
 * Reset and prepare the buffer for sending data.
 *
 * @return the prepared buffer.
 */
buffer_t *
Mld6igmpVif::buffer_send_prepare()
{
    BUFFER_RESET(_buffer_send);
    
    return (_buffer_send);
}

/**
 * Calculate the checksum of an IPv6 "pseudo-header" as described
 * in RFC 2460.
 * 
 * @param src the source address of the pseudo-header.
 * @param dst the destination address of the pseudo-header.
 * @param len the upper-layer packet length of the pseudo-header
 * (in host-order).
 * @param protocol the upper-layer protocol number.
 * @return the checksum of the IPv6 "pseudo-header".
 */
uint16_t
Mld6igmpVif::calculate_ipv6_pseudo_header_checksum(const IPvX& src,
						   const IPvX& dst,
						   size_t len,
						   uint8_t protocol)
{
    struct ip6_pseudo_hdr {
	struct in6_addr	ip6_src;	// Source address
	struct in6_addr	ip6_dst;	// Destination address
	uint32_t	ph_len;		// Upper-layer packet length
	uint8_t		ph_zero[3];	// Zero
	uint8_t		ph_next;	// Upper-layer protocol number
    } ip6_pseudo_header;	// TODO: may need __attribute__((__packed__))
    
    src.copy_out(ip6_pseudo_header.ip6_src);
    dst.copy_out(ip6_pseudo_header.ip6_dst);
    ip6_pseudo_header.ph_len = htonl(len);
    ip6_pseudo_header.ph_zero[0] = 0;
    ip6_pseudo_header.ph_zero[1] = 0;
    ip6_pseudo_header.ph_zero[2] = 0;
    ip6_pseudo_header.ph_next = protocol;
    
    uint16_t cksum = inet_checksum(
	reinterpret_cast<const uint8_t *>(&ip6_pseudo_header),
	sizeof(ip6_pseudo_header));
    
    return (cksum);
}

/**
 * Notify the interested parties that there is membership change among
 * the local members.
 *
 * @param source the source address of the (S,G) entry that has changed.
 * In case of group-specific membership, it could be IPvX::ZERO().
 * @param group the group address of the (S,G) entry that has changed.
 * @param action_jp the membership change: @ref ACTION_JOIN
 * or @ref ACTION_PRUNE.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
Mld6igmpVif::join_prune_notify_routing(const IPvX& source,
				       const IPvX& group,
				       action_jp_t action_jp) const
{
    XLOG_TRACE(mld6igmp_node().is_log_trace(),
	       "Notify routing %s membership for (%s, %s) on vif %s",
	       (action_jp == ACTION_JOIN)? "add" : "delete",
	       cstring(source), cstring(group), name().c_str());

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
						      action_jp)
	    != XORP_OK) {
	    //
	    // TODO: remove <module_id, module_instance_name> ??
	    //
	}
    }
    
    return (XORP_OK);
}

bool
Mld6igmpVif::i_am_querier() const
{
    if (_proto_flags & MLD6IGMP_VIF_QUERIER)
	return (true);
    else
	return (false);
}

void
Mld6igmpVif::set_i_am_querier(bool v)
{
    if (v) {
	_proto_flags |= MLD6IGMP_VIF_QUERIER;
	//
	// XXX: Restore the variables that might have been adopted from
	// the Querier.
	//
	restore_effective_variables();
    } else {
	_proto_flags &= ~MLD6IGMP_VIF_QUERIER;
    }
}

size_t
Mld6igmpVif::mld6igmp_constant_minlen() const
{
    if (proto_is_igmp())
	return (IGMP_MINLEN);

    if (proto_is_mld6())
	return (MLD_MINLEN);

    XLOG_UNREACHABLE();
    return (0);
}

uint32_t
Mld6igmpVif::mld6igmp_constant_timer_scale() const
{
    if (proto_is_igmp())
	return (IGMP_TIMER_SCALE);

    if (proto_is_mld6())
	return (MLD_TIMER_SCALE);

    XLOG_UNREACHABLE();
    return (0);
}

uint8_t
Mld6igmpVif::mld6igmp_constant_membership_query() const
{
    if (proto_is_igmp())
	return (IGMP_MEMBERSHIP_QUERY);

    if (proto_is_mld6())
	return (MLD_LISTENER_QUERY);

    XLOG_UNREACHABLE();
    return (0);
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
