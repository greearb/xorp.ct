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

// $XORP: xorp/fib2mrib/xrl_fib2mrib_node.hh,v 1.1 2004/02/18 00:10:56 pavlin Exp $

#ifndef __FIB2MRIB_XRL_FIB2MRIB_NODE_HH__
#define __FIB2MRIB_XRL_FIB2MRIB_NODE_HH__


//
// Fib2mrib XRL-aware node definition.
//

#include "libfeaclient/ifmgr_xrl_mirror.hh"

#include "xrl/interfaces/fea_fib_xif.hh"
#include "xrl/interfaces/rib_xif.hh"
#include "xrl/targets/fib2mrib_base.hh"

#include "fib2mrib_node.hh"


class XrlRouter;

//
// The top-level class that wraps-up everything together under one roof
//
class XrlFib2mribNode : public Fib2mribNode,
			public XrlFib2mribTargetBase {
public:
    XrlFib2mribNode(EventLoop& eventloop,
		    XrlRouter* xrl_router,
		    const string& fea_target,
		    const string& rib_target,
		    const IPv4& finder_host,
		    uint16_t finder_port);
    ~XrlFib2mribNode();

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

    /**
     *  Delete a route.
     *
     *  @param network the network address prefix of the route to delete.
     *
     *  @param ifname the name of the physical interface toward the
     *  destination.
     *
     *  @param vifname the name of the virtual interface toward the
     */
    XrlCmdError fea_fib_client_0_1_delete_route4(
	// Input values,
	const IPv4Net&	network,
	const string&	ifname,
	const string&	vifname);

    XrlCmdError fea_fib_client_0_1_delete_route6(
	// Input values,
	const IPv6Net&	network,
	const string&	ifname,
	const string&	vifname);

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

private:

    void ifmgr_startup();
    void ifmgr_shutdown();

    const IfMgrIfTree& iftree() const { return _ifmgr.iftree(); }

    void fea_fib_client_register_startup();
    void fea_fib_client_register_shutdown();
    void rib_register_startup();
    void rib_register_shutdown();

    /**
     * Inform the RIB about a route change.
     *
     * @param fib2mrib_route the route with the information about the change.
     */
    void inform_rib_route_change(const Fib2mribRoute& fib2mrib_route);

    void send_rib_route_change();
    void send_rib_route_change_cb(const XrlError& xrl_error);

    void send_fea_fib_client_registration();
    void send_add_fib_client4_cb(const XrlError& xrl_error);
    void send_add_fib_client6_cb(const XrlError& xrl_error);
    void send_fea_fib_client_deregistration();
    void send_delete_fib_client4_cb(const XrlError& xrl_error);
    void send_delete_fib_client6_cb(const XrlError& xrl_error);

    void send_rib_registration();
    void send_add_igp_table4_cb(const XrlError& xrl_error);
    void send_add_igp_table6_cb(const XrlError& xrl_error);
    void send_rib_deregistration();
    void send_delete_igp_table4_cb(const XrlError& xrl_error);
    void send_delete_igp_table6_cb(const XrlError& xrl_error);

    const string& my_xrl_target_name() {
	return XrlFib2mribTargetBase::name();
    }

    const string	_class_name;
    const string	_instance_name;
    XrlFeaFibV0p1Client	_xrl_fea_fib_client;
    XrlRibV0p1Client	_xrl_rib_client;
    const string	_fea_target;
    const string	_rib_target;
    IfMgrXrlMirror	_ifmgr;

    list<Fib2mribRoute>	_inform_rib_queue;
    XorpTimer		_inform_rib_queue_timer;
    bool		_is_fea_fib_client4_registered;
    bool		_is_fea_fib_client6_registered;
    XorpTimer		_fea_fib_client_registration_timer;
    bool		_is_rib_igp_table4_registered;
    bool		_is_rib_igp_table6_registered;
    XorpTimer		_rib_igp_table_registration_timer;
};

#endif // __FIB2MRIB_XRL_FIB2MRIB_NODE_HH__
