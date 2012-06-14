// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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


#ifndef __STATIC_ROUTES_XRL_STATIC_ROUTES_NODE_HH__
#define __STATIC_ROUTES_XRL_STATIC_ROUTES_NODE_HH__


//
// StaticRoutes XRL-aware node definition.
//

#include "libxipc/xrl_std_router.hh"
#include "libfeaclient/ifmgr_xrl_mirror.hh"
#include "xrl/interfaces/finder_event_notifier_xif.hh"
#include "xrl/interfaces/rib_xif.hh"
#include "xrl/targets/static_routes_base.hh"

#include "static_routes_node.hh"


//
// The top-level class that wraps-up everything together under one roof
//
class XrlStaticRoutesNode : public StaticRoutesNode,
			    public XrlStdRouter,
			    public XrlStaticRoutesTargetBase {
public:
    XrlStaticRoutesNode(EventLoop&	eventloop,
			const string&	class_name,
			const string&	finder_hostname,
			uint16_t	finder_port,
			const string&	finder_target,
			const string&	fea_target,
			const string&	rib_target);
    ~XrlStaticRoutesNode();

    /**
     * Startup the node operation.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int startup();

    /**
     * Shutdown the node operation.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int shutdown();

    /**
     * Get a reference to the XrlRouter instance.
     *
     * @return a reference to the XrlRouter (@ref XrlRouter) instance.
     */
    XrlRouter&	xrl_router() { return *this; }

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

    XrlCmdError common_0_1_startup();

    /**
     *  Announce target birth to observer.
     *
     *  @param target_class the target class name.
     *
     *  @param target_instance the target instance name.
     */
    XrlCmdError finder_event_observer_0_1_xrl_target_birth(
	// Input values,
	const string&	target_class,
	const string&	target_instance);

    /**
     *  Announce target death to observer.
     *
     *  @param target_class the target class name.
     *
     *  @param target_instance the target instance name.
     */
    XrlCmdError finder_event_observer_0_1_xrl_target_death(
	// Input values,
	const string&	target_class,
	const string&	target_instance);

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
	const IPv4Net&	network,
	const IPv4&	nexthop);

    XrlCmdError static_routes_0_1_delete_route6(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv6Net&	network,
	const IPv6&	nexthop);

    /**
     * Add/replace/delete multicast routes (not MRIB)
     *
     * @param mcast_addr  Multicast-address to be routed.
     *
     * @param input_if  Input interface name.
     *
     * @param input_ip  Input interface IP address.
     *
     * @param output_ifs Output interface name(s).  Space-separated list.
     */
    XrlCmdError static_routes_0_1_add_mcast_route4(
        // Input values,
        const IPv4&     mcast_addr,
        const string&   input_if,
        const IPv4&     input_ip,
        const string&   output_ifs);

    XrlCmdError static_routes_0_1_replace_mcast_route4(
        // Input values,
        const IPv4&     mcast_addr,
        const string&   input_if,
        const IPv4&     input_ip,
        const string&   output_ifs);
    XrlCmdError static_routes_0_1_delete_mcast_route4(
        // Input values,
        const IPv4&     mcast_addr,
        const string&   input_if,
        const IPv4&     input_ip,
        const string&   output_ifs);


    /**
     *  Add/replace/delete a backup static route.
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
    XrlCmdError static_routes_0_1_add_backup_route4(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv4Net&	network,
	const IPv4&	nexthop,
	const uint32_t&	metric);

    XrlCmdError static_routes_0_1_add_backup_route6(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv6Net&	network,
	const IPv6&	nexthop,
	const uint32_t&	metric);

    XrlCmdError static_routes_0_1_replace_backup_route4(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv4Net&	network,
	const IPv4&	nexthop,
	const uint32_t&	metric);

    XrlCmdError static_routes_0_1_replace_backup_route6(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv6Net&	network,
	const IPv6&	nexthop,
	const uint32_t&	metric);

    XrlCmdError static_routes_0_1_delete_backup_route4(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv4Net&	network,
	const IPv4&	nexthop);

    XrlCmdError static_routes_0_1_delete_backup_route6(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv6Net&	network,
	const IPv6&	nexthop);

    /**
     *  Add/replace/delete a static route by explicitly specifying the network
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

    XrlCmdError static_routes_0_1_delete_interface_route4(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv4Net&	network,
	const IPv4&	nexthop,
	const string&	ifname,
	const string&	vifname);

    XrlCmdError static_routes_0_1_delete_interface_route6(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv6Net&	network,
	const IPv6&	nexthop,
	const string&	ifname,
	const string&	vifname);

    /**
     *  Add/replace/delete a backup static route by explicitly specifying the
     *  network interface toward the destination.
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
    XrlCmdError static_routes_0_1_add_backup_interface_route4(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv4Net&	network,
	const IPv4&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric);

    XrlCmdError static_routes_0_1_add_backup_interface_route6(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv6Net&	network,
	const IPv6&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric);

    XrlCmdError static_routes_0_1_replace_backup_interface_route4(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv4Net&	network,
	const IPv4&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric);

    XrlCmdError static_routes_0_1_replace_backup_interface_route6(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv6Net&	network,
	const IPv6&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric);

    XrlCmdError static_routes_0_1_delete_backup_interface_route4(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv4Net&	network,
	const IPv4&	nexthop,
	const string&	ifname,
	const string&	vifname);

    XrlCmdError static_routes_0_1_delete_backup_interface_route6(
	// Input values,
	const bool&	unicast,
	const bool&	multicast,
	const IPv6Net&	network,
	const IPv6&	nexthop,
	const string&	ifname,
	const string&	vifname);

    /**
     *  Enable/disable the StaticRoutes trace log for all operations.
     *
     *  @param enable if true, then enable the trace log, otherwise disable it.
     */
    XrlCmdError static_routes_0_1_enable_log_trace_all(
	// Input values,
	const bool&	enable);

    /**
     * Configure a policy filter.
     *
     * @param filter Id of filter to configure.
     * @param conf Configuration of filter.
     */
    XrlCmdError policy_backend_0_1_configure(
        // Input values,
        const uint32_t& filter,
        const string&   conf);

    /**
     * Reset a policy filter.
     *
     * @param filter Id of filter to reset.
     */
    XrlCmdError policy_backend_0_1_reset(
        // Input values,
        const uint32_t& filter);

    /**
     * Push routes through policy filters for re-filtering.
     */
    XrlCmdError policy_backend_0_1_push_routes();


