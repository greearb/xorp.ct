// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/libxipc/xrl_dispatcher.cc,v 1.5 2003/09/16 21:53:57 hodson Exp $"

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
    inline bool on() const { return _do_trace; }
    operator bool() { return _do_trace; }

protected:
    bool _do_trace;
} xrl_trace;

#define trace_xrl_dispatch(p, x) 					      \
do {									      \
    if (xrl_trace.on()) XLOG_INFO((string(p) + x).c_str());	      	      \
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
