// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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
// Multicast group record information used by
// IGMPv1 and IGMPv2 (RFC 2236), IGMPv3 (RFC 3376),
// MLDv1 (RFC 2710), and MLDv2 (RFC 3810).
//


#include "mld6igmp_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "mld6igmp_group_record.hh"
#include "mld6igmp_node.hh"
#include "mld6igmp_vif.hh"


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
      _is_include_mode(true),
      _do_forward_sources(*this),
      _dont_forward_sources(*this),
      _last_reported_host(IPvX::ZERO(family())),
      _query_retransmission_count(0)
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
 * Find a source that should be forwarded.
 *
 * @param source the source address.
 * @return the corresponding source record (@ref Mld6igmpSourceRecord)
 * if found, otherwise NULL.
 */
Mld6igmpSourceRecord*
Mld6igmpGroupRecord::find_do_forward_source(const IPvX& source)
{
    return (_do_forward_sources.find_source_record(source));
}

/**
 * Find a source that should not be forwarded.
 *
 * @param source the source address.
 * @return the corresponding source record (@ref Mld6igmpSourceRecord)
 * if found, otherwise NULL.
 */
Mld6igmpSourceRecord*
Mld6igmpGroupRecord::find_dont_forward_source(const IPvX& source)
{
    return (_dont_forward_sources.find_source_record(source));
}

/**
 * Test whether the entry is unused.
 *
 * @return true if the entry is unused, otherwise false.
 */
bool
Mld6igmpGroupRecord::is_unused() const
{
    if (is_include_mode()) {
	if (_do_forward_sources.empty()) {
	    XLOG_ASSERT(_dont_forward_sources.empty());
	    return (true);
	}
	return (false);
    }

    if (is_exclude_mode()) {
	//
	// XXX: the group timer must be running in EXCLUDE mode,
	// otherwise there must have been transition to INCLUDE mode.
	//
	if (_group_timer.scheduled())
	    return (false);
	XLOG_ASSERT(_do_forward_sources.empty());
	XLOG_ASSERT(_dont_forward_sources.empty());
	return (true);
    }

    XLOG_UNREACHABLE();

    return (true);
}

/**
 * Process MODE_IS_INCLUDE report.
 *
 * @param sources the source addresses.
 * @param last_reported_host the address of the host that last reported
 * as member.
 */
void
Mld6igmpGroupRecord::process_mode_is_include(const set<IPvX>& sources,
					     const IPvX& last_reported_host)
{
    bool old_is_include_mode = is_include_mode();
    set<IPvX> old_do_forward_sources = _do_forward_sources.extract_source_addresses();
    set<IPvX> old_dont_forward_sources = _dont_forward_sources.extract_source_addresses();

    set_last_reported_host(last_reported_host);

    if (is_include_mode()) {
	//
	// Router State: INCLUDE (A)
	// Report Received: IS_IN (B)
	// New Router State: INCLUDE (A + B)
	// Actions: (B) = GMI
	//
	Mld6igmpSourceSet& a = _do_forward_sources;
	const set<IPvX>& b = sources;
	TimeVal gmi = _mld6igmp_vif.group_membership_interval();

	set_include_mode();

	_do_forward_sources = a + b;			// (A + B)

	_do_forward_sources.set_source_timer(b, gmi);	// (B) = GMI

	calculate_forwarding_changes(old_is_include_mode,
				     old_do_forward_sources,
				     old_dont_forward_sources);
	return;
    }

    if (is_exclude_mode()) {
	//
	// Router State: EXCLUDE (X, Y)
	// Report Received: IS_IN (A)
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

	calculate_forwarding_changes(old_is_include_mode,
				     old_do_forward_sources,
				     old_dont_forward_sources);
	return;
    }
}

/**
 * Process MODE_IS_EXCLUDE report.
 *
 * @param sources the source addresses.
 * @param last_reported_host the address of the host that last reported
 * as member.
 */
void
Mld6igmpGroupRecord::process_mode_is_exclude(const set<IPvX>& sources,
					     const IPvX& last_reported_host)
{
    bool old_is_include_mode = is_include_mode();
    set<IPvX> old_do_forward_sources = _do_forward_sources.extract_source_addresses();
    set<IPvX> old_dont_forward_sources = _dont_forward_sources.extract_source_addresses();

    set_last_reported_host(last_reported_host);

    if (is_include_mode()) {
	//
	// Router State: INCLUDE (A)
	// Report Received: IS_EX (B)
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

	calculate_forwarding_changes(old_is_include_mode,
				     old_do_forward_sources,
				     old_dont_forward_sources);
	return;
    }

    if (is_exclude_mode()) {
	//
	// Router State: EXCLUDE (X, Y)
	// Report Received: IS_EX (A)
	// New Router State: EXCLUDE (A - Y, Y * A)
	// Actions: (A - X - Y) = GMI
	//          Delete (X - A)
	//          Delete (Y - A)
	//          Group Timer = GMI
	//
	Mld6igmpSourceSet& x = _do_forward_sources;
	Mld6igmpSourceSet x_copy(x);
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

	calculate_forwarding_changes(old_is_include_mode,
				     old_do_forward_sources,
				     old_dont_forward_sources);
	return;
    }
}

