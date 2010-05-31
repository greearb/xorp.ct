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



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include <math.h>
#include <vector>

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "damping.hh"
#include "bgp.hh"

Damping::Damping(EventLoop& eventloop)
    :	 _eventloop(eventloop),
	 _damping(false),
	 _half_life(15),
	 _max_hold_down(60),
	 _cutoff(3000),
	 _tick(0)
{
    init();
}

void
Damping::set_damping(bool damping)
{
    _damping = damping;
    init();
}

bool
Damping::get_damping() const
{
    return _damping;
}

void
Damping::set_half_life(uint32_t half_life)
{
    _half_life = half_life;
    init();
}

void
Damping::set_max_hold_down(uint32_t max_hold_down)
{
    _max_hold_down = max_hold_down;
    init();
}

void
Damping::set_reuse(uint32_t reuse)
{
    _reuse = reuse;
    init();
}

void
Damping::set_cutoff(uint32_t cutoff)
{
    _cutoff = cutoff;
    init();
}

void
Damping::init()
{
    debug_msg("init\n");

    if (!_damping) {
	halt();
	return;
    }

    size_t array_size = _max_hold_down * 60;	// Into seconds.
    _decay.resize(array_size);

    double decay_1 = exp((1.0 / (_half_life * 60.0)) * log(1.0/2.0));
    double decay_i = decay_1;
    for (size_t i = 0; i < array_size; i++) {
	_decay[i] = static_cast<uint32_t>(decay_i * FIXED);
// 	printf("%d %d %f\n",i, _decay[i], decay_i);
	decay_i = pow(decay_1, static_cast<int>(i + 2));
    }

    // Start the timer to incement the tick
    _tick_tock = _eventloop.new_periodic_ms(1000,
					    callback(this, &Damping::tick));
}

void
Damping::halt()
{
    _tick_tock.unschedule();
}

bool
Damping::tick()
{
    _tick++;    

    return true;
}

uint32_t
Damping::compute_merit(uint32_t last_time, uint32_t last_merit) const
{
    debug_msg("last_time %d last_merit %d\n", last_time, last_merit);

    uint32_t tdiff = get_tick() - last_time;
    if (tdiff >= _max_hold_down * 60)
	return FIXED;
    else
	return ((last_merit * _decay[tdiff]) / FIXED) + FIXED;
}

uint32_t
Damping::get_reuse_time(uint32_t merit) const
{
    debug_msg("merit %d\n", merit);

    uint32_t damp_time = (((merit / _reuse) - 1) * _half_life * 60);
    uint32_t max_time = _max_hold_down * 60; 

    uint32_t reuse = damp_time > max_time ? max_time : damp_time;
    debug_msg("reuse %d\n", reuse);

    return reuse;
}
