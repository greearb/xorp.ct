// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/static_routes/static_routes_node.cc,v 1.30 2005/11/02 02:27:22 pavlin Exp $"

//
// StaticRoutes node implementation.
//

#include "static_routes_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "static_routes_node.hh"
#include "static_routes_varrw.hh"

StaticRoutesNode::StaticRoutesNode(EventLoop& eventloop)
    : ServiceBase("StaticRoutes"),
      _eventloop(eventloop),
      _protocol_name("static"),		// TODO: must be known by RIB
      _startup_requests_n(0),
      _shutdown_requests_n(0),
      _is_log_trace(true)		// XXX: default to print trace logs
{
    set_node_status(PROC_STARTUP);
}

StaticRoutesNode::~StaticRoutesNode()
{
    shutdown();
}

bool
StaticRoutesNode::startup()
{
    //
    // Test the service status
    //
    if ((ServiceBase::status() == SERVICE_STARTING)
	|| (ServiceBase::status() == SERVICE_RUNNING)) {
	return true;
    }
    if (ServiceBase::status() != SERVICE_READY) {
	return false;
    }

    //
    // Transition to SERVICE_RUNNING occurs when all transient startup
    // operations are completed (e.g., after we have the interface/vif/address
    // state available, after we have registered with the RIB, etc.)
    //
    ServiceBase::set_status(SERVICE_STARTING);

    //
    // Set the node status
    //
    set_node_status(PROC_STARTUP);

    //
    // Register with the FEA
    //
    fea_register_startup();

    //
    // Register with the RIB
    //
    rib_register_startup();

    return true;
}

bool
StaticRoutesNode::shutdown()
{
    //
    // Test the service status
    //
    if ((ServiceBase::status() == SERVICE_SHUTDOWN)
	|| (ServiceBase::status() == SERVICE_SHUTTING_DOWN)
	|| (ServiceBase::status() == SERVICE_FAILED)) {
	return true;
    }
    if ((ServiceBase::status() != SERVICE_RUNNING)
	&& (ServiceBase::status() != SERVICE_STARTING)
	&& (ServiceBase::status() != SERVICE_PAUSING)
	&& (ServiceBase::status() != SERVICE_PAUSED)
	&& (ServiceBase::status() != SERVICE_RESUMING)) {
	return false;
    }

    //
    // Transition to SERVICE_SHUTDOWN occurs when all transient shutdown
    // operations are completed (e.g., after we have deregistered with the FEA
    // and the RIB, etc.)
    //
    ServiceBase::set_status(SERVICE_SHUTTING_DOWN);

    //
    // De-register with the RIB
    //
    rib_register_shutdown();

    //
    // De-register with the FEA
    //
    fea_register_shutdown();

    //
    // Set the node status
    //
    set_node_status(PROC_SHUTDOWN);

    //
    // Update the node status
    //
    update_status();

    return true;
}

void
StaticRoutesNode::status_change(ServiceBase*  service,
				ServiceStatus old_status,
				ServiceStatus new_status)
{
    if (service == this) {
	// My own status has changed
	if ((old_status == SERVICE_STARTING)
	    && (new_status == SERVICE_RUNNING)) {
	    // The startup process has completed
	    set_node_status(PROC_READY);
	    return;
	}

	if ((old_status == SERVICE_SHUTTING_DOWN)
	    && (new_status == SERVICE_SHUTDOWN)) {
	    // The shutdown process has completed
	    set_node_status(PROC_DONE);
	    return;
	}

	//
	// TODO: check if there was an error
	//
	return;
    }

    if (service == ifmgr_mirror_service_base()) {
	if ((old_status == SERVICE_SHUTTING_DOWN)
	    && (new_status == SERVICE_SHUTDOWN)) {
	    decr_shutdown_requests_n();
	}
    }
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
    if (ServiceBase::status() == SERVICE_STARTING) {
	if (_startup_requests_n > 0)
	    return;

	// The startup process has completed
	ServiceBase::set_status(SERVICE_RUNNING);
	set_node_status(PROC_READY);
	return;
    }

    //
    // Test if the shutdown process has completed
    //
    if (ServiceBase::status() == SERVICE_SHUTTING_DOWN) {
	if (_shutdown_requests_n > 0)
	    return;

	// The shutdown process has completed
	ServiceBase::set_status(SERVICE_SHUTDOWN);
	set_node_status(PROC_DONE);
	return;
    }

    //
    // Test if we have failed
    //
    if (ServiceBase::status() == SERVICE_FAILED) {
	set_node_status(PROC_DONE);
	return;
    }
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
			      XORP_UINT_CAST(_startup_requests_n));
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
			      XORP_UINT_CAST(_shutdown_requests_n));
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

