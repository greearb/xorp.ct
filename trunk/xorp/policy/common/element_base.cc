// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008 International Computer Science Institute
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

#ident "$XORP: xorp/policy/common/element_base.cc,v 1.1 2008/08/06 08:10:18 abittau Exp $"

#include "policy/policy_module.h"
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
