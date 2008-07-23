// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/libxorp/clock.hh,v 1.7 2008/01/04 03:16:35 pavlin Exp $

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
class SystemClock : public ClockBase {
public:
    SystemClock();
    ~SystemClock();
    void advance_time();
    void current_time(TimeVal& tv);

private:
    TimeVal* _tv;
};

#endif // __LIBXORP_CLOCK_HH__