void
StaticRoutesNode::tree_complete()
{
    decr_startup_requests_n();

    //
    // XXX: we use same actions when the tree is completed or updates are made
    //
    updates_made();
}

void
StaticRoutesNode::updates_made()
{
    list<StaticRoute>::iterator route_iter;
    list<StaticRoute *> add_routes, replace_routes, delete_routes;
    list<StaticRoute *>::iterator pending_iter;

    for (route_iter = _static_routes.begin();
	 route_iter != _static_routes.end();
	 ++route_iter) {
	StaticRoute& static_route = *route_iter;
	bool is_old_up = false;
	bool is_new_up = false;
	string old_ifname, old_vifname, new_ifname, new_vifname;

	if (static_route.is_interface_route()) {
	    //
	    // Calculate whether the interface was UP before and now.
	    //
	    const IfMgrIfAtom* if_atom;
	    const IfMgrVifAtom* vif_atom;

	    if_atom = _iftree.find_if(static_route.ifname());
	    vif_atom = _iftree.find_vif(static_route.ifname(),
					static_route.vifname());
	    if ((if_atom != NULL) && (if_atom->enabled())
		&& (! if_atom->no_carrier())
		&& (vif_atom != NULL) && (vif_atom->enabled())) {
		is_old_up = true;
	    }

	    if_atom = ifmgr_iftree().find_if(static_route.ifname());
	    vif_atom = ifmgr_iftree().find_vif(static_route.ifname(),
					       static_route.vifname());
	    if ((if_atom != NULL) && (if_atom->enabled())
		&& (! if_atom->no_carrier())
		&& (vif_atom != NULL) && (vif_atom->enabled())) {
		is_new_up = true;
	    }
	} else {
	    //
	    // Calculate whether the next-hop router was directly connected
	    // before and now.
	    //
	    if (_iftree.is_directly_connected(static_route.nexthop(),
					      old_ifname,
					      old_vifname)) {
		is_old_up = true;
	    }
	    if (ifmgr_iftree().is_directly_connected(static_route.nexthop(),
						     new_ifname,
						     new_vifname)) {
		is_new_up = true;
	    }
	}

	if ((is_old_up == is_new_up)
	    && (old_ifname == new_ifname)
	    && (old_vifname == new_vifname)) {
	    continue;			// Nothing changed
	}

	if ((! is_old_up) && (! is_new_up)) {
	    //
	    // The interface s still down, so nothing to do
	    //
	    continue;
	}
	if ((! is_old_up) && (is_new_up)) {
	    //
	    // The interface is now up, hence add the route
	    //
	    add_routes.push_back(&static_route);
	    continue;
	}
	if ((is_old_up) && (! is_new_up)) {
	    //
	    // The interface went down, hence cancel all pending requests,
	    // and withdraw the route.
	    //
	    delete_routes.push_back(&static_route);
	    continue;
	}
	if (is_old_up && is_new_up) {
	    //
	    // The interface remains up, hence probably the interface or
	    // the vif name has changed.
	    // Delete the route and then add it again so the information
	    // in the RIB will be updated.
	    //
	    replace_routes.push_back(&static_route);
	    continue;
	}
    }

    //
    // Update the local copy of the interface tree
    //
    _iftree = ifmgr_iftree();

    //
    // Process all pending "add route" requests
    //
    for (pending_iter = add_routes.begin();
	 pending_iter != add_routes.end();
	 ++pending_iter) {
	StaticRoute& orig_route = *(*pending_iter);
	StaticRoute copy_route = orig_route;
	prepare_route_for_transmission(orig_route, copy_route);
	copy_route.set_add_route();
	inform_rib(copy_route);
    }

    //
    // Process all pending "replace route" requests
    //
    for (pending_iter = replace_routes.begin();
	 pending_iter != replace_routes.end();
	 ++pending_iter) {
	StaticRoute& orig_route = *(*pending_iter);
	StaticRoute copy_route = orig_route;
	// First delete the route, then add the route
	prepare_route_for_transmission(orig_route, copy_route);
	copy_route.set_delete_route();
	inform_rib(copy_route);
	copy_route = orig_route;
	prepare_route_for_transmission(orig_route, copy_route);
	copy_route.set_add_route();
	inform_rib(copy_route);
    }

    //
    // Process all pending "delete route" requests
    //
    for (pending_iter = delete_routes.begin();
	 pending_iter != delete_routes.end();
	 ++pending_iter) {
	StaticRoute& orig_route = *(*pending_iter);
	cancel_rib_route_change(orig_route);
	StaticRoute copy_route = orig_route;
	prepare_route_for_transmission(orig_route, copy_route);
	copy_route.set_delete_route();
	inform_rib(copy_route);
    }
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
 * @param ifname of the name of the physical interface toward the
 * destination.
 * @param vifname of the name of the virtual interface toward the
 * destination.
 * @param metric the metric distance for this route.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
StaticRoutesNode::add_route4(bool unicast, bool multicast,
			     const IPv4Net& network, const IPv4& nexthop,
			     const string& ifname, const string& vifname,
			     uint32_t metric, string& error_msg)
{
    StaticRoute static_route(unicast, multicast, network, nexthop,
			     ifname, vifname, metric);

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
 * @param ifname of the name of the physical interface toward the
 * destination.
 * @param vifname of the name of the virtual interface toward the
 * destination.
 * @param metric the metric distance for this route.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
StaticRoutesNode::add_route6(bool unicast, bool multicast,
			     const IPv6Net& network, const IPv6& nexthop,
			     const string& ifname, const string& vifname,
			     uint32_t metric, string& error_msg)
{
    StaticRoute static_route(unicast, multicast, network, nexthop,
			     ifname, vifname, metric);

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
 * @param ifname of the name of the physical interface toward the
 * destination.
 * @param vifname of the name of the virtual interface toward the
 * destination.
 * @param metric the metric distance for this route.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
StaticRoutesNode::replace_route4(bool unicast, bool multicast,
				 const IPv4Net& network, const IPv4& nexthop,
				 const string& ifname, const string& vifname,
				 uint32_t metric, string& error_msg)
{
    StaticRoute static_route(unicast, multicast, network, nexthop,
			     ifname, vifname, metric);

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
 * @param ifname of the name of the physical interface toward the
 * destination.
 * @param vifname of the name of the virtual interface toward the
 * destination.
 * @param metric the metric distance for this route.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
StaticRoutesNode::replace_route6(bool unicast, bool multicast,
				 const IPv6Net& network, const IPv6& nexthop,
				 const string& ifname, const string& vifname,
				 uint32_t metric, string& error_msg)
{
    StaticRoute static_route(unicast, multicast, network, nexthop,
			     ifname, vifname, metric);

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
			     network.masked_addr().ZERO(), "", "", 0);

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
			     network.masked_addr().ZERO(), "", "", 0);

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
    StaticRoute updated_route = static_route;

    //
    // Update the route
    //
    update_route(_iftree, updated_route);

    //
    // Check the entry
    //
    if (updated_route.is_valid_entry(error_msg) != true) {
	error_msg = c_format("Cannot add route for %s: %s",
			     updated_route.network().str().c_str(),
			     error_msg.c_str());
	return XORP_ERROR;
    }

    //
    // Check if the route was added already
    //
    list<StaticRoute>::iterator iter;
    for (iter = _static_routes.begin(); iter != _static_routes.end(); ++iter) {
	StaticRoute& orig_route = *iter;
	if (orig_route.network() != updated_route.network())
	    continue;
	if ((orig_route.unicast() != updated_route.unicast())
	    || (orig_route.multicast() != updated_route.multicast())) {
	    continue;
	}
	error_msg = c_format("Cannot add %s route for %s: "
			     "the route already exists",
			     (updated_route.unicast())?
			     "unicast" : "multicast",
			     updated_route.network().str().c_str());
	return XORP_ERROR;
    }

    //
    // Add the route
    //
    _static_routes.push_back(updated_route);

    //
    // Create a copy of the route and inform the RIB if necessary
    //
    StaticRoute& orig_route = _static_routes.back();
    StaticRoute copy_route = orig_route;
    prepare_route_for_transmission(orig_route, copy_route);

    //
    // Inform the RIB about the change
    //
    inform_rib(copy_route);

    return XORP_OK;
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
    StaticRoute updated_route = static_route;

    //
    // Update the route
    //
    update_route(_iftree, updated_route);

    //
    // Check the entry
    //
    if (updated_route.is_valid_entry(error_msg) != true) {
	error_msg = c_format("Cannot replace route for %s: %s",
			     updated_route.network().str().c_str(),
			     error_msg.c_str());
	return XORP_ERROR;
    }

    //
    // Find the route and replace it
    //
    list<StaticRoute>::iterator iter;
    for (iter = _static_routes.begin(); iter != _static_routes.end(); ++iter) {
	StaticRoute& orig_route = *iter;
	if (orig_route.network() != updated_route.network())
	    continue;
	if ((orig_route.unicast() != updated_route.unicast())
	    || (orig_route.multicast() != updated_route.multicast())) {
	    continue;
	}

	//
	// Route found. Overwrite its value.
	//
	bool was_accepted = orig_route.is_accepted_by_rib();
	orig_route = updated_route;

	//
	// Create a copy of the route and inform the RIB if necessary
	//
	StaticRoute copy_route = orig_route;
	prepare_route_for_transmission(orig_route, copy_route);

	//
	// XXX: If necessary, change the type of the route.
	// E.g., "replace route" may become "add route" or "delete route".
	//
	if (copy_route.is_accepted_by_rib()) {
	    if (was_accepted) {
		copy_route.set_replace_route();
	    } else {
		copy_route.set_add_route();
	    }
	} else {
	    if (was_accepted) {
		copy_route.set_delete_route();
	    } else {
		return XORP_OK;
	    }
	}

	//
	// Inform the RIB about the change
	//
	inform_rib(copy_route);

	return XORP_OK;
    }

    //
    // Couldn't find the route to replace
    //
    error_msg = c_format("Cannot replace %s route for %s: "
			 "no such route",
			 (updated_route.unicast())? "unicast" : "multicast",
			 updated_route.network().str().c_str());
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
    StaticRoute updated_route = static_route;

    //
    // Update the route
    //
    update_route(_iftree, updated_route);

    //
    // Check the entry
    //
    if (updated_route.is_valid_entry(error_msg) != true) {
	error_msg = c_format("Cannot delete route for %s: %s",
			     updated_route.network().str().c_str(),
			     error_msg.c_str());
	return XORP_ERROR;
    }

    //
    // Find the route and delete it
    //
    list<StaticRoute>::iterator iter;
    for (iter = _static_routes.begin(); iter != _static_routes.end(); ++iter) {
	StaticRoute& orig_route = *iter;
	if (orig_route.network() != updated_route.network())
	    continue;
	if ((orig_route.unicast() != updated_route.unicast())
	    || (orig_route.multicast() != updated_route.multicast())) {
	    continue;
	}

	//
	// Route found. Create a copy of it and erase it.
	//
	bool was_accepted = orig_route.is_accepted_by_rib();
	StaticRoute copy_route = orig_route;
	prepare_route_for_transmission(orig_route, copy_route);
	_static_routes.erase(iter);

	copy_route.set_delete_route();

	//
	// If the original route wasn't transmitted, then the RIB doesn't
	// know about it.
	//
	if (! was_accepted)
	    return XORP_OK;

	//
	// Inform the RIB about the change
	//
	inform_rib(copy_route);

	return XORP_OK;
    }

    //
    // Couldn't find the route to delete
    //
    error_msg = c_format("Cannot delete %s route for %s: "
			 "no such route",
			 (updated_route.unicast())? "unicast" : "multicast",
			 updated_route.network().str().c_str());
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

/**
 * Test whether the route is accepted for transmission to the RIB.
 *
 * @return true if route is accepted for transmission to the RIB,
 * otherwise false.
 */
bool
StaticRoute::is_accepted_by_rib() const
{
    return (is_accepted_by_nexthop() && (! is_filtered()));
}

void
StaticRoutesNode::configure_filter(const uint32_t& filter, const string& conf)
{
    _policy_filters.configure(filter, conf);
}

void
StaticRoutesNode::reset_filter(const uint32_t& filter) {
    _policy_filters.reset(filter);
}

void
StaticRoutesNode::push_routes()
{
    list<StaticRoute>::iterator iter;

    // XXX: not a background task
    for (iter = _static_routes.begin(); iter != _static_routes.end(); ++iter) {
	StaticRoute& orig_route = *iter;
	bool was_accepted = orig_route.is_accepted_by_rib();

	//
	// Create a copy of the route and inform the RIB if necessary
	//
	StaticRoute copy_route = orig_route;
	prepare_route_for_transmission(orig_route, copy_route);

	//
	// XXX: If necessary, change the type of the route.
	// E.g., "replace route" may become "add route" or "delete route".
	//
	if (copy_route.is_accepted_by_rib()) {
	    if (was_accepted) {
		copy_route.set_replace_route();
	    } else {
		copy_route.set_add_route();
	    }
	} else {
	    if (was_accepted) {
		copy_route.set_delete_route();
	    } else {
		continue;
	    }
	}

	//
	// Inform the RIB about the change
	//
	inform_rib(copy_route);
    }
}

bool
StaticRoutesNode::is_accepted_by_nexthop(const StaticRoute& route) const
{
    if (route.is_interface_route()) {
	const IfMgrIfAtom* if_atom;
	const IfMgrVifAtom* vif_atom;
	bool is_up = false;

	if_atom = _iftree.find_if(route.ifname());
	vif_atom = _iftree.find_vif(route.ifname(), route.vifname());
	if ((if_atom != NULL) && (if_atom->enabled())
	    && (! if_atom->no_carrier())
	    && (vif_atom != NULL) && (vif_atom->enabled())) {
	    is_up = true;
	}
	if (is_up) {
	    return (true);
	}
    } else {
	string ifname, vifname;
	if (_iftree.is_directly_connected(route.nexthop(), ifname, vifname)) {
	    return (true);
	}
    }

    return (false);
}

void
StaticRoutesNode::prepare_route_for_transmission(StaticRoute& orig_route,
						 StaticRoute& copy_route)
{
    //
    // We do not want to modify original route, so we may re-filter routes on
    // filter configuration changes. Hence, copy the route.
    //
    copy_route = orig_route;

    // Do policy filtering and other acceptance tests
    bool filtered = (! do_filtering(copy_route));
    bool accepted_by_nexthop = is_accepted_by_nexthop(copy_route);
    copy_route.set_filtered(filtered);
    copy_route.set_accepted_by_nexthop(accepted_by_nexthop);

    // Tag the original route
    orig_route.set_filtered(filtered);
    orig_route.set_accepted_by_nexthop(accepted_by_nexthop);
}

void
StaticRoutesNode::inform_rib(const StaticRoute& route)
{
    //
    // Inform the RIB about the change
    //
    if (route.is_add_route() || route.is_replace_route()) {
	if (route.is_accepted_by_rib())
	    inform_rib_route_change(route);
    }
    if (route.is_delete_route()) {
	inform_rib_route_change(route);
    }
}

/**
 * Update a route received from the user configuration.
 *
 * Currently, this method is a no-op.
 *
 * @param iftree the tree with the interface state to update the route.
 * @param route the route to update.
 * @return true if the route was updated, otherwise false.
 */
bool
StaticRoutesNode::update_route(const IfMgrIfTree& iftree, StaticRoute& route)
{
    UNUSED(iftree);
    UNUSED(route);

    return (false);
}

bool
StaticRoutesNode::do_filtering(StaticRoute& route)
{
    try {
	StaticRoutesVarRW varrw(route);

	// Import filtering
	bool accepted;

	debug_msg("[STATIC] Running filter: %s on route: %s\n",
		  filter::filter2str(filter::IMPORT).c_str(),
		  route.network().str().c_str());
	accepted = _policy_filters.run_filter(filter::IMPORT, varrw);

	route.set_filtered(!accepted);

	// Route Rejected 
	if (!accepted) 
	    return accepted;

	StaticRoutesVarRW varrw2(route);

	// Export source-match filtering
	debug_msg("[STATIC] Running filter: %s on route: %s\n",
		  filter::filter2str(filter::EXPORT_SOURCEMATCH).c_str(),
		  route.network().str().c_str());

	_policy_filters.run_filter(filter::EXPORT_SOURCEMATCH, varrw2);

	return accepted;
    } catch(const PolicyException& e) {
	XLOG_FATAL("PolicyException: %s", e.str().c_str());

	// FIXME: What do we do ?
	XLOG_UNFINISHED();
    }
}
