// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2006 International Computer Science Institute
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

#ident "$XORP$"

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "libxorp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"

#include "roundrobin.hh"

#include <strings.h>

RoundRobinBase::RoundRobinBase()
    : _weight(0), _next(0), _prev(0)
{
}

void
RoundRobinBase::link(RoundRobin& rr_queue, int weight)
{
    _weight = weight;
    if (rr_queue._next_to_run == 0) {
	// this is the first element
	rr_queue._next_to_run = this;
	rr_queue._run_count = 0;
	_next = this;
	_prev = this;
    } else {
	// insert just before the next entry to be run, so this is at the
	// end of the queue
	_next = rr_queue._next_to_run;
	_prev = _next->prev();
	_prev->set_next(this);
	_next->set_prev(this);
    }
    rr_queue._elements++;
}

void
RoundRobinBase::unlink(RoundRobin& rr_queue)
{
    if (_next == this) {
	// this is the only item in the list
	rr_queue._next_to_run = 0;
    } else {
	if (rr_queue._next_to_run == this) {
	    rr_queue._next_to_run = _next;
	    rr_queue._run_count = 0;
	}
	_prev->set_next(_next);
	_next->set_prev(_prev);
    }
    _prev = 0;
    _next = 0;
    rr_queue._elements--;
}

bool
RoundRobinBase::scheduled() const
{
    return (!(_next == 0));
}


RoundRobin::RoundRobin()
    : _next_to_run(0), _run_count(0), _elements(0)
{
}

void
RoundRobin::push(RoundRobinBase *p, int weight)
{
    assert(p);
    assert(weight > 0);
    p->link(*this, weight);
}

void
RoundRobin::pop_obj(RoundRobinBase *p)
{
    assert(p);
    p->unlink(*this);
}

void
RoundRobin::pop()
{
    assert(_next_to_run);
    pop_obj(_next_to_run);
}

RoundRobinBase*
RoundRobin::get_next_entry()
{
    RoundRobinBase *top = _next_to_run;
    if (top != 0) {
	assert(_run_count < top->weight());
	_run_count++;
	if (_run_count == top->weight()) {
	    _next_to_run = _next_to_run->_next;
	    _run_count = 0;
	}
    }
    return top;
}
