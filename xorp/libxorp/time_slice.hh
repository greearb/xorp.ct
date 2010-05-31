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

// $XORP: xorp/libxorp/time_slice.hh,v 1.13 2008/10/02 21:57:35 bms Exp $


#ifndef __LIBXORP_TIME_SLICE_HH__
#define __LIBXORP_TIME_SLICE_HH__

//
// Time-slice class declaration.
//

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "timer.hh"
#include "timeval.hh"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

/**
 * @short A class for computing whether some processing is taking too long.
 * 
 * This class can be used to compute whether some processing is taking
 * too long time to complete. It is up to the program that uses
 * TimeSlice to check whether the processing is taking too long,
 * and suspend processing of that task if necessary.
 *
 * Example of usage:
<pre>
    TimeSlice time_slice(100000, 20); // 100ms, test every 20th iteration
    
    for (size_t i = 0; i < 1000000000; i++) {
        if (time_slice.is_expired()) {
            // Stop processing.
            // Schedule a timer after 0 ms to continue processing.
            // If needed, save state to continue from here.
            return;
        }
        // Do something CPU-hungry
    }
    time_slice.reset(); // Prepare it again for later usage if needed
</pre>
 */
class TimeSlice {
public:
    /**
     * Constructor for a given time limit and test frequency.
     * 
     * Create a TimeSlice object that will measure time slices
     * with given frequency of testing.
     * The time slice is measured once in @ref test_iter_frequency
     * times when method is_expired() is called.
     * 
     * @param usec_limit the time slice to measure in microseconds.
     * @param test_iter_frequency the frequency of measuring the time slice.
     */
    TimeSlice(uint32_t usec_limit, size_t test_iter_frequency);
    
    /**
     * Reset the TimeSlice object.
     */
    void reset();
    
    /**
     * Test if the time slice has expired.
     * 
     * If the time slice has expired, automatically prepare this object
     * to start measuring again the time slice.
     * 
     * @return true if the time slice has expired, otherwise false.
     */
    bool is_expired();
    
private:
    TimeVal		_time_slice_limit;	// The time slice to measure
    const size_t	_test_iter_frequency;	// The frequency of measuring
    
    TimeVal		_time_slice_start;	// When the time slice started
    size_t		_remain_iter;		// Remaning iterations to
						// check the time
};

//
// Deferred definitions
//
inline bool
TimeSlice::is_expired()
{
    if (--_remain_iter)
	return (false);		// Don't test the time yet
    
    _remain_iter = _test_iter_frequency;
    
    // Test if the time slice has expired
    TimeVal now;
    TimerList::system_gettimeofday(&now);
    if (now - _time_slice_limit < _time_slice_start)
	return (false);		// The time slice has not expired
    
    // The time slice has expired
    _time_slice_start = now;
    return (true);
}

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __LIBXORP_TIME_SLICE_HH__
