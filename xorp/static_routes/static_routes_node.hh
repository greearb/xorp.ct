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

// $XORP: xorp/static_routes/static_routes_node.hh,v 1.32 2008/10/02 21:58:29 bms Exp $

#ifndef __STATIC_ROUTES_STATIC_ROUTES_NODE_HH__
#define __STATIC_ROUTES_STATIC_ROUTES_NODE_HH__


//
// StaticRoutes node definition.
//



#include "libxorp/service.hh"
#include "libxorp/status_codes.h"

#include "libfeaclient/ifmgr_xrl_mirror.hh"

#include "policy/backend/policytags.hh"
#include "policy/backend/policy_filters.hh"

class EventLoop;

/**
 * @short A StaticRoute helper class.
 * 
 * This class is used to store a routing entry.
 */
class StaticRoute {
public:
    /**
     * Constructor for a given IPv4 static route.
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
     * @param is_backup_route if true, then this is a backup route.
     */
    StaticRoute(bool unicast, bool multicast,
		const IPv4Net& network, const IPv4& nexthop,
		const string& ifname, const string& vifname,
		uint32_t metric, bool is_backup_route)
	: _unicast(unicast), _multicast(multicast),
	  _network(network), _nexthop(nexthop),
	  _ifname(ifname), _vifname(vifname),
	  _metric(metric), _is_backup_route(is_backup_route),
	  _route_type(IDLE_ROUTE), _is_ignored(false),
	  _is_filtered(false), _is_accepted_by_nexthop(false) {}

#ifdef XORP_USE_USTL
    StaticRoute() { }
#endif

    /**
     * Constructor for a given IPv6 static route.
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
     * @param is_backup_route if true, then this is a backup route.
     */
    StaticRoute(bool unicast, bool multicast,
		const IPv6Net& network, const IPv6& nexthop,
		const string& ifname, const string& vifname,
		uint32_t metric, bool is_backup_route)
	: _unicast(unicast), _multicast(multicast),
	  _network(network), _nexthop(nexthop),
	  _ifname(ifname), _vifname(vifname),
	  _metric(metric), _is_backup_route(is_backup_route),
	  _route_type(IDLE_ROUTE), _is_ignored(false),
	  _is_filtered(false), _is_accepted_by_nexthop(false) {}

    /**
     * Equality Operator
     * 
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically same as the
     * right-hand operand.
     */
    bool operator==(const StaticRoute& other) const {
	return (is_same_route(other)
		&& (_route_type == other._route_type)
		&& (_policytags == other._policytags));
    }

    /**
     * Test whether both routes contain same routing information.
     *
     * @param other the route to compare against.
     * @return true if both routes contain same routing information,
     * otherwise false.
     */
    bool is_same_route(const StaticRoute& other) const {
	return ((_unicast == other.unicast())
		&& (_multicast == other.multicast())
		&& (_network == other.network())
		&& (_nexthop == other.nexthop())
		&& (_ifname == other.ifname())
		&& (_vifname == other.vifname())
		&& (_metric == other.metric()));
    }

    /**
     * Test if this is an IPv4 route.
     * 
     * @return true if this is an IPv4 route, otherwise false.
     */
    bool is_ipv4() const { return _network.is_ipv4(); }

    /**
     * Test if this is an IPv6 route.
     * 
     * @return true if this is an IPv6 route, otherwise false.
     */
    bool is_ipv6() const { return _network.is_ipv6(); }

    /**
     * Test if this route would be used for unicast routing.
     * 
     * @return true if this route would be used for unicast routing,
     * otherwise false.
     */
    bool unicast() const { return _unicast; }

    /**
     * Test if this route would be used for multicast routing.
     * 
     * @return true if this route would be used for multicast routing,
     * otherwise false.
     */
    bool multicast() const { return _multicast; }

    /**
     * Get the network address prefix this route applies to.
     * 
     * @return the network address prefix this route appies to.
     */
    const IPvXNet& network() const { return _network; }

