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

// $XORP: xorp/rib/xrl_target.hh,v 1.27 2004/09/17 14:00:05 abittau Exp $

#ifndef __RIB_XRL_TARGET_HH__
#define __RIB_XRL_TARGET_HH__

#include "libxipc/xrl_router.hh"

#include "xrl/targets/rib_base.hh"

#include "rib.hh"
#include "vifmanager.hh"


/**
 * @short Implement RIB Xrl target methods.
 *
 * XrlRibTarget implements the auto-generated sub methods to handle
 * XRL requests from the routing protocols to the RIB.
 */
class XrlRibTarget : public XrlRibTargetBase {
public:
    /**
     * XrlRibTarget constructor
     *
     * @param xrl_router the XrlRouter instance handling sending and receiving
     * XRLs for this process.
     * @param urib4 the IPv4 unicast RIB.
     * @param mrib4 the IPv4 multicast RIB.
     * @param urib6 the IPv6 unicast RIB.
     * @param mrib6 the IPv6 multicast RIB.
     * @param vif_manager the VifManager for this process handling
     * communication with the FEA regarding VIF changes.
     * @param rib_manager the RibManager for this process.
     */
    XrlRibTarget(XrlRouter* xrl_router,
		 RIB<IPv4>& urib4, RIB<IPv4>& mrib4,
		 RIB<IPv6>& urib6, RIB<IPv6>& mrib6,
		 VifManager& vif_manager, RibManager* rib_manager)
	: XrlRibTargetBase(xrl_router),
	  _urib4(urib4), _mrib4(mrib4), _urib6(urib6), _mrib6(mrib6),
	  _vif_manager(vif_manager), _rib_manager(rib_manager) {}
    /**
     * XrlRibTarget destructor
     */
    ~XrlRibTarget() {}

protected:
    RIB<IPv4>&	_urib4;
    RIB<IPv4>&	_mrib4;
    RIB<IPv6>&	_urib6;
    RIB<IPv6>&	_mrib6;
    VifManager& _vif_manager;
    RibManager*	_rib_manager;

protected:

    /**
     *  Get name of Xrl Target
     */
    XrlCmdError common_0_1_get_target_name(
	// Output values,
	string&	name);

    /**
     *  Get version string from Xrl Target
     */
    XrlCmdError common_0_1_get_version(
	// Output values,
	string&	version);

    /**
     *  Get status of Xrl Target
     */
    XrlCmdError common_0_1_get_status(
	// Output values,
        uint32_t& status,
	string&	reason);

    /**
     *  Request clean shutdown of Xrl Target
     */
    XrlCmdError common_0_1_shutdown();

    /**
     *  Enable/disable/start/stop RIB.
     */
    XrlCmdError rib_0_1_enable_rib();

    XrlCmdError rib_0_1_disable_rib();

    XrlCmdError rib_0_1_start_rib();

    XrlCmdError rib_0_1_stop_rib();

    /**
     *  Make errors fatal; used to detect errors we'd normally mask
     */
    XrlCmdError rib_0_1_make_errors_fatal();

    /**
     *  Add/delete an IGP or EGP table.
     *
     *  @param protocol the name of the protocol.
     *
     *  @param target_class the target class of the protocol.
     *
     *  @param target_instance the target instance of the protocol.
     *
     *  @param unicast true if the table is for the unicast RIB.
     *
     *  @param multicast true if the table is for the multicast RIB.
     */
    XrlCmdError rib_0_1_add_igp_table4(
	// Input values,
	const string&	protocol,
	const string&	target_class,
	const string&	target_instance,
	const bool&	unicast,
	const bool&	multicast);

    XrlCmdError rib_0_1_add_igp_table6(
	// Input values,
	const string&	protocol,
	const string&	target_class,
	const string&	target_instance,
	const bool&	unicast,
	const bool&	multicast);

    XrlCmdError rib_0_1_delete_igp_table4(
	// Input values,
	const string&	protocol,
	const string&	target_class,
	const string&	target_instance,
	const bool&	unicast,
	const bool&	multicast);

    XrlCmdError rib_0_1_delete_igp_table6(
	// Input values,
	const string&	protocol,
	const string&	target_class,
	const string&	target_instance,
	const bool&	unicast,
	const bool&	multicast);

    XrlCmdError rib_0_1_add_egp_table4(
	// Input values,
	const string&	protocol,
	const string&	target_class,
	const string&	target_instance,
	const bool&	unicast,
	const bool&	multicast);

