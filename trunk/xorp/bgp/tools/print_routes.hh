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

// $XORP: xorp/bgp/tools/print_routes.hh,v 1.11 2004/06/09 10:14:05 atanu Exp $

#ifndef __BGP_TOOLS_PRINT_PEER_HH__
#define __BGP_TOOLS_PRINT_PEER_HH__

#include "bgptools_module.h"
#include "config.h"
#include "bgp/aspath.hh"
#include "bgp/path_attribute.hh"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxipc/xrl_std_router.hh"
#include "libxipc/xrl_args.hh"
#include "xrl/interfaces/bgp_xif.hh"

#define MAX_REQUESTS 100

template <typename A>
class PrintRoutes : public XrlBgpV0p2Client {
public:
    enum detail_t {SUMMARY, NORMAL, DETAIL};
    PrintRoutes(detail_t verbose, int interval, bool unicast, bool multicast,
		int lines = -1);
    void get_route_list_start(bool unicast, bool multicast);
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
