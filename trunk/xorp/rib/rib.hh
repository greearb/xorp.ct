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

// $XORP: xorp/rib/rib.hh,v 1.17 2004/03/23 11:24:25 pavlin Exp $

#ifndef __RIB_RIB_HH__
#define __RIB_RIB_HH__

#include <string>

#include "libxorp/xorp.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"
#include "libxorp/nexthop.hh"
#include "libxorp/vif.hh"

#include "route.hh"
#include "rt_tab_base.hh"
#include "rt_tab_origin.hh"
#include "rt_tab_merged.hh"
#include "rt_tab_extint.hh"
#include "rt_tab_redist.hh"
#include "rt_tab_register.hh"
#include "rt_tab_export.hh"


class RegisterServer;
class RibClient;
class RibManager;

enum RibTransportType {
    UNICAST	= 1,
    MULTICAST	= 2
};

/**
 * @short Master class for a RIB.
 *
 * RIB is the master class for a Routing Information Base.  It holds
 * the Vif table, routing tables for each protocol, etc.  Typically we
 * would have one RIB for IPv4 unicast, one for IPv4 multicast
 * topology, one for IPv6 unicast and one for IPv6 multicast.
 *
 * Note that the XRL commands assume some level of filtering has already
 * taken place to route to command to the right RIB.
 */
template<class A>
class RIB {
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
     * Initialize the RIB's ExportTable so that the winning routes are
     * exported to the RIB clients (e.g., the FEA).
     * Note that it is an error to initialize the table twice.
     *
     * @see ExportTable
     * @param rib_clients_list a pointer to the list of RIB clients.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int initialize_export(list<RibClient* >* rib_clients_list);

    /**
     * Initialize the RIB's RegisterTable.  The RegisterTable allows
     * routing protocols such as BGP to register interest in routing
     * information that affects specfic addresses.
     * Note that it is an error to initialize the table twice.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int initialize_register(RegisterServer* regserv);

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
    int new_origin_table(const string&	tablename,
			 const string&	target_class,
			 const string&	target_instance,
			 int		admin_distance,
			 ProtocolType	protocol_type);

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
    virtual int delete_vif(const string& vifname);

    /**
     * Add an address and subnet to a existing VIF. Each VIF may have
     * multiple addresses and associated subnets.
     *
     * @param vifname the name of the VIF the address will be added to.
     * @param addr the address to be added.  This must be one of the
     * addresses of this router.
     * @param net the subnet that is connected to this VIF
     * corresponding to the address addr.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_vif_address(const string&	vifname,
				const A& 	addr,
				const IPNet<A>&	net);

    /**
     * Remove an address and the associated subnet from an existing VIF.
     * @param vifname the name of the VIF the address will be removed from.
     * @param addr the address to be removed.  This must be an address
     * previously added by add_vif_address.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_vif_address(const string& vifname, const A& addr);

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
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_route(const string&		tablename,
			  const IPNet<A>&	net,
			  const A&		nexthop_addr,
			  const string&		ifname,
			  const string&		vifname,
			  uint32_t		metric);

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
     * route.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int replace_route(const string&	tablename,
			      const IPNet<A>&	net,
			      const A&		nexthop_addr,
			      const string&	ifname,
			      const string&	vifname,
			      uint32_t		metric);

    /**
     * Verify that expected routing information is correct.  This is
     * intended for testing purposes only.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int verify_route(const A&		lookupaddr,
			     const string&	ifname,
			     const A&		nexthop_addr,
			     uint32_t		metric);

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
     * available, A::ZERO() otherwise.
     */
    virtual const A& lookup_route(const A& lookupaddr);

    /**
     * Used for debugging only
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
     * Enable Redistribution.
     * Note that it is an error if redistribution is already enabled.
     *
     * @param from_table the name of the source redistribition table.
     * @param to_table the name of the destination table to which
     * routes should be redistributed (must be an OriginTable<A>
     * name).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int redist_enable(const string& from_table,
			      const string& to_table);

    /**
     * Disable redistribution.
     *
     * @param from_table the name of the source redistribition table.
     * @param to_table the name of the destination table to which
     * routes were previously redistributed (must be an OriginTable<A>
     * name).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int redist_disable(const string& from_table,
			       const string& to_table);

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

private:
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
    int add_origin_table(const string& tablename,
			 const string& target_class,
			 const string& target_instance,
			 ProtocolType protocol_type);

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
     * Add a RedistTable behind OriginTable.  This allows routes
     * associated with the OriginTable to be redistributed in future.
     *
     * @param origin_tablename Name of OriginTable.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_redist_table(const string& origin_tablename);

    /**
     * track_back trough the RouteTables' parent pointers to find the
     * last (i.e, nearest the OriginTable) table that matches the mask
     * in @ref typemask.  If the table given by @ref rt doesn't match
     * the mask, return it anyway.  If a table has more than one
     * parent, then this is an error.
     *
     * @param rt the routing table to start with.
     * @param typemask the bitwise-or of the routing table types that
     * we may track back through.
     * @return the last matching table, or @ref rt if rt itself doesn't match.
     */
    RouteTable<A>* track_back(RouteTable<A>* rt, int typemask) const;

