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

#ident "$XORP: xorp/libxipc/xrl_dispatcher.cc,v 1.3 2003/06/19 00:44:43 hodson Exp $"

#include "libxorp/debug.h"
#include "xrl_dispatcher.hh"

// ----------------------------------------------------------------------------
// XrlDispatcher methods

XrlError
XrlDispatcher::dispatch_xrl(const string&  method_name,
			    const XrlArgs& inputs,
			    XrlArgs&       outputs) const
{
    const XrlCmdEntry* c = get_handler(method_name.c_str());
    if (c) {
	return c->dispatch(inputs, &outputs);
    }
    debug_msg("No handler for %s\n", method_name.c_str());
    return XrlError::NO_SUCH_METHOD();
}
