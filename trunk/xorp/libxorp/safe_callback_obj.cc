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

#ident "$XORP: xorp/devnotes/template.cc,v 1.2 2003/01/16 19:08:48 mjh Exp $"

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
    std::vector<SafeCallbackBase*>::iterator i = _cbs.begin();
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
    assert(_cbs.empty());
}

