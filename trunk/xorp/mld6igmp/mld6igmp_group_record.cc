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

#ident "$XORP: xorp/mld6igmp/mld6igmp_group_record.cc,v 1.7 2006/06/12 05:13:05 pavlin Exp $"

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

    _do_forward_sources.delete_payload_and_clear();
    _dont_forward_sources.delete_payload_and_clear();
}

/**
 * Get the corresponding event loop.
 *
 * @return the corresponding event loop.
 */
EventLoop&
Mld6igmpGroupRecord::eventloop()
{
    return (_mld6igmp_vif.mld6igmp_node().eventloop());
}

/**
 * Process MODE_IS_INCLUDE report.
 *
 * @param sources the source addresses.
 */
void
Mld6igmpGroupRecord::process_mode_is_include(const set<IPvX>& sources)
{
    if (is_include_mode()) {
	//
	// New Router State: INCLUDE (A + B)
	// Actions: (B) = GMI
	//
	Mld6igmpSourceSet& a = _do_forward_sources;
	const set<IPvX>& b = sources;
	TimeVal gmi = _mld6igmp_vif.group_membership_interval();

	set_include_mode();

	_do_forward_sources = a + b;			// (A + B)

	_do_forward_sources.set_source_timer(b, gmi);	// (B) = GMI
	
	return;
    }

    if (is_exclude_mode()) {
	//
	// New Router State: EXCLUDE (X + A, Y - A)
	// Actions: (A) = GMI
	//
	Mld6igmpSourceSet& x = _do_forward_sources;
	Mld6igmpSourceSet& y = _dont_forward_sources;
	const set<IPvX>& a = sources;
	TimeVal gmi = _mld6igmp_vif.group_membership_interval();

	set_exclude_mode();

	// XXX: first transfer (Y * A) from (Y) to (X)
	Mld6igmpSourceSet y_and_a = y * a;
	_do_forward_sources = x + y_and_a;
	_do_forward_sources = x + a;			// (X + A)
	_dont_forward_sources = y - a;			// (Y - A)

	_do_forward_sources.set_source_timer(a, gmi);	// (A) = GMI

	return;
    }
}

/**
 * Process MODE_IS_EXCLUDE report.
 *
 * @param sources the source addresses.
 */
void
Mld6igmpGroupRecord::process_mode_is_exclude(const set<IPvX>& sources)
{
    if (is_include_mode()) {
	//
	// New Router State: EXCLUDE (A * B, B - A)
	// Actions: (B - A) = 0
	//          Delete (A - B)
	//          Group Timer = GMI
	//
	Mld6igmpSourceSet& a = _do_forward_sources;
	const set<IPvX>& b = sources;
	TimeVal gmi = _mld6igmp_vif.group_membership_interval();

	set_exclude_mode();

	Mld6igmpSourceSet a_minus_b = a - b;			// A - B
	_dont_forward_sources = _dont_forward_sources + b;	// B
	_dont_forward_sources = _dont_forward_sources - a;	// B - A
	_do_forward_sources = a * b;				// A * B

	_dont_forward_sources.cancel_source_timer();		// (B - A) = 0
	a_minus_b.delete_payload_and_clear();			// Delete (A-B)
	_group_timer = eventloop().new_oneoff_after(
	    gmi,
	    callback(this, &Mld6igmpGroupRecord::group_timer_timeout));

	return;
    }

    if (is_exclude_mode()) {
	//
	// New Router State: EXCLUDE (A - Y, Y * A)
	// Actions: (A - X - Y) = GMI
	//          Delete (X - A)
	//          Delete (Y - A)
	//          Group Timer = GMI
	//
	Mld6igmpSourceSet& x = _do_forward_sources;
	Mld6igmpSourceSet x_copy = x;
	Mld6igmpSourceSet& y = _dont_forward_sources;
	const set<IPvX>& a = sources;
	TimeVal gmi = _mld6igmp_vif.group_membership_interval();

	set_exclude_mode();

	Mld6igmpSourceSet x_minus_a = x - a;			// X - A
	Mld6igmpSourceSet y_minus_a = y - a;			// Y - A
	// XXX: The (X * A) below is needed to preserve the original timers
	_do_forward_sources = x * a;				// X * A
	_do_forward_sources = _do_forward_sources + a;		// A
	_do_forward_sources = _do_forward_sources - y;		// A - Y
	_dont_forward_sources = y * a;				// Y * A
	Mld6igmpSourceSet a_minus_x_minus_y(*this);
	a_minus_x_minus_y = _do_forward_sources - x_copy;	// A - X - Y

	a_minus_x_minus_y.set_source_timer(gmi);	// (A - X - Y) = GMI
	x_minus_a.delete_payload_and_clear();		// Delete (X - A)
	y_minus_a.delete_payload_and_clear();		// Delete (Y - A)
	_group_timer = eventloop().new_oneoff_after(
	    gmi,
	    callback(this, &Mld6igmpGroupRecord::group_timer_timeout));

	return;
    }
}

