// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/libxorp/round_robin.hh,v 1.8 2008/10/02 21:57:33 bms Exp $

#ifndef __LIBXORP_ROUND_ROBIN_HH__
#define __LIBXORP_ROUND_ROBIN_HH__

class RoundRobin;

/**
 * Objects stored in the RoundRobinQueue should inherit from this class.
 */
class RoundRobinObjBase {
public:
    RoundRobinObjBase();

    int weight() const { return _weight; }
    void set_weight(int v) { _weight = v; }
    RoundRobinObjBase* next() const { return _next; }
    RoundRobinObjBase* prev() const { return _prev; }
    void set_next(RoundRobinObjBase* next) { _next = next; }
    void set_prev(RoundRobinObjBase* prev) { _prev = prev; }
    bool scheduled() const;

private:
    int _weight;

    // Links to build a circular list
    RoundRobinObjBase* _next;
    RoundRobinObjBase* _prev;
};

/**
 * The Round-robin queue.
 */
class RoundRobinQueue {
public:
    RoundRobinQueue();
    void push(RoundRobinObjBase* obj, int weight);
    void pop_obj(RoundRobinObjBase* obj);
    void pop();

    RoundRobinObjBase* get_next_entry();

    /**
     * Get the number of elements in the heap.
     *
     * @return the number of elements in the heap.
     */
    size_t size() const { return _elements; }

private:
    void link_object(RoundRobinObjBase* obj, int weight);
    void unlink_object(RoundRobinObjBase* obj);

    RoundRobinObjBase* _next_to_run;
    int _run_count;	// How many times we've run the current task in a row
    int _elements;
};

#endif // __LIBXORP_ROUND_ROBIN_HH__