    XrlCmdError rib_0_1_add_egp_table6(
	// Input values,
	const string&	protocol,
	const string&	target_class,
	const string&	target_instance,
	const bool&	unicast,
	const bool&	multicast);

    XrlCmdError rib_0_1_delete_egp_table4(
	// Input values,
	const string&	protocol,
	const string&	target_class,
	const string&	target_instance,
	const bool&	unicast,
	const bool&	multicast);

    XrlCmdError rib_0_1_delete_egp_table6(
	// Input values,
	const string&	protocol,
	const string&	target_class,
	const string&	target_instance,
	const bool&	unicast,
	const bool&	multicast);

    /**
     *  Add/replace/delete a route.
     *
     *  @param protocol the name of the protocol this route comes from.
     *
     *  @param unicast true if the route is for the unicast RIB.
     *
     *  @param multicast true if the route is for the multicast RIB.
     *
     *  @param network the network address prefix of the route.
     *
     *  @param nexthop the address of the next-hop router toward the
     *  destination.
     *
     *  @param metric the routing metric.
     *
     *  @param policytags the policy-tags for this route.
     */
    XrlCmdError rib_0_1_add_route4(
	// Input values,
	const string&	protocol,
	const bool&	unicast,
	const bool&	multicast,
	const IPv4Net&	network,
	const IPv4&     nexthop,
	const uint32_t& metric,
	const XrlAtomList&	policytags);

    XrlCmdError rib_0_1_add_route6(
	// Input values,
	const string&	protocol,
	const bool&	unicast,
	const bool&	multicast,
	const IPv6Net&	network,
	const IPv6&	nexthop,
	const uint32_t& metric,
	const XrlAtomList&	policytags);

    XrlCmdError rib_0_1_replace_route4(
	// Input values,
	const string&	protocol,
	const bool&	unicast,
	const bool&	multicast,
	const IPv4Net&	network,
	const IPv4&     nexthop,
	const uint32_t& metric,
	const XrlAtomList&	policytags);

    XrlCmdError rib_0_1_replace_route6(
	// Input values,
	const string&	protocol,
	const bool&	unicast,
	const bool&	multicast,
	const IPv6Net&	network,
	const IPv6&	nexthop,
	const uint32_t& metric,
	const XrlAtomList&	policytags);

    XrlCmdError rib_0_1_delete_route4(
	// Input values,
	const string&	protocol,
	const bool&	unicast,
	const bool&	multicast,
	const IPv4Net&	network);

    XrlCmdError rib_0_1_delete_route6(
	// Input values,
	const string&	protocol,
	const bool&	unicast,
	const bool&	multicast,
	const IPv6Net&	network);

    /**
     *  Add/replace a route by explicitly specifying the network interface
     *  toward the destination.
     *
     *  @param protocol the name of the protocol this route comes from.
     *
     *  @param unicast true if the route is for the unicast RIB.
     *
     *  @param multicast true if the route is for the multicast RIB.
     *
     *  @param network the network address prefix of the route.
     *
     *  @param nexthop the address of the next-hop router toward the
     *  destination.
     *
     *  @param ifname of the name of the physical interface toward the
     *  destination.
     *
     *  @param vifname of the name of the virtual interface toward the
     *  destination.
     *
     *  @param metric the routing metric.
     *
     *  @param policytags the policy-tags for this route.
     */
    XrlCmdError rib_0_1_add_interface_route4(
	// Input values,
	const string&	    protocol,
	const bool&	    unicast,
	const bool&	    multicast,
	const IPv4Net&	    network,
	const IPv4&	    nexthop,
	const string&	    ifname,
	const string&	    vifname,
	const uint32_t&	    metric,
	const XrlAtomList&  policytags);

    XrlCmdError rib_0_1_add_interface_route6(
	// Input values,
	const string&	    protocol,
	const bool&	    unicast,
	const bool&	    multicast,
	const IPv6Net&	    network,
	const IPv6&	    nexthop,
	const string&	    ifname,
	const string&	    vifname,
	const uint32_t&	    metric,
	const XrlAtomList&  policytags);

    XrlCmdError rib_0_1_replace_interface_route4(
	// Input values,
	const string&	    protocol,
	const bool&	    unicast,
	const bool&	    multicast,
	const IPv4Net&	    network,
	const IPv4&	    nexthop,
	const string&	    ifname,
	const string&	    vifname,
	const uint32_t&	    metric,
	const XrlAtomList&  policytags);

