// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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

// $XORP: xorp/contrib/olsr/route_manager.hh,v 1.3 2008/10/02 21:56:35 bms Exp $

#ifndef __OLSR_ROUTE_MANAGER_HH__
#define __OLSR_ROUTE_MANAGER_HH__

class Olsr;
class FaceManager;
class Neighborhood;
class TopologyManager;
class ExternalRoutes;

/**
 * @short An OLSR internal route entry.
 *
 * OLSRv1, unlike OSPF, does not implement areas, therefore there is no
 * need in the current design to have logical separation between the routes
 * which it processes internally and those which it exports to the RIB.
 *
 * It also has no concept of network LSAs [HNA routes are not used in
 * calculating reachability in the OLSR topology] therefore the address
 * property is not present here, nor is the 'discard' flag, nor is the
 * path type.
 *
 * TODO: Import this definition into UML.
 * TODO: Templatize to hold IPv6.
 */
class RouteEntry {
  public:
    RouteEntry()
     : _destination_type(OlsrTypes::VT_UNKNOWN),
       _direct(false),
       _nexthop(IPv4::ZERO()),
       _faceid(OlsrTypes::UNUSED_FACE_ID),
       _cost(0),
       _originator(IPv4::ZERO()),
       _main_address(IPv4::ZERO()),
       _filtered(false)
    {}

    /*
     * @return An integer which identifies the part of OLSR from which we
     * have learned this route: neighborhood, TC, HNA, MID.
     */
    inline OlsrTypes::VertexType destination_type() const {
	return _destination_type;
    }

    /**
     * Set the type of the destination in this route entry.
     * The VertexType is re-used for this.
     *
     * @param vt destination type.
     */
    inline void set_destination_type(const OlsrTypes::VertexType vt) {
	_destination_type = vt;
    }

    /**
     * @return true if the destination is directly connected.
     *
     * Strictly speaking, we should still add it to the RIB; the local
     * node MAY use ARP for L2 resolution, but we may still wish to
     * redistribute routes for these destinations, in order to see OLSR
     * protocol specific information for those destinations.
     */
    inline bool direct() const { return _direct; }

    /**
     * Set if the destination is directly connected.
     *
     * @param is_direct true if destination is directly connected.
     */
    inline void set_direct(bool is_direct) { _direct = is_direct; }

    /**
     * @return protocol address of next hop.
     */
    inline IPv4 nexthop() const { return _nexthop; }

    /**
     * Set protocol address of next hop.
     *
     * @param nexthop address of next hop.
     */
    inline void set_nexthop(const IPv4& nexthop) { _nexthop = nexthop; }

    /**
     * @return the ID of the interface used to reach the destination.
     */
    inline OlsrTypes::FaceID faceid() { return _faceid; }

    /**
     * Set the ID of the interface used to reach the destination.
     *
     * @param faceid the ID of the outward interface.
     */
    inline void set_faceid(const OlsrTypes::FaceID faceid) {
	_faceid = faceid;
    }

    /**
     * @return OLSR protocol cost of route.
     */
    inline uint32_t cost() const { return _cost; }

    /**
     * Set OLSR protocol cost of route.
     *
     * @param cost OLSR protocol cost of route.
     */
    inline void set_cost(const uint32_t cost) { _cost = cost; }

    /**
     * @return main address of node which originated this route.
     */
    inline IPv4 originator() const { return _originator; }

    /**
     * Set the originator of this route.
     *
     * @param originator main address of node originating this route.
     */
    inline void set_originator(const IPv4& originator) {
	_originator = originator;
    }

    /**
     * @return the OLSR main address of the destination.
     * May be unset.
     */
    inline IPv4 main_address() const { return _main_address; }

    /**
     * Set the main address of the destination.
     * Applicable only if if this is a route which is tagged with such
     * information (N1, N2, TC, MID).
     * May be the same as the destination.
     *
     * @param main_addr the OLSR main address of the destination.
     */
    inline void set_main_address(const IPv4& main_addr) {
	_main_address = main_addr;
    }

    /**
     * @return true if this route has been filtered by policy filters.
     */
    inline bool filtered() const { return _filtered; }

    /**
     * Set if this route has been filtered by policy filters.
     * @param was_filtered true if the route has been filtered.
     */
    inline void set_filtered(const bool was_filtered) {
	_filtered = was_filtered;
    }

