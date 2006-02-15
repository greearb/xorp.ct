// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/ospf/xrl_target.cc,v 1.26 2006/01/12 10:24:32 atanu Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"
#include <string>
#include <set>

#include "ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"
#include "libxipc/xrl_std_router.hh"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#include "ospf.hh"
#include "xrl_io.hh"
#include "xrl_target.hh"

static time_t
decode_time_string(const string& time_string)
{
    const char* s;
    const char* format = "%Y-%m-%d.%H:%M";
    struct tm tm;
    time_t result;

    if (time_string.empty()) {
	result = 0;
	return (result);
    }

    memset(&tm, 0, sizeof(tm));

    s = xorp_strptime(time_string.c_str(), format, &tm);
    if ((s == NULL) || (*s != '\0')) {
	result = -1;
	return (result);
    }

    result = mktime(&tm);
    return (result);
}

XrlOspfV2Target::XrlOspfV2Target(XrlRouter *r, Ospf<IPv4>& ospf,
				 XrlIO<IPv4>& io)
	: XrlOspfv2TargetBase(r), _ospf(ospf), _xrl_io(io)
{
}

XrlOspfV3Target::XrlOspfV3Target(XrlRouter *r,
				 Ospf<IPv4>& ospf_ipv4, 
				 Ospf<IPv6>& ospf_ipv6, 
				 XrlIO<IPv4>& io_ipv4,
				 XrlIO<IPv6>& io_ipv6)
    : XrlOspfv3TargetBase(r),
      _ospf_ipv4(ospf_ipv4), _ospf_ipv6(ospf_ipv6),
      _xrl_io_ipv4(io_ipv4), _xrl_io_ipv6(io_ipv6)
{
}