    XrlCmdError rib_0_1_replace_interface_route6(
	// Input values,
	const string&	    protocol,
	const bool&	    unicast,
	const bool&	    multicast,
	const IPv6Net&	    network,
	const IPv6&	    nexthop,
	const string&	    ifname,
	const string&	    vifname,
	const uint32_t&	    metric,
	const XrlAtomList&  policytags);

    /**
     *  Lookup nexthop.
     *
     *  @param addr address to lookup.
     *
     *  @param unicast look in unicast RIB.
     *
     *  @param multicast look in multicast RIB.
     *
     *  @param nexthop contains the resolved nexthop if successful, IPv4::ZERO
     *  otherwise. It is an error for the unicast and multicast fields to both
     *  be true or both false.
     */
    XrlCmdError rib_0_1_lookup_route_by_dest4(
	// Input values,
	const IPv4&	addr,
	const bool&	unicast,
	const bool&	multicast,
	// Output values,
	IPv4&		nexthop);

    /**
     *  Lookup nexthop.
     *
     *  @param addr address to lookup.
     *
     *  @param unicast look in unicast RIB.
     *
     *  @param multicast look in multicast RIB.
     *
     *  @param nexthop contains the resolved nexthop if successful, IPv6::ZERO
     *  otherwise. It is an error for the unicast and multicast fields to both
     *  be true or both false.
     */
    XrlCmdError rib_0_1_lookup_route_by_dest6(
	// Input values,
	const IPv6&	addr,
	const bool&	unicast,
	const bool&	multicast,
	// Output values,
	IPv6&		nexthop);

    /**
     *  Add a vif or a vif address to the RIB. This interface should be used
     *  only for testing purpose.
     *
     *  @param name the name of the vif.
     */
    XrlCmdError rib_0_1_new_vif(
	// Input values,
	const string&	name);

    /**
     *  Add a vif address to the RIB. This interface should be used only for
     *  testing purpose.
     *
     *  @param name the name of the vif.
     *
     *  @param addr the address to add.
     *
     *  @param subnet the subnet address to add.
     */
    XrlCmdError rib_0_1_add_vif_addr4(
	// Input values,
	const string&	name,
	const IPv4&	addr,
	const IPv4Net&	subnet);

    XrlCmdError rib_0_1_add_vif_addr6(
	// Input values,
	const string&	name,
	const IPv6&	addr,
	const IPv6Net&	subnet);

    /**
     *  Enable route redistribution from one routing protocol to another.
     *
     *  @param to_xrl_target the XRL Target instance name of the caller. The
     *  caller must implement redist4/0.1.
     *
     *  @param from_protocol the name of the routing process routes are to be
     *  redistributed from.
     *
     *  @param unicast enable for unicast RIBs matching from and to.
     *
     *  @param multicast enable for multicast RIBs matching from and to.
     *
     *  @param cookie a text value passed back to creator in each call from the
     *  RIB. This allows creators to identity the source of updates it receives
     *  through the redist4/0.1 interface.
     */
    XrlCmdError rib_0_1_redist_enable4(
	// Input values,
	const string&	to_xrl_target,
	const string&	from_protocol,
	const bool&	unicast,
	const bool&	multicast,
	const string&	cookie);

    /**
     *  Enable route redistribution from one routing protocol to another.
     *
     *  @param to_xrl_target the XRL Target instance name of the caller. The
     *  caller must implement redist6/0.1.
     *
     *  @param from_protocol the name of the routing process routes are to be
     *  redistributed from.
     *
     *  @param unicast enable for unicast RIBs matching from and to.
     *
     *  @param multicast enable for multicast RIBs matching from and to.
     *
     *  @param cookie a text value passed back to creator in each call from the
     *  RIB. This allows creators to identity the source of updates it receives
     *  through the redist6/0.1 interface.
     */
    XrlCmdError rib_0_1_redist_enable6(
	// Input values,
	const string&	to_xrl_target,
	const string&	from_protocol,
	const bool&	unicast,
	const bool&	multicast,
	const string&	cookie);

    /**
     *  Disable route redistribution from one routing protocol to another.
     *
     *  @param to_xrl_target the XRL Target instance name of the caller. The
     *  caller must implement redist4/0.1 and previously called redist_enable4.
     *
     *  @param unicast disable for unicast RIBs matching from and to.
     *
     *  @param multicast disable for multicast RIBs matching from and to.
     */
    XrlCmdError rib_0_1_redist_disable4(
	// Input values,
	const string&	to_xrl_target,
	const string&	from_protocol,
	const bool&	unicast,
	const bool&	multicast,
	const string&	cookie);

