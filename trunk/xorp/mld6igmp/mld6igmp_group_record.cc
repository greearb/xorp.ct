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

#ident "$XORP: xorp/mld6igmp/mld6igmp_member_query.cc,v 1.19 2006/06/06 23:09:04 pavlin Exp $"

//
// Multicast group membership information used by
// MLDv1 (RFC 2710), IGMPv1 and IGMPv2 (RFC 2236).
//


#include "mld6igmp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "mld6igmp_group_record.hh"
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
 * Mld6igmpGroupRecord::Mld6igmpGroupRecord:
 * @mld6igmp_vif: The vif interface this entry belongs to.
 * @group: The entry group address.
 * 
 * Create a (S,G) or (*,G) entry used by IGMP or MLD to query host members.
 * 
 * Return value: 
 **/
Mld6igmpGroupRecord::Mld6igmpGroupRecord(Mld6igmpVif& mld6igmp_vif,
					 const IPvX& group)
    : _mld6igmp_vif(mld6igmp_vif),
      _group(group)
{
    
}

/**
 * Mld6igmpGroupRecord::~Mld6igmpGroupRecord:
 * @: 
 * 
 * Mld6igmpGroupRecord destrictor.
 **/
Mld6igmpGroupRecord::~Mld6igmpGroupRecord()
{
    // TODO: Notify routing (-)
    // TODO: ??? Maybe not the right place, or this should
    // be the only place to use ACTION_PRUNE notification??
    // join_prune_notify_routing(IPvX::ZERO(family()), group(), ACTION_PRUNE);
}

/**
 * Mld6igmpGroupRecord::timeout_sec:
 * @: 
 * 
 * Get the number of seconds until time to query for host members.
 * Return value: the number of seconds until time to query for host members.
 **/
uint32_t
Mld6igmpGroupRecord::timeout_sec() const
{
    TimeVal tv;
    
    _member_query_timer.time_remaining(tv);
    
    return (tv.sec());
}

/**
 * Mld6igmpGroupRecord::member_query_timer_timeout:
 * 
 * Timeout: expire a multicast group entry.
 **/
void
Mld6igmpGroupRecord::member_query_timer_timeout()
{
    _last_member_query_timer.unschedule();
    if (mld6igmp_vif().mld6igmp_node().proto_is_igmp())
	_igmpv1_host_present_timer.unschedule();
    
    // notify routing (-)
    mld6igmp_vif().join_prune_notify_routing(IPvX::ZERO(family()),
					     group(),
					     ACTION_PRUNE);
    
    // Remove the entry 
    map<IPvX, Mld6igmpGroupRecord *>::iterator iter;
    iter = mld6igmp_vif().members().find(group());
    if (iter != mld6igmp_vif().members().end()) {
	mld6igmp_vif().members().erase(iter);
	delete this;
	return;
    }
}

/**
 * Mld6igmpGroupRecord::last_member_query_timer_timeout:
 * 
 * Timeout: the last group member has expired or has left the group. Quickly
 * query if there are other members for that group.
 * XXX: a different timer (member_query_timer) will stop the process
 * and will cancel this timer `last_member_query_timer'.
 **/
void
Mld6igmpGroupRecord::last_member_query_timer_timeout()
{
    string dummy_error_msg;

    UNUSED(dummy_error_msg);

    //
    // XXX: The spec says that we shouldn't care if we changed
    // from a Querier to a non-Querier. Hence, send the group-specific
    // query (see the bottom part of Section 4.)
    //
#ifdef HAVE_IPV4_MULTICAST_ROUTING
    if (mld6igmp_vif().proto_is_igmp()) {
	// TODO: XXX: ignore the fact that now there may be IGMPv1 routers?
	TimeVal scaled_max_resp_time =
	    mld6igmp_vif().query_last_member_interval().get() * IGMP_TIMER_SCALE;
	mld6igmp_vif().mld6igmp_send(mld6igmp_vif().primary_addr(),
				     group(),
				     IGMP_MEMBERSHIP_QUERY,
				     scaled_max_resp_time.sec(),
				     group(),
				     dummy_error_msg);
	_last_member_query_timer =
	    mld6igmp_vif().mld6igmp_node().eventloop().new_oneoff_after(
		mld6igmp_vif().query_last_member_interval().get(),
		callback(this, &Mld6igmpGroupRecord::last_member_query_timer_timeout));
    }
#endif // HAVE_IPV4_MULTICAST_ROUTING

#ifdef HAVE_IPV6_MULTICAST_ROUTING
    if (mld6igmp_vif().proto_is_mld6()) {
	TimeVal scaled_max_resp_time =
	    mld6igmp_vif().query_last_member_interval().get() * MLD_TIMER_SCALE;
	mld6igmp_vif().mld6igmp_send(mld6igmp_vif().primary_addr(),
				     group(),
				     MLD_LISTENER_QUERY,
				     scaled_max_resp_time.sec(),
				     group(),
				     dummy_error_msg);
	_last_member_query_timer =
	    mld6igmp_vif().mld6igmp_node().eventloop().new_oneoff_after(
		mld6igmp_vif().query_last_member_interval().get(),
		callback(this, &Mld6igmpGroupRecord::last_member_query_timer_timeout));
    }
#endif // HAVE_IPV6_MULTICAST_ROUTING
}
