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

#ident "$XORP: xorp/libxipc/finder_xrl_queue.cc,v 1.2 2003/04/23 20:50:48 hodson Exp $"

#include "finder_module.h"
#include "libxorp/debug.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "finder_messenger.hh"
#include "finder_xrl_queue.hh"

inline EventLoop&
FinderXrlCommandQueue::eventloop()
{
    return _m->eventloop();
}

inline void
FinderXrlCommandQueue::push()
{
    debug_msg("push\n");
    if (false == _pending && false == _cmds.empty()) {
	_cmds.front()->dispatch();
	_pending = true;
    }
}	

void
FinderXrlCommandQueue::enqueue(const FinderXrlCommandQueue::Command& cmd)
{
    debug_msg("enqueue\n");
    _cmds.push_back(cmd);
    push();
}

void
FinderXrlCommandQueue::crank()
{
    debug_msg("crank\n");
    XLOG_ASSERT(_pending == true);
    _cmds.pop_front();
    _pending = false;
    push();
}

void
FinderXrlCommandQueue::kill_messenger()
{
    debug_msg("killing messenger\n");
    delete _m;
}
