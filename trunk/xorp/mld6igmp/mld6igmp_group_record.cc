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

#ident "$XORP: xorp/mld6igmp/mld6igmp_group_record.cc,v 1.3 2006/06/07 22:56:45 pavlin Exp $"

//
// Multicast group record information used by
// IGMPv1 and IGMPv2 (RFC 2236), IGMPv3 (RFC 3376), MLDv1 (RFC 2710), and
// MLDv2 (RFC 3810).
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
 * Return value: 
 **/
Mld6igmpGroupRecord::Mld6igmpGroupRecord(Mld6igmpVif& mld6igmp_vif,
					 const IPvX& group)
    : _mld6igmp_vif(mld6igmp_vif),
      _group(group),
      _is_include_mode(false),
      _do_forward_sources(*this),
      _dont_forward_sources(*this),
      _last_reported_host(IPvX::ZERO(family()))
{
    
}

/**
 * Mld6igmpGroupRecord::~Mld6igmpGroupRecord:
 * @: 
 * 
 * Mld6igmpGroupRecord destructor.
 **/
Mld6igmpGroupRecord::~Mld6igmpGroupRecord()
{
    // TODO: Notify routing (-)
    // TODO: ??? Maybe not the right place, or this should
    // be the only place to use ACTION_PRUNE notification??
    // join_prune_notify_routing(IPvX::ZERO(family()), group(), ACTION_PRUNE);
}

/**
 * Process MODE_IS_INCLUDE record.
 *
 * @param sources the source addresses.
 */
void
Mld6igmpGroupRecord::mode_is_include(const set<IPvX>& sources)
{
    set<IPvX>::const_iterator iter;

    if (is_include_mode()) {
	// New Router State: INCLUDE (A + B)
	// Actions: (B) = GMI
	// _do_forward_sources.
	
	return;
    }


    if (is_exclude_mode()) {
	return;
    }
    UNUSED(sources);
}

/**
 * Process MODE_IS_EXCLUDE record.
 *
 * @param sources the source addresses.
 */
void
Mld6igmpGroupRecord::mode_is_exclude(const set<IPvX>& sources)
{
    UNUSED(sources);
}

/**
 * Process CHANGE_TO_INCLUDE_MODE record.
 *
 * @param sources the source addresses.
 */
void
Mld6igmpGroupRecord::change_to_include_mode(const set<IPvX>& sources)
{
    UNUSED(sources);
}

/**
 * Process CHANGE_TO_EXCLUDE_MODE record.
 *
 * @param sources the source addresses.
 */
void
Mld6igmpGroupRecord::change_to_exclude_mode(const set<IPvX>& sources)
{
    UNUSED(sources);
}

/**
 * Process ALLOW_NEW_SOURCES record.
 *
 * @param sources the source addresses.
 */
void
Mld6igmpGroupRecord::allow_new_sources(const set<IPvX>& sources)
{
    UNUSED(sources);
}

/**
 * Process BLOCK_OLD_SOURCES record.
 *
 * @param sources the source addresses.
 */
