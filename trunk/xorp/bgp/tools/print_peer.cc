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

#ident "$XORP: xorp/bgp/tools/print_peer.cc,v 1.2 2003/01/18 04:02:29 mjh Exp $"

#include "print_peer.hh"

PrintPeers::PrintPeers(bool verbose) 
    : XrlBgpV0p2Client(&_xrl_rtr), 
    _eventloop(), _xrl_rtr(_eventloop, "print_peers"), _verbose(verbose),
    _token(0), _done(false), _count(0)
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
    _count++;
    printf("Peer %d: local %s/%d remote %s/%d\n", _count,
	   local_ip->str().c_str(), *local_port,
	   peer_ip->str().c_str(), *peer_port);
    if (_verbose) {
	_more = *more;
	print_peer_verbose(*local_ip, *local_port, 
			   *peer_ip, *peer_port);
	return;
    } else {
	if (*more == false) {
	    _done = true;
	    return;
	}
	get_peer_list_next();
    }
}

void 
PrintPeers::print_peer_verbose(const IPv4& local_ip, 
			       uint32_t local_port, 
			       const IPv4& peer_ip, 
			       uint32_t peer_port) 
{
    //node: ports are still in network byte order

    //pipeline all the requests so we get as close to possible to an
    //instantaneous snapshot of the peer config
    _received = 0;
    XorpCallback2<void, const XrlError&, const IPv4*>::RefPtr cb1;
    cb1 = callback(this, &PrintPeers::get_peer_id_done);
    send_get_peer_id("bgp", local_ip, local_port, peer_ip, peer_port, cb1);

    XorpCallback3<void, const XrlError&, const uint32_t*, 
	const uint32_t*>::RefPtr cb2;
    cb2 = callback(this, &PrintPeers::get_peer_status_done);
    send_get_peer_status("bgp", local_ip, local_port, peer_ip, peer_port, cb2);

    XorpCallback2<void, const XrlError&, const int32_t*>::RefPtr cb3;
    cb3 = callback(this, &PrintPeers::get_peer_negotiated_version_done);
    send_get_peer_negotiated_version("bgp", local_ip, local_port, peer_ip, 
				     peer_port, cb3);

    XorpCallback2<void, const XrlError&, const uint32_t*>::RefPtr cb4;
    cb4 = callback(this, &PrintPeers::get_peer_as_done);
    send_get_peer_as("bgp", local_ip, local_port, peer_ip, peer_port, cb4);

    XorpCallback7<void, const XrlError&, const uint32_t*, const uint32_t*, 
	const uint32_t*, const uint32_t*, const uint32_t*, 
	const uint32_t*>::RefPtr cb5;
    cb5 = callback(this, &PrintPeers::get_peer_msg_stats_done);
    send_get_peer_msg_stats("bgp", local_ip, local_port, 
			    peer_ip, peer_port, cb5);

    XorpCallback3<void, const XrlError&, const uint32_t*, 
	const uint32_t*>::RefPtr cb6;
    cb6 = callback(this, &PrintPeers::get_peer_established_stats_done);
    send_get_peer_established_stats("bgp", local_ip, local_port, 
				    peer_ip, peer_port, cb6);

    XorpCallback7<void, const XrlError&, const uint32_t*, const uint32_t*, 
	const uint32_t*, const uint32_t*, const uint32_t*, 
	const uint32_t*>::RefPtr cb7;
    cb7 = callback(this, &PrintPeers::get_peer_timer_config_done);
    send_get_peer_timer_config("bgp", local_ip, local_port, 
			       peer_ip, peer_port, cb7);
}

void 
PrintPeers::get_peer_id_done(const XrlError& e, 
			     const IPv4* peer_id)
{
    if (e != XrlError::OKAY()) {
	printf("Failed to retrieve verbose data (%s)\n", e.str().c_str());
	if (_more)
	    get_peer_list_next();
	return;
    }
    _peer_id = *peer_id;
    _received++;
    if (_received == 7)
	do_verbose_peer_print();
}

void 
PrintPeers::get_peer_status_done(const XrlError& e, 
				 const uint32_t* peer_state, 
				 const uint32_t* admin_status)
{
    if (e != XrlError::OKAY()) {
	printf("Failed to retrieve verbose data\n");
	if (_more)
	    get_peer_list_next();
	return;
    }
    _peer_state = *peer_state;
    _admin_state = *admin_status;
    _received++;
    if (_received == 7)
	do_verbose_peer_print();
}

void 
PrintPeers::get_peer_negotiated_version_done(const XrlError& e, 
					     const int32_t* neg_version)
{
    if (e != XrlError::OKAY()) {
	printf("Failed to retrieve verbose data\n");
	if (_more)
	    get_peer_list_next();
	return;
    }
    _negotiated_version = *neg_version;
    _received++;
    if (_received == 7)
	do_verbose_peer_print();
}