    /**
     * @return a string representation of the route entry.
     */
    string str();

  private:
    OlsrTypes::VertexType   _destination_type; // MID, TC, HNA. neighbor, etc.
    bool		_direct;	// destination is directly connected

    IPv4		_nexthop;
    OlsrTypes::FaceID	_faceid;	// face used to reach nexthop

    uint32_t		_cost;		// OLSR link cost.

    IPv4		_originator;	// advertising router
    IPv4		_main_address;	// If this is an N1, N2, TC or MID
					// route, this is the main address
					// of the destination; similar to
					// OSPF router ID.

    bool		_filtered;	// true if policy filtered this route.
};

/**
 * @short Routing table manager.
 *
 * Performs incremental and/or deferred update of the OLSR routing table
 * for the domain which this routing process interacts with.
 *
 * Whilst incremental updates are possible, the code does not
 * currently do this because the incremental shortest-path-tree code
 * does not yet support incremental updates. The interface is however
 * written with this in mind as it may turn out to offer faster convergence
 * for link-state MANET protocols like OLSR.
 *
 * Classes which produce artifacts for RouteManager to turn into routes
 * therefore call schedule_route_update() whenever their state changes.
 * RouteManager will call them back. We do not use polymorphism for this
 * as the relationships do not significantly change.
 */
class RouteManager {
  public:
    RouteManager(Olsr& olsr, EventLoop& eventloop,
		 FaceManager* fm, Neighborhood* nh,
		 TopologyManager* tm, ExternalRoutes* er);
    ~RouteManager();

    /**
     * Schedule a recalculation of the entire routing table.
     */
    void schedule_route_update();

    /**
     * Schedule a recalculation of external routes.
     */
    void schedule_external_route_update();

    /**
     * Add a link to a one-hop neighbor to the SPT.
     *
     * Given a link at radius 1 in the routing graph from this node,
     * add its endpoint node and the edge to the SPT graph.
     *
     * TODO: Bias for MPRs and willingness correctly.
     *
     * @param l pointer to a link between the origin and the Neighbor n.
     * @param n pointer to the Neighbor n.
     * @return true if the link was added OK.
     */
    bool add_onehop_link(const LogicalLink* l, const Neighbor* n);

    /**
     * Add a two-hop link and neighbor to the SPT.
     *
     * Given a link at radius 2 in the routing graph from the origin,
     * add its endpoint nodes and the edge to the SPT graph.
     *
     * In the absence of ETX measurements, the only possible cost for a 2-hop
     * link which we can infer from link state is 1 (just the hop), and
     * this is what we get from Neighborhood.
     *
     * @param n pointer to the Neighbor n, which must already have been added
     *          to the SPT graph.
     * @param l2 pointer to a link between Neighbor n and TwoHopNeighbor n2.
     * @param n2 pointer to a TwoHopNeighbor.
     * @return true if the two-hop link was added OK.
     */
    bool add_twohop_link(const Neighbor* n, const TwoHopLink* l2,
			 const TwoHopNeighbor* n2);

    /**
     * Add a TC-derived link and neighbor to the SPT.
     *
     * Given a link at radius > 2 in the routing graph from the origin,
     * add its far endpoint nodes and the edge to the SPT graph.
     * In the absence of ETX measurements, the only possible cost for a TC
     * link which we can infer from link state is 1 (just the hop), and
     * this is what we get from TopologyManager.
     *
     * @param tc The topology entry being considered for SPT add.
     * @return true if a link and vertex for the far endpoint was added
     *         to the SPT, otherwise false.
     */
    bool add_tc_link(const TopologyEntry* tc);

    /**
     * Add an external route, possibly HNA derived, to the current trie.
     *
     * Section 12: Non-OLSR Interfaces.
     * The metric of an HNA route in RFC-compliant OLSR is identical to
     * that of its last-hop. If the origin of the HNA route is not reachable
     * in the OLSR SPT, the HNA route will be rejected; see NOTES.
     *
     * @param dest the destination prefix.
     * @param lasthop the last hop advertising @param dest
     *                Note: This is a main address, not an interface address.
     * @param distance the number of OLSR hops to @param lasthop
     * @return true if the route was added OK.
     */
    bool add_hna_route(const IPv4Net& dest,
		       const IPv4& lasthop,
		       const uint16_t distance);

    /**
     * Backend method to: push all the routes through the policy filters
     * whenever they are updated.
     */
    void push_routes();

