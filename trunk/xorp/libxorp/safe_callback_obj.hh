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

// $XORP: xorp/libxorp/safe_callback_obj.hh,v 1.1 2003/12/20 00:26:01 hodson Exp $

#ifndef __LIBXORP_SAFE_CALLBACK_OBJ_HH__
#define __LIBXORP_SAFE_CALLBACK_OBJ_HH__

#include <algorithm>
#include <vector>

struct SafeCallbackBase;

/**
 * @short Base class for objects that are callback safe.
 *
 * Objects that wish to be callback safe shout be derived from this
 * class.  The class works in conjunction with SafeCallbackBase and
 * between them implement callback and callback-object tracking.  When
 * a CallbackSafeObject is destructed it informs all the callbacks that
 * refer to it that this is the case and invalidates (sets to null)
 * the object they point to.
 */
class CallbackSafeObject {
public:
    inline CallbackSafeObject() {}
    virtual ~CallbackSafeObject();

    inline void ref_cb(SafeCallbackBase* scb);
    inline void unref_cb(SafeCallbackBase* scb);

protected:
    // Copy operations are not supported.  It's hard to know what the
    // correct thing to do on assignment or copy, so best bet is not
    // to do anything.
    CallbackSafeObject(const CallbackSafeObject&);		// Not implemented
    CallbackSafeObject& operator=(const CallbackSafeObject&);	// Not implemented

protected:
    std::vector<SafeCallbackBase*> _cbs;
};

/**
 * @short Base class for safe callbacks.
 *
 * These are object callbacks that are only dispatched if target of
 * callback is non-null.
 */
class SafeCallbackBase {
public:
    /**
     * Constructor.
     *
     * Informs CallbackSafeObject that this callback operates on it.
     */
    SafeCallbackBase(CallbackSafeObject* o);

    /**
     * Destructor.
     *
     * Informs CallbackSafeObject that is tracking callback
     * instances that this callback no longer exists.
     */
    ~SafeCallbackBase();

    void invalidate();

    bool valid() const;

protected:
    SafeCallbackBase();					  // Not implemented
    SafeCallbackBase(const SafeCallbackBase&);		  // Not implemented
    SafeCallbackBase& operator=(const SafeCallbackBase&); // Not implemented

    CallbackSafeObject* _cso;
};


// ----------------------------------------------------------------------------
// Inline CallbackSafeObject methods

inline void
CallbackSafeObject::ref_cb(SafeCallbackBase* scb)
{
    _cbs.push_back(scb);
}

inline void
CallbackSafeObject::unref_cb(SafeCallbackBase* scb)
{
    std::vector<SafeCallbackBase*>::iterator i =
	find(_cbs.begin(), _cbs.end(), scb);
    if (i != _cbs.end())
	_cbs.erase(i);
}

#endif // __LIBXORP_SAFE_CALLBACK_OBJ_HH__
