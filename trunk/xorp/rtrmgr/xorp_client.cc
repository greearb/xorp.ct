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

#ident "$XORP: xorp/rtrmgr/xorp_client.cc,v 1.11 2003/05/01 07:55:28 mjh Exp $"

//#define DEBUG_LOGGING
#include "rtrmgr_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/eventloop.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/timer.hh"
#include "unexpanded_xrl.hh"
#include "module_manager.hh"
#include "template_commands.hh"
#include "xorp_client.hh"


/***********************************************************************/
/* XorpClient                                                          */
/***********************************************************************/

XorpClient::XorpClient(EventLoop& eventloop, XrlRouter& xrlrouter) 
    : _eventloop(eventloop), _xrlrouter(xrlrouter)
{
}

int
XorpClient::send_now(const Xrl &xrl, XrlRouter::XrlCallback cb, 
		     const string& xrl_return_spec,
		     bool do_exec) {
    if (do_exec) {
	debug_msg("send_sync before sending\n");
	_xrlrouter.send(xrl, cb);
	debug_msg("send_sync after sending\n");
    } else {
	debug_msg("send_sync before sending\n");
	debug_msg("DUMMY SEND: immediate callback dispatch\n");
	if (!cb.is_empty()) {
	    XrlArgs args = fake_return_args(xrl_return_spec);
	    cb->dispatch(XrlError::OKAY(), &args);
	}
	debug_msg("send_sync after sending\n");
    }
    return XORP_OK;
}

/* fake_return_args is purely needed for testing, so that XRLs that
   should return a value don't completely fail */
XrlArgs 
XorpClient::fake_return_args(const string& xrl_return_spec) {
    if (xrl_return_spec.empty()) {
	return XrlArgs();
    }
    debug_msg("fake_return_args %s\n", xrl_return_spec.c_str());
    list <string> args;
    string s = xrl_return_spec;
    while (1) {
	string::size_type start = s.find("&");
	if (start == string::npos) {
	    args.push_back(s);
	    break;
	}
	args.push_back(s.substr(0, start));
	s = s.substr(start+1, s.size()-(start+1));
    }
    XrlArgs xargs;
    list <string>::const_iterator i;
    for(i = args.begin(); i!= args.end(); i++) {
	debug_msg("ARG: %s\n", i->c_str());
	string::size_type eq = i->find("=");
	XrlAtom atom;
	if (eq == string::npos) {
	    debug_msg("ARG2: %s\n", i->c_str());
	    atom = XrlAtom(i->c_str());
	} else {
	    debug_msg("ARG2: >%s<\n", i->substr(0, eq).c_str());
	    atom = XrlAtom(i->substr(0, eq).c_str());
	}
	switch (atom.type()) {
	case xrlatom_no_type:
	    abort();
	case xrlatom_int32:
	    xargs.add(XrlAtom(atom.name().c_str(), (int32_t)0) );
	    break;
	case xrlatom_uint32:
	    xargs.add(XrlAtom(atom.name().c_str(), (uint32_t)0) );
	    break;
	case xrlatom_ipv4:
	    xargs.add(XrlAtom(atom.name().c_str(), IPv4("0.0.0.0")) );
	    break;
	case xrlatom_ipv4net:
	    xargs.add(XrlAtom(atom.name().c_str(), IPv4Net("0.0.0.0/0")) );
	    break;
	case xrlatom_text:
	    xargs.add(XrlAtom(atom.name().c_str(), string("")) );
	    break;
	case xrlatom_ipv6:
	case xrlatom_ipv6net:
	case xrlatom_mac:
	case xrlatom_list:
	case xrlatom_boolean:
	case xrlatom_binary:
	    //XXX TBD
	    abort();
	    break;
	}
    }
    debug_msg("ARGS: %s\n", xargs.str().c_str());
    return xargs;
}

int
XorpClient::send_xrl(const UnexpandedXrl& xrl, 
		     XrlRouter::XrlCallback cb,
		     bool do_exec)
{
    UNUSED(xrl);
    UNUSED(cb);
    UNUSED(do_exec);
    return XORP_OK;
}

