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

#ident "$XORP: xorp/mrt/timer2xorp.cc,v 1.8 2002/12/09 18:29:23 hodson Exp $"


//
// Mtimer to xorp timer implementation
//


#include "mrt_module.h"
#include "libxorp/xorp.h"
#include "timer.hh"


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//


//
// Local variables
//
static XorpTimer _xorp_mtimer;

//
// Local functions prototypes
//
static void	xorp_process_mtimer(void *);


// A dummy function to process the first mtimer that is pending
static void
xorp_process_mtimer(void *)
{
    // XXX: xorp_schedule_mtimer() will be executed immediately after
    // that and it will call TIMER_PROCESS()
    // TIMER_PROCESS();
}

// Return: true if Mtimer rescheduled, otherwise false.
bool
xorp_schedule_mtimer(EventLoop& e)
{
    static struct timeval timeval_curr_abs = { FOREVER, FOREVER };
    struct timeval timeval_next, timeval_next_abs;
    
    TIMER_PROCESS();
    
    timer_next_timeval(&timeval_next);
    TIMEVAL_ADD(&timeval_next, NOW(), &timeval_next_abs);
    
    if (TIMEVAL_CMP(&timeval_curr_abs, NOW(), <=)
	|| TIMEVAL_CMP(&timeval_next_abs, &timeval_curr_abs, < )) {
	TIMEVAL_COPY(&timeval_next_abs, &timeval_curr_abs);
	_xorp_mtimer.unschedule();
	_xorp_mtimer = e.new_oneoff_at(timeval_next_abs, xorp_process_mtimer);
	return (true);
    }
    
    return (false);
}
