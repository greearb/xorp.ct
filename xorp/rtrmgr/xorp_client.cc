// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
//
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net




#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/timer.hh"

#include "template_commands.hh"
#include "unexpanded_xrl.hh"
#include "xorp_client.hh"


/***********************************************************************/
/* XorpClient                                                          */
/***********************************************************************/

XorpClient::XorpClient(EventLoop& eventloop, XrlRouter& xrl_router)
    : _eventloop(eventloop),
      _xrl_router(xrl_router)
{
}

XorpClient::~XorpClient() {
    _eventloop.remove_timer(_delay_timer);
}

void
XorpClient::send_now(const Xrl& xrl, XrlRouter::XrlCallback cb,
		     const string& xrl_return_spec, bool do_exec)
{
    if (do_exec) {
	debug_msg("send_sync before sending\n");
	_xrl_router.send(xrl, cb);
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
	uint8_t data[] = {0}; //Needed for xrlatom_binary
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
	    xargs.add(XrlAtom(atom.name().c_str(), IPv6("::")) );
	    break;
	case xrlatom_ipv6net:
	    xargs.add(XrlAtom(atom.name().c_str(), IPv6Net("::/0")) );
	    break;
	case xrlatom_mac:
	    xargs.add(XrlAtom(atom.name().c_str(), Mac("00:00:00:00:00:00")) );
	    break;
	case xrlatom_list:
	    xargs.add(XrlAtom(atom.name().c_str(), XrlAtomList()) );
	    break;
	case xrlatom_boolean:
	    xargs.add(XrlAtom(atom.name().c_str(), false) );
	    break;
	case xrlatom_binary:
	    xargs.add(XrlAtom(atom.name().c_str(), data, sizeof(data)) );
	    break;
	case xrlatom_uint64:
	    xargs.add(XrlAtom(atom.name().c_str(), (uint64_t)0) );
	    break;
	case xrlatom_int64:
	    xargs.add(XrlAtom(atom.name().c_str(), (int64_t)0) );
	    break;
	case xrlatom_fp64:
	    xargs.add(XrlAtom(atom.name().c_str(), (fp64_t)0) );
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
