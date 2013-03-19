// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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


#ifndef __RIB_RIB_HH__
#define __RIB_RIB_HH__


#include "libxorp/xorp.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"
#include "libxorp/nexthop.hh"
#include "libxorp/vif.hh"

#include "route.hh"
#include "rt_tab_base.hh"
#include "rt_tab_origin.hh"
#include "rt_tab_extint.hh"
#include "rt_tab_redist.hh"
#include "rt_tab_pol_redist.hh"
#include "rt_tab_register.hh"
#include "rt_tab_pol_conn.hh"

#include "policy/backend/policytags.hh"
#include "policy/backend/policy_redist_map.hh"

class RegisterServer;
class RibManager;

template<class A>
class RibVif;

enum RibTransportType {
    UNICAST	= 1,
    MULTICAST	= 2
};

enum RibVerifyType {
    MISS	= 0,	// No route to destination
    DISCARD	= 1,	// Discard route for destination
    UNREACHABLE	= 2,	// Unreachable route for destination
    IP		= 3	// Protocol route to destination
};

/**
 * @short Master class for a RIB.
 *
 * RIB is the master class for a Routing Information Base.  It holds
 * the RibVif table, routing tables for each protocol, etc.  Typically we
 * would have one RIB for IPv4 unicast, one for IPv4 multicast
 * topology, one for IPv6 unicast and one for IPv6 multicast.
 *
 * Note that the XRL commands assume some level of filtering has already
 * taken place to route to command to the right RIB.
 */