    /**
     *  Disable route redistribution from one routing protocol to another.
     *
     *  @param to_xrl_target the XRL Target instance name of the caller. The
     *  caller must implement redist6/0.1 and previously called redist_enable6.
     *
     *  @param unicast disable for unicast RIBs matching from and to.
     *
     *  @param multicast disable for multicast RIBs matching from and to.
     */
    XrlCmdError rib_0_1_redist_disable6(
	// Input values,
	const string&	to_xrl_target,
	const string&	from_protocol,
	const bool&	unicast,
	const bool&	multicast,
	const string&	cookie);

    /**
     *  Enable transaction-based route redistribution from one routing protocol
     *  to another.
     *
     *  @param to_xrl_target the XRL Target instance name of the caller. The
     *  caller must implement redist_transaction4/0.1.
     *
     *  @param from_protocol the name of the routing process routes are to be
     *  redistributed from.
     *
     *  @param unicast enable for unicast RIBs matching from and to.
     *
     *  @param multicast enable for multicast RIBs matching from and to.
     *
     *  @param cookie a text value passed back to creator in each call from the
     *  RIB. This allows creators to identity the source of updates it receives
     *  through the redist_transaction4/0.1 interface.
     */
    XrlCmdError rib_0_1_redist_transaction_enable4(
	// Input values,
	const string&	to_xrl_target,
	const string&	from_protocol,
	const bool&	unicast,
	const bool&	multicast,
	const string&	cookie);

    /**
     *  Enable transaction-based route redistribution from one routing protocol
     *  to another.
     *
     *  @param to_xrl_target the XRL Target instance name of the caller. The
     *  caller must implement redist_transaction6/0.1.
     *
     *  @param from_protocol the name of the routing process routes are to be
     *  redistributed from.
     *
     *  @param unicast enable for unicast RIBs matching from and to.
     *
     *  @param multicast enable for multicast RIBs matching from and to.
     *
     *  @param cookie a text value passed back to creator in each call from the
     *  RIB. This allows creators to identity the source of updates it receives
     *  through the redist_transaction6/0.1 interface.
     */
    XrlCmdError rib_0_1_redist_transaction_enable6(
	// Input values,
	const string&	to_xrl_target,
	const string&	from_protocol,
	const bool&	unicast,
	const bool&	multicast,
	const string&	cookie);

    /**
     *  Disable transaction-based route redistribution from one routing
     *  protocol to another.
     *
     *  @param to_xrl_target the XRL Target instance name of the caller. The
     *  caller must implement redist_transaction4/0.1 and previously called
     *  redist_transaction_enable4.
     *
     *  @param unicast disable for unicast RIBs matching from and to.
     *
     *  @param multicast disable for multicast RIBs matching from and to.
     */
    XrlCmdError rib_0_1_redist_transaction_disable4(
	// Input values,
	const string&	to_xrl_target,
	const string&	from_protocol,
	const bool&	unicast,
	const bool&	multicast,
	const string&	cookie);

    /**
     *  Disable transaction-based route redistribution from one routing
     *  protocol to another.
     *
     *  @param to_xrl_target the XRL Target instance name of the caller. The
     *  caller must implement redist_transaction6/0.1 and previously called
     *  redist_transaction_enable6.
     *
     *  @param unicast disable for unicast RIBs matching from and to.
     *
     *  @param multicast disable for multicast RIBs matching from and to.
     */
    XrlCmdError rib_0_1_redist_transaction_disable6(
	// Input values,
	const string&	to_xrl_target,
	const string&	from_protocol,
	const bool&	unicast,
	const bool&	multicast,
	const string&	cookie);

    /**
     *  Register an interest in a route.
     *
     *  @param target the name of the XRL module to notify when the information
     *  returned by this call becomes invalid.
     *
     *  @param addr address of interest.
     *
     *  @param resolves returns whether or not the address resolves to a route
     *  that can be used for forwarding.
     *
     *  @param base_addr returns the address of interest (actually the base
     *  address of the subnet covered by addr/prefix_len).
     *
     *  @param prefix_len returns the prefix length that the registration
     *  covers. This response applies to all addresses in addr/prefix_len.
     *
     *  @param real_prefix_len returns the actual prefix length of the route
     *  that will be used to route addr. If real_prefix_len is not the same as
     *  prefix_len, this is because there are some more specific routes that
     *  overlap addr/real_prefix_len. real_prefix_len is primarily given for
     *  debugging reasons.
     *
     *  @param nexthop returns the address of the next hop for packets sent to
     *  addr.
     *
     *  @param metric returns the IGP metric for this route.
     */
    XrlCmdError rib_0_1_register_interest4(
	// Input values,
        const string&	target,
	const IPv4&	addr,
	// Output values,
	bool&		resolves,
	IPv4&		base_addr,
	uint32_t&	prefix_len,
	uint32_t&	real_prefix_len,
	IPv4&		nexthop,
	uint32_t&	metric);

