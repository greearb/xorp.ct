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

// $XORP: xorp/rip/xrl_target_ripng.hh,v 1.3 2004/02/19 22:01:00 hodson Exp $

#ifndef __RIP_XRL_TARGET_RIPNG_HH__
#define __RIP_XRL_TARGET_RIPNG_HH__

#include "libxorp/status_codes.h"
#include "xrl/targets/ripng_base.hh"

class XrlRouter;
class XrlProcessSpy;

template<typename A> class System;
template<typename A> class XrlPortManager;
template<typename A> class XrlRipCommonTarget;

class XrlRipngTarget : public XrlRipngTargetBase {
public:
    XrlRipngTarget(EventLoop&			e,
		   XrlRouter& 			xr,
		   XrlProcessSpy& 		xps,
		   XrlPortManager<IPv6>&	xpm,
		   bool& 			should_exit);
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
    ripng_0_1_set_route_expiry_seconds(const string&	ifname,
				       const string&	vifname,
				       const IPv6&	addr,
				       const uint32_t&	t_secs);

    XrlCmdError
    ripng_0_1_route_expiry_seconds(const string&	ifname,
				   const string&	vifname,
				   const IPv6&		addr,
				   uint32_t&		t_secs);

    XrlCmdError
    ripng_0_1_set_route_deletion_seconds(
					const string&	ifname,
					const string&	vifname,
					const IPv6&	addr,
					const uint32_t&	t_secs);

    XrlCmdError
    ripng_0_1_route_deletion_seconds(const string&	ifname,
				     const string&	vifname,
				     const IPv6&	addr,
				     uint32_t&		t_secs);

    XrlCmdError
    ripng_0_1_set_table_request_seconds(const string&	ifname,
					const string&	vifname,
					const IPv6&	addr,
					const uint32_t&	t_secs);

    XrlCmdError
    ripng_0_1_table_request_seconds(const string&	ifname,
				    const string&	vifname,
				    const IPv6&		addr,
				    uint32_t&		t_secs);

    XrlCmdError
    ripng_0_1_set_unsolicited_response_min_seconds(
						const string&	ifname,
						const string&	vifname,
						const IPv6&	addr,
						const uint32_t& t_secs);

    XrlCmdError
    ripng_0_1_unsolicited_response_min_seconds(const string&	ifname,
					       const string&	vifname,
					       const IPv6&	addr,
					       uint32_t&	t_secs);

    XrlCmdError
    ripng_0_1_set_unsolicited_response_max_seconds(
						const string&	ifname,
						const string&	vifname,
						const IPv6&	addr,
						const uint32_t& t_secs
						);

    XrlCmdError
    ripng_0_1_unsolicited_response_max_seconds(const string&	ifname,
					       const string&	vifname,
					       const IPv6&	addr,
					       uint32_t&	t_secs);

    XrlCmdError
    ripng_0_1_set_triggered_update_min_seconds(const string&	ifname,
					       const string&	vifname,
					       const IPv6&	addr,
					       const uint32_t&	t_secs);

    XrlCmdError
    ripng_0_1_triggered_update_min_seconds(const string&	ifname,
					   const string&	vifname,
					   const IPv6&		addr,
					   uint32_t&		t_secs);

    XrlCmdError
    ripng_0_1_set_triggered_update_max_seconds(const string&	ifname,
					       const string&	vifname,
					       const IPv6&	addr,
					       const uint32_t&	t_secs);

    XrlCmdError
    ripng_0_1_triggered_update_max_seconds(const string&	ifname,
					   const string&	vifname,
					   const IPv6&		addr,
					   uint32_t&		t_secs);

    XrlCmdError
    ripng_0_1_set_interpacket_delay_milliseconds(
					const string&	ifname,
					const string&	vifname,
					const IPv6&	addr,
					const uint32_t&	t_msecs
					);

    XrlCmdError
    ripng_0_1_interpacket_delay_milliseconds(const string&	ifname,
					     const string&	vifname,
					     const IPv6&	addr,
					     uint32_t&		t_msecs);

    XrlCmdError ripng_0_1_rip_address_status(const string&	ifname,
					     const string&	vifname,
					     const IPv6&	addr,
					     string&		status);

    XrlCmdError ripng_0_1_add_static_route(const IPv6Net& 	network,
					   const IPv6& 		nexthop,
					   const uint32_t& 	cost);

    XrlCmdError ripng_0_1_delete_static_route(const IPv6Net& 	network);

    XrlCmdError socket6_user_0_1_recv_event(const string&	sockid,
					    const IPv6&		src_host,
					    const uint32_t&	src_port,
					    const vector<uint8_t>& pdata);

    XrlCmdError socket6_user_0_1_connect_event(const string&	sockid,
					       const IPv6&	src_host,
					       const uint32_t&	src_port,
					       const string&	new_sockid,
					       bool&		accept);

    XrlCmdError socket6_user_0_1_error_event(const string&	sockid,
					     const string& 	reason,
					     const bool&	fatal);

    XrlCmdError socket6_user_0_1_close_event(const string&	sockid,
					     const string&	reason);

protected:
    EventLoop& 			_e;
    XrlRipCommonTarget<IPv6>* 	_ct;
};

#endif // __RIP_XRL_TARGET_RIPNG_HH__
