// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/policy/backend/policy_profiler.cc,v 1.4 2008/08/06 08:30:57 abittau Exp $"

#include "policy/policy_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/profile.hh"
#include "policy_profiler.hh"

PolicyProfiler::PolicyProfiler() : _samplec(0), _stopped(true)
{
}

void
PolicyProfiler::start()
{
    XLOG_ASSERT(_stopped);
    XLOG_ASSERT(_samplec < MAX_SAMPLES);

    _samples[_samplec] = SP::sample();
    _stopped = false;
}

void
PolicyProfiler::stop()
{
    TU now = SP::sample();

    XLOG_ASSERT(!_stopped);
    XLOG_ASSERT(now >= _samples[_samplec]);

    _samples[_samplec] = now - _samples[_samplec];

    _stopped = true;
    _samplec++;
}

unsigned
PolicyProfiler::count()
{
    return _samplec;
}

PolicyProfiler::TU
PolicyProfiler::sample(unsigned idx)
{
    XLOG_ASSERT(idx < _samplec);

    return _samples[idx];
}

void
PolicyProfiler::clear()
{
    _samplec = 0;
    _stopped = true;
}