/**
 * Process CHANGE_TO_INCLUDE_MODE report.
 *
 * @param sources the source addresses.
 */
void
Mld6igmpGroupRecord::process_change_to_include_mode(const set<IPvX>& sources)
{
    if (is_include_mode()) {
	//
	// New Router State: INCLUDE (A + B)
	// Actions: (B) = GMI
	//          Send Q(G, A - B)
	//
	Mld6igmpSourceSet& a = _do_forward_sources;
	const set<IPvX>& b = sources;
	TimeVal gmi = _mld6igmp_vif.group_membership_interval();

	set_include_mode();

	Mld6igmpSourceSet a_minus_b = a - b;		// A - B
	_do_forward_sources = a + b;			// A + B

	_do_forward_sources.set_source_timer(b, gmi);	// (B) = GMI
	// TODO: XXX: PAVPAVPAV: Send Q(G, A - B) with a_minus_b

	return;
    }

    if (is_exclude_mode()) {
	//
	// New Router State: EXCLUDE (X + A, Y - A)
	// Actions: (A) = GMI
	//          Send Q(G, X - A)
	//          Send Q(G)
	//
	Mld6igmpSourceSet& x = _do_forward_sources;
	Mld6igmpSourceSet& y = _dont_forward_sources;
	const set<IPvX>& a = sources;
	TimeVal gmi = _mld6igmp_vif.group_membership_interval();

	set_exclude_mode();

	Mld6igmpSourceSet x_minus_a = x - a;		// X - A
	// XXX: first transfer (Y * A) from (Y) to (X)
	Mld6igmpSourceSet y_and_a = y * a;
	_do_forward_sources = x + y_and_a;
	_do_forward_sources = x + a;			// X + A
	_dont_forward_sources = y - a;			// Y - A

	_do_forward_sources.set_source_timer(a, gmi);	// (A) = GMI
	// TODO: XXX: PAVPAVPAV: Send Q(G, X - A) with x_minus_a
	// TODO: XXX: PAVPAVPAV: Send Q(G)

	return;
    }
}

/**
 * Process CHANGE_TO_EXCLUDE_MODE report.
 *
 * @param sources the source addresses.
 */
