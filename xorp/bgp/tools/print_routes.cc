// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2010 XORP, Inc and Others
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



#include "print_routes.hh"

// ----------------------------------------------------------------------------
// Specialized PrintRoutes implementation

template <>
void
PrintRoutes<IPv4>::get_route_list_start(IPNet<IPv4> net, bool unicast,
					bool multicast)
{
    _active_requests = 0;
    send_get_v4_route_list_start("bgp", net, unicast, multicast,
		 callback(this, &PrintRoutes::get_route_list_start_done));
}

template <>
void
PrintRoutes<IPv4>::get_route_list_next()
{
    send_get_v4_route_list_next("bgp",	_token,
		callback(this, &PrintRoutes::get_route_list_next_done));
}

// ----------------------------------------------------------------------------
// Common PrintRoutes implementation

template <typename A>
PrintRoutes<A>::PrintRoutes(detail_t verbose, int interval, IPNet<A> net,
			    bool unicast, bool multicast, int lines)
    : XrlBgpV0p3Client(&_xrl_rtr),
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
	get_route_list_start(net, _unicast, _multicast);
	while (_done == false || _active_requests > 0) {
	    _eventloop.run();
	    if (_lines == static_cast<int>(_count)) {
		printf("Output truncated at %u lines\n",
		       XORP_UINT_CAST(_count));
		break;
	    }
	}
	// Infinite loop by design.
	// The command will be repeated every interval seconds until
	// interrupted by the user.
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
	    printf("\n\nBGP is not running or may not be configured\n");
	_prev_no_bgp = true;
	_done = true;
	return;
    }
    _prev_no_bgp = false;

    switch(_verbose) {
    case SUMMARY:
    case NORMAL:
	printf("Status Codes: * valid route, > best route\n");
	printf("Origin Codes: i IGP, e EGP, ? incomplete\n\n");
	printf("   Prefix                Nexthop                    "
	       "Peer            AS Path\n");
	printf("   ------                -------                    "
	       "----            -------\n");
	break;
    case DETAIL:
	break;
    }

    _token = *token;
    for (uint32_t i = 0; i < MAX_REQUESTS; i++) {
	_active_requests++;
	get_route_list_next();
    }
}

// See RFC 1657 (BGP MIB) for full definitions of return values.

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
    UNUSED(calc_localpref);
    UNUSED(attr_unknown);
    UNUSED(aspath);

    if (e != XrlError::OKAY() || false == *valid) {
	_active_requests--;
	_done = true;
	return;
    }
    _count++;

    uint8_t best = (*best_and_origin)>>16;
    uint8_t origin = (*best_and_origin)&255;

    ASPath asp((const uint8_t*)(&((*aspath)[0])), aspath->size());

    switch(_verbose) {
    case SUMMARY:
    case NORMAL:
	//XXX this should be used to indicate a route is valid
	printf("*");

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

	printf(" %-20s  %-25s  %-12s  %s ", net->str().c_str(),
	       nexthop->str().c_str(),
	       peer_id->str().c_str(),
	       asp.short_str().c_str());

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
	break;
    case DETAIL:
	printf("%s\n", cstring(*net));
	printf("\tFrom peer: %s\n", cstring(*peer_id));
	printf("\tRoute: ");
	switch (best) {
	case 1:
	    printf("Not Used\n");
	    break;
	case 2:
	    printf("Winner\n");
	    break;
	default:
	    printf("UNKNOWN\n");
	}

	printf("\tOrigin: ");
	switch (origin) {
	case IGP:
	    printf("IGP\n");
	    break;
	case EGP:
	    printf("EGP\n");
	    break;
	case INCOMPLETE:
	    printf("INCOMPLETE\n");
	    break;
	default:
	    printf("BAD ORIGIN\n");
	    break;
	}

	printf("\tAS Path: %s\n", asp.short_str().c_str());
	printf("\tNexthop: %s\n", cstring(*nexthop));
	if (INVALID != *med)
	    printf("\tMultiple Exit Discriminator: %d\n", *med);
	if (INVALID != *localpref)
	    printf("\tLocal Preference: %d\n", *localpref);
	if (2 == *atomic_agg)
	    printf("\tAtomic Aggregate: Less Specific Route Selected\n");
#if	0
	printf("\tAtomic Aggregate: ");
	switch (*atomic_agg) {
	case 1:
	    printf("Less Specific Route Not Selected\n");
	    break;
	case 2:
	    printf("Less Specific Route Selected\n");
	    break;
	default:
	    printf("UNKNOWN\n");
	    break;
	}
#endif
	if (!aggregator->empty()) {
	    XLOG_ASSERT(6 == aggregator->size());
	    A agg(&((*aggregator)[0]));
	    AsNum asnum(&((*aggregator)[4]));
	    
	    printf("\tAggregator: %s %s\n", cstring(agg), cstring(asnum));
	}
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

#ifdef HAVE_IPV6

template <>
void
PrintRoutes<IPv6>::get_route_list_start(IPNet<IPv6> net, bool unicast,
					bool multicast)
{
    _active_requests = 0;
    send_get_v6_route_list_start("bgp", net, unicast, multicast,
		 callback(this, &PrintRoutes::get_route_list_start_done));
}

template <>
void
PrintRoutes<IPv6>::get_route_list_next()
{
    send_get_v6_route_list_next("bgp", _token,
		callback(this, &PrintRoutes::get_route_list_next_done));
}

template class PrintRoutes<IPv6>;

#endif // ipv6