void
Mld6igmpGroupRecord::block_old_sources(const set<IPvX>& sources)
{
    UNUSED(sources);
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
    Mld6igmpGroupSet::iterator iter;
    iter = mld6igmp_vif().group_records().find(group());
    if (iter != mld6igmp_vif().group_records().end()) {
	mld6igmp_vif().group_records().erase(iter);
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

    //
    // XXX: The spec says that we shouldn't care if we changed
    // from a Querier to a non-Querier. Hence, send the group-specific
    // query (see the bottom part of Section 4.)
    //
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
}

Mld6igmpGroupSet::Mld6igmpGroupSet(Mld6igmpVif& mld6igmp_vif)
    : _mld6igmp_vif(mld6igmp_vif)
{
    
}

Mld6igmpGroupSet::~Mld6igmpGroupSet()
{
    // XXX: don't delete the payload, because it might be used elsewhere
}

/**
 * Delete the payload of the set, and clear the set itself.
 */
void
Mld6igmpGroupSet::delete_payload_and_clear()
{
    Mld6igmpGroupSet::iterator iter;

    //
    // Delete the payload of the set
    //
    for (iter = this->begin(); iter != this->end(); ++iter) {
	Mld6igmpGroupRecord* group_record = iter->second;
	delete group_record;
    }

    //
    // Clear the set itself
    //
    this->clear();
}

/**
 * Process MODE_IS_INCLUDE record.
 *
 * @param group the group address.
 * @param sources the source addresses.
 */
void
Mld6igmpGroupSet::mode_is_include(const IPvX& group, const set<IPvX>& sources)
{
    Mld6igmpGroupSet::iterator iter;
    Mld6igmpGroupRecord* group_record = NULL;

    iter = this->find(group);
    if (iter != this->end()) {
	group_record = iter->second;
    } else {
	group_record = new Mld6igmpGroupRecord(_mld6igmp_vif, group);
	this->insert(make_pair(group, group_record));
    }
    XLOG_ASSERT(group_record != NULL);

    group_record->mode_is_include(sources);
}

/**
 * Process MODE_IS_EXCLUDE record.
 *
 * @param group the group address.
 * @param sources the source addresses.
 */
void
Mld6igmpGroupSet::mode_is_exclude(const IPvX& group, const set<IPvX>& sources)
{
    Mld6igmpGroupSet::iterator iter;
    Mld6igmpGroupRecord* group_record = NULL;

    iter = this->find(group);
    if (iter != this->end()) {
	group_record = iter->second;
    } else {
	group_record = new Mld6igmpGroupRecord(_mld6igmp_vif, group);
	this->insert(make_pair(group, group_record));
    }
    XLOG_ASSERT(group_record != NULL);

    group_record->mode_is_exclude(sources);
}

/**
 * Process CHANGE_TO_INCLUDE_MODE record.
 *
 * @param group the group address.
 * @param sources the source addresses.
 */
void
Mld6igmpGroupSet::change_to_include_mode(const IPvX& group,
					 const set<IPvX>& sources)
{
    Mld6igmpGroupSet::iterator iter;
    Mld6igmpGroupRecord* group_record = NULL;

    iter = this->find(group);
    if (iter != this->end()) {
	group_record = iter->second;
    } else {
	group_record = new Mld6igmpGroupRecord(_mld6igmp_vif, group);
	this->insert(make_pair(group, group_record));
    }
    XLOG_ASSERT(group_record != NULL);

    group_record->change_to_include_mode(sources);
}

/**
 * Process CHANGE_TO_EXCLUDE_MODE record.
 *
 * @param group the group address.
 * @param sources the source addresses.
 */
void
Mld6igmpGroupSet::change_to_exclude_mode(const IPvX& group,
					 const set<IPvX>& sources)
{
    Mld6igmpGroupSet::iterator iter;
    Mld6igmpGroupRecord* group_record = NULL;

    iter = this->find(group);
    if (iter != this->end()) {
	group_record = iter->second;
    } else {
	group_record = new Mld6igmpGroupRecord(_mld6igmp_vif, group);
	this->insert(make_pair(group, group_record));
    }
    XLOG_ASSERT(group_record != NULL);

    group_record->change_to_exclude_mode(sources);
}

/**
 * Process ALLOW_NEW_SOURCES record.
 *
 * @param group the group address.
 * @param sources the source addresses.
 */
void
Mld6igmpGroupSet::allow_new_sources(const IPvX& group,
				    const set<IPvX>& sources)
{
    Mld6igmpGroupSet::iterator iter;
    Mld6igmpGroupRecord* group_record = NULL;

    iter = this->find(group);
    if (iter != this->end()) {
	group_record = iter->second;
    } else {
	group_record = new Mld6igmpGroupRecord(_mld6igmp_vif, group);
	this->insert(make_pair(group, group_record));
    }
    XLOG_ASSERT(group_record != NULL);

    group_record->allow_new_sources(sources);
}

/**
 * Process BLOCK_OLD_SOURCES record.
 *
 * @param group the group address.
 * @param sources the source addresses.
 */
void
Mld6igmpGroupSet::block_old_sources(const IPvX& group,
				    const set<IPvX>& sources)
{
    Mld6igmpGroupSet::iterator iter;
    Mld6igmpGroupRecord* group_record = NULL;

    iter = this->find(group);
    if (iter != this->end()) {
	group_record = iter->second;
    } else {
	group_record = new Mld6igmpGroupRecord(_mld6igmp_vif, group);
	this->insert(make_pair(group, group_record));
    }
    XLOG_ASSERT(group_record != NULL);

    group_record->block_old_sources(sources);
}