void 
PrintPeers::get_peer_as_done(const XrlError& e, 
			     const uint32_t* peer_as)
{
    if (e != XrlError::OKAY()) {
	printf("Failed to retrieve verbose data\n");
	if (_more)
	    get_peer_list_next();
	return;
    }
    _peer_as = *peer_as;
    _received++;
    if (_received == 7)
	do_verbose_peer_print();
}

void 
PrintPeers::get_peer_msg_stats_done(const XrlError& e, 
				    const uint32_t* in_updates, 
				    const uint32_t* out_updates, 
				    const uint32_t* in_msgs, 
				    const uint32_t* out_msgs, 
				    const uint32_t* last_error, 
				    const uint32_t* in_update_elapsed)
{
    if (e != XrlError::OKAY()) {
	printf("Failed to retrieve verbose data\n");
	if (_more)
	    get_peer_list_next();
	return;
    }
    _in_updates = *in_updates;
    _out_updates = *out_updates;
    _in_msgs = *in_msgs;
    _out_msgs = *out_msgs;
    _last_error = *last_error;
    _in_update_elapsed = *in_update_elapsed;
    _received++;
    if (_received == 7)
	do_verbose_peer_print();
}

void 
PrintPeers::get_peer_established_stats_done(const XrlError& e, 
					    const uint32_t* transitions, 
					    const uint32_t* established_time)
{
    if (e != XrlError::OKAY()) {
	printf("Failed to retrieve verbose data\n");
	if (_more)
	    get_peer_list_next();
	return;
    }
    _transitions = *transitions;
    _established_time = *established_time;
    _received++;
    if (_received == 7)
	do_verbose_peer_print();
}

void
PrintPeers::get_peer_timer_config_done(const XrlError& e, 
				       const uint32_t* retry_interval, 
				       const uint32_t* hold_time, 
				       const uint32_t* keep_alive, 
				       const uint32_t* hold_time_conf, 
				       const uint32_t* keep_alive_conf, 
				       const uint32_t* min_as_origination_interval)
{
    if (e != XrlError::OKAY()) {
	printf("Failed to retrieve verbose data\n");
	if (_more)
	    get_peer_list_next();
	return;
    }
    _retry_interval = *retry_interval;
    _hold_time = *hold_time;
    _keep_alive = *keep_alive;
    _hold_time_conf = *hold_time_conf;
    _keep_alive_conf = *keep_alive_conf;
    _min_as_origination_interval = *min_as_origination_interval;
    _received++;
    if (_received == 7)
	do_verbose_peer_print();
}

void 
PrintPeers::do_verbose_peer_print()
{
    printf("  Peer ID: %s\n", _peer_id.str().c_str());
    printf("  Peer State: ");
    switch (_peer_state) {
    case 1:
	printf("IDLE\n");
	break;
    case 2:
	printf("CONNECT\n");
	break;
    case 3:
	printf("ACTIVE\n");
	break;
    case 4:
	printf("OPENSENT\n");
	break;
    case 5:
	printf("OPENCONFIRM\n");
	break;
    case 6:
	printf("ESTABLISHED\n");
	break;
    case 7:
	printf("IDLE(*)\n");
	break;
    default:
	printf("UNKNOWN (THIS SHOULDN'T HAPPEN)\n");
    }

    printf("  Admin State: ");
    switch (_admin_state) {
    case 1:
	printf("STOPPED\n");
	break;
    case 2:
	printf("START\n");
	break;
    default:
	printf("UNKNOWN (THIS SHOULDN'T HAPPEN)\n");
    }

    printf("  Negotiated BGP Version: %d\n", _negotiated_version);
    printf("  Peer AS Number: %d\n", _peer_as);
    printf("  Updates Received: %d,  Updates Sent: %d\n",
	   _in_updates, _out_updates);
    printf("  Messages Received: %d,  Messages Sent: %d\n",
	   _in_msgs, _out_msgs);
    printf("  Time since last received update: %d seconds\n",
	   _in_update_elapsed);
    printf("  Number of transitions to ESTABLISHED: %d\n",
	   _transitions);
    printf("  Time since last transition to ESTABLISHED: %d seconds\n",
	   _established_time);
    printf("  Retry Interval: %d secs\n  Hold Time: %d secs,  Keep Alive Time: %d secs\n",
	   _retry_interval, _hold_time, _keep_alive);
    printf("  Configured Hold Time: %d secs,  Configured Keep Alive Time: %d secs\n", _hold_time_conf, _keep_alive_conf);
    printf("  Minimum AS Origination Interval: %d secs\n", 
	   _min_as_origination_interval);
    if (_more) {
	printf("\n");
	get_peer_list_next();
    } else {
	_done = true;
    }
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