void
Mld6igmpGroupRecord::process_change_to_exclude_mode(const set<IPvX>& sources)
{
    if (is_include_mode()) {
	//
	// New Router State: EXCLUDE (A * B, B - A)
	// Actions: (B - A) = 0
	//          Delete (A - B)
	//          Send Q(G, A * B)
	//          Group Timer = GMI
	//
	Mld6igmpSourceSet& a = _do_forward_sources;
	const set<IPvX>& b = sources;
	TimeVal gmi = _mld6igmp_vif.group_membership_interval();

	set_exclude_mode();

	Mld6igmpSourceSet a_minus_b = a - b;			// A - B
	_dont_forward_sources = _dont_forward_sources + b;	// B
	_dont_forward_sources = _dont_forward_sources - a;	// B - A
	_do_forward_sources = a * b;				// A * B

	_dont_forward_sources.cancel_source_timer();		// (B - A) = 0
	a_minus_b.delete_payload_and_clear();			// Delete (A-B)
	_group_timer = eventloop().new_oneoff_after(
	    gmi,
	    callback(this, &Mld6igmpGroupRecord::group_timer_timeout));
	// TODO: XXX: PAVPAVPAV: Send Q(G, A * B) with _do_forward_sources

	return;
    }

    if (is_exclude_mode()) {
	//
	// New Router State: EXCLUDE (A - Y, Y * A)
	// Actions: (A - X - Y) = Group Timer
	//          Delete (X - A)
	//          Delete (Y - A)
	//          Send Q(G, A - Y)
	//          Group Timer = GMI
	//
	Mld6igmpSourceSet& x = _do_forward_sources;
	Mld6igmpSourceSet x_copy = x;
	Mld6igmpSourceSet& y = _dont_forward_sources;
	const set<IPvX>& a = sources;
	TimeVal gmi = _mld6igmp_vif.group_membership_interval();
	TimeVal gt;
	_group_timer.time_remaining(gt);

	set_exclude_mode();

	Mld6igmpSourceSet x_minus_a = x - a;			// X - A
	Mld6igmpSourceSet y_minus_a = y - a;			// Y - A
	// XXX: The (X * A) below is needed to preserve the original timers
	_do_forward_sources = x * a;				// X * A
	_do_forward_sources = _do_forward_sources + a;		// A
	_do_forward_sources = _do_forward_sources - y;		// A - Y
	_dont_forward_sources = y * a;				// Y * A
	Mld6igmpSourceSet a_minus_x_minus_y(*this);
	a_minus_x_minus_y = _do_forward_sources - x_copy;	// A - X - Y

	a_minus_x_minus_y.set_source_timer(gt);	// (A - X - Y) = Group Timer
	x_minus_a.delete_payload_and_clear();		// Delete (X - A)
	y_minus_a.delete_payload_and_clear();		// Delete (Y - A)
	_group_timer = eventloop().new_oneoff_after(
	    gmi,
	    callback(this, &Mld6igmpGroupRecord::group_timer_timeout));
	// TODO: XXX: PAVPAVPAV: Send Q(G, A - Y) with _do_forward_sources

	return;
    }
}

/**
 * Process ALLOW_NEW_SOURCES report.
 *
 * @param sources the source addresses.
 */
void
Mld6igmpGroupRecord::process_allow_new_sources(const set<IPvX>& sources)
{
    if (is_include_mode()) {
	//
	// New Router State: INCLUDE (A + B)
	// Actions: (B) = GMI
	//
	Mld6igmpSourceSet& a = _do_forward_sources;
	const set<IPvX>& b = sources;
	TimeVal gmi = _mld6igmp_vif.group_membership_interval();

	set_include_mode();

	_do_forward_sources = a + b;			// A + B

	_do_forward_sources.set_source_timer(b, gmi);	// (B) = GMI

	return;
    }

    if (is_exclude_mode()) {
	//
	// New Router State: EXCLUDE (X + A, Y - A)
	// Actions: (A) = GMI
	//
	Mld6igmpSourceSet& x = _do_forward_sources;
	Mld6igmpSourceSet& y = _dont_forward_sources;
	const set<IPvX>& a = sources;
	TimeVal gmi = _mld6igmp_vif.group_membership_interval();

	set_exclude_mode();

	// XXX: first transfer (Y * A) from (Y) to (X)
	Mld6igmpSourceSet y_and_a = y * a;
	_do_forward_sources = x + y_and_a;
	_do_forward_sources = x + a;			// X + A
	_dont_forward_sources = y - a;			// Y - A

	_do_forward_sources.set_source_timer(a, gmi);   // (A) = GMI

	return;
    }
}

/**
 * Process BLOCK_OLD_SOURCES report.
 *
 * @param sources the source addresses.
 */
