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

#ident "$XORP: xorp/mld6igmp/mld6_proto.cc,v 1.36 2005/06/03 19:00:05 pavlin Exp $"


//
// Multicast Listener Discovery protocol implementation.
// MLDv1 (RFC 2710)
//


#include "mld6igmp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "mrt/inet_cksum.h"

#include "mld6_proto.h"
#include "mld6igmp_node.hh"
#include "mld6igmp_vif.hh"


#ifdef HAVE_IPV6_MULTICAST_ROUTING


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
 * Mld6igmpVif::mld6_process:
 * @src: The message source address.
 * @dst: The message destination address.
 * @ip_ttl: The IP TTL of the message. If it has a negative value,
 * it should be ignored.
 * @ip_tos: The IP TOS of the message. If it has a negative value,
 * it should be ignored.
 * @is_router_alert: True if the received IP packet had the Router Alert
 * IP option set.
 * @buffer: The buffer with the message.
 * @error_msg: The error message (if error).
 * 
 * Process MLD message and pass the control to the type-specific functions.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::mld6_process(const IPvX& src, const IPvX& dst,
			  int ip_ttl,
			  int ip_tos,
			  bool is_router_alert,
			  buffer_t *buffer,
			  string& error_msg)
{
    uint8_t message_type;
    int max_resp_time;
    IPvX group_address(family());
    uint16_t cksum;
    
    //
    // Message length check.
    //
    if (BUFFER_DATA_SIZE(buffer) < MLD_MINLEN) {
	error_msg = c_format("RX %s from %s to %s on vif %s: "
			     "too short data field (%u bytes)",
			     module_name(),
			     cstring(src), cstring(dst),
			     name().c_str(),
			     XORP_UINT_CAST(BUFFER_DATA_SIZE(buffer)));
	XLOG_WARNING("%s", error_msg.c_str());
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
    //
    // XXX: MLD messages do not have an explicit field for protocol version.
    // Protocol version check is performed later, per (some) message type.
    //
    
    //
    // Get the message type and the max. resp. time.
    //
    BUFFER_GET_OCTET(message_type, buffer);
    BUFFER_GET_SKIP(1, buffer);			// The `Code' field: unused
    BUFFER_GET_SKIP(2, buffer);			// The `Checksum' field
    BUFFER_GET_HOST_16(max_resp_time, buffer);
    BUFFER_GET_SKIP(2, buffer);			// The `Reserved' field
    BUFFER_GET_IPVX(family(), group_address, buffer);
    
    XLOG_TRACE(mld6igmp_node().is_log_trace(), 
	       "RX %s from %s to %s on vif %s",
	       proto_message_type2ascii(message_type),
	       cstring(src), cstring(dst),
	       name().c_str());
    
    //
    // IP Router Alert option check
    //
    if (_ip_router_alert_option_check.get()) {
	switch (message_type) {
	case MLD_LISTENER_QUERY:
	case MLD_LISTENER_REPORT:
	case MLD_LISTENER_DONE:
	    if (! is_router_alert) {
		error_msg = c_format("RX %s from %s to %s on vif %s: "
				     "missing IP Router Alert option",
				     proto_message_type2ascii(message_type),
				     cstring(src), cstring(dst),
				     name().c_str());
		XLOG_WARNING("%s", error_msg.c_str());
		return (XORP_ERROR);
	    }
	    //
	    // TODO: check the TTL and TOS if we are running in secure mode
	    //
	    UNUSED(ip_ttl);
	    UNUSED(ip_tos);
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
	    break;
	case MLD_MTRACE:
	    // TODO: perform the appropriate checks
	    break;
	default:
	    break;
	}
    }
    
    //
    // Source address check.
    //
    if (! src.is_unicast()) {
	// Source address must always be unicast
	// The kernel should have checked that, but just in case
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
    switch (message_type) {
    case MLD_LISTENER_QUERY:
    case MLD_LISTENER_REPORT:
    case MLD_LISTENER_DONE:
	// XXX: the source address must be a link-local address
	if (! src.is_linklocal_unicast()) {
	    error_msg = c_format("RX %s from %s to %s on vif %s: "
				 "source is not a link-local address",
				 proto_message_type2ascii(message_type),
				 cstring(src), cstring(dst),
				 name().c_str());
	    XLOG_WARNING("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
	break;
    case MLD_MTRACE:
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
    case MLD_LISTENER_QUERY:
    case MLD_LISTENER_REPORT:
    case MLD_LISTENER_DONE:
	// Destination must be multicast
	if (! dst.is_multicast()) {
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
	// TODO: Multicast address scope check for IPv6
	//
	break;
    case MLD_MTRACE:
	// TODO: perform the appropriate checks
	break;
    default:
	break;
    }
    
    //
    // Message-specific checks.
    //
    switch (message_type) {
    case MLD_LISTENER_QUERY:
    case MLD_LISTENER_REPORT:
    case MLD_LISTENER_DONE:
	// Inner multicast address scope check
	if (group_address.is_nodelocal_multicast()) {
	    error_msg = c_format("RX %s from %s to %s on vif %s: "
				 "invalid node-local scope of inner "
				 "multicast address: %s",
				 proto_message_type2ascii(message_type),
				 cstring(src), cstring(dst),
				 name().c_str(),
				 cstring(group_address));
	    XLOG_WARNING("%s", error_msg.c_str());
	    return (XORP_ERROR);
	}
	break;
    case MLD_MTRACE:
	/* TODO: perform the appropriate checks */
	break;
    default:
	break;
    }
    
    //
    // Origin router neighbor check.
    //
    // XXX: in MLD we don't need such check
    
    //
    // Process each message, based on its type.
    //
    switch (message_type) {
    case MLD_LISTENER_QUERY:
	mld6_listener_query_recv(src, dst,
				 message_type, max_resp_time,
				 group_address, buffer);
	break;
    case MLD_LISTENER_REPORT:
	mld6_listener_report_recv(src, dst,
				  message_type, max_resp_time,
				  group_address, buffer);
	break;
    case MLD_LISTENER_DONE:
	mld6_listener_done_recv(src, dst,
				message_type, max_resp_time,
				group_address, buffer);
	break;
    case MLD_MTRACE:
	mld6_mtrace_recv(src, dst,
			 message_type, max_resp_time,
			 group_address, buffer);
	break;
    default:
	break;
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
 * Mld6igmpVif::mld6_listener_query_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The message type.
 * @max_resp_time: The Maximum Response Time from the MLD header.
 * @group_address: The Group Address from the MLD message.
 * @buffer: The buffer with the rest of the message.
 * 
 * Receive and process MLD_LISTENER_QUERY message from another router.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::mld6_listener_query_recv(const IPvX& src,
				      const IPvX& , // dst
				      uint8_t message_type,
				      int max_resp_time,
				      const IPvX& group_address,
				      buffer_t *buffer)
{
    // Ignore my own queries
    if (mld6igmp_node().is_my_addr(src))
	return (XORP_ERROR);
    
    //
    // Compare this querier address with my address
    // XXX: here we should compare the old and new querier
    // addresses, but we don't really care.
    //
    XLOG_ASSERT(primary_addr() != IPvX::ZERO(family()));
    if (src < primary_addr()) {
	// New querier
	_query_timer.unschedule();
	set_querier_addr(src);
	_proto_flags &= ~MLD6IGMP_VIF_QUERIER;
	TimeVal other_querier_present_interval =
	    static_cast<int>(robust_count().get()) * query_interval().get()
	    + query_response_interval().get() / 2;
	_other_querier_timer =
	    mld6igmp_node().eventloop().new_oneoff_after(
		other_querier_present_interval,
		callback(this, &Mld6igmpVif::other_querier_timer_timeout));
    }
    
    //
    // From RFC 2710:
    // "When a router in Non-Querier state receives a Multicast-Address-
    // Specific Query, if its timer value for the identified multicast
    // address is greater than [Last Listener Query Count] times the Maximum
    // Response Delay specified in the message, it sets the address's timer
    // to that latter value."
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

	    // "Last Listener Query Count"
	    sec = (robust_count().get() * max_resp_time) / MLD_TIMER_SCALE;
	    usec = (robust_count().get() * max_resp_time) % MLD_TIMER_SCALE;
	    usec *= (1000000 / MLD_TIMER_SCALE); // microseconds
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
    UNUSED(message_type);
    UNUSED(buffer);
    
    return (XORP_OK);
}

/**
 * Mld6igmpVif::mld6_listener_report_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The message type.
 * @max_resp_time: The Maximum Response Time from the MLD header.
 * @group_address: The Group Address from the MLD message.
 * @buffer: The buffer with the rest of the message.
 * 
 * Receive and process MLD_LISTENER_REPORT
 * message from a host.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::mld6_listener_report_recv(const IPvX& src,
				       const IPvX& dst,
				       uint8_t message_type,
				       int max_resp_time,
				       const IPvX& group_address,
				       buffer_t *buffer)
{
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

    TimeVal group_membership_interval
	= static_cast<int>(robust_count().get()) * query_interval().get()
	+ query_response_interval().get();
    member_query->_member_query_timer =
	mld6igmp_node().eventloop().new_oneoff_after(
	    group_membership_interval,
	    callback(member_query, &MemberQuery::member_query_timer_timeout));

    return (XORP_OK);
    
    UNUSED(max_resp_time);
    UNUSED(buffer);
}

/**
 * Mld6igmpVif::mld6_listener_done_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The message type.
 * @max_resp_time: The Maximum Response Time from the MLD header.
 * @group_address: The Group Address from the MLD message.
 * @buffer: The buffer with the rest of the message.
 * 
 * Receive and process MLD_LISTENER_DONE message from a host.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::mld6_listener_done_recv(const IPvX& src,
				     const IPvX& dst,
				     uint8_t message_type,
				     int max_resp_time,
				     const IPvX& group_address,
				     buffer_t *buffer)
{
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
    
    // Find if this group already has an entry
    list<MemberQuery *>::iterator iter;
    for (iter = _members.begin(); iter != _members.end(); ++iter) {
	MemberQuery *member_query = *iter;
	if (group_address != member_query->group())
	    continue;

	// Group found
	if (_proto_flags & MLD6IGMP_VIF_QUERIER) {
	    member_query->_member_query_timer =
		mld6igmp_node().eventloop().new_oneoff_after(
		    (query_last_member_interval().get()
		     * static_cast<int>(robust_count().get())), // "Last Listener Query Count"
		    callback(member_query,
			     &MemberQuery::member_query_timer_timeout));
	    
	    // Send group-specific query
	    TimeVal scaled_max_resp_time =
		query_last_member_interval().get() * MLD_TIMER_SCALE;
	    mld6igmp_send(primary_addr(),
			  member_query->group(),
			  MLD_LISTENER_QUERY,
			  scaled_max_resp_time.sec(),
			  member_query->group(),
			  dummy_error_msg);
	    member_query->_last_member_query_timer =
		mld6igmp_node().eventloop().new_oneoff_after(
		    query_last_member_interval().get(),
		    callback(member_query,
			     &MemberQuery::last_member_query_timer_timeout));
	}
	return (XORP_OK);
    }
    
    // Nothing found. Ignore.
    
    UNUSED(max_resp_time);
    UNUSED(buffer);
    
    return (XORP_ERROR);
}

/**
 * Mld6igmpVif::mld6_mtrace_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The message type.
 * @max_resp_time: The Maximum Response Time from the MLD header.
 * @group_address: The Group Address from the MLD message.
 * @buffer: The buffer with the rest of the message.
 * 
 * Receive and process MLD_MTRACE message.
 * TODO: is it the new message sent by 'mtrace'??
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::mld6_mtrace_recv(const IPvX& , // src
			      const IPvX& , // dst
			      uint8_t message_type,
			      int max_resp_time,
			      const IPvX& , // group_address
			      buffer_t *buffer)
{
    /* TODO: implement it */
    
    // UNUSED(src);
    // UNUSED(dst);
    UNUSED(message_type);
    UNUSED(max_resp_time);
    // UNUSED(group_address);
    UNUSED(buffer);
    
    return (XORP_OK);
}

#endif // HAVE_IPV6_MULTICAST_ROUTING
