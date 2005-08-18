// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/eventloop.cc,v 1.10 2005/08/01 14:08:47 bms Exp $"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "libxorp_module.h"
#include "xorp.h"
#include "eventloop.hh"
#include "xlog.h"
#include "debug.h"

#include "win_dispatcher.hh"

//
// Number of EventLoop instances.
// XXX: Warning: static instance. Not suitable for shared library use.
//
static int instance_count = 0;

//
// Last call to EventLoop::run.  0 is a special value that indicates
// EventLoop::run has not been called.
//
// XXX: Warning: static instance. Not suitable for shared library use.
//
static time_t last_ev_run;

EventLoop::EventLoop()
    : _clock(new SystemClock), _timer_list(_clock),
#ifdef HOST_OS_WINDOWS
      _win_dispatcher(_clock)
#else
      _selector_list(_clock)
#endif
{
    instance_count++;
    XLOG_ASSERT(instance_count == 1);
    last_ev_run = 0;
}

EventLoop::~EventLoop()
{
    instance_count--;
    XLOG_ASSERT(instance_count == 0);
    delete _clock;
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

    _timer_list.advance_time();
    _timer_list.get_next_delay(t);
#ifdef HOST_OS_WINDOWS
    _win_dispatcher.wait_and_dispatch(&t);
#else
    _selector_list.wait_and_dispatch(&t);
#endif
    _timer_list.run();

    last_ev_run = time(0);
}

bool
EventLoop::add_ioevent_cb(XorpFd fd, IoEventType type, const IoEventCb& cb)
{
#ifdef HOST_OS_WINDOWS
    return _win_dispatcher.add_ioevent_cb(fd, type, cb);
#else
    return _selector_list.add_ioevent_cb(fd, type, cb);
#endif
}

bool
EventLoop::remove_ioevent_cb(XorpFd fd, IoEventType type)
{
#ifdef HOST_OS_WINDOWS
    return _win_dispatcher.remove_ioevent_cb(fd, type);
#else
    _selector_list.remove_ioevent_cb(fd, type);
    return true;
#endif
}
