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

#ident "$XORP: xorp/static_routes/xrl_static_routes_node.cc,v 1.3 2004/02/14 00:08:00 pavlin Exp $"

#include "static_routes_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/status_codes.h"

#include "libxipc/xrl_router.hh"

#include "static_routes_node.hh"
#include "xrl_static_routes_node.hh"


XrlStaticRoutesNode::XrlStaticRoutesNode(EventLoop& eventloop,
					 XrlRouter* xrl_router,
					 const string& fea_target,
					 const string& rib_target,
					 const IPv4& finder_host,
					 uint16_t finder_port)
    : StaticRoutesNode(eventloop),
      XrlStaticRoutesTargetBase(xrl_router),
      _class_name(xrl_router->class_name()),
      _instance_name(xrl_router->instance_name()),
      _xrl_rib_client(xrl_router),
      _fea_target(fea_target),
      _rib_target(rib_target),
      _ifmgr(eventloop, fea_target.c_str(), finder_host, finder_port),
      _is_rib_igp_table4_registered(false),
      _is_rib_igp_table6_registered(false)
{
    _ifmgr.attach_hint_observer(dynamic_cast<StaticRoutesNode*>(this));
}

XrlStaticRoutesNode::~XrlStaticRoutesNode()
{
    StaticRoutesNode::shutdown();

    _ifmgr.detach_hint_observer(dynamic_cast<StaticRoutesNode*>(this));
}

void
XrlStaticRoutesNode::startup()
{
    StaticRoutesNode::startup();
}

void
XrlStaticRoutesNode::shutdown()
{
    StaticRoutesNode::shutdown();
}

void
XrlStaticRoutesNode::ifmgr_startup()
{
    // TODO: XXX: we should startup the ifmgr only if it hasn't started yet
    StaticRoutesNode::incr_startup_requests_n();

    _ifmgr.startup();

    //
    // XXX: when the startup is completed, IfMgrHintObserver::tree_complete()
    // will be called.
    //
}

void
XrlStaticRoutesNode::ifmgr_shutdown()
{
    StaticRoutesNode::incr_shutdown_requests_n();

    _ifmgr.shutdown();

    // TODO: XXX: PAVPAVPAV: use ServiceChangeObserverBase
    // to signal when the interface manager shutdown has completed.
    StaticRoutesNode::decr_shutdown_requests_n();
}

void
XrlStaticRoutesNode::rib_register_startup()
{
    if (! _is_rib_igp_table4_registered)
	StaticRoutesNode::incr_startup_requests_n();
    if (! _is_rib_igp_table6_registered)
	StaticRoutesNode::incr_startup_requests_n();

    send_rib_registration();
}

void
XrlStaticRoutesNode::rib_register_shutdown()
{
    if (_is_rib_igp_table4_registered)
	StaticRoutesNode::incr_shutdown_requests_n();
    if (_is_rib_igp_table6_registered)
	StaticRoutesNode::incr_shutdown_requests_n();

    send_rib_deregistration();
}

//
// Register with the RIB
//
void
XrlStaticRoutesNode::send_rib_registration()
{
    bool success = true;

    if (! _is_rib_igp_table4_registered) {
	bool success4;
	success4 = _xrl_rib_client.send_add_igp_table4(
	    _rib_target.c_str(),
	    StaticRoutesNode::protocol_name(),
	    _class_name,
	    _instance_name,
	    true,	/* unicast */
	    true,	/* multicast */
	    callback(this, &XrlStaticRoutesNode::send_add_igp_table4_cb));
	if (success4 != true) {
	    XLOG_ERROR("Failed to register IPv4 IGP table with the RIB. "
		"Will try again.");
	    success = false;
	}
    }

    if (! _is_rib_igp_table6_registered) {
	bool success6;
	success6 = _xrl_rib_client.send_add_igp_table6(
	    _rib_target.c_str(),
	    StaticRoutesNode::protocol_name(),
	    _class_name,
	    _instance_name,
	    true,	/* unicast */
	    true,	/* multicast */
	    callback(this, &XrlStaticRoutesNode::send_add_igp_table6_cb));
	if (success6 != true) {
	    XLOG_ERROR("Failed to register IPv4 IGP table with the RIB. "
		"Will try again.");
	    success = false;
	}
    }

    if (success)
	return;		// OK

    //
    // If an error, then start a timer to try again
    // TODO: XXX: the timer value is hardcoded here!!
    //
    _rib_igp_table_registration_timer
	= StaticRoutesNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlStaticRoutesNode::send_rib_registration));
}