    /**
     * track_forward trough the RouteTables' child pointers to find
     * the last (i.e, nearest the ExportTable) table that matches the
     * mask in @ref typemask.  Unlike track_back, if @ref rt doesn't
     * match, but the next does, the track forward anyway.
     *
     * @param rt the routing table to start with.
     * @param typemask the bitwise-or of the routing table types that
     * we may track forward through.
     * @return the last matching table.
     */
    RouteTable<A>* track_forward(RouteTable<A>* rt, int typemask) const;

    /**
     * Find a routing table, given its table name
     *
     * @param tablename the name of the table to search for.
     * @return pointer to table if exists, NULL otherwise.
     */
    inline RouteTable<A>* find_table(const string& tablename);

    /**
     * Find a routing table, given its protocol name and XRL target
     * instance name.
     *
     * @param tablename the name of the protocol to search for.
     * @param target_class the name of the target class to search for.
     * @param target_instance the name of the target instance to search for.
     * @return pointer to table if exists, NULL otherwise.
     */
    inline OriginTable<A>* find_table_by_instance(const string& tablename,
						  const string&	target_class,
						  const string& target_instance);

    /**
     * Find a routing protcol, given its protocol name
     *
     * @param protocol the name of the table to search for.
     * @return pointer to table if exists, NULL otherwise.
     */
    inline Protocol* find_protocol(const string& protocol);

    /**
     * Add table to RIB, but don't do any plumbing.
     *
     * It is an error to add the same table twice or multiple tables
     * with the same name.
     *
     * @param table the table to be added.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    inline int add_table(RouteTable<A>* table);

    /**
     * Remove table from RIB, but don't do any unplumbing.
     * The table is not deleted by this.
     *
     * @param tablename the name of the table to be removed.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    inline int remove_table(const string& tablename);

    /**
     * Lookup the default admin distance associated with a routing
     * protocol name.
     *
     * @param protocol_name the canonical name of a routing protocol,
     * in lower case.  Eg "ospf", "ibgp", etc.
     * @return the admin distance.
     */
    inline int admin_distance(const string& protocol_name);

    /**
     * Find the virtual interface associated with one of this router's
     * addresses.
     *
     * @param addr the IP address to lookup.
     * @return pointer to Vif on success, NULL otherwise.
     */
    inline Vif* find_vif(const A& addr);

    /**
     * Find the IP External Nexthop class instance associated with an IP
     * address.
     *
     * @param addr the IP address of the nexthop router.
     * @return pointer to external next hop if it exists, NULL otherwise.
     */
    inline IPExternalNextHop<A>* find_external_nexthop(const A& addr);

    /**
     * Find the IP Peer Nexthop class instance associated with an IP
     * address.
     *
     * @param addr the IP address of the nexthop router.
     * @return pointer to peer next hop if it exists, NULL otherwise.
     */
    inline IPPeerNextHop<A>* find_peer_nexthop(const A& addr);

    /**
     * Find or create the IP External Nexthop class instance
     * associated with an IP address.
     *
     * @param addr the IP address of the nexthop router.
     * @return the IPExternalNextHop class instance for @ref addr
     */
    inline IPExternalNextHop<A>* find_or_create_external_nexthop(const A& addr);

    /**
     * Find or create the IP Peer Nexthop class instance
     * associated with an IP address.
     *
     * @param addr the IP address of the nexthop router.
     * @return the IPPeerNextHop class instance for @ref addr.
     */
    inline IPPeerNextHop<A>* find_or_create_peer_nexthop(const A& addr);

    /**
     * not implemented - force use of default copy constuctor to fail.
     */
    RIB(const RIB& );

    /**
     * not implemented - force use of default assignment operator to fail.
     */
    RIB& operator=(const RIB& );

    /**
     * Flush out routing table changes to other processes.
     */
    void flush();

protected:
    RibManager&		_rib_manager;
    EventLoop&		_eventloop;
    RouteTable<A>*	_final_table;
    RegisterTable<A>*	_register_table;
    bool		_multicast;
    bool		_errors_are_fatal;

    list<RouteTable<A>* >		_tables;
    map<const string, Protocol* >	_protocols;
    map<const string, OriginTable<A>* > _routing_protocol_instances;
    map<const string, Vif>		_vifs;
    map<const string, int>		_admin_distances;
    map<const A, IPExternalNextHop<A> >	_external_nexthops;
    map<const A, IPPeerNextHop<A> > 	_peer_nexthops;
};

typedef RIB<IPv4> IPv4RIB;
typedef RIB<IPv6> IPv6RIB;

#endif // __RIB_RIB_HH__
