// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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

#ident "$XORP: xorp/contrib/olsr/link.cc,v 1.1 2008/04/24 15:19:52 bms Exp $"

#include "olsr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include "olsr.hh"
#include "link.hh"
#include "neighbor.hh"
#include "topology.hh"

// TODO: Implement link hysteresis Section 14.3.
// TODO: Implement ETX histogram measurements.

/*
 * Link hysteresis protocol constants.
 */
double OlsrTypes::DEFAULT_HYST_THRESHOLD_HIGH = 0.8f;
double OlsrTypes::DEFAULT_HYST_THRESHOLD_LOW = 0.3f;
double OlsrTypes::DEFAULT_HYST_SCALING = 0.3f;

LogicalLink::LogicalLink(Neighborhood* nh,
    EventLoop& eventloop,
    const OlsrTypes::LogicalLinkID id,
    const TimeVal& vtime,
    const IPv4& remote_addr,
    const IPv4& local_addr)
 : _nh(nh),
   _eventloop(eventloop),
   _id(id),
   _faceid(OlsrTypes::UNUSED_FACE_ID),
   _neighborid(OlsrTypes::UNUSED_NEIGHBOR_ID),
   _destination(0),
   _remote_addr(remote_addr),
   _local_addr(local_addr),
   _is_pending(false)
{
    // Section 7.1.1, 1: HELLO Message Processing.
    //
    // L_SYM_time is not yet scheduled, for the link is not yet symmetric.
    // L_ASYM_time is not yet scheduled; will be scheduled by the first
    //             call to update_timers().
    // L_LOST_time is only scheduled when link-layer loss is reported.
    // L_time is scheduled here for the first time, although it will
    //        also be updated by update_timers().
    //
    _dead_timer = _eventloop.new_oneoff_after(
			  vtime,
			  callback(this, &LogicalLink::event_dead_timer));
}

void
LogicalLink::update_timers(const TimeVal& vtime, bool saw_self,
    const LinkCode lc)
{
    debug_msg("Vtime %s Saw_self %s LinkCode %s\n",
	      cstring(vtime), bool_c_str(saw_self), cstring(lc));

    // 2.1 Update L_ASYM_time.
    if (_asym_timer.scheduled())
	_asym_timer.clear();
    _asym_timer = _eventloop.new_oneoff_after(
			  vtime,
			  callback(this, &LogicalLink::event_asym_timer));

    TimeVal dead_time = _dead_timer.expiry();

    // If our own address appears in a local link tuple,
    // our link with the neighbor may now be symmetric.
    if (saw_self) {
	if (lc.is_lost_link()) {
	    // 2.2.1  L_SYM_time = current time - 1 (i.e., expired)
	    // (We can hear them, but they can't hear us.)
	    if (_sym_timer.scheduled())
		_sym_timer.clear();
	} else if (lc.is_sym_link() || lc.is_asym_link()) {
	    // 2.2.2  L_SYM_time = current time + validity time
	    if (_sym_timer.scheduled())
		_sym_timer.clear();
	    _sym_timer = _eventloop.new_oneoff_after(vtime,
			    callback(this, &LogicalLink::event_sym_timer));

	    // 7.1.1, 2.2.2: L_time = L_SYM_time + NEIGHB_HOLD_TIME
	    dead_time = _sym_timer.expiry() + _nh->get_neighbor_hold_time();
	}
    }

    // 2.3 L_time = max(L_time,L_ASYM_time)
    // A link losing its symmetry should still be advertised for
    // at least 'vtime', even if the link has been lost, to allow
    // neighbors to detect link breakage.
    dead_time = max(_dead_timer.expiry(), _asym_timer.expiry());

    if (_dead_timer.scheduled())
	_dead_timer.clear();
    _dead_timer = _eventloop.new_oneoff_at(dead_time,
	callback(this, &LogicalLink::event_dead_timer));
}

OlsrTypes::LinkType
LogicalLink::link_type() const
{
    // Section 14.2, 1: If L_LOST_LINK_time is not expired, the
    // link is advertised with a link type of LOST_LINK.
    if (_lost_timer.scheduled())
	return OlsrTypes::LOST_LINK;

    if (_sym_timer.scheduled())		    // L_SYM_time not expired
	return OlsrTypes::SYM_LINK;
    else if (_asym_timer.scheduled())	    // L_ASYM_time not expired
	return OlsrTypes::ASYM_LINK;

    return OlsrTypes::LOST_LINK;	    // Both timers expired
}

void
LogicalLink::event_sym_timer()
{
    _nh->event_link_sym_timer(id());
}

void
LogicalLink::event_asym_timer()
{
    _nh->event_link_asym_timer(id());
}

void
LogicalLink::event_lost_timer()
{
    _nh->event_link_lost_timer(id());
}

void
LogicalLink::event_dead_timer()
{
    _nh->event_link_dead_timer(id());
}