/**
 * Process CHANGE_TO_INCLUDE_MODE report.
 *
 * @param sources the source addresses.
 * @param last_reported_host the address of the host that last reported
 * as member.
 */
void
Mld6igmpGroupRecord::process_change_to_include_mode(const set<IPvX>& sources,
						    const IPvX& last_reported_host)
{
    bool old_is_include_mode = is_include_mode();
    set<IPvX> old_do_forward_sources = _do_forward_sources.extract_source_addresses();
    set<IPvX> old_dont_forward_sources = _dont_forward_sources.extract_source_addresses();
    string dummy_error_msg;

    set_last_reported_host(last_reported_host);

    if (is_include_mode()) {
	//
	// Router State: INCLUDE (A)
	// Report Received: TO_IN (B)
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

	// Send Q(G, A - B) with a_minus_b
	_mld6igmp_vif.mld6igmp_group_source_query_send(
	    group(),
	    a_minus_b.extract_source_addresses(),
	    dummy_error_msg);

	calculate_forwarding_changes(old_is_include_mode,
				     old_do_forward_sources,
				     old_dont_forward_sources);
	return;
    }

    if (is_exclude_mode()) {
	//
	// Router State: EXCLUDE (X, Y)
	// Report Received: TO_IN (A)
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

	// Send Q(G, X - A) with x_minus_a
	_mld6igmp_vif.mld6igmp_group_source_query_send(
	    group(),
	    x_minus_a.extract_source_addresses(),
	    dummy_error_msg);

	// Send Q(G)
	_mld6igmp_vif.mld6igmp_group_query_send(group(), dummy_error_msg);

	calculate_forwarding_changes(old_is_include_mode,
				     old_do_forward_sources,
				     old_dont_forward_sources);
	return;
    }
}

/**
 * Process CHANGE_TO_EXCLUDE_MODE report.
 *
 * @param sources the source addresses.
 * @param last_reported_host the address of the host that last reported
 * as member.
 */
void
Mld6igmpGroupRecord::process_change_to_exclude_mode(const set<IPvX>& sources,
						    const IPvX& last_reported_host)
{
    bool old_is_include_mode = is_include_mode();
    set<IPvX> old_do_forward_sources = _do_forward_sources.extract_source_addresses();
    set<IPvX> old_dont_forward_sources = _dont_forward_sources.extract_source_addresses();
    string dummy_error_msg;

    set_last_reported_host(last_reported_host);

    if (is_include_mode()) {
	//
	// Router State: INCLUDE (A)
	// Report Received: TO_EX (B)
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

	// Send Q(G, A * B) with _do_forward_sources
	_mld6igmp_vif.mld6igmp_group_source_query_send(
	    group(),
	    _do_forward_sources.extract_source_addresses(),
	    dummy_error_msg);

	calculate_forwarding_changes(old_is_include_mode,
				     old_do_forward_sources,
				     old_dont_forward_sources);
	return;
    }

    if (is_exclude_mode()) {
	//
	// Router State: EXCLUDE (X, Y)
	// Report Received: TO_EX (A)
	// New Router State: EXCLUDE (A - Y, Y * A)
	// Actions: (A - X - Y) = Group Timer
	//          Delete (X - A)
	//          Delete (Y - A)
	//          Send Q(G, A - Y)
	//          Group Timer = GMI
	//
	Mld6igmpSourceSet& x = _do_forward_sources;
	Mld6igmpSourceSet x_copy(x);
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

	// Send Q(G, A - Y) with _do_forward_sources
	_mld6igmp_vif.mld6igmp_group_source_query_send(
	    group(),
	    _do_forward_sources.extract_source_addresses(),
	    dummy_error_msg);

	calculate_forwarding_changes(old_is_include_mode,
				     old_do_forward_sources,
				     old_dont_forward_sources);
	return;
    }
}

/**
 * Process ALLOW_NEW_SOURCES report.
 *
 * @param sources the source addresses.
 * @param last_reported_host the address of the host that last reported
 * as member.
 */
void
Mld6igmpGroupRecord::process_allow_new_sources(const set<IPvX>& sources,
					       const IPvX& last_reported_host)
{
    bool old_is_include_mode = is_include_mode();
    set<IPvX> old_do_forward_sources = _do_forward_sources.extract_source_addresses();
    set<IPvX> old_dont_forward_sources = _dont_forward_sources.extract_source_addresses();

    set_last_reported_host(last_reported_host);

    if (is_include_mode()) {
	//
	// Router State: INCLUDE (A)
	// Report Received: ALLOW (B)
	// New Router State: INCLUDE (A + B)
	// Actions: (B) = GMI
	//
	Mld6igmpSourceSet& a = _do_forward_sources;
	const set<IPvX>& b = sources;
	TimeVal gmi = _mld6igmp_vif.group_membership_interval();

	set_include_mode();

	_do_forward_sources = a + b;			// A + B

	_do_forward_sources.set_source_timer(b, gmi);	// (B) = GMI

	calculate_forwarding_changes(old_is_include_mode,
				     old_do_forward_sources,
				     old_dont_forward_sources);
	return;
    }

    if (is_exclude_mode()) {
	//
	// Router State: EXCLUDE (X, Y)
	// Report Received: ALLOW (A)
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

	calculate_forwarding_changes(old_is_include_mode,
				     old_do_forward_sources,
				     old_dont_forward_sources);
	return;
    }
}

