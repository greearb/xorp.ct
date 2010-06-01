// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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
