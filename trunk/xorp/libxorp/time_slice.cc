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

#ident "$XORP: xorp/libxorp/time_slice.cc,v 1.12 2008/10/02 21:57:35 bms Exp $"


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