    /**
     * Get the address of the next-hop router for this route.
     * 
     * @return the address of the next-hop router for this route.
     */
    const IPvX& nexthop() const { return _nexthop; }

    /**
     * Set the address of the next-hop router for this route.
     *
     * @param v the address of the next-hop router for this route.
     */
    void set_nexthop(const IPvX& v) { _nexthop = v; }

    /**
     * Get the name of the physical interface toward the destination.
     * 
     * @return the name of the physical interface toward the destination.
     */
    const string& ifname() const { return _ifname; }

    /**
     * Set the name of the physical interface toward the destination.
     *
     * @param v the name of the physical interface toward the destination.
     */
    void set_ifname(const string& v) { _ifname = v; }

    /**
     * Get the name of the virtual interface toward the destination.
     * 
     * @return the name of the virtual interface toward the destination.
     */
    const string& vifname() const { return _vifname; }

    /**
     * Set the name of the virtual interface toward the destination.
     *
     * @param v the name of the virtual interface toward the destination.
     */
    void set_vifname(const string& v) { _vifname = v; }

    /**
     * Get the metric distance for this route.
     * 
     * @return the metric distance for this route.
     */
    uint32_t metric() const { return _metric; }

    /**
     * Test if this is a backup route.
     *
     * @return truf if this is a backup route, otherwise false.
     */
    bool is_backup_route() const { return _is_backup_route; }

    /**
     * Test if this is a route to add.
     * 
     * @return true if this is a route to add, otherwise false.
     */
    bool is_add_route() const { return (_route_type == ADD_ROUTE); }

    /**
     * Test if this is a replacement route.
     * 
     * @return true if this is a replacement route, otherwise false.
     */
    bool is_replace_route() const { return (_route_type == REPLACE_ROUTE); }

    /**
     * Test if this is a route to delete.
     * 
     * @return true if this is a route to delete, otherwise false.
     */
    bool is_delete_route() const { return (_route_type == DELETE_ROUTE); }

    /**
     * Set the type of this route to "a route to add".
     */
    void set_add_route() { _route_type = ADD_ROUTE; }

    /**
     * Set the type of this route to "a replacement route".
     */
    void set_replace_route() { _route_type = REPLACE_ROUTE; }

    /**
     * Set the type of this route to "a route to delete".
     */
    void set_delete_route() { _route_type = DELETE_ROUTE; }

    /**
     * Test if the route is interface-specific (e.g., if the interface
     * is explicitly specified).
     * 
     * @return true if the route is interface-specific, otherwise false.
     */
    bool is_interface_route() const { return ! (_ifname.empty()
						&& _vifname.empty()); }

    /**
     * Check whether the route entry is valid.
     * 
     * @param error_msg the error message (if error).
     * @return true if the route entry is valid, otherwise false.
     */
    bool is_valid_entry(string& error_msg) const;

    /**
     * Test if the route is to be ignored.
     * 
     * This method is used only for internal purpose when passing the route
     * around.
     * 
     * @return true if the route is to be ignored, otherwise false.
     */
    bool is_ignored() const { return _is_ignored; }

    /**
     * Set whether the route is to be ignored.
     * 
     * This method is used only for internal purpose when passing the route
     * around.
     * 
     * @param v true if the route is to be ignored, otherwise false.
     */
    void set_ignored(bool v) { _is_ignored = v; }

    /**
     * @return policy-tags for this route.
     */
    PolicyTags& policytags() { return _policytags; }

    /**
     * Test whether the route has been rejected by a policy filter.
     *
     * @return true if route has been rejected by a policy filter, otherwise
     * false.
     */
    bool is_filtered() const { return _is_filtered; }

    /**
     * Set a flag that indicates whether the route is to be considered
     * filtered [rejected by the policy filter].
     *
     * @param v true if the route should be considered filtered, otherwise
     * false.
     */
    void set_filtered(bool v) { _is_filtered = v; }

