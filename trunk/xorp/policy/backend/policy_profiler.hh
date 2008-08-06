// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008 International Computer Science Institute
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

// $XORP$

#ifndef __POLICY_BACKEND_POLICY_PROFILER_HH__
#define __POLICY_BACKEND_POLICY_PROFILER_HH__

class PolicyProfiler {
public:
    typedef uint64_t	TU;
    typedef TU (*GT)(void);

    static const unsigned int MAX_SAMPLES = 128;
    static void set_get_time(GT x);

    PolicyProfiler();

    void     clear();
    void     start();
    void     stop();
    unsigned count();
    TU	     sample(unsigned idx);

private:
    TU		_samples[MAX_SAMPLES];
    unsigned	_samplec;
    static GT   _gt;
};

#endif // __POLICY_BACKEND_POLICY_PROFILER_HH__
