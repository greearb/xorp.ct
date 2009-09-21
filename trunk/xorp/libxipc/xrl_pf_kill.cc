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



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "xrl_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/exceptions.hh"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

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

bool
XrlPFKillSender::send(const Xrl&			x,
		      bool				direct_call,
		      const XrlPFSender::SendCallback&	cb)
{
#ifdef HOST_OS_WINDOWS
    // XXX: Windows has no notion of process signals.
    return false;
    UNUSED(x);
    UNUSED(direct_call);
    UNUSED(cb);
#else
    try {
	int32_t sig = x.args().get_int32("signal");
	int err = ::kill(_pid, sig);
	if (direct_call) {
	    return false;
	} else {
	    if (err < 0)
		cb->dispatch(XrlError(COMMAND_FAILED, strerror(errno)), 0);
	    else
		cb->dispatch(XrlError::OKAY(), 0);
	    return true;
	}
    } catch (XrlArgs::BadArgs) {
    }

    if (direct_call) {
	return false;
    } else {
	cb->dispatch(XrlError(SEND_FAILED, "Bad XRL format"), 0);
	return true;
    }
#endif
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
