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

#ident "$XORP: xorp/mld6igmp/mld6_proto.cc,v 1.7 2003/03/14 03:13:47 pavlin Exp $"


//
// Multicast Listener Discovery protocol implementation.
// MLDv1 (RFC 2710)
//


#include "mld6igmp_module.h"
#include "mld6igmp_private.hh"
#include "mrt/inet_cksum.h"
#include "mld6_proto.h"
#include "mld6igmp_vif.hh"


#ifdef HAVE_IPV6


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
 * @buffer: The buffer with the message.
 * 
 * Process MLD6 message and pass the control to the type-specific functions.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
Mld6igmpVif::mld6_process(const IPvX& src, const IPvX& dst, buffer_t *buffer)
{
    uint8_t message_type;
    int max_resp_time;
    IPvX group_address(family());
    struct in6_addr in6_addr;
    uint16_t cksum;
    
    //
    // Message length check.
    //
    if (BUFFER_DATA_SIZE(buffer) < sizeof(struct mld6_hdr)) {
	XLOG_WARNING("RX %s from %s to %s: "
		     "too short data field (%u bytes)",
		     module_name(),
		     cstring(src), cstring(dst),
		     (uint32_t)BUFFER_DATA_SIZE(buffer));
	return (XORP_ERROR);
    }
    
    //
    // Checksum verification.
    //
    cksum = INET_CKSUM(BUFFER_DATA_HEAD(buffer), BUFFER_DATA_SIZE(buffer));
#ifdef HAVE_IPV6
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
#endif // HAVE_IPV6
    if (cksum) {
	XLOG_WARNING("RX %s from %s to %s: "
		     "checksum error",
		     module_name(),
		     cstring(src), cstring(dst));
	return (XORP_ERROR);
    }
    
    //
    // Protocol version check.
    //
    //
    // XXX: MLD6 messages do not have an explicit field for protocol version.
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
    
    XLOG_TRACE(mld6igmp_node().is_log_trace(), "RX %s from %s to %s",
	       proto_message_type2ascii(message_type),
	       cstring(src),
	       cstring(dst));
    
#if 0	// TODO: do we need the TTL and Router Alert option checks?
    //
    // TTL (aka. Hop-limit in IPv6) and Router Alert option check.
    //
    switch (message_type) {
    case MLD6_LISTENER_QUERY:
    case MLD6_LISTENER_REPORT:
    case MLD6_LISTENER_DONE:
	if (ip_ttl != 1) {
	    XLOG_WARNING("RX %s from %s to %s: "
			 "ip_ttl = %d instead of %d",
			 proto_message_type2ascii(message_type),
			 cstring(src), cstring(dst), ip_ttl, 1);
	    return (XORP_ERROR);
	}
	//
	// TODO: check for Router Alert option and ignore message
	// if the option is missing and we are running in secure mode.
	//
	break;
    case MLD6_MTRACE:
	// TODO: perform the appropriate checks
	break;
    default:
	break;
    }
#endif // 0/1
    
#if 1	// TODO: do we need unicast/multicast destination address check?
    //
    // Source and destination address check.
    //
    switch (message_type) {
    case MLD6_LISTENER_QUERY:
    case MLD6_LISTENER_REPORT:
    case MLD6_LISTENER_DONE:
	// Destination must be multicast
	if (! dst.is_multicast()) {
	    XLOG_WARNING("RX %s from %s to %s: "
			 "destination must be multicast. "
			 "Packet ignored.",
			 proto_message_type2ascii(message_type),
			 cstring(src), cstring(dst));
	    return (XORP_ERROR);
	}
	// Source address check
	src.copy_out(in6_addr);
	if (! IN6_IS_ADDR_LINKLOCAL(&in6_addr)) {
	    XLOG_WARNING("RX %s from %s to %s: "
			 "source is not a link-local address",
			 proto_message_type2ascii(message_type),
			 cstring(src), cstring(dst));
	    return (XORP_ERROR);
	}
	//
	// TODO: Multicast address scope check for IPv6
	//
	break;
    case MLD6_MTRACE:
	// TODO: perform the appropriate checks
	break;
    default:
	break;
    }
#endif /* 1/0 */

    
    //
    // Message-specific checks.
    //
    switch (message_type) {
    case MLD6_LISTENER_QUERY:
    case MLD6_LISTENER_REPORT:
    case MLD6_LISTENER_DONE:
	// Inner multicast address scope check
	group_address.copy_out(in6_addr);
	if (IN6_IS_ADDR_MC_NODELOCAL(&in6_addr)) {
	    XLOG_WARNING("RX %s from %s to %s: "
			 "invalid scope of inner multicast address: %s",
			 proto_message_type2ascii(message_type),
			 cstring(src), cstring(dst),
			 cstring(group_address));
	    return (XORP_ERROR);
	}
	break;
    case MLD6_MTRACE:
	/* TODO: perform the appropriate checks */
	break;
    default:
	break;
    }
    
    //
    // Origin router neighbor check.
    //
    // XXX: in MLD6 we don't need such check
    
    //
    // Process each message, based on its type.
    //
    switch (message_type) {
    case MLD6_LISTENER_QUERY:
	mld6_listener_query_recv(src, dst,
				 message_type, max_resp_time,
				 group_address, buffer);
	break;
    case MLD6_LISTENER_REPORT:
	mld6_listener_report_recv(src, dst,
				  message_type, max_resp_time,
				  group_address, buffer);
	break;
    case MLD6_LISTENER_DONE:
	mld6_listener_done_recv(src, dst,
				message_type, max_resp_time,
				group_address, buffer);
	break;
    case MLD6_MTRACE:
	mld6_mtrace_recv(src, dst,
			 message_type, max_resp_time,
			 group_address, buffer);
	break;
    default:
	break;
    }
    
    return (XORP_OK);
    
 rcvlen_error:
    XLOG_ASSERT(false);
    XLOG_WARNING("RX %s packet from %s to %s: "
		 "some fields are too short",
		 module_name(),
		 cstring(src), cstring(dst));
    return (XORP_ERROR);
}

