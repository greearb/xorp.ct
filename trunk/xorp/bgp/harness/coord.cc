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

#ident "$XORP: xorp/bgp/harness/coord.cc,v 1.4 2003/03/10 23:20:09 hodson Exp $"

#include "config.h"
#include "bgp/bgp_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libxorp/eventloop.hh"
#include "libxipc/xrl_std_router.hh"
#include "bgp/socket.hh"

#include "coord.hh"
#include "command.hh"

static const char SERVER[] = "coord";/* This servers name */
static const char VERSION[] = "0.1";

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

    version = VERSION;
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
XrlCoordTarget::datain_0_1_receive(const string&  peer, const bool& status, 
				   const uint32_t& secs,const uint32_t& micro,
				   const vector<uint8_t>&  data)
{
    debug_msg("peer: %s status: %d secs: %d micro: %d data length: %u\n",
	      peer.c_str(), status, secs, micro, (uint32_t)data.size());

    _coord.datain(peer, status, secs, micro, data);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCoordTarget::datain_0_1_error(const string&  peer, const string& reason)
{
    debug_msg("peer: %s reason: %s\n", peer.c_str(), reason.c_str());

    _coord.datain_error(peer, reason);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlCoordTarget::datain_0_1_closed(const string&  peer)
{
    debug_msg("peer: %s\n", peer.c_str());

    _coord.datain_closed(peer);

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

bool
Coord::pending() 
{
    return _command.pending();
}

void
Coord::datain(const string&  peer, const bool& status, const uint32_t& secs,
	      const uint32_t& micro, const vector<uint8_t>&  data)
{
    TimeVal tv(secs, micro);
    _command.datain(peer, status, tv, data);
}

void
Coord::datain_error(const string& peer, const string& reason)
{
    _command.datain_error(peer, reason);
}

void
Coord::datain_closed(const string& peer)
{
    _command.datain_closed(peer);
}

bool 
Coord::done() 
{
    return _done;
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

	while(!coord.done()) {
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