void
XrlStaticRoutesNode::send_add_igp_table4_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_rib_igp_table4_registered = true;
	StaticRoutesNode::decr_startup_requests_n();
	return;
    }

    //
    // If an error, then start a timer to try again (unless the timer is
    // already running).
    // TODO: XXX: the timer value is hardcoded here!!
    //
    if (_rib_igp_table_registration_timer.scheduled())
	return;
    _rib_igp_table_registration_timer
	= StaticRoutesNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlStaticRoutesNode::send_rib_registration));
}

void
XrlStaticRoutesNode::send_add_igp_table6_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_rib_igp_table6_registered = true;
	StaticRoutesNode::decr_startup_requests_n();
	return;
    }

    //
    // If an error, then start a timer to try again (unless the timer is
    // already running).
    // TODO: XXX: the timer value is hardcoded here!!
    //
    if (_rib_igp_table_registration_timer.scheduled())
	return;
    _rib_igp_table_registration_timer
	= StaticRoutesNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlStaticRoutesNode::send_rib_registration));
}

//
// De-register with the RIB
//
void
XrlStaticRoutesNode::send_rib_deregistration()
{
    bool success = true;

    if (_is_rib_igp_table4_registered) {
	bool success4;
	success4 = _xrl_rib_client.send_delete_igp_table4(
	    _rib_target.c_str(),
	    StaticRoutesNode::protocol_name(),
	    _class_name,
	    _instance_name,
	    true,	/* unicast */
	    true,	/* multicast */
	    callback(this, &XrlStaticRoutesNode::send_delete_igp_table4_cb));
	if (success4 != true) {
	    XLOG_ERROR("Failed to deregister IPv4 IGP table with the RIB. "
		"Will give up.");
	    success = false;
	}
    }

    if (_is_rib_igp_table4_registered) {
	bool success6;
	success6 = _xrl_rib_client.send_delete_igp_table6(
	    _rib_target.c_str(),
	    StaticRoutesNode::protocol_name(),
	    _class_name,
	    _instance_name,
	    true,	/* unicast */
	    true,	/* multicast */
	    callback(this, &XrlStaticRoutesNode::send_delete_igp_table6_cb));
	if (success6 != true) {
	    XLOG_ERROR("Failed to deregister IPv6 IGP table with the RIB. "
		"Will give up.");
	    success = false;
	}
    }

    if (success)
	return;		// OK

    StaticRoutesNode::set_status(FAILED);
    StaticRoutesNode::update_status();
}

void
XrlStaticRoutesNode::send_delete_igp_table4_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_rib_igp_table4_registered = false;
	StaticRoutesNode::decr_shutdown_requests_n();
	return;
    }

    XLOG_ERROR("Failed to deregister IPv4 IGP table with the RIB: %s. "
	       "Will give up.",
	       xrl_error.str().c_str());

    StaticRoutesNode::set_status(FAILED);
    StaticRoutesNode::update_status();
}

void
XrlStaticRoutesNode::send_delete_igp_table6_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_rib_igp_table6_registered = false;
	StaticRoutesNode::decr_shutdown_requests_n();
	return;
    }

    XLOG_ERROR("Failed to deregister IPv6 IGP table with the RIB: %s. "
	       "Will give up.",
	       xrl_error.str().c_str());

    StaticRoutesNode::set_status(FAILED);
    StaticRoutesNode::update_status();
}


