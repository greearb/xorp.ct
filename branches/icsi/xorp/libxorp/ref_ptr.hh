// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/libxorp/ref_ptr.hh,v 1.11 2002/12/09 18:29:13 hodson Exp $

#ifndef __LIBXORP_REF_PTR_HH__
#define __LIBXORP_REF_PTR_HH__

#include "config.h"
#include "xorp.h"
#include <vector>
#include <stdio.h>

/**
 * @short class for maintaining the storage of counters used by ref_ptr.
 *
 * The ref_counter_pool is a singleton class that maintains the counters
 * for all ref_ptr objects.  The counters are maintained in a vector.  This
 * class is used by ref_ptr and not intended any other purpose.
 */
class ref_counter_pool
{
private:
    vector<int32_t> _counters;
    int		    _free_index;

    static const int LAST_FREE = -1;
    static ref_counter_pool _the_instance;

    /**
     * Expand counter storage.
     */
    void grow();

public:
    /**
     * Create a new counter.
     * @return index associated with counter.
     */
    int new_counter();

    /**
     * Increment the count associated with counter by 1.
     * @param index the counter to increment.
     */
    int incr_counter(int index);

    /**
     * Decrement the count associated with counter by 1.
     * @param index the counter to decrement.
     */
    int decr_counter(int index);

    /**
     * Get the count associated with counter.
     * @param index of the counter to query.
     * @return the counter value.
     */
    int count(int index);

    /**
     * Recycle counter.  Places counter on free-list.
     * @param index of the counter to recycle.
     */
    void recycle(int index);

    /**
     * Dumps counter info to stdout.  Debugging function.
     */
    void dump();

    /**
     * Sanity check internal data structure.  Debugging function.
     */
    void check();

    /**
     * @return singleton ref_counter_pool.
     */
    static ref_counter_pool& instance();


    ref_counter_pool();
};

/**
 * @short Reference Counted Pointer Class.
 *
 * The ref_ptr class is a strong reference class.  It maintains a count of
 * how many references to an object exist and releases the memory associated
 * with the object when the reference count reaches zero.  The reference 
 * pointer can be dereferenced like an ordinary pointer to call methods
 * on the reference counted object.
 *
 * At the time of writing the only supported memory management is
 * through the new and delete operators.  At a future date, this class
 * should support the STL allocator classes or an equivalent to
 * provide greater flexibility.
 */
template <class _Tp> 
class ref_ptr {
public:
    /**
     * Construct a reference pointer for object.
     *
     * @param p pointer to object to be reference counted.  p must be 
     * allocated using operator new as it will be destructed using delete
     * when the reference count reaches zero.
     */
    ref_ptr(_Tp* __p = 0) 
        : _M_ptr(__p), 
	_M_counter(ref_counter_pool::instance().new_counter())
	{}

    /**
     * Copy Constructor
     * 
     * Constructs a reference pointer for object.  Raises reference count
     * associated with object by 1.
     */
    ref_ptr(const ref_ptr& __r)
        : _M_ptr(0), _M_counter(-1) {
        ref(&__r);
    }

    /**
     * Assignment Operator
     *
     * Assigns reference pointer to new object.
     */
    ref_ptr& operator=(const ref_ptr& __r) {
        if (&__r != this) {
            unref();
            ref(&__r);
        }
        return *this;
    }

    /**
     * Destruct reference pointer instance and lower reference count on
     * object being tracked.  The object being tracked will be deleted if
     * the reference count falls to zero because of the destruction of the
     * reference pointer.
     */
    ~ref_ptr() {
        unref();
    }

    /**
     * Dereference reference counted object.
     * @return reference to object.
     */
    inline _Tp& operator*() const {
        return *_M_ptr;
    }

    /**
     * Dereference pointer to refence counted object.
     * @return pointer to object.
     */
    inline _Tp* operator->() const {
        return _M_ptr;
    }

    /**
     * Dereference pointer to refence counted object.
     * @return pointer to object.
     */
    inline _Tp* get() const {
	return _M_ptr;
    }

    /**
     * Equality Operator
     * @return true if reference pointers refer to same object.
     */
    inline bool operator==(const ref_ptr& rp) const {
	return _M_ptr == rp._M_ptr;
    }
    
    /**
     * Check if reference pointer refers to an object or whether it has
     * been assigned a null object.
     * @return true if reference pointer refers to a null object.
     */
    inline bool is_empty() const {
	return _M_ptr == 0;
    }

    /**
     * @return true if reference pointer represents only reference to object.
     */
    inline bool is_only() const {
	return ref_counter_pool::instance().count(_M_counter) == 1;
    }

    /**
     * @param n minimum count.
     * @return true if there are at least n references to object.
     */
    bool at_least(int n) const {
	return ref_counter_pool::instance().count(_M_counter) >= n;
    }

    /**
     * Release reference on object.  The reference pointers underlying
     * object is set to null, and the former object is destructed if
     * necessary.  
     */
    inline void release() const {
	unref();
    }

private:
    /**
     * Add reference.
     */
    void ref(const ref_ptr* __r) const {
	_M_ptr = __r->_M_ptr;
	_M_counter = __r->_M_counter;
	ref_counter_pool::instance().incr_counter(_M_counter);
    }

    /**
     * Remove reference.
     */
    void unref() const {
	if (_M_ptr && 
	    ref_counter_pool::instance().decr_counter(_M_counter) == 0) {
	    delete _M_ptr;
	}
	_M_ptr = 0;
    }

    mutable _Tp* _M_ptr;
    mutable int	 _M_counter;	// index in ref_counter_pool
};

#endif // __LIBXORP_REF_PTR_HH__
