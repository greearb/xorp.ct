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

// $XORP: xorp/static_routes/xrl_static_routes_node.hh,v 1.3 2004/03/30 03:24:12 pavlin Exp $

#ifndef __STATIC_ROUTES_XRL_STATIC_ROUTES_NODE_HH__
#define __STATIC_ROUTES_XRL_STATIC_ROUTES_NODE_HH__


//
// StaticRoutes XRL-aware node definition.
//

#include "libfeaclient/ifmgr_xrl_mirror.hh"

#include "xrl/interfaces/rib_xif.hh"
#include "xrl/targets/static_routes_base.hh"

#include "static_routes_node.hh"


class XrlRouter;

//
// The top-level class that wraps-up everything together under one roof
//
class XrlStaticRoutesNode : public StaticRoutesNode,
			    public XrlStaticRoutesTargetBase {
public:
    XrlStaticRoutesNode(EventLoop& eventloop,
			XrlRouter* xrl_router,
			const string& fea_target,
			const string& rib_target,
			const IPv4& finder_host,
			uint16_t finder_port);
    ~XrlStaticRoutesNode();

    /**
     * Start the node operation.
     */
    void	startup();

    /**
     * Shutdown the node operation.
     */
    void	shutdown();

protected:
    //
    // XRL target methods
    //

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
	uint32_t&	status, 
	string&	reason);

    /**
     *  Request clean shutdown of Xrl Target
     */
    XrlCmdError common_0_1_shutdown();

    /**
     *  Enable/disable/start/stop StaticRoutes.
     *
     *  @param enable if true, then enable StaticRoutes, otherwise disable it.
     */
    XrlCmdError static_routes_0_1_enable_static_routes(
	// Input values,
	const bool&	enable);

    XrlCmdError static_routes_0_1_start_static_routes();

    XrlCmdError static_routes_0_1_stop_static_routes();

    /**
     *  Add/replace/delete a static route.
     *
     *  @param unicast if true, then the route would be used for unicast
     *  routing.
     *
     *  @param multicast if true, then the route would be used in the MRIB
     *  (Multicast Routing Information Base) for multicast purpose (e.g.,
     *  computing the Reverse-Path Forwarding information).
     *
     *  @param network the network address prefix this route applies to.
     *
     *  @param nexthop the address of the next-hop router for this route.
     *
     *  @param metric the metric distance for this route.
     */
    XrlCmdError static_routes_0_1_add_route4(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv4Net&	network,
	const IPv4&	nexthop,
	const uint32_t&	metric);

    XrlCmdError static_routes_0_1_add_route6(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv6Net&	network,
	const IPv6&	nexthop,
	const uint32_t&	metric);

    XrlCmdError static_routes_0_1_replace_route4(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv4Net&	network,
	const IPv4&	nexthop,
	const uint32_t&	metric);

    XrlCmdError static_routes_0_1_replace_route6(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv6Net&	network,
	const IPv6&	nexthop,
	const uint32_t&	metric);

    XrlCmdError static_routes_0_1_delete_route4(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv4Net&	network);

    XrlCmdError static_routes_0_1_delete_route6(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv6Net&	network);

    /**
     *  Add/replace a static route by explicitly specifying the network
     *  interface toward the destination.
     *
     *  @param unicast if true, then the route would be used for unicast
     *  routing.
     *
     *  @param multicast if true, then the route would be used in the MRIB
     *  (Multicast Routing Information Base) for multicast purpose (e.g.,
     *  computing the Reverse-Path Forwarding information).
     *
     *  @param network the network address prefix this route applies to.
     *
     *  @param nexthop the address of the next-hop router for this route.
     *
     *  @param ifname of the name of the physical interface toward the
     *  destination.
     *
     *  @param vifname of the name of the virtual interface toward the
     *  destination.
     *
     *  @param metric the metric distance for this route.
     */
    XrlCmdError static_routes_0_1_add_interface_route4(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv4Net&	network,
	const IPv4&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric);

    XrlCmdError static_routes_0_1_add_interface_route6(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv6Net&	network,
	const IPv6&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric);

    XrlCmdError static_routes_0_1_replace_interface_route4(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv4Net&	network,
	const IPv4&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric);

    XrlCmdError static_routes_0_1_replace_interface_route6(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv6Net&	network,
	const IPv6&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric);

    /**
     *  Enable/disable the StaticRoutes trace log for all operations.
     *
     *  @param enable if true, then enable the trace log, otherwise disable it.
     */
    XrlCmdError static_routes_0_1_enable_log_trace_all(
	// Input values,
	const bool&	enable);

private:

    void ifmgr_startup();
    void ifmgr_shutdown();

    const IfMgrIfTree& ifmgr_iftree() const { return _ifmgr.iftree(); }

    void rib_register_startup();
    void rib_register_shutdown();

    /**
     * Inform the RIB about a route change.
     *
     * @param static_route the route with the information about the change.
     */
    void inform_rib_route_change(const StaticRoute& static_route);

    /**
     * Cancel a pending request to inform the RIB about a route change.
     *
     * @param static_route the route with the request that would be canceled.
     */
    void cancel_rib_route_change(const StaticRoute& static_route);

    void send_rib_route_change();
    void send_rib_route_change_cb(const XrlError& xrl_error);

    void send_rib_registration();
    void send_add_igp_table4_cb(const XrlError& xrl_error);
    void send_add_igp_table6_cb(const XrlError& xrl_error);
    void send_rib_deregistration();
    void send_delete_igp_table4_cb(const XrlError& xrl_error);
    void send_delete_igp_table6_cb(const XrlError& xrl_error);

    const string& my_xrl_target_name() {
	return XrlStaticRoutesTargetBase::name();
    }

    const string	_class_name;
    const string	_instance_name;
    XrlRibV0p1Client	_xrl_rib_client;
    const string	_fea_target;
    const string	_rib_target;
    IfMgrXrlMirror	_ifmgr;
    list<StaticRoute>	_inform_rib_queue;
    XorpTimer		_inform_rib_queue_timer;
    bool		_is_rib_igp_table4_registered;
    bool		_is_rib_igp_table6_registered;
    XorpTimer		_rib_igp_table_registration_timer;
};

#endif // __STATIC_ROUTES_XRL_STATIC_ROUTES_NODE_HH__
