// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2006-2008 International Computer Science Institute
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

// $XORP: xorp/libxorp/round_robin.hh,v 1.5 2007/02/16 22:46:22 pavlin Exp $

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