/**
 * Process BLOCK_OLD_SOURCES report.
 *
 * @param sources the source addresses.
 * @param last_reported_host the address of the host that last reported
 * as member.
 */
void
Mld6igmpGroupRecord::process_block_old_sources(const set<IPvX>& sources,
					       const IPvX& last_reported_host)
{
    bool old_is_include_mode = is_include_mode();
    set<IPvX> old_do_forward_sources = _do_forward_sources.extract_source_addresses();
    set<IPvX> old_dont_forward_sources = _dont_forward_sources.extract_source_addresses();
    string dummy_error_msg;

    set_last_reported_host(last_reported_host);

    if (is_include_mode()) {
	//
	// Router State: INCLUDE (A)
	// Report Received: BLOCK (B)
	// New Router State: INCLUDE (A)
	// Actions: Send Q(G, A * B)
	//
	Mld6igmpSourceSet& a = _do_forward_sources;
	const set<IPvX>& b = sources;

	set_include_mode();

	Mld6igmpSourceSet a_and_b = a * b;

	// Send Q(G, A * B) with a_and_b
	_mld6igmp_vif.mld6igmp_group_source_query_send(
	    group(),
	    a_and_b.extract_source_addresses(),
	    dummy_error_msg);

	calculate_forwarding_changes(old_is_include_mode,
				     old_do_forward_sources,
				     old_dont_forward_sources);
	return;
    }

    if (is_exclude_mode()) {
	//
	// Router State: EXCLUDE (X, Y)
	// Report Received: BLOCK (A)
	// New Router State: EXCLUDE (X + (A - Y), Y)
	// Actions: (A - X - Y) = Group Timer
	//          Send Q(G, A - Y)
	//
	Mld6igmpSourceSet& x = _do_forward_sources;
	Mld6igmpSourceSet x_copy(x);
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

	// Send Q(G, A - Y) with a_minus_y
	_mld6igmp_vif.mld6igmp_group_source_query_send(
	    group(),
	    a_minus_y.extract_source_addresses(),
	    dummy_error_msg);

	calculate_forwarding_changes(old_is_include_mode,
				     old_do_forward_sources,
				     old_dont_forward_sources);
	return;
    }
}

/**
 * Lower the group timer.
 *
 * @param timeval the timeout interval the timer should be lowered to.
 */
void
Mld6igmpGroupRecord::lower_group_timer(const TimeVal& timeval)
{
    TimeVal timeval_remaining;

    //
    // Lower the group timer
    //
    _group_timer.time_remaining(timeval_remaining);
    if (timeval < timeval_remaining) {
	_group_timer = eventloop().new_oneoff_after(
	    timeval,
	    callback(this, &Mld6igmpGroupRecord::group_timer_timeout));
    }
}

/**
 * Lower the source timer for a set of sources.
 *
 * @param sources the source addresses.
 * @param timeval the timeout interval the timer should be lowered to.
 */
void
Mld6igmpGroupRecord::lower_source_timer(const set<IPvX>& sources,
					 const TimeVal& timeval)
{
    //
    // Lower the source timer
    //
    _do_forward_sources.lower_source_timer(sources, timeval);
}

/**
 * Take the appropriate actions for a source that has expired.
 *
 * @param source_record the source record that has expired.
 */
void
Mld6igmpGroupRecord::source_expired(Mld6igmpSourceRecord* source_record)
{
    Mld6igmpSourceSet::iterator iter;

    // Erase the source record from the appropriate source set
    iter = _do_forward_sources.find(source_record->source());
    XLOG_ASSERT(iter != _do_forward_sources.end());
    _do_forward_sources.erase(iter);

    if (is_include_mode()) {
	// notify routing (-)
	mld6igmp_vif().join_prune_notify_routing(source_record->source(),
						 group(),
						 ACTION_PRUNE);
	// Delete the source record
	delete source_record;

	// If no more source records, then delete the group record
	if (_do_forward_sources.empty()) {
	    XLOG_ASSERT(_dont_forward_sources.empty());
	    mld6igmp_vif().group_records().erase(group());
	    delete this;
	}
	return;
    }

    if (is_exclude_mode()) {
	// notify routing (-)
	//
	// XXX: Note that we send a PRUNE twice: the first one to remove the
	// original JOIN for the source, and the second one to create
	// PRUNE state for the source.
	//
	mld6igmp_vif().join_prune_notify_routing(source_record->source(),
						 group(),
						 ACTION_PRUNE);
	mld6igmp_vif().join_prune_notify_routing(source_record->source(),
						 group(),
						 ACTION_PRUNE);

	// Do not remove record, but add it to the appropriate set
	_dont_forward_sources.insert(make_pair(source_record->source(),
					       source_record));
	return;
    }
}

