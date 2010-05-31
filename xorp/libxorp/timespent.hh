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

// $XORP: xorp/libxorp/timespent.hh,v 1.15 2008/10/02 21:57:35 bms Exp $

#ifndef __LIBXORP_TIMESPENT_HH__
#define __LIBXORP_TIMESPENT_HH__

#include "libxorp/timeval.hh"
#include "libxorp/timer.hh"

static const int TIMESPENT_LIMIT = 10;	// Time allowed in seconds.

/**
 * @short (Debugging) Used to find code that has taken too long to execute.
 *
 * It is expected that this class will not be used directly but via
 * the macros below. Thus allowing file, function and line number
 * information to be captured.
 */
class TimeSpent {
public:
    TimeSpent(const char *function, const char *file, int line, int limit)
	: _function(function), _file(file), _line(line),
	  _limit(TimeVal(limit,0))
    {
	TimerList::system_gettimeofday(&_start);
    }

    /**
     * @param delta the time that has passed.
     * @return true if the alloted time has been exceeded.
     */
    bool overlimit(TimeVal& delta)
    {
	TimeVal now;
	TimerList::system_gettimeofday(&now);

	delta = now - _start;

	return delta > _limit;
    }

    /**
     * @return true if the alloted time has been exceeded.
     */
    bool overlimit()
    {
	TimeVal delta;

	return overlimit(delta);
    }

    /**
     * Has the alloted time been exceeded? If it has print a warning message.
     */
    void check(const char *function, const char *file, int line)
    {
	TimeVal delta;

	if (overlimit(delta))
	    XLOG_WARNING("Function %s +%d %s took %s\n", function, line, file,
		   delta.str().c_str());
    }

    ~TimeSpent()
    {
	check(_function, _file, _line);
    }

private:
    TimeVal _start;
    const char *_function;
    const char *_file;
    int _line;
    TimeVal _limit;
};

#ifdef	CHECK_TIME
/**
 * To be placed in suspect method.
 */
#define	TIMESPENT()       TimeSpent _t(__FUNCTION__,__FILE__,__LINE__, \
				       TIMESPENT_LIMIT)
/**
 * Verify that thus far into the method the time limit has not been exceeded.
 *
 * If the alloted time has been exceeded a warning message will be printed.
 */
#define TIMESPENT_CHECK()	_t.check(__FUNCTION__, __FILE__, __LINE__)

/**
 * A boolean that will return true if the alloted time has been exceeded.
 */
#define TIMESPENT_OVERLIMIT()	_t.overlimit()

#else	// ! CHECK_TIME
#define	TIMESPENT()
#define TIMESPENT_CHECK()
#define TIMESPENT_OVERLIMIT()	0
#endif

#endif // __LIBXORP_TIMESPENT_HH__
