// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/mld6igmp/igmp_proto.cc,v 1.25 2004/03/04 03:02:09 pavlin Exp $"


//
// Internet Group Management Protocol implementation.
// IGMPv1 and IGMPv2 (RFC 2236)
//


#include "mld6igmp_module.h"
#include "mld6igmp_private.hh"
#include "mrt/inet_cksum.h"
#include "igmp_proto.h"
#include "mld6igmp_node.hh"
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
 * Mld6igmpVif::igmp_process:
 * @src: The message source address.
 * @dst: The message destination address.
 * @ip_ttl: The IP TTL of the message. If it has a negative value,
 * it should be ignored.
 * @ip_tos: The IP TOS of the message. If it has a negative value,
 * it should be ignored.
 * @router_alert_bool: True if the received IP packet had the Router Alert
 * IP option set.
 * @buffer: The buffer with the message.
 * 
 * Process IGMP message and pass the control to the type-specific functions.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::igmp_process(const IPvX& src, const IPvX& dst,
			  int ip_ttl,
			  int ip_tos,
			  bool router_alert_bool,
			  buffer_t *buffer)
{
    uint8_t message_type;
    int max_resp_time, igmp_code;
    IPvX group_address(family());
    uint16_t cksum;
    
    //
    // Message length check.
    //
    if (BUFFER_DATA_SIZE(buffer) < IGMP_MINLEN) {
	XLOG_WARNING("RX packet from %s to %s on vif %s: "
		     "too short data field (%u octets)",
		     cstring(src), cstring(dst),
		     name().c_str(),
		     (uint32_t)BUFFER_DATA_SIZE(buffer));
	return (XORP_ERROR);
    }
    
    //
    // Checksum verification.
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
    if (cksum) {
	XLOG_WARNING("RX packet from %s to %s on vif %s: "
		     "checksum error",
		     cstring(src), cstring(dst),
		     name().c_str());
	return (XORP_ERROR);
    }
    
    //
    // Protocol version check.
    //
    //
    // XXX: IGMP messages do not have an explicit field for protocol version.
    // Protocol version check is performed later, per (some) message type.
    //
    
    //
    // Get the message type and the max. resp. time (named `igmp_code' in
    // `struct igmp').
    //
    BUFFER_GET_OCTET(message_type, buffer);
    BUFFER_GET_OCTET(max_resp_time, buffer);
    BUFFER_GET_SKIP(2, buffer);			// The checksum
    BUFFER_GET_IPVX(family(), group_address, buffer);

    XLOG_TRACE(mld6igmp_node().is_log_trace(),
	       "RX %s from %s to %s on vif %s",
	       proto_message_type2ascii(message_type),
	       cstring(src), cstring(dst),
	       name().c_str());
    
    //
    // TODO: if we are running in secure mode, then check ip_ttl, ip_tos and
    // @router_alert_bool (e.g. (ip_ttl == MINTTL) && (router_alert_bool))
    //
    UNUSED(ip_ttl);
    UNUSED(ip_tos);
    UNUSED(router_alert_bool);
#if 0
    //
    // TTL (aka. Hop-limit in IPv6) and Router Alert option check.
    //
    switch (message_type) {
    case IGMP_MEMBERSHIP_QUERY:
    case IGMP_V1_MEMBERSHIP_REPORT:
    case IGMP_V2_MEMBERSHIP_REPORT:
    case IGMP_V2_LEAVE_GROUP:
	if (ip_ttl != 1) {
	    XLOG_WARNING("RX %s from %s to %s on vif %s: "
			 "ip_ttl = %d instead of %d",
			 proto_message_type2ascii(message_type),
			 cstring(src), cstring(dst),
			 name().c_str(),
			 ip_ttl, 1);
	    return (XORP_ERROR);
	}
	//
	// TODO: check for Router Alert option and ignore message
	// if the option is missing and we are running in secure mode.
	//
	break;
    case IGMP_DVMRP:
    case IGMP_MTRACE:
	// TODO: perform the appropriate checks
	break;
    default:
	break;
    }
#endif // 0/1
    
    //
    // Source address check.
    //
    if (! src.is_unicast()) {
	// Source address must always be unicast
	// The kernel should have checked that, but just in case
	XLOG_WARNING("RX %s from %s to %s on vif %s: "
		     "source must be unicast",
		     proto_message_type2ascii(message_type),
		     cstring(src), cstring(dst),
		     name().c_str());
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
    switch (message_type) {
    case IGMP_MEMBERSHIP_QUERY:
    case IGMP_V1_MEMBERSHIP_REPORT:
    case IGMP_V2_MEMBERSHIP_REPORT:
    case IGMP_V2_LEAVE_GROUP:
	// XXX: source address check not needed for IPv4-only protocols
	break;
    case IGMP_DVMRP:
    case IGMP_MTRACE:
	// TODO: perform the appropriate checks
	break;
    default:
	break;
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
    switch (message_type) {
    case IGMP_MEMBERSHIP_QUERY:
    case IGMP_V1_MEMBERSHIP_REPORT:
    case IGMP_V2_MEMBERSHIP_REPORT:
    case IGMP_V2_LEAVE_GROUP:
	// Destination must be multicast
	if (! dst.is_multicast()) {
	    XLOG_WARNING("RX %s from %s to %s on vif %s: "
			 "destination must be multicast. "
			 "Packet ignored.",
			 proto_message_type2ascii(message_type),
			 cstring(src), cstring(dst),
			 name().c_str());
	    return (XORP_ERROR);
	}
	break;
    case IGMP_DVMRP:
    case IGMP_MTRACE:
	// TODO: perform the appropriate checks
	break;
    default:
	break;
    }
    
    //
    // Message-specific checks.
    //
    switch (message_type) {
    case IGMP_MEMBERSHIP_QUERY:
    case IGMP_V1_MEMBERSHIP_REPORT:
    case IGMP_V2_MEMBERSHIP_REPORT:
    case IGMP_V2_LEAVE_GROUP:
	break;
    case IGMP_DVMRP:
    case IGMP_MTRACE:
	// TODO: perform the appropriate checks
	break;
    default:
	break;
    }
    
    //
    // Origin router neighbor check.
    //
    // XXX: in IGMP we don't need such check
    
    //
    // Process each message, based on its type.
    //
    switch (message_type) {
    case IGMP_MEMBERSHIP_QUERY:
	igmp_membership_query_recv(src, dst,
				   message_type, max_resp_time,
				   group_address, buffer);
	break;
    case IGMP_V1_MEMBERSHIP_REPORT:
    case IGMP_V2_MEMBERSHIP_REPORT:
	igmp_membership_report_recv(src, dst,
				    message_type, max_resp_time,
				    group_address, buffer);
	break;
    case IGMP_V2_LEAVE_GROUP:
	igmp_leave_group_recv(src, dst,
			      message_type, max_resp_time,
			      group_address, buffer);
	break;
    case IGMP_DVMRP:
	//
	// XXX: We care only about the DVMRP messages that are used
	// by mrinfo.
	//
	// XXX: the older purpose of the 'igmp_code' field
	igmp_code = max_resp_time;
	switch (igmp_code) {
	case DVMRP_ASK_NEIGHBORS:
	    // Some old DVMRP messages from mrinfo
	    igmp_dvmrp_ask_neighbors_recv(src, dst,
					  message_type, max_resp_time,
					  group_address, buffer);
	    break;
	case DVMRP_ASK_NEIGHBORS2:
	    // Used for mrinfo support
	    igmp_dvmrp_ask_neighbors2_recv(src, dst,
					   message_type, max_resp_time,
					   group_address, buffer);
	    break;
	case DVMRP_INFO_REQUEST:
	    // Information request (TODO: used by mrinfo?)
	    igmp_dvmrp_info_request_recv(src, dst,
					 message_type, max_resp_time,
					 group_address, buffer);
	default:
	    // XXX: We don't care about the rest of the DVMRP_* messages
	    break;
	}
    case IGMP_MTRACE:
	igmp_mtrace_recv(src, dst,
			 message_type, max_resp_time,
			 group_address, buffer);
	break;
    default:
	// XXX: We don't care about the rest of the messages
	break;
    }
    
    return (XORP_OK);
    
 rcvlen_error:
    XLOG_UNREACHABLE();
    XLOG_WARNING("RX packet from %s to %s on vif %s: "
		 "some fields are too short",
		 cstring(src), cstring(dst),
		 name().c_str());
    return (XORP_ERROR);
}

/**
 * Mld6igmpVif::igmp_membership_query_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The message type.
 * @max_resp_time: The Maximum Response Time from the IGMP header.
 * @group_address: The Group Address from the IGMP message.
 * @buffer: The buffer with the rest of the message.
 * 
 * Receive and process IGMP_MEMBERSHIP_QUERY message from another router.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::igmp_membership_query_recv(const IPvX& src,
					const IPvX& dst,
					uint8_t message_type,
					int max_resp_time,
					const IPvX& group_address,
					buffer_t *buffer)
{
    int igmp_message_version;
    
    // Ignore my own queries
    if (mld6igmp_node().is_my_addr(src))
	return (XORP_ERROR);
    
    igmp_message_version = IGMP_V2;
    
    if (max_resp_time == 0) {
	//
	// A query from a IGMPv1 router.
	// Start a timer that this interface is running in V1 mode.
	//
	igmp_message_version = IGMP_V1;
	// TODO: set the timer for any router or only if I am a querier?
	_igmpv1_router_present_timer =
	    mld6igmp_node().eventloop().set_flag_after(
		TimeVal(IGMP_VERSION1_ROUTER_PRESENT_TIMEOUT, 0),
		&_dummy_flag);
    }
    igmp_v1_config_consistency_check(src, dst, message_type,
				     igmp_message_version);
    
    //
    // Compare this querier address with my address
    // XXX: here we should compare the old and new querier
    // addresses, but we don't really care.
    //
    XLOG_ASSERT(primary_addr() != IPvX::ZERO(family()));
    if (src < primary_addr()) {
	// Eventually a new querier
	_query_timer.unschedule();
	_querier_addr = src;
	_proto_flags &= ~MLD6IGMP_VIF_QUERIER;
	_other_querier_timer =
	    mld6igmp_node().eventloop().new_oneoff_after(
		TimeVal(IGMP_OTHER_QUERIER_PRESENT_INTERVAL, 0),
		callback(this, &Mld6igmpVif::other_querier_timer_timeout));
    }
    
    //
    // From RFC 2236:
    // "When a non-Querier receives a Group-Specific Query message, if its
    // existing group membership timer is greater than [Last Member Query
    // Count] times the Max Response Time specified in the message, it sets
    // its group membership timer to that value."
    //
    if ( (!group_address.is_zero())
	 && (max_resp_time != 0)
	 && !(_proto_flags & MLD6IGMP_VIF_QUERIER)) {
	// Find if already we have an entry for this group
	
	list<MemberQuery *>::iterator iter;
	for (iter = _members.begin(); iter != _members.end(); ++iter) {
	    MemberQuery *member_query = *iter;
	    if (group_address != member_query->group())
		continue;
	    
	    // Group found
	    uint32_t sec, usec;
	    TimeVal received_resp_tv;
	    TimeVal left_resp_tv;
	    
	    sec = (IGMP_LAST_MEMBER_QUERY_COUNT * max_resp_time)
		/ IGMP_TIMER_SCALE;
	    usec = (IGMP_LAST_MEMBER_QUERY_COUNT * max_resp_time)
		% IGMP_TIMER_SCALE;
	    usec *= (1000000 / IGMP_TIMER_SCALE); // microseconds
	    received_resp_tv = TimeVal(sec, usec);
	    member_query->_member_query_timer.time_remaining(left_resp_tv);
	    
	    if (left_resp_tv > received_resp_tv) {
		member_query->_member_query_timer =
		    mld6igmp_node().eventloop().new_oneoff_after(
			received_resp_tv,
			callback(member_query,
				 &MemberQuery::member_query_timer_timeout));
	    }
	    
	    break;
	}
    }
    
    // UNUSED(dst);
    UNUSED(buffer);
    
    return (XORP_OK);
}

/**
 * Mld6igmpVif::igmp_membership_report_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The message type.
 * @max_resp_time: The Maximum Response Time from the IGMP header.
 * @group_address: The Group Address from the IGMP message.
 * @buffer: The buffer with the rest of the message.
 * 
 * Receive and process IGMP_V1_MEMBERSHIP_REPORT or IGMP_V2_MEMBERSHIP_REPORT
 * message from a host.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::igmp_membership_report_recv(const IPvX& src,
					 const IPvX& dst,
					 uint8_t message_type,
					 int max_resp_time,
					 const IPvX& group_address,
					 buffer_t *buffer)
{
    int igmp_message_version;
    MemberQuery *member_query = NULL;
    IPvX source_address(family());		// XXX: ANY
    
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
    
    // Find if we already have an entry for this group
    list<MemberQuery *>::iterator iter;
    for (iter = _members.begin(); iter != _members.end(); ++iter) {
	MemberQuery *member_query_tmp = *iter;
	if (group_address == member_query_tmp->group()) {
	    // Group found
	    // TODO: XXX: cancel the g-s rxmt timer?? Not in spec!
	    member_query = member_query_tmp;
	    member_query->_last_member_query_timer.unschedule();
	    break;
	}
    }
    
    if (member_query != NULL) {
	member_query->_last_reported_host = src;
    } else {
	// A new group
	member_query = new MemberQuery(*this, source_address, group_address);
	member_query->_last_reported_host = src;
	_members.push_back(member_query);
	// notify routing (+)    
	join_prune_notify_routing(member_query->source(),
				  member_query->group(),
				  ACTION_JOIN);
    }
    
    member_query->_member_query_timer =
	mld6igmp_node().eventloop().new_oneoff_after(
	    TimeVal(IGMP_GROUP_MEMBERSHIP_INTERVAL, 0),
	    callback(member_query, &MemberQuery::member_query_timer_timeout));
    
    igmp_message_version = IGMP_V2;
    if (message_type == IGMP_V1_MEMBERSHIP_REPORT) {
	igmp_message_version = IGMP_V1;
	//
	// XXX: start the v1 host timer even if I am not querier.
	// The non-querier state diagram in RFC 2236 is incomplete,
	// and inconsistent with the text in Section 5.
	// Indeed, RFC 3376 (IGMPv3) also doesn't specify that the
	// corresponding timer has to be started only for queriers.
	//
	member_query->_igmpv1_host_present_timer =
	    mld6igmp_node().eventloop().set_flag_after(
		TimeVal(IGMP_GROUP_MEMBERSHIP_INTERVAL, 0),
		&_dummy_flag);
    }
    
    return (XORP_OK);
    
    UNUSED(max_resp_time);
    UNUSED(buffer);
}

/**
 * Mld6igmpVif::igmp_leave_group_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The message type.
 * @max_resp_time: The Maximum Response Time from the IGMP header.
 * @group_address: The Group Address from the IGMP message.
 * @buffer: The buffer with the rest of the message.
 * 
 * Receive and process IGMP_V2_LEAVE_GROUP message from a host.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::igmp_leave_group_recv(const IPvX& src,
				   const IPvX& dst,
				   uint8_t message_type,
				   int max_resp_time,
				   const IPvX& group_address,
				   buffer_t *buffer)
{
    if (is_igmpv1_mode()) {
	//
	// Ignore this 'Leave Group' message because this
	// interface is operating in IGMPv1 mode.
	//
	return (XORP_OK);
    }
    
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
    
    // Find if this group already has an entry
    list<MemberQuery *>::iterator iter;
    for (iter = _members.begin(); iter != _members.end(); ++iter) {
	MemberQuery *member_query = *iter;
	if (group_address != member_query->group())
	    continue;

	// Group found
	if (member_query->_igmpv1_host_present_timer.scheduled()) {
	    //
	    // Ignore this 'Leave Group' message because this
	    // group has IGMPv1 hosts members.
	    //
	    return (XORP_OK);
	}
	if (_proto_flags & MLD6IGMP_VIF_QUERIER) {
	    member_query->_member_query_timer =
		mld6igmp_node().eventloop().new_oneoff_after(
		    TimeVal(IGMP_LAST_MEMBER_QUERY_INTERVAL
			    * IGMP_LAST_MEMBER_QUERY_COUNT,
			    0),
		    callback(member_query,
			     &MemberQuery::member_query_timer_timeout));
	    
	    // Send group-specific query
	    mld6igmp_send(primary_addr(),
			  member_query->group(),
			  IGMP_MEMBERSHIP_QUERY,
			  (IGMP_LAST_MEMBER_QUERY_INTERVAL * IGMP_TIMER_SCALE),
			  member_query->group());
	    member_query->_last_member_query_timer =
		mld6igmp_node().eventloop().new_oneoff_after(
		    TimeVal(IGMP_LAST_MEMBER_QUERY_INTERVAL, 0),
		    callback(member_query,
			     &MemberQuery::last_member_query_timer_timeout));
	}
	return (XORP_OK);
    }
    
    // Nothing found. Ignore.
    
    UNUSED(max_resp_time);
    UNUSED(buffer);
    
    return (XORP_OK);
}

/**
 * Mld6igmpVif::igmp_dvmrp_ask_neighbors_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The message type.
 * @max_resp_time: The Maximum Response Time from the IGMP header.
 * @group_address: The Group Address from the IGMP message.
 * @buffer: The buffer with the rest of the message.
 * 
 * Receive and process IGMP_DVMRP:DVMRP_ASK_NEIGHBORS message.
 * XXX: Some old DVMRP message from mrinfo(?).
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::igmp_dvmrp_ask_neighbors_recv(const IPvX& , // src
					   const IPvX& , // dst
					   uint8_t message_type,
					   int max_resp_time,
					   const IPvX& , // group_address
					   buffer_t *buffer)
{
    // Some old DVMRP messages from mrinfo(?)
    // TODO: do we really need this message implemented?
    
    // UNUSED(src);
    // UNUSED(dst);
    UNUSED(message_type);
    UNUSED(max_resp_time);
    // UNUSED(group_address);
    UNUSED(buffer);
    
    return (XORP_OK);
}

/**
 * Mld6igmpVif::igmp_dvmrp_ask_neighbors2_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The message type.
 * @max_resp_time: The Maximum Response Time from the IGMP header.
 * @group_address: The Group Address from the IGMP message.
 * @buffer: The buffer with the rest of the message.
 * 
 * Receive and process IGMP_DVMRP:DVMRP_ASK_NEIGHBORS2 message.
 * Used for mrinfo support.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::igmp_dvmrp_ask_neighbors2_recv(const IPvX& , // src
					    const IPvX& , // dst
					    uint8_t message_type,
					    int max_resp_time,
					    const IPvX& , // group_address
					    buffer_t *buffer)
{
    // TODO: implement it
    
    // UNUSED(src);
    // UNUSED(dst);
    UNUSED(message_type);
    UNUSED(max_resp_time);
    // UNUSED(group_address);
    UNUSED(buffer);
    
    return (XORP_OK);
}

/**
 * Mld6igmpVif::igmp_dvmrp_info_request_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The message type.
 * @max_resp_time: The Maximum Response Time from the IGMP header.
 * @group_address: The Group Address from the IGMP message.
 * @buffer: The buffer with the rest of the message.
 * 
 * Receive and process IGMP_DVMRP:DVMRP_INFO_REQUEST message.
 * Information request (TODO: used by mrinfo?).
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::igmp_dvmrp_info_request_recv(const IPvX& , // src
					  const IPvX& , // dst
					  uint8_t message_type,
					  int max_resp_time,
					  const IPvX& , // group_address
					  buffer_t *buffer)
{
    // TODO: implement it
    
    // UNUSED(src);
    // UNUSED(dst);
    UNUSED(message_type);
    UNUSED(max_resp_time);
    // UNUSED(group_address);
    UNUSED(buffer);
    
    return (XORP_OK);
}

/**
 * Mld6igmpVif::igmp_mtrace_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The message type.
 * @max_resp_time: The Maximum Response Time from the IGMP header.
 * @group_address: The Group Address from the IGMP message.
 * @buffer: The buffer with the rest of the message.
 * 
 * Receive and process IGMP_MTRACE message.
 * TODO: is it the new message sent by 'mtrace'??
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::igmp_mtrace_recv(const IPvX& , // src
			      const IPvX& , // dst
			      uint8_t message_type,
			      int max_resp_time,
			      const IPvX& , // group_address
			      buffer_t *buffer)
{
    // TODO: implement it
    
    // UNUSED(src);
    // UNUSED(dst);
    UNUSED(message_type);
    UNUSED(max_resp_time);
    // UNUSED(group_address);
    UNUSED(buffer);
    
    return (XORP_OK);
}

/**
 * igmp_v1_config_consistency_check:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The type of the IGMP message.
 * @igmp_message_version: The protocol version of the received message:
 * IGMP_V1 or IGMP_V2.
 * 
 * Check for IGMP protocol version interface configuration consistency.
 * For example, if the received message was IGMPv1, a correctly
 * configured local interface must be operating in IGMPv1 mode.
 * Similarly, if the local interface is operating in IGMPv1 mode,
 * all other neighbor routers (for that interface) must be
 * operating in IGMPv1 as well.
 * 
 * Return value: %XORP_OK if consistency, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::igmp_v1_config_consistency_check(const IPvX& src,
					      const IPvX& dst,
					      uint8_t message_type,
					      int igmp_message_version)
{
    switch (igmp_message_version) {
    case IGMP_V1:
	if (!is_igmpv1_mode()) {
	    // TODO: rate-limit the warning
	    XLOG_WARNING("RX %s from %s to %s on vif %s: "
			 "this interface is not in v1 mode",
			 proto_message_type2ascii(message_type),
			 cstring(src), cstring(dst),
			 name().c_str());
	    XLOG_WARNING("Please configure properly all routers on "
			 "that subnet to use IGMPv1");
	    return (XORP_ERROR);
	}
	break;
    default:
	if (is_igmpv1_mode()) {
	    // TODO: rate-limit the warning
	    XLOG_WARNING("RX %s(v2+) from %s to %s on vif %s: "
			 "this interface is not in V1 mode. "
			 "Please configure properly all routers on "
			 "that subnet to use IGMPv1.",
			 proto_message_type2ascii(message_type),
			 cstring(src), cstring(dst),
			 name().c_str());
	    return (XORP_ERROR);
	}
	break;
    }
    
    return (XORP_OK);
}

