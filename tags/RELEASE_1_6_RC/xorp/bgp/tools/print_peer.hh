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

// $XORP: xorp/bgp/tools/print_peer.hh,v 1.18 2008/07/23 05:09:44 pavlin Exp $

#ifndef __BGP_TOOLS_PRINT_PEER_HH__
#define __BGP_TOOLS_PRINT_PEER_HH__

#include "bgptools_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "libxipc/xrl_std_router.hh"
#include "libxipc/xrl_args.hh"

#include "xrl/interfaces/bgp_xif.hh"


class PrintPeers : public XrlBgpV0p3Client {
public:
    PrintPeers(bool verbose, int interval);
    void get_peer_list_start();
    void get_peer_list_start_done(const XrlError& e, 
				  const uint32_t* token, 
				  const bool* more);
    void get_peer_list_next();
    void get_peer_list_next_done(const XrlError& e, 
				 const string* local_ip, 
				 const uint32_t* local_port, 
				 const string* peer_ip, 
				 const uint32_t* peer_port, 
				 const bool* more);
    void print_peer_verbose(const string& local_ip, 
			    uint32_t local_port, 
			    const string& peer_ip, 
			    uint32_t peer_port);
    void get_peer_id_done(const XrlError& e, 
			  const IPv4* peer_id);
    void get_peer_status_done(const XrlError& e, 
			      const uint32_t* peer_state, 
			      const uint32_t* admin_status);
    void get_peer_negotiated_version_done(const XrlError&, 
					  const int32_t* neg_version);
    void get_peer_as_done(const XrlError& e, 
			  const string* peer_as);
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
				    const uint32_t* min_as_origination_interval,
				    const uint32_t* min_route_adv_interval);
    void do_verbose_peer_print();
    string time_units(uint32_t s) const;
    
private:
    EventLoop _eventloop;
    XrlStdRouter _xrl_rtr;
    bool _verbose;
    uint32_t _token;
    bool _done;
    uint32_t _count;
    bool _prev_no_bgp;
    bool _prev_no_peers;

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
    uint32_t _min_route_adv_interval;
};

#endif // __BGP_TOOLS_PRINT_PEER_HH__
