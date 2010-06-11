// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"



#include "libxipc/xrl_std_router.hh"

#include "ospf.hh"
#include "xrl_io.hh"
#include "xrl_target3.hh"


XrlOspfV3Target::XrlOspfV3Target(XrlRouter *r,
				 /*Ospf<IPv4>& ospf_ipv4, */
				 Ospf<IPv6>& ospf_ipv6, 
				 /*XrlIO<IPv4>& io_ipv4,*/
				 XrlIO<IPv6>& io_ipv6)
    : XrlOspfv3TargetBase(r),
      /*_ospf_ipv4(ospf_ipv4),*/ _ospf_ipv6(ospf_ipv6),
      /*_xrl_io_ipv4(io_ipv4),*/ _xrl_io_ipv6(io_ipv6)
{
}

XrlCmdError
XrlOspfV3Target::common_0_1_get_target_name(string&  name)
{
    name = "ospfv3";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::common_0_1_get_version(string&	version)
{
    version = "0.1";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::common_0_1_get_status(uint32_t& status,
				       string& reason)
{
    status = _ospf_ipv6.status(reason);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::common_0_1_startup()
{
    // Starts by default...nothing to do here.
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::common_0_1_shutdown()
{
    _ospf_ipv6.shutdown();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::raw_packet4_client_0_1_recv(
    // Input values,
    const string&	/*if_name*/,
    const string&	/*vif_name*/,
    const IPv4&		/*src_address*/,
    const IPv4&		/*dst_address*/,
    const uint32_t&	/*ip_protocol*/,
    const int32_t&	/*ip_ttl*/,
    const int32_t&	/*ip_tos*/,
    const bool&		/*ip_router_alert*/,
    const bool&		/*ip_internet_control*/,
    const vector<uint8_t>& /*payload*/)
{
#if	0
    _xrl_io_ipv4.recv(if_name,
		      vif_name,
		      src_address,
		      dst_address,
		      ip_protocol,
		      ip_ttl,
		      ip_tos,
		      ip_router_alert,
		      ip_internet_control,
		      payload);
#else
    XLOG_UNFINISHED();
#endif

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::raw_packet6_client_0_1_recv(
    // Input values,
    const string&	if_name,
    const string&	vif_name,
    const IPv6&		src_address,
    const IPv6&		dst_address,
    const uint32_t&	ip_protocol,
    const int32_t&	ip_ttl,
    const int32_t&	ip_tos,
    const bool&		ip_router_alert,
    const bool&		ip_internet_control,
    const XrlAtomList&	ext_headers_type,
    const XrlAtomList&	ext_headers_payload,
    const vector<uint8_t>& payload)
{
    UNUSED(ext_headers_type);
    UNUSED(ext_headers_payload);

    _xrl_io_ipv6.recv(if_name,
		      vif_name,
		      src_address,
		      dst_address,
		      ip_protocol,
		      ip_ttl,
		      ip_tos,
		      ip_router_alert,
		      ip_internet_control,
		      payload);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::policy_backend_0_1_configure(const uint32_t& filter,
					      const string& conf)
{
    debug_msg("policy filter: %u conf: %s\n", filter, conf.c_str());

    try {
	_ospf_ipv6.configure_filter(filter,conf);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("Filter configure failed: " +
					   e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::policy_backend_0_1_reset(const uint32_t& filter)
{
    debug_msg("policy filter reset: %u\n", filter);

    try {
	_ospf_ipv6.reset_filter(filter);
    } catch(const PolicyException& e){ 
	return XrlCmdError::COMMAND_FAILED("Filter reset failed: " +
					   e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::policy_backend_0_1_push_routes()
{
    debug_msg("policy route push\n");

    _ospf_ipv6.push_routes();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::policy_redist6_0_1_add_route6(const IPv6Net& network,
					       const bool& unicast,
					       const bool& multicast,
					       const IPv6& nexthop,
					       const uint32_t& metric,
					       const XrlAtomList& policytags)
{
    debug_msg("Net: %s Nexthop: %s Unicast: %s Multicast %s metric %d\n",
	      cstring(network), cstring(nexthop), bool_c_str(unicast),
	      bool_c_str(multicast), metric);

    if (!unicast)
	return XrlCmdError::OKAY();

    if (!_ospf_ipv6.originate_route(network, nexthop, metric, policytags)) {
	return XrlCmdError::COMMAND_FAILED("Network: " + network.str());
    }

    return XrlCmdError::OKAY();
}


XrlCmdError
XrlOspfV3Target::policy_redist6_0_1_delete_route6(const IPv6Net& network,
						  const bool& unicast,
						  const bool& multicast)
{
    debug_msg("Net: %s Unicast: %s Multicast %s\n",
	      cstring(network), bool_c_str(unicast), bool_c_str(multicast));

    if (!unicast)
	return XrlCmdError::OKAY();

    if (!_ospf_ipv6.withdraw_route(network)) {
	return XrlCmdError::COMMAND_FAILED("Network: " + network.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_set_instance_id(const uint32_t& id)
{
    _ospf_ipv6.set_instance_id(id);

    return XrlCmdError::OKAY();
}


XrlCmdError
XrlOspfV3Target::ospfv3_0_1_set_router_id(const IPv4& id)
{
    OspfTypes::RouterID rid = ntohl(id.addr());

    _ospf_ipv6.set_router_id(rid);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_set_ip_router_alert(const bool& ip_router_alert)
{
    if (!_ospf_ipv6.set_ip_router_alert(ip_router_alert))
	return XrlCmdError::COMMAND_FAILED("Failed to set IP router alert");

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfV3Target::ospfv3_0_1_create_area_router(const IPv4& a,
					       const string& type)
{
    bool status;
    OspfTypes::AreaType t = from_string_to_area_type(type, status);
    if (!status)
	return XrlCmdError::COMMAND_FAILED("Unrecognised type " + type);
	
    OspfTypes::AreaID area = ntohl(a.addr());
    if (!_ospf_ipv6.get_peer_manager().create_area_router(area, t))
	return XrlCmdError::COMMAND_FAILED("Failed to create area " +
					   pr_id(area));

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfV3Target::ospfv3_0_1_change_area_router_type(const IPv4& a,
						    const string& type)
{
    bool status;
    OspfTypes::AreaType t = from_string_to_area_type(type, status);
    if (!status)
	return XrlCmdError::COMMAND_FAILED("Unrecognised type " + type);
	
    OspfTypes::AreaID area = ntohl(a.addr());
    if (!_ospf_ipv6.get_peer_manager().change_area_router_type(area, t))
	return XrlCmdError::COMMAND_FAILED("Failed to create area " +
					   pr_id(area));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_destroy_area_router(const IPv4& a)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    if (!_ospf_ipv6.get_peer_manager().destroy_area_router(area))
	return XrlCmdError::COMMAND_FAILED("Failed to destroy area " +
					   pr_id(area));

    return XrlCmdError::OKAY();
}


XrlCmdError
XrlOspfV3Target::ospfv3_0_1_create_peer(const string& ifname,
					const string& vifname,
					const string& type,
					const IPv4& a)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    bool status;
    OspfTypes::LinkType linktype = from_string_to_link_type(type, status);
    if (!status)
	return XrlCmdError::COMMAND_FAILED("Unrecognised type " + type);

    try {
	_ospf_ipv6.get_peer_manager().create_peer(ifname, vifname,
						  IPv6::ZERO(),
						  linktype, area);
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfV3Target::ospfv3_0_1_delete_peer(const string& ifname,
					const string& vifname)
{
    debug_msg("interface %s vif %s\n", ifname.c_str(), vifname.c_str());

    OspfTypes::PeerID peerid;
    try {
	peerid = _ospf_ipv6.get_peer_manager().get_peerid(ifname, vifname);
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }
    if (!_ospf_ipv6.get_peer_manager().delete_peer(peerid))
	return XrlCmdError::COMMAND_FAILED("Failed to delete peer");

    return XrlCmdError::OKAY();
}


XrlCmdError
XrlOspfV3Target::ospfv3_0_1_set_peer_state(const string& ifname,
					   const string& vifname,
					   const bool& enable)
{
    debug_msg("interface %s vif %s enable %s\n", ifname.c_str(),
	      vifname.c_str(), bool_c_str(enable));

    OspfTypes::PeerID peerid;
    try {
	peerid = _ospf_ipv6.get_peer_manager().get_peerid(ifname, vifname);
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }
    if (!_ospf_ipv6.get_peer_manager().set_state_peer(peerid, enable))
	return XrlCmdError::COMMAND_FAILED("Failed to set peer state");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_add_address_peer(const string& ifname,
					     const string& vifname,
					     const IPv4& a,
					     const IPv6& addr)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("interface %s vif %s area %s address %s\n", ifname.c_str(),
	      vifname.c_str(), pr_id(area).c_str(), cstring(addr));

    if (!_ospf_ipv6.get_peer_manager().add_address_peer(ifname, vifname, area,
							addr))
	return XrlCmdError::COMMAND_FAILED("Failed to add address");

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfV3Target::ospfv3_0_1_remove_address_peer(const string& ifname,
						const string& vifname,
						const IPv4& a,
						const IPv6& addr)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("interface %s vif %s area %s address %s\n", ifname.c_str(),
	      vifname.c_str(), pr_id(area).c_str(), cstring(addr));

    OspfTypes::PeerID peerid;
    try {
	peerid = _ospf_ipv6.get_peer_manager().get_peerid(ifname, vifname);
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }
    if (!_ospf_ipv6.get_peer_manager().remove_address_peer(peerid, area, addr))
	return XrlCmdError::COMMAND_FAILED("Failed to remove address");

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfV3Target::ospfv3_0_1_set_address_state_peer(const string& ifname,
						   const string& vifname,
						   const IPv4& a,
						   const IPv6& addr,
						   const bool& enable)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("interface %s vif %s area %s address %s enable %s\n",
	      ifname.c_str(), vifname.c_str(), pr_id(area).c_str(),
	      cstring(addr), bool_c_str(enable));

    OspfTypes::PeerID peerid;
    try {
	peerid = _ospf_ipv6.get_peer_manager().get_peerid(ifname, vifname);
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }
    if (!_ospf_ipv6.get_peer_manager().
	set_address_state_peer(peerid, area, addr, enable))
	return XrlCmdError::COMMAND_FAILED("Failed to set address state");

    return XrlCmdError::OKAY();
}


XrlCmdError
XrlOspfV3Target::ospfv3_0_1_activate_peer(const string&	ifname,
					  const string&	vifname,
					  const IPv4& a)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("interface %s vif %s area %s\n", ifname.c_str(),
	      vifname.c_str(), pr_id(area).c_str());

    if (!_ospf_ipv6.get_peer_manager().activate_peer(ifname, vifname, area))
	return XrlCmdError::COMMAND_FAILED("Failed to activate peer");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_update_peer(const string& ifname,
					const string& vifname,
					const IPv4& a)

{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("interface %s vif %s area %s\n", ifname.c_str(),
	      vifname.c_str(), pr_id(area).c_str());

    if (!_ospf_ipv6.get_peer_manager().update_peer(ifname, vifname, area))
	return XrlCmdError::COMMAND_FAILED("Failed to update peer");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_add_neighbour(const string&	ifname,
					  const string&	vifname,
					  const IPv4& addr,
					  const IPv6& neighbour_address,
					  const IPv4& neighbour_id)
{
    OspfTypes::AreaID area = ntohl(addr.addr());
    OspfTypes::RouterID rid = ntohl(neighbour_id.addr());
    debug_msg("interface %s vif %s area %s address %s id %s\n", ifname.c_str(),
	      vifname.c_str(), pr_id(area).c_str(), 
	      cstring(neighbour_address),pr_id(rid).c_str());

    OspfTypes::PeerID peerid;
    try {
	peerid = _ospf_ipv6.get_peer_manager().get_peerid(ifname, vifname);
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }
    if (!_ospf_ipv6.get_peer_manager().add_neighbour(peerid, area,
						     neighbour_address,
						     rid))
	return XrlCmdError::COMMAND_FAILED("Failed to add neighbour " +
					   neighbour_address.str());

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfV3Target::ospfv3_0_1_remove_neighbour(const string& ifname,
					     const string& vifname,
					     const IPv4& addr,
					     const IPv6& neighbour_address,
					     const IPv4& neighbour_id)
{
    OspfTypes::AreaID area = ntohl(addr.addr());
    OspfTypes::RouterID rid = ntohl(neighbour_id.addr());
    debug_msg("interface %s vif %s area %s address %s id %s\n", ifname.c_str(),
	      vifname.c_str(), pr_id(area).c_str(), 
	      cstring(neighbour_address),pr_id(rid).c_str());

    OspfTypes::PeerID peerid;
    try {
	peerid = _ospf_ipv6.get_peer_manager().get_peerid(ifname, vifname);
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }
    if (!_ospf_ipv6.get_peer_manager().remove_neighbour(peerid, area,
							neighbour_address,
							rid))
	return XrlCmdError::COMMAND_FAILED("Failed to remove neighbour" +
					   neighbour_address.str());

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_create_virtual_link(const IPv4& neighbour_id,
						const IPv4& a)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    OspfTypes::RouterID rid = ntohl(neighbour_id.addr());
    debug_msg("Neighbour's router ID %s configuration area %s\n",
	      pr_id(rid).c_str(), pr_id(area).c_str());

    if (OspfTypes::BACKBONE != area) {
	return XrlCmdError::
	    COMMAND_FAILED(c_format("Virtual link must be in area %s",
				    pr_id(OspfTypes::BACKBONE).c_str()));
    }

    if (!_ospf_ipv6.create_virtual_link(rid))
	return XrlCmdError::COMMAND_FAILED("Failed to create virtual link");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_delete_virtual_link(const IPv4& neighbour_id)
{
    OspfTypes::RouterID rid = ntohl(neighbour_id.addr());

    if (!_ospf_ipv6.delete_virtual_link(rid))
	return XrlCmdError::COMMAND_FAILED("Failed to delete virtual link");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_transit_area_virtual_link(const IPv4& neighbour_id,
						      const IPv4& transit_area)
{
    OspfTypes::RouterID rid = ntohl(neighbour_id.addr());
    OspfTypes::AreaID area = ntohl(transit_area.addr());

    if (!_ospf_ipv6.transit_area_virtual_link(rid, area))
	return XrlCmdError::COMMAND_FAILED("Failed to configure transit area");

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfV3Target::ospfv3_0_1_set_interface_cost(const string& ifname,
					       const string& vifname,
					       const IPv4& a,
					       const uint32_t& cost)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("interface %s vif %s area %s cost %d\n", ifname.c_str(),
	      vifname.c_str(), pr_id(area).c_str(), cost);

    if (!_ospf_ipv6.set_interface_cost(ifname, vifname, area, cost))
	return XrlCmdError::COMMAND_FAILED("Failed to set "
					   "interface cost");
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfV3Target::ospfv3_0_1_set_retransmit_interval(const string& ifname,
						    const string& vifname,
						    const IPv4& a,
						    const uint32_t& interval)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("interface %s vif %s area %s interval %d\n", ifname.c_str(),
	      vifname.c_str(), pr_id(area).c_str(), interval);

    if (!_ospf_ipv6.set_retransmit_interval(ifname, vifname, area, interval))
	return XrlCmdError::COMMAND_FAILED("Failed to set "
					   "RxmtInterval interval");

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfV3Target::ospfv3_0_1_set_inftransdelay(const string& ifname,
					      const string& vifname,
					      const IPv4& a,
					      const uint32_t& delay)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("interface %s vif %s area %s delay %d\n", ifname.c_str(),
	      vifname.c_str(), pr_id(area).c_str(), delay);

    if (!_ospf_ipv6.set_inftransdelay(ifname, vifname, area, delay))
	return XrlCmdError::COMMAND_FAILED("Failed to set "
					   "inftransdelay delay");

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfV3Target::ospfv3_0_1_set_router_priority(const string& ifname,
						const string& vifname,
						const IPv4& a,
						const uint32_t& priority)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("interface %s vif %s area %s priority %d\n", ifname.c_str(),
	      vifname.c_str(), pr_id(area).c_str(), priority);

    if (!_ospf_ipv6.set_router_priority(ifname, vifname, area, priority))
	return XrlCmdError::COMMAND_FAILED("Failed to set priority");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_set_hello_interval(const string& ifname,
					       const string& vifname,
					       const IPv4& a,
					       const uint32_t& interval)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("interface %s vif %s area %s interval %d\n", ifname.c_str(),
	      vifname.c_str(), pr_id(area).c_str(), interval);

    if (!_ospf_ipv6.set_hello_interval(ifname, vifname, area, interval))
	return XrlCmdError::COMMAND_FAILED("Failed to set hello interval");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_set_router_dead_interval(const string& ifname,
						     const string& vifname,
						     const IPv4& a,
						     const uint32_t& interval)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("interface %s vif %s area %s interval %d\n", ifname.c_str(),
	      vifname.c_str(), pr_id(area).c_str(), interval);

    if (!_ospf_ipv6.set_router_dead_interval(ifname, vifname, area, interval))
	return XrlCmdError::COMMAND_FAILED("Failed to set "
					   "router dead interval");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_set_passive(const string& ifname,
					const string& vifname,
					const IPv4& a,
					const bool& passive)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("interface %s vif %s area %s passive %s\n",
	      ifname.c_str(), vifname.c_str(), pr_id(area).c_str(),
	      bool_c_str(passive));

    if (!_ospf_ipv6.set_passive(ifname, vifname, area, passive, false))
	return XrlCmdError::COMMAND_FAILED("Failed to configure make passive");

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfV3Target::ospfv3_0_1_originate_default_route(const IPv4&	a,
						    const bool&	enable)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("area %s enable %s\n", pr_id(area).c_str(), bool_c_str(enable));

    if (!_ospf_ipv6.originate_default_route(area, enable))
	return XrlCmdError::
	    COMMAND_FAILED("Failed to configure default route");

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfV3Target::ospfv3_0_1_stub_default_cost(const IPv4& a,
					      const uint32_t& cost)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("area %s cost %u\n", pr_id(area).c_str(), cost);

    if (!_ospf_ipv6.stub_default_cost(area, cost))
	return XrlCmdError::
	    COMMAND_FAILED("Failed set StubDefaultCost");

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfV3Target::ospfv3_0_1_summaries(const IPv4& a,
				      const bool& enable)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("area %s enable %s\n", pr_id(area).c_str(), bool_c_str(enable));

    if (!_ospf_ipv6.summaries(area, enable))
	return XrlCmdError::
	    COMMAND_FAILED("Failed to configure summaries");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_area_range_add(const IPv4& a,
					   const IPv6Net& net,
					   const bool& advertise)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("area %s net %s advertise %s\n", pr_id(area).c_str(),
	      cstring(net), bool_c_str(advertise));

    if (!_ospf_ipv6.area_range_add(area, net, advertise))
	return XrlCmdError::
	    COMMAND_FAILED(c_format("Failed to add area range "
				    "area %s net %s advertise %s\n",
				    pr_id(area).c_str(), cstring(net),
				    bool_c_str(advertise)));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_area_range_delete(const IPv4& a,
					      const IPv6Net& net)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("area %s net %s\n", pr_id(area).c_str(), cstring(net));

    if (!_ospf_ipv6.area_range_delete(area, net))
	return XrlCmdError::
	    COMMAND_FAILED(c_format("Failed to delete area range "
				    "area %s net %s\n",
				    pr_id(area).c_str(), cstring(net)));


    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_area_range_change_state(const IPv4& a,
						    const IPv6Net& net,
						    const bool&	advertise)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("area %s net %s advertise %s\n", pr_id(area).c_str(),
	      cstring(net), bool_c_str(advertise));

    if (!_ospf_ipv6.area_range_change_state(area, net, advertise))
	return XrlCmdError::
	    COMMAND_FAILED(c_format("Failed to change area range "
				    "area %s net %s advertise %s\n",
				    pr_id(area).c_str(), cstring(net),
				    bool_c_str(advertise)));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_trace(const string&	tvar, const bool& enable)
{
    debug_msg("trace variable %s enable %s\n", tvar.c_str(),
	      bool_c_str(enable));

    if (tvar == "all") {
	_ospf_ipv6.trace().all(enable);
    } else {
	return XrlCmdError::
	    COMMAND_FAILED(c_format("Unknown variable %s", tvar.c_str()));
    } 

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::XrlOspfV3Target::ospfv3_0_1_get_lsa(const IPv4& a,
						     const uint32_t& index,
						     bool& valid,
						     bool& toohigh,
						     bool& self,
						     vector<uint8_t>& lsa)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("area %s index %u\n", pr_id(area).c_str(), index);

    if (!_ospf_ipv6.get_lsa(area, index, valid, toohigh, self, lsa))
	return XrlCmdError::COMMAND_FAILED("Unable to get LSA");

    debug_msg("area %s index %u valid %s toohigh %s self %s\n",
	      pr_id(area).c_str(), index, bool_c_str(valid),
	      bool_c_str(toohigh), bool_c_str(self));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_get_area_list(XrlAtomList& areas)
{
    list<OspfTypes::AreaID> arealist;

    if (!_ospf_ipv6.get_area_list(arealist))
	return XrlCmdError::COMMAND_FAILED("Unable to get area list");

    list<OspfTypes::AreaID>::const_iterator i;
    for (i = arealist.begin(); i != arealist.end(); i++)
	areas.append(XrlAtom(*i));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_get_neighbour_list(XrlAtomList& neighbours)
{
    list<OspfTypes::NeighbourID> neighbourlist;

    if (!_ospf_ipv6.get_neighbour_list(neighbourlist))
	return XrlCmdError::COMMAND_FAILED("Unable to get neighbour list");

    list<OspfTypes::NeighbourID>::const_iterator i;
    for (i = neighbourlist.begin(); i != neighbourlist.end(); i++)
	neighbours.append(XrlAtom(*i));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_get_neighbour_info(const uint32_t& nid,
					       string& address,
					       string& interface,
					       string& state,
					       IPv4& rid,
					       uint32_t& priority,
					       uint32_t& deadtime,
					       IPv4& area,
					       uint32_t& opt,
					       IPv4& dr,
					       IPv4& bdr,
					       uint32_t& up,
					       uint32_t& adjacent)
{
    NeighbourInfo ninfo;

    if (!_ospf_ipv6.get_neighbour_info(nid, ninfo))
	return XrlCmdError::COMMAND_FAILED("Unable to get neighbour info");

#define copy_ninfo(var) 	var = ninfo._ ## var
    copy_ninfo(address);
    copy_ninfo(interface);
    copy_ninfo(state);
    copy_ninfo(rid);
    copy_ninfo(priority);
    copy_ninfo(deadtime);
    copy_ninfo(area);
    copy_ninfo(opt);
    copy_ninfo(dr);
    copy_ninfo(bdr);
    copy_ninfo(up);
    copy_ninfo(adjacent);
#undef copy_ninfo

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_clear_database()
{
    if (!_ospf_ipv6.clear_database())
	return XrlCmdError::COMMAND_FAILED("Unable clear database");

   return XrlCmdError::OKAY();
}
