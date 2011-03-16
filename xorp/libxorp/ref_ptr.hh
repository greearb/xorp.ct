// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// $XORP: xorp/libxorp/ref_ptr.hh,v 1.26 2008/10/02 21:57:32 bms Exp $

#ifndef __LIBXORP_REF_PTR_HH__
#define __LIBXORP_REF_PTR_HH__

#include "libxorp/xorp.h"




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
    int32_t	    _free_index;
    int32_t	    _balance;

    static const int32_t LAST_FREE = -1;
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
    int32_t new_counter();

    /**
     * Increment the count associated with counter by 1.
     * @param index the counter to increment.
     */
    int32_t incr_counter(int32_t index);

    /**
     * Decrement the count associated with counter by 1.
     * @param index the counter to decrement.
     */
    int32_t decr_counter(int32_t index);

    /**
     * Get the count associated with counter.
     * @param index of the counter to query.
     * @return the counter value.
     */
    int32_t count(int32_t index);

    /**
     * Recycle counter.  Places counter on free-list.
     * @param index of the counter to recycle.
     */
    void recycle(int32_t index);

    /**
     * Dumps counter info to stdout.  Debugging function.
     */
    void dump();

    /**
     * Sanity check internal data structure.  Debugging function.
     */
    void check();

    /**
     * Check index is on free list.
     */
    bool on_free_list(int32_t index);

    /**
     * Return number of valid ref pointer entries in pool.
     */
    int32_t balance() const { return _balance; }

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
        : _M_ptr(__p), _M_index(0)
    {
	if (_M_ptr)
	    _M_index = ref_counter_pool::instance().new_counter();
    }

    /**
     * Copy Constructor
     *
     * Constructs a reference pointer for object.  Raises reference count
     * associated with object by 1.
     */
    ref_ptr(const ref_ptr& __r)
        : _M_ptr(0), _M_index(-1) {
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
    _Tp& operator*() const { return *_M_ptr; }

    /**
     * Dereference pointer to reference counted object.
     * @return pointer to object.
     */
    _Tp* operator->() const { return _M_ptr; }

    /**
     * Dereference pointer to reference counted object.
     * @return pointer to object.
     */
    _Tp* get() const { return _M_ptr; }

#ifdef XORP_USE_USTL

    // Compare pointed-to items.
    bool operator==(const ref_ptr& rp) const {
	if (_M_ptr == rp._M_ptr)
	    return true;
	if (_M_ptr && rp._M_ptr)
	    return (*_M_ptr == *rp._M_ptr);
	return false;
    }

    // Compare pointed-to items.
    bool operator< (const ref_ptr& b) {
	if (_M_ptr && b._M_ptr)
	    return (*_M_ptr < *b._M_ptr);
	if (_M_ptr == b._M_ptr)
	    return false;
	if (b._M_ptr)
	    return true;
    }

#else
    /**
     * Equality Operator
     * @return true if reference pointers refer to same object.
     */
    bool operator==(const ref_ptr& rp) const { return _M_ptr == rp._M_ptr; }
#endif

    /**
     * Check if reference pointer refers to an object or whether it has
     * been assigned a null object.
     * @return true if reference pointer refers to a null object.
     */
    bool is_empty() const { return _M_ptr == 0; }

    /**
     * @return true if reference pointer represents only reference to object.
     */
    bool is_only() const {
	return ref_counter_pool::instance().count(_M_index) == 1;
    }

    /**
     * @param n minimum count.
     * @return true if there are at least n references to object.
     */
    bool at_least(int32_t n) const {
	return ref_counter_pool::instance().count(_M_index) >= n;
    }

    /**
     * Release reference on object.  The reference pointers underlying
     * object is set to null, and the former object is destructed if
     * necessary.
     */
    void release() const { unref(); }
    /* mimic functionality of boost weak_ptr, same as release()
     */
    void reset() const { unref(); }

    ref_ptr(_Tp* data, int32_t index) : _M_ptr(data), _M_index(index)
    {
	ref_counter_pool::instance().incr_counter(_M_index);
    }

protected:

    /**
     * Add reference.
     */
    void ref(const ref_ptr* __r) const {
	_M_ptr = __r->_M_ptr;
	_M_index = __r->_M_index;
	if (_M_ptr) {
	    ref_counter_pool::instance().incr_counter(_M_index);
	}
    }

    /**
     * Remove reference.
     */
    void unref() const {
	if (_M_ptr &&
	    ref_counter_pool::instance().decr_counter(_M_index) == 0) {
	    delete _M_ptr;
	}
	_M_ptr = 0;
    }

    mutable _Tp*    _M_ptr;
    mutable int32_t _M_index;	// index in ref_counter_pool
};

#if 0
template <typename _Tp>
ref_ptr<const _Tp>::ref_ptr(const ref_ptr<_Tp>& __r)
	: _M_ptr(0), _M_index(_r->_M_index)
{
    ref_counter_pool::instance().incr_counter(_M_index);
}
#endif

