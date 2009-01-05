// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2006-2009 XORP, Inc.
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

#ident "$XORP: xorp/libxorp/round_robin.cc,v 1.8 2008/10/02 21:57:33 bms Exp $"

#include "libxorp_module.h"
#include "libxorp/xorp.h"

#include <strings.h>

#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"

#include "round_robin.hh"

RoundRobinObjBase::RoundRobinObjBase()
    : _weight(0), _next(NULL), _prev(NULL)
{
}

bool
RoundRobinObjBase::scheduled() const
{
    return (_next != NULL);
}

RoundRobinQueue::RoundRobinQueue()
    : _next_to_run(NULL), _run_count(0), _elements(0)
{
}

void
RoundRobinQueue::push(RoundRobinObjBase* obj, int weight)
{
    XLOG_ASSERT(obj != NULL);
    XLOG_ASSERT(weight > 0);
    link_object(obj, weight);
}

void
RoundRobinQueue::pop_obj(RoundRobinObjBase* obj)
{
    XLOG_ASSERT(obj != NULL);
    unlink_object(obj);
}

void
RoundRobinQueue::pop()
{
    XLOG_ASSERT(_next_to_run != NULL);
    pop_obj(_next_to_run);
}

RoundRobinObjBase*
RoundRobinQueue::get_next_entry()
{
    RoundRobinObjBase* top = _next_to_run;
    if (top != NULL) {
	XLOG_ASSERT(_run_count < top->weight());
	_run_count++;
	if (_run_count == top->weight()) {
	    _next_to_run = _next_to_run->next();
	    _run_count = 0;
	}
    }
    return top;
}

void
RoundRobinQueue::link_object(RoundRobinObjBase* obj, int weight)
{
    obj->set_weight(weight);

    if (_next_to_run == NULL) {
	// This is the first element
	_next_to_run = obj;
	_run_count = 0;
	obj->set_next(obj);
	obj->set_prev(obj);
    } else {
	//
	// Insert just before the next entry to be run, so this is at the
	// end of the queue.
	//
	obj->set_next(_next_to_run);
	obj->set_prev(_next_to_run->prev());
	obj->prev()->set_next(obj);
	obj->next()->set_prev(obj);
    }
    _elements++;
}

void
RoundRobinQueue::unlink_object(RoundRobinObjBase* obj)
{
    if (obj->next() == obj) {
	// This is the only item in the list
	_next_to_run = NULL;
    } else {
	if (_next_to_run == obj) {
	    _next_to_run = obj->next();
	    _run_count = 0;
	}
	obj->prev()->set_next(obj->next());
	obj->next()->set_prev(obj->prev());
    }
    obj->set_prev(NULL);
    obj->set_next(NULL);
    _elements--;
}
