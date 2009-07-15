// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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
#define PROFILE_UTILS_REQUIRED

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/status_codes.h"

#include "libxipc/xrl_std_router.hh"

#include "xrl/interfaces/profile_client_xif.hh"

#include "xrl_target.hh"
#include "rt_tab_register.hh"
#include "rib_manager.hh"
#include "vifmanager.hh"
#include "profile_vars.hh"

XrlCmdError
XrlRibTarget::common_0_1_get_target_name(string& name)
{
    name = XrlRibTargetBase::name();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::common_0_1_get_version(string& v)
{
    v = string(version());
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::common_0_1_get_status(
    // Output values,
    uint32_t& status,
    string&	reason)
{
    status = _rib_manager->status(reason);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::common_0_1_shutdown()
{
    _rib_manager->stop();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_enable_rib()
{
    _rib_manager->enable();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_disable_rib()
{
    _rib_manager->disable();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_start_rib()
{
    if (_rib_manager->start() != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED("Failed to start rib manager");
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_stop_rib()
{
    if (_rib_manager->stop() != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED("Failed to stop rib manager");
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_make_errors_fatal()
{
    _rib_manager->make_errors_fatal();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_get_registered_protocols(
    // Input values,
    const bool&	ipv4,
    const bool&	ipv6,
    const bool&	unicast,
    const bool&	multicast,
    // Output values,
    XrlAtomList&	ipv4_unicast_protocols,
    XrlAtomList&	ipv6_unicast_protocols,
    XrlAtomList&	ipv4_multicast_protocols,
    XrlAtomList&	ipv6_multicast_protocols)
{
    list<string> names;
    list<string>::iterator iter;

    if (ipv4) {
	if (unicast) {
	    names = _urib4.registered_protocol_names();
	    for (iter = names.begin(); iter != names.end(); ++iter)
		ipv4_unicast_protocols.append(XrlAtom(*iter));
	}
	if (multicast) {
	    names = _mrib4.registered_protocol_names();
	    for (iter = names.begin(); iter != names.end(); ++iter)
		ipv4_multicast_protocols.append(XrlAtom(*iter));
	}
    }
    if (ipv6) {
	if (unicast) {
	    names = _urib6.registered_protocol_names();
	    for (iter = names.begin(); iter != names.end(); ++iter)
		ipv6_unicast_protocols.append(XrlAtom(*iter));
	}
	if (multicast) {
	    names = _mrib6.registered_protocol_names();
	    for (iter = names.begin(); iter != names.end(); ++iter)
		ipv6_multicast_protocols.append(XrlAtom(*iter));
	}
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_add_igp_table4(const string&	protocol,
				     const string&	target_class,
				     const string&	target_instance,
				     const bool&	unicast,
				     const bool&	multicast)
{
    if (unicast &&
	_urib4.add_igp_table(protocol, target_class, target_instance)
	!= XORP_OK) {
	string err = c_format("Could not add unicast IPv4 igp table \"%s\"",
			      protocol.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (multicast &&
	_mrib4.add_igp_table(protocol, target_class, target_instance)
	!= XORP_OK) {
	string err = c_format("Could not add multicast IPv4 igp table \"%s\"",
			      protocol.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_add_igp_table6(const string&	protocol,
				     const string&	target_class,
				     const string&	target_instance,
				     const bool&	unicast,
				     const bool&	multicast)
{
    if (unicast &&
	_urib6.add_igp_table(protocol, target_class, target_instance)
	!= XORP_OK) {
	string err = c_format("Could not add unicast IPv6 igp table \"%s\"",
			      protocol.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (multicast &&
	_mrib6.add_igp_table(protocol, target_class, target_instance)
	!= XORP_OK) {
	string err = c_format("Could not add multicast IPv6 igp table \"%s\"",
			      protocol.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_delete_igp_table4(const string&	protocol,
				     const string&	target_class,
					const string&	target_instance,
					const bool&	unicast,
					const bool&	multicast)
{
    if (unicast &&
	_urib4.delete_igp_table(protocol, target_class, target_instance)
	!= XORP_OK) {
	string err = c_format("Could not delete unicast IPv4 igp table \"%s\"",
			      protocol.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (multicast &&
	_mrib4.delete_igp_table(protocol, target_class, target_instance)
	!= XORP_OK) {
	string err = c_format("Could not delete multicast IPv4 igp table "
			      "\"%s\"", protocol.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_delete_igp_table6(const string&	protocol,
					const string&	target_class,
					const string&	target_instance,
					const bool&	unicast,
					const bool&	multicast)
{
    if (unicast &&
	_urib6.delete_igp_table(protocol, target_class, target_instance)
	!= XORP_OK) {
	string err = c_format("Could not delete unicast IPv6 igp table \"%s\"",
			      protocol.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (multicast &&
	_mrib6.delete_igp_table(protocol, target_class, target_instance)
	!= XORP_OK) {
	string err = c_format("Could not delete multicast IPv6 igp table \"%s\"",
			      protocol.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_add_egp_table4(const string&	protocol,
				     const string&	target_class,
				     const string&	target_instance,
				     const bool&	unicast,
				     const bool&	multicast)
{
    if (unicast &&
	_urib4.add_egp_table(protocol, target_class, target_instance)
	!= XORP_OK) {
	string err = c_format("Could not add unicast IPv4 egp table \"%s\"",
			      protocol.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (multicast &&
	_mrib4.add_egp_table(protocol, target_class, target_instance)
	!= XORP_OK) {
	string err = c_format("Could not add multicast IPv4 egp table \"%s\"",
			      protocol.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_add_egp_table6(const string&	protocol,
				     const string&	target_class,
				     const string&	target_instance,
				     const bool&	unicast,
				     const bool&	multicast)
{
    if (unicast &&
	_urib6.add_egp_table(protocol, target_class, target_instance)
	!= XORP_OK) {
	string err = c_format("Could not add unicast IPv6 egp table \"%s\"",
			      protocol.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (multicast &&
	_mrib6.add_egp_table(protocol, target_class, target_instance)
	!= XORP_OK) {
	string err = c_format("Could not add multicast IPv6 egp table \"%s\"",
			      protocol.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_delete_egp_table4(const string&	protocol,
					const string&	target_class,
					const string&	target_instance,
					const bool&	unicast,
					const bool&	multicast)
{
    if (unicast &&
	_urib4.delete_egp_table(protocol, target_class, target_instance)
	!= XORP_OK) {
	string err = c_format("Could not delete unicast IPv4 egp table \"%s\"",
			      protocol.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (multicast &&
	_mrib4.delete_egp_table(protocol, target_class, target_instance)
	!= XORP_OK) {
	string err = c_format("Could not delete multicast IPv4 egp table "
			      "\"%s\"", protocol.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_delete_egp_table6(const string&	protocol,
					const string&	target_class,
					const string&	target_instance,
					const bool&	unicast,
					const bool&	multicast)
{
    if (unicast &&
	_urib6.delete_egp_table(protocol, target_class, target_instance)
	!= XORP_OK) {
	string err = c_format("Could not delete unicast IPv6 egp table \"%s\"",
			      protocol.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (multicast &&
	_mrib6.delete_egp_table(protocol, target_class, target_instance)
	!= XORP_OK) {
	string err = c_format("Could not delete multicast IPv6 egp table \"%s\"",
			      protocol.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_add_route4(const string&	protocol,
				 const bool&	unicast,
				 const bool&	multicast,
				 const IPv4Net&	network,
				 const IPv4&	nexthop,
				 const uint32_t& metric,
				 const XrlAtomList& policytags)
{
    debug_msg("add_route4 protocol: %s unicast: %s multicast: %s "
	      "network %s nexthop %s metric %u\n",
	      protocol.c_str(),
	      bool_c_str(unicast),
	      bool_c_str(multicast),
	      network.str().c_str(),
	      nexthop.str().c_str(),
	      XORP_UINT_CAST(metric));
 
    if (_rib_manager->profile().enabled(profile_route_ribin)) {
	_rib_manager->profile().log(profile_route_ribin,
				    c_format("add %s %s%s %s %s %u",
					     protocol.c_str(),
					     unicast ? "u" : "",
					     multicast ? "m" : "",
					     network.str().c_str(),
					     nexthop.str().c_str(),
					     XORP_UINT_CAST(metric)));
    }
    
    if (unicast &&
	_urib4.add_route(protocol, network, nexthop, "", "", metric, policytags)
	!= XORP_OK) {
	string err = c_format("Could not add IPv4 route "
			      "net %s, nexthop: %s to unicast RIB",
			      network.str().c_str(), nexthop.str().c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (multicast &&
	_mrib4.add_route(protocol, network, nexthop, "", "", metric, policytags)
	!= XORP_OK) {
	string err = c_format("Could not add IPv4 route "
			      "net %s, nexthop: %s to multicast RIB",
			      network.str().c_str(), nexthop.str().c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_add_route6(const string&	protocol,
				 const bool&	unicast,
				 const bool&	multicast,
				 const IPv6Net&	network,
				 const IPv6&	nexthop,
				 const uint32_t& metric,
				 const XrlAtomList& policytags)
{
    debug_msg("add_route6 protocol: %s unicast: %s multicast: %s "
	      "network %s nexthop %s metric %u\n",
	      protocol.c_str(),
	      bool_c_str(unicast),
	      bool_c_str(multicast),
	      network.str().c_str(),
	      nexthop.str().c_str(),
	      XORP_UINT_CAST(metric));

    if (_rib_manager->profile().enabled(profile_route_ribin)) {
	_rib_manager->profile().log(profile_route_ribin,
				    c_format("add %s %s%s %s %s %u",
					     protocol.c_str(),
					     unicast ? "u" : "",
					     multicast ? "m" : "",
					     network.str().c_str(),
					     nexthop.str().c_str(),
					     XORP_UINT_CAST(metric)));
    }

    if (unicast &&
	_urib6.add_route(protocol, network, nexthop, "", "", metric,
			 policytags)
	!= XORP_OK) {
	string err = c_format("Could not add IPv6 route "
			      "net %s, nexthop: %s to unicast RIB",
			      network.str().c_str(), nexthop.str().c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (multicast &&
	_mrib6.add_route(protocol, network, nexthop, "", "", metric,
			 policytags)
	!= XORP_OK) {
	string err = c_format("Could not add IPv6 route "
			      "net %s, nexthop: %s to multicast RIB",
			      network.str().c_str(), nexthop.str().c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_replace_route4(const string&	protocol,
				     const bool&	unicast,
				     const bool&	multicast,
				     const IPv4Net&	network,
				     const IPv4&	nexthop,
				     const uint32_t&	metric,
				     const XrlAtomList& policytags)
{
    debug_msg("replace_route4 protocol: %s unicast: %s multicast: %s "
	      "network %s nexthop %s metric %u\n",
	      protocol.c_str(),
	      bool_c_str(unicast),
	      bool_c_str(multicast),
	      network.str().c_str(),
	      nexthop.str().c_str(),
	      XORP_UINT_CAST(metric));

    if (_rib_manager->profile().enabled(profile_route_ribin)) {
	_rib_manager->profile().log(profile_route_ribin,
				    c_format("replace %s %s%s %s %s %u",
					     protocol.c_str(),
					     unicast ? "u" : "",
					     multicast ? "m" : "",
					     network.str().c_str(),
					     nexthop.str().c_str(),
					     XORP_UINT_CAST(metric)));
    }

    if (unicast &&
	_urib4.replace_route(protocol, network, nexthop, "", "",
					metric, policytags)
	!= XORP_OK) {
	string err = "Could not replace IPv4 route in unicast RIB";
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (multicast &&
	_mrib4.replace_route(protocol, network, nexthop, "", "",
					metric, policytags)
	!= XORP_OK) {
	string err = "Could not replace IPv4 route in multicast RIB";
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_replace_route6(const string&	protocol,
				     const bool&	unicast,
				     const bool&	multicast,
				     const IPv6Net&	network,
				     const IPv6&	nexthop,
				     const uint32_t&	metric,
				     const XrlAtomList& policytags)
{
    debug_msg("replace_route6 protocol: %s unicast: %s multicast: %s "
	      "network %s nexthop %s metric %u\n",
	      protocol.c_str(),
	      bool_c_str(unicast),
	      bool_c_str(multicast),
	      network.str().c_str(),
	      nexthop.str().c_str(),
	      XORP_UINT_CAST(metric));

    if (_rib_manager->profile().enabled(profile_route_ribin)) {
	_rib_manager->profile().log(profile_route_ribin,
				    c_format("replace %s %s%s %s %s %u",
					     protocol.c_str(),
					     unicast ? "u" : "",
					     multicast ? "m" : "",
					     network.str().c_str(),
					     nexthop.str().c_str(),
					     XORP_UINT_CAST(metric)));
    }

    if (unicast &&
	_urib6.replace_route(protocol, network, nexthop, "", "", metric,
			     policytags)
	!= XORP_OK) {
	string err = "Could not replace IPv6 route in unicast RIB";
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (multicast &&
	_mrib6.replace_route(protocol, network, nexthop, "", "", metric,
			     policytags)
	!= XORP_OK) {
	string err = "Could not add IPv6 route in multicast RIB";
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_delete_route4(const string&	protocol,
				    const bool&		unicast,
				    const bool&		multicast,
				    const IPv4Net&	network)
{
    debug_msg("delete_route4 protocol: %s unicast: %s multicast: %s "
	      "network %s\n",
	      protocol.c_str(),
	      bool_c_str(unicast),
	      bool_c_str(multicast),
	      network.str().c_str());

    if (_rib_manager->profile().enabled(profile_route_ribin)) {
	_rib_manager->profile().log(profile_route_ribin,
				    c_format("delete %s %s%s %s",
					     protocol.c_str(),
					     unicast ? "u" : "",
					     multicast ? "m" : "",
					     network.str().c_str()));
    }

    if (unicast && _urib4.delete_route(protocol, network) != XORP_OK) {
	string err = "Could not delete IPv4 route from unicast RIB";
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (multicast && _mrib4.delete_route(protocol, network) != XORP_OK) {
	string err = "Could not delete IPv4 route from multicast RIB";
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_delete_route6(const string&	protocol,
				    const bool&		unicast,
				    const bool&		multicast,
				    const IPv6Net&	network)
{
    debug_msg("delete_route6 protocol: %s unicast: %s multicast: %s "
	      "network %s\n",
	      protocol.c_str(),
	      bool_c_str(unicast),
	      bool_c_str(multicast),
	      network.str().c_str());

    if (_rib_manager->profile().enabled(profile_route_ribin)) {
	_rib_manager->profile().log(profile_route_ribin,
				    c_format("delete %s %s%s %s",
					     protocol.c_str(),
					     unicast ? "u" : "",
					     multicast ? "m" : "",
					     network.str().c_str()));
    }

    if (unicast && _urib6.delete_route(protocol, network) != XORP_OK) {
	string err = "Could not delete IPv6 route from unicast RIB";
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (multicast && _mrib6.delete_route(protocol, network) != XORP_OK) {
	string err = "Could not delete IPv6 route from multicast RIB";
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_add_interface_route4(const string&	protocol,
					   const bool&		unicast,
					   const bool&		multicast,
					   const IPv4Net&	network,
					   const IPv4&		nexthop,
					   const string&	ifname,
					   const string&	vifname,
					   const uint32_t&	metric,
					   const XrlAtomList&	policytags)
{
    debug_msg("add_interface_route4 protocol: %s unicast: %s multicast: %s "
	      "network %s nexthop %s ifname %s vifname %s metric %u\n",
	      protocol.c_str(),
	      bool_c_str(unicast),
	      bool_c_str(multicast),
	      network.str().c_str(),
	      nexthop.str().c_str(),
	      ifname.c_str(),
	      vifname.c_str(),
	      XORP_UINT_CAST(metric));

    if (_rib_manager->profile().enabled(profile_route_ribin)) {
	_rib_manager->profile().log(profile_route_ribin,
				    c_format("add %s %s%s %s %s %s/%s %u",
					     protocol.c_str(),
					     unicast ? "u" : "",
					     multicast ? "m" : "",
					     network.str().c_str(),
					     nexthop.str().c_str(),
					     ifname.c_str(),
					     vifname.c_str(),
					     XORP_UINT_CAST(metric)));
    }

    if (unicast &&
	_urib4.add_route(protocol, network, nexthop, ifname, vifname, metric,
			 policytags)
	!= XORP_OK) {
	string err = "Could not add IPv4 interface route to unicast RIB";
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (multicast &&
	_mrib4.add_route(protocol, network, nexthop, ifname, vifname, metric,
			 policytags)
	!= XORP_OK) {
	string err = "Could not add IPv4 interface route to multicast RIB";
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_add_interface_route6(const string&	protocol,
					   const bool&		unicast,
					   const bool&		multicast,
					   const IPv6Net&	network,
					   const IPv6&		nexthop,
					   const string&	ifname,
					   const string&	vifname,
					   const uint32_t&	metric,
					   const XrlAtomList&	policytags)
{
    debug_msg("add_interface_route6 protocol: %s unicast: %s multicast: %s "
	      "network %s nexthop %s ifname %s vifname %s metric %u\n",
	      protocol.c_str(),
	      bool_c_str(unicast),
	      bool_c_str(multicast),
	      network.str().c_str(),
	      nexthop.str().c_str(),
	      ifname.c_str(),
	      vifname.c_str(),
	      XORP_UINT_CAST(metric));

    if (_rib_manager->profile().enabled(profile_route_ribin)) {
	_rib_manager->profile().log(profile_route_ribin,
				    c_format("add %s %s%s %s %s %s/%s %u",
					     protocol.c_str(),
					     unicast ? "u" : "",
					     multicast ? "m" : "",
					     network.str().c_str(),
					     nexthop.str().c_str(),
					     ifname.c_str(),
					     vifname.c_str(),
					     XORP_UINT_CAST(metric)));
    }

    if (unicast &&
	_urib6.add_route(protocol, network, nexthop, ifname, vifname,
					metric, policytags)
	!= XORP_OK) {
	string err = "Could not add IPv6 interface route to unicast RIB";
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (multicast &&
	_mrib6.add_route(protocol, network, nexthop, ifname, vifname, metric,
					policytags)
	!= XORP_OK) {
	string err = "Could not add IPv6 interface route to multicast RIB";
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_replace_interface_route4(const string&	    protocol,
					       const bool&	    unicast,
					       const bool&	    multicast,
					       const IPv4Net&	    network,
					       const IPv4&	    nexthop,
					       const string&	    ifname,
					       const string&	    vifname,
					       const uint32_t&	    metric,
					       const XrlAtomList&   policytags)
{
    debug_msg("replace_interface_route4 protocol: %s unicast: %s multicast: %s "
	      "network %s nexthop %s ifname %s vifname %s metric %u\n",
	      protocol.c_str(),
	      bool_c_str(unicast),
	      bool_c_str(multicast),
	      network.str().c_str(),
	      nexthop.str().c_str(),
	      ifname.c_str(),
	      vifname.c_str(),
	      XORP_UINT_CAST(metric));

    if (_rib_manager->profile().enabled(profile_route_ribin)) {
	_rib_manager->profile().log(profile_route_ribin,
				    c_format("replace %s %s%s %s %s %s/%s %u",
					     protocol.c_str(),
					     unicast ? "u" : "",
					     multicast ? "m" : "",
					     network.str().c_str(),
					     nexthop.str().c_str(),
					     ifname.c_str(),
					     vifname.c_str(),
					     XORP_UINT_CAST(metric)));
    }

    if (unicast &&
	_urib4.replace_route(protocol, network, nexthop, ifname, vifname,
			     metric, policytags)
	!= XORP_OK) {
	string err = "Could not replace IPv4 interface route in unicast RIB";
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (multicast &&
	_mrib4.replace_route(protocol, network, nexthop, ifname, vifname,
			     metric, policytags)
	!= XORP_OK) {
	string err = "Could not replace IPv4 interface route in multicast RIB";
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_replace_interface_route6(const string&	    protocol,
					       const bool&	    unicast,
					       const bool&	    multicast,
					       const IPv6Net&	    network,
					       const IPv6&	    nexthop,
					       const string&	    ifname,
					       const string&	    vifname,
					       const uint32_t&	    metric,
					       const XrlAtomList&   policytags)
{
    debug_msg("replace_interface_route6 protocol: %s unicast: %s multicast: %s "
	      "network %s nexthop %s ifname %s vifname %s metric %u\n",
	      protocol.c_str(),
	      bool_c_str(unicast),
	      bool_c_str(multicast),
	      network.str().c_str(),
	      nexthop.str().c_str(),
	      ifname.c_str(),
	      vifname.c_str(),
	      XORP_UINT_CAST(metric));

    if (_rib_manager->profile().enabled(profile_route_ribin)) {
	_rib_manager->profile().log(profile_route_ribin,
				    c_format("replace %s %s%s %s %s %s/%s %u",
					     protocol.c_str(),
					     unicast ? "u" : "",
					     multicast ? "m" : "",
					     network.str().c_str(),
					     nexthop.str().c_str(),
					     ifname.c_str(),
					     vifname.c_str(),
					     XORP_UINT_CAST(metric)));
    }

    if (unicast &&
	_urib6.replace_route(protocol, network, nexthop, ifname, vifname,
			     metric, policytags)
	!= XORP_OK) {
	string err = "Could not replace IPv6 interface route in unicast RIB";
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (multicast &&
	_mrib6.replace_route(protocol, network, nexthop, ifname, vifname,
			     metric, policytags)
	!= XORP_OK) {
	string err = "Could not replace IPv6 interface route in multicast RIB";
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_lookup_route_by_dest4(
    // Input values,
    const IPv4&	addr,
    const bool&	unicast,
    const bool&	multicast,
    // Output values,
    IPv4& nexthop)
{
    // if unicast and multicast then fail, can only look one place at time
    if (unicast == multicast) {
	nexthop = IPv4::ZERO();
    } else if (unicast) {
	nexthop = _urib4.lookup_route(addr);
    } else if (multicast) {
	nexthop = _mrib4.lookup_route(addr);
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_lookup_route_by_dest6(
    // Input values,
    const IPv6&	addr,
    const bool&	unicast,
    const bool&	multicast,
    // Output values,
    IPv6& nexthop)
{
    // Must look in exactly one RIB
    if (unicast == multicast) {
	nexthop = IPv6::ZERO();
    } else if (unicast) {
	nexthop = _urib6.lookup_route(addr);
    } else if (multicast) {
	nexthop = _mrib6.lookup_route(addr);
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_new_vif(const string& name)
{
    //
    // One vif per RIB or one shared VifStore ? Latter as no guarantee that
    // all vifs have valid IPv4/IPv6/Unicast/Multicast meaning
    //

    Vif v(name);
    //
    // XXX: this is an XRL interface only for testing purpose, and
    // eventually this interface should go away in the future.
    // Hence, for simplicity so we automatically assign the vif flags
    // here with some values that should be appropriate for the
    // limited testing purpose.
    //
    v.set_p2p(false);
    v.set_loopback(false);
    v.set_multicast_capable(true);
    v.set_broadcast_capable(true);
    v.set_underlying_vif_up(true);
    v.set_mtu(1500);

    // XXX probably want something more selective (eg rib selector)
    if (_urib4.new_vif(name, v) != XORP_OK) {
	string err = c_format("Failed to add vif \"%s\" to unicast IPv4 rib",
			      name.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (_mrib4.new_vif(name, v) != XORP_OK) {
	string err = c_format("Failed to add vif \"%s\" to multicast IPv4 rib",
			      name.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (_urib6.new_vif(name, v) != XORP_OK) {
	string err = c_format("Failed to add vif \"%s\" to unicast IPv6 rib",
			      name.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (_mrib6.new_vif(name, v) != XORP_OK) {
	string err = c_format("Failed to add vif \"%s\" to multicast IPv6 rib",
			      name.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_add_vif_addr4(const string&	name,
				    const IPv4&		addr,
				    const IPv4Net&	subnet)
{
    if (_urib4.add_vif_address(name, addr, subnet, IPv4::ZERO(), IPv4::ZERO())
	!= XORP_OK) {
	string err = "Failed to add IPv4 Vif address to unicast RIB";
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (_mrib4.add_vif_address(name, addr, subnet, IPv4::ZERO(), IPv4::ZERO())
	!= XORP_OK) {
	string err = "Failed to add IPv4 Vif address to multicast RIB";
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_add_vif_addr6(const string&	name,
				    const IPv6&		addr,
				    const IPv6Net&	subnet)
{
    if (_urib6.add_vif_address(name, addr, subnet, IPv6::ZERO(), IPv6::ZERO())
	!= XORP_OK) {
	string err = "Failed to add IPv6 Vif address to unicast RIB";
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (_mrib6.add_vif_address(name, addr, subnet, IPv6::ZERO(), IPv6::ZERO())
	!= XORP_OK) {
	string err = "Failed to add IPv6 Vif address to multicast RIB";
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_redist_enable4(const string&	target_name,
				     const string&	from,
				     const bool&	ucast,
				     const bool&	mcast,
				     const IPv4Net&	network_prefix,
				     const string&	cookie)
{
    if (_rib_manager->add_redist_xrl_output4(target_name, from, ucast, mcast,
					     network_prefix, cookie, false)
	!= XORP_OK) {
	string err = c_format("Failed to enable route redistribution from "
			      "protocol \"%s\" to XRL target \"%s\"",
			      from.c_str(), target_name.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_redist_enable6(const string&	target_name,
				     const string&	from,
				     const bool&	ucast,
				     const bool&	mcast,
				     const IPv6Net&	network_prefix,
				     const string&	cookie)
{
    if (_rib_manager->add_redist_xrl_output6(target_name, from, ucast, mcast,
					     network_prefix, cookie, false)
	!= XORP_OK) {
	string err = c_format("Failed to enable route redistribution from "
			      "protocol \"%s\" to XRL target \"%s\"",
			      from.c_str(), target_name.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_redist_disable4(const string&	target_name,
				      const string&	from,
				      const bool&	ucast,
				      const bool&	mcast,
				      const string&	cookie)
{
    if (_rib_manager->delete_redist_xrl_output4(target_name, from,
						ucast, mcast,
						cookie, false) != XORP_OK) {
	string err = c_format("Failed to disable route redistribution from "
			      "protocol \"%s\" to XRL target \"%s\"",
			      from.c_str(), target_name.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_redist_disable6(const string&	target_name,
				      const string&	from,
				      const bool&	ucast,
				      const bool&	mcast,
				      const string&	cookie)
{
    if (_rib_manager->delete_redist_xrl_output6(target_name, from,
						ucast, mcast,
						cookie, false) != XORP_OK) {
	string err = c_format("Failed to disable route redistribution from "
			      "protocol \"%s\" to XRL target \"%s\"",
			      from.c_str(), target_name.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_redist_transaction_enable4(const string&	target_name,
						 const string&	from,
						 const bool&	ucast,
						 const bool&	mcast,
						 const IPv4Net&	network_prefix,
						 const string&	cookie)
{
    if (_rib_manager->add_redist_xrl_output4(target_name, from, ucast, mcast,
					     network_prefix, cookie, true)
	!= XORP_OK) {
	string err = c_format("Failed to enable transaction-based "
			      "route redistribution from "
			      "protocol \"%s\" to XRL target \"%s\"",
			      from.c_str(), target_name.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_redist_transaction_enable6(const string&	target_name,
						 const string&	from,
						 const bool&	ucast,
						 const bool&	mcast,
						 const IPv6Net&	network_prefix,
						 const string&	cookie)
{
    if (_rib_manager->add_redist_xrl_output6(target_name, from, ucast, mcast,
					     network_prefix, cookie, true)
	!= XORP_OK) {
	string err = c_format("Failed to enable transaction-based "
			      "route redistribution from "
			      "protocol \"%s\" to XRL target \"%s\"",
			      from.c_str(), target_name.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_redist_transaction_disable4(const string&	target_name,
						  const string&	from,
						  const bool&	ucast,
						  const bool&	mcast,
						  const string&	cookie)
{
    if (_rib_manager->delete_redist_xrl_output4(target_name, from,
						ucast, mcast,
						cookie, true) != XORP_OK) {
	string err = c_format("Failed to disable transaction-based "
			      "route redistribution from "
			      "protocol \"%s\" to XRL target \"%s\"",
			      from.c_str(), target_name.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_redist_transaction_disable6(const string&	target_name,
						  const string&	from,
						  const bool&	ucast,
						  const bool&	mcast,
						  const string&	cookie)
{
    if (_rib_manager->delete_redist_xrl_output6(target_name, from,
						ucast, mcast,
						cookie, true) != XORP_OK) {
	string err = c_format("Failed to disable transaction-based "
			      "route redistribution from "
			      "protocol \"%s\" to XRL target \"%s\"",
			      from.c_str(), target_name.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_register_interest4(// Input values,
					 const string& target,
					 const IPv4& addr,
					 // Output values,
					 bool& resolves,
					 IPv4&	base_addr,
					 uint32_t& prefix_len,
					 uint32_t& real_prefix_len,
					 IPv4&	nexthop,
					 uint32_t& metric)
{
    debug_msg("register_interest4 target = %s addr = %s\n",
	      target.c_str(), addr.str().c_str());

    RouteRegister<IPv4>* rt_reg = _urib4.route_register(addr, target);
    if (rt_reg->route() == NULL) {
	base_addr = rt_reg->valid_subnet().masked_addr();
	prefix_len = real_prefix_len = rt_reg->valid_subnet().prefix_len();
	resolves = false;
	debug_msg("#### XRL -> REGISTER INTEREST UNRESOLVABLE %s\n",
		  rt_reg->valid_subnet().str().c_str());
    } else {
	metric = rt_reg->route()->metric();
	base_addr = rt_reg->valid_subnet().masked_addr();
	prefix_len = real_prefix_len = rt_reg->valid_subnet().prefix_len();
	NextHop *nh = rt_reg->route()->nexthop();
	switch (nh->type()) {
	case GENERIC_NEXTHOP:
	    // this shouldn't be possible
	    XLOG_UNREACHABLE();
	case PEER_NEXTHOP:
	case ENCAPS_NEXTHOP:
	    resolves = true;
	    nexthop = ((IPNextHop<IPv4>*)nh)->addr();
	    real_prefix_len = rt_reg->route()->prefix_len();
	    break;
	case EXTERNAL_NEXTHOP:
	case DISCARD_NEXTHOP:
	case UNREACHABLE_NEXTHOP:
	    resolves = false;
	    break;
	}
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_deregister_interest4(// Input values,
					   const string& target,
					   const IPv4& addr,
					   const uint32_t& prefix_len)
{
    if (_urib4.route_deregister(IPv4Net(addr, prefix_len), target)
	!= XORP_OK) {
	string error_msg = c_format("Failed to deregister target %s for "
				    "prefix %s/%u",
				    target.c_str(),
				    addr.str().c_str(),
				    XORP_UINT_CAST(prefix_len));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_register_interest6(// Input values,
					 const string& target,
					 const IPv6& addr,
					 // Output values,
					 bool& resolves,
					 IPv6& base_addr,
					 uint32_t& prefix_len,
					 uint32_t& real_prefix_len,
					 IPv6&	nexthop,
					 uint32_t& metric)
{
    debug_msg("register_interest6 target = %s addr = %s\n",
	      target.c_str(), addr.str().c_str());

    RouteRegister<IPv6>* rt_reg = _urib6.route_register(addr, target);
    if (rt_reg->route() == NULL) {
	base_addr = rt_reg->valid_subnet().masked_addr();
	prefix_len = real_prefix_len = rt_reg->valid_subnet().prefix_len();
	resolves = false;
	debug_msg("#### XRL -> REGISTER INTEREST UNRESOLVABLE %s\n",
		  rt_reg->valid_subnet().str().c_str());
    } else {
	metric = rt_reg->route()->metric();
	base_addr = rt_reg->valid_subnet().masked_addr();
	prefix_len = real_prefix_len = rt_reg->valid_subnet().prefix_len();
	NextHop *nh = rt_reg->route()->nexthop();
	switch (nh->type()) {
	case GENERIC_NEXTHOP:
	    // this shouldn't be possible
	    XLOG_UNREACHABLE();
	case PEER_NEXTHOP:
	case ENCAPS_NEXTHOP:
	    resolves = true;
	    nexthop = ((IPNextHop<IPv6>*)nh)->addr();
	    real_prefix_len = rt_reg->route()->prefix_len();
	    break;
	case EXTERNAL_NEXTHOP:
	case DISCARD_NEXTHOP:
	case UNREACHABLE_NEXTHOP:
	    resolves = false;
	    break;
	}
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_deregister_interest6(// Input values,
					   const string& target,
					   const IPv6&	addr,
					   const uint32_t& prefix_len)
{
    if (_urib6.route_deregister(IPv6Net(addr, prefix_len), target)
	!= XORP_OK) {
	string error_msg = c_format("Failed to deregister target %s for "
				    "prefix %s/%u",
				    target.c_str(),
				    addr.str().c_str(),
				    XORP_UINT_CAST(prefix_len));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_get_protocol_admin_distances(
    // Input values,
    const bool&     ipv4,
    const bool&     unicast,
    // Output values,
    XrlAtomList&    protocols,
    XrlAtomList&    admin_distances)
{

    if (ipv4 && unicast) {
	// ipv4 unicast
	map<string, uint32_t>& rad = _urib4.get_protocol_admin_distances();
	map<string, uint32_t>::iterator iter;
	for (iter = rad.begin(); iter != rad.end(); ++iter) {
	    protocols.append(XrlAtom(iter->first));
	    admin_distances.append(XrlAtom(iter->second));
	}
    } else if (ipv4 && !unicast) {
	// ipv4 multicast
	map<string, uint32_t>& rad = _mrib4.get_protocol_admin_distances();
	map<string, uint32_t>::iterator iter;
	for (iter = rad.begin(); iter != rad.end(); ++iter) {
	    protocols.append(XrlAtom(iter->first));
	    admin_distances.append(XrlAtom(iter->second));
	}
    } else if (!ipv4 && unicast) {
	// ipv6 unicast
	map<string, uint32_t>& rad = _urib6.get_protocol_admin_distances();
	map<string, uint32_t>::iterator iter;
	for (iter = rad.begin(); iter != rad.end(); ++iter) {
	    protocols.append(XrlAtom(iter->first));
	    admin_distances.append(XrlAtom(iter->second));
	}
    } else if (!ipv4 && !unicast) {
	// ipv6 multicast
	map<string, uint32_t>& rad = _mrib6.get_protocol_admin_distances();
	map<string, uint32_t>::iterator iter;
	for (iter = rad.begin(); iter != rad.end(); ++iter) {
	    protocols.append(XrlAtom(iter->first));
	    admin_distances.append(XrlAtom(iter->second));
	}
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_get_protocol_admin_distance(
    // Input values,
    const string& protocol,
    const bool&	ipv4,
    const bool&	unicast,
    // Output values,
    uint32_t& admin_distance)
{

    if (ipv4 && unicast) {
	admin_distance = _urib4.get_protocol_admin_distance(protocol);
    } else if (ipv4 && !unicast) {
	admin_distance = _mrib4.get_protocol_admin_distance(protocol);
    } else if (!ipv4 && unicast) {
	admin_distance = _urib6.get_protocol_admin_distance(protocol);
    } else if (!ipv4 && !unicast) {
	admin_distance = _mrib6.get_protocol_admin_distance(protocol);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_set_protocol_admin_distance(
    // Input values,
    const string& protocol,
    const bool& ipv4,
    const bool& ipv6,
    const bool&	unicast,
    const bool&	multicast,
    const uint32_t& admin_distance)
{

    // Only the RIB may set an admin distance outside of the
    // ranges 1 to 255, as 0 is reserved for directly-connected
    // routes, and anything >= 255 will never make it into the FIB.
    if (admin_distance <= CONNECTED_ADMIN_DISTANCE ||
	admin_distance > UNKNOWN_ADMIN_DISTANCE) {
	string err = c_format("Admin distance %d out of range for %s"
			      "%s protocol \"%s\"; must be between "
			      "1 and 255 inclusive.",
			      admin_distance, "unicast", "IPv4",
			      protocol.c_str());
	return XrlCmdError::BAD_ARGS(err);
    }

    if (ipv4 && unicast &&
	_urib4.set_protocol_admin_distance(protocol, admin_distance)
	!= XORP_OK) {
	string err = c_format("Could not set admin distance for %s "
			      "%s protocol \"%s\"",
			      "IPv4", "unicast", protocol.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (ipv4 && multicast &&
	_mrib4.set_protocol_admin_distance(protocol, admin_distance)
	!= XORP_OK) {
	string err = c_format("Could not set admin distance for %s "
			      "%s protocol \"%s\"",
			      "IPv4", "multicast", protocol.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (ipv6 && unicast &&
	_urib6.set_protocol_admin_distance(protocol, admin_distance)
	!= XORP_OK) {
	string err = c_format("Could not set admin distance for %s "
			      "%s protocol \"%s\"",
			      "IPv6", "unicast", protocol.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    if (ipv6 && multicast &&
	_mrib6.set_protocol_admin_distance(protocol, admin_distance)
	!= XORP_OK) {
	string err = c_format("Could not set admin distance for %s "
			      "%s protocol \"%s\"",
			      "IPv6", "multicast", protocol.c_str());
	return XrlCmdError::COMMAND_FAILED(err);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::finder_event_observer_0_1_xrl_target_birth(
        const string&	target_class,
	const string&	target_instance)
{
    debug_msg("Target Birth: class = %s instance = %s\n",
	      target_class.c_str(), target_instance.c_str());

    UNUSED(target_class);
    UNUSED(target_instance);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::finder_event_observer_0_1_xrl_target_death(
	const string&	target_class,
	const string&	target_instance)
{
    debug_msg("Target Death: class = %s instance = %s\n",
	      target_class.c_str(), target_instance.c_str());

    _rib_manager->target_death(target_class, target_instance);
    return XrlCmdError::OKAY();
}


XrlCmdError
XrlRibTarget::policy_backend_0_1_configure(const uint32_t& filter,
					   const string&   conf)
{
    try {
	_rib_manager->configure_filter(filter, conf);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("Filter configure failed: " +
					   e.str());
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::policy_backend_0_1_reset(const uint32_t& filter)
{
    try {
	_rib_manager->reset_filter(filter);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("Filter reset failed: " +
					   e.str());
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::policy_backend_0_1_push_routes()
{
    _rib_manager->push_routes();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_insert_policy_redist_tags(const string& protocol,
						const XrlAtomList& policytags)
{
    // doubt these will ever be used
    try {
	_rib_manager->insert_policy_redist_tags(protocol, policytags);
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("Insert policy redist tags failed: "
					   + e.str());
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_reset_policy_redist_tags()
{
    // just a guard for the future.
    try {
	_rib_manager->reset_policy_redist_tags();
    } catch(const PolicyException& e) {
	return XrlCmdError::COMMAND_FAILED("Reset policy redist tags failed: " +
					   e.str());
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::profile_0_1_enable(const string& pname)
{
    debug_msg("enable profile variable %s\n", pname.c_str());

    try {
	_rib_manager->profile().enable(pname);
    } catch(PVariableUnknown& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    } catch(PVariableLocked& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::profile_0_1_disable(const string&	pname)
{
    debug_msg("disable profile variable %s\n", pname.c_str());

    try {
	_rib_manager->profile().disable(pname);
    } catch(PVariableUnknown& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlRibTarget::profile_0_1_get_entries(const string& pname,
				      const string& instance_name)
{
    debug_msg("profile variable %s instance %s\n", pname.c_str(),
	      instance_name.c_str());

    // Lock and initialize.
    try {
	_rib_manager->profile().lock_log(pname);
    } catch(PVariableUnknown& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    } catch(PVariableLocked& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    ProfileUtils::transmit_log(pname,
			       &_rib_manager->xrl_router(), instance_name,
			       &_rib_manager->profile());

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::profile_0_1_clear(const string& pname)
{
    debug_msg("clear profile variable %s\n", pname.c_str());

    try {
	_rib_manager->profile().clear(pname);
    } catch(PVariableUnknown& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    } catch(PVariableLocked& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::profile_0_1_list(string& info)
{
    debug_msg("\n");
    
    info = _rib_manager->profile().list();
    return XrlCmdError::OKAY();
}
