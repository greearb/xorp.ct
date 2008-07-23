// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/policy/common/varrw.cc,v 1.9 2008/01/04 03:17:20 pavlin Exp $"

#include "policy/policy_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "varrw.hh"
#include "element.hh"


VarRW::VarRW() : _allow_trace(true), _trace(0)
{
}

VarRW::~VarRW()
{
}

const Element&
VarRW::read_trace(const Id& id)
{
    const Element& e = read(id);
    if (_allow_trace)
	_tracelog << "Read " << id << ": " << e.str() << endl;
    return e;
}

void
VarRW::write_trace(const Id& id, const Element& e)
{
    // trace is a special variable, not to be implemented by upper layers...
    if (id == VAR_TRACE) {
	XLOG_ASSERT(e.type() == ElemU32::id);
	
	const ElemU32& u32 = dynamic_cast<const ElemU32&>(e);
	_trace = u32.val();
	if (_allow_trace)
	    _tracelog << "Write " << id << ": " << _trace << endl;
	return;
    }

    if (_allow_trace)
	_tracelog << "Write " << id << ": " << e.str() << endl;
    write(id, e);
}

uint32_t
VarRW::trace()
{
    return _trace;
}

string
VarRW::tracelog()
{
    return _tracelog.str();
}

string
VarRW::more_tracelog()
{
    return "";
}

void
VarRW::reset_trace()
{
    _trace = 0;
}
