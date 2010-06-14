// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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



#include "libxorp_module.h"

#include "libxorp/xorp.h"

#include "safe_callback_obj.hh"

// ----------------------------------------------------------------------------
// SafeCallbackBase implementation

SafeCallbackBase::SafeCallbackBase(CallbackSafeObject* o) : _cso(o)
{
    _cso->ref_cb(this);
}

SafeCallbackBase::~SafeCallbackBase()
{
    if (valid())
	invalidate();
}

void
SafeCallbackBase::invalidate()
{
    if (valid()) {
	_cso->unref_cb(this);
	_cso = 0;
    }
}

bool
SafeCallbackBase::valid() const
{
    return _cso != 0;
}


// ----------------------------------------------------------------------------
// CallbackSafeObject implementation

CallbackSafeObject::~CallbackSafeObject()
{
    vector<SafeCallbackBase*>::iterator i = _cbs.begin();
    while (_cbs.empty() == false) {
	SafeCallbackBase* scb = *i;
	if (scb == 0) {
	    _cbs.erase(_cbs.begin());
	    continue;
	}
	if (scb->valid()) {
	    scb->invalidate();
	}
    }
}