/**
 * Get the number of seconds until the group timer expires.
 * 
 * @return the number of seconds until the group timer expires.
 */
uint32_t
Mld6igmpGroupRecord::timeout_sec() const
{
    TimeVal tv;
    
    _group_timer.time_remaining(tv);
    
    return (tv.sec());
}

/**
 * Timeout: the group timer has expired.
 */
void
Mld6igmpGroupRecord::group_timer_timeout()
{
    if (is_include_mode()) {
	// XXX: Nothing to do when in INCLUDE mode.
	return;
    }

    if (is_exclude_mode()) {
	// Clear the state for all excluded sources
	Mld6igmpSourceSet::const_iterator source_iter;
	for (source_iter = this->dont_forward_sources().begin();
	     source_iter != this->dont_forward_sources().end();
	     ++source_iter) {
	    const Mld6igmpSourceRecord *source_record = source_iter->second;
	    mld6igmp_vif().join_prune_notify_routing(source_record->source(),
						     this->group(),
						     ACTION_JOIN);
	}

	// Delete the source records with zero timers
	_dont_forward_sources.delete_payload_and_clear();

	// notify routing (-)
	mld6igmp_vif().join_prune_notify_routing(IPvX::ZERO(family()),
						 group(),
						 ACTION_PRUNE);

	if (! _do_forward_sources.empty()) {
	    // Transition to INCLUDE mode
	    set_include_mode();
	    return;
	}

	//
	// No sources with running source timers.
	// Delete the group record and return immediately.
	//
	mld6igmp_vif().group_records().erase(group());
	delete this;
	return;
    }
}

/**
 * Schedule periodic Group-Specific and Group-and-Source-Specific Query
 * retransmission.
 *
 * If the sources list is empty, we schedule Group-Specific Query,
 * otherwise we schedule Group-and-Source-Specific Query.
 *
 * @param sources the source addresses.
 */
void
Mld6igmpGroupRecord::schedule_periodic_group_query(const set<IPvX>& sources)
{
    Mld6igmpSourceSet::iterator source_iter;
    size_t count = _mld6igmp_vif.last_member_query_count() - 1;

    //
    // Reset the count for query retransmission for all "don't forward" sources
    //
    for (source_iter = _dont_forward_sources.begin();
	 source_iter != _dont_forward_sources.end();
	 ++source_iter) {
	Mld6igmpSourceRecord* source_record = source_iter->second;
	source_record->set_query_retransmission_count(0);
    }

    if (_mld6igmp_vif.last_member_query_count() == 0)
	return;
    if (_mld6igmp_vif.query_last_member_interval().get() == TimeVal::ZERO())
	return;

    //
    // Set the count for query retransmissions
    //
    if (sources.empty()) {
	//
	// Set the count for Group-Specific Query retransmission
	//
	_query_retransmission_count = count;
    } else {
	//
	// Set the count for Group-and-Source-Specific Query retransmission
	//
	set<IPvX>::const_iterator ipvx_iter;
	for (ipvx_iter = sources.begin();
	     ipvx_iter != sources.end();
	     ++ipvx_iter) {
	    const IPvX& ipvx = *ipvx_iter;
	    Mld6igmpSourceRecord* source_record = find_do_forward_source(ipvx);
	    if (source_record == NULL)
		continue;
	    source_record->set_query_retransmission_count(count);
	}
    }

    //
    // Set the periodic timer for SSM Group-Specific and
    // Group-and-Source-Specific Queries.
    //
    // Note that we set the timer only if it wasn't running already.
    //
    if (! _group_query_timer.scheduled()) {
	_group_query_timer = eventloop().new_periodic(
	    _mld6igmp_vif.query_last_member_interval().get(),
	    callback(this, &Mld6igmpGroupRecord::group_query_periodic_timeout));
    }
}

/**
 * Periodic timeout: time to send the next Group-Specific and
 * Group-and-Source-Specific Queries.
 *
 * @return true if the timer should be scheduled again, otherwise false.
 */
