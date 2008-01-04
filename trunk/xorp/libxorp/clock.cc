// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/libxorp/clock.cc,v 1.6 2007/09/04 16:00:53 bms Exp $

#include "libxorp_module.h"
#include "xorp.h"

#include "timeval.hh"
#include "clock.hh"

ClockBase::~ClockBase()
{
}

SystemClock::SystemClock()
{
    _tv = new TimeVal();
    SystemClock::advance_time();
}

SystemClock::~SystemClock()
{
    delete _tv;
}

void
SystemClock::advance_time()
#if defined(HAVE_CLOCK_GETTIME) && defined(HAVE_CLOCK_MONOTONIC)
{
    int error;
    struct timespec ts;

    error = ::clock_gettime(CLOCK_MONOTONIC, &ts);
    assert(error == 0);
    _tv->copy_in(ts);
}
#elif defined(HOST_OS_WINDOWS)
{
    FILETIME ft;

    ::GetSystemTimeAsFileTime(&ft);
    _tv->copy_in(ft);
}
#else
{
    struct timeval t;

    ::gettimeofday(&t, 0);
    _tv->copy_in(t);
}
#endif

void
SystemClock::current_time(TimeVal& tv)
{
    tv = *_tv;
}
