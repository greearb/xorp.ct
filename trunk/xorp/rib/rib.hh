// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/rib/rib.hh,v 1.2 2003/02/25 04:05:20 mjh Exp $

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

class FeaClient;
class RegisterServer;

enum RibTransportType {
    UNICAST = 1,
    MULTICAST = 2
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
     */
    RIB(RibTransportType rib_type);

    /**
     * RIB Destructor.
     */
    virtual ~RIB();

    /**
     * set test-mode: don't try to send to FEA
     */
    void no_fea() {_no_fea = true;}

    /**
     * Initialize the RIB's ExportTable so that the winning routes are
     * exported to the Forwarding Engine.
     *
     * @see ExportTable
     * @return -1 if already initialized.
     */
    int initialize_export(FeaClient *fea);

    /**
     * Initialize the RIB's RegisterTable.  The RegisterTable allows
     * routing protocols such as BGP to register interest in routing
     * information that affects specfic addresses.
     *
     * @return -1 if already initialized.
     */
    int initialize_register(RegisterServer *regserv);

    /**
     * Add a new OriginTable.  Use is deprecated, except in test suites.
     *
     * @see OriginTable
     * @param tablename human-readable name for this table to help in
     * debugging
     * @param admin_distance default administrative distance to be
     * applied to routes that enter the RIB through this OriginTable.
     * @param igp true if the routing protocol that will inject routes
     * is a Interior Gateway Protocol such as OSPF.  False if it's an
     * EGP such as BGP (or IBGP).
     * @return -1 if table cannot be created, 0 otherwise.
     */
    int new_origin_table(const string&	tablename,
			 int		admin_distance,
			 int		igp);

    /**
     * Add a new MergedTable.  Use is deprecated, except in test suites.
     *
     * @see MergedTable
     * @param tablename human-readable name for this table to help in
     * debugging
     * @param table_a parent routing table that will feed routes in to
     * this MergedTable
     * @param table_b parent routing table that will feed routes in to
     * this MergedTable
     * @return 0 on success, -1 if table_a or table_b does not exist.
     */
    int new_merged_table(const string& tablename,
			 const string& table_a,
			 const string& table_b);

    /**
     * Add a new ExtIntTable.  Use is deprecated, except in test suites.
     *
     * @see ExtIntTable
     * @param tablename human-readable name for this table to help in
     * debugging
     * @param t_ext parent routing table that will feed EGP routes in to
     * this ExtIntTable
     * @param t_int parent routing table that will feed IGP routes in to
     * this ExtIntTable
     * @return 0 on success, -1 if t_ext or t_int does not exist.
     */
    int new_extint_table(const string& tablename,
			 const string& t_ext,
			 const string& t_int);

    /**
     * Inform the RIB about the existence of a Virtual Interface.
     *
     * @see Vif
     * @param vifname the name of the VIF, as understood by the FEA.
     * @param vif Vif class instance giving the information about this vif.
     * @return 0 on success, -1 if vif named vifname already exists.
     */
    virtual int new_vif(const string& vifname, const Vif& vif);

    /**
     * Inform the RIB that a VIF no longer exists.
     *
     * @param vifname the name of the VIF, as previously indicated by new_vif.
     * @return 0 on success, -1 if vif named vifname doesn't exist.
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
     * @return 0 on success, -1 if vif is unknown.
     */
    virtual int add_vif_address(const string&	vifname,
				const A& 	addr,
				const IPNet<A>&	net);

    /**
     * Remove an address and the associated subnet from an existing VIF.
     * @param vifname the name of the VIF the address will be removed from.
     * @param addr the address to be removed.  This must be an address
     * previously added by add_vif_address
     * @return 0 on success, -1 if vif is unknown or addr is not an
     * address on this vif.
     */
    virtual int delete_vif_address(const string& vifname,
				   const A& addr);

    /**
     * Add a route via the OriginTable called tablename.
     *
     * @param tablename the name of the OriginTable into which the
     * route should be inserted.
     * @param net the subnet (address and prefix length) of the route.
     * @param addr the nexthop that packets destined for net should be
     * forwarded to.
     * @param the routing protocol metric associated with this route.
     * @return 0 on success, -1 otherwise.
     */
    virtual int add_route(const string& tablename,
			  const IPNet<A>& net,
			  const A& addr,
			  uint32_t metric);

