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

#ident "$XORP: xorp/fib2mrib/xrl_fib2mrib_node.cc,v 1.2 2004/03/15 23:36:31 pavlin Exp $"

#include "fib2mrib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/status_codes.h"

#include "libxipc/xrl_router.hh"

#include "fib2mrib_node.hh"
#include "xrl_fib2mrib_node.hh"


XrlFib2mribNode::XrlFib2mribNode(EventLoop& eventloop,
				 XrlRouter* xrl_router,
				 const string& fea_target,
				 const string& rib_target,
				 const IPv4& finder_host,
				 uint16_t finder_port)
    : Fib2mribNode(eventloop),
      XrlFib2mribTargetBase(xrl_router),
      _class_name(xrl_router->class_name()),
      _instance_name(xrl_router->instance_name()),
      _xrl_fea_fib_client(xrl_router),
      _xrl_rib_client(xrl_router),
      _fea_target(fea_target),
      _rib_target(rib_target),
      _ifmgr(eventloop, fea_target.c_str(), finder_host, finder_port),
      _is_fea_fib_client4_registered(false),
      _is_fea_fib_client6_registered(false),
      _is_rib_igp_table4_registered(false),
      _is_rib_igp_table6_registered(false)
{
    _ifmgr.attach_hint_observer(dynamic_cast<Fib2mribNode*>(this));
}

XrlFib2mribNode::~XrlFib2mribNode()
{
    Fib2mribNode::shutdown();

    _ifmgr.detach_hint_observer(dynamic_cast<Fib2mribNode*>(this));
}

void
XrlFib2mribNode::startup()
{
    Fib2mribNode::startup();
}

void
XrlFib2mribNode::shutdown()
{
    Fib2mribNode::shutdown();
}

void
XrlFib2mribNode::ifmgr_startup()
{
    // TODO: XXX: we should startup the ifmgr only if it hasn't started yet
    Fib2mribNode::incr_startup_requests_n();

    _ifmgr.startup();

    //
    // XXX: when the startup is completed, IfMgrHintObserver::tree_complete()
    // will be called.
    //
}

void
XrlFib2mribNode::ifmgr_shutdown()
{
    Fib2mribNode::incr_shutdown_requests_n();

    _ifmgr.shutdown();

    // TODO: XXX: PAVPAVPAV: use ServiceChangeObserverBase
    // to signal when the interface manager shutdown has completed.
    Fib2mribNode::decr_shutdown_requests_n();
}

void
XrlFib2mribNode::fea_fib_client_register_startup()
{
    if (! _is_fea_fib_client4_registered)
	Fib2mribNode::incr_startup_requests_n();
    if (! _is_fea_fib_client6_registered)
	Fib2mribNode::incr_startup_requests_n();

    send_fea_fib_client_registration();
}

void
XrlFib2mribNode::fea_fib_client_register_shutdown()
{
    if (_is_fea_fib_client4_registered)
	Fib2mribNode::incr_shutdown_requests_n();
    if (_is_fea_fib_client6_registered)
	Fib2mribNode::incr_shutdown_requests_n();

    send_fea_fib_client_deregistration();
}

void
XrlFib2mribNode::rib_register_startup()
{
    if (! _is_rib_igp_table4_registered)
	Fib2mribNode::incr_startup_requests_n();
    if (! _is_rib_igp_table6_registered)
	Fib2mribNode::incr_startup_requests_n();

    send_rib_registration();
}

void
XrlFib2mribNode::rib_register_shutdown()
{
    if (_is_rib_igp_table4_registered)
	Fib2mribNode::incr_shutdown_requests_n();
    if (_is_rib_igp_table6_registered)
	Fib2mribNode::incr_shutdown_requests_n();

    send_rib_deregistration();
}

