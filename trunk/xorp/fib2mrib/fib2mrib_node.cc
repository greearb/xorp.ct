// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP: xorp/fib2mrib/fib2mrib_node.cc,v 1.12 2004/07/26 06:30:01 pavlin Exp $"


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
    shutdown();
}

bool
Fib2mribNode::startup()
{
    //
    // Test the service status
    //
    if ((ServiceBase::status() == STARTING)
	|| (ServiceBase::status() == RUNNING))
	return true;

    if (ServiceBase::status() != READY)
	return false;

    //
    // Transition to RUNNING occurs when all transient startup operations
    // are completed (e.g., after we have the interface/vif/address state
    // available, after we have registered with the RIB, etc.)
    //
    ServiceBase::set_status(STARTING);

    //
    // Set the node status
    //
    _node_status = PROC_STARTUP;

    //
    // Startup the interface manager
    //
    if (ifmgr_startup() != true) {
	ServiceBase::set_status(FAILED);
	return false;
    }

    //
    // Register as an FEA FIB client
    //
    fea_fib_client_register_startup();

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
    // We cannot shutdown if our status is SHUTDOWN or FAILED.
    //
    if ((ServiceBase::status() == SHUTDOWN)
	|| (ServiceBase::status() == FAILED)) {
	return true;
    }

    if (ServiceBase::status() != RUNNING)
	return false;

    //
    // Transition to SHUTDOWN occurs when all transient shutdown operations
    // are completed (e.g., after we have deregistered with the FEA
    // and the RIB, etc.)
    //
    ServiceBase::set_status(SHUTTING_DOWN);

    //
    // De-register with the RIB
    //
    rib_register_shutdown();

    //
    // De-register with as an FEA FIB client
    //
    fea_fib_client_register_shutdown();

    //
    // Shutdown the interface manager
    //
    if (ifmgr_shutdown() != true) {
	ServiceBase::set_status(FAILED);
	return false;
    }

    //
    // Set the node status
    //
    _node_status = PROC_SHUTDOWN;

    return true;
}

void
Fib2mribNode::status_change(ServiceBase*  service,
			    ServiceStatus old_status,
			    ServiceStatus new_status)
{
    if (service == this) {
	// My own status has changed
	if ((old_status == STARTING) && (new_status == RUNNING)) {
	    // The startup process has completed
	    _node_status = PROC_READY;
	    return;
	}

	if ((old_status == SHUTTING_DOWN) && (new_status == SHUTDOWN)) {
	    // The shutdown process has completed
	    _node_status = PROC_DONE;
	    return;
	}

	//
	// TODO: check if there was an error
	//
	return;
    }

    if (service == ifmgr_mirror_service_base()) {
	if ((old_status == SHUTTING_DOWN) && (new_status == SHUTDOWN)) {
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
			      static_cast<uint32_t>(_startup_requests_n));
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
			      static_cast<uint32_t>(_shutdown_requests_n));
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
    multimap<IPvXNet, Fib2mribRoute>::const_iterator route_iter;

    for (route_iter = _fib2mrib_routes.begin();
	 route_iter != _fib2mrib_routes.end();
	 ++route_iter) {
	const Fib2mribRoute& fib2mrib_route = route_iter->second;
	bool is_old_interface_up = false;
	bool is_new_interface_up = false;

	if (fib2mrib_route.is_interface_route()) {
	    //
	    // Calculate whether the interface was UP before and now.
	    //
	    const IfMgrVifAtom* vif_atom;
	    vif_atom = _iftree.find_vif(fib2mrib_route.ifname(),
					fib2mrib_route.vifname());
	    if ((vif_atom != NULL) && (vif_atom->enabled()))
		is_old_interface_up = true;
	    vif_atom = ifmgr_iftree().find_vif(fib2mrib_route.ifname(),
					       fib2mrib_route.vifname());
	    if ((vif_atom != NULL) && (vif_atom->enabled()))
		is_new_interface_up = true;
	} else {
	    //
	    // Calculate whether the next-hop router was directly connected
	    // before and now.
	    //
	    if (is_directly_connected(_iftree, fib2mrib_route.nexthop()))
		is_old_interface_up = true;
	    if (is_directly_connected(ifmgr_iftree(), fib2mrib_route.nexthop()))
		is_new_interface_up = true;
	}

	if (is_old_interface_up == is_new_interface_up)
	    continue;			// Nothing changed

	if (is_new_interface_up) {
	    //
	    // The interface is now up, hence add the route
	    //
	    inform_rib_route_change(fib2mrib_route);
	}
	if (! is_new_interface_up) {
	    //
	    // The interface went down, hence cancel all pending requests,
	    // and withdraw the route.
	    //
	    cancel_rib_route_change(fib2mrib_route);

	    Fib2mribRoute tmp_route(fib2mrib_route);
	    tmp_route.set_delete_route();	// XXX: mark as deleted route
	    inform_rib_route_change(tmp_route);
	}
    }

    //
    // Update the local copy of the interface tree
    //
    _iftree = ifmgr_iftree();
}

