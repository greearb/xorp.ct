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
// Fib2mrib node implementation.
//


#include "fib2mrib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "fib2mrib_node.hh"


Fib2mribNode::Fib2mribNode(EventLoop& eventloop)
    : ServiceBase("Fib2mrib"),
      _eventloop(eventloop),
      _protocol_name("fib2mrib"),	// TODO: must be known by RIB
      _startup_requests_n(0),
      _shutdown_requests_n(0),
      _is_log_trace(true)		// XXX: default to print trace logs
{
    //
    // Set the node status
    //
    _node_status = PROC_STARTUP;
}

Fib2mribNode::~Fib2mribNode()
{
    if ((ServiceBase::status() != SHUTDOWN)
	&& (ServiceBase::status() != FAILED)) {
	shutdown();
    }
}

void
Fib2mribNode::startup()
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
Fib2mribNode::shutdown()
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
Fib2mribNode::incr_startup_requests_n()
{
    _startup_requests_n++;
    XLOG_ASSERT(_startup_requests_n > 0);
}

void
Fib2mribNode::decr_startup_requests_n()
{
    XLOG_ASSERT(_startup_requests_n > 0);
    _startup_requests_n--;

    update_status();
}

void
Fib2mribNode::incr_shutdown_requests_n()
{
    _shutdown_requests_n++;
    XLOG_ASSERT(_shutdown_requests_n > 0);
}

void
Fib2mribNode::decr_shutdown_requests_n()
{
    XLOG_ASSERT(_shutdown_requests_n > 0);
    _shutdown_requests_n--;

    update_status();
}

void
Fib2mribNode::update_status()
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
Fib2mribNode::tree_complete()
{
    decr_startup_requests_n();
}

void
Fib2mribNode::updates_made()
{
    // TODO: XXX: PAVPAVPAV: implement it!!
}

ProcessStatus
Fib2mribNode::node_status(string& reason_msg)
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
	reason_msg = c_format("Waiting for %d startup events",
			      _startup_requests_n);
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
	reason_msg = c_format("Waiting for %d shutdown events",
			      _shutdown_requests_n);
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
 * Add an IPv4 route.
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
Fib2mribNode::add_route4(bool unicast, bool multicast,
			 const IPv4Net& network, const IPv4& nexthop,
			 uint32_t metric, string& error_msg)
{
    Fib2mribRoute fib2mrib_route(unicast, multicast, network, nexthop, metric);

    fib2mrib_route.set_add_route();

    return (add_route(fib2mrib_route, error_msg));
}

