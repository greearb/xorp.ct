// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



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
