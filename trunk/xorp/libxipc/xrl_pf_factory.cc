
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

#ident "$XORP: xorp/libxipc/xrl_pf_factory.cc,v 1.6 2003/04/22 23:27:19 hodson Exp $"

#include "config.h"

#include <string>
#include <map>

#include "xrl_module.h"
#include "xrl_pf_factory.hh"
#include "xrl_pf_inproc.hh"
#include "xrl_pf_sudp.hh"
#include "xrl_pf_stcp.hh"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

static bool s_running = false;

// STCP senders are a special case.  Constructing an STCP sender has
// real cost, unlike InProc and SUDP, so we maintain a cache of
// STCP senders with one per sender destination address.

static list<XrlPFSTCPSender*> stcps;

static XrlPFSTCPSender*
stcp_find_or_create(EventLoop& e, const char* address)
{
    list<XrlPFSTCPSender*>::iterator i;
    for (i = stcps.begin(); i != stcps.end(); ++i) {
	XrlPFSTCPSender* s = *i;
	if (s->address() == address) {
	    return s;
	}
    }

    stcps.push_back(new XrlPFSTCPSender(e, address));
    return stcps.back();
}

static void
stcp_cleanup()
{
    while (stcps.empty() == false) {
	delete stcps.front();
	stcps.pop_front();
    }
}

XrlPFSender*
XrlPFSenderFactory::create_sender(EventLoop&	eventloop,
				  const char*	protocol,
				  const char*	address)
{
    XLOG_ASSERT(s_running);
    debug_msg("instantiating sender pf = \"%s\", addr = \"%s\"\n",
	      protocol, address);
    try {
	if (strcmp(XrlPFSUDPSender::protocol_name(), protocol) == 0) {
	    return new XrlPFSUDPSender(eventloop, address);
	} else if (strcmp(XrlPFSTCPSender::protocol_name(), protocol) == 0) {
	    return stcp_find_or_create(eventloop, address);
	} else if (strcmp(XrlPFInProcSender::protocol_name(), protocol) == 0) {
	    return new XrlPFInProcSender(eventloop, address);
	}
    } catch (XorpException& e) {
	XLOG_ERROR("XrlPFSenderFactory::create failed: %s\n", e.str().c_str());
    }
    return 0;
}

XrlPFSender*
XrlPFSenderFactory::create_sender(EventLoop& eventloop,
				  const char* protocol_colon_address)
{
    XLOG_ASSERT(s_running);
    char *colon = strstr(protocol_colon_address, ":");
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
    XLOG_ASSERT(s_running);
    debug_msg("destroying sender pf = \"%s\", addr = \"%s\"\n",
	      s->protocol(), s->address().c_str());

    if (strcmp(s->protocol(), XrlPFSTCPSender::protocol_name()) == 0)
	return;

    delete s;
}

void
XrlPFSenderFactory::startup()
{
    s_running = true;
}

void
XrlPFSenderFactory::shutdown()
{
    s_running = false;
    stcp_cleanup();
}
