// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2006 International Computer Science Institute
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

// $XORP: xorp/libxorp/round_robin.hh,v 1.1 2006/08/10 22:05:40 mjh Exp $

#ifndef __LIBXORP_ROUND_ROBIN_HH__
#define __LIBXORP_ROUND_ROBIN_HH__

/**
 * Objects stored in the RoundRobin queue should inherit from this class
 */

class RoundRobin;

class RoundRobinBase {
public:
    RoundRobinBase();
    inline int weight() const {return _weight;}
    inline RoundRobinBase *next() const {return _next;}
    inline RoundRobinBase *prev() const {return _prev;}
    inline void set_next(RoundRobinBase *next) {_next = next;}
    inline void set_prev(RoundRobinBase *prev) {_prev = prev;}
    bool scheduled() const;
private:
    void link(RoundRobin &rr_queue, int weight);
    void unlink(RoundRobin &rr_queue);
    
    int _weight;

    RoundRobinBase *_next, *_prev; // links to build a circular list

    friend class RoundRobin;
};

class RoundRobin {
public:
    RoundRobin();
    void push(RoundRobinBase *p, int weight);
    void pop_obj(RoundRobinBase *p);
    void pop();

    RoundRobinBase *get_next_entry();
    
    /**
     * Get the number of elements in the heap.
     *
     * @return the number of elements in the heap.
     */
    size_t size() const { return _elements; }

private:
    RoundRobinBase* _next_to_run;
    int _run_count;  // how many times we've run the current task in a row.
    int _elements;
    
    friend class RoundRobinBase;
};

#endif // __LIBXORP_ROUND_ROBIN_HH__