//
// Register as an FEA FIB client
//
void
XrlFib2mribNode::send_fea_fib_client_registration()
{
    bool success = true;

    if (! _is_fea_fib_client4_registered) {
	bool success4;
	success4 = _xrl_fea_fib_client.send_add_fib_client4(
	    _fea_target.c_str(),
	    my_xrl_target_name(),
	    callback(this, &XrlFib2mribNode::send_add_fib_client4_cb));
	if (success4 != true) {
	    XLOG_ERROR("Failed to register IPv4 FIB client with the FEA. "
		"Will try again.");
	    success = false;
	}
    }

    if (! _is_fea_fib_client6_registered) {
	bool success6;
	success6 = _xrl_fea_fib_client.send_add_fib_client6(
	    _fea_target.c_str(),
	    my_xrl_target_name(),
	    callback(this, &XrlFib2mribNode::send_add_fib_client6_cb));
	if (success6 != true) {
	    XLOG_ERROR("Failed to register IPv6 FIB client with the FEA. "
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
    _fea_fib_client_registration_timer
	= Fib2mribNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlFib2mribNode::send_fea_fib_client_registration));
}

void
XrlFib2mribNode::send_add_fib_client4_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_fea_fib_client4_registered = true;
	Fib2mribNode::decr_startup_requests_n();
	return;
    }

    //
    // If an error, then start a timer to try again (unless the timer is
    // already running).
    // TODO: XXX: the timer value is hardcoded here!!
    //
    if (_fea_fib_client_registration_timer.scheduled())
	return;
    _fea_fib_client_registration_timer
	= Fib2mribNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlFib2mribNode::send_fea_fib_client_registration));
}

void
XrlFib2mribNode::send_add_fib_client6_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_fea_fib_client6_registered = true;
	Fib2mribNode::decr_startup_requests_n();
	return;
    }

    //
    // If an error, then start a timer to try again (unless the timer is
    // already running).
    // TODO: XXX: the timer value is hardcoded here!!
    //
    if (_fea_fib_client_registration_timer.scheduled())
	return;
    _fea_fib_client_registration_timer
	= Fib2mribNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlFib2mribNode::send_fea_fib_client_registration));
}

//
// De-register as an FEA FIB client
//
void
XrlFib2mribNode::send_fea_fib_client_deregistration()
{
    bool success = true;

    if (_is_fea_fib_client4_registered) {
	bool success4;
	success4 = _xrl_fea_fib_client.send_delete_fib_client4(
	    _fea_target.c_str(),
	    my_xrl_target_name(),
	    callback(this, &XrlFib2mribNode::send_delete_fib_client4_cb));
	if (success4 != true) {
	    XLOG_ERROR("Failed to deregister IPv4 FIB client with the FEA. "
		"Will give up.");
	    success = false;
	}
    }

    if (_is_fea_fib_client6_registered) {
	bool success6;
	success6 = _xrl_fea_fib_client.send_delete_fib_client6(
	    _fea_target.c_str(),
	    my_xrl_target_name(),
	    callback(this, &XrlFib2mribNode::send_delete_fib_client6_cb));
	if (success6 != true) {
	    XLOG_ERROR("Failed to deregister IPv6 FIB client with the FEA. "
		"Will give up.");
	    success = false;
	}
    }

    if (success)
	return;		// OK

    Fib2mribNode::set_status(FAILED);
    Fib2mribNode::update_status();
}

void
XrlFib2mribNode::send_delete_fib_client4_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_fea_fib_client4_registered = false;
	Fib2mribNode::decr_shutdown_requests_n();
	return;
    }

    XLOG_ERROR("Failed to deregister IPv4 FIB client with the FEA: %s. "
	       "Will give up.",
	       xrl_error.str().c_str());

    Fib2mribNode::set_status(FAILED);
    Fib2mribNode::update_status();
}