bool
Mld6igmpGroupRecord::group_query_periodic_timeout()
{
    string dummy_error_msg;
    bool s_flag = false;
    set<IPvX> no_sources;		// XXX: empty set
    set<IPvX> sources_with_s_flag;
    set<IPvX> sources_without_s_flag;
    Mld6igmpSourceSet::iterator source_iter;
    TimeVal max_resp_time = mld6igmp_vif().query_last_member_interval().get();
    bool do_send_group_query = true;

    //
    // XXX: Don't send Group-Specific or Group-and-Source-Specific Queries
    // for entries that are in IGMPv1 mode.
    //
    if (is_igmpv1_mode())
	return (false);

    //
    // XXX: The IGMPv3/MLDv2 spec doesn't say what to do here if we changed
    // from a Querier to a non-Querier.
    // However, the IGMPv2 spec says that Querier to non-Querier transitions
    // are to be ignored (see the bottom part of Section 3 of RFC 2236).
    // Hence, for this reason and for robustness purpose we send the Query
    // messages without taking into account any Querier to non-Querier
    // transitions.
    //

    //
    // Send the Group-Specific Query message
    //
    if (_query_retransmission_count == 0) {
	do_send_group_query = false;	// No more queries to send
    } else {
	_query_retransmission_count--;
	//
	// Calculate the group-specific "Suppress Router-Side Processing" bit
	//
	TimeVal timeval_remaining;
	group_timer().time_remaining(timeval_remaining);
	if (timeval_remaining > _mld6igmp_vif.last_member_query_time())
	    s_flag = true;

	_mld6igmp_vif.mld6igmp_query_send(mld6igmp_vif().primary_addr(),
					  group(),
					  max_resp_time,
					  group(),
					  no_sources,
					  s_flag,
					  dummy_error_msg);
    }

    //
    // Select all the sources that should be queried, and add them to
    // the appropriate set.
    //
    for (source_iter = _do_forward_sources.begin();
	 source_iter != _do_forward_sources.end();
	 ++source_iter) {
	Mld6igmpSourceRecord* source_record = source_iter->second;
	size_t count = source_record->query_retransmission_count();
	bool s_flag = false;
	if (count == 0)
	    continue;
	source_record->set_query_retransmission_count(count - 1);
	//
	// Calculate the "Suppress Router-Side Processing" bit
	//
	TimeVal timeval_remaining;
	source_record->source_timer().time_remaining(timeval_remaining);
	if (timeval_remaining > _mld6igmp_vif.last_member_query_time())
	    s_flag = true;
	if (s_flag)
	    sources_with_s_flag.insert(source_record->source());
	else
	    sources_without_s_flag.insert(source_record->source());
    }

    //
    // Send the Group-and-Source-Specific Query messages
    //
    if ((! sources_with_s_flag.empty()) && (! do_send_group_query)) {
	//
	// According to RFC 3376, Section 6.6.3.2:
	// "If a group specific query is scheduled to be transmitted at the
	// same time as a group and source specific query for the same group,
	// then transmission of the group and source specific message with the
	// "Suppress Router-Side Processing" bit set may be suppressed."
	//
	// The corresponding text from RFC 3810, Section 7.6.3.2 is similar.
	//
	_mld6igmp_vif.mld6igmp_query_send(mld6igmp_vif().primary_addr(),
					  group(),
					  max_resp_time,
					  group(),
					  sources_with_s_flag,
					  true,		// XXX: set the s_flag
					  dummy_error_msg);
    }
    if (! sources_without_s_flag.empty()) {
	_mld6igmp_vif.mld6igmp_query_send(mld6igmp_vif().primary_addr(),
					  group(),
					  max_resp_time,
					  group(),
					  sources_without_s_flag,
					  false,       // XXX: reset the s_flag
					  dummy_error_msg);
    }

    if (sources_with_s_flag.empty()
	&& sources_without_s_flag.empty()
	&& (! do_send_group_query)) {
	return (false);			// No more queries to send
    }

    return (true);		// Schedule the next timeout
}

/**
 * Record that an older Membership report message has been received.
 *
 * @param message_version the corresponding protocol version of the
 * received message.
 */
void
Mld6igmpGroupRecord::received_older_membership_report(int message_version)
{
    TimeVal timeval = _mld6igmp_vif.older_version_host_present_interval();

    if (_mld6igmp_vif.proto_is_igmp()) {
	switch (message_version) {
	case IGMP_V1:
	    if (_mld6igmp_vif.is_igmpv2_mode()) {
		//
		// XXX: The value specified in RFC 2236 is different from
		// the value specified in RFC 3376.
		//
		timeval = _mld6igmp_vif.group_membership_interval();
	    }
	    _igmpv1_host_present_timer = eventloop().new_oneoff_after(
		timeval,
		callback(this, &Mld6igmpGroupRecord::older_version_host_present_timer_timeout));
	    break;
	case IGMP_V2:
	    _igmpv2_mldv1_host_present_timer = eventloop().new_oneoff_after(
		timeval,
		callback(this, &Mld6igmpGroupRecord::older_version_host_present_timer_timeout));
	    break;
	default:
	    break;
	}
    }

    if (_mld6igmp_vif.proto_is_mld6()) {
	switch (message_version) {
	case MLD_V1:
	    _igmpv2_mldv1_host_present_timer = eventloop().new_oneoff_after(
		timeval,
		callback(this, &Mld6igmpGroupRecord::older_version_host_present_timer_timeout));
	    break;
	default:
	    break;
	}
    }
}

void
Mld6igmpGroupRecord::older_version_host_present_timer_timeout()
{
    // XXX: nothing to do
}

/**
 * Test if the group is running in IGMPv1 mode.
 *
 * @return true if the group is running in IGMPv1 mode, otherwise false.
 */
