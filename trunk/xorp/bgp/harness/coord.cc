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

#ident "$XORP: xorp/bgp/harness/coord.cc,v 1.12 2003/09/15 19:08:16 hodson Exp $"

#include "config.h"
#include "bgp/bgp_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/status_codes.h"

#include "libxorp/eventloop.hh"
#include "libxipc/xrl_std_router.hh"
#include "bgp/socket.hh"

#include "coord.hh"
#include "command.hh"

static const char SERVER[] = "coord";/* This servers name */
static const char SERVER_VERSION[] = "0.1";

/*
-------------------- XRL --------------------
*/

XrlCoordTarget::XrlCoordTarget(XrlRouter *r, Coord& coord)
    : XrlCoordTargetBase(r), _coord(coord), _incommand(0)
{
    debug_msg("\n");
}

XrlCmdError
XrlCoordTarget::common_0_1_get_target_name(string& name)
{
    debug_msg("\n");

    name = SERVER;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCoordTarget::common_0_1_get_version(string& version)
{
    debug_msg("\n");

    version = SERVER_VERSION;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCoordTarget::common_0_1_get_status(// Output values,
				      uint32_t& status,
				      string& reason)
{
    //XXX placeholder only
    status = PROC_READY;
    reason = "Ready";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCoordTarget::common_0_1_shutdown()
{
    _coord.mark_done();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCoordTarget::coord_0_1_command(const string&	command)
{
    debug_msg("command: <%s>\n", command.c_str());

    _incommand++;
    try {
	_coord.command(command);
    } catch(const XorpException& e) {
	_incommand--;
	return XrlCmdError::COMMAND_FAILED(e.why() + "\nPending operation: " +
					   (_coord.pending() ?
					   "true" : "false"));
    }
    _incommand--;

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCoordTarget::coord_0_1_status(const string& peer, string& status)
{
    debug_msg("status: %s\n", peer.c_str());

    _incommand++;
    try {
	_coord.status(peer, status);
    } catch(const XorpException& e) {
	_incommand--;
	return XrlCmdError::COMMAND_FAILED(e.why() + "\nPending operation: " +
					   (_coord.pending() ?
					   "true" : "false"));
    }
    _incommand--;

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCoordTarget::coord_0_1_pending(bool& pending)
{
    debug_msg("pending:\n");

    if(0 != _incommand)
	pending = true;
    else
	pending = _coord.pending();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCoordTarget::datain_0_1_receive(const string&  peer, const uint32_t& genid,
				   const bool& status, const uint32_t& secs,
				   const uint32_t& micro,
				   const vector<uint8_t>&  data)
{
    debug_msg("peer: %s genid: %u status: %d secs: %d micro: %d data length: %u\n",
	      peer.c_str(), genid, status, secs, micro, (uint32_t)data.size());

    _coord.datain(peer, genid, status, secs, micro, data);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCoordTarget::datain_0_1_error(const string&  peer, const uint32_t& genid,
				 const string& reason)
{
    debug_msg("peer: %s genid: %u reason: %s\n", peer.c_str(), genid,
	      reason.c_str());

    _coord.datain_error(peer, genid, reason);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCoordTarget::datain_0_1_closed(const string&  peer, const uint32_t& genid)
{
    debug_msg("peer: %s\n", peer.c_str());

    _coord.datain_closed(peer, genid);

    return XrlCmdError::OKAY();
}

/*
-------------------- IMPLEMENTATION --------------------
*/
Coord::Coord(EventLoop& eventloop, Command& command)
    : _done(false), _eventloop(eventloop), _command(command)
{
}

void
Coord::command(const string& command)
{
    _command.command(command);
}

void
Coord::status(const string& peer, string& status)
{
    _command.status(peer, status);
}

bool
Coord::pending()
{
    return _command.pending();
}

void
Coord::datain(const string&  peer, const uint32_t& genid, const bool& status,
	      const uint32_t& secs, const uint32_t& micro,
	      const vector<uint8_t>&  data)
{
    TimeVal tv(secs, micro);
    _command.datain(peer, genid, status, tv, data);
}

void
Coord::datain_error(const string& peer, const uint32_t& genid,
		    const string& reason)
{
    _command.datain_error(peer, genid, reason);
}

void
Coord::datain_closed(const string& peer, const uint32_t& genid)
{
    _command.datain_closed(peer, genid);
}

bool
Coord::done()
{
    return _done;
}

void
Coord::mark_done()
{
    _done = true;
}

void
usage(char *name)
{
    fprintf(stderr,"usage: %s [-h (finder host)]\n", name);
    exit(-1);
}

int
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    int c;
    const char *finder_host = "localhost";
    const char *server = SERVER;

    while((c = getopt (argc, argv, "h:")) != EOF) {
	switch(c) {
	case 'h':
	    finder_host = optarg;
	    break;
	case '?':
	    usage(argv[0]);
	}
    }

    try {
	EventLoop eventloop;
	XrlStdRouter router(eventloop, server, finder_host);
	Command com(eventloop, router);
	Coord coord(eventloop, com);
	XrlCoordTarget xrl_target(&router, coord);

	{
	    bool timed_out(false);
	    XorpTimer t = eventloop.set_flag_after_ms(5000, &timed_out);
	    while (router.ready() == false && timed_out == false) {
		eventloop.run();
	    }
	    if (router.ready() == false && timed_out)
		XLOG_FATAL("Xrl router did not become ready - No Finder?");
	}

	while (coord.done() == false) {
	    eventloop.run();
	}

    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();
    debug_msg("Bye!\n");
    return 0;
}


