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

#ident "$XORP: xorp/static_routes/static_routes_node.cc,v 1.2 2004/02/14 00:09:20 pavlin Exp $"


//
// StaticRoutes node implementation.
//


#include "static_routes_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "static_routes_node.hh"


StaticRoutesNode::StaticRoutesNode(EventLoop& eventloop)
    : ServiceBase("StaticRoutes"),
      _eventloop(eventloop),
      _protocol_name("static"),		// TODO: must be known by RIB
      _startup_requests_n(0),
      _shutdown_requests_n(0),
      _is_log_trace(true)		// XXX: default to print trace logs
{
    //
    // Set the node status
    //
    _node_status = PROC_STARTUP;
}

StaticRoutesNode::~StaticRoutesNode()
{
    if ((ServiceBase::status() != SHUTDOWN)
	&& (ServiceBase::status() != FAILED)) {
	shutdown();
    }
}

void
StaticRoutesNode::startup()
{
    //
    // Transition to RUNNING occurs when all transient startup operations
    // are completed (e.g., after we have the interface/vif/address state
    // available, after we have registered with the RIB, etc.
    //
    ServiceBase::set_status(STARTING);

    //
    // Set the node status
    //
    _node_status = PROC_STARTUP;

    //
    // Startup the interface manager
    //
    ifmgr_startup();

    //
    // Register with the RIB
    //
    rib_register_startup();
}

void
StaticRoutesNode::shutdown()
{
    //
    // Transition to SHUTDOWN occurs when all transient shutdown operations
    // are completed (e.g., after we have deregistered with the FEA
    // and the RIB, etc.)
    //
    ServiceBase::set_status(SHUTTING_DOWN);

    // De-register with the RIB
    rib_register_shutdown();

    //
    // Shutdown the interface manager
    //
    ifmgr_shutdown();

    //
    // Set the node status
    //
    _node_status = PROC_SHUTDOWN;
}

void
StaticRoutesNode::incr_startup_requests_n()
{
    _startup_requests_n++;
    XLOG_ASSERT(_startup_requests_n > 0);
}

void
StaticRoutesNode::decr_startup_requests_n()
{
    XLOG_ASSERT(_startup_requests_n > 0);
    _startup_requests_n--;

    update_status();
}

void
StaticRoutesNode::incr_shutdown_requests_n()
{
    _shutdown_requests_n++;
    XLOG_ASSERT(_shutdown_requests_n > 0);
}

void
StaticRoutesNode::decr_shutdown_requests_n()
{
    XLOG_ASSERT(_shutdown_requests_n > 0);
    _shutdown_requests_n--;

    update_status();
}

void
StaticRoutesNode::update_status()
{
    //
    // Test if the startup process has completed
    //
    if (ServiceBase::status() == STARTING) {
	if (_startup_requests_n > 0)
	    return;

	// The startup process has completed
	ServiceBase::set_status(RUNNING);
	_node_status = PROC_READY;
	return;
    }

    //
    // Test if the shutdown process has completed
    //
    if (ServiceBase::status() == SHUTTING_DOWN) {
	if (_shutdown_requests_n > 0)
	    return;

	// The shutdown process has completed
	ServiceBase::set_status(SHUTDOWN);
	// Set the node status
	_node_status = PROC_DONE;
	return;
    }

    //
    // Test if we have failed
    //
    if (ServiceBase::status() == FAILED) {
	// Set the node status
	_node_status = PROC_DONE;
	return;
    }
}

void
StaticRoutesNode::tree_complete()
{
    decr_startup_requests_n();
}

void
StaticRoutesNode::updates_made()
{
    // TODO: XXX: PAVPAVPAV: implement it!!
}

ProcessStatus
StaticRoutesNode::node_status(string& reason_msg)
{
    ProcessStatus status = _node_status;

    // Set the return message with the reason
    reason_msg = "";
    switch (status) {
    case PROC_NULL:
	// Can't be running and in this state
	XLOG_UNREACHABLE();
	break;
    case PROC_STARTUP:
	// Get the message about the startup progress
	reason_msg = c_format("Waiting for %u startup events",
			      reinterpret_cast<uint32_t>(_startup_requests_n));
	break;
    case PROC_NOT_READY:
	// XXX: this state is unused
	XLOG_UNREACHABLE();
	break;
    case PROC_READY:
	reason_msg = c_format("Node is READY");
	break;
    case PROC_SHUTDOWN:
	// Get the message about the shutdown progress
	reason_msg = c_format("Waiting for %u shutdown events",
			      reinterpret_cast<uint32_t>(_shutdown_requests_n));
	break;
    case PROC_FAILED:
	// XXX: this state is unused
	XLOG_UNREACHABLE();
	break;
    case PROC_DONE:
	// Process has completed operation
	break;
    default:
	// Unknown status
	XLOG_UNREACHABLE();
	break;
    }
    
    return (status);
}