/**
 * Add an IPv6 route.
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
Fib2mribNode::add_route6(bool unicast, bool multicast,
			 const IPv6Net& network, const IPv6& nexthop,
			 uint32_t metric, string& error_msg)
{
    Fib2mribRoute fib2mrib_route(unicast, multicast, network, nexthop, metric);

    fib2mrib_route.set_add_route();

    return (add_route(fib2mrib_route, error_msg));
}

/**
 * Replace an IPv4 route.
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
Fib2mribNode::replace_route4(bool unicast, bool multicast,
				 const IPv4Net& network, const IPv4& nexthop,
				 uint32_t metric, string& error_msg)
{
    Fib2mribRoute fib2mrib_route(unicast, multicast, network, nexthop, metric);

    fib2mrib_route.set_replace_route();

    return (replace_route(fib2mrib_route, error_msg));
}

/**
 * Replace an IPv6 route.
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
Fib2mribNode::replace_route6(bool unicast, bool multicast,
			     const IPv6Net& network, const IPv6& nexthop,
			     uint32_t metric, string& error_msg)
{
    Fib2mribRoute fib2mrib_route(unicast, multicast, network, nexthop, metric);

    fib2mrib_route.set_replace_route();

    return (replace_route(fib2mrib_route, error_msg));
}

/**
 * Delete an IPv4 route.
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
Fib2mribNode::delete_route4(bool unicast, bool multicast,
			    const IPv4Net& network, string& error_msg)
{
    Fib2mribRoute fib2mrib_route(unicast, multicast, network,
				 network.masked_addr().ZERO(), 0);

    fib2mrib_route.set_delete_route();

    return (delete_route(fib2mrib_route, error_msg));
}

/**
 * Delete an IPv6 route.
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
Fib2mribNode::delete_route6(bool unicast, bool multicast,
			    const IPv6Net& network, string& error_msg)
{
    Fib2mribRoute fib2mrib_route(unicast, multicast, network,
				 network.masked_addr().ZERO(), 0);

    fib2mrib_route.set_delete_route();

    return (delete_route(fib2mrib_route, error_msg));
}

/**
 * Add an IPvX route.
 *
 * @param fib2mrib_route the route to add.
 * @see Fib2mribRoute
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
Fib2mribNode::add_route(const Fib2mribRoute& fib2mrib_route,
			string& error_msg)
{
    //
    // Check the entry
    //
    if (fib2mrib_route.is_valid_entry(error_msg) != true) {
	error_msg = c_format("Cannot add route for %s: %s",
			     fib2mrib_route.network().str().c_str(),
			     error_msg.c_str());
	return XORP_ERROR;
    }

    //
    // Check if the route was added already
    //
    list<Fib2mribRoute>::iterator iter;
    for (iter = _fib2mrib_routes.begin();
	 iter != _fib2mrib_routes.end();
	 ++iter) {
	Fib2mribRoute& tmp_route = *iter;
	if (tmp_route.network() != fib2mrib_route.network())
	    continue;
	if ((tmp_route.unicast() != fib2mrib_route.unicast())
	    || (tmp_route.multicast() != fib2mrib_route.multicast())) {
	    continue;
	}
	error_msg = c_format("Cannot add %s route for %s: "
			     "the route already exists",
			     (fib2mrib_route.unicast())? "unicast" : "multicast",
			     fib2mrib_route.network().str().c_str());
	return XORP_ERROR;
    }

    //
    // Add the route
    //
    _fib2mrib_routes.push_back(fib2mrib_route);

    //
    // Inform the RIB about the change
    //
    inform_rib_route_change(fib2mrib_route);

    return (XORP_OK);
}

/**
 * Replace an IPvX route.
 *
 * @param fib2mrib_route the replacement route.
 * @see Fib2mribRoute
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
Fib2mribNode::replace_route(const Fib2mribRoute& fib2mrib_route,
			    string& error_msg)
{
    //
    // Check the entry
    //
    if (fib2mrib_route.is_valid_entry(error_msg) != true) {
	error_msg = c_format("Cannot replace route for %s: %s",
			     fib2mrib_route.network().str().c_str(),
			     error_msg.c_str());
	return XORP_ERROR;
    }

    //
    // Find the route and replace it
    //
    list<Fib2mribRoute>::iterator iter;
    for (iter = _fib2mrib_routes.begin();
	 iter != _fib2mrib_routes.end();
	 ++iter) {
	Fib2mribRoute& tmp_route = *iter;
	if (tmp_route.network() != fib2mrib_route.network())
	    continue;
	if ((tmp_route.unicast() != fib2mrib_route.unicast())
	    || (tmp_route.multicast() != fib2mrib_route.multicast())) {
	    continue;
	}
	//
	// Route found. Overwrite its value.
	//
	tmp_route = fib2mrib_route;

	//
	// Inform the RIB about the change
	//
	inform_rib_route_change(fib2mrib_route);

	return XORP_OK;
    }

    //
    // Coudn't find the route to replace
    //
    error_msg = c_format("Cannot replace %s route for %s: "
			 "no such route",
			 (fib2mrib_route.unicast())? "unicast" : "multicast",
			 fib2mrib_route.network().str().c_str());
    return XORP_ERROR;
}

/**
 * Delete an IPvX route.
 *
 * @param fib2mrib_route the route to delete.
 * @see Fib2mribRoute
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
Fib2mribNode::delete_route(const Fib2mribRoute& fib2mrib_route,
			   string& error_msg)
{
    //
    // Check the entry
    //
    if (fib2mrib_route.is_valid_entry(error_msg) != true) {
	error_msg = c_format("Cannot delete route for %s: %s",
			     fib2mrib_route.network().str().c_str(),
			     error_msg.c_str());
	return XORP_ERROR;
    }

    //
    // Find the route and delete it
    //
    list<Fib2mribRoute>::iterator iter;
    for (iter = _fib2mrib_routes.begin();
	 iter != _fib2mrib_routes.end();
	 ++iter) {
	Fib2mribRoute& tmp_route = *iter;
	if (tmp_route.network() != fib2mrib_route.network())
	    continue;
	if ((tmp_route.unicast() != fib2mrib_route.unicast())
	    || (tmp_route.multicast() != fib2mrib_route.multicast())) {
	    continue;
	}

	//
	// Route found. Erase it.
	//
	_fib2mrib_routes.erase(iter);

	//
	// Inform the RIB about the change
	//
	inform_rib_route_change(fib2mrib_route);
	return XORP_OK;
    }

    //
    // Coudn't find the route to delete
    //
    error_msg = c_format("Cannot delete %s route for %s: "
			 "no such route",
			 (fib2mrib_route.unicast())? "unicast" : "multicast",
			 fib2mrib_route.network().str().c_str());
    return XORP_ERROR;
}

/**
 * Check whether the route entry is valid.
 * 
 * @param error_msg the error message (if error).
 * @return true if the route entry is valid, otherwise false.
 */
bool
Fib2mribRoute::is_valid_entry(string& error_msg) const
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
