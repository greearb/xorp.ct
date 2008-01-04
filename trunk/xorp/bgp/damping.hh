// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/bgp/damping.hh,v 1.4 2007/02/16 22:45:11 pavlin Exp $

#ifndef __BGP_DAMPING_HH__
#define __BGP_DAMPING_HH__

#include "libxorp/timer.hh"
#include "libxorp/eventloop.hh"

/* Route Flap Damping (RFC2439)
 *
 * The damping parameters plus general support code.
 */
class Damping {
 public:
    static const uint32_t FIXED = 1000;

    Damping(EventLoop& eventloop);

    void set_damping(bool damping);

    bool get_damping() const;

    void set_half_life(uint32_t half_life);

    void set_max_hold_down(uint32_t max_hold_down);

    void set_reuse(uint32_t reuse);

    void set_cutoff(uint32_t cutoff);

    /**
     * Get the current clock tick.
     */
    uint32_t get_tick() const {
	return _tick;
    }

    /**
     * Merit value to use the first time a route is encountered.
     */
    uint32_t get_merit() const {
	return FIXED;
    }

    /**
     * Compute the merit value given the last time and merit values.
     */
    uint32_t compute_merit(uint32_t last_time, uint32_t last_merit) const;

    /**
     * True of the merit value has passed the cutoff threshold.
     */
    bool cutoff(uint32_t merit) const {
	return merit > _cutoff;
    }

    /**
     * True of the merit value is above the reuse threshold.
     */
    bool reuse(uint32_t merit) const {
	return merit > _reuse;
    }

    /**
     * Compute how long the route should be damped in seconds.
     * The time for this figure of merit to decay to the reuse threshold.
     */
    uint32_t get_reuse_time(uint32_t merit) const;
	
 private:
    EventLoop& _eventloop;
    bool _damping;		// True if damping is enabled.
    uint32_t _half_life;	// Half life in minutes.
    uint32_t _max_hold_down;	// Maximum hold down time in minutes.
    uint32_t _reuse;		// Reuse threshold.
    uint32_t _cutoff;		// Cutoff threshold.

    vector<uint32_t> _decay;	// Per tick delay.
    uint32_t _tick;		// incremented every tick.
    XorpTimer _tick_tock;	// Timer to increment the tick.

    /**
     * Called when damping is enabled.
     */
    void init();

    /**
     * Called when damping is disabled.
     */
    void halt();

    /*
     * Called every clock tick.
     */
    bool tick();
};

#endif // __BGP_DAMPING_HH__