private:
    const ServiceBase* ifmgr_mirror_service_base() const {
	return dynamic_cast<const ServiceBase*>(&_ifmgr);
    }
    const IfMgrIfTree& ifmgr_iftree() const { return _ifmgr.iftree(); }

    /**
     * Called when Finder connection is established.
     *
     * Note that this method overwrites an XrlRouter virtual method.
     */
    virtual void finder_connect_event();

    /**
     * Called when Finder disconnect occurs.
     *
     * Note that this method overwrites an XrlRouter virtual method.
     */
    virtual void finder_disconnect_event();

    void fea_register_startup();
    void finder_register_interest_fea_cb(const XrlError& xrl_error);
    void fea_register_shutdown();
    void finder_deregister_interest_fea_cb(const XrlError& xrl_error);

    void rib_register_startup();
    void finder_register_interest_rib_cb(const XrlError& xrl_error);
    void rib_register_shutdown();
    void finder_deregister_interest_rib_cb(const XrlError& xrl_error);
    void send_rib_add_tables();
    void rib_client_send_add_igp_table4_cb(const XrlError& xrl_error);
#ifdef HAVE_IPV6
    void rib_client_send_add_igp_table6_cb(const XrlError& xrl_error);
#endif
    void send_rib_delete_tables();
    void rib_client_send_delete_igp_table4_cb(const XrlError& xrl_error);
#ifdef HAVE_IPV6
    void rib_client_send_delete_igp_table6_cb(const XrlError& xrl_error);
#endif

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

    EventLoop&		_eventloop;
    XrlRibV0p1Client	_xrl_rib_client;
    const string	_finder_target;
    const string	_fea_target;
    const string	_rib_target;

    IfMgrXrlMirror	_ifmgr;
    list<StaticRoute>	_inform_rib_queue;
    XorpTimer		_inform_rib_queue_timer;
    XrlFinderEventNotifierV0p1Client	_xrl_finder_client;

    static const TimeVal RETRY_TIMEVAL;

    bool		_is_finder_alive;

    bool		_is_fea_alive;
    bool		_is_fea_registered;
    bool		_is_fea_registering;
    bool		_is_fea_deregistering;
    XorpTimer		_fea_register_startup_timer;
    XorpTimer		_fea_register_shutdown_timer;

    bool		_is_rib_alive;
    bool		_is_rib_registered;
    bool		_is_rib_registering;
    bool		_is_rib_deregistering;
    bool		_is_rib_igp_table4_registered;
#ifdef HAVE_IPV6
    bool		_is_rib_igp_table6_registered;
#endif
    XorpTimer		_rib_register_startup_timer;
    XorpTimer		_rib_register_shutdown_timer;
    XorpTimer		_rib_igp_table_registration_timer;
};

#endif // __STATIC_ROUTES_XRL_STATIC_ROUTES_NODE_HH__
