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
#include "element_base.hh"
#include "libxorp/xlog.h"
#include "policy_exception.hh"
#include "element.hh"

Element::~Element()
{
}

Element::Element(Hash hash) : _refcount(1), _hash(hash)
{
    if (_hash >= HASH_ELEM_MAX)
        xorp_throw(PolicyException,
                   "Too many elems for dispatcher---find a better hashing mechanism\n");
}

// TODO do a proper refcount implementation, factory, object reuse, etc.
void
Element::ref() const
{
    _refcount++;
    XLOG_ASSERT(_refcount);
}

void
Element::unref()
{
    XLOG_ASSERT(_refcount > 0);

    _refcount--;

    if (_refcount == 0)
        delete this;
}

uint32_t
Element::refcount() const
{
    return _refcount;
}

Element::Hash
Element::hash() const
{
    return _hash;
}