bool
Mld6igmpGroupRecord::is_igmpv1_mode() const
{
    if (! _mld6igmp_vif.proto_is_igmp())
	return (false);

    if (_mld6igmp_vif.is_igmpv1_mode())
	return (true);		// XXX: explicitly configured in IGMPv1 mode

    return (_igmpv1_host_present_timer.scheduled());
}

/**
 * Test if the group is running in IGMPv2 mode.
 *
 * @return true if the group is running in IGMPv2 mode, otherwise false.
 */
bool
Mld6igmpGroupRecord::is_igmpv2_mode() const
{
    if (! _mld6igmp_vif.proto_is_igmp())
	return (false);

    if (is_igmpv1_mode())
	return (false);

    return (_igmpv2_mldv1_host_present_timer.scheduled());
}

/**
 * Test if the group is running in IGMPv3 mode.
 *
 * @return true if the group is running in IGMPv3 mode, otherwise false.
 */
bool
Mld6igmpGroupRecord::is_igmpv3_mode() const
{
    if (! _mld6igmp_vif.proto_is_igmp())
	return (false);

    if (is_igmpv1_mode() || is_igmpv2_mode())
	return (false);

    return (true);
}

/**
 * Test if the group is running in MLDv1 mode.
 *
 * @return true if the group is running in MLDv1 mode, otherwise false.
 */
bool
Mld6igmpGroupRecord::is_mldv1_mode() const
{
    if (! _mld6igmp_vif.proto_is_mld6())
	return (false);

    if (_mld6igmp_vif.is_mldv1_mode())
	return (true);		// XXX: explicitly configured in MLDv1 mode

    return (_igmpv2_mldv1_host_present_timer.scheduled());
}

/**
 * Test if the group is running in MLDv2 mode.
 *
 * @return true if the group is running in MLDv2 mode, otherwise false.
 */
bool
Mld6igmpGroupRecord::is_mldv2_mode() const
{
    if (! _mld6igmp_vif.proto_is_mld6())
	return (false);

    if (is_mldv1_mode())
	return (false);

    return (true);
}

/**
 * Calculate the forwarding changes and notify the interested parties.
 *
 * @param old_is_include mode if true, the old filter mode was INCLUDE,
 * otherwise was EXCLUDE.
 * @param old_do_forward_sources the old set of sources to forward.
 * @param old_dont_forward_sources the old set of sources not to forward.
 */
