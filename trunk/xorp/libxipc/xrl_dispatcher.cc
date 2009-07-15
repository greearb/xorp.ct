// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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



#include "ipc_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "xrl_dispatcher.hh"


// ----------------------------------------------------------------------------
// Xrl Tracing central

static class TraceXrl {
public:
    TraceXrl() {
	_do_trace = !(getenv("XRLDISPATCHTRACE") == 0);
    }
    bool on() const { return _do_trace; }
    operator bool() { return _do_trace; }

protected:
    bool _do_trace;
} xrl_trace;

#define trace_xrl_dispatch(p, x) 					      \
do {									      \
    if (xrl_trace.on()) XLOG_INFO("%s", (string(p) + x).c_str());	      \
} while (0)

// ----------------------------------------------------------------------------
// XrlDispatcher methods

XrlError
XrlDispatcher::dispatch_xrl(const string&  method_name,
			    const XrlArgs& inputs,
			    XrlArgs&       outputs) const
{
    const XrlCmdEntry* c = get_handler(method_name.c_str());
    if (c == 0) {
	trace_xrl_dispatch("dispatch_xrl (invalid) ", method_name);
	debug_msg("No handler for %s\n", method_name.c_str());
	return XrlError::NO_SUCH_METHOD();
    }

    trace_xrl_dispatch("dispatch_xrl (valid) ", method_name);
    return c->dispatch(inputs, &outputs);
}

XrlDispatcher::XI*
XrlDispatcher::lookup_xrl(const string& name) const
{
    const XrlCmdEntry* c = get_handler(name.c_str());
    if (!c)
	return NULL;

    return new XI(c);
}

XrlError
XrlDispatcher::dispatch_xrl_fast(const XI& xi, XrlArgs& outputs) const
{
    return xi._cmd->dispatch(xi._xrl.args(), &outputs);
}
