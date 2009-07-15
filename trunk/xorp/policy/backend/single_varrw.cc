// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "policy/policy_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "policy/common/elem_null.hh"
#include "single_varrw.hh"

SingleVarRW::SingleVarRW() : _trashc(0), _did_first_read(false), _pt(NULL)
{
    memset(&_elems, 0, sizeof(_elems));
    memset(&_modified, 0, sizeof(_modified));
}

SingleVarRW::~SingleVarRW()
{
    for (unsigned i = 0; i < _trashc; i++)
        delete _trash[i];
}

const Element&
SingleVarRW::read(const Id& id)
{
    // Maybe there was a write before a read for this variable, if so, just
    // return the value... no need to bother the client.
    const Element* e = _elems[id];

    // nope... no value found.
    if(!e) {

	// if it's the first read, inform the client.
	if(!_did_first_read) {
	    start_read();
	    _did_first_read = true;

	    // try again, old clients initialize on start_read()
	    e = _elems[id];

	    // no luck... need to explicitly read...
	    if (!e)
		initialize(id, single_read(id));
	}
	// client already had chance to initialize... but apparently didn't...
	else
	   initialize(id, single_read(id));

	// the client may have initialized the variables after the start_read
	// marker, so try reading again...
	e = _elems[id];

	// out of luck...
	if(!e)
	    xorp_throw(SingleVarRWErr, "Unable to read variable " + id);
    }

    return *e;
}

void
SingleVarRW::write(const Id& id, const Element& e)
{
    // XXX no paranoid checks on what we write

    _elems[id] = &e;
    _modified[id] = true;
}

void
SingleVarRW::sync()
{
    bool first = true;

    // it's faster doing it this way rather than STL set if VAR_MAX is small...
    for (unsigned i = 0; i < VAR_MAX; i++) {
	if (!_modified[i])
	    continue;

	const Element* e = _elems[i];
	XLOG_ASSERT(e);
	_modified[i] = false;

	if (first) {
	    // alert derived class we are committing
	    start_write();
	    first = false;
	}

	if (_pt) {
	    switch (i) {
	    case VAR_POLICYTAGS:
		_pt->set_ptags(*e);
		continue;

	    case VAR_TAG:
		_pt->set_tag(*e);
		continue;
	    }
	}

	single_write(i, *e);
    }

    // done commiting [so the derived class may sync]
    end_write();

    // clear cache
    memset(&_elems, 0, sizeof(_elems));
    
    // delete all garbage
    for (unsigned i = 0; i < _trashc; i++)
        delete _trash[i];
    _trashc = 0;
}

void
SingleVarRW::initialize(const Id& id, Element* e)
{
    // check if we already have a value for a variable.
    // if so, do nothing.
    //
    // Consider clients initializing variables on start_read.
    // Consider a variable being written to before any reads. In such a case, the
    // SingleVarRW will already have the correct value for that variable, so we
    // need to ignore any initialize() called for that variable.
    if(_elems[id]) {
	if(e)
	    delete e;
	return;
    }

    // special case nulls [for supported variables, but not present in this
    // particular case].
    if(!e)
	e = new ElemNull();
    
    _elems[id] = e;

    // we own the pointers.
    XLOG_ASSERT(_trashc < sizeof(_trash)/sizeof(Element*));
    _trash[_trashc] = e;
    _trashc++;
}

void
SingleVarRW::initialize(PolicyTags& pt)
{
    _pt = &pt;

    initialize(VAR_POLICYTAGS, _pt->element());
    initialize(VAR_TAG, _pt->element_tag());
}
