// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2006 International Computer Science Institute
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

#ident "$XORP: xorp/mld6igmp/mld6igmp_proto.cc,v 1.19 2006/03/16 00:04:44 pavlin Exp $"


//
// Internet Group Management Protocol implementation.
// IGMPv1 and IGMPv2 (RFC 2236)
//
// AND
//
// Multicast Listener Discovery protocol implementation.
// MLDv1 (RFC 2710)
//



#include "mld6igmp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

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
 * @max_resp_time: The Maximum Response Time from the MLD/IGMP header.
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
					    int max_resp_time,
					    const IPvX& group_address,
					    buffer_t *buffer)
{
    // Ignore my own queries
    if (mld6igmp_node().is_my_addr(src))
	return (XORP_ERROR);

#ifdef HAVE_IPV4_MULTICAST_ROUTING
    if (proto_is_igmp()) {
	int igmp_message_version = IGMP_V2;

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
    }
#endif // HAVE_IPV4_MULTICAST_ROUTING
    
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
    if ( (!group_address.is_zero())
	 && (max_resp_time != 0)
	 && !(_proto_flags & MLD6IGMP_VIF_QUERIER)) {
	// Find if we already have an entry for this group
	
	list<MemberQuery *>::iterator iter;
	for (iter = _members.begin(); iter != _members.end(); ++iter) {
	    MemberQuery *member_query = *iter;
	    if (group_address != member_query->group())
		continue;
	    
	    // Group found
	    uint32_t sec, usec;
	    uint32_t timer_scale = mld6igmp_constant_timer_scale();
	    TimeVal received_resp_tv;
	    TimeVal left_resp_tv;

	    // "Last Member Query Count" / "Last Listener Query Count"
	    sec = (robust_count().get() * max_resp_time) / timer_scale;
	    usec = (robust_count().get() * max_resp_time) % timer_scale;
	    usec *= (1000000 / timer_scale); // microseconds
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
    
    UNUSED(buffer);
    
    return (XORP_OK);
}

/**
 * Mld6igmpVif::mld6igmp_membership_report_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The message type.
 * @max_resp_time: The Maximum Response Time from the MLD/IGMP header.
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
					     int max_resp_time,
					     const IPvX& group_address,
					     buffer_t *buffer)
{
    MemberQuery *member_query = NULL;
    IPvX source_address = IPvX::ZERO(family());		// XXX: ANY
    
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

#ifdef HAVE_IPV4_MULTICAST_ROUTING
    if (proto_is_igmp()) {
	int igmp_message_version = IGMP_V2;
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
		    group_membership_interval,
		    &_dummy_flag);
	}
    }
#endif // HAVE_IPV4_MULTICAST_ROUTING

    UNUSED(max_resp_time);
    UNUSED(buffer);

    return (XORP_OK);
}

/**
 * Mld6igmpVif::mld6igmp_leave_group_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @message_type: The message type.
 * @max_resp_time: The Maximum Response Time from the MLD/IGMP header.
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
				       int max_resp_time,
				       const IPvX& group_address,
				       buffer_t *buffer)
{
    string dummy_error_msg;

    if (proto_is_igmp()) {
	if (is_igmpv1_mode()) {
	    //
	    // Ignore this 'Leave Group' message because this
	    // interface is operating in IGMPv1 mode.
	    //
	    return (XORP_OK);
	}
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
	if (proto_is_igmp()) {
	    if (member_query->_igmpv1_host_present_timer.scheduled()) {
		//
		// Ignore this 'Leave Group' message because this
		// group has IGMPv1 hosts members.
		//
		return (XORP_OK);
	    }
	}
	if (_proto_flags & MLD6IGMP_VIF_QUERIER) {
	    // "Last Member Query Count" / "Last Listener Query Count"
	    uint32_t query_count = robust_count().get();

	    member_query->_member_query_timer =
		mld6igmp_node().eventloop().new_oneoff_after(
		    (query_last_member_interval().get() * query_count),
		    callback(member_query,
			     &MemberQuery::member_query_timer_timeout));

	    //
	    // Send group-specific query
	    //
	    uint32_t timer_scale = mld6igmp_constant_timer_scale();
	    TimeVal scaled_max_resp_time =
		(query_last_member_interval().get() * timer_scale);
	    mld6igmp_send(primary_addr(),
			  member_query->group(),
			  mld6igmp_constant_membership_query(),
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
    
    return (XORP_OK);
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
    _proto_flags |= MLD6IGMP_VIF_QUERIER;

    //
    // Now I am the querier. Send a general membership query.
    //
    uint32_t timer_scale = mld6igmp_constant_timer_scale();
    TimeVal scaled_max_resp_time =
	(query_response_interval().get() * timer_scale);
    mld6igmp_send(primary_addr(),
		  IPvX::MULTICAST_ALL_SYSTEMS(family()),
		  mld6igmp_constant_membership_query(),
		  is_igmpv1_mode() ? 0: scaled_max_resp_time.sec(),
		  IPvX::ZERO(family()),			// XXX: ANY
		  dummy_error_msg);
    _startup_query_count = 0;		// XXX: not a startup case
    _query_timer = mld6igmp_node().eventloop().new_oneoff_after(
	query_interval().get(),
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

    UNUSED(interval);
    UNUSED(dummy_error_msg);

    if (! (_proto_flags & MLD6IGMP_VIF_QUERIER))
	return;		// I am not the querier anymore. Ignore.

    //
    // Send a general membership query
    //
    uint32_t timer_scale = mld6igmp_constant_timer_scale();
    TimeVal scaled_max_resp_time =
	(query_response_interval().get() * timer_scale);

    mld6igmp_send(primary_addr(),
		  IPvX::MULTICAST_ALL_SYSTEMS(family()),
		  mld6igmp_constant_membership_query(),
		  is_igmpv1_mode() ? 0: scaled_max_resp_time.sec(),
		  IPvX::ZERO(family()),			// XXX: ANY
		  dummy_error_msg);
    if (_startup_query_count > 0)
	_startup_query_count--;
    if (_startup_query_count > 0)
	interval = query_interval().get() / 4; // "Startup Query Interval"
    else
	interval = query_interval().get();

    _query_timer = mld6igmp_node().eventloop().new_oneoff_after(
	interval,
	callback(this, &Mld6igmpVif::query_timer_timeout));
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
    if (! proto_is_igmp())
	return (XORP_OK);

    switch (igmp_message_version) {
    case IGMP_V1:
	if (! is_igmpv1_mode()) {
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