//
// XRL target methods
//

/**
 *  Get name of Xrl Target
 */
XrlCmdError
XrlStaticRoutesNode::common_0_1_get_target_name(
    // Output values, 
    string&	name)
{
    name = my_xrl_target_name();

    return XrlCmdError::OKAY();
}

/**
 *  Get version string from Xrl Target
 */
XrlCmdError
XrlStaticRoutesNode::common_0_1_get_version(
    // Output values, 
    string&	version)
{
    version = XORP_MODULE_VERSION;

    return XrlCmdError::OKAY();
}

/**
 *  Get status of Xrl Target
 */
XrlCmdError
XrlStaticRoutesNode::common_0_1_get_status(
    // Output values, 
    uint32_t&	status, 
    string&	reason)
{
    status = StaticRoutesNode::node_status(reason);

    return XrlCmdError::OKAY();
}

/**
 *  Request clean shutdown of Xrl Target
 */
XrlCmdError
XrlStaticRoutesNode::common_0_1_shutdown()
{
    StaticRoutesNode::shutdown();

    return XrlCmdError::OKAY();
}

/**
 *  Enable/disable/start/stop StaticRoutes.
 *
 *  @param enable if true, then enable StaticRoutes, otherwise disable it.
 */
XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_enable_static_routes(
    // Input values,
    const bool&	enable)
{
    UNUSED(enable);

    // XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_start_static_routes()
{
    // XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_stop_static_routes()
{
    XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

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
XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_add_route4(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv4Net&	network,
    const IPv4&		nexthop,
    const uint32_t&	metric)
{
    string error_msg;

    if (StaticRoutesNode::add_route4(unicast, multicast, network, nexthop,
				     metric, error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_add_route6(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv6Net&	network,
    const IPv6&		nexthop,
    const uint32_t&	metric)
{
    string error_msg;

    if (StaticRoutesNode::add_route6(unicast, multicast, network, nexthop,
				     metric, error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}


XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_replace_route4(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv4Net&	network,
    const IPv4&		nexthop,
    const uint32_t&	metric)
{
    string error_msg;

    if (StaticRoutesNode::replace_route4(unicast, multicast, network, nexthop,
					 metric, error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_replace_route6(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv6Net&	network,
    const IPv6&		nexthop,
    const uint32_t&	metric)
{
    string error_msg;

    if (StaticRoutesNode::replace_route6(unicast, multicast, network, nexthop,
					 metric, error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_delete_route4(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv4Net&	network)
{
    string error_msg;

    if (StaticRoutesNode::delete_route4(unicast, multicast, network,
					error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_delete_route6(
    // Input values,
    const bool&		unicast,
    const bool&		multicast,
    const IPv6Net&	network)
{
    string error_msg;

    if (StaticRoutesNode::delete_route6(unicast, multicast, network,
					error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

/**
 *  Enable/disable the StaticRoutes trace log for all operations.
 *
 *  @param enable if true, then enable the trace log, otherwise disable it.
 */
XrlCmdError
XrlStaticRoutesNode::static_routes_0_1_enable_log_trace_all(
    // Input values,
    const bool&	enable)
{
    StaticRoutesNode::set_log_trace(enable);

    return XrlCmdError::OKAY();
}

/**
 * Inform the RIB about a route change.
 *
 * @param static_route the route with the information about the change.
 */
void
XrlStaticRoutesNode::inform_rib_route_change(const StaticRoute& static_route)
{
    // Add the request to the queue
    _inform_rib_queue.push_back(static_route);

    // If the queue was empty before, start sending the routes
    if (_inform_rib_queue.size() == 1) {
	send_rib_route_change();
    }
}

void
XrlStaticRoutesNode::send_rib_route_change()
{
    bool success = false;

    if (_inform_rib_queue.empty())
	return;		// No more route changes to send

    StaticRoute& static_route = _inform_rib_queue.front();

    //
    // Check whether we have already registered with the RIB
    //
    if (static_route.is_ipv4() && (! _is_rib_igp_table4_registered))
	goto error_label;

    if (static_route.is_ipv6() && (! _is_rib_igp_table6_registered))
	goto error_label;

    //
    // Send the appropriate XRL
    //
    if (static_route.is_add_route()) {
	if (static_route.is_ipv4()) {
	    success = _xrl_rib_client.send_add_route4(
		_rib_target.c_str(),
		StaticRoutesNode::protocol_name(),
		static_route.unicast(),
		static_route.multicast(),
		static_route.network().get_ipv4net(),
		static_route.nexthop().get_ipv4(),
		static_route.metric(),
		callback(this, &XrlStaticRoutesNode::send_rib_route_change_cb));
	}
	if (static_route.is_ipv6()) {
	    success = _xrl_rib_client.send_add_route6(
		_rib_target.c_str(),
		StaticRoutesNode::protocol_name(),
		static_route.unicast(),
		static_route.multicast(),
		static_route.network().get_ipv6net(),
		static_route.nexthop().get_ipv6(),
		static_route.metric(),
		callback(this, &XrlStaticRoutesNode::send_rib_route_change_cb));
	}
    }

    if (static_route.is_replace_route()) {
	if (static_route.is_ipv4()) {
	    success = _xrl_rib_client.send_replace_route4(
		_rib_target.c_str(),
		StaticRoutesNode::protocol_name(),
		static_route.unicast(),
		static_route.multicast(),
		static_route.network().get_ipv4net(),
		static_route.nexthop().get_ipv4(),
		static_route.metric(),
		callback(this, &XrlStaticRoutesNode::send_rib_route_change_cb));
	}
	if (static_route.is_ipv6()) {
	    success = _xrl_rib_client.send_replace_route6(
		_rib_target.c_str(),
		StaticRoutesNode::protocol_name(),
		static_route.unicast(),
		static_route.multicast(),
		static_route.network().get_ipv6net(),
		static_route.nexthop().get_ipv6(),
		static_route.metric(),
		callback(this, &XrlStaticRoutesNode::send_rib_route_change_cb));
	}
    }

    if (static_route.is_delete_route()) {
	if (static_route.is_ipv4()) {
	    success = _xrl_rib_client.send_delete_route4(
		_rib_target.c_str(),
		StaticRoutesNode::protocol_name(),
		static_route.unicast(),
		static_route.multicast(),
		static_route.network().get_ipv4net(),
		callback(this, &XrlStaticRoutesNode::send_rib_route_change_cb));
	}
	if (static_route.is_ipv6()) {
	    success = _xrl_rib_client.send_delete_route6(
		_rib_target.c_str(),
		StaticRoutesNode::protocol_name(),
		static_route.unicast(),
		static_route.multicast(),
		static_route.network().get_ipv6net(),
		callback(this, &XrlStaticRoutesNode::send_rib_route_change_cb));
	}
    }

    if (success)
	return;		// OK

 error_label:
    //
    // If an error, then start a timer to try again
    // TODO: XXX: the timer value is hardcoded here!!
    //
    _inform_rib_queue_timer = StaticRoutesNode::eventloop().new_oneoff_after(
	TimeVal(1, 0),
	callback(this, &XrlStaticRoutesNode::send_rib_route_change));
}

void
XrlStaticRoutesNode::send_rib_route_change_cb(const XrlError& xrl_error)
{
    // If success, then send the next route change
    if (xrl_error == XrlError::OKAY()) {
	_inform_rib_queue.pop_front();
	send_rib_route_change();
	return;
    }

    //
    // If an error, then start a timer to try again
    // TODO: XXX: the timer value is hardcoded here!!
    //
    _inform_rib_queue_timer = StaticRoutesNode::eventloop().new_oneoff_after(
	TimeVal(1, 0),
	callback(this, &XrlStaticRoutesNode::send_rib_route_change));
}