    /**
     * Test whether the route is accepted based on its next-hop information.
     *
     * @return true if the route is accepted based on its next-hop
     * information, otherwise false.
     */
    bool is_accepted_by_nexthop() const { return _is_accepted_by_nexthop; }

    /**
     * Set a flag that indicates whether the route is accepted based
     * on its next-hop information.
     *
     * @param v true if the route is accepted based on its next-hop
     * information, otherwise false.
     */
    void set_accepted_by_nexthop(bool v) { _is_accepted_by_nexthop = v; }

    /**
     * Test whether the route is accepted for transmission to the RIB.
     *
     * @return true if route is accepted for transmission to the RIB,
     * otherwise false.
     */
    bool is_accepted_by_rib() const;

private:
    bool	_unicast;
    bool	_multicast;
    IPvXNet	_network;
    IPvX	_nexthop;
    string	_ifname;
    string	_vifname;
    uint32_t	_metric;
    bool	_is_backup_route;
    enum RouteType { IDLE_ROUTE, ADD_ROUTE, REPLACE_ROUTE, DELETE_ROUTE };
    RouteType	_route_type;
    bool	_is_ignored;	// True if the route is to be ignored
    bool	_is_filtered;	// True if rejected by a policy filter
    bool	_is_accepted_by_nexthop; // True if the route is accepted based on its next-hop information
    PolicyTags	_policytags;
};


/**
 * @short The StaticRoutes node class.
 * 
 * There should be one node per StaticRoutes instance.
 */
