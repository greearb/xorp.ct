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

#ident "$XORP$"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "libxorp/xorp.h"

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/uio.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#include <string>

#include "xrl_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/exceptions.hh"

#include "header.hh"
#include "xrl_error.hh"
#include "xrl_pf_kill.hh"
#include "xrl_dispatcher.hh"
#include "xuid.hh"
#include "sockutil.hh"

// ----------------------------------------------------------------------------
// SUDP is "simple udp" - a minimalist and simple udp transport
// mechanism for XRLs.  It is intended as a placeholder to allow
// modules to start using XRL whilst we develop other mechanisms.
//
// Resolved names have protocol name "sudp" and specify addresses as
// "host/port"
//

// ----------------------------------------------------------------------------
// Constants

const char		XrlPFKillSender::_protocol[] = "kill";

// ----------------------------------------------------------------------------
// XrlPFKillSender

XrlPFKillSender::XrlPFKillSender(EventLoop& e, const char* pid_str)
    throw (XrlPFConstructorError)
    : XrlPFSender(e, pid_str)
{
    char* end_ptr;
    long l = strtol(pid_str, &end_ptr, 0);
    if (!*pid_str || *end_ptr || ((l == LONG_MIN || l == LONG_MAX) && errno == ERANGE))
	xorp_throw(XrlPFConstructorError,
		   c_format("Bad process ID: %s\n", pid_str));

    _pid = l;
}

XrlPFKillSender::~XrlPFKillSender()
{
}

void
XrlPFKillSender::send(const Xrl& x, const XrlPFSender::SendCallback& cb)
{
    try {
	int32_t sig = x.args().get_int32("signal");
	int err = ::kill(_pid, sig);
	if (err < 0)
	    cb->dispatch(XrlError(COMMAND_FAILED, strerror(errno)), 0);
	cb->dispatch(XrlError(), 0);
	return;
    } catch (XrlArgs::XrlAtomNotFound) {
    }

    cb->dispatch(XrlError(SEND_FAILED, "Bad XRL format"), 0);
}

bool
XrlPFKillSender::sends_pending() const
{
    return false;
}

bool
XrlPFKillSender::alive() const
{
    return true;
}

const char*
XrlPFKillSender::protocol() const
{
    return _protocol;
}
