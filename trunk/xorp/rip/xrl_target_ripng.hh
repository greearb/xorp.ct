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

// $XORP: xorp/rip/xrl_target_ripng.hh,v 1.29 2008/07/23 05:11:38 pavlin Exp $

#ifndef __RIP_XRL_TARGET_RIPNG_HH__
#define __RIP_XRL_TARGET_RIPNG_HH__

#include "libxorp/status_codes.h"
#include "xrl/targets/ripng_base.hh"

class XrlRouter;
class XrlProcessSpy;

template <typename A> class System;
template <typename A> class XrlPortManager;
template <typename A> class XrlRipCommonTarget;
template <typename A> class XrlRedistManager;

class XrlRipngTarget : public XrlRipngTargetBase {
public:
    XrlRipngTarget(EventLoop&			e,
		   XrlRouter& 			xr,
		   XrlProcessSpy& 		xps,
		   XrlPortManager<IPv6>&	xpm,
		   XrlRedistManager<IPv6>&	xrm,
		   bool& 			should_exit,
		   System<IPv6>&		rip_system);
    ~XrlRipngTarget();

    void set_status(ProcessStatus ps, const string& annotation = "");

    XrlCmdError common_0_1_get_target_name(string& name);
    XrlCmdError common_0_1_get_version(string& version);
    XrlCmdError common_0_1_get_status(uint32_t& status, string& reason);
    XrlCmdError common_0_1_shutdown();

    XrlCmdError
    finder_event_observer_0_1_xrl_target_birth(const string& class_name,
					       const string& instance_name);

    XrlCmdError
    finder_event_observer_0_1_xrl_target_death(const string& class_name,
					       const string& instance_name);

    XrlCmdError
    ripng_0_1_add_rip_address(const string&	ifname,
			      const string&	vifname,
			      const IPv6&	addr);

    XrlCmdError
    ripng_0_1_remove_rip_address(const string&	ifname,
				 const string&	vifname,
				 const IPv6&	addr);

    XrlCmdError
    ripng_0_1_set_rip_address_enabled(const string&	ifname,
				      const string&	vifname,
				      const IPv6&	addr,
				      const bool&	enabled);

    XrlCmdError
    ripng_0_1_rip_address_enabled(const string&	ifname,
				  const string&	vifname,
				  const IPv6&	addr,
				  bool&		enabled);

    XrlCmdError ripng_0_1_set_cost(const string&	ifname,
				   const string&	vifname,
				   const IPv6&		addr,
				   const uint32_t&	cost);

    XrlCmdError ripng_0_1_cost(const string&	ifname,
			       const string&	vifname,
			       const IPv6&	addr,
			       uint32_t&	cost);

    XrlCmdError ripng_0_1_set_horizon(const string&	ifname,
				      const string&	vifname,
				      const IPv6&	addr,
				      const string&	horizon);

    XrlCmdError ripng_0_1_horizon(const string&	ifname,
				  const string&	vifname,
				  const IPv6&	addr,
				  string&	horizon);

    XrlCmdError ripng_0_1_set_passive(const string&	ifname,
				      const string&	vifname,
				      const IPv6&	addr,
				      const bool&	passive);

    XrlCmdError ripng_0_1_passive(const string&	ifname,
				  const string&	vifname,
				  const IPv6&	addr,
				  bool&		passive);

    XrlCmdError
    ripng_0_1_set_accept_non_rip_requests(const string&	ifname,
					  const string&	vifname,
					  const IPv6&	addr,
					  const bool&	accept);

    XrlCmdError ripng_0_1_accept_non_rip_requests(const string&	ifname,
						  const string&	vifname,
						  const IPv6&	addr,
						  bool&		accept);

    XrlCmdError ripng_0_1_set_accept_default_route(const string& ifname,
						   const string& vifname,
						   const IPv6&	 addr,
						   const bool&	 accept);

    XrlCmdError ripng_0_1_accept_default_route(const string&	ifname,
					       const string&	vifname,
					       const IPv6&	addr,
					       bool&		accept);

    XrlCmdError
    ripng_0_1_set_advertise_default_route(const string&	ifname,
					  const string&	vifname,
					  const IPv6&	addr,
					  const bool&	advertise);

    XrlCmdError ripng_0_1_advertise_default_route(const string&	ifname,
						  const string&	vifname,
						  const IPv6&	addr,
						  bool&		advertise);

    XrlCmdError
    ripng_0_1_set_route_timeout(const string&	ifname,
				const string&	vifname,
				const IPv6&	addr,
				const uint32_t&	t_secs);

    XrlCmdError
    ripng_0_1_route_timeout(const string&	ifname,
			    const string&	vifname,
			    const IPv6&		addr,
			    uint32_t&		t_secs);

    XrlCmdError
    ripng_0_1_set_deletion_delay(const string&		ifname,
				 const string&		vifname,
				 const IPv6&		addr,
				 const uint32_t&	t_secs);

    XrlCmdError
    ripng_0_1_deletion_delay(const string&	ifname,
			     const string&	vifname,
			     const IPv6&	addr,
			     uint32_t&		t_secs);

