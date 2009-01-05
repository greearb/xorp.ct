// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/bgp/tools/print_routes.hh,v 1.25 2008/10/02 21:56:28 bms Exp $

#ifndef __BGP_TOOLS_PRINT_PEER_HH__
#define __BGP_TOOLS_PRINT_PEER_HH__

#include "bgptools_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "libxipc/xrl_std_router.hh"
#include "libxipc/xrl_args.hh"

#include "xrl/interfaces/bgp_xif.hh"

#include "bgp/aspath.hh"
#include "bgp/path_attribute.hh"


template <typename A>
class PrintRoutes : public XrlBgpV0p3Client {
public:
    static const uint32_t MAX_REQUESTS = 100;
    static const int32_t INVALID = -1;
    enum detail_t {SUMMARY, NORMAL, DETAIL};
    PrintRoutes(detail_t verbose, int interval, IPNet<A> net, bool unicast,
		bool multicast,	int lines = -1);
    void get_route_list_start(IPNet<A> net, bool unicast, bool multicast);
    void get_route_list_start_done(const XrlError& e,
				   const uint32_t* token);
    void get_route_list_next();
    void get_route_list_next_done(const XrlError& 	 e,
				  const IPv4* 		 peer_id,
				  const IPNet<A>* 	 net,
				  const uint32_t 	 *best_and_origin,
				  const vector<uint8_t>* aspath,
				  const A* 		 nexthop,
				  const int32_t* 	 med,
				  const int32_t* 	 localpref,
				  const int32_t* 	 atomic_agg,
				  const vector<uint8_t>* aggregator,
				  const int32_t* 	 calc_localpref,
				  const vector<uint8_t>* attr_unknown,
				  const bool* 		 valid,
				  const bool* 		 unicast,
				  const bool* 		 multicast);
private:
    void timer_expired();

    EventLoop 	 	_eventloop;
    XrlStdRouter 	_xrl_rtr;
    detail_t 		_verbose;
    uint32_t 		_token;
    bool 		_done;
    uint32_t 		_count;
    bool 		_prev_no_bgp;
    bool 		_prev_no_routes;

    XorpTimer 		_timer;
    int 		_active_requests;
    bool 		_unicast;
    bool 		_multicast;
    int			_lines;
};

#endif // __BGP_TOOLS_PRINT_PEER_HH__
