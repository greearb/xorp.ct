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

// $XORP: xorp/rib/xrl_target.hh,v 1.3 2003/03/10 23:20:58 hodson Exp $

#ifndef __RIB_XRL_TARGET_HH__
#define __RIB_XRL_TARGET_HH__

#include "xrl/targets/rib_base.hh"
#include "rib.hh"
#include "vifmanager.hh"

/**
 * @short Implement RIB Xrl target methods
 *
 * XrlRibTarget implements the auto-generated sub methods to handle
 * XRL requests from the routing protocols to the RIB.  
 */
class XrlRibTarget : public XrlRibTargetBase {
public:
    /**
     * XrlRibTarget constructor
     *
     * @param r the XrlRouter instance handling sending and receiving
     * XRLs for this process
     * @param u4 the IPv4 unicast RIB.
     * @param m4 the IPv4 multicast RIB.
     * @param u6 the IPv6 unicast RIB.
     * @param m6 the IPv6 multicast RIB.
     * @param vifmanager the VifManager for this process handling
     * communication with the FEA regarding VIF changes.
     */ 
    XrlRibTarget(XrlRouter *r,
		 RIB<IPv4>& u4, RIB<IPv4>& m4,
		 RIB<IPv6>& u6, RIB<IPv6>& m6,
		 VifManager& vifmanager, RibManager *ribmanager) : 
	XrlRibTargetBase(r), _urib4(u4), _mrib4(m4), _urib6(u6), _mrib6(m6),
	_vifmanager(vifmanager), _ribmanager(ribmanager) {}
    /**
     * XrlRibTarget destructor
     */
    ~XrlRibTarget() {}

protected:
    RIB<IPv4>&	_urib4;
    RIB<IPv4>&	_mrib4;
    RIB<IPv6>&	_urib6;
    RIB<IPv6>&	_mrib6;
    VifManager& _vifmanager;
    RibManager	*_ribmanager;

protected:

    virtual XrlCmdError common_0_1_get_target_name(
	// Output values, 
	string&	name);

    virtual XrlCmdError common_0_1_get_version(
	// Output values, 
	string&	version);

    virtual XrlCmdError rib_0_1_no_fea();

    virtual XrlCmdError rib_0_1_add_igp_table4(
	// Input values, 
	const string&	protocol, 
	const bool&	unicast,
	const bool&	multicast);

    virtual XrlCmdError rib_0_1_add_igp_table6(
	// Input values, 
	const string&	protocol, 
	const bool&	unicast,
	const bool&	multicast);

    virtual XrlCmdError rib_0_1_delete_igp_table4(
	// Input values, 
	const string&	protocol, 
	const bool&	unicast,
	const bool&	multicast);

    virtual XrlCmdError rib_0_1_delete_igp_table6(
	// Input values, 
	const string&	protocol, 
	const bool&	unicast,
	const bool&	multicast);

    virtual XrlCmdError rib_0_1_add_egp_table4(
	// Input values, 
	const string&	protocol, 
	const bool&	unicast,
	const bool&	multicast);

    virtual XrlCmdError rib_0_1_add_egp_table6(
	// Input values, 
	const string&	protocol, 
	const bool&	unicast,
	const bool&	multicast);

    virtual XrlCmdError rib_0_1_delete_egp_table4(
	// Input values, 
	const string&	protocol, 
	const bool&	unicast,
	const bool&	multicast);

    virtual XrlCmdError rib_0_1_delete_egp_table6(
	// Input values, 
	const string&	protocol, 
	const bool&	unicast,
	const bool&	multicast);

    virtual XrlCmdError rib_0_1_add_route4(
	// Input values, 
	const string&	protocol, 
	const bool&	unicast,
	const bool&	multicast,
	const IPv4Net&	network, 
	const IPv4&     nexthop,
	const uint32_t&       metric);

    virtual XrlCmdError rib_0_1_add_route6(
	// Input values, 
	const string&	protocol, 
	const bool&	unicast,
	const bool&	multicast,
	const IPv6Net&	network, 
	const IPv6&	nexthop,
	const uint32_t&       metric);

    virtual XrlCmdError rib_0_1_replace_route4(
	// Input values, 
	const string&	protocol, 
	const bool&	unicast,
	const bool&	multicast,
	const IPv4Net&	network, 
	const IPv4&     nexthop,
	const uint32_t&       metric);