    XrlCmdError
    ripng_0_1_set_request_interval(const string&	ifname,
				   const string&	vifname,
				   const IPv6&		addr,
				   const uint32_t&	t_secs);

    XrlCmdError
    ripng_0_1_request_interval(const string&	ifname,
			       const string&	vifname,
			       const IPv6&	addr,
			       uint32_t&	t_secs);

    XrlCmdError
    ripng_0_1_set_update_interval(const string&		ifname,
				  const string&		vifname,
				  const IPv6&		addr,
				  const uint32_t&	t_secs);

    XrlCmdError
    ripng_0_1_update_interval(const string&	ifname,
			      const string&	vifname,
			      const IPv6&	addr,
			      uint32_t&		t_secs);

    XrlCmdError
    ripng_0_1_set_update_jitter(const string&	ifname,
				const string&	vifname,
				const IPv6&	addr,
				const uint32_t& t_jitter);

    XrlCmdError
    ripng_0_1_update_jitter(const string&	ifname,
			    const string&	vifname,
			    const IPv6&		addr,
			    uint32_t&		t_jitter);

    XrlCmdError
    ripng_0_1_set_triggered_update_delay(const string&		ifname,
					 const string&		vifname,
					 const IPv6&		addr,
					 const uint32_t&	t_secs);

    XrlCmdError
    ripng_0_1_triggered_update_delay(const string&	ifname,
				     const string&	vifname,
				     const IPv6&	addr,
				     uint32_t&		t_secs);

    XrlCmdError
    ripng_0_1_set_triggered_update_jitter(const string&		ifname,
					  const string&		vifname,
					  const IPv6&		addr,
					  const uint32_t&	t_jitter);

    XrlCmdError
    ripng_0_1_triggered_update_jitter(const string&	ifname,
				      const string&	vifname,
				      const IPv6&	addr,
				      uint32_t&		t_jitter);

    XrlCmdError
    ripng_0_1_set_interpacket_delay(const string&	ifname,
				    const string&	vifname,
				    const IPv6&		addr,
				    const uint32_t&	t_msecs);

    XrlCmdError
    ripng_0_1_interpacket_delay(const string&	ifname,
				const string&	vifname,
				const IPv6&	addr,
				uint32_t&	t_msecs);

    XrlCmdError ripng_0_1_rip_address_status(const string&	ifname,
					     const string&	vifname,
					     const IPv6&	addr,
					     string&		status);

    XrlCmdError ripng_0_1_get_all_addresses(XrlAtomList&	ifnames,
					    XrlAtomList&	vifnames,
					    XrlAtomList&	addrs);

    XrlCmdError ripng_0_1_get_peers(const string&		ifname,
				    const string&		vifname,
				    const IPv6&			addr,
				    XrlAtomList&		peers);

    XrlCmdError ripng_0_1_get_all_peers(XrlAtomList&	peers,
					XrlAtomList&	ifnames,
					XrlAtomList&	vifnames,
					XrlAtomList&	addrs);

    XrlCmdError ripng_0_1_get_counters(const string&	ifname,
				       const string&	vifname,
				       const IPv6&	addr,
				       XrlAtomList&	descriptions,
				       XrlAtomList&	values);

    XrlCmdError ripng_0_1_get_peer_counters(
				const string&	ifname,
				const string&	vifname,
				const IPv6&	addr,
				const IPv6&	peer,
				XrlAtomList&	descriptions,
				XrlAtomList&	values,
				uint32_t&	peer_last_active
				);

    XrlCmdError socket6_user_0_1_recv_event(const string&	sockid,
					    const string&	if_name,
					    const string&	vif_name,
					    const IPv6&		src_host,
					    const uint32_t&	src_port,
					    const vector<uint8_t>& pdata);

    XrlCmdError socket6_user_0_1_inbound_connect_event(
	const string&	sockid,
	const IPv6&	src_host,
	const uint32_t&	src_port,
	const string&	new_sockid,
	bool&		accept);

    XrlCmdError socket6_user_0_1_outgoing_connect_event(
	const string&	sockid);

    XrlCmdError socket6_user_0_1_error_event(const string&	sockid,
					     const string& 	reason,
					     const bool&	fatal);

    XrlCmdError socket6_user_0_1_disconnect_event(const string&	sockid);


    XrlCmdError policy_backend_0_1_configure(
        // Input values,
        const uint32_t& filter,
        const string&   conf);

    XrlCmdError policy_backend_0_1_reset(
        // Input values,
        const uint32_t& filter);

    XrlCmdError policy_backend_0_1_push_routes();

    XrlCmdError policy_redist6_0_1_add_route6(
        // Input values,
        const IPv6Net&  network,
        const bool&     unicast,
        const bool&     multicast,
        const IPv6&     nexthop,
        const uint32_t& metric,
        const XrlAtomList&      policytags);

    XrlCmdError policy_redist6_0_1_delete_route6(
        // Input values,
        const IPv6Net&  network,
        const bool&     unicast,
        const bool&     multicast);

protected:
    EventLoop& 			_e;
    XrlRipCommonTarget<IPv6>* 	_ct;
};

#endif // __RIP_XRL_TARGET_RIPNG_HH__