bool
Fib2mribNode::is_directly_connected(const IfMgrIfTree& if_tree,
				    const IPvX& addr) const
{
    IfMgrIfTree::IfMap::const_iterator if_iter;

    for (if_iter = if_tree.ifs().begin();
	 if_iter != if_tree.ifs().end();
	 ++if_iter) {
	const IfMgrIfAtom& iface = if_iter->second;

	// Test if interface is enabled
	if (! iface.enabled())
	    continue;

	IfMgrIfAtom::VifMap::const_iterator vif_iter;
	for (vif_iter = iface.vifs().begin();
	     vif_iter != iface.vifs().end();
	     ++vif_iter) {
	    const IfMgrVifAtom& vif = vif_iter->second;

	    // Test if vif is enabled
	    if (! vif.enabled())
		continue;

	    // Test if there is matching IPv4 address
	    if (addr.is_ipv4()) {
		IPv4 addr4 = addr.get_ipv4();
		IfMgrVifAtom::V4Map::const_iterator a4_iter;

		for (a4_iter = vif.ipv4addrs().begin();
		     a4_iter != vif.ipv4addrs().end();
		     ++a4_iter) {
		    const IfMgrIPv4Atom& a4 = a4_iter->second;

		    if (! a4.enabled())
			continue;

		    // Test if my own address
		    if (a4.addr() == addr4)
			return (true);

		    // Test if p2p address
		    if (a4.has_endpoint()) {
			if (a4.endpoint_addr() == addr4)
			    return (true);
		    }

		    // Test if same subnet
		    if (IPv4Net(addr4, a4.prefix_len())
			== IPv4Net(a4.addr(), a4.prefix_len())) {
			return (true);
		    }
		}
	    }

	    // Test if there is matching IPv6 address
	    if (addr.is_ipv6()) {
		IPv6 addr6 = addr.get_ipv6();
		IfMgrVifAtom::V6Map::const_iterator a6_iter;

		for (a6_iter = vif.ipv6addrs().begin();
		     a6_iter != vif.ipv6addrs().end();
		     ++a6_iter) {
		    const IfMgrIPv6Atom& a6 = a6_iter->second;

		    if (! a6.enabled())
			continue;

		    // Test if my own address
		    if (a6.addr() == addr6)
			return (true);

		    // Test if p2p address
		    if (a6.has_endpoint()) {
			if (a6.endpoint_addr() == addr6)
			    return (true);
		    }

		    // Test if same subnet
		    if (IPv6Net(addr6, a6.prefix_len())
			== IPv6Net(a6.addr(), a6.prefix_len())) {
			return (true);
		    }
		}
	    }
	}
    }

    return (false);
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
    multimap<IPvXNet, Fib2mribRoute>::iterator iter;
    iter = _fib2mrib_routes.find(fib2mrib_route.network());
    for ( ; iter != _fib2mrib_routes.end(); ++iter) {
	Fib2mribRoute& tmp_route = iter->second;
	if (tmp_route.network() != fib2mrib_route.network())
	    break;
	if ((tmp_route.ifname() != fib2mrib_route.ifname())
	    || (tmp_route.vifname() != fib2mrib_route.vifname())) {
	    continue;
	}
	error_msg = c_format("Cannot add route for %s: "
			     "the route already exists",
			     fib2mrib_route.network().str().c_str());
	return XORP_ERROR;
    }

    //
    // Add the route
    //
    _fib2mrib_routes.insert(make_pair(fib2mrib_route.network(),
				      fib2mrib_route));

    //
    // Inform the RIB about the change
    //
    if (fib2mrib_route.is_interface_route()) {
	const IfMgrVifAtom* vif_atom;
	vif_atom = _iftree.find_vif(fib2mrib_route.ifname(),
				    fib2mrib_route.vifname());
	if ((vif_atom != NULL) && (vif_atom->enabled()))
	    inform_rib_route_change(fib2mrib_route);
    } else {
	if (is_directly_connected(_iftree, fib2mrib_route.nexthop()))
	    inform_rib_route_change(fib2mrib_route);
    }

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
    bool is_route_replaced = false;

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
    multimap<IPvXNet, Fib2mribRoute>::iterator iter, iter2;
    iter = _fib2mrib_routes.find(fib2mrib_route.network());
    if ((iter == _fib2mrib_routes.end())
	|| (iter->second.network() != fib2mrib_route.network())) {
	//
	// Couldn't find the route to replace
	//
	error_msg = c_format("Cannot replace route for %s: "
			     "no such route",
			     fib2mrib_route.network().str().c_str());
	return XORP_ERROR;
    }

    //
    // First check if there is a route with the same ifname and vifname.
    // If there is such route, then update its value. Otherwise,
    // update the first route for the same subnet.
    //
    for (iter2 = iter; iter2 != _fib2mrib_routes.end(); ++iter2) {
	Fib2mribRoute& tmp_route = iter2->second;
	if (tmp_route.network() != fib2mrib_route.network())
	    break;
	if ((tmp_route.ifname() != fib2mrib_route.ifname())
	    || (tmp_route.vifname() != fib2mrib_route.vifname())) {
	    continue;
	}

	//
	// Route found. Overwrite its value.
	//
	tmp_route = fib2mrib_route;
	is_route_replaced = true;
	break;
    }

    if (! is_route_replaced) {
	//
	// No route found with the same ifname and vifname.
	// Update the first route for the same subnet.
	//
	Fib2mribRoute& tmp_route = iter->second;

	tmp_route = fib2mrib_route;
    }

    //
    // Inform the RIB about the change
    //
    if (fib2mrib_route.is_interface_route()) {
	const IfMgrVifAtom* vif_atom;
	vif_atom = _iftree.find_vif(fib2mrib_route.ifname(),
				    fib2mrib_route.vifname());
	if ((vif_atom != NULL) && (vif_atom->enabled()))
	    inform_rib_route_change(fib2mrib_route);
    } else {
	if (is_directly_connected(_iftree, fib2mrib_route.nexthop()))
	    inform_rib_route_change(fib2mrib_route);
    }

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
    multimap<IPvXNet, Fib2mribRoute>::iterator iter, iter2;
    iter = _fib2mrib_routes.find(fib2mrib_route.network());
    if ((iter == _fib2mrib_routes.end())
	|| (iter->second.network() != fib2mrib_route.network())) {
	//
	// Couldn't find the route to delete
	//
    error_label:
	error_msg = c_format("Cannot delete route for %s: "
			     "no such route",
			     fib2mrib_route.network().str().c_str());
	return XORP_ERROR;
    }

    //
    // First check if there is a route with the same ifname and vifname.
    // If there is such route, then delete it. Otherwise,
    // delete the first route for the same subnet.
    //
    for (iter2 = iter; iter2 != _fib2mrib_routes.end(); ++iter2) {
	Fib2mribRoute& tmp_route = iter2->second;
	if (tmp_route.network() != fib2mrib_route.network())
	    break;
	if ((tmp_route.ifname() != fib2mrib_route.ifname())
	    || (tmp_route.vifname() != fib2mrib_route.vifname())) {
	    continue;
	}

	//
	// Route found
	//
	break;
    }

    if (iter2 == _fib2mrib_routes.end()
	|| (iter2->second.network() != fib2mrib_route.network())) {
	//
	// No route found with the same ifname and vifname.
	// If this is an interface-specific route, then this is an error.
	// Otherwise, delete the first route for the same subnet.
	//
	if (fib2mrib_route.is_interface_route())
	    goto error_label;
	iter2 = iter;
    }

    //
    // Create a copy of the route and erase it.
    //
    Fib2mribRoute copy_route = iter2->second;
    copy_route.set_delete_route();
    _fib2mrib_routes.erase(iter2);

    //
    // Inform the RIB about the change
    //
    if (copy_route.is_interface_route()) {
	const IfMgrVifAtom* vif_atom;
	vif_atom = _iftree.find_vif(copy_route.ifname(),
				    copy_route.vifname());
	if ((vif_atom != NULL) && (vif_atom->enabled()))
	    inform_rib_route_change(copy_route);
    } else {
	if (is_directly_connected(_iftree, copy_route.nexthop()))
	    inform_rib_route_change(copy_route);
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
Fib2mribRoute::is_valid_entry(string& error_msg) const
{
    UNUSED(error_msg);

    return true;
}