void
XrlFib2mribNode::send_delete_fib_client6_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_fea_fib_client6_registered = false;
	Fib2mribNode::decr_shutdown_requests_n();
	return;
    }

    XLOG_ERROR("Failed to deregister IPv6 FIB client with the FEA: %s. "
	       "Will give up.",
	       xrl_error.str().c_str());

    Fib2mribNode::set_status(FAILED);
    Fib2mribNode::update_status();
}

//
// Register with the RIB
//
void
XrlFib2mribNode::send_rib_registration()
{
    bool success = true;

    if (! _is_rib_igp_table4_registered) {
	bool success4;
	success4 = _xrl_rib_client.send_add_igp_table4(
	    _rib_target.c_str(),
	    Fib2mribNode::protocol_name(),
	    _class_name,
	    _instance_name,
	    false,	/* unicast */
	    true,	/* multicast */
	    callback(this, &XrlFib2mribNode::send_add_igp_table4_cb));
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
	    Fib2mribNode::protocol_name(),
	    _class_name,
	    _instance_name,
	    false,	/* unicast */
	    true,	/* multicast */
	    callback(this, &XrlFib2mribNode::send_add_igp_table6_cb));
	if (success6 != true) {
	    XLOG_ERROR("Failed to register IPv6 IGP table with the RIB. "
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
	= Fib2mribNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlFib2mribNode::send_rib_registration));
}

void
XrlFib2mribNode::send_add_igp_table4_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_rib_igp_table4_registered = true;
	Fib2mribNode::decr_startup_requests_n();
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
	= Fib2mribNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlFib2mribNode::send_rib_registration));
}

void
XrlFib2mribNode::send_add_igp_table6_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_rib_igp_table6_registered = true;
	Fib2mribNode::decr_startup_requests_n();
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
	= Fib2mribNode::eventloop().new_oneoff_after(
	    TimeVal(1, 0),
	    callback(this, &XrlFib2mribNode::send_rib_registration));
}

//
// De-register with the RIB
//
void
XrlFib2mribNode::send_rib_deregistration()
{
    bool success = true;

    if (_is_rib_igp_table4_registered) {
	bool success4;
	success4 = _xrl_rib_client.send_delete_igp_table4(
	    _rib_target.c_str(),
	    Fib2mribNode::protocol_name(),
	    _class_name,
	    _instance_name,
	    false,	/* unicast */
	    true,	/* multicast */
	    callback(this, &XrlFib2mribNode::send_delete_igp_table4_cb));
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
	    Fib2mribNode::protocol_name(),
	    _class_name,
	    _instance_name,
	    false,	/* unicast */
	    true,	/* multicast */
	    callback(this, &XrlFib2mribNode::send_delete_igp_table6_cb));
	if (success6 != true) {
	    XLOG_ERROR("Failed to deregister IPv6 IGP table with the RIB. "
		"Will give up.");
	    success = false;
	}
    }

    if (success)
	return;		// OK

    Fib2mribNode::set_status(FAILED);
    Fib2mribNode::update_status();
}

void
XrlFib2mribNode::send_delete_igp_table4_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_rib_igp_table4_registered = false;
	Fib2mribNode::decr_shutdown_requests_n();
	return;
    }

    XLOG_ERROR("Failed to deregister IPv4 IGP table with the RIB: %s. "
	       "Will give up.",
	       xrl_error.str().c_str());

    Fib2mribNode::set_status(FAILED);
    Fib2mribNode::update_status();
}

void
XrlFib2mribNode::send_delete_igp_table6_cb(const XrlError& xrl_error)
{
    // If success, then we are done
    if (xrl_error == XrlError::OKAY()) {
	_is_rib_igp_table6_registered = false;
	Fib2mribNode::decr_shutdown_requests_n();
	return;
    }

    XLOG_ERROR("Failed to deregister IPv6 IGP table with the RIB: %s. "
	       "Will give up.",
	       xrl_error.str().c_str());

    Fib2mribNode::set_status(FAILED);
    Fib2mribNode::update_status();
}


//
// XRL target methods
//

/**
 *  Get name of Xrl Target
 */