/**
 * Mld6igmpVif::mld6_listener_query_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The message type.
 * @max_resp_time: The Maximum Response Time from the MLD6 header.
 * @group_address: The Group Address from the MLD6 message.
 * @buffer: The buffer with the rest of the message.
 * 
 * Receive and process MLD6_LISTENER_QUERY message from another router.
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
    if (mld6igmp_node().vif_find_by_addr(src))
	return (XORP_ERROR);
    
    // Compare this querier address with my address
    // XXX: here we should compare the old and new querier
    // addresses, but we don't really care.
    if (src < *addr_ptr()) {
	// New querier
	_query_timer.cancel();
	_querier_addr = src;
	_proto_flags &= ~MLD6IGMP_VIF_QUERIER;
	_other_querier_timer.start(MLD6_OTHER_QUERIER_PRESENT_INTERVAL, 0,
				   mld6_other_querier_timeout, this);
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
	    struct timeval received_resp_timeval;
	    struct timeval left_resp_timeval;
	    
	    sec = (MLD6_LAST_LISTENER_QUERY_COUNT * max_resp_time)
		/ MLD6_TIMER_SCALE;
	    usec = (MLD6_LAST_LISTENER_QUERY_COUNT * max_resp_time)
		% MLD6_TIMER_SCALE;
	    usec *= (1000000 / MLD6_TIMER_SCALE); // microseconds
	    TIMEVAL_SET(&received_resp_timeval, sec, usec);
	    member_query->_member_query_timer.left_timeval(&left_resp_timeval);
	    
	    if (TIMEVAL_CMP(&left_resp_timeval, &received_resp_timeval, >)) {
		member_query->_member_query_timer.start(
		    sec, usec, mld6_member_query_timeout, member_query);
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
 * @max_resp_time: The Maximum Response Time from the MLD6 header.
 * @group_address: The Group Address from the MLD6 message.
 * @buffer: The buffer with the rest of the message.
 * 
 * Receive and process MLD6_LISTENER_REPORT
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
	XLOG_WARNING("RX %s from %s to %s: "
		     "the group address %s is not "
		     "valid multicast address",
		     proto_message_type2ascii(message_type),
		     cstring(src), cstring(dst),
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
	    member_query->_last_member_query_timer.cancel();
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
    
    member_query->_member_query_timer.start(MLD6_MULTICAST_LISTENER_INTERVAL,
					    0, mld6_member_query_timeout,
					    member_query);
    
    UNUSED(max_resp_time);
    UNUSED(buffer);
    
    return (XORP_OK);
}

/**
 * Mld6igmpVif::mld6_listener_done_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The message type.
 * @max_resp_time: The Maximum Response Time from the MLD6 header.
 * @group_address: The Group Address from the MLD6 message.
 * @buffer: The buffer with the rest of the message.
 * 
 * Receive and process MLD6_LISTENER_DONE message from a host.
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
    // The group address must be a valid multicast address
    if (! group_address.is_multicast()) {
	XLOG_WARNING("RX %s from %s to %s: "
		     "the group address %s is not "
		     "valid multicast address",
		     proto_message_type2ascii(message_type),
		     cstring(src), cstring(dst),
		     cstring(group_address));
	return (XORP_ERROR);
    }
    
    // Find if this group already has an entry
    list<MemberQuery *>::iterator iter;
    for (iter = _members.begin(); iter != _members.end(); ++iter) {
	MemberQuery *member_query = *iter;
	if (group_address == member_query->group()) {
	    // Group found
	    if (_proto_flags & MLD6IGMP_VIF_QUERIER) {
		member_query->_member_query_timer.start(
		    (MLD6_LAST_LISTENER_QUERY_INTERVAL
		     * MLD6_LAST_LISTENER_QUERY_COUNT), 0,
		    mld6_member_query_timeout,
		    member_query);
		
		// Send group-specific query
		mld6igmp_send(member_query->group(),
			      MLD6_LISTENER_QUERY,
			      (MLD6_LAST_LISTENER_QUERY_INTERVAL
			       * MLD6_TIMER_SCALE),
			      member_query->group());
		member_query->_last_member_query_timer.start(
		    MLD6_LAST_LISTENER_QUERY_INTERVAL, 0,
		    mld6_last_member_query_timeout, member_query);
	    }
	    return (XORP_OK);
	}
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
 * @max_resp_time: The Maximum Response Time from the MLD6 header.
 * @group_address: The Group Address from the MLD6 message.
 * @buffer: The buffer with the rest of the message.
 * 
 * Receive and process MLD6_MTRACE message.
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

/**
 * mld6_query_timeout:
 * @data_pointer: The #Mld6igmpVif interface to send the query on.
 * 
 * Timeout: time to send a membership query.
 **/
