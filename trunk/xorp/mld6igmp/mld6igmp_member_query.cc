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

#ident "$XORP: xorp/mld6igmp/mld6igmp_member_query.cc,v 1.7 2003/11/12 19:10:07 pavlin Exp $"

//
// Multicast group membership information used by
// MLDv1 (RFC 2710), IGMPv1 and IGMPv2 (RFC 2236).
//


#include "mld6igmp_module.h"
#include "mld6igmp_private.hh"
#include "mld6igmp_member_query.hh"
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
 * MemberQuery::MemberQuery:
 * @mld6igmp_vif: The vif interface this entry belongs to.
 * @source: The entry source address. It could be NULL for (*,G) entries.
 * @group: The entry group address.
 * 
 * Create a (S,G) or (*,G) entry used by IGMP or MLD to query host members.
 * 
 * Return value: 
 **/
MemberQuery::MemberQuery(Mld6igmpVif& mld6igmp_vif, const IPvX& source,
			 const IPvX& group)
    : _mld6igmp_vif(mld6igmp_vif),
      _source(source),
      _group(group)
{
    
}

/**
 * MemberQuery::~MemberQuery:
 * @: 
 * 
 * MemberQuery destrictor.
 **/
MemberQuery::~MemberQuery()
{
    // TODO: Notify routing (-)
    // TODO: ??? Maybe not the right place, or this should
    // be the only place to use ACTION_PRUNE notification??
    // join_prune_notify_routing(source(), group(), ACTION_PRUNE);
}

/**
 * MemberQuery::timeout_sec:
 * @: 
 * 
 * Get the number of seconds until time to query for host members.
 * Return value: the number of seconds until time to query for host members.
 **/
uint32_t
MemberQuery::timeout_sec() const
{
    TimeVal tv;
    
    _member_query_timer.time_remaining(tv);
    
    return (tv.sec());
}

/**
 * MemberQuery::member_query_timer_timeout:
 * 
 * Timeout: expire a multicast group entry.
 **/
void
MemberQuery::member_query_timer_timeout()
{
    _last_member_query_timer.unschedule();
    if (mld6igmp_vif().mld6igmp_node().proto_is_igmp())
	_igmpv1_host_present_timer.unschedule();
    
    // notify routing (-)
    mld6igmp_vif().join_prune_notify_routing(source(),
					     group(),
					     ACTION_PRUNE);
    
    // Remove the entry 
    list<MemberQuery *>::iterator iter;
    for (iter = mld6igmp_vif()._members.begin();
	 iter != mld6igmp_vif()._members.end();
	 ++iter) {
	if (*iter == this) {
	    mld6igmp_vif()._members.erase(iter);
	    delete this;
	    return;
	}
    }
}

/**
 * MemberQuery::last_member_query_timer_timeout:
 * 
 * Timeout: the last group member has expired or has left the group. Quickly
 * query if there are other members for that group.
 * XXX: a different timer (member_query_timer) will stop the process
 * and will cancel this timer `last_member_query_timer'.
 **/
void
MemberQuery::last_member_query_timer_timeout()
{
    //
    // XXX: The spec says that we shouldn't care if we changed
    // from a Querier to a non-Querier. Hence, send the group-specific
    // query (see the bottom part of Section 4.)
    //
    if (mld6igmp_vif().proto_is_igmp()) {
	// TODO: XXX: ignore the fact that now there may be IGMPv1 routers?
	mld6igmp_vif().mld6igmp_send(mld6igmp_vif().primary_addr(),
				     group(),
				     IGMP_MEMBERSHIP_QUERY,
				     (IGMP_LAST_MEMBER_QUERY_INTERVAL
				      * IGMP_TIMER_SCALE),
				     group());
	_last_member_query_timer =
	    mld6igmp_vif().mld6igmp_node().eventloop().new_oneoff_after(
		TimeVal(IGMP_LAST_MEMBER_QUERY_INTERVAL, 0),
		callback(this, &MemberQuery::last_member_query_timer_timeout));
    }

#ifdef HAVE_IPV6_MULTICAST_ROUTING
    if (mld6igmp_vif().proto_is_mld6()) {
	mld6igmp_vif().mld6igmp_send(mld6igmp_vif().primary_addr(),
				     group(),
				     MLD_LISTENER_QUERY,
				     (MLD_LAST_LISTENER_QUERY_INTERVAL
				      * MLD_TIMER_SCALE),
				     group());
	_last_member_query_timer =
	    mld6igmp_vif().mld6igmp_node().eventloop().new_oneoff_after(
		TimeVal(MLD_LAST_LISTENER_QUERY_INTERVAL, 0),
		callback(this, &MemberQuery::last_member_query_timer_timeout));
    }
#endif // HAVE_IPV6_MULTICAST_ROUTING
}