void
Mld6igmpGroupRecord::process_block_old_sources(const set<IPvX>& sources)
{
    if (is_include_mode()) {
	//
	// New Router State: INCLUDE (A)
	// Actions: Send Q(G, A * B)
	//
	Mld6igmpSourceSet& a = _do_forward_sources;
	const set<IPvX>& b = sources;

	set_include_mode();

	Mld6igmpSourceSet a_and_b = a * b;

	// TODO: XXX: PAVPAVPAV: Send Q(G, A * B) with a_and_b

	return;
    }

    if (is_exclude_mode()) {
	//
	// New Router State: EXCLUDE (X + (A - Y), Y)
	// Actions: (A - X - Y) = Group Timer
	//          Send Q(G, A - Y)
	//
	Mld6igmpSourceSet& x = _do_forward_sources;
	Mld6igmpSourceSet x_copy = x;
	Mld6igmpSourceSet& y = _dont_forward_sources;
	const set<IPvX>& a = sources;
	TimeVal gt;
	_group_timer.time_remaining(gt);

	set_exclude_mode();

	Mld6igmpSourceSet a_minus_y(*this);
	a_minus_y = a_minus_y + a;			// A
	a_minus_y = a_minus_y - y;			// A - Y
	_do_forward_sources = x + a_minus_y;		// X + (A - Y)

	Mld6igmpSourceSet a_minus_x_minus_y = _do_forward_sources; // (X+(A-Y))
	a_minus_x_minus_y = a_minus_x_minus_y - x_copy;	// A - X - Y
	a_minus_x_minus_y = a_minus_x_minus_y - y;	// A - X - Y
	Mld6igmpSourceSet y_minus_a = y - a;		// Y - A

	a_minus_x_minus_y.set_source_timer(gt);	// (A - X - Y) = Group Timer
	// TODO: XXX: PAVPAVPAV: Send Q(G, A - Y) with a_minus_y

	return;
    }
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
	_last_member_query_timer = eventloop().new_oneoff_after(
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
	_last_member_query_timer = eventloop().new_oneoff_after(
	    mld6igmp_vif().query_last_member_interval().get(),
	    callback(this, &Mld6igmpGroupRecord::last_member_query_timer_timeout));
    }
}

/**
 * Timeout: the group timer has expired.
 */
void
Mld6igmpGroupRecord::group_timer_timeout()
{
    // TODO: XXX: PAVPAVPAV: implement it!
}

/**
 * Constructor for a given vif.
 * 
 * @param mld6igmp_vif the interface this set belongs to.
 */
Mld6igmpGroupSet::Mld6igmpGroupSet(Mld6igmpVif& mld6igmp_vif)
    : _mld6igmp_vif(mld6igmp_vif)
{
    
}

/**
 * Destructor.
 */
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
 * Process MODE_IS_INCLUDE report.
 *
 * @param group the group address.
 * @param sources the source addresses.
 */
void
Mld6igmpGroupSet::process_mode_is_include(const IPvX& group,
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

    group_record->process_mode_is_include(sources);
}

/**
 * Process MODE_IS_EXCLUDE report.
 *
 * @param group the group address.
 * @param sources the source addresses.
 */
void
Mld6igmpGroupSet::process_mode_is_exclude(const IPvX& group,
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

    group_record->process_mode_is_exclude(sources);
}

/**
 * Process CHANGE_TO_INCLUDE_MODE report.
 *
 * @param group the group address.
 * @param sources the source addresses.
 */
void
Mld6igmpGroupSet::process_change_to_include_mode(const IPvX& group,
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

    group_record->process_change_to_include_mode(sources);
}

/**
 * Process CHANGE_TO_EXCLUDE_MODE report.
 *
 * @param group the group address.
 * @param sources the source addresses.
 */
void
Mld6igmpGroupSet::process_change_to_exclude_mode(const IPvX& group,
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

    group_record->process_change_to_exclude_mode(sources);
}

/**
 * Process ALLOW_NEW_SOURCES report.
 *
 * @param group the group address.
 * @param sources the source addresses.
 */
void
Mld6igmpGroupSet::process_allow_new_sources(const IPvX& group,
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

    group_record->process_allow_new_sources(sources);
}

/**
 * Process BLOCK_OLD_SOURCES report.
 *
 * @param group the group address.
 * @param sources the source addresses.
 */
void
Mld6igmpGroupSet::process_block_old_sources(const IPvX& group,
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

    group_record->process_block_old_sources(sources);
}
