// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/mrt/timer.hh,v 1.11 2002/12/09 18:29:23 hodson Exp $


#ifndef __MRT_TIMER_HH__
#define __MRT_TIMER_HH__


//
// Timer class implementation.
//


#include "timer.h"
#include "libxorp/eventloop.hh"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

// The timer class
/**
 * @short The timer class.
 * 
 * Note that this timer is used only by the multicast code.
 */
class Timer {
public:
    /**
     * Default constructor
     */
    Timer()	{ TIMER_NULL(_mtimer); }

    /**
     * Destructor
     */
    ~Timer()	{ TIMER_CANCEL(_mtimer); }
    
    /**
     * Test if the timer is set.
     * 
     * @return true if the timer is set (i.e., running), otherwise false.
     */
    bool is_set() const { return (TIMER_ISSET(_mtimer)); }
    
    /**
     * Start the timer
     * 
     * This method will start a one-time timer. If the timer should be
     * periodic, it should be restarted by the timeout function.
     * 
     * @param delay_sec the number of seconds to the timer expiration.
     * @param delay_usec the number of microseconds (in addition to
     * the number of seconds @ref delay_sec) to the timer expiration.
     * @param func the timeout function to call when the timer expires.
     * @param data_pointer the data to supply to the timeout function @ref func
     * when the timer expires.
     */
    void start(uint32_t delay_sec, uint32_t delay_usec,
	  cfunc_t func, void *data_pointer) {
	TIMER_START(_mtimer, delay_sec, delay_usec, func, data_pointer);
    }

    /**
     * Start the timer with a randomized timeout expiration period.
     * 
     * This method will start a one-time timer. If the timer should be
     * (randomly) periodic, it should be restarted by the timeout function.
     * The scheduled randomized time is uniformly randomly selected in
     * the interval delay +- delay*random_factor. For example,
     * if @ref random_factor is 0.5, the random interval is
     * (0.5*delay, 1.5*delay)
     * 
     * @param delay_sec the number of seconds (used in the randomized
     * computation) to the timer expiration.
     * @param delay_usec the number of microseconds (used in the randomized
     * computation in addition to the number of seconds @ref delay_sec) to the
     * timer expiration.
     * @param func the timeout function to call when the timer expires.
     * @param data_pointer the data to supply to the timeout function @ref func
     * when the timer expires.
     * @param random_factor the randomization factor.
     */
    void start_random(uint32_t delay_sec, uint32_t delay_usec,
		      cfunc_t func, void *data_pointer,
		      double random_factor) {
	TIMER_START_RANDOM(_mtimer, delay_sec, delay_usec, func, data_pointer,
			   random_factor);
    }

    /**
     * Start a semaphore
     * 
     * A semaphore is a timer without a timeout function. It is used
     * only to indicate whether some interval of time has expired or not.
     * 
     * @param delay_sec the number of seconds to the semaphore expiration.
     * @param delay_usec the number of microseconds (in addition to
     * the number of seconds @ref delay_sec) to the semaphore expiration.
     */
    void start_semaphore(uint32_t delay_sec, uint32_t delay_usec) {
	TIMER_START_SEMAPHORE(_mtimer, delay_sec, delay_usec);
    }
    
    /**
     * Cancel the timer
     */
    void cancel() { TIMER_CANCEL(_mtimer); }
    
    /**
     * Get the number of seconds until the timer expires.
     * 
     * @return the number of seconds until the timer expires. Note that
     * if the timer is not set, the return value is 0 (zero seconds).
     */
    uint32_t left_sec() const { return (TIMER_LEFT_SEC(_mtimer)); }
    
    /**
     * Get the remaining expiration time
     * 
     * Compute the expiration time for the timer.
     * 
     * @param timeval_diff the timeval structure to store the result. Note that
     * if the timer is not set, the timeval structure will be reset to
     * (0, 0); i.e., zero seconds and zero microseconds.
     * @return the number of seconds until the timer expire. Note that
     * if the timer is not set, the return value is 0 (zero seconds).
     */
    uint32_t left_timeval(struct timeval *timeval_diff) const {
	return (TIMER_LEFT_TIMEVAL(_mtimer, timeval_diff));
    }
    
private:
    mtimer_t *_mtimer;	// A pointer to the real time structure if running,
			// or NULL otherwise.
};

//
// Global variables
//

//
// Global functions prototypes
//

/**
 * A hook to hook the multicast timers (@ref Mtimer) with the
 * Xorp timers (@ref XorpTimer).
 * 
 * Process all @ref Mtimer that have timeout since the last processing,
 * and schedule a new @ref XorpTimer that will expire when the earliest
 * @ref Mtimer will expire.
 * 
 * @param e the event loop to add the multicast timers to.
 * @return true if an @ref Mtimer was scheduled, otherwise false.
 */
bool
xorp_schedule_mtimer(EventLoop& e);

#endif // __MRT_TIMER_HH__
