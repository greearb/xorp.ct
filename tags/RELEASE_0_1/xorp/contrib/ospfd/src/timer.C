/*
 *   OSPFD routing daemon
 *   Copyright (C) 1998 by John T. Moy
 *   
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

/* Routines implementing OSPF timer support
 */

#include <stdlib.h>
#include "ospfinc.h"

extern SPFtime sys_etime;

/* Get a random number of milliseconds.
 * Used to jitter timers.
 */

int Timer::random_period(int period)

{
    float fperiod = period;
    return((int) ((fperiod*rand())/(RAND_MAX+1.0)));
}

/* Stop a timer. Noop if the timer is not running.
 */

void Timer::stop()

{
    if (!is_running())
	return;
    active = false;
    timerq.priq_delete(this);
}

/* When a timer is destoyed, make sure that it is
 * stopped.
 */

Timer::~Timer()

{
    stop();
}

/* Start a single shot timer with a given period.
 * If timer is already running, do nothing - restart
 * must be called to restart a running timer.
 * Timer is randmized by plus or minus half a second.
 */

void Timer::start(int milliseconds, bool randomize)

{
    SPFtime interval;
    SPFtime etime;

    // Stop timer
    if (is_running())
	return;
    // Set period
    period = milliseconds;
    active = true;

    // Randomize?
    if (randomize && milliseconds >= 1000)
	milliseconds += random_period(1000) - 500;

    // Add to timer queue
    interval.sec = milliseconds/SECOND;
    interval.msec = milliseconds%SECOND;
    time_add(sys_etime, interval, &etime);
    cost0 = etime.sec;
    cost1 = etime.msec;
    timerq.priq_add(this);
}

/* Restart a timer, but only if it is running.
 * Optionally start the timer with a new period.
 */

void Timer::restart(int milliseconds)

{
    if (!is_running())
	return;
    stop();
    if (milliseconds == 0)
	milliseconds = period;
    start(milliseconds);
}

/* Start an interval timer. First interval is
 * randomized, set to between zero and the full
 * period. After that, each period the timer fires
 * at the exact time.
 */

void ITimer::start(int milliseconds, bool randomize)

{
    SPFtime etime;

    if (is_running())
	return;
    // Set period
    period = milliseconds;
    active = true;

    // Randomize?
    if (randomize)
	milliseconds = random_period(period) + 1;

    // Add to timer queue
    time_add(sys_etime, milliseconds, &etime);
    cost0 = etime.sec;
    cost1 = etime.msec;
    timerq.priq_add(this);
}

/* Fire a single shot timer. The timer is no longer
 * active, so mark it as such. Then call the associated
 * action routine.
 */

void Timer::fire()

{
    active = false;
    action();
}

/* Fire an interval timer.
 * Requeue the timer w/o randomness., and then
 * execute the action routine.
 * Requeue first so that the action routine can stop
 * an interval timer, if it wants.
 */

void ITimer::fire()

{
    SPFtime firetime;
    SPFtime etime;
    
    // Add to timer queue
    firetime.sec = cost0;
    firetime.msec = cost1;
    time_add(firetime, period, &etime);
    cost0 = etime.sec;
    cost1 = etime.msec;
    timerq.priq_add(this);
    // Execute action routine
    action();
}

/* Return the number of milliseconds until the timer will
 * fire.
 */

int Timer::milliseconds_to_firing()

{
    SPFtime fire_time;

    if (!is_running())
        return(0);
    fire_time.sec = cost0;
    fire_time.msec = cost1;
    return(time_diff(fire_time, sys_etime));
}

/* Timer utilities.
 * time_less() returns whether the first argument is less recent
 * than the second.
 * time_add() adds two time constants, filling in a passed result
 * time_equal() returns whether the two time constants are the same
 * time_diff() returns the difference of the two time constants,
 * in millseconds.
 */

bool time_less(const SPFtime &a, const SPFtime &b)

{
    return ((a.sec < b.sec) ||
	    (a.sec == b.sec && a.msec < b.msec));
}

void time_add(const SPFtime &a, const SPFtime &b, SPFtime *result)

{
    result->sec = a.sec + b.sec;
    result->msec = a.msec + b.msec;
    result->sec += result->msec/Timer::SECOND;
    result->msec = result->msec % Timer::SECOND;
}

void time_sub(const SPFtime &a, const SPFtime &b, SPFtime *result)

{
    int msec = (int)a.msec - (int)b.msec;
    if (msec < 0) {
	result->sec  = a.sec - b.sec - 1;
	result->msec = (msec + Timer::SECOND);
    } else {
	result->sec  = a.sec - b.sec;
	result->msec = msec;
    }
}

bool time_equal(const SPFtime &a, const SPFtime &b)

{
    return (a.sec == b.sec && a.msec == b.msec);
}

int time_diff(const SPFtime &a, const SPFtime &b)

{
    int diff;

    diff = (a.sec - b.sec)*1000;
    diff += (a.msec - b.msec);
    return(diff);
}
