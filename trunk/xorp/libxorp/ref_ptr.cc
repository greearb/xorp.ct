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

#ident "$XORP: xorp/libxorp/ref_ptr.cc,v 1.4 2003/03/31 16:40:03 hodson Exp $"

#include <assert.h>
#include <iostream>

#include "config.h"
#include "xorp.h"
#include "ref_ptr.hh"

///////////////////////////////////////////////////////////////////////////////
//
// Debugging macro's
//

#define POOL_PARANOIA(x) /* x */
#define VERBOSE_POOL_PARANOIA(x) /* x */

///////////////////////////////////////////////////////////////////////////////
//
// ref_counter_pool implementation
//

ref_counter_pool ref_counter_pool::_the_instance;

ref_counter_pool&
ref_counter_pool::instance()
{
    return ref_counter_pool::_the_instance;
}

void
ref_counter_pool::grow()
{
    size_t old_size = _counters.size();
    _counters.resize(old_size + old_size / 8 + 1);

    for (size_t i = old_size; i < _counters.size(); i++) {
	_counters[i] = _free_index;
	_free_index = i;
    }
}

void
ref_counter_pool::check()
{
    int32_t i = _free_index;
    size_t n = 0;
    VERBOSE_POOL_PARANOIA(cout << "L: ");
    while (_counters[i] != LAST_FREE) {
	VERBOSE_POOL_PARANOIA(cout << i << " ");
	i = _counters[i];
	n++;
	if (n == _counters.size()) {
	    dump();
	    abort();
	}
    }
    VERBOSE_POOL_PARANOIA(cout << endl);
}

void
ref_counter_pool::dump()
{
    for (size_t i = 0; i < _counters.size(); i++) {
	cout << i << " " << _counters[i] << endl;
    }
    cout << "Free index: " << _free_index << endl;
}

ref_counter_pool::ref_counter_pool()
{
    const size_t n = 1;
    _counters.resize(n);
    _free_index = 0; // first free item
    _counters[n - 1] = LAST_FREE;
    grow();
    grow();
    VERBOSE_POOL_PARANOIA(dump());
}

int32_t
ref_counter_pool::new_counter()
{
    VERBOSE_POOL_PARANOIA(dump());
    if (_counters[_free_index] == LAST_FREE) {
	grow();
    }
    POOL_PARANOIA(assert(_counters[_free_index] != LAST_FREE));
    POOL_PARANOIA(check());
    int32_t new_counter = _free_index;
    _free_index = _counters[_free_index];
    _counters[new_counter] = 1;
    POOL_PARANOIA(check());
    return new_counter;
}

int32_t
ref_counter_pool::incr_counter(int32_t index)
{
    POOL_PARANOIA(check());
    assert((size_t)index < _counters.size());
    ++_counters[index];
    POOL_PARANOIA(check());
    return _counters[index];
}

int32_t
ref_counter_pool::decr_counter(int32_t index)
{
    POOL_PARANOIA(assert((size_t)index < _counters.size()));
    int32_t c = --_counters[index];
    if (c == 0) {
	POOL_PARANOIA(check());
	/* recycle */
	_counters[index] = _free_index;
	_free_index = index;	
	VERBOSE_POOL_PARANOIA(dump()); 
	POOL_PARANOIA(check());
    }
    assert(c >= 0);
    return c;
}

int32_t
ref_counter_pool::count(int32_t index)
{
    POOL_PARANOIA(assert((size_t)index < _counters.size()));
    return _counters[index];
}

///////////////////////////////////////////////////////////////////////////////
//
// cref_counter_pool implementation
//

cref_counter_pool cref_counter_pool::_the_instance;

cref_counter_pool&
cref_counter_pool::instance()
{
    return cref_counter_pool::_the_instance;
}

void
cref_counter_pool::grow()
{
    size_t old_size = _counters.size();
    _counters.resize(2 * old_size);

    for (size_t i = old_size; i < _counters.size(); i++) {
	_counters[i].count = _free_index;
	_free_index = i;
    }
}

void
cref_counter_pool::check()
{
    int32_t i = _free_index;
    size_t n = 0;
    VERBOSE_POOL_PARANOIA(cout << "L: ");
    while (_counters[i].count != LAST_FREE) {
	VERBOSE_POOL_PARANOIA(cout << i << " ");
	i = _counters[i].count;
	n++;
	if (n == _counters.size()) {
	    dump();
	    abort();
	}
    }
    VERBOSE_POOL_PARANOIA(cout << endl);
}

void
cref_counter_pool::dump()
{
    for (size_t i = 0; i < _counters.size(); i++) {
	cout << i << " " << _counters[i].count << endl;
    }
    cout << "Free index: " << _free_index << endl;
}

cref_counter_pool::cref_counter_pool()
{
    const size_t n = 1;
    _counters.resize(n);
    _free_index = 0; // first free item
    _counters[n - 1].count = LAST_FREE;
    grow();
    grow();
    VERBOSE_POOL_PARANOIA(dump());
}

int32_t
cref_counter_pool::new_counter(void* data)
{
    VERBOSE_POOL_PARANOIA(dump());
    if (_counters[_free_index].count == LAST_FREE) {
	grow();
    }
    POOL_PARANOIA(assert(_counters[_free_index].count != LAST_FREE));
    POOL_PARANOIA(check());
    int32_t new_counter = _free_index;
    _free_index = _counters[_free_index].count;
    _counters[new_counter].count = 1;
    _counters[new_counter].data = data;
    POOL_PARANOIA(check());
    return new_counter;
}

int32_t
cref_counter_pool::incr_counter(int32_t index)
{
    POOL_PARANOIA(check());
    assert((size_t)index < _counters.size());
    _counters[index].count++;
    POOL_PARANOIA(check());
    return _counters[index].count;
}

int32_t
cref_counter_pool::decr_counter(int32_t index)
{
    POOL_PARANOIA(assert((size_t)index < _counters.size()));
    int32_t c = --_counters[index].count;
    if (c == 0) {
	POOL_PARANOIA(check());
	/* recycle */
	_counters[index].count = _free_index;
	_free_index = index;	
	VERBOSE_POOL_PARANOIA(dump()); 
	POOL_PARANOIA(check());
    }
    assert(c >= 0);
    return c;
}

int32_t
cref_counter_pool::count(int32_t index)
{
    POOL_PARANOIA(assert((size_t)index < _counters.size()));
    return _counters[index].count;
}

void*
cref_counter_pool::data(int32_t index)
{
    POOL_PARANOIA(assert((size_t)index < _counters.size()));
    return _counters[index].data;
}