XrlCmdError
XrlOspfV2Target::common_0_1_get_target_name(string&  name)
{
    name = "ospfv2";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::common_0_1_get_target_name(string&  name)
{
    name = "ospfv3";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::common_0_1_get_version(string&	version)
{
    version = "0.1";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::common_0_1_get_version(string&	version)
{
    version = "0.1";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::common_0_1_get_status(uint32_t& status,
				       string& reason)
{

    status = _ospf.status(reason);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::common_0_1_get_status(uint32_t& /*status*/,
				       string& /*reason*/)
{
    XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::common_0_1_shutdown()
{
    _ospf.shutdown();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::common_0_1_shutdown()
{
    XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::raw_packet4_client_0_1_recv(
    // Input values,
    const string&	if_name,
    const string&	vif_name,
    const IPv4&		src_address,
    const IPv4&		dst_address,
    const uint32_t&	ip_protocol,
    const int32_t&	ip_ttl,
    const int32_t&	ip_tos,
    const bool&		ip_router_alert,
    const vector<uint8_t>& payload)
{
    _xrl_io.recv(if_name,
		 vif_name,
		 src_address,
		 dst_address,
		 ip_protocol,
		 ip_ttl,
		 ip_tos,
		 ip_router_alert,
		 payload);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::raw_packet4_client_0_1_recv(
    // Input values,
    const string&	if_name,
    const string&	vif_name,
    const IPv4&		src_address,
    const IPv4&		dst_address,
    const uint32_t&	ip_protocol,
    const int32_t&	ip_ttl,
    const int32_t&	ip_tos,
    const bool&		ip_router_alert,
    const vector<uint8_t>& payload)
{
    _xrl_io_ipv4.recv(if_name,
		      vif_name,
		      src_address,
		      dst_address,
		      ip_protocol,
		      ip_ttl,
		      ip_tos,
		      ip_router_alert,
		      payload);

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
    const vector<uint8_t>& payload)
{
    _xrl_io_ipv6.recv(if_name,
		      vif_name,
		      src_address,
		      dst_address,
		      ip_protocol,
		      ip_ttl,
		      ip_tos,
		      ip_router_alert,
		      payload);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::policy_backend_0_1_configure(const uint32_t& filter,
					      const string& conf)
{
    debug_msg("policy filter: %u conf: %s\n", filter, conf.c_str());

    try {
	_ospf.configure_filter(filter,conf);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("Filter configure failed: " +
					   e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::policy_backend_0_1_reset(const uint32_t& filter)
{
    debug_msg("policy filter reset: %u\n", filter);

    try {
	_ospf.reset_filter(filter);
    } catch(const PolicyException& e){ 
	return XrlCmdError::COMMAND_FAILED("Filter reset failed: " +
					   e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::policy_backend_0_1_push_routes()
{
    debug_msg("policy route push\n");

    _ospf.push_routes();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::policy_redist4_0_1_add_route4(const IPv4Net& network,
					       const bool& /*unicast*/,
					       const bool& /*multicast*/,
					       const IPv4& nexthop,
					       const uint32_t& metric,
					       const XrlAtomList& policytags)
{
    if (!_ospf.originate_route(network, nexthop, metric, policytags)) {
	return XrlCmdError::COMMAND_FAILED("Network: " + network.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::policy_redist4_0_1_delete_route4(const IPv4Net& network,
						  const bool& /*unicast*/,
						  const bool& /*multicast*/)
{
    if (!_ospf.withdraw_route(network)) {
	return XrlCmdError::COMMAND_FAILED("Network: " + network.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_set_router_id(const IPv4& id)
{
    OspfTypes::RouterID rid = ntohl(id.addr());

    _ospf.set_router_id(rid);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_set_ip_router_alert(const bool& ip_router_alert)
{
    if (!_ospf.set_ip_router_alert(ip_router_alert))
	return XrlCmdError::COMMAND_FAILED("Failed to set IP router alert");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV3Target::ospfv3_0_1_set_router_id(const uint32_t& /*id*/)
{
    XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfV2Target::ospfv2_0_1_create_area_router(const IPv4& a,
					       const string& type)
{
    bool status;
    OspfTypes::AreaType t = from_string_to_area_type(type, status);
    if (!status)
	return XrlCmdError::COMMAND_FAILED("Unrecognised type " + type);
	
    OspfTypes::AreaID area = ntohl(a.addr());
    if (!_ospf.get_peer_manager().create_area_router(area, t))
	return XrlCmdError::COMMAND_FAILED("Failed to create area " +
					   pr_id(area));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_destroy_area_router(const IPv4& a)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    if (!_ospf.get_peer_manager().destroy_area_router(area))
	return XrlCmdError::COMMAND_FAILED("Failed to destroy area " +
					   pr_id(area));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_create_peer(const string& ifname,
					const string& vifname,
					const IPv4& addr,
					const uint32_t& prefix_len,
					const uint32_t&	mtu,
					const string& type,
					const IPv4& a)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    bool status;
    OspfTypes::LinkType linktype = from_string_to_link_type(type, status);
    if (!status)
	return XrlCmdError::COMMAND_FAILED("Unrecognised type " + type);

    try {
	_ospf.get_peer_manager().create_peer(ifname, vifname, addr, prefix_len,
					     mtu, linktype, area);
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfV2Target::ospfv2_0_1_delete_peer(const string& ifname,
					const string& vifname)
{
    debug_msg("interface %s vif %s\n", ifname.c_str(), vifname.c_str());

    PeerID peerid;
    try {
	peerid = _ospf.get_peer_manager().get_peerid(ifname, vifname);
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }
    if (!_ospf.get_peer_manager().delete_peer(peerid))
	return XrlCmdError::COMMAND_FAILED("Failed to delete peer");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_set_peer_state(const string& ifname,
					   const string& vifname,
					   const bool& enable)
{
    debug_msg("interface %s vif %s enable %s\n", ifname.c_str(),
	      vifname.c_str(), pb(enable));

    PeerID peerid;
    try {
	peerid = _ospf.get_peer_manager().get_peerid(ifname, vifname);
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }
    if (!_ospf.get_peer_manager().set_state_peer(peerid, enable))
	return XrlCmdError::COMMAND_FAILED("Failed to set peer state");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_add_neighbour(const string&	ifname,
					  const string&	vifname,
					  const IPv4& addr,
					  const IPv4& neighbour_address,
					  const IPv4& neighbour_id)
{
    OspfTypes::AreaID area = ntohl(addr.addr());
    OspfTypes::RouterID rid = ntohl(neighbour_id.addr());
    debug_msg("interface %s vif %s area %s address %s id %s\n", ifname.c_str(),
	      vifname.c_str(), pr_id(area).c_str(), 
	      cstring(neighbour_address),pr_id(rid).c_str());

    PeerID peerid;
    try {
	peerid = _ospf.get_peer_manager().get_peerid(ifname, vifname);
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }
    if (!_ospf.get_peer_manager().add_neighbour(peerid, area,
						neighbour_address,
						rid))
	return XrlCmdError::COMMAND_FAILED("Failed to add neighbour " +
					   neighbour_address.str());

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfV2Target::ospfv2_0_1_remove_neighbour(const string& ifname,
					     const string& vifname,
					     const IPv4& addr,
					     const IPv4& neighbour_address,
					     const IPv4& neighbour_id)
{
    OspfTypes::AreaID area = ntohl(addr.addr());
    OspfTypes::RouterID rid = ntohl(neighbour_id.addr());
    debug_msg("interface %s vif %s area %s address %s id %s\n", ifname.c_str(),
	      vifname.c_str(), pr_id(area).c_str(), 
	      cstring(neighbour_address),pr_id(rid).c_str());

    PeerID peerid;
    try {
	peerid = _ospf.get_peer_manager().get_peerid(ifname, vifname);
    } catch(XorpException& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }
    if (!_ospf.get_peer_manager().remove_neighbour(peerid, area,
						   neighbour_address,
						   rid))
	return XrlCmdError::COMMAND_FAILED("Failed to remove neighbour" +
					   neighbour_address.str());

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_create_virtual_link(const IPv4& neighbour_id,
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

    if (!_ospf.create_virtual_link(rid))
	return XrlCmdError::COMMAND_FAILED("Failed to create virtual link");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_delete_virtual_link(const IPv4& neighbour_id)
{
    OspfTypes::RouterID rid = ntohl(neighbour_id.addr());

    if (!_ospf.delete_virtual_link(rid))
	return XrlCmdError::COMMAND_FAILED("Failed to delete virtual link");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_transit_area_virtual_link(const IPv4& neighbour_id,
						      const IPv4& transit_area)
{
    OspfTypes::RouterID rid = ntohl(neighbour_id.addr());
    OspfTypes::AreaID area = ntohl(transit_area.addr());

    if (!_ospf.transit_area_virtual_link(rid, area))
	return XrlCmdError::COMMAND_FAILED("Failed to configure transit area");

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfV2Target::ospfv2_0_1_set_interface_cost(const string& ifname,
					       const string& vifname,
					       const IPv4& a,
					       const uint32_t& cost)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("interface %s vif %s area %s cost %d\n", ifname.c_str(),
	      vifname.c_str(), pr_id(area).c_str(), cost);

    if (!_ospf.set_interface_cost(ifname, vifname, area, cost))
	return XrlCmdError::COMMAND_FAILED("Failed to set "
					   "interface cost");
    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfV2Target::ospfv2_0_1_set_retransmit_interval(const string& ifname,
						    const string& vifname,
						    const IPv4& a,
						    const uint32_t& interval)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("interface %s vif %s area %s interval %d\n", ifname.c_str(),
	      vifname.c_str(), pr_id(area).c_str(), interval);

    if (!_ospf.set_retransmit_interval(ifname, vifname, area, interval))
	return XrlCmdError::COMMAND_FAILED("Failed to set "
					   "RxmtInterval interval");

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfV2Target::ospfv2_0_1_set_inftransdelay(const string& ifname,
					      const string& vifname,
					      const IPv4& a,
					      const uint32_t& delay)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("interface %s vif %s area %s delay %d\n", ifname.c_str(),
	      vifname.c_str(), pr_id(area).c_str(), delay);

    if (!_ospf.set_inftransdelay(ifname, vifname, area, delay))
	return XrlCmdError::COMMAND_FAILED("Failed to set "
					   "inftransdelay delay");

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlOspfV2Target::ospfv2_0_1_set_router_priority(const string& ifname,
						const string& vifname,
						const IPv4& a,
						const uint32_t& priority)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("interface %s vif %s area %s priority %d\n", ifname.c_str(),
	      vifname.c_str(), pr_id(area).c_str(), priority);

    if (!_ospf.set_router_priority(ifname, vifname, area, priority))
	return XrlCmdError::COMMAND_FAILED("Failed to set priority");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_set_hello_interval(const string& ifname,
					       const string& vifname,
					       const IPv4& a,
					       const uint32_t& interval)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("interface %s vif %s area %s interval %d\n", ifname.c_str(),
	      vifname.c_str(), pr_id(area).c_str(), interval);

    if (!_ospf.set_hello_interval(ifname, vifname, area, interval))
	return XrlCmdError::COMMAND_FAILED("Failed to set hello interval");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_set_router_dead_interval(const string& ifname,
						     const string& vifname,
						     const IPv4& a,
						     const uint32_t& interval)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("interface %s vif %s area %s interval %d\n", ifname.c_str(),
	      vifname.c_str(), pr_id(area).c_str(), interval);

    if (!_ospf.set_router_dead_interval(ifname, vifname, area, interval))
	return XrlCmdError::COMMAND_FAILED("Failed to set "
					   "router dead interval");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_set_simple_authentication_key(
    // Input values,
    const string&	ifname,
    const string&	vifname,
    const IPv4&		area,
    const string&	password)
{
    string error_msg;
    OspfTypes::AreaID area_id = ntohl(area.addr());

    debug_msg("set_simple_authentication_key(): "
	      "interface %s vif %s area %s password %s\n",
	      ifname.c_str(), vifname.c_str(), pr_id(area_id).c_str(),
	      password.c_str());

    if (!_ospf.set_simple_authentication_key(ifname, vifname, area_id,
					     password, error_msg)) {
	error_msg = c_format("Failed to set simple authentication key: %s",
			     error_msg.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_delete_simple_authentication_key(
    // Input values,
    const string&	ifname,
    const string&	vifname,
    const IPv4&		area)
{
    string error_msg;
    OspfTypes::AreaID area_id = ntohl(area.addr());

    debug_msg("delete_simple_authentication_key(): "
	      "interface %s vif %s area %s\n",
	      ifname.c_str(), vifname.c_str(), pr_id(area_id).c_str());

    if (!_ospf.delete_simple_authentication_key(ifname, vifname, area_id,
						error_msg)) {
	error_msg = c_format("Failed to delete simple authentication key: %s",
			     error_msg.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_set_md5_authentication_key(
    // Input values,
    const string&	ifname,
    const string&	vifname,
    const IPv4&		area,
    const uint32_t&	key_id,
    const string&	password,
    const string&	start_time,
    const string&	end_time)
{
    string error_msg;
    uint32_t start_secs = 0;
    uint32_t end_secs = 0;
    time_t decoded_time;
    OspfTypes::AreaID area_id = ntohl(area.addr());

    debug_msg("set_md5_authentication_key(): "
	      "interface %s vif %s area %s key_id %u password %s "
	      "start_time %s end_time %s\n",
	      ifname.c_str(), vifname.c_str(), pr_id(area_id).c_str(),
	      key_id, password.c_str(), start_time.c_str(), end_time.c_str());

    //
    // Check the key ID
    //
    if (key_id > 255) {
	error_msg = c_format("Invalid key ID %u (valid range is [0, 255])",
			     XORP_UINT_CAST(key_id));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // Decode the start and end time
    //
    decoded_time = decode_time_string(start_time);
    if (decoded_time == -1) {
	error_msg = c_format("Invalid start time: %s", start_time.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    start_secs = decoded_time;
    if (end_time.empty()) {
	// XXX: if end_secs is same as start_secs, then the key never expires
	end_secs = start_secs;
    } else {
	decoded_time = decode_time_string(end_time);
	if (decoded_time == -1) {
	    error_msg = c_format("Invalid end time: %s", end_time.c_str());
	    return XrlCmdError::COMMAND_FAILED(error_msg);
	}
	end_secs = decoded_time;
    }

    if (!_ospf.set_md5_authentication_key(ifname, vifname, area_id, key_id,
					  password, start_secs, end_secs,
					  error_msg)) {
	error_msg = c_format("Failed to set MD5 authentication key: %s",
			     error_msg.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_delete_md5_authentication_key(
    // Input values,
    const string&	ifname,
    const string&	vifname,
    const IPv4&		area,
    const uint32_t&	key_id)
{
    string error_msg;
    OspfTypes::AreaID area_id = ntohl(area.addr());

    debug_msg("delete_md5_authentication_key(): "
	      "interface %s vif %s area %s key_id %u\n",
	      ifname.c_str(), vifname.c_str(), pr_id(area_id).c_str(), key_id);

    //
    // Check the key ID
    //
    if (key_id > 255) {
	error_msg = c_format("Invalid key ID %u (valid range is [0, 255])",
			     XORP_UINT_CAST(key_id));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (!_ospf.delete_md5_authentication_key(ifname, vifname, area_id, key_id,
					     error_msg)) {
	error_msg = c_format("Failed to delete MD5 authentication key: %s",
			     error_msg.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_set_passive(const string& ifname,
					const string& vifname,
					const IPv4& a,
					const bool& passive)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("interface %s vif %s area %s passive %s\n",
	      ifname.c_str(), vifname.c_str(), pr_id(area).c_str(),
	      pb(passive));

    if (!_ospf.set_passive(ifname, vifname, area, passive))
	return XrlCmdError::COMMAND_FAILED("Failed to configure make passive");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_area_range_add(const IPv4& a,
					   const IPv4Net& net,
					   const bool& advertise)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("area %s net %s advertise %s\n", pr_id(area).c_str(),
	      cstring(net), pb(advertise));

    if (!_ospf.area_range_add(area, net, advertise))
	return XrlCmdError::
	    COMMAND_FAILED(c_format("Failed to add area range "
				    "area %s net %s advertise %s\n",
				    pr_id(area).c_str(), cstring(net),
				    pb(advertise)));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_area_range_delete(const IPv4& a,
					      const IPv4Net& net)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("area %s net %s\n", pr_id(area).c_str(), cstring(net));

    if (!_ospf.area_range_delete(area, net))
	return XrlCmdError::
	    COMMAND_FAILED(c_format("Failed to delete area range "
				    "area %s net %s\n",
				    pr_id(area).c_str(), cstring(net)));


    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_area_range_change_state(const IPv4& a,
						    const IPv4Net& net,
						    const bool&	advertise)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("area %s net %s advertise %s\n", pr_id(area).c_str(),
	      cstring(net), pb(advertise));

    if (!_ospf.area_range_change_state(area, net, advertise))
	return XrlCmdError::
	    COMMAND_FAILED(c_format("Failed to change area range "
				    "area %s net %s advertise %s\n",
				    pr_id(area).c_str(), cstring(net),
				    pb(advertise)));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_trace(const string&	tvar, const bool& enable)
{
    debug_msg("trace variable %s enable %s\n", tvar.c_str(), pb(enable));

    if (tvar == "all") {
	_ospf.trace().all(enable);
    } else {
	return XrlCmdError::
	    COMMAND_FAILED(c_format("Unknown variable %s", tvar.c_str()));
    } 

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::XrlOspfV2Target::ospfv2_0_1_get_lsa(const IPv4& a,
						     const uint32_t& index,
						     bool& valid,
						     bool& toohigh,
						     bool& self,
						     vector<uint8_t>& lsa)
{
    OspfTypes::AreaID area = ntohl(a.addr());
    debug_msg("area %s index %u\n", pr_id(area).c_str(), index);

    if (!_ospf.get_lsa(area, index, valid, toohigh, self, lsa))
	return XrlCmdError::COMMAND_FAILED("Unable to get LSA");

    debug_msg("area %s index %u valid %s toohigh %s self %s\n",
	      pr_id(area).c_str(), index, pb(valid), pb(toohigh), pb(self));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_get_area_list(XrlAtomList& areas)
{
    list<OspfTypes::AreaID> arealist;

    if (!_ospf.get_area_list(arealist))
	return XrlCmdError::COMMAND_FAILED("Unable to get area list");

    list<OspfTypes::AreaID>::const_iterator i;
    for (i = arealist.begin(); i != arealist.end(); i++)
	areas.append(XrlAtom(*i));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_get_neighbour_list(XrlAtomList& neighbours)
{
    list<OspfTypes::NeighbourID> neighbourlist;

    if (!_ospf.get_neighbour_list(neighbourlist))
	return XrlCmdError::COMMAND_FAILED("Unable to get neighbour list");

    list<OspfTypes::NeighbourID>::const_iterator i;
    for (i = neighbourlist.begin(); i != neighbourlist.end(); i++)
	neighbours.append(XrlAtom(*i));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlOspfV2Target::ospfv2_0_1_get_neighbour_info(const uint32_t& nid,
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

    if (!_ospf.get_neighbour_info(nid, ninfo))
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