    /**
     * Replace  an existing route via the OriginTable called tablename.
     *
     * @param tablename the name of the OriginTable in which the
     * route should be replaced.
     * @param net the subnet (address and prefix length) of the route.
     * @param addr the new nexthop that packets destined for @ref net should be
     * forwarded to.
     * @param the new routing protocol metric associated with this route.
     * @return 0 on success, -1 otherwise.
     */
    virtual int replace_route(const string& tablename,
			      const IPNet<A>& net,
			      const A& addr,
			      uint32_t metric);

    /**
     * Verify that expected routing information is correct.  This is
     * intended for testing purposes only.
     *
     * @return 0 on successful
     * verification, -1 otherwise.
     */
    virtual int verify_route(const A&	   lookupaddr,
			     const string& ifname,
			     const A&	   nexthop,
			     uint32_t metric);

    /**
     * Delete an existing route via the OriginTable called tablename.
     *
     * @param tablename the name of the OriginTable in which the
     * route should be deleted.
     * @param subnet the subnet (address and prefix length) of the
     * route to be deleted.
     * @return 0 on success, -1 otherwise.
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
     * particular address
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
     * @return true on successful deregistration, false if the entry
     * to be deregistered was not found.
     */
    virtual bool route_deregister(const IPNet<A> &subnet, const string &module);

    /**
     * Enable Redistribution.
     *
     * @param fromtable the name of the source redistribition table.
     * @param totable the name of the destination table to which
     * routes should be redistributed (must be an OriginTable<A>
     * name).
     * @return 0 on success, -1 if either table does not exists or
     * redistribution is already enabled.
     */
    virtual int redist_enable(const string& fromtable, const string& totable);

    /**
     * Disable redistribution.
     *
     * @param fromtable the name of the source redistribition table.
     * @param totable the name of the destination table to which
     * routes were previously redistributed (must be an OriginTable<A>
     * name).
     * @return 0 on success, -1 if table does not exist.
     */
    virtual int redist_disable(const string& fromtable, const string& totable);

    /**
     * Create the OriginTable for an IGP protocol and plumb it into
     * the RIB.  Typically this will be called when a new instance of
     * an IGP routing protocol such as OSPF starts up.
     *
     * @param tablename the routing protocol name.  This should be one
     * of the list of names the RIB knows about, or the incorrect
     * default administrative distance will be applied.
     * @return 0 on success, -1 otherwise.
     */
    virtual int add_igp_table(const string& tablename);

    /**
     * Delete the OriginTable for an IGP protocol and unplumb it from
     * the RIB.  Typically this will be called when an instance of
     * an IGP routing protocol such as OSPF exits.
     *
     * @param tablename the routing protocol name, previously
     * registered using @ref add_igp_table .
     * @return 0 on success, -1 otherwise.
     */
    virtual int delete_igp_table(const string& tablename);

    /**
     * Create the OriginTable for an EGP protocol and plumb it into
     * the RIB.  Typically this will be called when a new instance of
     * an EGP routing protocol such as EBGP or IBGP starts up.  Note
     * that EBGP and IBGP should register separately.
     *
     * @param tablename the routing protocol name.  This should be one
     * of the list of names the RIB knows about, or the incorrect
     * default administrative distance will be applied.
     * @return 0 on success, -1 otherwise.
     */
    virtual int add_egp_table(const string& tablename);

    /**
     * Delete the OriginTable for an EGP protocol and unplumb it from
     * the RIB.  Typically this will be called when an instance of
     * an EGP routing protocol such as BGP exits.
     *
     * @param tablename the routing protocol name, previously
     * registered using @ref add_igp_table .
     * @return 0 on success, -1 otherwise.
     */
    virtual int delete_egp_table(const string& tablename);

    /**
     * Print the RIB structure for debugging
     */
    void print_rib() const;
private:
    /**
     * Used to implement @ref add_igp_table and @ref add_egp_table.
     *
     * @param tablename the routing protocol name.
     * @param type IGP == 1, EGP == 2
     */
    int add_origin_table(const string& tablename, int type);

