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

#include "varrw.hh"
#include "element.hh"


VarRW::VarRW() : _do_trace(false), _trace(0)
{
}

VarRW::~VarRW()
{
}

const Element&
VarRW::read_trace(const Id& id)
{
    const Element& e = read(id);

    if (_do_trace)
	_tracelog << "Read " << id << ": " << e.str() << endl;

    return e;
}

void
VarRW::write_trace(const Id& id, const Element& e)
{
    if (_do_trace)
	_tracelog << "Write " << id << ": " << e.str() << endl;

    // trace is a special variable, not to be implemented by upper layers...
    if (id == VAR_TRACE) {
	XLOG_ASSERT(e.type() == ElemU32::id);

	const ElemU32& u32 = dynamic_cast<const ElemU32&>(e);
	_trace = u32.val();

	return;
    }

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

void
VarRW::enable_trace(bool on)
{
    _do_trace = on;
}

void
VarRW::sync()
{
}
