// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/libxorp/clock.cc,v 1.9 2008/10/02 21:57:29 bms Exp $

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
