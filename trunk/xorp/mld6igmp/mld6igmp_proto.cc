// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/mld6igmp/mld6igmp_proto.cc,v 1.48 2008/01/04 03:16:51 pavlin Exp $"


//
// Internet Group Management Protocol implementation.
// IGMPv1, IGMPv2 (RFC 2236), and IGMPv3 (RFC 3376).
//
// AND
//
// Multicast Listener Discovery protocol implementation.
// MLDv1 (RFC 2710) and MLDv2 (RFC 3810).
//



#include "mld6igmp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include <set>

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
 * Mld6igmpVif::mld6igmp_membership_query_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The message type.
 * @max_resp_code: The Maximum Response Code from the MLD/IGMP header.
 * @group_address: The Group Address from the MLD/IGMP message.
 * @buffer: The buffer with the rest of the message.
 * 
 * Receive and process IGMP_MEMBERSHIP_QUERY/MLD_LISTENER_QUERY message
 * from another router.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::mld6igmp_membership_query_recv(const IPvX& src,
					    const IPvX& dst,
					    uint8_t message_type,
					    uint16_t max_resp_code,
					    const IPvX& group_address,
					    buffer_t *buffer)
{
    int message_version = 0;

    // Ignore my own queries
    if (mld6igmp_node().is_my_addr(src))
	return (XORP_ERROR);

    //
    // Determine the protocol version of the Query message
    //
    if (proto_is_igmp()) {
	//
	// The IGMP version of a Membership Query message is:
	// - IGMPv1 Query: length = 8 AND Max Resp Code field is zero
	// - IGMPv2 Query: length = 8 AND Max Resp Code field is non-zero
	// - IGMPv3 Query: length >= 12
	//
	do {
	    //
	    // Note that the Query processing so far has decoded the message
	    // up to the group address included (i.e., 8 octets), hence
	    // we add-back the size of the decoded portion.
	    //
	    size_t data_size = BUFFER_DATA_SIZE(buffer) + IGMP_MINLEN;

	    if ((data_size == IGMP_MINLEN) && (max_resp_code == 0)) {
		message_version = IGMP_V1;
		break;
	    }
	    if ((data_size == IGMP_MINLEN) && (max_resp_code != 0)) {
		message_version = IGMP_V2;
		break;
	    }
	    if (data_size >= IGMP_V3_QUERY_MINLEN) {
		message_version = IGMP_V3;
		break;
	    }

	    //
	    // Silently ignore all other Query messages that don't match
	    // any of the above conditions.
	    //
	    return (XORP_ERROR);
	} while (false);
	XLOG_ASSERT(message_version > 0);

	//
	// Query version consistency check.
	// If there is mismatch, then drop the message.
	// See RFC 3376 Section 7.3.1, and RFC 3810 Section 8.3.1.
	//
	if (mld6igmp_query_version_consistency_check(src, dst, message_type,
						     message_version)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
    }

    if (proto_is_mld6()) {
	//
	// The MLD version of a Membership Query message is:
	// - MLDv1 Query: length = 24
	// - MLDv2 Query: length >= 28
	//
	do {
	    //
	    // Note that the Query processing so far has decoded the message
	    // up to the group address included (i.e., 24 octets), hence
	    // we add-back the size of the decoded portion.
	    //
	    size_t data_size = BUFFER_DATA_SIZE(buffer) + MLD_MINLEN;

	    if (data_size == MLD_MINLEN) {
		message_version = MLD_V1;
		break;
	    }
	    if (data_size >= MLD_V2_QUERY_MINLEN) {
		message_version = MLD_V2;
		break;
	    }

	    //
	    // Silently ignore all other Query messages that don't match
	    // any of the above conditions.
	    //
	    return (XORP_ERROR);
	} while (false);
	XLOG_ASSERT(message_version > 0);

	//
	// Query version consistency check.
	// If there is mismatch, then drop the message.
	// See RFC 3376 Section 7.3.1, and RFC 3810 Section 8.3.1.
	//
	if (mld6igmp_query_version_consistency_check(src, dst, message_type,
						     message_version)
	    != XORP_OK) {
	    return (XORP_ERROR);
	}
    }

    XLOG_ASSERT(message_version > 0);
    
    //
    // Compare this querier address with my address.
    // XXX: Here we should compare the old and new querier
    // addresses, but we don't really care.
    //
    XLOG_ASSERT(primary_addr() != IPvX::ZERO(family()));
    if (src < primary_addr()) {
	// Eventually a new querier
	_query_timer.unschedule();
	set_querier_addr(src);
	set_i_am_querier(false);
	TimeVal other_querier_present_interval =
	    effective_query_interval() * effective_robustness_variable()
	    + query_response_interval().get() / 2;
	_other_querier_timer =
	    mld6igmp_node().eventloop().new_oneoff_after(
		other_querier_present_interval,
		callback(this, &Mld6igmpVif::other_querier_timer_timeout));
    }

    //
    // XXX: if this is IGMPv3 or MLDv2 Query, then process separately
    // the rest the message.
    //
    if ((proto_is_igmp() && (message_version >= IGMP_V3))
	|| (proto_is_mld6() && (message_version >= MLD_V2))) {
	mld6igmp_ssm_membership_query_recv(src, dst, message_type,
					   max_resp_code, group_address,
					   buffer);
	return (XORP_OK);
    }

    //
    // From RFC 2236:
    // "When a non-Querier receives a Group-Specific Query message, if its
    // existing group membership timer is greater than [Last Member Query
    // Count] times the Max Response Time specified in the message, it sets
    // its group membership timer to that value."
    //
    //
    // From RFC 2710:
    // "When a router in Non-Querier state receives a Multicast-Address-
    // Specific Query, if its timer value for the identified multicast
    // address is greater than [Last Listener Query Count] times the Maximum
    // Response Delay specified in the message, it sets the address's timer
    // to that latter value."
    //
    if ( (! group_address.is_zero())
	 && (max_resp_code != 0)
	 && (! i_am_querier())) {
	    uint32_t timer_scale = mld6igmp_constant_timer_scale();
	    TimeVal received_resp_tv;

	    // "Last Member Query Count" / "Last Listener Query Count"
	    received_resp_tv = TimeVal(effective_robustness_variable() * max_resp_code, 0);
	    received_resp_tv = received_resp_tv / timer_scale;
	    _group_records.lower_group_timer(group_address, received_resp_tv);

    }

    return (XORP_OK);
}

/**
 * Mld6igmpVif::mld6igmp_ssm_membership_query_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The message type.
 * @max_resp_code: The Maximum Response Code from the MLD/IGMP header.
 * @group_address: The Group Address from the MLD/IGMP message.
 * @buffer: The buffer with the rest of the message.
 * 
 * Receive and process IGMPv3/MLDv2 IGMP_MEMBERSHIP_QUERY/MLD_LISTENER_QUERY
 * message from another router.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::mld6igmp_ssm_membership_query_recv(const IPvX& src,
						const IPvX& dst,
						uint8_t message_type,
						uint16_t max_resp_code,
						const IPvX& group_address,
						buffer_t *buffer)
{
    bool s_flag = false;
    uint8_t qrv = 0;
    uint8_t qqic = 0;
    uint16_t sources_n = 0;
    TimeVal max_resp_time, qqi;
    set<IPvX> sources;
    string error_msg;

    //
    // Decode the Max Resp Code
    //
    if (proto_is_igmp()) {
	decode_exp_time_code8(max_resp_code, max_resp_time,
			      mld6igmp_constant_timer_scale());
    }
    if (proto_is_mld6()) {
	decode_exp_time_code16(max_resp_code, max_resp_time,
			       mld6igmp_constant_timer_scale());
    }

    //
    // Decode the rest of the message header
    //
    BUFFER_GET_OCTET(qrv, buffer);
    BUFFER_GET_OCTET(qqic, buffer);
    BUFFER_GET_HOST_16(sources_n, buffer);
    if (proto_is_igmp()) {
	s_flag = IGMP_SFLAG(qrv);
	qrv = IGMP_QRV(qrv);
    }
    if (proto_is_mld6()) {
	s_flag = MLD_SFLAG(qrv);
	qrv = MLD_QRV(qrv);
    }
    decode_exp_time_code8(qqic, qqi, 1);

    //
    // Check the remaining size of the message
    //
    if (BUFFER_DATA_SIZE(buffer) < sources_n * IPvX::addr_bytelen(family())) {
	error_msg = c_format("RX %s from %s to %s on vif %s: "
			     "source addresses array size too short"
			     "(received %u expected at least %u)",
			     proto_message_type2ascii(message_type),
			     cstring(src), cstring(dst),
			     name().c_str(),
			     XORP_UINT_CAST(BUFFER_DATA_SIZE(buffer)),
			     XORP_UINT_CAST(sources_n * IPvX::addr_bytelen(family())));
	XLOG_WARNING("%s", error_msg.c_str());
	return (XORP_ERROR);
    }

    //
    // Decode the array of source addresses
    //
    while (sources_n != 0) {
	IPvX ipvx(family());
	BUFFER_GET_IPVX(family(), ipvx, buffer);
	sources.insert(ipvx);
	sources_n--;
    }

    //
    // Adopt the Querier's Robustness Variable and Query Interval
    //
    if (! i_am_querier()) {
	if (qrv != 0) {
	    set_effective_robustness_variable(qrv);
	} else {
	    set_effective_robustness_variable(configured_robust_count().get());
	}
	if (qqi != TimeVal::ZERO()) {
	    set_effective_query_interval(qqi);
	} else {
	    set_effective_query_interval(configured_query_interval().get());
	}
    }

    //
    // Lower the group and source timers
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

    return (XORP_OK);

 rcvlen_error:
    XLOG_UNREACHABLE();
    error_msg = c_format("RX %s from %s to %s on vif %s: "
			 "some fields are too short",
			 proto_message_type2ascii(message_type),
			 cstring(src), cstring(dst),
			 name().c_str());
    XLOG_WARNING("%s", error_msg.c_str());
    return (XORP_ERROR);
}

/**
 * Mld6igmpVif::mld6igmp_membership_report_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The message type.
 * @max_resp_code: The Maximum Response Code from the MLD/IGMP header.
 * @group_address: The Group Address from the MLD/IGMP message.
 * @buffer: The buffer with the rest of the message.
 * 
 * Receive and process
 * (IGMP_V1_MEMBERSHIP_REPORT or IGMP_V2_MEMBERSHIP_REPORT)/MLD_LISTENER_REPORT
 * message from a host.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::mld6igmp_membership_report_recv(const IPvX& src,
					     const IPvX& dst,
					     uint8_t message_type,
					     uint16_t max_resp_code,
					     const IPvX& group_address,
					     buffer_t *buffer)
{
    Mld6igmpGroupRecord *group_record = NULL;
    
    // The group address must be a valid multicast address
    if (! group_address.is_multicast()) {
	XLOG_WARNING("RX %s from %s to %s on vif %s: "
		     "the group address %s is not "
		     "valid multicast address",
		     proto_message_type2ascii(message_type),
		     cstring(src), cstring(dst),
		     name().c_str(),
		     cstring(group_address));
	return (XORP_ERROR);
    }

    set<IPvX> no_sources;		// XXX: empty set
    group_records().process_mode_is_exclude(group_address, no_sources, src);

    //
    // Check whether an older Membership report has been received
    //
    int message_version = 0;
    if (proto_is_igmp()) {
	switch (message_type) {
	case IGMP_V1_MEMBERSHIP_REPORT:
	    message_version = IGMP_V1;
	    break;
	case IGMP_V2_MEMBERSHIP_REPORT:
	    message_version = IGMP_V2;
	    break;
	case IGMP_V3_MEMBERSHIP_REPORT:
	    message_version = IGMP_V3;
	    break;
	default:
	    message_version = IGMP_V2;
	    break;
	}
    }
    if (proto_is_mld6()) {
	switch (message_type) {
	case MLD_LISTENER_REPORT:
	    message_version = MLD_V1;
	    break;
	case MLDV2_LISTENER_REPORT:
	    message_version = MLD_V2;
	    break;
	default:
	    message_version = MLD_V1;
	    break;
	}
    }
    XLOG_ASSERT(message_version > 0);
    group_record = _group_records.find_group_record(group_address);
    XLOG_ASSERT(group_record != NULL);
    group_record->received_older_membership_report(message_version);

    UNUSED(max_resp_code);
    UNUSED(buffer);

    return (XORP_OK);
}

/**
 * Mld6igmpVif::mld6igmp_leave_group_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The message type.
 * @max_resp_code: The Maximum Response Code from the MLD/IGMP header.
 * @group_address: The Group Address from the MLD/IGMP message.
 * @buffer: The buffer with the rest of the message.
 * 
 * Receive and process IGMP_V2_LEAVE_GROUP/MLD_LISTENER_DONE message
 * from a host.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::mld6igmp_leave_group_recv(const IPvX& src,
				       const IPvX& dst,
				       uint8_t message_type,
				       uint16_t max_resp_code,
				       const IPvX& group_address,
				       buffer_t *buffer)
{
    Mld6igmpGroupRecord *group_record = NULL;
    string dummy_error_msg;

    // The group address must be a valid multicast address
    if (! group_address.is_multicast()) {
	XLOG_WARNING("RX %s from %s to %s on vif %s: "
		     "the group address %s is not "
		     "valid multicast address",
		     proto_message_type2ascii(message_type),
		     cstring(src), cstring(dst),
		     name().c_str(),
		     cstring(group_address));
	return (XORP_ERROR);
    }

    //
    // Find if we already have an entry for this group
    //
    group_record = _group_records.find_group_record(group_address);
    if (group_record == NULL) {
	// Nothing found. Ignore.
	return (XORP_OK);
    }

    //
    // Group found
    //
    if (is_igmpv1_mode(group_record)) {
	//
	// Ignore this 'Leave Group' message because this
	// group has IGMPv1 hosts members.
	//
	return (XORP_OK);
    }

    set<IPvX> no_sources;		// XXX: empty set
    group_records().process_change_to_include_mode(group_address, no_sources,
						   src);
    return (XORP_OK);

    UNUSED(max_resp_code);
    UNUSED(buffer);
    
    return (XORP_OK);
}

/**
 * Mld6igmpVif::mld6igmp_ssm_membership_report_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The message type.
 * @buffer: The buffer with the rest of the message.
 * 
 * Receive and process IGMP_V3_MEMBERSHIP_REPORT/MLDV2_LISTENER_REPORT
 * message from a host.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::mld6igmp_ssm_membership_report_recv(const IPvX& src,
						 const IPvX& dst,
						 uint8_t message_type,
						 buffer_t *buffer)
{
    uint16_t group_records_n = 0;
    string error_msg;
    typedef pair<IPvX, set<IPvX> > gs_record;	// XXX: a handy typedef
    list<gs_record> mode_is_include_groups;
    list<gs_record> mode_is_exclude_groups;
    list<gs_record> change_to_include_mode_groups;
    list<gs_record> change_to_exclude_mode_groups;
    list<gs_record> allow_new_sources_groups;
    list<gs_record> block_old_sources_groups;
    list<gs_record>::iterator gs_iter;

    //
    // Decode the rest of the message header
    //
    BUFFER_GET_SKIP(2, buffer);		// The 'Reserved' field
    BUFFER_GET_HOST_16(group_records_n, buffer);

    //
    // Decode the array of group records
    //
    while (group_records_n != 0) {
	uint8_t record_type;
	uint8_t aux_data_len;
	uint16_t sources_n;
	IPvX group_address(family());
	set<IPvX> source_addresses;
	list<gs_record>* gs_record_ptr = NULL;

	BUFFER_GET_OCTET(record_type, buffer);
	BUFFER_GET_OCTET(aux_data_len, buffer);
	BUFFER_GET_HOST_16(sources_n, buffer);
	BUFFER_GET_IPVX(family(), group_address, buffer);

	// Decode the array of source addresses
	while (sources_n != 0) {
	    IPvX ipvx(family());
	    BUFFER_GET_IPVX(family(), ipvx, buffer);
	    source_addresses.insert(ipvx);
	    sources_n--;
	}

	// XXX: Skip the 'Auxiliary Data', because we don't use it
	BUFFER_GET_SKIP(aux_data_len, buffer);

	//
	// Select the appropriate set, and add the group and the sources to it
	//
	if (proto_is_igmp()) {
	    switch (record_type) {
	    case IGMP_MODE_IS_INCLUDE:
		gs_record_ptr = &mode_is_include_groups;
		break;
	    case IGMP_MODE_IS_EXCLUDE:
		gs_record_ptr = &mode_is_exclude_groups;
		break;
	    case IGMP_CHANGE_TO_INCLUDE_MODE:
		gs_record_ptr = &change_to_include_mode_groups;
		break;
	    case IGMP_CHANGE_TO_EXCLUDE_MODE:
		gs_record_ptr = &change_to_exclude_mode_groups;
		break;
	    case IGMP_ALLOW_NEW_SOURCES:
		gs_record_ptr = &allow_new_sources_groups;
		break;
	    case IGMP_BLOCK_OLD_SOURCES:
		gs_record_ptr = &block_old_sources_groups;
		break;
	    default:
		break;
	    }
	}
	if (proto_is_mld6()) {
	    switch (record_type) {
	    case MLD_MODE_IS_INCLUDE:
		gs_record_ptr = &mode_is_include_groups;
		break;
	    case MLD_MODE_IS_EXCLUDE:
		gs_record_ptr = &mode_is_exclude_groups;
		break;
	    case MLD_CHANGE_TO_INCLUDE_MODE:
		gs_record_ptr = &change_to_include_mode_groups;
		break;
	    case MLD_CHANGE_TO_EXCLUDE_MODE:
		gs_record_ptr = &change_to_exclude_mode_groups;
		break;
	    case MLD_ALLOW_NEW_SOURCES:
		gs_record_ptr = &allow_new_sources_groups;
		break;
	    case MLD_BLOCK_OLD_SOURCES:
		gs_record_ptr = &block_old_sources_groups;
		break;
	    default:
		break;
	    }
	}
	if (gs_record_ptr != NULL) {
	    gs_record_ptr->push_back(make_pair(group_address,
					       source_addresses));
	} else {
	    error_msg = c_format("RX %s from %s to %s on vif %s: "
				 "unrecognized record type %d (ignored)",
				 proto_message_type2ascii(message_type),
				 cstring(src), cstring(dst),
				 name().c_str(),
				 record_type);
	    XLOG_WARNING("%s", error_msg.c_str());
	}

	group_records_n--;
    }

    //
    // Process the records
    //
    for (gs_iter = mode_is_include_groups.begin();
	 gs_iter != mode_is_include_groups.end();
	 ++gs_iter) {
	group_records().process_mode_is_include(gs_iter->first,
						gs_iter->second,
						src);
    }
    for (gs_iter = mode_is_exclude_groups.begin();
	 gs_iter != mode_is_exclude_groups.end();
	 ++gs_iter) {
	group_records().process_mode_is_exclude(gs_iter->first,
						gs_iter->second,
						src);
    }
    for (gs_iter = change_to_include_mode_groups.begin();
	 gs_iter != change_to_include_mode_groups.end();
	 ++gs_iter) {
	group_records().process_change_to_include_mode(gs_iter->first,
						       gs_iter->second,
						       src);
    }
    for (gs_iter = change_to_exclude_mode_groups.begin();
	 gs_iter != change_to_exclude_mode_groups.end();
	 ++gs_iter) {
	group_records().process_change_to_exclude_mode(gs_iter->first,
						       gs_iter->second,
						       src);
    }
    for (gs_iter = allow_new_sources_groups.begin();
	 gs_iter != allow_new_sources_groups.end();
	 ++gs_iter) {
	group_records().process_allow_new_sources(gs_iter->first,
						  gs_iter->second,
						  src);
    }
    for (gs_iter = block_old_sources_groups.begin();
	 gs_iter != block_old_sources_groups.end();
	 ++gs_iter) {
	group_records().process_block_old_sources(gs_iter->first,
						  gs_iter->second,
						  src);
    }

    return (XORP_OK);

 rcvlen_error:
    error_msg = c_format("RX %s from %s to %s on vif %s: "
			 "some fields are too short",
			 proto_message_type2ascii(message_type),
			 cstring(src), cstring(dst),
			 name().c_str());
    XLOG_WARNING("%s", error_msg.c_str());
    return (XORP_ERROR);
}

/**
 * Mld6igmpVif::other_querier_timer_timeout:
 * 
 * Timeout: the previous querier has expired. I will become the querier.
 **/
void
Mld6igmpVif::other_querier_timer_timeout()
{
    string dummy_error_msg;

    if (primary_addr() == IPvX::ZERO(family())) {
	// XXX: the vif address is unknown; this cannot happen if the
	// vif status is UP.
	XLOG_ASSERT(! is_up());
	return;
    }
    
    set_querier_addr(primary_addr());
    set_i_am_querier(true);

    //
    // Now I am the querier. Send a general membership query.
    //
    TimeVal max_resp_time = query_response_interval().get();
    set<IPvX> no_sources;		// XXX: empty set
    mld6igmp_query_send(primary_addr(),
			IPvX::MULTICAST_ALL_SYSTEMS(family()),
			max_resp_time,
			IPvX::ZERO(family()),			// XXX: ANY
			no_sources,
			false,
			dummy_error_msg);
    _startup_query_count = 0;		// XXX: not a startup case
    _query_timer = mld6igmp_node().eventloop().new_oneoff_after(
	effective_query_interval(),
	callback(this, &Mld6igmpVif::query_timer_timeout));
}

/**
 * Mld6igmpVif::query_timer_timeout:
 * 
 * Timeout: time to send a membership query.
 **/
void
Mld6igmpVif::query_timer_timeout()
{
    TimeVal interval;
    string dummy_error_msg;

    if (! i_am_querier())
	return;		// I am not the querier anymore. Ignore.

    //
    // Send a general membership query
    //
    TimeVal max_resp_time = query_response_interval().get();
    set<IPvX> no_sources;		// XXX: empty set
    mld6igmp_query_send(primary_addr(),
			IPvX::MULTICAST_ALL_SYSTEMS(family()),
			max_resp_time,
			IPvX::ZERO(family()),			// XXX: ANY
			no_sources,
			false,
			dummy_error_msg);
    if (_startup_query_count > 0)
	_startup_query_count--;
    if (_startup_query_count > 0) {
	// "Startup Query Interval"
	interval = effective_query_interval() / 4;
    } else {
	interval = effective_query_interval();
    }

    _query_timer = mld6igmp_node().eventloop().new_oneoff_after(
	interval,
	callback(this, &Mld6igmpVif::query_timer_timeout));
}

/**
 * mld6igmp_query_version_consistency_check:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The type of the MLD/IGMP message.
 * @message_version: The protocol version of the received Query message:
 * (IGMP_V1, IGMP_V2, IGMP_V3 for IGMP) or (MLD_V1, MLD_V2 for MLD).
 * 
 * Check for MLD/IGMP protocol version interface configuration consistency.
 * For example, if the received Query message was IGMPv1, a correctly
 * configured local interface must be operating in IGMPv1 mode.
 * Similarly, if the local interface is operating in IGMPv1 mode,
 * all other neighbor routers (for that interface) must be
 * operating in IGMPv1 as well.
 * 
 * Return value: %XORP_OK if consistency, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::mld6igmp_query_version_consistency_check(const IPvX& src,
						      const IPvX& dst,
						      uint8_t message_type,
						      int message_version)
{
    string proto_name, mode_config, mode_received;

    if (message_version == proto_version())
	return (XORP_OK);

    if (proto_is_igmp())
	proto_name = "IGMP";
    if (proto_is_mld6())
	proto_name = "MLD";
    mode_config = c_format("%sv%u", proto_name.c_str(), proto_version());
    mode_received = c_format("%sv%u", proto_name.c_str(), message_version);

    // TODO: rate-limit the warning
    XLOG_WARNING("RX %s from %s to %s on vif %s: "
		 "this interface is in %s mode, but received %s message",
		 proto_message_type2ascii(message_type),
		 cstring(src), cstring(dst),
		 name().c_str(),
		 mode_config.c_str(),
		 mode_received.c_str());
    XLOG_WARNING("Please configure properly all routers on "
		 "that subnet to use same %s version",
		 proto_name.c_str());

    return (XORP_ERROR);
}

void
Mld6igmpVif::set_configured_query_interval_cb(TimeVal v)
{
    set_effective_query_interval(v);
}

void
Mld6igmpVif::set_effective_query_interval(const TimeVal& v)
{
    _effective_query_interval = v;
    recalculate_effective_query_interval();
}

void
Mld6igmpVif::recalculate_effective_query_interval()
{
    recalculate_group_membership_interval();
    recalculate_older_version_host_present_interval();
}

void
Mld6igmpVif::set_query_last_member_interval_cb(TimeVal v)
{
    UNUSED(v);
    recalculate_last_member_query_time();
}

void
Mld6igmpVif::set_query_response_interval_cb(TimeVal v)
{
    UNUSED(v);
    recalculate_group_membership_interval();
    recalculate_older_version_host_present_interval();
}

void
Mld6igmpVif::set_configured_robust_count_cb(uint32_t v)
{
    set_effective_robustness_variable(v);
}

void
Mld6igmpVif::set_effective_robustness_variable(uint32_t v)
{
    _effective_robustness_variable = v;
    recalculate_effective_robustness_variable();
}

void
Mld6igmpVif::recalculate_effective_robustness_variable()
{
    recalculate_group_membership_interval();
    recalculate_last_member_query_count();
    recalculate_older_version_host_present_interval();
}

void
Mld6igmpVif::recalculate_last_member_query_count()
{
    _last_member_query_count = effective_robustness_variable();
    recalculate_last_member_query_time();
}

void
Mld6igmpVif::recalculate_group_membership_interval()
{
    _group_membership_interval =
	effective_query_interval() * effective_robustness_variable()
	+ query_response_interval().get();
}

void
Mld6igmpVif::recalculate_last_member_query_time()
{
    _last_member_query_time = query_last_member_interval().get()
	* last_member_query_count();
}

void
Mld6igmpVif::recalculate_older_version_host_present_interval()
{
    _older_version_host_present_interval =
	effective_query_interval() * effective_robustness_variable()
	+ query_response_interval().get();
}

void
Mld6igmpVif::restore_effective_variables()
{
    // Restore the default Query Interval and Robustness Variable
    set_effective_robustness_variable(configured_robust_count().get());
    set_effective_query_interval(configured_query_interval().get());
}

void
Mld6igmpVif::decode_exp_time_code8(uint8_t code, TimeVal& timeval,
				   uint32_t timer_scale)
{
    uint32_t decoded_time = 0;

    //
    // From RFC 3376 Section 4.1.1, and RFC 3810 Section 5.1.9:
    //
    // If Code < 128, Time = Code
    //
    // If Code >= 128, Code represents a floating-point value as follows:
    //
    //     0 1 2 3 4 5 6 7
    //    +-+-+-+-+-+-+-+-+
    //    |1| exp | mant  |
    //    +-+-+-+-+-+-+-+-+
    //
    // Time = (mant | 0x10) << (exp + 3)
    //
    if (code < 128) {
	decoded_time = code;
    } else {
	uint8_t mant = code & 0xf;
	uint8_t exp = (code >> 4) & 0x7;
	decoded_time = (mant | 0x10) << (exp + 3);
    }

    timeval = TimeVal(decoded_time, 0);
    timeval = timeval / timer_scale;
}

void
Mld6igmpVif::decode_exp_time_code16(uint16_t code, TimeVal& timeval,
				    uint32_t timer_scale)
{
    uint32_t decoded_time = 0;

    //
    // From RFC 3810 Section 5.1.9:
    //
    // If Code < 32768, Time = Code
    //
    // If Code >= 32768, Code represents a floating-point value as follows:
    //
    //     0 1 2 3 4 5 6 7 8 9 A B C D E F
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //    |1| exp |          mant         |
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // Time = (mant | 0x1000) << (exp + 3)
    //
    if (code < 32768) {
	decoded_time = code;
    } else {
	uint8_t mant = code & 0xfff;
	uint8_t exp = (code >> 12) & 0x7;
	decoded_time = (mant | 0x1000) << (exp + 3);
    }

    timeval = TimeVal(decoded_time, 0);
    timeval = timeval / timer_scale;
}

void
Mld6igmpVif::encode_exp_time_code8(const TimeVal& timeval,
				   uint8_t& code,
				   uint32_t timer_scale)
{
    TimeVal scaled_max_resp_time = timeval * timer_scale;
    uint32_t decoded_time = scaled_max_resp_time.sec();

    code = 0;

    //
    // From RFC 3376 Section 4.1.1, and RFC 3810 Section 5.1.9:
    //
    // If Code < 128, Time = Code
    //
    // If Code >= 128, Code represents a floating-point value as follows:
    //
    //     0 1 2 3 4 5 6 7
    //    +-+-+-+-+-+-+-+-+
    //    |1| exp | mant  |
    //    +-+-+-+-+-+-+-+-+
    //
    // Time = (mant | 0x10) << (exp + 3)
    //
    if (decoded_time < 128) {
	code = decoded_time;
    } else {
	uint8_t mant = 0;
	uint8_t exp = 0;

	// Calculate the "mant" and the "exp"
	while ((decoded_time >> (exp + 3)) > 0x1f) {
	    exp++;
	}
	mant = (decoded_time >> (exp + 3)) & 0xf;

	code = 0x80 | (exp << 4) | mant;
    }
}

void
Mld6igmpVif::encode_exp_time_code16(const TimeVal& timeval,
				    uint16_t& code,
				    uint32_t timer_scale)
{
    TimeVal scaled_max_resp_time = timeval * timer_scale;
    uint32_t decoded_time = scaled_max_resp_time.sec();

    code = 0;

    //
    // From RFC 3810 Section 5.1.9:
    //
    // If Code < 32768, Time = Code
    //
    // If Code >= 32768, Code represents a floating-point value as follows:
    //
    //     0 1 2 3 4 5 6 7 8 9 A B C D E F
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //    |1| exp |          mant         |
    //    +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //
    // Time = (mant | 0x1000) << (exp + 3)
    //
    if (decoded_time < 32768) {
	code = decoded_time;
    } else {
	uint8_t mant = 0;
	uint8_t exp = 0;

	// Calculate the "mant" and the "exp"
	while ((decoded_time >> (exp + 3)) > 0x1fff) {
	    exp++;
	}
	mant = (decoded_time >> (exp + 3)) & 0xfff;

	code = 0x8000 | (exp << 12) | mant;
    }
}