void
Mld6igmpGroupRecord::calculate_forwarding_changes(
    bool old_is_include_mode,
    const set<IPvX>& old_do_forward_sources,
    const set<IPvX>& old_dont_forward_sources) const
{
    bool new_is_include_mode = is_include_mode();
    set<IPvX> new_do_forward_sources = _do_forward_sources.extract_source_addresses();
    set<IPvX> new_dont_forward_sources = _dont_forward_sources.extract_source_addresses();
    set<IPvX>::const_iterator iter;

    if (old_is_include_mode) {
	if (new_is_include_mode) {
	    // INCLUDE -> INCLUDE
	    XLOG_ASSERT(old_dont_forward_sources.empty());
	    XLOG_ASSERT(new_dont_forward_sources.empty());

	    // Join all new sources that are to be forwarded
	    for (iter = new_do_forward_sources.begin();
		 iter != new_do_forward_sources.end();
		 ++iter) {
		const IPvX& ipvx = *iter;
		if (old_do_forward_sources.find(ipvx)
		    == old_do_forward_sources.end()) {
		    mld6igmp_vif().join_prune_notify_routing(ipvx,
							     group(),
							     ACTION_JOIN);
		}
	    }

	    // Prune all old sources that were forwarded
	    for (iter = old_do_forward_sources.begin();
		 iter != old_do_forward_sources.end();
		 ++iter) {
		const IPvX& ipvx = *iter;
		if (new_do_forward_sources.find(ipvx)
		    == new_do_forward_sources.end()) {
		    mld6igmp_vif().join_prune_notify_routing(ipvx,
							     group(),
							     ACTION_PRUNE);
		}
	    }
	}

	if (! new_is_include_mode) {
	    // INCLUDE -> EXCLUDE
	    XLOG_ASSERT(old_dont_forward_sources.empty());

	    // Prune the old sources that were forwarded
	    for (iter = old_do_forward_sources.begin();
		 iter != old_do_forward_sources.end();
		 ++iter) {
		const IPvX& ipvx = *iter;
		if (new_do_forward_sources.find(ipvx)
		    == new_do_forward_sources.end()) {
		    mld6igmp_vif().join_prune_notify_routing(ipvx,
							     group(),
							     ACTION_PRUNE);
		}
	    }

	    // Join the group itself
	    mld6igmp_vif().join_prune_notify_routing(IPvX::ZERO(family()),
						     group(),
						     ACTION_JOIN);

	    // Join all new sources that are to be forwarded
	    for (iter = new_do_forward_sources.begin();
		 iter != new_do_forward_sources.end();
		 ++iter) {
		const IPvX& ipvx = *iter;
		if (old_do_forward_sources.find(ipvx)
		    == old_do_forward_sources.end()) {
		    mld6igmp_vif().join_prune_notify_routing(ipvx,
							     group(),
							     ACTION_JOIN);
		}
	    }

	    // Prune all new sources that are not to be forwarded
	    for (iter = new_dont_forward_sources.begin();
		 iter != new_dont_forward_sources.end();
		 ++iter) {
		const IPvX& ipvx = *iter;
		if (old_dont_forward_sources.find(ipvx)
		    == old_dont_forward_sources.end()) {
		    mld6igmp_vif().join_prune_notify_routing(ipvx,
							     group(),
							     ACTION_PRUNE);
		}
	    }
	}
    }

    if (! old_is_include_mode) {
	if (new_is_include_mode) {
	    // EXCLUDE -> INCLUDE
	    XLOG_ASSERT(new_dont_forward_sources.empty());

	    // Join all old sources that were not to be forwarded
	    for (iter = old_dont_forward_sources.begin();
		 iter != old_dont_forward_sources.end();
		 ++iter) {
		const IPvX& ipvx = *iter;
		if (new_dont_forward_sources.find(ipvx)
		    == new_dont_forward_sources.end()) {
		    mld6igmp_vif().join_prune_notify_routing(ipvx,
							     group(),
							     ACTION_JOIN);
		}
	    }

	    // Prune the group itself
	    mld6igmp_vif().join_prune_notify_routing(IPvX::ZERO(family()),
						     group(),
						     ACTION_PRUNE);

	    // Join all new sources that are to be forwarded
	    for (iter = new_do_forward_sources.begin();
		 iter != new_do_forward_sources.end();
		 ++iter) {
		const IPvX& ipvx = *iter;
		if (old_do_forward_sources.find(ipvx)
		    == old_do_forward_sources.end()) {
		    mld6igmp_vif().join_prune_notify_routing(ipvx,
							     group(),
							     ACTION_JOIN);
		}
	    }
	}

	if (! new_is_include_mode) {
	    // EXCLUDE -> EXCLUDE

	    // Join all new sources that are to be forwarded
	    for (iter = new_do_forward_sources.begin();
		 iter != new_do_forward_sources.end();
		 ++iter) {
		const IPvX& ipvx = *iter;
		if (old_do_forward_sources.find(ipvx)
		    == old_do_forward_sources.end()) {
		    mld6igmp_vif().join_prune_notify_routing(ipvx,
							     group(),
							     ACTION_JOIN);
		}
	    }

	    // Prune all old sources that were forwarded
	    for (iter = old_do_forward_sources.begin();
		 iter != old_do_forward_sources.end();
		 ++iter) {
		const IPvX& ipvx = *iter;
		if (new_do_forward_sources.find(ipvx)
		    == new_do_forward_sources.end()) {
		    mld6igmp_vif().join_prune_notify_routing(ipvx,
							     group(),
							     ACTION_PRUNE);
		}
	    }

	    // Join all old sources that were not to be forwarded
	    for (iter = old_dont_forward_sources.begin();
		 iter != old_dont_forward_sources.end();
		 ++iter) {
		const IPvX& ipvx = *iter;
		if (new_dont_forward_sources.find(ipvx)
		    == new_dont_forward_sources.end()) {
		    mld6igmp_vif().join_prune_notify_routing(ipvx,
							     group(),
							     ACTION_JOIN);
		}
	    }

	    // Prune all new sources that are not to be forwarded
	    for (iter = new_dont_forward_sources.begin();
		 iter != new_dont_forward_sources.end();
		 ++iter) {
		const IPvX& ipvx = *iter;
		if (old_dont_forward_sources.find(ipvx)
		    == old_dont_forward_sources.end()) {
		    mld6igmp_vif().join_prune_notify_routing(ipvx,
							     group(),
							     ACTION_PRUNE);
		}
	    }
	}
    }
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
 * Find a group record.
 *
 * @param group the group address.
 * @return the corresponding group record (@ref Mld6igmpGroupRecord)
 * if found, otherwise NULL.
 */
Mld6igmpGroupRecord*
Mld6igmpGroupSet::find_group_record(const IPvX& group)
{
    Mld6igmpGroupSet::iterator iter = this->find(group);

    if (iter != this->end())
	return (iter->second);

    return (NULL);
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
 * @param last_reported_host the address of the host that last reported
 * as member.
 */
void
Mld6igmpGroupSet::process_mode_is_include(const IPvX& group,
					  const set<IPvX>& sources,
					  const IPvX& last_reported_host)
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

    group_record->process_mode_is_include(sources, last_reported_host);

    //
    // If the group record is not used anymore, then delete it
    //
    if (group_record->is_unused()) {
	this->erase(group);
	delete group_record;
    }
}

/**
 * Process MODE_IS_EXCLUDE report.
 *
 * @param group the group address.
 * @param sources the source addresses.
 * @param last_reported_host the address of the host that last reported
 * as member.
 */
void
Mld6igmpGroupSet::process_mode_is_exclude(const IPvX& group,
					  const set<IPvX>& sources,
					  const IPvX& last_reported_host)
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

    group_record->process_mode_is_exclude(sources, last_reported_host);

    //
    // If the group record is not used anymore, then delete it
    //
    if (group_record->is_unused()) {
	this->erase(group);
	delete group_record;
    }
}

