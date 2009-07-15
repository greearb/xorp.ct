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



#include "xrl_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include <map>

#include "xrl_pf_factory.hh"
#include "xrl_pf_inproc.hh"
#include "xrl_pf_sudp.hh"
#include "xrl_pf_stcp.hh"
#include "xrl_pf_kill.hh"
#include "xrl_pf_unix.hh"


// STCP senders are a special case.  Constructing an STCP sender has
// real cost, unlike InProc and SUDP, so we maintain a cache of
// STCP senders with one per sender destination address.

XrlPFSender*
XrlPFSenderFactory::create_sender(EventLoop&	eventloop,
				  const char*	protocol,
				  const char*	address)
{
    debug_msg("instantiating sender pf = \"%s\", addr = \"%s\"\n",
	      protocol, address);
    try {
	if (strcmp(XrlPFSUDPSender::protocol_name(), protocol) == 0) {
	    return new XrlPFSUDPSender(eventloop, address);
	} else if (strcmp(XrlPFSTCPSender::protocol_name(), protocol) == 0) {
	    return new XrlPFSTCPSender(eventloop, address);
	} else if (strcmp(XrlPFInProcSender::protocol_name(), protocol) == 0) {
	    return new XrlPFInProcSender(eventloop, address);
	} else if (strcmp(XrlPFKillSender::protocol_name(), protocol) == 0) {
	    return new XrlPFKillSender(eventloop, address);
	} else if (strcmp(XrlPFUNIXSender::protocol_name(), protocol) == 0)
	    return new XrlPFUNIXSender(eventloop, address);
    } catch (XorpException& e) {
	XLOG_ERROR("XrlPFSenderFactory::create failed: %s\n", e.str().c_str());
    }
    return 0;
}

XrlPFSender*
XrlPFSenderFactory::create_sender(EventLoop& eventloop,
				  const char* protocol_colon_address)
{
    char *colon = strstr(const_cast<char*>(protocol_colon_address), ":");
    if (colon == 0) {
	debug_msg("No colon in supposedly colon separated <protocol><address>"
		  "combination\n\t\"%s\".\n", protocol_colon_address);
	return 0;
    }

    string protocol(protocol_colon_address, colon - protocol_colon_address);
    return create_sender(eventloop, protocol.c_str(), colon + 1);
}

void
XrlPFSenderFactory::destroy_sender(XrlPFSender* s)
{
    debug_msg("destroying sender pf = \"%s\", addr = \"%s\"\n",
	      s->protocol(), s->address().c_str());

    delete s;
}

void
XrlPFSenderFactory::startup()
{
}

void
XrlPFSenderFactory::shutdown()
{
}
