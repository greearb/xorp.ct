// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/fib2mrib/xrl_fib2mrib_node.hh,v 1.24 2008/10/02 21:57:14 bms Exp $

#ifndef __FIB2MRIB_XRL_FIB2MRIB_NODE_HH__
#define __FIB2MRIB_XRL_FIB2MRIB_NODE_HH__


//
// Fib2mrib XRL-aware node definition.
//

#include "libxipc/xrl_std_router.hh"

#include "libfeaclient/ifmgr_xrl_mirror.hh"

#include "xrl/interfaces/finder_event_notifier_xif.hh"
#include "xrl/interfaces/fti_xif.hh"
#include "xrl/interfaces/fea_fib_xif.hh"
#include "xrl/interfaces/rib_xif.hh"
#include "xrl/targets/fib2mrib_base.hh"

#include "fib2mrib_node.hh"


//
// The top-level class that wraps-up everything together under one roof
//
class XrlFib2mribNode : public Fib2mribNode,
			public XrlStdRouter,
			public XrlFib2mribTargetBase {
public:
    XrlFib2mribNode(EventLoop&		eventloop,
		    const string&	class_name,
		    const string&	finder_hostname,
		    uint16_t		finder_port,
		    const string&	finder_target,
		    const string&	fea_target,
		    const string&	rib_target);
    ~XrlFib2mribNode();

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
     *  Add a route.
     *
     *  @param network the network address prefix of the route to add.
     *
     *  @param nexthop the address of the next-hop router toward the
     *  destination.
     *
     *  @param ifname the name of the physical interface toward the
     *  destination.
     *
     *  @param vifname the name of the virtual interface toward the
     *  destination.
     *
     *  @param metric the routing metric toward the destination.
     *
     *  @param admin_distance the administratively defined distance toward the
     *  destination.
     *
     *  @param protocol_origin the name of the protocol that originated this
     *  route.
     *
     *  @param xorp_route true if this route was installed by XORP.
     */
    XrlCmdError fea_fib_client_0_1_add_route4(
	// Input values,
	const IPv4Net&	network,
	const IPv4&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	protocol_origin,
	const bool&	xorp_route);

    /**
     *  Replace a route.
     *
     *  @param network the network address prefix of the route to replace.
     *
     *  @param nexthop the address of the next-hop router toward the
     *  destination.
     *
     *  @param ifname the name of the physical interface toward the
     *  destination.
     *
     *  @param vifname the name of the virtual interface toward the
     *  destination.
     *
     *  @param metric the routing metric toward the destination.
     *
     *  @param admin_distance the administratively defined distance toward the
     *  destination.
     *
     *  @param protocol_origin the name of the protocol that originated this
     *  route.
     *
     *  @param xorp_route true if this route was installed by XORP.
     */
    XrlCmdError fea_fib_client_0_1_replace_route4(
	// Input values,
	const IPv4Net&	network,
	const IPv4&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	protocol_origin,
	const bool&	xorp_route);

    /**
     *  Delete a route.
     *
     *  @param network the network address prefix of the route to delete.
     *
     *  @param ifname the name of the physical interface toward the
     *  destination.
     *
     *  @param vifname the name of the virtual interface toward the
     *  destination.
     */
    XrlCmdError fea_fib_client_0_1_delete_route4(
	// Input values,
	const IPv4Net&	network,
	const string&	ifname,
	const string&	vifname);

    /**
     *  Route resolve notification.
     *
     *  @param network the network address prefix of the lookup
     *  which failed or for which upper layer intervention is
     *  requested from the FIB.
     */
    XrlCmdError fea_fib_client_0_1_resolve_route4(
	// Input values,
	const IPv4Net&	network);

    /**
     *  Enable/disable/start/stop Fib2mrib.
     *
     *  @param enable if true, then enable Fib2mrib, otherwise disable it.
     */
    XrlCmdError fib2mrib_0_1_enable_fib2mrib(
	// Input values,
	const bool&	enable);

    XrlCmdError fib2mrib_0_1_start_fib2mrib();

    XrlCmdError fib2mrib_0_1_stop_fib2mrib();

    /**
     *  Enable/disable the Fib2mrib trace log for all operations.
     *
     *  @param enable if true, then enable the trace log, otherwise disable it.
     */
    XrlCmdError fib2mrib_0_1_enable_log_trace_all(
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


#ifdef HAVE_IPV6
    XrlCmdError fea_fib_client_0_1_add_route6(
	// Input values,
	const IPv6Net&	network,
	const IPv6&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	protocol_origin,
	const bool&	xorp_route);


    XrlCmdError fea_fib_client_0_1_replace_route6(
	// Input values,
	const IPv6Net&	network,
	const IPv6&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	protocol_origin,
	const bool&	xorp_route);

    XrlCmdError fea_fib_client_0_1_delete_route6(
	// Input values,
	const IPv6Net&	network,
	const string&	ifname,
	const string&	vifname);

    XrlCmdError fea_fib_client_0_1_resolve_route6(
	// Input values,
	const IPv6Net&	network);

#endif

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

    void send_fea_add_fib_client();
    void fea_fti_client_send_have_ipv4_cb(const XrlError& xrl_error,
					  const bool* result);
#ifdef HAVE_IPV6
    void fea_fti_client_send_have_ipv6_cb(const XrlError& xrl_error,
					  const bool* result);
    void fea_fib_client_send_add_fib_client6_cb(const XrlError& xrl_error);
    void fea_fib_client_send_delete_fib_client6_cb(const XrlError& xrl_error);
    void rib_client_send_add_igp_table6_cb(const XrlError& xrl_error);
    void rib_client_send_delete_igp_table6_cb(const XrlError& xrl_error);
#endif

    void fea_fib_client_send_add_fib_client4_cb(const XrlError& xrl_error);
    void send_fea_delete_fib_client();
    void fea_fib_client_send_delete_fib_client4_cb(const XrlError& xrl_error);

    void rib_register_startup();
    void finder_register_interest_rib_cb(const XrlError& xrl_error);
    void rib_register_shutdown();
    void finder_deregister_interest_rib_cb(const XrlError& xrl_error);
    void send_rib_add_tables();
    void rib_client_send_add_igp_table4_cb(const XrlError& xrl_error);
    void send_rib_delete_tables();
    void rib_client_send_delete_igp_table4_cb(const XrlError& xrl_error);

    /**
     * Inform the RIB about a route change.
     *
     * @param fib2mrib_route the route with the information about the change.
     */
    void inform_rib_route_change(const Fib2mribRoute& fib2mrib_route);

    /**
     * Cancel a pending request to inform the RIB about a route change.
     *
     * @param fib2mrib_route the route with the request that would be canceled.
     */
    void cancel_rib_route_change(const Fib2mribRoute& fib2mrib_route);

    void send_rib_route_change();
    void send_rib_route_change_cb(const XrlError& xrl_error);

    EventLoop&		_eventloop;
    XrlFtiV0p2Client	_xrl_fea_fti_client;
    XrlFeaFibV0p1Client	_xrl_fea_fib_client;
    XrlRibV0p1Client	_xrl_rib_client;
    const string	_finder_target;
    const string	_fea_target;
    const string	_rib_target;

    IfMgrXrlMirror	_ifmgr;
    list<Fib2mribRoute>	_inform_rib_queue;
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

#ifdef HAVE_IPV6
    bool		_is_fea_have_ipv6_tested;
    bool		_fea_have_ipv6;
    bool		_is_fea_fib_client6_registered;
    bool		_is_rib_igp_table6_registered;
#endif

    bool		_is_fea_have_ipv4_tested;
    bool		_fea_have_ipv4;
    bool		_is_fea_fib_client4_registered;
    XorpTimer		_fea_fib_client_registration_timer;

    bool		_is_rib_alive;
    bool		_is_rib_registered;
    bool		_is_rib_registering;
    bool		_is_rib_deregistering;
    bool		_is_rib_igp_table4_registered;
    XorpTimer		_rib_register_startup_timer;
    XorpTimer		_rib_register_shutdown_timer;
    XorpTimer		_rib_igp_table_registration_timer;
};

#endif // __FIB2MRIB_XRL_FIB2MRIB_NODE_HH__
