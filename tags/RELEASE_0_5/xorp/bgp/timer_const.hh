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

// $XORP: xorp/bgp/timer_const.hh,v 1.1.1.1 2002/12/11 23:55:50 hodson Exp $

#ifndef __BGP_TIMER_CONST_HH__
#define __BGP_TIMER_CONST_HH__

#include <sys/types.h>
#include "libxorp/debug.h"
class BGPTimerConst 
{
public:
    BGPTimerConst()				{
	    _hold_duration = 1000 * 90;
	    _retry_duration = 1000 * 120;
	    _keepalive_duration = 1000 * 30;
	    // _hold_duration = 90;
	    // _retry_duration = 120;
	    // _keepalive_duration = 30;
    }

    uint32_t get_hold_duration()		{ return _hold_duration; }
    void set_hold_duration(uint32_t d)		{ _hold_duration = d; }
    uint32_t get_retry_duration()		{ return _retry_duration; }
    void set_retry_duration(uint32_t d)		{ _retry_duration = d; }
    uint32_t get_keepalive_duration()		{ return _keepalive_duration; }
    void set_keepalive_duration(uint32_t d)	{ _keepalive_duration = d; }
private:
    uint32_t _hold_duration;		// XXX what units are these ?
    uint32_t _retry_duration;
    uint32_t _keepalive_duration;
};

#endif	// __BGP_TIMER_CONST_HH__