/**
 * Process CHANGE_TO_INCLUDE_MODE report.
 *
 * @param group the group address.
 * @param sources the source addresses.
 * @param last_reported_host the address of the host that last reported
 * as member.
 */
void
Mld6igmpGroupSet::process_change_to_include_mode(const IPvX& group,
						 const set<IPvX>& sources,
						 const IPvX& last_reported_host)
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

    if (_mld6igmp_vif.is_igmpv1_mode(group_record)) {
	//
	// XXX: Ignore CHANGE_TO_INCLUDE_MODE messages when in
	// IGMPv1 mode.
	//
    } else {
	group_record->process_change_to_include_mode(sources,
						     last_reported_host);
    }

    //
    // If the group record is not used anymore, then delete it
    //
    if (group_record->is_unused()) {
	this->erase(group);
	delete group_record;
    }
}

/**
 * Process CHANGE_TO_EXCLUDE_MODE report.
 *
 * @param group the group address.
 * @param sources the source addresses.
 * @param last_reported_host the address of the host that last reported
 * as member.
 */
void
Mld6igmpGroupSet::process_change_to_exclude_mode(const IPvX& group,
						 const set<IPvX>& sources,
						 const IPvX& last_reported_host)
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

    if (_mld6igmp_vif.is_igmpv1_mode(group_record)
	|| _mld6igmp_vif.is_igmpv2_mode(group_record)
	|| _mld6igmp_vif.is_mldv1_mode(group_record)) {
	//
	// XXX: Ignore the source list in the CHANGE_TO_EXCLUDE_MODE
	// messages when in IGMPv1, IGMPv2, or MLDv1 mode.
	//
	set<IPvX> no_sources;		// XXX: empty set
	group_record->process_change_to_exclude_mode(no_sources,
						     last_reported_host);
    } else {
	group_record->process_change_to_exclude_mode(sources,
						     last_reported_host);
    }

    //
    // If the group record is not used anymore, then delete it
    //
    if (group_record->is_unused()) {
	this->erase(group);
	delete group_record;
    }
}

/**
 * Process ALLOW_NEW_SOURCES report.
 *
 * @param group the group address.
 * @param sources the source addresses.
 * @param last_reported_host the address of the host that last reported
 * as member.
 */
void
Mld6igmpGroupSet::process_allow_new_sources(const IPvX& group,
					    const set<IPvX>& sources,
					    const IPvX& last_reported_host)
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

    group_record->process_allow_new_sources(sources, last_reported_host);

    //
    // If the group record is not used anymore, then delete it
    //
    if (group_record->is_unused()) {
	this->erase(group);
	delete group_record;
    }
}

/**
 * Process BLOCK_OLD_SOURCES report.
 *
 * @param group the group address.
 * @param sources the source addresses.
 * @param last_reported_host the address of the host that last reported
 * as member.
 */
void
Mld6igmpGroupSet::process_block_old_sources(const IPvX& group,
					    const set<IPvX>& sources,
					    const IPvX& last_reported_host)
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

    if (_mld6igmp_vif.is_igmpv1_mode(group_record)
	|| _mld6igmp_vif.is_igmpv2_mode(group_record)
	|| _mld6igmp_vif.is_mldv1_mode(group_record)) {
	//
	// XXX: Ignore BLOCK_OLD_SOURCES messages when in
	// IGMPv1, IGMPv2, or MLDv1 mode.
	//
    } else {
	group_record->process_block_old_sources(sources, last_reported_host);
    }

    //
    // If the group record is not used anymore, then delete it
    //
    if (group_record->is_unused()) {
	this->erase(group);
	delete group_record;
    }
}

/**
 * Lower the group timer.
 *
 * @param group the group address.
 * @param timeval the timeout interval the timer should be lowered to.
 */
void
Mld6igmpGroupSet::lower_group_timer(const IPvX& group,
				    const TimeVal& timeval)
{
    Mld6igmpGroupSet::iterator iter;

    iter = this->find(group);
    if (iter != this->end()) {
	Mld6igmpGroupRecord* group_record = iter->second;
	group_record->lower_group_timer(timeval);
    }
}

/**
 * Lower the source timer for a set of sources.
 *
 * @param group the group address.
 * @param sources the source addresses.
 * @param timeval the timeout interval the timer should be lowered to.
 */
void
Mld6igmpGroupSet::lower_source_timer(const IPvX& group,
				     const set<IPvX>& sources,
				     const TimeVal& timeval)
{
    Mld6igmpGroupSet::iterator iter;

    iter = this->find(group);
    if (iter != this->end()) {
	Mld6igmpGroupRecord* group_record = iter->second;
	group_record->lower_source_timer(sources, timeval);
    }
}
