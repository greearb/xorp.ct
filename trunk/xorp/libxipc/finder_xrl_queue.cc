// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/libxipc/finder_xrl_queue.cc,v 1.11 2008/07/23 05:10:42 pavlin Exp $"

#include "finder_module.h"
#include "libxorp/debug.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "finder_messenger.hh"
#include "finder_xrl_queue.hh"

FinderXrlCommandQueue::FinderXrlCommandQueue(FinderMessengerBase* messenger)
    : _m(messenger), _pending(false)
{
}

FinderXrlCommandQueue::FinderXrlCommandQueue(const FinderXrlCommandQueue& oq)
    : _m(oq._m), _pending(oq._pending)
{
    XLOG_ASSERT(oq._cmds.empty());
    XLOG_ASSERT(oq._pending == false);
}

FinderXrlCommandQueue::~FinderXrlCommandQueue()
{
}

inline EventLoop&
FinderXrlCommandQueue::eventloop()
{
    return _m->eventloop();
}

inline void
FinderXrlCommandQueue::push()
{
    debug_msg("push\n");
    if (false == _pending && _cmds.empty() == false&&
	_dispatcher.scheduled() == false) {
	_dispatcher = eventloop().new_oneoff_after_ms(0,
			callback(this, &FinderXrlCommandQueue::dispatch_one));
    }
}

void
FinderXrlCommandQueue::dispatch_one()
{
    debug_msg("dispatch_one\n");
    XLOG_ASSERT(_cmds.empty() == false);
    _cmds.front()->dispatch();
    _pending = true;
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