void
mld6_query_timeout(void *data_pointer)
{
    Mld6igmpVif *mld6igmp_vif;
    
    mld6igmp_vif = (Mld6igmpVif *)data_pointer;
    mld6igmp_vif->mld6_query_timeout_process();
}

/**
 * Mld6igmpVif::igmp_query_timeout_process:
 * @void:
 *
 * Send a membership query.
 **/
void
Mld6igmpVif::mld6_query_timeout_process(void)
{
    IPvX ipaddr_zero(family());			// XXX: ANY
    int query_interval;
    
    if (!(_proto_flags & MLD6IGMP_VIF_QUERIER))
	return;		// I am not the querier anymore. Ignore.
    
    // Send a general membership query
    mld6igmp_send(IPvX::MULTICAST_ALL_SYSTEMS(family()),
		  MLD6_LISTENER_QUERY,
		  is_igmpv1_mode() ? 0:
		  (MLD6_QUERY_RESPONSE_INTERVAL * MLD6_TIMER_SCALE),
		  ipaddr_zero);
    if (_startup_query_count > 0)
	_startup_query_count--;
    if (_startup_query_count > 0)
	query_interval = MLD6_STARTUP_QUERY_INTERVAL;
    else
	query_interval = MLD6_QUERY_INTERVAL;
    _query_timer.start(query_interval, 0, mld6_query_timeout, this);
}