    /**
     * Used to implement @ref delete_igp_table and @ref delete_egp_table.
     *
     * @param tablename the routing protocol name.
     */
    int delete_origin_table(const string& tablename);

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
     * @returns the last matching table, or @ref rt if rt itself doesn't match.
     */
    RouteTable<A>* track_back(RouteTable<A> *rt, int typemask) const;

    /**
     * track_forward trough the RouteTables' child pointers to find
     * the last (i.e, nearest the ExportTable) table that matches the
     * mask in @ref typemask.  Unlike track_back, if @ref rt doesn't
     * match, but the next does, the track forward anyway.
     *
     * @param rt the routing table to start with.
     * @param typemask the bitwise-or of the routing table types that
     * we may track forward through.
     * @returns the last matching table
     */
    RouteTable<A>* track_forward(RouteTable<A> *rt, int typemask) const;

    /**
     * Find a routing table, given its table name
     *
     * @param tablename the name of the table to search for.
     * @return pointer to table if exists, 0 otherwise.
     */
    inline RouteTable<A>* find_table(const string& tablename);

    /**
     * Find a routing protcol, given its protocol name
     *
     * @param protocol the name of the table to search for.
     * @return pointer to table if exists, 0 otherwise.
     */
    inline Protocol* find_protocol(const string& protocol);

    /**
     * Add table to RIB, but don't do any plumbing.  The caller should
     * first check that table does not already exist using @ref
     * find_table.
     *
     * @param tablename the name of the table to be added.
     * @param table the table to be added.
     * @return 0 on success, -1 if named table already exists.
     */
    inline int add_table(const string& tablename, RouteTable<A>* table);

    /**
     * Remove table from RIB, but don't do any unplumbing.
     * The table is not deleted by this.
     *
     * @param tablename the name of the table to be removed.
     * @return 0 on success, -1 if named table does not exist.
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
     * addresses
     *
     * @param addr the IP address to lookup
     * @return pointer to Vif on success, 0 otherwise.
     */
    inline Vif* find_vif(const A& addr);

    /**
     * Find the IP External Nexthop class instance associated with an IP
     * address.
     *
     * @param addr the IP address of the nexthop router
     * @return pointer to external next hop if it exists, 0 otherwise.
     */
    inline IPExternalNextHop<A>* find_external_nexthop(const A& addr);

    /**
     * Find the IP Peer Nexthop class instance associated with an IP
     * address.
     *
     * @param addr the IP address of the nexthop router
     * @return pointer to peer next hop if it exists, 0 otherwise.
     */
    inline IPPeerNextHop<A>* find_peer_nexthop(const A& addr);

    /**
     * Find or create the IP External Nexthop class instance
     * associated with an IP address.
     *
     * @param addr the IP address of the nexthop router
     * @returns the IPExternalNextHop class instance for @ref addr
     */
    inline IPExternalNextHop<A>*
    find_or_create_external_nexthop(const A& addr);

    /**
     * Find or create the IP Peer Nexthop class instance
     * associated with an IP address.
     *
     * @param addr the IP address of the nexthop router
     * @returns the IPPeerNextHop class instance for @ref addr
     */
    inline IPPeerNextHop<A>*
    find_or_create_peer_nexthop(const A& addr);

    /**
     * not implemented - force use of default copy constuctor to fail
     */
    RIB(const RIB& );

    /**
     * not implemented - force use of default assignment operator to fail
     */
    RIB& operator=(const RIB&);

    /**
     * Flush out routing table changes to other processes.
     */
    void flush();

protected:
    RouteTable<A>*	_final_table;
    RegisterTable<A>*	_register_table;
    bool _mcast;
    bool _no_fea;

    map<const string, RouteTable<A>*>	_tables;
    map<const string, Protocol*>        _protocols;
    map<const string, Vif>		_vifs;
    map<const string, int>		_admin_distances;
    map<const A, IPExternalNextHop<A> >	_external_nexthops;
    map<const A, IPPeerNextHop<A> > 	_peer_nexthops;
};

typedef RIB<IPv4> IPv4RIB;
typedef RIB<IPv6> IPv6RIB;

#endif // __RIB_RIB_HH__