    virtual XrlCmdError rib_0_1_replace_route6(
	// Input values, 
	const string&	protocol, 
	const bool&	unicast,
	const bool&	multicast,
	const IPv6Net&	network, 
	const IPv6&	nexthop,
	const uint32_t&       metric);

    virtual XrlCmdError rib_0_1_delete_route4(
	// Input values, 
	const string&	protocol, 
	const bool&	unicast,
	const bool&	multicast,
	const IPv4Net&	network);

    virtual XrlCmdError rib_0_1_delete_route6(
	// Input values, 
	const string&	protocol, 
	const bool&	unicast,
	const bool&	multicast,
	const IPv6Net&	network);

    virtual XrlCmdError rib_0_1_lookup_route4(
	// Input values, 
	const IPv4&	addr, 
	const bool&	unicast,
	const bool&	multicast,
	// Output values, 
	IPv4&	nexthop);

    virtual XrlCmdError rib_0_1_lookup_route6(
	// Input values, 
	const IPv6&	addr, 
	const bool&	unicast,
	const bool&	multicast,
	// Output values, 
	IPv6&		nexthop);

    virtual XrlCmdError rib_0_1_new_vif(
	// Input values, 
	const string&	name);

    virtual XrlCmdError rib_0_1_add_vif_addr4(
	// Input values, 
	const string&	name, 
	const IPv4&	addr, 
	const IPv4Net&	subnet);

    virtual XrlCmdError rib_0_1_add_vif_addr6(
	// Input values, 
	const string&	name, 
	const IPv6&	addr, 
	const IPv6Net&	subnet);

    virtual XrlCmdError rib_0_1_redist_enable4(
	// Input values, 
	const string&	from, 
	const string&	to, 
	const bool&	unicast,
	const bool&	multicast);

    virtual XrlCmdError rib_0_1_redist_enable6(
	// Input values, 
	const string&	from, 
	const string&	to, 
	const bool&	unicast,
	const bool&	multicast);

    virtual XrlCmdError rib_0_1_redist_disable4(
	// Input values, 
	const string&	from, 
	const string&	to, 
	const bool&	unicast,
	const bool&	multicast);

    virtual XrlCmdError rib_0_1_redist_disable6(
	// Input values, 
	const string&	from, 
	const string&	to, 
	const bool&	unicast,
	const bool&	multicast);

    virtual XrlCmdError rib_0_1_register_interest4(
	// Input values, 
        const string&	target,
	const IPv4&	addr, 
	// Output values, 
	bool& resolves,
	IPv4&	base_addr, 
	uint32_t&	prefix, 
	uint32_t&	realprefix, 
	IPv4&	nexthop, 
	uint32_t&	metric);

    virtual XrlCmdError rib_0_1_deregister_interest4(
	// Input values,  
        const string&	target,
	const IPv4&	addr, 
	const uint32_t&	prefix);

    virtual XrlCmdError rib_0_1_register_interest6(
	// Input values, 
        const string&	target,
	const IPv6&	addr, 
	// Output values, 
	bool& resolves,
	IPv6&	base_addr, 
	uint32_t&	prefix, 
	uint32_t&	realprefix, 
	IPv6&	nexthop, 
	uint32_t&	metric);

    virtual XrlCmdError rib_0_1_deregister_interest6(
	// Input values,  
        const string&	target,
	const IPv6&	addr, 
	const uint32_t&	prefix);

    virtual XrlCmdError fea_ifmgr_client_0_1_interface_update(
	// Input values, 
	const string&	ifname, 
	const uint32_t&	event);

    virtual XrlCmdError fea_ifmgr_client_0_1_vif_update(
	// Input values, 
	const string&	ifname, 
	const string&	vifname, 
	const uint32_t&	event);

    virtual XrlCmdError fea_ifmgr_client_0_1_vifaddr4_update(
	// Input values, 
	const string&	ifname, 
	const string&	vifname, 
	const IPv4&	addr, 
	const uint32_t&	event);

    virtual XrlCmdError fea_ifmgr_client_0_1_vifaddr6_update(
	// Input values, 
	const string&	ifname, 
	const string&	vifname, 
	const IPv6&	addr, 
	const uint32_t&	event);
};

#endif // __RIB_XRL_TARGET_HH__
