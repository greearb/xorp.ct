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

#ident "$XORP: xorp/bgp/tools/print_peer.cc,v 1.1 2003/01/17 18:01:02 mjh Exp $"

#include "print_peer.hh"

PrintPeers::PrintPeers(bool verbose) 
    : XrlBgpV0p2Client(&_xrl_rtr), 
    _eventloop(), _xrl_rtr(_eventloop, "print_peers"), _verbose(verbose),
    _token(0), _done(false)
{
    get_peer_list_start();
    while (_done == false) {
	_eventloop.run();
    }
}

void
PrintPeers::get_peer_list_start() {
    XorpCallback3<void, const XrlError&, const uint32_t*, 
	const bool*>::RefPtr cb;
    cb = callback(this, &PrintPeers::get_peer_list_start_done);
    send_get_peer_list_start("bgp", cb);
}

void
PrintPeers::get_peer_list_start_done(const XrlError& e, 
				     const uint32_t* token, 
				     const bool* more) 
{
    if (e != XrlError::OKAY()) {
	fprintf(stderr, "Failed to get peer list start\n");
	_done = true;
	return;
    }
    if (*more == false) {
	_done = true;
	return;
    }

    _token = *token;
    get_peer_list_next();
}

void
PrintPeers::get_peer_list_next() {
    XorpCallback6<void, const XrlError&, const IPv4*, 
	const uint32_t*, const IPv4*, const uint32_t*, 
	const bool*>::RefPtr cb;
    cb = callback(this, &PrintPeers::get_peer_list_next_done);
    send_get_peer_list_next("bgp", _token, cb);
}

void
PrintPeers::get_peer_list_next_done(const XrlError& e, 
				    const IPv4* local_ip, 
				    const uint32_t* local_port, 
				    const IPv4* peer_ip, 
				    const uint32_t* peer_port, 
				    const bool* more) 
{
    if (e != XrlError::OKAY()) {
	fprintf(stderr, "Failed to get peer list next\n");
	_done = true;
	return;
    }
    printf("Peer: local %s/%d remote %s/%d\n",
	   local_ip->str().c_str(), htons(*local_port),
	   peer_ip->str().c_str(), htons(*peer_port));
    if (*more == false) {
	_done = true;
	return;
    }
    get_peer_list_next();
}

void usage()
{
    fprintf(stderr,
    "Usage: print_peer [-v]\n"
    "where -v enables verbose output.\n");
}


int main(int argc, char **argv)
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

    bool verbose = false;
    char c;
    while ((c = getopt(argc, argv, "v")) != -1) {
	switch (c) {
	case 'v':
	    verbose = true;
	    break;
	default:
	    usage();
	    return -1;
	}
    }
    try {
	PrintPeers peer_printer(verbose);
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}