/**
 * @short class for maintaining the storage of counters used by cref_ptr.
 *
 * The cref_counter_pool is a singleton class that maintains the counters
 * for all cref_ptr objects.  The counters are maintained in a vector.  This
 * class is used by cref_ptr and not intended any other purpose.
 */
class cref_counter_pool
{
private:
    struct pool_item {
	int32_t	count;
	void*	data;
    };
    vector<pool_item> _counters;
    int32_t	      _free_index;

    static const int32_t LAST_FREE = -1;
    static cref_counter_pool _the_instance;

    /**
     * Expand counter storage.
     */
    void grow();

public:
    /**
     * Create a new counter.
     * @return index associated with counter.
     */
    int32_t new_counter(void *data);

    /**
     * Increment the count associated with counter by 1.
     * @param index the counter to increment.
     */
    int32_t incr_counter(int32_t index);

    /**
     * Decrement the count associated with counter by 1.
     * @param index the counter to decrement.
     */
    int32_t decr_counter(int32_t index);

    /**
     * Get the count associated with counter.
     * @param index of the counter to query.
     * @return the counter value.
     */
    int32_t count(int32_t index);

    void* data(int32_t index);

    /**
     * Recycle counter.  Places counter on free-list.
     * @param index of the counter to recycle.
     */
    void recycle(int32_t index);

    /**
     * Dumps counter info to stdout.  Debugging function.
     */
    void dump();

    /**
     * Sanity check internal data structure.  Debugging function.
     */
    void check();

    /**
     * @return singleton cref_counter_pool.
     */
    static cref_counter_pool& instance();

    cref_counter_pool();
};

/**
 * @short Compact Reference Counted Pointer Class.
 *
 * The cref_ptr class is a strong reference class.  It maintains a count of
 * how many references to an object exist and releases the memory associated
 * with the object when the reference count reaches zero.  The reference 
 * pointer can be dereferenced like an ordinary pointer to call methods
 * on the reference counted object.
 *
 * In contrast to the ref_ptr class, accessing the pointer requires
 * two levels of indirection as the pointer is stored in the same
 * table as the counter.  This is a performance hit, but means each
 * cref_ptr only consumes 4-bytes.
 *
 * At the time of writing the only supported memory management is
 * through the new and delete operators.  At a future date, this class
 * should support the STL allocator classes or an equivalent to
 * provide greater flexibility.
 */
template <class _Tp>
class cref_ptr {
public:
    /**
     * Construct a compact reference pointer for object.
     *
     * @param p pointer to object to be reference counted.  p must be
     * allocated using operator new as it will be destructed using delete
     * when the reference count reaches zero.
     */
    cref_ptr(_Tp* __p = 0)
        : _M_index(cref_counter_pool::instance().new_counter(__p))
	{}

    /**
     * Copy Constructor
     *
     * Constructs a reference pointer for object.  Raises reference count
     * associated with object by 1.
     */
    cref_ptr(const cref_ptr& __r)
        : _M_index(-1) {
        ref(&__r);
    }

    /**
     * Assignment Operator
     *
     * Assigns reference pointer to new object.
     */
    cref_ptr& operator=(const cref_ptr& __r) {
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
    ~cref_ptr() {
        unref();
    }

    /**
     * Dereference pointer to reference counted object.
     * @return pointer to object.
     */
    _Tp* get() const {
	return reinterpret_cast<_Tp*>
	    (cref_counter_pool::instance().data(_M_index));
    }

    /**
     * Dereference reference counted object.
     * @return reference to object.
     */
    _Tp& operator*() const { return *(get()); }

    /**
     * Dereference pointer to reference counted object.
     * @return pointer to object.
     */
    _Tp* operator->() const { return get(); }

    /**
     * Equality Operator
     * @return true if reference pointers refer to same object.
     */
    bool operator==(const cref_ptr& rp) const { return get() == rp.get(); }

    /**
     * Check if reference pointer refers to an object or whether it has
     * been assigned a null object.
     * @return true if reference pointer refers to a null object.
     */
    bool is_empty() const { return _M_index < 0 || 0 == get(); }

    /**
     * @return true if reference pointer represents only reference to object.
     */
    bool is_only() const {
	return cref_counter_pool::instance().count(_M_index) == 1;
    }

    /**
     * @param n minimum count.
     * @return true if there are at least n references to object.
     */
    bool at_least(int32_t n) const {
	return cref_counter_pool::instance().count(_M_index) >= n;
    }

    /**
     * Release reference on object.  The reference pointers underlying
     * object is set to null, and the former object is destructed if
     * necessary.
     */
    void release() const { unref(); }

private:
    /**
     * Add reference.
     */
    void ref(const cref_ptr* __r) const {
	_M_index = __r->_M_index;
	cref_counter_pool::instance().incr_counter(_M_index);
    }

    /**
     * Remove reference.
     */
    void unref() const {
	if (_M_index >= 0 &&
	    cref_counter_pool::instance().decr_counter(_M_index) == 0) {
	    delete get();
	    _M_index = -1;
	}
    }

    mutable int32_t _M_index;	// index in cref_counter_pool
};

#endif // __LIBXORP_REF_PTR_HH__
