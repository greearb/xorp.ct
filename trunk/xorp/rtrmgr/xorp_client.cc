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

#ident "$XORP: xorp/rtrmgr/xorp_client.cc,v 1.17 2004/01/14 21:36:08 pavlin Exp $"

// #define DEBUG_LOGGING
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
    : _eventloop(eventloop),
      _xrlrouter(xrlrouter)
{
}

void
XorpClient::send_now(const Xrl& xrl, XrlRouter::XrlCallback cb, 
		     const string& xrl_return_spec, bool do_exec) 
{
    if (do_exec) {
	debug_msg("send_sync before sending\n");
	_xrlrouter.send(xrl, cb);
	debug_msg("send_sync after sending\n");
    } else {
	debug_msg("send_sync before sending\n");
	debug_msg("DUMMY SEND: immediate callback dispatch\n");
	if (!cb.is_empty()) {
	    _delay_timer = _eventloop.new_oneoff_after_ms(0,
				callback(this, &XorpClient::fake_send_done, 
					 xrl_return_spec, cb));
	}
	debug_msg("send_sync after sending\n");
    }
}

/**
 * fake_send_done decouples the XRL response callback for debug_mode calls
 * to send_now, so that the callback doesn't happen until after
 * send_now has returned.  
 */
void XorpClient::fake_send_done(string xrl_return_spec,
				XrlRouter::XrlCallback cb) 
{
    XrlArgs args = fake_return_args(xrl_return_spec);
    cb->dispatch(XrlError::OKAY(), &args);
}

/**
 * fake_return_args is purely needed for testing, so that XRLs that
 * should return a value don't completely fail
 */
XrlArgs 
XorpClient::fake_return_args(const string& xrl_return_spec)
{
    list<string> args;
    string s = xrl_return_spec;

    debug_msg("fake_return_args %s\n", xrl_return_spec.c_str());

    if (xrl_return_spec.empty())
	return XrlArgs();

    while (true) {
	string::size_type start = s.find("&");
	if (start == string::npos) {
	    args.push_back(s);
	    break;
	}
	args.push_back(s.substr(0, start));
	s = s.substr(start + 1, s.size() - (start + 1));
    }

    XrlArgs xargs;
    list<string>::const_iterator iter;
    for (iter = args.begin(); iter != args.end(); ++iter) {
	string::size_type eq = iter->find("=");
	XrlAtom atom;

	debug_msg("ARG: %s\n", iter->c_str());

	if (eq == string::npos) {
	    debug_msg("ARG2: %s\n", iter->c_str());
	    atom = XrlAtom(iter->c_str());
	} else {
	    debug_msg("ARG2: >%s<\n", iter->substr(0, eq).c_str());
	    atom = XrlAtom(iter->substr(0, eq).c_str());
	}

	switch (atom.type()) {
	case xrlatom_no_type:
	    XLOG_UNREACHABLE();
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
	    XLOG_UNFINISHED();
	    break;
	}
    }
    debug_msg("ARGS: %s\n", xargs.str().c_str());
    return xargs;
}

#if 0
int
XorpClient::send_xrl(const UnexpandedXrl& unexpanded_xrl, string& errmsg,
		     XrlRouter::XrlCallback cb, bool do_exec)
{
    Xrl* xrl = unexpanded_xrl.expand(errmsg);

    if (xrl == NULL)
	return XORP_ERROR;
    string return_spec = unexpanded_xrl.return_spec();
    send_now(*xrl, cb, return_spec, do_exec);
    return XORP_OK;
}
#endif // 0
