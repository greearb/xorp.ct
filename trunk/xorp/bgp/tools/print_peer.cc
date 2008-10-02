// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/bgp/tools/print_peer.cc,v 1.22 2008/07/23 05:09:44 pavlin Exp $"

#include "print_peer.hh"
#include "libxorp/asnum.hh"

PrintPeers::PrintPeers(bool verbose, int interval) 
    : XrlBgpV0p3Client(&_xrl_rtr), 
    _xrl_rtr(_eventloop, "print_peers"), _verbose(verbose)
{
    _prev_no_bgp = false;
    _prev_no_peers = false;

    // Wait for the finder to become ready.
    {
	bool timed_out = false;
	XorpTimer t = _eventloop.set_flag_after_ms(10000, &timed_out);
	while (_xrl_rtr.connected() == false && timed_out == false) {
	    _eventloop.run();
	}

	if (_xrl_rtr.connected() == false) {
	    XLOG_WARNING("XrlRouter did not become ready. No Finder?");
	}
    }

    for (;;) {
	_done = false;
	_token = 0;
	_count = 0;
	get_peer_list_start();
	while (_done == false) {
	    _eventloop.run();
	}
	if (interval <= 0)
	    break;
	TimerList::system_sleep(TimeVal(interval, 0));
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
	//fprintf(stderr, "Failed to get peer list start\n");
	if (_prev_no_bgp == false)
	    printf("\n\nNo BGP Exists\n");
	_prev_no_bgp = true;
	_done = true;
	return;
    }
    _prev_no_bgp = false;
    if (*more == false) {
	if (_prev_no_peers == false)
	    printf("\n\nNo Peerings Exist\n");
	_prev_no_peers = true;
	_done = true;
	return;
    }
    _prev_no_peers = false;

    _token = *token;
    get_peer_list_next();
}

void
PrintPeers::get_peer_list_next()
{
    XorpCallback6<void, const XrlError&, const string*, 
	const uint32_t*, const string*, const uint32_t*, 
	const bool*>::RefPtr cb;
    cb = callback(this, &PrintPeers::get_peer_list_next_done);
    send_get_peer_list_next("bgp", _token, cb);
}

