// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/devnotes/template.cc,v 1.1.1.1 2002/12/11 23:55:54 hodson Exp $"

#include "config.h"
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

#include "libxorp_module.h"
#include "xlog.h"
#include "xorp.h"

#include "callback.hh"
#include "c_format.hh"
#include "timeval.hh"

/**
 * Maximum callback dispatch duration.
 */
static const TimeVal MAX_CALLBACK_DURATION(5, 0);

/**
 * Start time of last callback.
 */
static timeval s_tv;

void
trace_dispatch_enter(const char*, int)
{
    gettimeofday(&s_tv, 0);
}

void
trace_dispatch_leave(const char* file, int line)
{
    timeval now;
    gettimeofday(&now, 0);
    TimeVal delta = TimeVal(now) - TimeVal(s_tv);
    if (delta >= MAX_CALLBACK_DURATION) {
	string s = c_format("Callback originating at %s:%d took "
			    "%d.%06d seconds\n",
			    file, line, delta.secs(), delta.usecs());
	if (xlog_is_running()) {
	    XLOG_ERROR(s.c_str());
	} else {
	    fprintf(stderr, "ERROR: %s\n", s.c_str());
	}
    }
}

#endif /* DEBUG_CALLBACKS */
