// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/static_routes/static_routes_node.cc,v 1.41 2008/10/02 21:58:29 bms Exp $"

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
      _is_enabled(true),		// XXX: enabled by default
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

int
StaticRoutesNode::startup()
{
    //
    // Test the service status
    //
    if ((ServiceBase::status() == SERVICE_STARTING)
	|| (ServiceBase::status() == SERVICE_RUNNING)) {
	return (XORP_OK);
    }
    if (ServiceBase::status() != SERVICE_READY) {
	return (XORP_ERROR);
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

    return (XORP_OK);
}

int
StaticRoutesNode::shutdown()
{
    //
    // Test the service status
    //
    if ((ServiceBase::status() == SERVICE_SHUTDOWN)
	|| (ServiceBase::status() == SERVICE_SHUTTING_DOWN)
	|| (ServiceBase::status() == SERVICE_FAILED)) {
	return (XORP_OK);
    }
    if ((ServiceBase::status() != SERVICE_RUNNING)
	&& (ServiceBase::status() != SERVICE_STARTING)
	&& (ServiceBase::status() != SERVICE_PAUSING)
	&& (ServiceBase::status() != SERVICE_PAUSED)
	&& (ServiceBase::status() != SERVICE_RESUMING)) {
	return (XORP_ERROR);
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

    return (XORP_OK);
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
    StaticRoutesNode::Table::iterator route_iter;
    list<StaticRoute *> add_routes, replace_routes, delete_routes;
    list<StaticRoute *>::iterator pending_iter;

    for (route_iter = _static_routes.begin();
	 route_iter != _static_routes.end();
	 ++route_iter) {
	StaticRoute& static_route = route_iter->second;
	bool is_old_up = false;
	bool is_new_up = false;
	string old_ifname, old_vifname, new_ifname, new_vifname;

	if (static_route.is_interface_route()) {
	    //
	    // Calculate whether the interface was UP before and now.
	    //
	    const IfMgrIfAtom* if_atom;
	    const IfMgrVifAtom* vif_atom;

	    if_atom = _iftree.find_interface(static_route.ifname());
	    vif_atom = _iftree.find_vif(static_route.ifname(),
					static_route.vifname());
	    if ((if_atom != NULL) && (if_atom->enabled())
		&& (! if_atom->no_carrier())
		&& (vif_atom != NULL) && (vif_atom->enabled())) {
		is_old_up = true;
	    }

	    if_atom = ifmgr_iftree().find_interface(static_route.ifname());
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
 * Enable/disable node operation.
 *
 * Note that for the time being it affects only whether the routes
 * are installed into RIB. In the future it may affect the interaction
 * with other modules as well.
 *
 * @param enable if true then enable node operation, otherwise disable it.
 */
void
StaticRoutesNode::set_enabled(bool enable)
{
    if (enable == is_enabled())
	return;			// XXX: nothing changed

    if (enable) {
	_is_enabled = true;
	push_pull_rib_routes(true);
    } else {
	push_pull_rib_routes(false);
	_is_enabled = false;
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
 * @param is_backup_route if true, then this is a backup route operation.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
StaticRoutesNode::add_route4(bool unicast, bool multicast,
			     const IPv4Net& network, const IPv4& nexthop,
			     const string& ifname, const string& vifname,
			     uint32_t metric, bool is_backup_route,
			     string& error_msg)
{
    StaticRoute static_route(unicast, multicast, network, nexthop,
			     ifname, vifname, metric, is_backup_route);

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
 * @param is_backup_route if true, then this is a backup route operation.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
StaticRoutesNode::add_route6(bool unicast, bool multicast,
			     const IPv6Net& network, const IPv6& nexthop,
			     const string& ifname, const string& vifname,
			     uint32_t metric, bool is_backup_route,
			     string& error_msg)
{
    StaticRoute static_route(unicast, multicast, network, nexthop,
			     ifname, vifname, metric, is_backup_route);

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
 * @param is_backup_route if true, then this is a backup route operation.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
StaticRoutesNode::replace_route4(bool unicast, bool multicast,
				 const IPv4Net& network, const IPv4& nexthop,
				 const string& ifname, const string& vifname,
				 uint32_t metric, bool is_backup_route,
				 string& error_msg)
{
    StaticRoute static_route(unicast, multicast, network, nexthop,
			     ifname, vifname, metric, is_backup_route);

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
 * @param is_backup_route if true, then this is a backup route operation.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
StaticRoutesNode::replace_route6(bool unicast, bool multicast,
				 const IPv6Net& network, const IPv6& nexthop,
				 const string& ifname, const string& vifname,
				 uint32_t metric, bool is_backup_route,
				 string& error_msg)
{
    StaticRoute static_route(unicast, multicast, network, nexthop,
			     ifname, vifname, metric, is_backup_route);

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
 * @param nexthop the address of the next-hop router for this route.
 * @param ifname of the name of the physical interface toward the
 * destination.
 * @param vifname of the name of the virtual interface toward the
 * destination.
 * @param is_backup_route if true, then this is a backup route operation.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
StaticRoutesNode::delete_route4(bool unicast, bool multicast,
				const IPv4Net& network, const IPv4& nexthop,
				const string& ifname, const string& vifname,
				bool is_backup_route, string& error_msg)
{
    StaticRoute static_route(unicast, multicast, network, nexthop,
			     ifname, vifname, 0, is_backup_route);

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
 * @param nexthop the address of the next-hop router for this route.
 * @param ifname of the name of the physical interface toward the
 * destination.
 * @param vifname of the name of the virtual interface toward the
 * destination.
 * @param is_backup_route if true, then this is a backup route operation.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
StaticRoutesNode::delete_route6(bool unicast, bool multicast,
				const IPv6Net& network, const IPv6& nexthop,
				const string& ifname, const string& vifname,
				bool is_backup_route, string& error_msg)
{
    StaticRoute static_route(unicast, multicast, network, nexthop,
			     ifname, vifname, 0, is_backup_route);

    static_route.set_delete_route();

    return (delete_route(static_route, error_msg));
}

/**
 * Find a route from the routing table.
 *
 * @param table the routing table to seach.
 * @param key_route the route information to search for.
 * @return a table iterator to the route. If the route is not found,
 * the iterator will point to the end of the table.
 */
StaticRoutesNode::Table::iterator
StaticRoutesNode::find_route(StaticRoutesNode::Table& table,
			     const StaticRoute& key_route)
{
    StaticRoutesNode::Table::iterator iter;

    iter = table.find(key_route.network());
    for ( ; iter != table.end(); ++iter) {
	StaticRoute& orig_route = iter->second;
	if (orig_route.network() != key_route.network())
	    break;

	// Check whether the route attributes match
	if ((orig_route.unicast() != key_route.unicast())
	    || (orig_route.multicast() != key_route.multicast())) {
	    continue;
	}
	if (orig_route.is_backup_route() != key_route.is_backup_route()) {
	    continue;
	}

	if (! key_route.is_backup_route()) {
	    // XXX: There can be a single (primary) route to a destination
	    return (iter);
	}

	// A backup route: check the next-hop information
	if ((orig_route.ifname() == key_route.ifname())
	    && (orig_route.vifname() == key_route.vifname())
	    && (orig_route.nexthop() == key_route.nexthop())) {
	    // XXX: Exact route found
	    return (iter);
	}
    }

    return (table.end());		// XXX: Nothing found
}

/**
 * Find the best accepted route from the routing table.
 *
 * @param table the routing table to seach.
 * @param key_route the route information to search for.
 * @return a table iterator to the route. If the route is not found,
 * the iterator will point to the end of the table.
 */
StaticRoutesNode::Table::iterator
StaticRoutesNode::find_best_accepted_route(StaticRoutesNode::Table& table,
					   const StaticRoute& key_route)
{
    StaticRoutesNode::Table::iterator iter, best_iter = table.end();

    iter = table.find(key_route.network());
    for ( ; iter != table.end(); ++iter) {
	StaticRoute& orig_route = iter->second;
	if (orig_route.network() != key_route.network())
	    break;

	// Check whether the route attributes match
	if ((orig_route.unicast() != key_route.unicast())
	    || (orig_route.multicast() != key_route.multicast())) {
	    continue;
	}

	// Consider only routes that are accepted by the RIB
	if (! orig_route.is_accepted_by_rib())
	    continue;

	if (best_iter == table.end()) {
	    // The first matching route
	    best_iter = iter;
	    continue;
	}

	// Select the route with the better metric
	StaticRoute& best_route = best_iter->second;
	if (orig_route.metric() < best_route.metric()) {
	    best_iter = iter;
	    continue;
	}
    }

    return (best_iter);
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
    StaticRoutesNode::Table::iterator iter = find_route(_static_routes,
							updated_route);
    if (iter != _static_routes.end()) {
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
    iter = _static_routes.insert(make_pair(updated_route.network(),
					   updated_route));

    //
    // Create a copy of the route and inform the RIB if necessary
    //
    StaticRoute& orig_route = iter->second;
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
    // Find the route and replace it.
    //
    StaticRoutesNode::Table::iterator iter = find_route(_static_routes,
							updated_route);
    if (iter == _static_routes.end()) {
	//
	// Couldn't find the route to replace
	//
	error_msg = c_format("Cannot replace route for %s: "
			     "no such route",
			     updated_route.network().str().c_str());
	return XORP_ERROR;
    }

    //
    // Route found. Overwrite its value.
    //
    do {
	StaticRoute& orig_route = iter->second;

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
    } while (false);

    return XORP_OK;
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
    // Find the route and delete it.
    // XXX: If this is a primary route (i.e., not a backup route), then
    // delete all routes to the same destination.
    //
    StaticRoutesNode::Table::iterator iter = find_route(_static_routes,
							updated_route);
    if (iter == _static_routes.end()) {
	//
	// Couldn't find the route to delete
	//
	error_msg = c_format("Cannot delete %s route for %s: "
			     "no such route",
			     (updated_route.unicast())? "unicast" : "multicast",
			     updated_route.network().str().c_str());
	return XORP_ERROR;
    }

    //
    // XXX: We need to remove from the table all routes that are to be
    // deleted and then propagate the deletes to the RIB. Otherwise,
    // deleting the best route might add the second best route right
    // before that route is also deleted.
    //
    list<StaticRoute> delete_routes;

    iter = _static_routes.find(updated_route.network());
    while (iter != _static_routes.end()) {
	StaticRoutesNode::Table::iterator orig_iter = iter;
	StaticRoute& orig_route = orig_iter->second;
	++iter;
	
	if (orig_route.network() != updated_route.network())
	    break;

	if ((orig_route.unicast() != updated_route.unicast())
	    || (orig_route.multicast() != updated_route.multicast())) {
	    continue;
	}

	if (updated_route.is_backup_route()) {
	    if ((orig_route.is_backup_route() != updated_route.is_backup_route())
		|| (orig_route.ifname() != updated_route.ifname())
		|| (orig_route.vifname() != updated_route.vifname())
		|| (orig_route.nexthop() != updated_route.nexthop())) {
		continue;
	    }
	}

	//
	// Route found. Add a copy to the list of routes to be deleted
	// and delete the original route;
	//
	delete_routes.push_back(orig_route);
	_static_routes.erase(orig_iter);
    }

    list<StaticRoute>::iterator delete_iter;
    for (delete_iter = delete_routes.begin();
	 delete_iter != delete_routes.end();
	 ++delete_iter) {
	StaticRoute& orig_route = *delete_iter;		// XXX: actually a copy

	bool was_accepted = orig_route.is_accepted_by_rib();
	StaticRoute copy_route = orig_route;
	prepare_route_for_transmission(orig_route, copy_route);

	copy_route.set_delete_route();

	//
	// If the original route wasn't transmitted, then the RIB doesn't
	// know about it.
	//
	if (! was_accepted)
	    continue;

	//
	// Inform the RIB about the change
	//
	inform_rib(copy_route);
    }

    return XORP_OK;
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
    StaticRoutesNode::Table::iterator iter;

    // XXX: not a background task
    for (iter = _static_routes.begin(); iter != _static_routes.end(); ++iter) {
	StaticRoute& orig_route = iter->second;
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

void
StaticRoutesNode::push_pull_rib_routes(bool is_push)
{
    StaticRoutesNode::Table::iterator iter;

    // XXX: not a background task
    for (iter = _static_routes.begin(); iter != _static_routes.end(); ++iter) {
	StaticRoute& orig_route = iter->second;

	//
	// Create a copy of the route and inform the RIB if necessary
	//
	StaticRoute copy_route = orig_route;
	prepare_route_for_transmission(orig_route, copy_route);

	//
	// XXX: Only routes that are accepted by RIB should be added or deleted
	//
	if (! copy_route.is_accepted_by_rib())
	    continue;

	if (is_push) {
	    copy_route.set_add_route();
	} else {
	    copy_route.set_delete_route();
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

	if_atom = _iftree.find_interface(route.ifname());
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
    StaticRoute modified_route(route);
    StaticRoutesNode::Table::iterator best_iter;
    map<IPvXNet, StaticRoute> *winning_routes_ptr;
    map<IPvXNet, StaticRoute>::iterator old_winning_iter;

    if (! is_enabled())
	return;

    //
    // Set the pointer to the appropriate table with the winning routes
    //
    if (route.unicast()) {
	winning_routes_ptr = &_winning_routes_unicast;
    } else {
	winning_routes_ptr = &_winning_routes_multicast;
    }

    //
    // Find the iterators for the current best route and the old winning route
    //
    best_iter = find_best_accepted_route(_static_routes, route);
    old_winning_iter = winning_routes_ptr->find(route.network());

    //
    // Decide whether we need to inform the RIB about the change.
    // If yes, then update the table with the winning routes.
    //
    bool do_inform = false;
    if (route.is_add_route() || route.is_replace_route()) {
	if (route.is_accepted_by_rib()) {
	    XLOG_ASSERT(best_iter != _static_routes.end());
	    StaticRoute& best_route = best_iter->second;

	    if (route.is_same_route(best_route)) {
		//
		// If there was already a route sent to the RIB, then
		// an "add" route should become a "replace" route.
		//
		if (old_winning_iter != winning_routes_ptr->end()) {
		    if (route.is_add_route()) {
			modified_route.set_replace_route();
		    }
		    winning_routes_ptr->erase(old_winning_iter);
		}
		winning_routes_ptr->insert(make_pair(modified_route.network(),
						     modified_route));
		do_inform = true;
	    } else {
		//
		// Check whether the best route is now different. If yes, then
		// send the current best route instead (as a "replace" route).
		//
		if (old_winning_iter != winning_routes_ptr->end()) {
		    const StaticRoute& old_winning_route = old_winning_iter->second;
		    if (! best_route.is_same_route(old_winning_route)) {
			winning_routes_ptr->erase(old_winning_iter);
			modified_route = best_route;
			modified_route.set_replace_route();
			winning_routes_ptr->insert(make_pair(modified_route.network(),
							     modified_route));
			do_inform = true;
		    }
		}
	    }
	}
    }

    if (route.is_delete_route()) {
	if (old_winning_iter != winning_routes_ptr->end()) {
	    StaticRoute& old_winning_route = old_winning_iter->second;
	    if (route.is_same_route(old_winning_route)) {
		winning_routes_ptr->erase(old_winning_iter);
		do_inform = true;
		//
		// Check whether there is another best route. If yes, then
		// send the current best route instead (as a "replace" route).
		//
		if (best_iter != _static_routes.end()) {
		    StaticRoute& best_route = best_iter->second;
		    modified_route = best_route;
		    modified_route.set_replace_route();
		    winning_routes_ptr->insert(make_pair(modified_route.network(),
							 modified_route));
		}
	    }
	}
    }
    
    //
    // Inform the RIB about the change
    //
    if (do_inform)
	inform_rib_route_change(modified_route);
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
