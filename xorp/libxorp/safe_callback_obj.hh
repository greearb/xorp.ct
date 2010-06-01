// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2010 XORP, Inc and Others
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

// $XORP: xorp/libxorp/safe_callback_obj.hh,v 1.11 2008/10/02 21:57:33 bms Exp $

#ifndef __LIBXORP_SAFE_CALLBACK_OBJ_HH__
#define __LIBXORP_SAFE_CALLBACK_OBJ_HH__

#include <algorithm>
#include <vector>
#include "xorp.h"


struct SafeCallbackBase;

/**
 * @short Base class for objects that are callback safe.
 *
 * Objects that wish to be callback safe should be derived from this
 * class.  The class works in conjunction with SafeCallbackBase and
 * between them implement callback and callback-object tracking.  When
 * a CallbackSafeObject is destructed it informs all the callbacks that
 * refer to it that this is the case and invalidates (sets to null)
 * the object they point to.
 *
 * Copy operations are not supported.  It's hard to know what the
 * correct thing to do on assignment or copy, so best bet is not
 * to do anything.
 */
class CallbackSafeObject :
    public NONCOPYABLE
{
public:
    CallbackSafeObject() {}
    virtual ~CallbackSafeObject();

    void ref_cb(SafeCallbackBase* scb);
    void unref_cb(SafeCallbackBase* scb);

protected:
    std::vector<SafeCallbackBase*> _cbs;
};

/**
 * @short Base class for safe callbacks.
 *
 * These are object callbacks that are only dispatched if target of
 * callback is non-null.
 */
class SafeCallbackBase :
    public NONCOPYABLE
{
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
    SafeCallbackBase();			  // Not directly constructible

protected:
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