void
PrintPeers::get_peer_list_next_done(const XrlError& e, 
				    const string* local_ip, 
				    const uint32_t* local_port, 
				    const string* peer_ip, 
				    const uint32_t* peer_port, 
				    const bool* more) 
{
    if (e != XrlError::OKAY()) {
	fprintf(stderr, "Failed to get peer list next\n");
	_done = true;
	return;
    }
    if (local_port == 0 && peer_port == 0) {
	//this is an error condition where the last peer was deleted
	//before we could list it.
	_done = true;
	return;
    }
	
    _count++;
    printf("Peer %u: local %s/%u remote %s/%u\n", XORP_UINT_CAST(_count),
	   local_ip->c_str(), XORP_UINT_CAST(*local_port),
	   peer_ip->c_str(), XORP_UINT_CAST(*peer_port));
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
PrintPeers::print_peer_verbose(const string& local_ip, 
			       uint32_t local_port, 
			       const string& peer_ip, 
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

    XorpCallback2<void, const XrlError&, const string*>::RefPtr cb4;
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

    XorpCallback8<void, const XrlError&, const uint32_t*, const uint32_t*, 
	const uint32_t*, const uint32_t*, const uint32_t*, 
	const uint32_t*, const uint32_t*>::RefPtr cb7;
    cb7 = callback(this, &PrintPeers::get_peer_timer_config_done);
    send_get_peer_timer_config("bgp", local_ip, local_port, 
			       peer_ip, peer_port, cb7);
}

void 
PrintPeers::get_peer_id_done(const XrlError& e, 
			     const IPv4* peer_id)
{
    if (e != XrlError::OKAY()) {
	//printf("Failed to retrieve verbose data (%s)\n", e.str().c_str());
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
	//printf("Failed to retrieve verbose data\n");
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
	//printf("Failed to retrieve verbose data\n");
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
			     const string* peer_as)
{
    if (e != XrlError::OKAY()) {
	//printf("Failed to retrieve verbose data\n");
	if (_more)
	    get_peer_list_next();
	return;
    }
    AsNum asn(*peer_as);
    _peer_as = asn.as4();
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
	//printf("Failed to retrieve verbose data\n");
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
	//printf("Failed to retrieve verbose data\n");
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
				       const uint32_t* min_as_origination_interval,
				       const uint32_t* min_route_adv_interval)
{
    if (e != XrlError::OKAY()) {
	printf("Failed to retrieve verbose data\n");
	if (_more)
	    get_peer_list_next();
	else
	    _done = true;
	return;
    }
    _retry_interval = *retry_interval;
    _hold_time = *hold_time;
    _keep_alive = *keep_alive;
    _hold_time_conf = *hold_time_conf;
    _keep_alive_conf = *keep_alive_conf;
    _min_as_origination_interval = *min_as_origination_interval;
    _min_route_adv_interval = *min_route_adv_interval;
    _received++;
    if (_received == 7)
	do_verbose_peer_print();
}

string
PrintPeers::time_units(uint32_t secs) const {
    string s;
    if (secs == 1)
	s = "1 second";
    else
	s = c_format("%u seconds", XORP_UINT_CAST(secs));
    return s;
}

void 
PrintPeers::do_verbose_peer_print()
{
    if (_peer_state == 6) {
	printf("  Peer ID: %s\n", _peer_id.str().c_str());
    } else {
	printf("  Peer ID: none\n");
    }
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

    if (_peer_state == 6)
	printf("  Negotiated BGP Version: %d\n",
	       XORP_INT_CAST(_negotiated_version));
    else
	printf("  Negotiated BGP Version: n/a\n");
    printf("  Peer AS Number: %u\n", XORP_UINT_CAST(_peer_as));
    printf("  Updates Received: %u,  Updates Sent: %u\n",
	   XORP_UINT_CAST(_in_updates), XORP_UINT_CAST(_out_updates));
    printf("  Messages Received: %u,  Messages Sent: %u\n",
	   XORP_UINT_CAST(_in_msgs), XORP_UINT_CAST(_out_msgs));
    if (_in_updates > 0) {
	printf("  Time since last received update: %s\n",
	       time_units(_in_update_elapsed).c_str());
    } else {
	printf("  Time since last received update: n/a\n");
    }
    printf("  Number of transitions to ESTABLISHED: %u\n",
	   XORP_UINT_CAST(_transitions));
    if (_peer_state == 6) {
	printf("  Time since last entering ESTABLISHED state: %s\n",
	       time_units(_established_time).c_str());
    } else if (_transitions > 0) {
	printf("  Time since last in ESTABLISHED state: %s\n",
	       time_units(_established_time).c_str());
    } else {
	printf("  Time since last in ESTABLISHED state: n/a\n");
    }
    printf("  Retry Interval: %s\n", time_units(_retry_interval).c_str());
    if (_peer_state == 6) {
	printf("  Hold Time: %s,  Keep Alive Time: %s\n",
	       time_units(_hold_time).c_str(), 
	       time_units(_keep_alive).c_str());
    } else {
	printf("  Hold Time: n/a,  Keep Alive Time: n/a\n");
    }
    printf("  Configured Hold Time: %s,  Configured Keep Alive Time: %s\n", 
	   time_units(_hold_time_conf).c_str(), 
	   time_units(_keep_alive_conf).c_str());
    printf("  Minimum AS Origination Interval: %s\n", 
	   time_units(_min_as_origination_interval).c_str());
    printf("  Minimum Route Advertisement Interval: %s\n", 
	   time_units(_min_route_adv_interval).c_str());
    if (_more) {
	printf("\n");
	get_peer_list_next();
    } else {
	_done = true;
    }
}