class StaticRoutesNode : public IfMgrHintObserver,
			 public ServiceBase,
			 public ServiceChangeObserverBase {
public:
    typedef multimap<IPvXNet, StaticRoute> Table;

    /**
     * Constructor for a given event loop.
     * 
     * @param eventloop the event loop to use.
     */
    StaticRoutesNode(EventLoop& eventloop);

    /**
     * Destructor
     */
    virtual ~StaticRoutesNode();

    /**
     * Get the event loop this node is added to.
     * 
     * @return the event loop this node is added to.
     */
    EventLoop&	eventloop()	{ return _eventloop; }

    /**
     * Get the protocol name.
     * 
     * @return a string with the protocol name.
     */
    const string& protocol_name() const { return _protocol_name; }

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
     * Get the node status (see @ref ProcessStatus).
     * 
     * @param reason_msg return-by-reference string that contains
     * human-readable information about the status.
     * @return the node status (see @ref ProcessStatus).
     */
    ProcessStatus	node_status(string& reason_msg);

    /**
     * Test if the node processing is done.
     * 
     * @return true if the node processing is done, otherwise false.
     */
    bool	is_done() const { return (_node_status == PROC_DONE); }

    /**
     * Test whether the node operation is enabled.
     *
     * @return true if the node operation is enabled, otherwise false.
     */
    bool	is_enabled() const { return _is_enabled; }

    /**
     * Enable/disable node operation.
     *
     * Note that for the time being it affects only whether the routes
     * are installed into RIB. In the future it may affect the interaction
     * with other modules as well.
     *
     * @param enable if true then enable node operation, otherwise disable it.
     */
    void	set_enabled(bool enable);
    
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
    int add_route4(bool unicast, bool multicast,
		   const IPv4Net& network, const IPv4& nexthop,
		   const string& ifname, const string& vifname,
		   uint32_t metric, bool is_backup_route, string& error_msg);

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
    int add_route6(bool unicast, bool multicast,
		   const IPv6Net& network, const IPv6& nexthop,
		   const string& ifname, const string& vifname,
		   uint32_t metric, bool is_backup_route, string& error_msg);

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
    int replace_route4(bool unicast, bool multicast,
		       const IPv4Net& network, const IPv4& nexthop,
		       const string& ifname, const string& vifname,
		       uint32_t metric, bool is_backup_route,
		       string& error_msg);

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
    int replace_route6(bool unicast, bool multicast,
		       const IPv6Net& network, const IPv6& nexthop,
		       const string& ifname, const string& vifname,
		       uint32_t metric, bool is_backup_route,
		       string& error_msg);

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
    int delete_route4(bool unicast, bool multicast, const IPv4Net& network,
		      const IPv4& nexthop, const string& ifname,
		      const string& vifname, bool is_backup_route,
		      string& error_msg);

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
    int delete_route6(bool unicast, bool multicast, const IPv6Net& network,
		      const IPv6& nexthop, const string& ifname,
		      const string& vifname, bool is_backup_route,
		      string& error_msg);

    /**
     * Find a route from the routing table.
     *
     * @param table the routing table to seach.
     * @param key_route the route information to search for.
     * @return a table iterator to the route. If the route is not found,
     * the iterator will point to the end of the table.
     */
    StaticRoutesNode::Table::iterator find_route(
	StaticRoutesNode::Table& table,
	const StaticRoute& key_route);

    /**
     * Find the best accepted route from the routing table.
     *
     * @param table the routing table to seach.
     * @param key_route the route information to search for.
     * @return a table iterator to the route. If the route is not found,
     * the iterator will point to the end of the table.
     */
    StaticRoutesNode::Table::iterator find_best_accepted_route(
	StaticRoutesNode::Table& table,
	const StaticRoute& key_route);

    //
    // Debug-related methods
    //
    
    /**
     * Test if trace log is enabled.
     * 
     * This method is used to test whether to output trace log debug messges.
     * 
     * @return true if trace log is enabled, otherwise false.
     */
    bool	is_log_trace() const { return (_is_log_trace); }
    
    /**
     * Enable/disable trace log.
     * 
     * This method is used to enable/disable trace log debug messages output.
     * 
     * @param is_enabled if true, trace log is enabled, otherwise is disabled.
     */
    void	set_log_trace(bool is_enabled) { _is_log_trace = is_enabled; }

    /**
     * Configure a policy filter.
     *
     * Will throw an exception on error.
     *
     * Export filter is not supported by static routes.
     *
     * @param filter identifier of filter to configure.
     * @param conf configuration of the filter.
     */
    void configure_filter(const uint32_t& filter, const string& conf);

    /**
     * Reset a policy filter.
     *
     * @param filter identifier of filter to reset.
     */
    void reset_filter(const uint32_t& filter);

    /**
     * Push all the routes through the policy filters for re-filtering.
     */
    void push_routes();

    /**
     * Push or pull all the routes to/from the RIB.
     *
     * @param is_push if true, then push the routes, otherwise pull them
     */
    void push_pull_rib_routes(bool is_push);


protected:
    //
    // IfMgrHintObserver methods
    //
    void tree_complete();
    void updates_made();

    void incr_startup_requests_n();
    void decr_startup_requests_n();
    void incr_shutdown_requests_n();
    void decr_shutdown_requests_n();
    void update_status();

private:
    /**
     * A method invoked when the status of a service changes.
     * 
     * @param service the service whose status has changed.
     * @param old_status the old status.
     * @param new_status the new status.
     */
    void status_change(ServiceBase*  service,
		       ServiceStatus old_status,
		       ServiceStatus new_status);

    /**
     * Get a reference to the service base of the interface manager.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     *
     * @return a reference to the service base of the interface manager.
     */
    virtual const ServiceBase* ifmgr_mirror_service_base() const = 0;

    /**
     * Get a reference to the interface manager tree.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     *
     * @return a reference to the interface manager tree.
     */
    virtual const IfMgrIfTree&	ifmgr_iftree() const = 0;

    /**
     * Initiate registration with the FEA.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     */
    virtual void fea_register_startup() = 0;

    /**
     * Initiate de-registration with the FEA.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     */
    virtual void fea_register_shutdown() = 0;

    /**
     * Initiate registration with the RIB.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     */
    virtual void rib_register_startup() = 0;

    /**
     * Initiate de-registration with the RIB.
     * 
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     */
    virtual void rib_register_shutdown() = 0;

    /**
     * Add a static IPvX route.
     *
     * @param static_route the route to add.
     * @see StaticRoute
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_route(const StaticRoute& static_route, string& error_msg);

    /**
     * Replace a static IPvX route.
     *
     * @param static_route the replacement route.
     * @see StaticRoute
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int replace_route(const StaticRoute& static_route, string& error_msg);

    /**
     * Delete a static IPvX route.
     *
     * @param static_route the route to delete.
     * @see StaticRoute
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_route(const StaticRoute& static_route, string& error_msg);

    /**
     * Prepare a copy of a route for transmission to the RIB.
     *
     * Note that the original route will be modified as appropriate.
     *
     * @param orig_route the original route to prepare.
     * @param copy_route the copy of the original route prepared for
     * transmission to the RIB.
     */
    void prepare_route_for_transmission(StaticRoute& orig_route,
					StaticRoute& copy_route);

    /**
     * Inform the RIB about a route change.
     *
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     *
     * @param static_route the route with the information about the change.
     */
    virtual void inform_rib_route_change(const StaticRoute& static_route) = 0;

    /**
     * Cancel a pending request to inform the RIB about a route change.
     *
     * This is a pure virtual function, and it must be implemented
     * by the communication-wrapper class that inherits this base class.
     *
     * @param static_route the route with the request that would be canceled.
     */
    virtual void cancel_rib_route_change(const StaticRoute& static_route) = 0;

    /**
     * Update a route received from the user configuration.
     *
     * Currently, this method is a no-op.
     *
     * @param iftree the tree with the interface state to update the route.
     * @param route the route to update.
     * @return true if the route was updated, otherwise false.
     */
    bool update_route(const IfMgrIfTree& iftree, StaticRoute& route);

    /**
     * Do policy filtering on a route.
     *
     * @param route route to filter.
     * @return true if route was accepted by policy filter, otherwise false.
     */
    bool do_filtering(StaticRoute& route);

    /**
     * Test whether a route is accepted based on its next-hop information.
     *
     * @param route the route to test.
     * @return true if the route is accepted based on its next-hop
     * information, otherwise false.
     */
    bool is_accepted_by_nexthop(const StaticRoute& route) const;

    /**
     * Inform the RIB about a route.
     *
     * @param r route which should be updated in the RIB.
     */
    void inform_rib(const StaticRoute& r);

    /**
     * Set the node status.
     *
     * @param v the new node status.
     */
    void set_node_status(ProcessStatus v) { _node_status = v; }

    EventLoop&		_eventloop;		// The event loop
    ProcessStatus	_node_status;		// The node/process status
    const string	_protocol_name;		// The protocol name
    bool		_is_enabled;		// Flag whether node is enabled

    //
    // The routes are stored in a multimap, because we allow more than one
    // route for the same subnet destination. We need this to support
    // floating static routes: routes to same destination but different
    // next-hop router and metric.
    //
    StaticRoutesNode::Table	_static_routes;		// The routes

    // The winning routes
    map<IPvXNet, StaticRoute>	_winning_routes_unicast;
    map<IPvXNet, StaticRoute>	_winning_routes_multicast;

    //
    // Status-related state
    //
    size_t		_startup_requests_n;
    size_t		_shutdown_requests_n;

    //
    // A local copy with the interface state information
    //
    IfMgrIfTree		_iftree;

    //
    // Debug and test-related state
    //
    bool		_is_log_trace;		// If true, enable XLOG_TRACE()

    PolicyFilters	_policy_filters;	// Only one instance of this!
};

#endif // __STATIC_ROUTES_STATIC_ROUTES_NODE_HH__
