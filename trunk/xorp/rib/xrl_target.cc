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

#ident "$XORP: xorp/rib/xrl_target.cc,v 1.11 2003/03/22 04:29:46 pavlin Exp $"

#include "version.h"
#include "rib_module.h"
#include "xrl_target.hh"
#include "rt_tab_register.hh"
#include "rib_manager.hh"
#include "vifmanager.hh"

#define RETURN_FAIL(x) return XrlCmdError::COMMAND_FAILED(x)

XrlCmdError
XrlRibTarget::common_0_1_get_target_name(string& name)
{
    name = XrlRibTargetBase::name();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::common_0_1_get_version(string& version)
{
    version = RIB_VERSION;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_enable_rib(
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    _rib_manager->enable();
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_disable_rib(
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    _rib_manager->disable();
    fail = false;
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_start_rib(
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    if (_rib_manager->start() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_stop_rib(
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    if (_rib_manager->stop() != XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_add_rib_client4(
    // Input values, 
    const string&	target_name, 
    const bool&		unicast, 
    const bool&		multicast, 
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    if (_rib_manager->add_rib_client(target_name, AF_INET, unicast,
				     multicast)
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_add_rib_client6(
    // Input values, 
    const string&	target_name, 
    const bool&		unicast, 
    const bool&		multicast, 
    // Output values, 
    bool&	fail, 
    string&	reason)
{
#ifndef HAVE_IPV6
    fail = true;
    reason = "IPv6 not supported";

    UNUSED(target_name);
    UNUSED(unicast);
    UNUSED(multicast);
    
    return XrlCmdError::OKAY();
    
#else
    if (_rib_manager->add_rib_client(target_name, AF_INET6, unicast,
				     multicast)
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
#endif // HAVE_IPV6
}

XrlCmdError
XrlRibTarget::rib_0_1_delete_rib_client4(
    // Input values, 
    const string&	target_name, 
    const bool&		unicast, 
    const bool&		multicast, 
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    if (_rib_manager->delete_rib_client(target_name, AF_INET, unicast,
					multicast)
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();

}

XrlCmdError
XrlRibTarget::rib_0_1_delete_rib_client6(
    // Input values, 
    const string&	target_name, 
    const bool&		unicast, 
    const bool&		multicast, 
    // Output values, 
    bool&	fail, 
    string&	reason)
{
#ifndef HAVE_IPV6
    fail = true;
    reason = "IPv6 not supported";

    UNUSED(target_name);
    UNUSED(unicast);
    UNUSED(multicast);
    
    return XrlCmdError::OKAY();
    
#else
    if (_rib_manager->delete_rib_client(target_name, AF_INET6, unicast,
					multicast)
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
#endif // HAVE_IPV6

}

XrlCmdError
XrlRibTarget::rib_0_1_enable_rib_client4(
    // Input values, 
    const string&	target_name, 
    const bool&		unicast, 
    const bool&		multicast, 
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    if (_rib_manager->enable_rib_client(target_name, AF_INET, unicast,
					multicast)
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();

}

XrlCmdError
XrlRibTarget::rib_0_1_enable_rib_client6(
    // Input values, 
    const string&	target_name, 
    const bool&		unicast, 
    const bool&		multicast, 
    // Output values, 
    bool&	fail, 
    string&	reason)
{
#ifndef HAVE_IPV6
    fail = true;
    reason = "IPv6 not supported";

    UNUSED(target_name);
    UNUSED(unicast);
    UNUSED(multicast);
    
    return XrlCmdError::OKAY();
    
#else
    if (_rib_manager->enable_rib_client(target_name, AF_INET6, unicast,
					multicast)
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
#endif // HAVE_IPV6

}

XrlCmdError
XrlRibTarget::rib_0_1_disable_rib_client4(
    // Input values, 
    const string&	target_name, 
    const bool&		unicast, 
    const bool&		multicast, 
    // Output values, 
    bool&	fail, 
    string&	reason)
{
    if (_rib_manager->disable_rib_client(target_name, AF_INET, unicast,
					 multicast)
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_disable_rib_client6(
    // Input values, 
    const string&	target_name, 
    const bool&		unicast, 
    const bool&		multicast, 
    // Output values, 
    bool&	fail, 
    string&	reason)
{
#ifndef HAVE_IPV6
    fail = true;
    reason = "IPv6 not supported";

    UNUSED(target_name);
    UNUSED(unicast);
    UNUSED(multicast);
    
    return XrlCmdError::OKAY();
    
#else
    if (_rib_manager->disable_rib_client(target_name, AF_INET6, unicast,
					 multicast)
	!= XORP_OK) {
	fail = true;
    } else {
	fail = false;
    }
    reason = "";
    
    return XrlCmdError::OKAY();
#endif // HAVE_IPV6
}

XrlCmdError 
XrlRibTarget::rib_0_1_no_fea()
{
    _rib_manager->no_fea();
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_add_igp_table4(const string&	protocol,
				     const bool&	unicast,
				     const bool&	multicast)
{
    if (unicast && _urib4.add_igp_table(protocol))
	RETURN_FAIL(c_format("Could not add unicast IPv4 igp table \"%s\"",
			     protocol.c_str()));

    if (multicast && _mrib4.add_igp_table(protocol))
	RETURN_FAIL(c_format("Could not add multicast IPv4 igp table \"%s\"",
			     protocol.c_str()));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_add_igp_table6(const string&	protocol,
				     const bool&	unicast,
				     const bool&	multicast)
{
    if (unicast && _urib6.add_igp_table(protocol))
	RETURN_FAIL(c_format("Could not add unicast IPv6 igp table \"%s\"",
			     protocol.c_str()));

    if (multicast && _mrib6.add_igp_table(protocol))
	RETURN_FAIL(c_format("Could not add multicast IPv6 igp table \"%s\"",
			     protocol.c_str()));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_delete_igp_table4(const string&	protocol,
					const bool&	unicast,
					const bool&	multicast)
{
    if (unicast && _urib4.delete_igp_table(protocol))
	RETURN_FAIL(c_format("Could not delete unicast IPv4 igp table \"%s\"",
			     protocol.c_str()));

    if (multicast && _mrib4.delete_igp_table(protocol))
	RETURN_FAIL(c_format("Could not delete multicast IPv4 igp table "
			     "\"%s\"", protocol.c_str()));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_delete_igp_table6(const string&	protocol,
					const bool&	unicast,
					const bool&	multicast)
{
    if (unicast && _urib6.delete_igp_table(protocol))
	RETURN_FAIL(c_format("Could not delete unicast IPv6 igp table \"%s\"",
			     protocol.c_str()));

    if (multicast && _mrib6.delete_igp_table(protocol))
	RETURN_FAIL(c_format("Could not delete multicast IPv6 igp table "
			     "\"%s\"", protocol.c_str()));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_add_egp_table4(const string&	protocol,
				     const bool&	unicast,
				     const bool&	multicast)
{
    if (unicast && _urib4.add_egp_table(protocol))
	RETURN_FAIL(c_format("Could not add unicast IPv4 egp table \"%s\"",
			     protocol.c_str()));

    if (multicast && _mrib4.add_egp_table(protocol))
	RETURN_FAIL(c_format("Could not add multicast IPv4 egp table \"%s\"",
			     protocol.c_str()));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_add_egp_table6(const string&	protocol,
				     const bool&	unicast,
				     const bool&	multicast)
{
    if (unicast && _urib6.add_egp_table(protocol))
	RETURN_FAIL(c_format("Could not add unicast IPv6 egp table \"%s\"",
			     protocol.c_str()));

    if (multicast && _mrib6.add_egp_table(protocol))
	RETURN_FAIL(c_format("Could not add multicast IPv6 egp table \"%s\"",
			     protocol.c_str()));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_delete_egp_table4(const string&	protocol,
					const bool&	unicast,
					const bool&	multicast)
{
    if (unicast && _urib4.delete_egp_table(protocol))
	RETURN_FAIL(c_format("Could not delete unicast IPv4 egp table \"%s\"",
			     protocol.c_str()));

    if (multicast && _mrib4.delete_egp_table(protocol))
	RETURN_FAIL(c_format("Could not delete multicast IPv4 egp table "
			     "\"%s\"", protocol.c_str()));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_delete_egp_table6(const string&	protocol,
					const bool&	unicast,
					const bool&	multicast)
{
    if (unicast && _urib6.delete_egp_table(protocol))
	RETURN_FAIL(c_format("Could not delete unicast IPv6 egp table \"%s\"",
			     protocol.c_str()));

    if (multicast && _mrib6.delete_egp_table(protocol))
	RETURN_FAIL(c_format("Could not delete multicast IPv6 egp table "
			     "\"%s\"", protocol.c_str()));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_add_route4(const string&	protocol,
				 const bool&	unicast,
				 const bool&	multicast,
				 const IPv4Net&	network,
				 const IPv4&	nexthop,
				 const uint32_t& metric)
{
    printf("#### XRL: ADD ROUTE net %s, nh: %s\n",
	   network.str().c_str(), nexthop.str().c_str());
    if (unicast && _urib4.add_route(protocol, network, nexthop, metric))
	RETURN_FAIL("Could not add IPv4 route to unicast RIB");

    if (multicast && _mrib4.add_route(protocol, network, nexthop, metric))
	RETURN_FAIL("Could not add IPv4 route to multicast RIB");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_add_route6(const string&	protocol,
				 const bool&	unicast,
				 const bool&	multicast,
				 const IPv6Net&	network,
				 const IPv6&	nexthop,
				 const uint32_t& metric)
{
    if (unicast && _urib6.add_route(protocol, network, nexthop, metric))
	RETURN_FAIL("Could not add IPv6 route to unicast RIB");

    if (multicast &&  _mrib6.add_route(protocol, network, nexthop, metric))
	RETURN_FAIL("Could not add IPv6 route to multicast RIB");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_replace_route4(const string&	protocol,
				     const bool&	unicast,
				     const bool&	multicast,
				     const IPv4Net&	network,
				     const IPv4&	nexthop,
				     const uint32_t& metric)
{
    if (unicast && _urib4.replace_route(protocol, network, nexthop, metric))
	RETURN_FAIL("Could not replace IPv4 route in unicast RIB");

    if (multicast && _mrib4.replace_route(protocol, network, nexthop, metric))
	RETURN_FAIL("Could not replace IPv4 route in multicast RIB");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_replace_route6(const string&	protocol,
				     const bool&	unicast,
				     const bool&	multicast,
				     const IPv6Net&	network,
				     const IPv6&	nexthop,
				     const uint32_t& metric)
{
    if (unicast && _urib6.replace_route(protocol, network, nexthop, metric))
	RETURN_FAIL("Could not replace IPv6 route in unicast RIB");

    if (multicast &&  _mrib6.replace_route(protocol, network, nexthop, metric))
	RETURN_FAIL("Could not add IPv6 route in multicast RIB");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_delete_route4(const string& protocol,
				    const bool&	unicast,
				    const bool&	multicast,
				    const IPv4Net& network)
{
    printf("#### XRL: DELETE ROUTE net %s\n",
	   network.str().c_str());
    if (unicast && _urib4.delete_route(protocol, network))
	RETURN_FAIL("Could not delete IPv4 route from unicast RIB");

    if (multicast && _mrib4.delete_route(protocol, network))
	RETURN_FAIL("Could not delete IPv4 route from multicast RIB");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_delete_route6(const string& protocol,
				    const bool& unicast,
				    const bool& multicast,
				    const IPv6Net& network)
{
    if (unicast && _urib6.delete_route(protocol, network))
	RETURN_FAIL("Could not delete IPv6 route from unicast RIB");

    if (multicast && _mrib6.delete_route(protocol, network))
	RETURN_FAIL("Could not delete IPv6 route from multicast RIB");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_lookup_route4(// Input values,
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
XrlRibTarget::rib_0_1_lookup_route6(// Input values,
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

    // XXX probably want something more selective (eg rib selector)
    if (_urib4.new_vif(name, v))
	RETURN_FAIL(c_format("Failed to add vif \"%s\" to unicast IPv4 rib",
			     name.c_str()));

    if (_mrib4.new_vif(name, v))
	RETURN_FAIL(c_format("Failed to add vif \"%s\" to multicast IPv4 rib",
			     name.c_str()));

    if (_urib6.new_vif(name, v))
	RETURN_FAIL(c_format("Failed to add vif \"%s\" to unicast IPv6 rib",
			     name.c_str()));

    if (_mrib6.new_vif(name, v))
	RETURN_FAIL(c_format("Failed to add vif \"%s\" to multicast IPv6 rib",
			     name.c_str()));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_add_vif_addr4(const string&	name,
				    const IPv4&		addr,
				    const IPv4Net&	subnet)
{
    if (_urib4.add_vif_address(name, addr, subnet))
	RETURN_FAIL("Failed to add IPv4 Vif address to unicast RIB");

    if (_mrib4.add_vif_address(name, addr, subnet))
	RETURN_FAIL("Failed to add IPv4 Vif address to multicast RIB");

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_add_vif_addr6(const string&	name,
				    const IPv6&		addr,
				    const IPv6Net&	subnet)
{
    if (_urib6.add_vif_address(name, addr, subnet))
	RETURN_FAIL("Failed to add IPv6 Vif address to unicast RIB");

    if (_mrib6.add_vif_address(name, addr, subnet))
	RETURN_FAIL("Failed to add IPv6 Vif address to multicast RIB");

    return XrlCmdError::OKAY();
}



XrlCmdError
XrlRibTarget::rib_0_1_redist_enable4(const string&	from,
				     const string&	to,
				     const bool&	unicast,
				     const bool&	multicast)
{
    if (unicast && _urib4.redist_enable(from, to))
	RETURN_FAIL(c_format("Failed to enable unicast IPv4 redistribution "
			     "from \"%s\" to \"%s\"",
			     from.c_str(), to.c_str()));

    if (multicast && _mrib4.redist_enable(from, to))
	RETURN_FAIL(c_format("Failed to enable multicast IPv4 redistribution "
			     "from \"%s\" to \"%s\"",
			     from.c_str(), to.c_str()));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_redist_enable6(const string&	from,
				     const string&	to,
				     const bool&	unicast,
				     const bool&	multicast)
{
    if (unicast && _urib6.redist_enable(from, to))
	RETURN_FAIL(c_format("Failed to enable unicast IPv6 redistribution "
			     "from \"%s\" to \"%s\"",
			     from.c_str(), to.c_str()));

    if (multicast && _mrib6.redist_enable(from, to))
	RETURN_FAIL(c_format("Failed to enable multicast IPv6 redistribution "
			     "from \"%s\" to \"%s\"",
			     from.c_str(), to.c_str()));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_redist_disable4(const string&	from,
				      const string&	to,
				      const bool&	unicast,
				      const bool&	multicast)
{
    if (unicast && _urib4.redist_disable(from, to))
	RETURN_FAIL(c_format("Failed to disable unicast IPv4 redistribution "
			     "from \"%s\" to \"%s\"",
			     from.c_str(), to.c_str()));

    if (multicast && _mrib4.redist_disable(from, to))
	RETURN_FAIL(c_format("Failed to disable multicast IPv4 redistribution "
			     "from \"%s\" to \"%s\"",
			     from.c_str(), to.c_str()));

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_redist_disable6(const string&	from,
				      const string&	to,
				      const bool&	unicast,
				      const bool&	multicast)
{
    if (unicast && _urib6.redist_disable(from, to))
	RETURN_FAIL(c_format("Failed to disable unicast IPv6 redistribution "
			     "from \"%s\" to \"%s\"",
			     from.c_str(), to.c_str()));

    if (multicast && _mrib6.redist_disable(from, to))
	RETURN_FAIL(c_format("Failed to disable multicast IPv6 redistribution "
			     "from \"%s\" to \"%s\"",
			     from.c_str(), to.c_str()));
    
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_register_interest4(// Input values,
					 const string& target,
					 const IPv4& addr,
					 // Output values,
					 bool& resolves,
					 IPv4&	base_addr,
					 uint32_t& prefix,
					 uint32_t& realprefix,
					 IPv4&	nexthop,
					 uint32_t& metric)
{
    printf("#### XRL: REGISTER INTEREST addr %s\n",
	   addr.str().c_str());
    RouteRegister<IPv4>* rt_reg = _urib4.route_register(addr, target);
    if (rt_reg->route() == NULL) {
	base_addr = rt_reg->valid_subnet().masked_addr();
	prefix = realprefix = rt_reg->valid_subnet().prefix_len();
	resolves = false;
	printf("#### XRL -> REGISTER INTEREST UNRESOLVABLE %s\n",
	       rt_reg->valid_subnet().str().c_str());
    } else {
	metric = rt_reg->route()->metric();
	base_addr = rt_reg->valid_subnet().masked_addr();
	prefix = rt_reg->valid_subnet().prefix_len();
	NextHop *nh = rt_reg->route()->nexthop();
	switch (nh->type()) {
	case GENERIC_NEXTHOP:
	    // this shouldn't be possible
	    abort();
	case PEER_NEXTHOP:
	case ENCAPS_NEXTHOP:
	    resolves = true;
	    nexthop = ((IPNextHop<IPv4>*)nh)->addr();
	    realprefix = rt_reg->route()->prefix_len();
	    break;
	case EXTERNAL_NEXTHOP:
	case DISCARD_NEXTHOP:
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
					   const uint32_t& prefix)
{
    _urib4.route_deregister(IPNet<IPv4>(addr, prefix), target);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::rib_0_1_register_interest6(// Input values,
					 const string& target,
					 const IPv6& addr,
					 // Output values,
					 bool& resolves,
					 IPv6& base_addr,
					 uint32_t& prefix,
					 uint32_t& realprefix,
					 IPv6&	nexthop,
					 uint32_t& metric)
{
    RouteRegister<IPv6>* rt_reg = _urib6.route_register(addr, target);
    if (rt_reg->route() == NULL) {
	base_addr = rt_reg->valid_subnet().masked_addr();
	prefix = rt_reg->valid_subnet().prefix_len();
	resolves = false;
    } else {
	metric = rt_reg->route()->metric();
	base_addr = rt_reg->valid_subnet().masked_addr();
	prefix = rt_reg->valid_subnet().prefix_len();
	NextHop *nh = rt_reg->route()->nexthop();
	switch (nh->type()) {
	case GENERIC_NEXTHOP:
	    // this shouldn't be possible
	    abort();
	case PEER_NEXTHOP:
	case ENCAPS_NEXTHOP:
	    resolves = true;
	    nexthop = ((IPNextHop<IPv6>*)nh)->addr();
	    realprefix = rt_reg->route()->prefix_len();
	    break;
	case EXTERNAL_NEXTHOP:
	case DISCARD_NEXTHOP:
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
					   const uint32_t& prefix)
{
    _urib6.route_deregister(IPNet<IPv6>(addr, prefix), target);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::fea_ifmgr_client_0_1_interface_update(// Input values,
						    const string& ifname,
						    const uint32_t& event)
{
    _vif_manager.interface_update(ifname, event);
    return XrlCmdError::OKAY();
}


XrlCmdError
XrlRibTarget::fea_ifmgr_client_0_1_vif_update(// Input values,
					      const string& ifname,
					      const string& vifname,
					      const uint32_t& event)
{
    _vif_manager.vif_update(ifname, vifname, event);
    return XrlCmdError::OKAY();
}


XrlCmdError
XrlRibTarget::fea_ifmgr_client_0_1_vifaddr4_update(// Input values,
						   const string& ifname,
						   const string& vifname,
						   const IPv4& addr,
						   const uint32_t& event)
{
    _vif_manager.vifaddr4_update(ifname, vifname, addr, event);
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlRibTarget::fea_ifmgr_client_0_1_vifaddr6_update(// Input values,
						   const string& ifname,
						   const string& vifname,
						   const IPv6& addr,
						   const uint32_t& event)
{
    _vif_manager.vifaddr6_update(ifname, vifname, addr, event);
    return XrlCmdError::OKAY();
}

