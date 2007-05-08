// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/fib2mrib/fib2mrib_node.cc,v 1.35 2007/04/23 23:05:09 pavlin Exp $"

//
// Fib2mrib node implementation.
//

#include "fib2mrib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "fib2mrib_node.hh"
#include "fib2mrib_varrw.hh"


Fib2mribNode::Fib2mribNode(EventLoop& eventloop)
    : ServiceBase("Fib2mrib"),
      _eventloop(eventloop),
      _protocol_name("fib2mrib"),	// TODO: must be known by RIB
      _is_enabled(true),		// XXX: enabled by default
      _startup_requests_n(0),
      _shutdown_requests_n(0),
      _is_log_trace(true)		// XXX: default to print trace logs
{
    set_node_status(PROC_STARTUP);
}

Fib2mribNode::~Fib2mribNode()
{
    shutdown();
}

bool
Fib2mribNode::startup()
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
Fib2mribNode::shutdown()
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
Fib2mribNode::status_change(ServiceBase*  service,
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
Fib2mribNode::tree_complete()
{
    decr_startup_requests_n();

    //
    // XXX: we use same actions when the tree is completed or updates are made
    //
    updates_made();
}

void
Fib2mribNode::updates_made()
{
    multimap<IPvXNet, Fib2mribRoute>::iterator route_iter;
    list<Fib2mribRoute *> add_routes, replace_routes, delete_routes;
    list<Fib2mribRoute *>::iterator pending_iter;

    for (route_iter = _fib2mrib_routes.begin();
	 route_iter != _fib2mrib_routes.end();
	 ++route_iter) {
	Fib2mribRoute& fib2mrib_route = route_iter->second;
	bool is_old_up = false;
	bool is_new_up = false;
	string old_ifname, old_vifname, new_ifname, new_vifname;

	update_route(ifmgr_iftree(), fib2mrib_route);

	if (fib2mrib_route.is_interface_route()) {
	    //
	    // Calculate whether the interface was UP before and now.
	    //
	    const IfMgrIfAtom* if_atom;
	    const IfMgrVifAtom* vif_atom;

	    if_atom = _iftree.find_interface(fib2mrib_route.ifname());
	    vif_atom = _iftree.find_vif(fib2mrib_route.ifname(),
					fib2mrib_route.vifname());
	    if ((if_atom != NULL) && (if_atom->enabled())
		&& (! if_atom->no_carrier())
		&& (vif_atom != NULL) && (vif_atom->enabled())) {
		is_old_up = true;
	    }

	    if_atom = ifmgr_iftree().find_interface(fib2mrib_route.ifname());
	    vif_atom = ifmgr_iftree().find_vif(fib2mrib_route.ifname(),
					       fib2mrib_route.vifname());
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
	    if (_iftree.is_directly_connected(fib2mrib_route.nexthop(),
					      old_ifname,
					      old_vifname)) {
		is_old_up = true;
	    }
	    if (ifmgr_iftree().is_directly_connected(fib2mrib_route.nexthop(),
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
	    add_routes.push_back(&fib2mrib_route);
	    continue;
	}
	if ((is_old_up) && (! is_new_up)) {
	    //
	    // The interface went down, hence cancel all pending requests,
	    // and withdraw the route.
	    //
	    delete_routes.push_back(&fib2mrib_route);
	    continue;
	}
	if (is_old_up && is_new_up) {
	    //
	    // The interface remains up, hence probably the interface or
	    // the vif name has changed.
	    // Delete the route and then add it again so the information
	    // in the RIB will be updated.
	    //
	    replace_routes.push_back(&fib2mrib_route);
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
	Fib2mribRoute& orig_route = *(*pending_iter);
	Fib2mribRoute copy_route = orig_route;
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
	Fib2mribRoute& orig_route = *(*pending_iter);
	Fib2mribRoute copy_route = orig_route;
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
	Fib2mribRoute& orig_route = *(*pending_iter);
	cancel_rib_route_change(orig_route);
	Fib2mribRoute copy_route = orig_route;
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
Fib2mribNode::set_enabled(bool enable)
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
 * Add an IPv4 route.
 *
 * @param network the network address prefix this route applies to.
 * @param nexthop the address of the next-hop router for this route.
 * @param ifname the name of the physical interface toward the
 * destination.
 * @param vifname the name of the virtual interface toward the
 * destination.
 * @param metric the routing metric for this route.
 * @param admin_distance the administratively defined distance for this
 * route.
 * @param protocol_origin the name of the protocol that originated this
 * route.
 * @param xorp_route true if this route was installed by XORP.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
Fib2mribNode::add_route4(const IPv4Net& network, const IPv4& nexthop,
			 const string& ifname, const string& vifname,
			 uint32_t metric, uint32_t admin_distance,
			 const string& protocol_origin, bool xorp_route,
			 string& error_msg)
{
    Fib2mribRoute fib2mrib_route(network, nexthop, ifname, vifname,
				 metric, admin_distance,
				 protocol_origin, xorp_route);

    fib2mrib_route.set_add_route();

    return (add_route(fib2mrib_route, error_msg));
}

/**
 * Add an IPv6 route.
 *
 * @param network the network address prefix this route applies to.
 * @param nexthop the address of the next-hop router for this route.
 * @param ifname the name of the physical interface toward the
 * destination.
 * @param vifname the name of the virtual interface toward the
 * destination.
 * @param metric the routing metric for this route.
 * @param admin_distance the administratively defined distance for this
 * route.
 * @param protocol_origin the name of the protocol that originated this
 * route.
 * @param xorp_route true if this route was installed by XORP.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
Fib2mribNode::add_route6(const IPv6Net& network, const IPv6& nexthop,
			 const string& ifname, const string& vifname,
			 uint32_t metric, uint32_t admin_distance,
			 const string& protocol_origin, bool xorp_route,
			 string& error_msg)
{
    Fib2mribRoute fib2mrib_route(network, nexthop, ifname, vifname,
				 metric, admin_distance,
				 protocol_origin, xorp_route);

    fib2mrib_route.set_add_route();

    return (add_route(fib2mrib_route, error_msg));
}

/**
 * Replace an IPv4 route.
 *
 * @param network the network address prefix this route applies to.
 * @param nexthop the address of the next-hop router for this route.
 * @param ifname the name of the physical interface toward the
 * destination.
 * @param vifname the name of the virtual interface toward the
 * destination.
 * @param metric the routing metric for this route.
 * @param admin_distance the administratively defined distance for this
 * route.
 * @param protocol_origin the name of the protocol that originated this
 * route.
 * @param xorp_route true if this route was installed by XORP.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
Fib2mribNode::replace_route4(const IPv4Net& network, const IPv4& nexthop,
			     const string& ifname, const string& vifname,
			     uint32_t metric, uint32_t admin_distance,
			     const string& protocol_origin, bool xorp_route,
			     string& error_msg)
{
    Fib2mribRoute fib2mrib_route(network, nexthop, ifname, vifname,
				 metric, admin_distance,
				 protocol_origin, xorp_route);

    fib2mrib_route.set_replace_route();

    return (replace_route(fib2mrib_route, error_msg));
}

/**
 * Replace an IPv6 route.
 *
 * @param network the network address prefix this route applies to.
 * @param nexthop the address of the next-hop router for this route.
 * @param ifname the name of the physical interface toward the
 * destination.
 * @param vifname the name of the virtual interface toward the
 * destination.
 * @param metric the routing metric for this route.
 * @param admin_distance the administratively defined distance for this
 * route.
 * @param protocol_origin the name of the protocol that originated this
 * route.
 * @param xorp_route true if this route was installed by XORP.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
Fib2mribNode::replace_route6(const IPv6Net& network, const IPv6& nexthop,
			     const string& ifname, const string& vifname,
			     uint32_t metric, uint32_t admin_distance,
			     const string& protocol_origin, bool xorp_route,
			     string& error_msg)
{
    Fib2mribRoute fib2mrib_route(network, nexthop, ifname, vifname,
				 metric, admin_distance,
				 protocol_origin, xorp_route);

    fib2mrib_route.set_replace_route();

    return (replace_route(fib2mrib_route, error_msg));
}

/**
 * Delete an IPv4 route.
 *
 * @param network the network address prefix this route applies to.
 * @param ifname the name of the physical interface toward the
 * destination.
 * @param vifname the name of the virtual interface toward the
 * destination.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
Fib2mribNode::delete_route4(const IPv4Net& network, const string& ifname,
			    const string& vifname, string& error_msg)
{
    Fib2mribRoute fib2mrib_route(network, network.masked_addr().ZERO(),
				 ifname, vifname, 0, 0, "", false);

    fib2mrib_route.set_delete_route();

    return (delete_route(fib2mrib_route, error_msg));
}

/**
 * Delete an IPv6 route.
 *
 * @param network the network address prefix this route applies to.
 * @param ifname the name of the physical interface toward the
 * destination.
 * @param vifname the name of the virtual interface toward the
 * destination.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
Fib2mribNode::delete_route6(const IPv6Net& network, const string& ifname,
			    const string& vifname, string& error_msg)
{
    Fib2mribRoute fib2mrib_route(network, network.masked_addr().ZERO(),
				 ifname, vifname, 0, 0, "", false);

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
    Fib2mribRoute updated_route = fib2mrib_route;

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
    multimap<IPvXNet, Fib2mribRoute>::iterator iter;
    iter = _fib2mrib_routes.find(updated_route.network());
    for ( ; iter != _fib2mrib_routes.end(); ++iter) {
	Fib2mribRoute& orig_route = iter->second;
	if (orig_route.network() != updated_route.network())
	    break;

	//
	// XXX: Route found. Ideally, if we receive add_route() from
	// the FEA, we should have received delete_route() before.
	// However, on FreeBSD-4.10 (at least), if a route points
	// to the local address of an interface, and if we delete
	// that address, the kernel automatically removes all affected
	// routes without reporting the routing information change
	// to the process(es) listening on routing sockets.
	// Therefore, to deal with such (mis)behavior of the FEA,
	// we just replace the previously received route with the
	// new one.
	//
	updated_route.set_replace_route();
	return (replace_route(updated_route, error_msg));
    }

    //
    // Add the route
    //
    iter = _fib2mrib_routes.insert(make_pair(updated_route.network(),
					     updated_route));

    //
    // Create a copy of the route and inform the RIB if necessary
    //
    Fib2mribRoute& orig_route = iter->second;
    Fib2mribRoute copy_route = orig_route;
    prepare_route_for_transmission(orig_route, copy_route);

    //
    // Inform the RIB about the change
    //
    inform_rib(copy_route);

    return XORP_OK;
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
    Fib2mribRoute updated_route = fib2mrib_route;
    Fib2mribRoute* route_to_replace_ptr = NULL;

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
    // If there is a route with the same ifname and vifname, then update
    // its value. Otherwise, update the first route for the same subnet.
    //
    multimap<IPvXNet, Fib2mribRoute>::iterator iter;
    iter = _fib2mrib_routes.find(updated_route.network());
    for ( ; iter != _fib2mrib_routes.end(); ++iter) {
	Fib2mribRoute& orig_route = iter->second;
	if (orig_route.network() != updated_route.network())
	    break;

	if ((orig_route.ifname() != updated_route.ifname())
	    || (orig_route.vifname() != updated_route.vifname())) {
	    if (route_to_replace_ptr == NULL) {
		// First route for same subnet
		route_to_replace_ptr = &orig_route;
	    }
	    continue;
	}

	route_to_replace_ptr = &orig_route;
	break;
    }
    if (route_to_replace_ptr == NULL) {
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
	Fib2mribRoute& orig_route = *route_to_replace_ptr;

	bool was_accepted = orig_route.is_accepted_by_rib();
	orig_route = updated_route;

	//
	// Create a copy of the route and inform the RIB if necessary
	//
	Fib2mribRoute copy_route = orig_route;
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
    Fib2mribRoute updated_route = fib2mrib_route;
    multimap<IPvXNet, Fib2mribRoute>::iterator route_to_delete_iter = _fib2mrib_routes.end();

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
    // If there is a route with the same ifname and vifname, then delete it.
    // Otherwise, if the route to delete is not interface-specific,
    // delete the first route for the same subnet.
    //
    multimap<IPvXNet, Fib2mribRoute>::iterator iter;
    iter = _fib2mrib_routes.find(updated_route.network());
    for ( ; iter != _fib2mrib_routes.end(); ++iter) {
	Fib2mribRoute& orig_route = iter->second;
	if (orig_route.network() != updated_route.network())
	    break;

	if ((orig_route.ifname() != updated_route.ifname())
	    || (orig_route.vifname() != updated_route.vifname())) {
	    if (route_to_delete_iter == _fib2mrib_routes.end()) {
		// First route for same subnet
		if (! updated_route.is_interface_route())
		    route_to_delete_iter = iter;
	    }
	    continue;
	}

	route_to_delete_iter = iter;
	break;
    }
    if (route_to_delete_iter == _fib2mrib_routes.end()) {
	//
	// Couldn't find the route to delete
	//
	error_msg = c_format("Cannot delete route for %s: "
			     "no such route",
			     updated_route.network().str().c_str());
	return XORP_ERROR;
    }

    //
    // Route found. Create a copy of it and erase it.
    //
    do {
	Fib2mribRoute& orig_route = route_to_delete_iter->second;

	bool was_accepted = orig_route.is_accepted_by_rib();
	Fib2mribRoute copy_route = orig_route;
	prepare_route_for_transmission(orig_route, copy_route);
	_fib2mrib_routes.erase(route_to_delete_iter);

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
    } while (false);

    return XORP_OK;
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
    UNUSED(error_msg);

    return true;
}

/**
 * Test whether the route is accepted for transmission to the RIB.
 *
 * @return true if route is accepted for transmission to the RIB,
 * otherwise false.
 */
bool
Fib2mribRoute::is_accepted_by_rib() const
{
    return (is_accepted_by_nexthop() && (! is_filtered()));
}

void
Fib2mribNode::configure_filter(const uint32_t& filter, const string& conf)
{
    _policy_filters.configure(filter, conf);
}

void
Fib2mribNode::reset_filter(const uint32_t& filter) {
    _policy_filters.reset(filter);
}

void
Fib2mribNode::push_routes()
{
    multimap<IPvXNet, Fib2mribRoute>::iterator iter;

    // XXX: not a background task
    for (iter = _fib2mrib_routes.begin();
	 iter != _fib2mrib_routes.end();
	 ++iter) {
	Fib2mribRoute& orig_route = iter->second;
	bool was_accepted = orig_route.is_accepted_by_rib();

	//
	// Create a copy of the route and inform the RIB if necessary
	//
	Fib2mribRoute copy_route = orig_route;
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
Fib2mribNode::push_pull_rib_routes(bool is_push)
{
    multimap<IPvXNet, Fib2mribRoute>::iterator iter;

    // XXX: not a background task
    for (iter = _fib2mrib_routes.begin();
	 iter != _fib2mrib_routes.end(); ++iter) {
	Fib2mribRoute& orig_route = iter->second;

	//
	// Create a copy of the route and inform the RIB if necessary
	//
	Fib2mribRoute copy_route = orig_route;
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
Fib2mribNode::is_accepted_by_nexthop(const Fib2mribRoute& route) const
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
Fib2mribNode::prepare_route_for_transmission(Fib2mribRoute& orig_route,
					     Fib2mribRoute& copy_route)
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
Fib2mribNode::inform_rib(const Fib2mribRoute& route)
{
    if (! is_enabled())
	return;

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
 * Update a route received from the FEA.
 * 
 * This method is needed as a work-around of FEA-related problems
 * with the routes the FEA sends to interested parties such as FIB2MRIB.
 * A route is updated with interface-related information or next-hop
 * address.
 * 
 * This method is needed as a work-around of FEA-related problems
 * with the routes the FEA sends to interested parties such as FIB2MRIB.
 * 
 * The routes received from the FEA for the directly-connected subnets
 * may not contain next-hop information and network interface information.
 * If the route is for a directly-connected subnet, and if it is missing
 * that information, then add the interface and next-hop router
 * information.
 * Furthermore, on startup, the routes received from the FEA may
 * contain interface-related information, but later updates of those
 * routes may be missing this information. This may create a mismatch,
 * therefore all routes are updated (if possible) to contain
 * interface-related information.
 * 
 * @param iftree the tree with the interface state to update the route.
 * @param route the route to update.
 * @return true if the route was updated, otherwise false.
 */
bool
Fib2mribNode::update_route(const IfMgrIfTree& iftree, Fib2mribRoute& route)
{
    //
    // Test if the route needs to be updated
    //
    if (route.is_interface_route())
	return (false);

    //
    // First find if the next-hop address is one of our interfaces.
    //
    string ifname, vifname;
    if (iftree.is_my_addr(route.nexthop(), ifname, vifname)) {
	route.set_ifname(ifname);
	route.set_vifname(vifname);
	return (true);
    }

    //
    // Find if there is a directly-connected subnet that matches the route
    // or the address of the next-hop router.
    //
    IfMgrIfTree::IfMap::const_iterator if_iter;
    for (if_iter = iftree.interfaces().begin();
	 if_iter != iftree.interfaces().end();
	 ++if_iter) {
	const IfMgrIfAtom& iface = if_iter->second;

	IfMgrIfAtom::VifMap::const_iterator vif_iter;
	for (vif_iter = iface.vifs().begin();
	     vif_iter != iface.vifs().end();
	     ++vif_iter) {
	    const IfMgrVifAtom& vif = vif_iter->second;

	    //
	    // Check the IPv4 subnets
	    //
	    if (route.is_ipv4()) {
		IfMgrVifAtom::IPv4Map::const_iterator a4_iter;
		for (a4_iter = vif.ipv4addrs().begin();
		     a4_iter != vif.ipv4addrs().end();
		     ++a4_iter) {
		    const IfMgrIPv4Atom& a4 = a4_iter->second;
		    IPvXNet ipvxnet(a4.addr(), a4.prefix_len());

		    // Update a directly-connected route
		    if (ipvxnet == route.network()) {
			route.set_ifname(iface.name());
			route.set_vifname(vif.name());
			if (route.nexthop().is_zero())
			    route.set_nexthop(a4.addr());
			return (true);
		    }

		    // Update the route if a directly-connected next-hop
		    if (ipvxnet.contains(route.nexthop())
			&& (! route.nexthop().is_zero())) {
			route.set_ifname(iface.name());
			route.set_vifname(vif.name());
			return (true);
		    }
		}
	    }

	    //
	    // Check the IPv6 subnets
	    //
	    if (route.is_ipv6()) {
		IfMgrVifAtom::IPv6Map::const_iterator a6_iter;
		for (a6_iter = vif.ipv6addrs().begin();
		     a6_iter != vif.ipv6addrs().end();
		     ++a6_iter) {
		    const IfMgrIPv6Atom& a6 = a6_iter->second;
		    IPvXNet ipvxnet(a6.addr(), a6.prefix_len());

		    // Update a directly-connected route
		    if (ipvxnet == route.network()) {
			route.set_ifname(iface.name());
			route.set_vifname(vif.name());
			if (route.nexthop().is_zero())
			    route.set_nexthop(a6.addr());
			return (true);
		    }

		    // Update the route if a directly-connected next-hop
		    if (ipvxnet.contains(route.nexthop())
			&& (! route.nexthop().is_zero())) {
			route.set_ifname(iface.name());
			route.set_vifname(vif.name());
			return (true);
		    }
		}
	    }
	}
    }

    return (false);
}

bool
Fib2mribNode::do_filtering(Fib2mribRoute& route)
{
    try {
	Fib2mribVarRW varrw(route);

	// Import filtering
	bool accepted;

	debug_msg("[FIB2MRIB] Running filter: %s on route: %s\n",
		  filter::filter2str(filter::IMPORT).c_str(),
		  route.network().str().c_str());
	accepted = _policy_filters.run_filter(filter::IMPORT, varrw);

	route.set_filtered(!accepted);

	// Route Rejected 
	if (!accepted) 
	    return accepted;

	Fib2mribVarRW varrw2(route);

	// Export source-match filtering
	debug_msg("[FIB2MRIB] Running filter: %s on route: %s\n",
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