XrlCmdError
XrlFib2mribNode::common_0_1_get_target_name(
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
XrlFib2mribNode::common_0_1_get_version(
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
XrlFib2mribNode::common_0_1_get_status(
    // Output values, 
    uint32_t&	status, 
    string&	reason)
{
    status = Fib2mribNode::node_status(reason);

    return XrlCmdError::OKAY();
}

/**
 *  Request clean shutdown of Xrl Target
 */
XrlCmdError
XrlFib2mribNode::common_0_1_shutdown()
{
    Fib2mribNode::shutdown();

    return XrlCmdError::OKAY();
}

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
XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_add_route4(
    // Input values,
    const IPv4Net&	network,
    const IPv4&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	protocol_origin,
    const bool&		xorp_route)
{
    string error_msg;

    if (Fib2mribNode::add_route4(network, nexthop, ifname, vifname,
				 metric, admin_distance,
				 protocol_origin, xorp_route,
				 error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_add_route6(
    // Input values,
    const IPv6Net&	network,
    const IPv6&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	protocol_origin,
    const bool&	xorp_route)
{
    string error_msg;

    if (Fib2mribNode::add_route6(network, nexthop, ifname, vifname,
				 metric, admin_distance,
				 protocol_origin, xorp_route,
				 error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

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
XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_replace_route4(
    // Input values,
    const IPv4Net&	network,
    const IPv4&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	protocol_origin,
    const bool&		xorp_route)
{
    string error_msg;

    if (Fib2mribNode::replace_route4(network, nexthop, ifname, vifname,
				     metric, admin_distance,
				     protocol_origin, xorp_route,
				     error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_replace_route6(
    // Input values,
    const IPv6Net&	network,
    const IPv6&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	protocol_origin,
    const bool&	xorp_route)
{
    string error_msg;

    if (Fib2mribNode::replace_route6(network, nexthop, ifname, vifname,
				     metric, admin_distance,
				     protocol_origin, xorp_route,
				     error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

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
XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_delete_route4(
    // Input values,
    const IPv4Net&	network,
    const string&	ifname,
    const string&	vifname)
{
    string error_msg;

    if (Fib2mribNode::delete_route4(network, ifname, vifname, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFib2mribNode::fea_fib_client_0_1_delete_route6(
    // Input values,
    const IPv6Net&	network,
    const string&	ifname,
    const string&	vifname)
{
    string error_msg;

    if (Fib2mribNode::delete_route6(network, ifname, vifname, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

/**
 *  Enable/disable/start/stop Fib2mrib.
 *
 *  @param enable if true, then enable Fib2mrib, otherwise disable it.
 */
XrlCmdError
XrlFib2mribNode::fib2mrib_0_1_enable_fib2mrib(
    // Input values,
    const bool&	enable)
{
    UNUSED(enable);

    // XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFib2mribNode::fib2mrib_0_1_start_fib2mrib()
{
    // XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFib2mribNode::fib2mrib_0_1_stop_fib2mrib()
{
    XLOG_UNFINISHED();

    return XrlCmdError::OKAY();
}

/**
 *  Enable/disable the Fib2mrib trace log for all operations.
 *
 *  @param enable if true, then enable the trace log, otherwise disable it.
 */
XrlCmdError
XrlFib2mribNode::fib2mrib_0_1_enable_log_trace_all(
    // Input values,
    const bool&	enable)
{
    Fib2mribNode::set_log_trace(enable);

    return XrlCmdError::OKAY();
}

/**
 * Inform the RIB about a route change.
 *
 * @param fib2mrib_route the route with the information about the change.
 */
void
XrlFib2mribNode::inform_rib_route_change(const Fib2mribRoute& fib2mrib_route)
{
    // Add the request to the queue
    _inform_rib_queue.push_back(fib2mrib_route);

    // If the queue was empty before, start sending the routes
    if (_inform_rib_queue.size() == 1) {
	send_rib_route_change();
    }
}

void
XrlFib2mribNode::send_rib_route_change()
{
    bool success = false;

    if (_inform_rib_queue.empty())
	return;		// No more route changes to send

    Fib2mribRoute& fib2mrib_route = _inform_rib_queue.front();

    //
    // Check whether we have already registered with the RIB
    //
    if (fib2mrib_route.is_ipv4() && (! _is_rib_igp_table4_registered))
	goto error_label;

    if (fib2mrib_route.is_ipv6() && (! _is_rib_igp_table6_registered))
	goto error_label;

    //
    // Send the appropriate XRL
    //
    if (fib2mrib_route.is_add_route()) {
	if (fib2mrib_route.is_ipv4()) {
	    success = _xrl_rib_client.send_add_route4(
		_rib_target.c_str(),
		Fib2mribNode::protocol_name(),
		false,			/* unicast */
		true,			/* multicast */
		fib2mrib_route.network().get_ipv4net(),
		fib2mrib_route.nexthop().get_ipv4(),
		fib2mrib_route.metric(),
		callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
	}
	if (fib2mrib_route.is_ipv6()) {
	    success = _xrl_rib_client.send_add_route6(
		_rib_target.c_str(),
		Fib2mribNode::protocol_name(),
		false,			/* unicast */
		true,			/* multicast */
		fib2mrib_route.network().get_ipv6net(),
		fib2mrib_route.nexthop().get_ipv6(),
		fib2mrib_route.metric(),
		callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
	}
    }

    if (fib2mrib_route.is_replace_route()) {
	if (fib2mrib_route.is_ipv4()) {
	    success = _xrl_rib_client.send_replace_route4(
		_rib_target.c_str(),
		Fib2mribNode::protocol_name(),
		false,			/* unicast */
		true,			/* multicast */
		fib2mrib_route.network().get_ipv4net(),
		fib2mrib_route.nexthop().get_ipv4(),
		fib2mrib_route.metric(),
		callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
	}
	if (fib2mrib_route.is_ipv6()) {
	    success = _xrl_rib_client.send_replace_route6(
		_rib_target.c_str(),
		Fib2mribNode::protocol_name(),
		false,			/* unicast */
		true,			/* multicast */
		fib2mrib_route.network().get_ipv6net(),
		fib2mrib_route.nexthop().get_ipv6(),
		fib2mrib_route.metric(),
		callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
	}
    }

    if (fib2mrib_route.is_delete_route()) {
	if (fib2mrib_route.is_ipv4()) {
	    success = _xrl_rib_client.send_delete_route4(
		_rib_target.c_str(),
		Fib2mribNode::protocol_name(),
		false,			/* unicast */
		true,			/* multicast */
		fib2mrib_route.network().get_ipv4net(),
		callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
	}
	if (fib2mrib_route.is_ipv6()) {
	    success = _xrl_rib_client.send_delete_route6(
		_rib_target.c_str(),
		Fib2mribNode::protocol_name(),
		false,			/* unicast */
		true,			/* multicast */
		fib2mrib_route.network().get_ipv6net(),
		callback(this, &XrlFib2mribNode::send_rib_route_change_cb));
	}
    }

    if (success)
	return;		// OK

 error_label:
    //
    // If an error, then start a timer to try again
    // TODO: XXX: the timer value is hardcoded here!!
    //
    _inform_rib_queue_timer = Fib2mribNode::eventloop().new_oneoff_after(
	TimeVal(1, 0),
	callback(this, &XrlFib2mribNode::send_rib_route_change));
}

void
XrlFib2mribNode::send_rib_route_change_cb(const XrlError& xrl_error)
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
    _inform_rib_queue_timer = Fib2mribNode::eventloop().new_oneoff_after(
	TimeVal(1, 0),
	callback(this, &XrlFib2mribNode::send_rib_route_change));
}
