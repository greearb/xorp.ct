// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/eventloop.cc,v 1.5 2004/02/20 19:02:18 hodson Exp $"

#include "libxorp_module.h"
#include "xorp.h"
#include "eventloop.hh"
#include "xlog.h"
#include "debug.h"

//
// Number of EventLoop instances.
//
static int instance_count = 0;

//
// Last call to EventLoop::run.  0 is a special value that indicates
// EventLoop::run has not been called.
//
static time_t last_ev_run;

EventLoop::EventLoop()
{
    instance_count++;
    XLOG_ASSERT(instance_count == 1);
    last_ev_run = 0;
}

EventLoop::~EventLoop()
{
    instance_count--;
    XLOG_ASSERT(instance_count == 0);
}

void
EventLoop::run()
{
    const time_t MAX_ALLOWED = 2;
    static time_t last_warned;

    if (last_ev_run == 0)
	last_ev_run = time(0);

    time_t now = time(0);
    time_t diff = now - last_ev_run;

    if (now - last_warned > 0 && (diff > MAX_ALLOWED)) {
	XLOG_WARNING("%d seconds between calls to EventLoop::run", (int)diff);
	last_warned = now;
    }

    TimeVal t;
    _timer_list.get_next_delay(t);
    _selector_list.select(&t);
    _timer_list.run();

    last_ev_run = time(0);
}
