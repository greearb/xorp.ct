// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/tools/print_routes.cc,v 1.12 2004/06/09 10:14:05 atanu Exp $"

#include "print_routes.hh"

// ----------------------------------------------------------------------------
// Specialized PrintRoutes implementation

template <>
void
PrintRoutes<IPv4>::get_route_list_start(bool unicast, bool multicast)
{
    _active_requests = 0;
    send_get_v4_route_list_start("bgp", unicast, multicast,
		 callback(this, &PrintRoutes::get_route_list_start_done));
}

template <>
void
PrintRoutes<IPv6>::get_route_list_start(bool unicast, bool multicast)
{
    _active_requests = 0;
    send_get_v6_route_list_start("bgp", unicast, multicast,
		 callback(this, &PrintRoutes::get_route_list_start_done));
}

template <>
void
PrintRoutes<IPv4>::get_route_list_next()
{
    send_get_v4_route_list_next("bgp",	_token,
		callback(this, &PrintRoutes::get_route_list_next_done));
}

template <>
void
PrintRoutes<IPv6>::get_route_list_next()
{
    send_get_v6_route_list_next("bgp", _token,
		callback(this, &PrintRoutes::get_route_list_next_done));
}

// ----------------------------------------------------------------------------
// Common PrintRoutes implementation

template <typename A>
PrintRoutes<A>::PrintRoutes(detail_t verbose, int interval, bool unicast,
			    bool multicast, int lines)
    : XrlBgpV0p2Client(&_xrl_rtr),
      _xrl_rtr(_eventloop, "print_routes"), _verbose(verbose),
      _unicast(unicast), _multicast(multicast), _lines(lines)
{
    _prev_no_bgp = false;
    _prev_no_routes = false;

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
	get_route_list_start(_unicast, _multicast);
	while (_done == false || _active_requests > 0) {
	    _eventloop.run();
	    if (_lines == static_cast<int>(_count)) {
		printf("Output truncated at %d lines\n", _count);
		break;
	    }
	}
	if (interval <= 0)
	    break;

	//delay before next call
	XorpCallback0<void>::RefPtr cb
	    = callback(this, &PrintRoutes::timer_expired);
	_done = false;
	_timer = _eventloop.new_oneoff_after_ms(interval*1000, cb);
	while (_done == false) {
	    _eventloop.run();
	}
    }
}

template <typename A>
void
PrintRoutes<A>::get_route_list_start_done(const XrlError& e,
					  const uint32_t* token)
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
    printf("Status Codes: * valid route, > best route\n");
    printf("Origin Codes: i IGP, e EGP, ? incomplete\n\n");
    printf(
"   Prefix                Nexthop                    Peer            AS Path\n"
);
    printf(
"   ------                -------                    ----            -------\n"
);
    _token = *token;
    for (int i = 0; i < MAX_REQUESTS; i++) {
	_active_requests++;
	get_route_list_next();
    }
}

template <typename A>
void
PrintRoutes<A>::get_route_list_next_done(const XrlError& e,
					 const IPv4* peer_id,
					 const IPNet<A>* net,
					 const uint32_t *best_and_origin,
					 const vector<uint8_t>* aspath,
					 const A* nexthop,
					 const int32_t* med,
					 const int32_t* localpref,
					 const int32_t* atomic_agg,
					 const vector<uint8_t>* aggregator,
					 const int32_t* calc_localpref,
					 const vector<uint8_t>* attr_unknown,
					 const bool* valid,
					 const bool* /*unicast*/,
					 const bool* /*multicast*/)
{
    UNUSED(med);
    UNUSED(localpref);
    UNUSED(atomic_agg);
    UNUSED(aggregator);
    UNUSED(calc_localpref);
    UNUSED(attr_unknown);
    UNUSED(aspath);

    if (e != XrlError::OKAY() || false == *valid) {
	_active_requests--;
	_done = true;
	return;
    }
    _count++;

    //XXX this should be used to indicate a route is valid
    printf("*");

    uint8_t best = (*best_and_origin)>>16;
    switch (best) {
    case 1:
	printf(" ");
	break;
    case 2:
	printf(">");
	break;
    default:
	printf("?");
    }

    AsPath asp((const uint8_t*)(&((*aspath)[0])), aspath->size());

    printf(" %-20s  %-25s  %-12s  %s ", net->str().c_str(),
	   nexthop->str().c_str(),
	   peer_id->str().c_str(),
	   asp.short_str().c_str());
    uint8_t origin = (*best_and_origin)&255;
    switch (origin) {
    case IGP:
	printf("i\n");
	break;
    case EGP:
	printf("e\n");
	break;
    case INCOMPLETE:
	printf("?\n");
	break;
    default:
	printf ("BAD ORIGIN\n");
	break;
    }

    get_route_list_next();
}

template <typename A>
void
PrintRoutes<A>::timer_expired()
{
    _done = true;
}


// ----------------------------------------------------------------------------
// Template Instantiations

template class PrintRoutes<IPv4>;
template class PrintRoutes<IPv6>;
