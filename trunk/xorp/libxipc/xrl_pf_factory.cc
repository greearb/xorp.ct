
// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/libxipc/xrl_pf_factory.cc,v 1.1.1.1 2002/12/11 23:56:04 hodson Exp $"

#include "config.h"

#include <string>

#include "xrl_module.h"
#include "xrl_pf_factory.hh"
#include "xrl_pf_sudp.hh"

#include "libxorp/debug.h"

XrlPFSender*
XrlPFSenderFactory::create(EventLoop& event_loop,
			   const char* protocol_colon_address) {
    char *colon = strstr(protocol_colon_address, ":");
    if (colon == NULL) {
	debug_msg("No colon in supposedly colon separated <protocol><address>"
		  "combination\n\t\"%s\".\n", protocol_colon_address);
	return NULL;
    }

    string protocol(protocol_colon_address, colon - protocol_colon_address);
    string address(colon + 1);

    try {
	char *address = colon + 1;	

	if (string(XrlPFSUDPSender::protocol()) == protocol)
	    return new XrlPFSUDPSender(event_loop, address);
    } catch (exception& e) {
	debug_msg("XrlPFSenderFactory::create failed: %s\n", e.what());
    }

    return NULL;
}




