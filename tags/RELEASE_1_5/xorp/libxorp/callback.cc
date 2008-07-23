// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/libxorp/callback.cc,v 1.11 2008/01/04 03:16:32 pavlin Exp $"

#include "libxorp_module.h"

#include "libxorp/xorp.h"

/*
 * This code is only compiled if DEBUG_CALLBACKS is defined in
 * config.h via 'configure --enable-callback-debug'.  All callback
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

#include <stack>


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
			    e.file, e.line, delta.secs(), delta.usecs());
	if (xlog_is_running()) {
	    XLOG_ERROR("%s", s.c_str());
	} else {
	    fprintf(stderr, "ERROR: %s\n", s.c_str());
	}
    }
    cb_stack.pop();
}

#endif /* DEBUG_CALLBACKS */