  protected:
    /**
     * Recompute the OLSR domain portion of the routing table.
     *
     * Until incremental computation is implemented, we perform the SPT
     * computation fully every time a recomputation is triggered. Producers
     * of routes call us back to populate the graph.
     */
    void recompute_all_routes();

    inline Vertex make_origin_vertex() {
	Vertex v;
	v.set_is_origin(true);
	v.set_main_addr(_fm->get_main_addr());
	return v;
    }

    //
    // Route table transactions.
    //

    /**
     * Begin the route computation transaction.
     *
     * This must be called before any routes are recomputed, to take
     * snapshots of the state which is present in the RIB.
     */
    void begin();

    /**
     * End the route computation transaction.
     *
     * This must be called after all routes are recomputed in order for
     * them to propagate to the RIB.
     */
    void end();

    //
    // Route table manipulation.
    //

    /**
     * Internal method to: Add a route entry to the current
     * internal trie.
     *
     * @param net the destination prefix.
     * @param rt the entry to add.
     * @return true if the entry was added OK.
     */
    bool add_entry(const IPv4Net& net, const RouteEntry& rt);

    /**
     * Internal method to: Delete a route entry from the current
     * internal trie.
     *
     * @param net the destination prefix.
     * @param rt the entry to delete.
     * @return true if the entry was deleted OK.
     */
    bool delete_entry(const IPv4Net& net, const RouteEntry& rt);

    /**
     * Internal method to: Replace a route entry in the current
     * internal trie.
     *
     * @param net the destination prefix.
     * @param rt the new route entry which replaces @param previous_rt
     * @param previous_rt the entry to replace.
     * @return true if the entry was replaced OK.
     */
    bool replace_entry(const IPv4Net& net, const RouteEntry& rt,
		       const RouteEntry& previous_rt);

    //
    // RIB interaction.
    //

    /**
     * Backend method to: add a route to the RIB with policy filtering.
     *
     * @param net the destination prefix.
     * @param nexthop the next-hop.
     * @param metric the computed metric of the route.
     * @param rt the route entry to add, containing all other fields.
     * @return true if the route was added to the RIB OK, otherwise false.
     */
    bool add_route(IPv4Net net, IPv4 nexthop,
		   uint32_t metric, RouteEntry& rt);

    /**
     * Backend method to: withdraw a route from the RIB.
     *
     * @param net the destination prefix.
     * @param rt the route entry to delete, containing all other fields.
     * @return true if the route was withdrawn from the RIB OK,
     *         otherwise false.
     */
    bool delete_route(const IPv4Net& net, const RouteEntry& rt);

    /**
     * Backend method to: replace a route that has been sent to the RIB.
     *
     * @param net the destination prefix.
     * @param nexthop the new next-hop.
     * @param metric the new computed metric of the route.
     * @param rt the route entry to add, containing all other fields.
     * @param previous_rt the route entry to replace.
     * @return true if the route was replaced in the RIB OK, otherwise false.
     */
    bool replace_route(IPv4Net net, IPv4 nexthop,
		       uint32_t metric,
		       RouteEntry& rt,
		       RouteEntry& previous_rt);

    //
    // Policy interaction.
    //

    /**
     * Backend method to: perform policy filtering when a route may be
     * plumbed to the RIB.
     * May not be declared const; policy may modify fields.
     *
     * @param net the destination prefix.
     * @param nexthop the next-hop.
     * @param metric the metric.
     * @param rt the route entry to filter, containing all other fields.
     * @param policytags the tags presented to us by the policy engine.
     * @return true if the route was accepted by the policy engine,
     *         otherwise false if any error occurred.
     */
    bool do_filtering(IPv4Net& net, IPv4& nexthop,
		      uint32_t& metric, RouteEntry& rt,
		      PolicyTags& policytags);

  private:
    Olsr&		_olsr;
    EventLoop&		_eventloop;
    FaceManager*	_fm;
    Neighborhood*	_nh;
    TopologyManager*	_tm;
    ExternalRoutes*	_er;

    Spt<Vertex>		_spt;
    Vertex		_origin;

    bool		_in_transaction;

    XorpTask		_route_update_task;

    Trie<IPv4, RouteEntry>*		_current;
    Trie<IPv4, RouteEntry>*		_previous;
};

#endif // __OLSR_ROUTE_MANAGER_HH__
