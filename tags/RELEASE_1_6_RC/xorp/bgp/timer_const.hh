// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/bgp/timer_const.hh,v 1.9 2008/07/23 05:09:40 pavlin Exp $

#ifndef __BGP_TIMER_CONST_HH__
#define __BGP_TIMER_CONST_HH__

#include <sys/types.h>
#include "libxorp/debug.h"
class BGPTimerConst 
{
public:
    BGPTimerConst()				{
	    _hold_duration = 90;
	    _retry_duration = 120;
	    _keepalive_duration = 30;
    }

    uint32_t get_hold_duration()		{ return _hold_duration; }
    void set_hold_duration(uint32_t d)		{ _hold_duration = d; }
    uint32_t get_retry_duration()		{ return _retry_duration; }
    void set_retry_duration(uint32_t d)		{ _retry_duration = d; }
    uint32_t get_keepalive_duration()		{ return _keepalive_duration; }
    void set_keepalive_duration(uint32_t d)	{ _keepalive_duration = d; }
private:
    // In seconds.
    uint32_t _hold_duration;
    uint32_t _retry_duration;
    uint32_t _keepalive_duration;
};

#endif	// __BGP_TIMER_CONST_HH__
