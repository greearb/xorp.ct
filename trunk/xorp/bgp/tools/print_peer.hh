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

// $XORP: xorp/bgp/tools/print_peer.hh,v 1.3 2003/01/18 04:02:29 mjh Exp $

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
    void print_peer_verbose(const IPv4& local_ip, 
			    uint32_t local_port, 
			    const IPv4& peer_ip, 
			    uint32_t peer_port);
    void get_peer_id_done(const XrlError& e, 
			  const IPv4* peer_id);
    void get_peer_status_done(const XrlError& e, 
			      const uint32_t* peer_state, 
			      const uint32_t* admin_status);
    void get_peer_negotiated_version_done(const XrlError&, 
					  const int32_t* neg_version);
    void get_peer_as_done(const XrlError& e, 
			  const uint32_t* peer_as);
    void get_peer_msg_stats_done(const XrlError& e, 
				 const uint32_t* in_updates, 
				 const uint32_t* out_updates, 
				 const uint32_t* in_msgs, 
				 const uint32_t* out_msgs, 
				 const uint32_t* last_error, 
				 const uint32_t* in_update_elapsed);
    void get_peer_established_stats_done(const XrlError&, 
					 const uint32_t* transitions, 
					 const uint32_t* established_time);
    void get_peer_timer_config_done(const XrlError& e, 
				    const uint32_t* retry_interval, 
				    const uint32_t* hold_time, 
				    const uint32_t* keep_alive, 
				    const uint32_t* hold_time_conf, 
				    const uint32_t* keep_alive_conf, 
				    const uint32_t* min_as_origination_interval);
    void do_verbose_peer_print();
    
private:
    EventLoop _eventloop;
    XrlStdRouter _xrl_rtr;
    bool _verbose;
    uint32_t _token;
    bool _done;
    uint32_t _count;

    //temporary storage while we retrieve peer data.
    bool _more; //after this one, are there more peers to retrieve
    uint32_t _received;
    IPv4 _peer_id;
    uint32_t _peer_state;
    uint32_t _admin_state;
    int32_t _negotiated_version;
    uint32_t _peer_as;
    uint32_t _in_updates;
    uint32_t _out_updates;
    uint32_t _in_msgs;
    uint32_t _out_msgs;
    uint32_t _last_error;
    uint32_t _in_update_elapsed;
    uint32_t _transitions;
    uint32_t _established_time;
    uint32_t _retry_interval;
    uint32_t _hold_time;
    uint32_t _keep_alive;
    uint32_t _hold_time_conf;
    uint32_t _keep_alive_conf;
    uint32_t _min_as_origination_interval;
};

#endif // __BGP_TOOLS_PRINT_PEER_HH__
