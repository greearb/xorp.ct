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

#ident "$XORP$"

#include "policy/policy_module.h"
#include "element_base.hh"
#include "libxorp/xlog.h"

Element::~Element()
{
}

Element::Element() : _refcount(1)
{
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
