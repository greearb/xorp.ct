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

/* Declaration of OSPF timer classes.
 */

/* Implementation of a timer
 * Base class implements a single shot timer.
 * Derived class implements an interval timer
 */

#include "sys/time.h"

class Timer : public PriQElt {
protected:
    int	active:1;
    uns32 period;		// Period in milliseconds
public:
    enum { SECOND = 1000};
    Timer() {
	active=false;
	period = 0;
    }

    static int random_period(int period);
    void stop();
    void restart(int millseconds=0);// Stop and start again
    inline int is_running();
    inline uns32 interval();
    int milliseconds_to_firing();
    virtual void start(int milliseconds, bool randomize=true);
    virtual void fire();
    virtual void action() = 0;
    virtual ~Timer();
};

// Inline functions
inline int Timer::is_running()
{
    return(active);
}
inline uns32 Timer::interval()
{
    return(active ? period : 0);
}

/* Implementation of a interval timer
 * When start, offsets initial interval in
 * an attempt to randomize. When fires, requeues
 * the timer automatically.
 */

class ITimer : public Timer {
public:
    virtual void start(int milliseconds, bool randomize=true);
    virtual void fire();
    virtual void action() = 0;
};

/* Time constants as used in OSPF timer queue. Time is divided
 * into seconds and microseconds since the program started.
 */

class SPFtime {
public:
    SPFtime() : sec(0), msec(0) {}
    SPFtime(uns32 seconds, uns32 milliseconds) : 
	sec(seconds), msec(milliseconds) {}
    SPFtime(const timeval& t) : sec(t.tv_sec), msec (t.tv_usec * 1000) {}
    timeval get_timeval() const {
	timeval t;
	t.tv_sec = sec;
	t.tv_usec = msec * 1000;
	return t;
    }
    uns32 sec;		// Seconds since start
    uns32 msec;		// Milliseconds since start
};

bool	time_less(const SPFtime &a, const SPFtime &b);
bool	time_equal(const SPFtime &a, const SPFtime &b);
int	time_diff(const SPFtime &a, const SPFtime &b);
void	time_add(const SPFtime &a, const SPFtime &b, SPFtime *result);
void	time_sub(const SPFtime &a, const SPFtime &b, SPFtime *result);

inline void time_add(SPFtime &a, int milliseconds, SPFtime *result)
{
    return time_add(a, SPFtime(0, milliseconds), result);
}

inline void time_sub(SPFtime &a, int milliseconds, SPFtime *result)
{
    return time_sub(a, SPFtime(0, milliseconds), result);
}