/**
 * mld6_last_member_query_timeout:
 * @data_pointer: The #MemberQuery entry that has expired.
 * 
 * Timeout: the last group member has expired or has left the group. Quickly
 * query if there are other members for that group.
 * XXX: a different timer (member_query_timer) will stop the process
 * and will cancel this timer `last_member_query_timer'.
 **/
void
mld6_last_member_query_timeout(void *data_pointer)
{
    MemberQuery *member_query = (MemberQuery *)data_pointer;
    Mld6igmpVif& mld6igmp_vif = member_query->mld6igmp_vif();
    
    //
    // XXX: The spec says that we shouldn't care if we changed
    // from a Querier to a non-Querier. Hence, send the group-specific
    // query (see the bottom part of Section 4.)
    //
    mld6igmp_vif.mld6igmp_send(member_query->group(),
			       MLD6_LISTENER_QUERY,
			       (MLD6_LAST_LISTENER_QUERY_INTERVAL
				* MLD6_TIMER_SCALE),
			       member_query->group());
    member_query->_last_member_query_timer.start(
	MLD6_LAST_LISTENER_QUERY_INTERVAL, 0,
	mld6_last_member_query_timeout, member_query);
}

/**
 * mld6_member_query_timeout:
 * @data_pointer: The #MemberQuery entry that has expired.
 * 
 * Timeout: expire a multicast group entry.
 **/
void
mld6_member_query_timeout(void *data_pointer)
{
    MemberQuery *member_query = (MemberQuery *)data_pointer;
    Mld6igmpVif& mld6igmp_vif = member_query->mld6igmp_vif();
    
    member_query->_last_member_query_timer.cancel();
    member_query->_igmpv1_host_present_timer.cancel();
    
    // notify routing (-)
    mld6igmp_vif.join_prune_notify_routing(member_query->group(),
					   member_query->group(),
					   ACTION_PRUNE);
    
    // Remove the entry
    list<MemberQuery *>::iterator iter;
    for (iter = mld6igmp_vif._members.begin();
	 iter != mld6igmp_vif._members.end(); ++iter) {
	if (*iter == member_query) {
	    mld6igmp_vif._members.erase(iter);
	    delete member_query;
	    break;
	}
    }
}

/**
 * mld6_other_querier_timeout:
 * @data_pointer: The #Mld6igmpVif interface to become a querier on.
 * 
 * Timeout: the previous querier has expired. I will become the querier.
 **/
void
mld6_other_querier_timeout(void *data_pointer)
{
    Mld6igmpVif *mld6igmp_vif = (Mld6igmpVif *)data_pointer;
    IPvX ipaddr_zero(mld6igmp_vif->family());	// XXX: ANY
    
    mld6igmp_vif->set_querier_addr(*mld6igmp_vif->addr_ptr());
    mld6igmp_vif->_proto_flags |= MLD6IGMP_VIF_QUERIER;
    
    // Now I am the querier. Send a general membership query.
    mld6igmp_vif->mld6igmp_send(IPvX::MULTICAST_ALL_SYSTEMS(mld6igmp_vif->family()),
				MLD6_LISTENER_QUERY,
				(MLD6_QUERY_RESPONSE_INTERVAL
				 * MLD6_TIMER_SCALE),
				ipaddr_zero);
    mld6igmp_vif->_startup_query_count = 0;	// XXX: not a startup case
    mld6igmp_vif->_query_timer.start(MLD6_QUERY_INTERVAL, 0,
				     mld6_query_timeout, mld6igmp_vif);
}

#endif /* HAVE_IPV6 */