template<class A>
class RIB :
    public NONCOPYABLE
{
public:
    /**
     * RIB Constructor.
     *
     * @param rib_type indicates whether this RIB holds UNICAST or
     * MULTICAST routing information.  In the case of multicast, this
     * is the topology information, not the forwarding information.
     * @param rib_manager the main RIB manager process holding stuff
     * that's common to all the individual RIBs.
     * @param eventloop the main event loop.
     */
    RIB(RibTransportType rib_type, RibManager& rib_manager,
	EventLoop& eventloop);

    /**
     * RIB Destructor.
     */
    virtual ~RIB();

    /**
     * Set test-mode: abort on some errors that we'd normally mask.
     */
    void set_errors_are_fatal() { _errors_are_fatal = true; }

    /**
     * Get the list with the registered protocol names.
     *
     * @return the list with the registered protocol names.
     */
    list<string> registered_protocol_names() const;

    /**
     * Initialize the RIB.
     * Note that it is an error to initialize the table twice.
     *
     * @param register_server the @ref RegisterServer to initialize
     * the Rib with.
     */
    void initialize(RegisterServer& register_server);

    /**
     * Initialize the RIB's RedistTable at the end so that the
     * winning routes are exported to the RIB clients (e.g., the FEA).
     * Note that it is an error to initialize the table twice.
     *
     * @see RedistTable
     * @param all a keyword string which can be used by RIB clients
     * to register with the RIB to receive the winning routes from
     * the RedistTable.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int initialize_redist_all(const string& all);

    /**
     * Initialize the RIB's PolicyRedistTable.  The PolicyRedistTable enables
     * route redistribution according to policy configuration.  Based on the
     * policy tags of routes passing through this table, a redistribution
     * request is sent to the relevant protocols.  If routes are being deleted,
     * protocols are informed to stop advertising the route.
     *
     */
    int initialize_policy_redist();

    /**
     * Initialize the RIB's RegisterTable.  The RegisterTable allows
     * routing protocols such as BGP to register interest in routing
     * information that affects specfic addresses.
     * Note that it is an error to initialize the table twice.
     *
     * @param register_server the @ref RegisterServer to initialize
     * the Rib with.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int initialize_register(RegisterServer& register_server);

    /**
     * Initialize the RIB's ExtIntTable.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int initialize_ext_int();

    /**
     * Add a new OriginTable.  Use is deprecated, except in test suites.
     *
     * @see OriginTable
     * @param tablename human-readable name for this table to help in
     * debugging.
     * @param target_class the XRL target class of the routing
     * protocol that will supply routes to this OriginTable.
     * @param target_instance the XRL target instance of the routing
     * protocol that will supply routes to this OriginTable.
     * @param admin_distance default administrative distance to be
     * applied to routes that enter the RIB through this OriginTable.
     * @param protocol_type the routing protocol type (@ref ProtocolType).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    template <ProtocolType  protocol_type>
    int new_origin_table(const string&	tablename,
			 const string&	target_class,
			 const string&	target_instance,
			 uint16_t	admin_distance);

    /**
     * Inform the RIB about the existence of a Virtual Interface.
     * Note that it is an error to add twice a vif with the same vifname.
     *
     * @see Vif
     * @param vifname the name of the VIF, as understood by the FEA.
     * @param vif Vif class instance giving the information about this vif.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int new_vif(const string& vifname, const Vif& vif);

    /**
     * Inform the RIB that a VIF no longer exists.
     *
     * @param vifname the name of the VIF, as previously indicated by new_vif.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_vif(const string& vifname, string& error_msg);

    /**
     * Destroy a VIF container for a VIF that no longer exists.
     *
     * @param rib_vif the VIF container that will be destroyed.
     */
    virtual void destroy_deleted_vif(RibVif<A>* rib_vif);

    /**
     * Set the vif flags of a configured vif.
     *
     * @param vifname the name of the vif.
     * @param is_pim_register true if the vif is a PIM Register interface.
     * @param is_p2p true if the vif is point-to-point interface.
     * @param is_loopback true if the vif is a loopback interface.
     * @param is_multicast true if the vif is multicast capable.
     * @param is_broadcast true if the vif is broadcast capable.
     * @param is_up true if the underlying vif is UP.
     * @param mtu the MTU of the vif.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_vif_flags(const string& vifname,
			      bool is_p2p,
			      bool is_loopback,
			      bool is_multicast,
			      bool is_broadcast,
			      bool is_up,
			      uint32_t mtu);

    /**
     * Add an address and subnet to a existing VIF. Each VIF may have
     * multiple addresses and associated subnets.
     *
     * @param vifname the name of the VIF the address will be added to.
     * @param addr the address to be added.  This must be one of the
     * addresses of this router.
     * @param subnet the subnet that is connected to this VIF
     * corresponding to the address addr.
     * @param broadcast the broadcast address to add. In case of IPv6
     * this address is ignored.
     * @param peer the peer address to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_vif_address(const string&	vifname,
				const A& 	addr,
				const IPNet<A>&	subnet,
				const A&	broadcast_addr,
				const A&	peer_addr);

    /**
     * Remove an address and the associated subnet from an existing VIF.
     * @param vifname the name of the VIF the address will be removed from.
     * @param addr the address to be removed.  This must be an address
     * previously added by add_vif_address.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_vif_address(const string& vifname, const A& addr);

    /**
     * Add a route to the "connected" OriginTable.
     *
     * @param vif the vif with the connected route.
     * @param net the subnet (address and prefix length) of the route.
     * @param nexthop_addr the nexthop address of the route to add.
     * @param peer_addr the peer address for the route to add
     * (if a point-to-point interface).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_connected_route(const RibVif<A>&	vif,
			    const IPNet<A>&	net,
			    const A&		nexthop_addr,
			    const A&		peer_addr);

    /**
     * Delete a route from the "connected" OriginTable.
     *
     * @param vif the vif with the connected route.
     * @param net the subnet (address and prefix length) of the route.
     * @param peer_addr the peer address for the route to delete
     * (if a point-to-point interface).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_connected_route(const RibVif<A>&	vif,
			       const IPNet<A>&	net,
			       const A&		peer_addr);

    /**
     * Add a route via the OriginTable called tablename.
     *
     * @param tablename the name of the OriginTable into which the
     * route should be inserted.
     * @param net the subnet (address and prefix length) of the route.
     * @param nexthop_addr the nexthop that packets destined for net should be
     * forwarded to.
     * @param ifname the name of the physical interface toward the
     * destination. If an empty string the interface will be chosen by RIB.
     * @param vifname the name of the virtual interface toward the
     * destination. If an empty string the interface will be chosen by RIB.
     * @param metric the routing protocol metric associated with this route.
     * @param policytags the policy-tags for this route.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_route(const string&		tablename,
			  const IPNet<A>&	net,
			  const A&		nexthop_addr,
			  const string&		ifname,
			  const string&		vifname,
			  uint32_t		metric,
			  const PolicyTags&	policytags);

    /**
     * Replace  an existing route via the OriginTable called tablename.
     *
     * @param tablename the name of the OriginTable in which the
     * route should be replaced.
     * @param net the subnet (address and prefix length) of the route.
     * @param nexthop_addr the new nexthop that packets destined for @ref net
     * should be forwarded to.
     * @param ifname the name of the physical interface toward the
     * destination. If an empty string the interface will be chosen by RIB.
     * @param vifname the name of the virtual interface toward the
     * destination. If an empty string the interface will be chosen by RIB.
     * @param metric the new routing protocol metric associated with this
     * @param policytags the policy-tags for this route.
     * route.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int replace_route(const string&	tablename,
			      const IPNet<A>&	net,
			      const A&		nexthop_addr,
			      const string&	ifname,
			      const string&	vifname,
			      uint32_t		metric,
			      const PolicyTags&	policytags);

    /**
     * Verify the result of a route lookup in the RIB matches the
     * parameters we expect. Intended for testing purposes only.
     *
     * @param lookupaddr the destination to be verified.
     * @param nexthop_addr the expected next hop address.
     * @param ifname the expected interface.
     * @param metric the expected routing protocol metric.
     * @param type the expected type of match.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int verify_route(const A&		lookupaddr,
			     const string&	ifname,
			     const A&		nexthop_addr,
			     uint32_t		metric,
			     RibVerifyType	matchtype);

    /**
     * Delete an existing route via the OriginTable called tablename.
     *
     * @param tablename the name of the OriginTable in which the
     * route should be deleted.
     * @param subnet the subnet (address and prefix length) of the
     * route to be deleted.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_route(const string&   tablename,
			     const IPNet<A>& subnet);

    /**
     * Lookup an address in the RIB to determine the nexthop router to
     * which packets for this address will be forwarded.
     *
     * @param lookupaddr the address to be looked up.
     * @return pointer to address of next hop for @ref lookupaddr if
     * available, otherwise A::ZERO().
     */
    virtual const A& lookup_route(const A& lookupaddr);

    /**
     * Used for debugging only,
     * Caller should free the memory
     */
    virtual RouteRange<A>* route_range_lookup(const A& lookupaddr);

    /**
     * Register interest in being notified about all changes to
     * routing information that would affect traffic destined for a
     * particular address.
     *
     * @param lookupaddr the address to register interest in.
     * @param module the XRL module name to which notifications of
     * changes should be sent.
     */
    virtual RouteRegister<A>* route_register(const A&	   lookupaddr,
					     const string& module);

    /**
     * De-register interest in being notified about all changes to
     * routing information for a particular address.
     *
     * @see RIB<A>::route_register
     *
     * @param lookupaddr the address to de-register interest in.
     * @param module the XRL module name to which notifications of
     * changes should no longer be sent.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int route_deregister(const IPNet<A>& subnet, const string& module);

    /**
     * Find a routing protocol, given its protocol name.
     *
     * @param protocol the name of the protocol to search for.
     * @return pointer to protocol if exists, NULL otherwise.
     */
    Protocol* find_protocol(const string& protocol);

    /**
     * Get route redistribution table for specified routing protocol.
     */
    RedistTable<A>* protocol_redist_table(const string& protocol);

    /**
     * Create the OriginTable for an IGP protocol and plumb it into
     * the RIB.  Typically this will be called when a new instance of
     * an IGP routing protocol such as OSPF starts up.
     *
     * @param tablename the routing protocol name.  This should be one
     * of the list of names the RIB knows about, or the incorrect
     * default administrative distance will be applied.
     * @param target_class the XRL target class of the routing
     * protocol that will supply routes to this OriginTable.
     * @param target_instance the XRL target instance of the routing
     * protocol that will supply routes to this OriginTable.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_igp_table(const string& tablename,
			      const string& target_class,
			      const string& target_instance);

    /**
     * Delete the OriginTable for an IGP protocol and unplumb it from
     * the RIB.  Typically this will be called when an instance of
     * an IGP routing protocol such as OSPF exits.
     *
     * @param tablename the routing protocol name, previously
     * registered using @ref add_igp_table.
     * @param target_class the XRL target class of the routing
     * protocol that supplied routes to this OriginTable.
     * @param target_instance the XRL target instance of the routing
     * protocol that supplied routes to this OriginTable.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_igp_table(const string& tablename,
				 const string& target_class,
				 const string& target_instance);

    /**
     * Create the OriginTable for an EGP protocol and plumb it into
     * the RIB.  Typically this will be called when a new instance of
     * an EGP routing protocol such as EBGP or IBGP starts up.  Note
     * that EBGP and IBGP should register separately.
     *
     * @param tablename the routing protocol name.  This should be one
     * of the list of names the RIB knows about, or the incorrect
     * default administrative distance will be applied.
     * @param target_class the XRL target class of the routing
     * protocol that will supply routes to this OriginTable.
     * @param target_instance the XRL target instance of the routing
     * protocol that will supply routes to this OriginTable.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_egp_table(const string& tablename,
			      const string& target_class,
			      const string& target_instance);

    /**
     * Delete the OriginTable for an EGP protocol and unplumb it from
     * the RIB.  Typically this will be called when an instance of
     * an EGP routing protocol such as BGP exits.
     *
     * @param tablename the routing protocol name, previously
     * registered using @ref add_igp_table.
     * @param target_class the XRL target class of the routing
     * protocol that supplied routes to this OriginTable.
     * @param target_instance the XRL target instance of the routing
     * protocol that supplied routes to this OriginTable.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_egp_table(const string& tablename,
				 const string& target_class,
				 const string& target_instance);

    /**
     * An XRL Target died.  We need to check if it's a routing
     * protocol, and if it was, clean up after it.
     *
     * @param target_class the XRL Class of the target that died.
     * @param target_instance the XRL Class Instance of the target that died.
     */
    void target_death(const string& target_class,
		      const string& target_instance);

    /**
     * Print the RIB structure for debugging
     */
    void print_rib() const;

    /**
     * Get RIB name.
     */
    string name() const;

    /**
     * Push routes through policy filters for re-filtering.
     */
    void push_routes();

    /**
     * Set the admin distance associated with a routing protocol.
     *
     * @param protocol_name the canonical name of a routing protocol,
     * in lower case.  Eg "ospf", "ibgp", etc.
     * @param admin_distance the admin distance to set.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_protocol_admin_distance(const string& protocol_name,
				    const uint32_t& admin_distance);

    /**
     * Get the map of all registered admin distances.
     *
     * @return a reference to the _admin_distance map.
     */
    map<string, uint32_t>& get_protocol_admin_distances() {
	return _admin_distances;
    }

    /**
     * Get the admin distance associated with a routing protocol.
     *
     * @param protocol_name the canonical name of a routing protocol,
     * in lower case.  Eg "ospf", "ibgp", etc.
     * @return the admin distance; UNKNOWN_ADMIN_DISTANCE if unknown.
     */
    uint32_t get_protocol_admin_distance(const string& protocol_name);

private:
    /**
     * Used to plumb origin table in to the RouteTable tree
     *
     * @param ot origin table that we're going to plumb in
     * @param existing_igp_table first occurrence of IGP origin table in tree
     * @param existing_egp_table first occurrence of EGP origin table in tree
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int plumb_origin_table(OriginTable<A>*& ot);

    /**
     * Used to implement @ref add_igp_table and @ref add_egp_table.
     *
     * @param tablename the routing protocol name.
     * @param target_class the XRL target class of the routing
     * protocol that will supply routes to this OriginTable.
     * @param target_instance the XRL target instance of the routing
     * protocol that will supply routes to this OriginTable.
     * @param protocol_type the routing protocol type (@ref ProtocolType).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    template <ProtocolType  protocol_type>
    int add_origin_table(const string& tablename,
			 const string& target_class,
			 const string& target_instance);

    /**
     * Used to implement @ref delete_igp_table and @ref delete_egp_table.
     *
     * @param tablename the routing protocol name.
     * @param target_class the XRL target class of the routing
     * protocol that will supply routes to this OriginTable.
     * @param target_instance the XRL target instance of the routing
     * protocol that supplied routes to this OriginTable.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_origin_table(const string& tablename,
			    const string& target_class,
			    const string& target_instance);

    /**
     * Add a table for policy filtering of connected routes.
     *
     * This is used to enable route redistribution of connected routes.
     *
     * @param origin_tablename The name of the origin table.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_policy_connected_table(RouteTable<A>* parent);

    /**
     * Add a RedistTable behind OriginTable.  This allows routes
     * associated with the OriginTable to be redistributed in future.
     *
     * @param origin_tablename Name of OriginTable.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_redist_table(RouteTable<A>* parent);

    /**
     * Find a origin routing table, given its table name
     *
     * @param tablename the name of the table to search for.
     * @return pointer to table if exists, NULL otherwise.
     */
    OriginTable<A>* find_origin_table(const string& tablename);

    template <ProtocolType protocol_type>
    OriginTable<A>* find_origin_table_smart(const string& tablename);

    /**
     * Find a IGP origin routing table, given its table name
     *
     * @param tablename the name of the table to search for.
     * @return pointer to table if exists, NULL otherwise.
     */
    OriginTable<A>* find_igp_origin_table(const string& tablename);

    /**
     * Find a EGP origin routing table, given its table name
     *
     * @param tablename the name of the table to search for.
     * @return pointer to table if exists, NULL otherwise.
     */
    OriginTable<A>* find_egp_origin_table(const string& tablename);

    /**
     * Find a redist routing table, given its table name
     *
     * @param tablename the name of the table to search for.
     * @return pointer to table if exists, NULL otherwise.
     */
    RedistTable<A>* find_redist_table(const string& tablename);

    /**
     * Find a origin routing table, given its protocol name and XRL target
     * instance name.
     *
     * @param tablename the name of the protocol to search for.
     * @param target_class the name of the target class to search for.
     * @param target_instance the name of the target instance to search for.
     * @return pointer to table if exists, NULL otherwise.
     */
    OriginTable<A>* find_table_by_instance(const string& tablename,
					   const string& target_class,
					   const string& target_instance);

    /**
     * Add origin table to RIB, but don't do any plumbing.
     *
     * It is an error to add the same table twice or multiple tables
     * with the same name.
     *
     * @param table the table to be added.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_table(OriginTable<A>* table);

    /**
     * Add redist table to RIB, but don't do any plumbing.
     *
     * It is an error to add the same table twice or multiple tables
     * with the same name.
     *
     * @param table the table to be added.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_table(RedistTable<A>* table);

    /**
     * Find the virtual interface associated with vif name
     *
     * @param vifname the vif name to search for.
     * @return pointer to RibVif on success, NULL otherwise.
     */
    RibVif<A>* find_vif(const string& vifname);

    /**
     * Create the IP External Nexthop class instance
     * associated with an IP address.
     *
     * IPRouteEntry IS RESPONSIBLE for freeing
     * allocated memory for IPExternalNextHop.
     *
     * @param addr the IP address of the nexthop router.
     * @return the IPExternalNextHop class instance for @ref addr
     */
    IPExternalNextHop<A>* create_external_nexthop(const A& addr);

    /**
     * Find or create the IP Peer Nexthop class instance
     * associated with an IP address.
     *
     * IPRouteEntry IS RESPONSIBLE for freeing
     * allocated memory for IPPeerNextHop.
     *
     * @param addr the IP address of the nexthop router.
     * @return the IPPeerNextHop class instance for @ref addr.
     */
    IPPeerNextHop<A>* create_peer_nexthop(const A& addr);

    /**
     * Flush out routing table changes to other processes.
     */
    void flush();

protected:
    typedef map<string, OriginTable<A>* > OriginTableMap;
    typedef map<string, RedistTable<A>* > RedistTableMap;

    RibManager&		_rib_manager;
    EventLoop&		_eventloop;
    RouteTable<A>*	_final_table;

    bool		_multicast;
    bool		_errors_are_fatal;



    OriginTableMap		_igp_origin_tables;
    OriginTableMap		_egp_origin_tables;
    RedistTableMap		_redist_tables;

    OriginTable<A>*		_connected_origin_table; // Helper to faster resolve if route, that we're adding, is
							 // directly connected. Shouldn't be freed!!! Instance, that
							 // is going to be freed is saved in _igp_origin_tables.

    RegisterTable<A>*		_register_table;
    PolicyRedistTable<A>*	_policy_redist_table;
    PolicyConnectedTable<A>*	_policy_connected_table;
    ExtIntTable<A>*		_ext_int_table;


    OriginTableMap			_routing_protocol_instances;
    map<string, RibVif<A>*>		_vifs;
    map<string, RibVif<A>*>		_deleted_vifs;
    map<string, uint32_t>		_admin_distances;
    map<A, IPExternalNextHop<A> >	_external_nexthops;
    map<A, IPPeerNextHop<A> >		_peer_nexthops;
};

typedef RIB<IPv4> IPv4RIB;
#ifdef HAVE_IPV6
typedef RIB<IPv6> IPv6RIB;
#endif

template<class A>
class RibVif : public Vif {
public:
    RibVif(RIB<A>* rib, const Vif& vif)
	: Vif(vif), _rib(rib),
	  _usage_counter(0),
	  _is_deleted(false) { }

    virtual ~RibVif() {}

    size_t copy_in(const Vif& from_vif) {
	Vif* to_vif = this;
	*to_vif = from_vif;
	return (sizeof(from_vif));
    }

    void set_deleted(bool v) { _is_deleted = v; }

    int32_t usage_counter() const { return (_usage_counter); }
    void incr_usage_counter() { _usage_counter++; }
    void decr_usage_counter() {
	_usage_counter--;
	assert(_usage_counter >= 0);
	if (_is_deleted && (_usage_counter == 0)) {
	    if (_rib)
		_rib->destroy_deleted_vif(this);
	}
    }

private:
    RIB<A>*	_rib;
    int	_usage_counter;
    bool	_is_deleted;
};

#endif // __RIB_RIB_HH__