/**
 * Add a static IPv4 route.
 *
 * @param unicast if true, then the route would be used for unicast
 * routing.
 * @param multicast if true, then the route would be used in the MRIB
 * (Multicast Routing Information Base) for multicast purpose (e.g.,
 * computing the Reverse-Path Forwarding information).
 * @param network the network address prefix this route applies to.
 * @param nexthop the address of the next-hop router for this route.
 * @param metric the metric distance for this route.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
StaticRoutesNode::add_route4(bool unicast, bool multicast,
			     const IPv4Net& network, const IPv4& nexthop,
			     uint32_t metric, string& error_msg)
{
    StaticRoute static_route(unicast, multicast, network, nexthop, metric);

    static_route.set_add_route();

    return (add_route(static_route, error_msg));
}

/**
 * Add a static IPv6 route.
 *
 * @param unicast if true, then the route would be used for unicast
 * routing.
 * @param multicast if true, then the route would be used in the MRIB
 * (Multicast Routing Information Base) for multicast purpose (e.g.,
 * computing the Reverse-Path Forwarding information).
 * @param network the network address prefix this route applies to.
 * @param nexthop the address of the next-hop router for this route.
 * @param metric the metric distance for this route.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
StaticRoutesNode::add_route6(bool unicast, bool multicast,
			     const IPv6Net& network, const IPv6& nexthop,
			     uint32_t metric, string& error_msg)
{
    StaticRoute static_route(unicast, multicast, network, nexthop, metric);

    static_route.set_add_route();

    return (add_route(static_route, error_msg));
}

/**
 * Replace a static IPv4 route.
 *
 * @param unicast if true, then the route would be used for unicast
 * routing.
 * @param multicast if true, then the route would be used in the MRIB
 * (Multicast Routing Information Base) for multicast purpose (e.g.,
 * computing the Reverse-Path Forwarding information).
 * @param network the network address prefix this route applies to.
 * @param nexthop the address of the next-hop router for this route.
 * @param metric the metric distance for this route.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
StaticRoutesNode::replace_route4(bool unicast, bool multicast,
				 const IPv4Net& network, const IPv4& nexthop,
				 uint32_t metric, string& error_msg)
{
    StaticRoute static_route(unicast, multicast, network, nexthop, metric);

    static_route.set_replace_route();

    return (replace_route(static_route, error_msg));
}

/**
 * Replace a static IPv6 route.
 *
 * @param unicast if true, then the route would be used for unicast
 * routing.
 * @param multicast if true, then the route would be used in the MRIB
 * (Multicast Routing Information Base) for multicast purpose (e.g.,
 * computing the Reverse-Path Forwarding information).
 * @param network the network address prefix this route applies to.
 * @param nexthop the address of the next-hop router for this route.
 * @param metric the metric distance for this route.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
StaticRoutesNode::replace_route6(bool unicast, bool multicast,
				 const IPv6Net& network, const IPv6& nexthop,
				 uint32_t metric, string& error_msg)
{
    StaticRoute static_route(unicast, multicast, network, nexthop, metric);

    static_route.set_replace_route();

    return (replace_route(static_route, error_msg));
}

/**
 * Delete a static IPv4 route.
 *
 * @param unicast if true, then the route would be used for unicast
 * routing.
 * @param multicast if true, then the route would be used in the MRIB
 * (Multicast Routing Information Base) for multicast purpose (e.g.,
 * computing the Reverse-Path Forwarding information).
 * @param network the network address prefix this route applies to.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
StaticRoutesNode::delete_route4(bool unicast, bool multicast,
				const IPv4Net& network, string& error_msg)
{
    StaticRoute static_route(unicast, multicast, network,
			     network.masked_addr().ZERO(), 0);

    static_route.set_delete_route();

    return (delete_route(static_route, error_msg));
}

/**
 * Delete a static IPv6 route.
 *
 * @param unicast if true, then the route would be used for unicast
 * routing.
 * @param multicast if true, then the route would be used in the MRIB
 * (Multicast Routing Information Base) for multicast purpose (e.g.,
 * computing the Reverse-Path Forwarding information).
 * @param network the network address prefix this route applies to.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
StaticRoutesNode::delete_route6(bool unicast, bool multicast,
				const IPv6Net& network, string& error_msg)
{
    StaticRoute static_route(unicast, multicast, network,
			     network.masked_addr().ZERO(), 0);

    static_route.set_delete_route();

    return (delete_route(static_route, error_msg));
}

/**
 * Add a static IPvX route.
 *
 * @param static_route the route to add.
 * @see StaticRoute
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
StaticRoutesNode::add_route(const StaticRoute& static_route,
			    string& error_msg)
{
    //
    // Check the entry
    //
    if (static_route.is_valid_entry(error_msg) != true) {
	error_msg = c_format("Cannot add route for %s: %s",
			     static_route.network().str().c_str(),
			     error_msg.c_str());
	return XORP_ERROR;
    }

    //
    // Check if the route was added already
    //
    list<StaticRoute>::iterator iter;
    for (iter = _static_routes.begin(); iter != _static_routes.end(); ++iter) {
	StaticRoute& tmp_route = *iter;
	if (tmp_route.network() != static_route.network())
	    continue;
	if ((tmp_route.unicast() != static_route.unicast())
	    || (tmp_route.multicast() != static_route.multicast())) {
	    continue;
	}
	error_msg = c_format("Cannot add %s route for %s: "
			     "the route already exists",
			     (static_route.unicast())? "unicast" : "multicast",
			     static_route.network().str().c_str());
	return XORP_ERROR;
    }

    //
    // Add the route
    //
    _static_routes.push_back(static_route);

    //
    // Inform the RIB about the change
    //
    inform_rib_route_change(static_route);

    return (XORP_OK);
}

/**
 * Replace a static IPvX route.
 *
 * @param static_route the replacement route.
 * @see StaticRoute
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
StaticRoutesNode::replace_route(const StaticRoute& static_route,
				string& error_msg)
{
    //
    // Check the entry
    //
    if (static_route.is_valid_entry(error_msg) != true) {
	error_msg = c_format("Cannot replace route for %s: %s",
			     static_route.network().str().c_str(),
			     error_msg.c_str());
	return XORP_ERROR;
    }

    //
    // Find the route and replace it
    //
    list<StaticRoute>::iterator iter;
    for (iter = _static_routes.begin(); iter != _static_routes.end(); ++iter) {
	StaticRoute& tmp_route = *iter;
	if (tmp_route.network() != static_route.network())
	    continue;
	if ((tmp_route.unicast() != static_route.unicast())
	    || (tmp_route.multicast() != static_route.multicast())) {
	    continue;
	}
	//
	// Route found. Overwrite its value.
	//
	tmp_route = static_route;

	//
	// Inform the RIB about the change
	//
	inform_rib_route_change(static_route);

	return XORP_OK;
    }

    //
    // Coudn't find the route to replace
    //
    error_msg = c_format("Cannot replace %s route for %s: "
			 "no such route",
			 (static_route.unicast())? "unicast" : "multicast",
			 static_route.network().str().c_str());
    return XORP_ERROR;
}

/**
 * Delete a static IPvX route.
 *
 * @param static_route the route to delete.
 * @see StaticRoute
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
StaticRoutesNode::delete_route(const StaticRoute& static_route,
			       string& error_msg)
{
    //
    // Check the entry
    //
    if (static_route.is_valid_entry(error_msg) != true) {
	error_msg = c_format("Cannot delete route for %s: %s",
			     static_route.network().str().c_str(),
			     error_msg.c_str());
	return XORP_ERROR;
    }

    //
    // Find the route and delete it
    //
    list<StaticRoute>::iterator iter;
    for (iter = _static_routes.begin(); iter != _static_routes.end(); ++iter) {
	StaticRoute& tmp_route = *iter;
	if (tmp_route.network() != static_route.network())
	    continue;
	if ((tmp_route.unicast() != static_route.unicast())
	    || (tmp_route.multicast() != static_route.multicast())) {
	    continue;
	}

	//
	// Route found. Erase it.
	//
	_static_routes.erase(iter);

	//
	// Inform the RIB about the change
	//
	inform_rib_route_change(static_route);
	return XORP_OK;
    }

    //
    // Coudn't find the route to delete
    //
    error_msg = c_format("Cannot delete %s route for %s: "
			 "no such route",
			 (static_route.unicast())? "unicast" : "multicast",
			 static_route.network().str().c_str());
    return XORP_ERROR;
}

/**
 * Check whether the route entry is valid.
 * 
 * @param error_msg the error message (if error).
 * @return true if the route entry is valid, otherwise false.
 */
bool
StaticRoute::is_valid_entry(string& error_msg) const
{
    //
    // Check the unicast and multicast flags
    //
    if ((_unicast == false) && (_multicast == false)) {
	error_msg = "the route is neither unicast nor multicast";
	return false;
    }
    if ((_unicast == true) && (_multicast == true)) {
	error_msg = "the route must be either unicast or multicast";
	return false;
    }

    return true;
}
