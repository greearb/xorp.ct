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

// $XORP: xorp/bgp/tools/print_routes.hh,v 1.5 2003/03/10 23:20:10 hodson Exp $

#ifndef __BGP_TOOLS_PRINT_PEER_HH__
#define __BGP_TOOLS_PRINT_PEER_HH__

#include "bgptools_module.h"
#include "config.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxipc/xrl_std_router.hh"
#include "libxipc/xrl_args.hh"
#include "xrl/interfaces/bgp_xif.hh"


class PrintRoutes : public XrlBgpV0p2Client {
public:
    enum detail_t {SUMMARY, NORMAL, DETAIL};
    PrintRoutes(detail_t verbose, int interval);
    void get_v4_route_list_start();
    void get_v4_route_list_start_done(const XrlError& e, 
				   const uint32_t* token);
    void get_v4_route_list_next();
    void get_v4_route_list_next_done(const XrlError& e, 
				     const IPv4* peer_id, 
				     const IPv4Net* net, 
				     const uint32_t *best_and_origin, 
				     const vector<uint8_t>* aspath, 
				     const IPv4* nexthop, 
				     const int32_t* med, 
				     const int32_t* localpref, 
				     const int32_t* atomic_agg, 
				     const vector<uint8_t>* aggregator, 
				     const int32_t* calc_localpref, 
				     const vector<uint8_t>* attr_unknown,
				     const bool* valid);
private:
    void timer_expired() { _done = true;}
    EventLoop _eventloop;
    XrlStdRouter _xrl_rtr;
    detail_t _verbose;
    uint32_t _token;
    bool _done;
    uint32_t _count;
    bool _prev_no_bgp;
    bool _prev_no_routes;

    XorpTimer _timer;
    int _active_requests;
};

#endif // __BGP_TOOLS_PRINT_PEER_HH__
