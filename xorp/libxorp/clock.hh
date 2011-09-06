// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/libxorp/clock.hh,v 1.9 2008/10/02 21:57:30 bms Exp $

#ifndef __LIBXORP_CLOCK_HH__
#define __LIBXORP_CLOCK_HH__

class TimeVal;

class ClockBase {
public:
    virtual ~ClockBase();

    /**
     * Update internal concept of time.
     */
    virtual void advance_time() = 0;

    /**
     * Get time it was when advance_time() was last called.  Successive calls
     * to current_time return the same value.  Time only advances when
     * advance_time() is called.
     *
     * @param tv TimeVal to be filled in with current time.
     */
    virtual void current_time(TimeVal& tv) = 0;
};

/**
 * An implementation of ClockBase that uses the underlying system's
 * 'get current system time' function as it's clock source.
 */
class SystemClock : public ClockBase, NONCOPYABLE {
public:
    SystemClock();
    virtual ~SystemClock();
    void advance_time();
    void current_time(TimeVal& tv);

private:
    TimeVal* _tv;

#ifdef __WIN32__
    int ms_time_res;
#endif
};

#endif // __LIBXORP_CLOCK_HH__
