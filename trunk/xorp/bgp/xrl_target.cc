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

#ident "$XORP: xorp/bgp/xrl_target.cc,v 1.4 2003/01/17 04:07:24 mjh Exp $"

#include "config.h"
#include "bgp_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libxorp/eventloop.hh"
#include "libxipc/xrl_std_router.hh"
#include "xrl_target.hh"

XrlBgpTarget::XrlBgpTarget(XrlRouter *r, BGPMain& bgp)
	: XrlBgpTargetBase(r),
	  _bgp(bgp),
	  _awaiting_config(true),
	  _awaiting_as(true),
	  _awaiting_bgpid(true),
	  _done(false)
{
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_bgp_version(
				      // Output values, 
				      uint32_t&	version)
{
    version = 4;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::bgp_0_2_local_config(
				   // Input values, 
				   const uint32_t&	as, 
				   const IPv4&		id)
{
    debug_msg("as %d id %s\n", as, id.str().c_str());

    /*
    ** We may already be configured so don't allow a reconfiguration.
    */
    if(!_awaiting_config)
	return XrlCmdError::COMMAND_FAILED("Attempt to reconfigure BGP");

    _bgp.local_config(as, id);

    _awaiting_config = false;

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::bgp_0_2_set_local_as(
				   // Input values, 
				   const uint32_t&	as)
{
    debug_msg("as %d\n", as);

    /*
    ** We may already be configured so don't allow a reconfiguration.
    */
    if(!_awaiting_as)
	return XrlCmdError::COMMAND_FAILED("Attempt to reconfigure BGP AS");

    _as = as;
    _awaiting_as = false;
    if(!_awaiting_as && !_awaiting_bgpid) {
	_bgp.local_config(_as, _id);
	_awaiting_config = false;	
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_local_as(
				   // Output values, 
				   uint32_t& as) 
{
    if(_awaiting_as)
	return XrlCmdError::COMMAND_FAILED("BGP AS not yet configured");
    as = _as;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::bgp_0_2_set_bgpid(
				// Input values, 
				const IPv4&	id)
{
    debug_msg("id %s\n", id.str().c_str());

    /*
    ** We may already be configured so don't allow a reconfiguration.
    */
    if(!_awaiting_bgpid)
	return XrlCmdError::COMMAND_FAILED("Attempt to reconfigure BGP ID");

    _id = id;
    _awaiting_bgpid = false;
    if(!_awaiting_as && !_awaiting_bgpid) {
	_bgp.local_config(_as, _id);
	_awaiting_config = false;	
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_bgpid(
				// Output values, 
				IPv4& id) 
{
    if(_awaiting_bgpid)
	return XrlCmdError::COMMAND_FAILED("BGP ID not yet configured");
    id = _id;
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_add_peer(
	// Input values, 
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port, 
	const uint32_t&	as, 
	const IPv4&	next_hop,
	const uint32_t&	holdtime)
{
    debug_msg("local ip %s local port %d peer ip %s peer port %d as %d"
	      " next_hop %s holdtime %d\n",
	      local_ip.c_str(), local_port, peer_ip.c_str(), peer_port,
	      as, next_hop.str().c_str(), holdtime);

    if(_awaiting_config)
	return XrlCmdError::COMMAND_FAILED("BGP Not configured!!!");

    Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(), peer_port);

    BGPPeerData *pd = new BGPPeerData(iptuple,
				      AsNum(static_cast<uint16_t>(as)),
				      next_hop, holdtime);

    if(!_bgp.create_peer(pd)) {
	delete pd;
	return XrlCmdError::COMMAND_FAILED();
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_delete_peer(
	// Input values, 
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port, 
	const uint32_t&	as)
{
    debug_msg("local ip %s local port %d peer ip %s peer port %d as %d\n",
	      local_ip.c_str(), local_port, peer_ip.c_str(), peer_port, as);

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_enable_peer(
	// Input values, 
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port)
{
    debug_msg("local ip %s local port %d peer ip %s peer port %d\n",
	      local_ip.c_str(), local_port, peer_ip.c_str(), peer_port);

    Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(), peer_port);

    if(!_bgp.enable_peer(iptuple))
	return XrlCmdError::COMMAND_FAILED();

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_disable_peer(
	// Input values, 
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port)
{
    debug_msg("local ip %s local port %d peer ip %s peer port %d\n",
	      local_ip.c_str(), local_port, peer_ip.c_str(), peer_port);

    Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(), peer_port);

    if(!_bgp.disable_peer(iptuple))
	return XrlCmdError::COMMAND_FAILED();

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_set_peer_state(
	// Input values, 
	const string&	local_ip, 
	const uint32_t&	local_port, 
	const string&	peer_ip, 
	const uint32_t&	peer_port, 
	const bool& toggle)
{
    debug_msg("local ip %s local port %d peer ip %s peer port %d toggle %d\n",
	  local_ip.c_str(), local_port, peer_ip.c_str(), peer_port, toggle);

    Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(), peer_port);

    if(toggle) {
	if(!_bgp.enable_peer(iptuple))
	    return XrlCmdError::COMMAND_FAILED();
    } else {
	if(!_bgp.disable_peer(iptuple))
	    return XrlCmdError::COMMAND_FAILED();
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_peer_list_start(
					  // Output values, 
					  uint32_t& token,
					  bool&	more)
{
    more = _bgp.get_peer_list_start(token);
    return  XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_peer_list_next(
					 // Input values, 
					 const uint32_t&	token, 
					 // Output values, 
					 IPv4&	local_ip, 
					 uint32_t&	local_port, 
					 IPv4&	peer_ip, 
					 uint32_t&	peer_port, 
					 bool&	more)
{
    more = _bgp.get_peer_list_next(token, local_ip, local_port, peer_ip, 
				   peer_port);
    return  XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_peer_id(
				  // Input values, 
				  const IPv4&	local_ip, 
				  const uint32_t&	local_port, 
				  const IPv4&	peer_ip, 
				  const uint32_t&	peer_port, 
				  // Output values, 
				  IPv4&	peer_id)
{
    Iptuple iptuple(local_ip, local_port, peer_ip, peer_port);
    if (!_bgp.get_peer_id(iptuple, peer_id)) {
	return XrlCmdError::COMMAND_FAILED();
    }
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_peer_status(
				      // Input values, 
				      const IPv4&	local_ip, 
				      const uint32_t&	local_port, 
				      const IPv4&	peer_ip, 
				      const uint32_t&	peer_port, 
				      // Output values, 
				      uint32_t&	peer_state, 
				      uint32_t&	admin_status)
{
    Iptuple iptuple(local_ip, local_port, peer_ip, peer_port);
    if (!_bgp.get_peer_status(iptuple, peer_state, admin_status)) {
	return XrlCmdError::COMMAND_FAILED();
    }
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_peer_negotiated_version(
						  // Input values, 
						  const IPv4& local_ip, 
						  const uint32_t& local_port, 
						  const IPv4& peer_ip, 
						  const uint32_t& peer_port, 
						  // Output values, 
						  int32_t& neg_version)
{
    Iptuple iptuple(local_ip, local_port, peer_ip, peer_port);
    if (!_bgp.get_peer_negotiated_version(iptuple, neg_version)) {
	return XrlCmdError::COMMAND_FAILED();
    }
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_peer_as(
				  // Input values, 
				  const IPv4& local_ip, 
				  const uint32_t& local_port, 
				  const IPv4&	peer_ip, 
				  const uint32_t& peer_port, 
				  // Output values, 
				  uint32_t& peer_as)
{
    Iptuple iptuple(local_ip, local_port, peer_ip, peer_port);
    if (!_bgp.get_peer_as(iptuple, peer_as)) {
	return XrlCmdError::COMMAND_FAILED();
    }
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_peer_msg_stats(
					 // Input values, 
					 const IPv4& local_ip, 
					 const uint32_t& local_port, 
					 const IPv4& peer_ip, 
					 const uint32_t& peer_port, 
					 // Output values, 
					 uint32_t&	in_updates, 
					 uint32_t&	out_updates, 
					 uint32_t&	in_msgs, 
					 uint32_t&	out_msgs, 
					 uint32_t&	last_error, 
					 uint32_t&	in_update_elapsed)
{
    Iptuple iptuple(local_ip, local_port, peer_ip, peer_port);
    uint16_t last_error_short;
    if (!_bgp.get_peer_msg_stats(iptuple, in_updates, out_updates,
				 in_msgs, out_msgs, last_error_short, 
				 in_update_elapsed)) {
	return XrlCmdError::COMMAND_FAILED();
    }
    last_error = last_error_short;
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_peer_established_stats(
						 // Input values, 
						 const IPv4& local_ip, 
						 const uint32_t& local_port, 
						 const IPv4& peer_ip, 
						 const uint32_t& peer_port, 
						 // Output values, 
						 uint32_t& transitions, 
						 uint32_t& established_time)
{
    Iptuple iptuple(local_ip, local_port, peer_ip, peer_port);
    if (!_bgp.get_peer_established_stats(iptuple, transitions, 
					 established_time)) {
	return XrlCmdError::COMMAND_FAILED();
    }
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_get_peer_timer_config(
					    // Input values, 
					    const IPv4&	local_ip, 
					    const uint32_t& local_port, 
					    const IPv4&	peer_ip, 
					    const uint32_t& peer_port, 
					    // Output values, 
					    uint32_t& retry_interval, 
					    uint32_t& hold_time, 
					    uint32_t& keep_alive, 
					    uint32_t& hold_time_conf, 
					    uint32_t& keep_alive_conf, 
					    uint32_t& min_as_origin_interval,
					    uint32_t& min_route_adv_interval)
{
    Iptuple iptuple(local_ip, local_port, peer_ip, peer_port);
    if (!_bgp.get_peer_timer_config(iptuple, retry_interval, hold_time,
				    keep_alive, hold_time_conf,
				    keep_alive_conf, min_as_origin_interval,
				    min_route_adv_interval)) {
	return XrlCmdError::COMMAND_FAILED();
    }
    return XrlCmdError::OKAY();
}


XrlCmdError
XrlBgpTarget::bgp_0_2_register_rib(
				   // Input values, 
				   const string&	name)
{
    debug_msg("%s\n", name.c_str());

    if(!_bgp.register_ribname(name)) {
	return XrlCmdError::COMMAND_FAILED(c_format(
			    "Couldn't register rib name %s",
					   name.c_str()));
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlBgpTarget::bgp_0_2_add_route(
		  // Input values, 
		  const int32_t&	origin, 
		  const int32_t&	asnum, 
		  const IPv4&	next_hop, 
		  const IPv4Net&	nlri)
{
    debug_msg("add_route: origin %d asnum %d next hop %s nlri %s\n",
	      origin, asnum, next_hop.str().c_str(), nlri.str().c_str());

    if(_awaiting_config)
	return XrlCmdError::COMMAND_FAILED("BGP Not configured!!!");

    OriginType ot;

    switch(origin) {
    case IGP:
	break;
    case EGP:
	break;
    case INCOMPLETE:
	break;
    default:
	XLOG_ERROR("Bad origin value %d", origin);
	return XrlCmdError::BAD_ARGS();
    }

    ot = static_cast<OriginType>(origin);

    if(!_bgp.add_route(ot, AsNum(static_cast<uint16_t>(asnum)),
		       next_hop, nlri))
	return XrlCmdError::COMMAND_FAILED();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::bgp_0_2_delete_route(
				   // Input values, 
				   const IPv4Net&	nlri)
{
    if(_awaiting_config)
	return XrlCmdError::COMMAND_FAILED("BGP Not configured!!!");

    if(!_bgp.delete_route(nlri))
	return XrlCmdError::COMMAND_FAILED();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlBgpTarget::bgp_0_2_terminate()
{
    if(!_awaiting_config) {
	_bgp.terminate();
    } else {
	_awaiting_config = false;
	_done = true;
    }

    return XrlCmdError::OKAY();
}

XrlCmdError XrlBgpTarget::rib_client_0_1_route_info_changed4(
        // Input values, 
        const IPv4& addr,
        const uint32_t& prefix_len,
	const IPv4&	nexthop, 
	const uint32_t&	metric)
{
    IPNet<IPv4> net(addr, prefix_len);
    debug_msg("IGP route into changed for net %s\n", net.str().c_str());
    debug_msg("Nexthop: %s Metric: %d\n", nexthop.str().c_str(), metric);

    if(!_bgp.rib_client_route_info_changed4(addr, prefix_len, nexthop, metric))
	return XrlCmdError::COMMAND_FAILED();

    return XrlCmdError::OKAY();
}

XrlCmdError XrlBgpTarget::rib_client_0_1_route_info_changed6(
	// Input values, 
	const IPv6&	addr, 
	const uint32_t&	prefix_len,
	const IPv6&	nexthop, 
	const uint32_t&	metric)
{
    IPNet<IPv6> net(addr, prefix_len);
    debug_msg("IGP route into changed for net %s\n", net.str().c_str());
    debug_msg("Nexthop: %s Metric: %d\n", nexthop.str().c_str(), metric);

    if(!_bgp.rib_client_route_info_changed6(addr, prefix_len, nexthop, metric))
	return XrlCmdError::COMMAND_FAILED();

    return XrlCmdError::OKAY();
}

XrlCmdError XrlBgpTarget::rib_client_0_1_route_info_invalid4(
	// Input values, 
	const IPv4&	addr, 
	const uint32_t&	prefix_len)
{
    IPNet<IPv4> net(addr, prefix_len);
    debug_msg("IGP route into changed for net %s\n", net.str().c_str());

    if(!_bgp.rib_client_route_info_invalid4(addr, prefix_len))
	return XrlCmdError::COMMAND_FAILED();

    return XrlCmdError::OKAY();
}

XrlCmdError XrlBgpTarget::rib_client_0_1_route_info_invalid6(
	// Input values, 
	const IPv6&	addr, 
	const uint32_t&	prefix_len)
{
    IPNet<IPv6> net(addr, prefix_len);
    debug_msg("IGP route into changed for net %s\n", net.str().c_str());

    if(!_bgp.rib_client_route_info_invalid6(addr, prefix_len))
	return XrlCmdError::COMMAND_FAILED();

    return XrlCmdError::OKAY();
}

 
XrlCmdError XrlBgpTarget::bgp_0_2_set_parameter(
				  // Input values,
				  const string&	local_ip, 
				  const uint32_t&	local_port, 
				  const string&	peer_ip, 
				  const uint32_t&	peer_port, 
				  const string& parameter)
{
    debug_msg("local ip %s local port %d peer ip %s peer port %d paremeter %s\n",
	      local_ip.c_str(), local_port, peer_ip.c_str(), peer_port, parameter.c_str());

    Iptuple iptuple(local_ip.c_str(), local_port, peer_ip.c_str(), peer_port);

    if(!_bgp.set_parameter(iptuple,parameter))
	return XrlCmdError::COMMAND_FAILED();

    return XrlCmdError::OKAY();
}

				     
bool 
XrlBgpTarget::waiting()
{
    return _awaiting_config;
}

bool 
XrlBgpTarget::done()
{
    return _done;
}