    /**
     *  De-register an interest in a route.
     *
     *  @param target the name of the XRL module that registered the interest.
     *
     *  @param addr the address of the previous registered interest. addr
     *  should be the base address of the add/prefix_len subnet.
     *
     *  @param prefix_len the prefix length of the registered interest, as
     *  given in the response from register_interest.
     */
    XrlCmdError rib_0_1_deregister_interest4(
	// Input values,
        const string&	target,
	const IPv4&	addr,
	const uint32_t&	prefix_len);

    /**
     *  Register an interest in a route.
     *
     *  @param target the name of the XRL module to notify when the information
     *  returned by this call becomes invalid.
     *
     *  @param addr address of interest.
     *
     *  @param resolves returns whether or not the address resolves to a route
     *  that can be used for forwarding.
     *
     *  @param base_addr returns the address of interest (actually the base
     *  address of the subnet covered by addr/prefix_len).
     *
     *  @param prefix_len returns the prefix length that the registration
     *  covers. This response applies to all addresses in addr/prefix_len.
     *
     *  @param real_prefix_len returns the actual prefix length of the route
     *  that will be used to route addr. If real_prefix_len is not the same as
     *  prefix_len, this is because there are some more specific routes that
     *  overlap addr/real_prefix_len. real_prefix_len is primarily given for
     *  debugging reasons.
     *
     *  @param nexthop returns the address of the next hop for packets sent to
     *  addr.
     *
     *  @param metric returns the IGP metric for this route.
     */
    XrlCmdError rib_0_1_register_interest6(
	// Input values,
        const string&	target,
	const IPv6&	addr,
	// Output values,
	bool&		resolves,
	IPv6&		base_addr,
	uint32_t&	prefix_len,
	uint32_t&	real_prefix_len,
	IPv6&		nexthop,
	uint32_t&	metric);

    /**
     *  De-register an interest in a route.
     *
     *  @param target the name of the XRL module that registered the interest.
     *
     *  @param addr the address of the previous registered interest. addr
     *  should be the base address of the add/prefix_len subnet.
     *
     *  @param prefix_len the prefix length of the registered interest, as
     *  given in the response from register_interest.
     */
    XrlCmdError rib_0_1_deregister_interest6(
	// Input values,
        const string&	target,
	const IPv6&	addr,
	const uint32_t&	prefix_len);

    /**
     *  Announce target birth.
     */
    XrlCmdError finder_event_observer_0_1_xrl_target_birth(
	// Input values,
	const string&	target_class,
	const string&	target_instance);

    /**
     *  Announce target death.
     */
    XrlCmdError finder_event_observer_0_1_xrl_target_death(
	// Input values,
	const string&	target_class,
	const string&	target_instance);


    /**
     * Configure a policy filter.
     *
     * @param filter id of filter to configure.
     * @param conf configuration of filter.
     */
    XrlCmdError policy_backend_0_1_configure(
        // Input values,
        const uint32_t& filter,
        const string&   conf);
      
    /**
     * Reset a policy filter.
     *
     * @param filter id of filter to reset.
     */
    XrlCmdError policy_backend_0_1_reset(
        // Input values,
        const uint32_t& filter);

    /**
     * Push routes through policy filters for re-filtering.
     */
    XrlCmdError policy_backend_0_1_push_routes();

    
    /**
     * Redistribute to a protocol based on policy-tags.
     *
     * @param protocol protocol to redistribute to
     * @param policytags policy-tags of routes which need to be redistributed.
     */
    XrlCmdError rib_0_1_insert_policy_redist_tags(
        // Input values,
        const string&   protocol,
        const XrlAtomList&      policytags);

    /**
     * Reset policy redistribution map.
     */
    XrlCmdError rib_0_1_reset_policy_redist_tags();

};

#endif // __RIB_XRL_TARGET_HH__
