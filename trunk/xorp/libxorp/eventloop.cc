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

#ident "$XORP: xorp/libxorp/eventloop.cc,v 1.2 2003/03/10 23:20:31 hodson Exp $"

#include "libxorp_module.h"
#include "xorp.h"
#include "eventloop.hh"
#include "xlog.h"
#include "debug.h"

void
EventLoop::run()
{
    const time_t MAX_ALLOWED = 2;
    static time_t last = time(0);
    time_t diff = time(0) - last;
    if(diff > MAX_ALLOWED)
	XLOG_WARNING("%d seconds between calls to EventLoop::run", (int)diff);

    TimeVal t;
    _timer_list.get_next_delay(t);
    _selector_list.select(&t);
    _timer_list.run();

    last = time(0);
}
