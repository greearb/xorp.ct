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

#ident "$XORP: xorp/libxorp/time_slice.cc,v 1.5 2003/04/02 19:59:14 hodson Exp $"


//
// Time-slice class implementation.
//


#include "libxorp_module.h"
#include "xorp.h"
#include "time_slice.hh"


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//

//
// Local variables
//

//
// Local functions prototypes
//


TimeSlice::TimeSlice(uint32_t usec_limit, size_t test_iter_frequency)
    : _test_iter_frequency(test_iter_frequency),
      _remain_iter(test_iter_frequency)
{
    _time_slice_limit = TimeVal(0, usec_limit);
    TimerList::system_gettimeofday(&_time_slice_start);
}

void
TimeSlice::reset()
{
    TimerList::system_gettimeofday(&_time_slice_start);
    _remain_iter = _test_iter_frequency;
}
