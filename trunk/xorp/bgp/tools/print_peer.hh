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

// $XORP: xorp/bgp/tools/print_peer.hh,v 1.2 2003/01/17 19:48:00 pavlin Exp $

#ifndef __BGP_TOOLS_PRINT_PEER_HH__
#define __BGP_TOOLS_PRINT_PEER_HH__

#include "bgptools_module.h"
#include "config.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxipc/xrl_std_router.hh"
#include "libxipc/xrl_args.hh"
#include "xrl/interfaces/bgp_xif.hh"

class PrintPeers : public XrlBgpV0p2Client {
public:
    PrintPeers(bool verbose);
    void get_peer_list_start();
    void get_peer_list_start_done(const XrlError& e, 
				  const uint32_t* token, 
				  const bool* more);
    void get_peer_list_next();
    void get_peer_list_next_done(const XrlError& e, 
				 const IPv4* local_ip, 
				 const uint32_t* local_port, 
				 const IPv4* peer_ip, 
				 const uint32_t* peer_port, 
				 const bool* more);
private:
    EventLoop _eventloop;
    XrlStdRouter _xrl_rtr;
    bool _verbose;
    uint32_t _token;
    bool _done;
};

#endif // __BGP_TOOLS_PRINT_PEER_HH__
