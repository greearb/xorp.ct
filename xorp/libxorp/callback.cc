// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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



#include "libxorp_module.h"

#include "libxorp/xorp.h"

/*
 * This code is only compiled if DEBUG_CALLBACKS is defined. All callback
 * debugging entails is watching that individual callbacks do not take
 * too long in their dispatch method.  Since XORP is event and timer
 * driven taking anything more than a few seconds is potentially bad.
 * Ideally the longest dispatch times should be of the order of 10-50
 * milliseconds.
 */

#ifdef DEBUG_CALLBACKS

#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/c_format.hh"
#include "libxorp/timeval.hh"
#include "libxorp/timer.hh"


/**
 * Maximum callback dispatch duration.
 */
static const TimeVal MAX_CALLBACK_DURATION(5, 0);

/**
 * Stack because one callback can dispatch another and callback data
 * structures can be deleted so we can't rely on file and line stored
 * there.
 */
struct CBStackElement {
    TimeVal	start;
    const char* file;
    int 	line;

    CBStackElement(const TimeVal& now, const char* f, int l)
	: start(now), file(f), line(l)
    {}
};
static stack<CBStackElement> cb_stack;

void
trace_dispatch_enter(const char* file, int line)
{
    TimeVal now;
    TimerList::system_gettimeofday(&now);
    cb_stack.push(CBStackElement(now, file, line));
}

void
trace_dispatch_leave()
{
    XLOG_ASSERT(cb_stack.empty() == false);

    TimeVal now;
    TimerList::system_gettimeofday(&now);

    const CBStackElement& e     = cb_stack.top();
    TimeVal 		  delta = now - e.start;

    if (delta >= MAX_CALLBACK_DURATION) {
	string s = c_format("Callback originating at %s:%d took "
			    "%d.%06d seconds\n",
			    e.file, e.line, delta.sec(), delta.usec());
	if (xlog_is_running()) {
	    XLOG_ERROR("%s", s.c_str());
	} else {
	    fprintf(stderr, "ERROR: %s\n", s.c_str());
	}
    }
    cb_stack.pop();
}

#endif /* DEBUG_CALLBACKS */
