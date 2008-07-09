// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP$

#ifndef __OLSR_EXTERNAL_HH__
#define __OLSR_EXTERNAL_HH__

class ExternalRoute;
class ExternalRoutes;
class RouteManager;

/**
 * @short Orders a sequence of OlsrTypes::ExternalID in descending
 * order of preference for route selection.
 *
 * Model of StrictWeakOrdering.
 */
struct ExternalRouteOrderPred {
    ExternalRoutes& _ers;

    inline ExternalRouteOrderPred(ExternalRoutes& ers) : _ers(ers) {}

    /**
     * Functor to: implement collation order on external route maps.
     *
     * 12.6: The distance to the last-hop, as measured from the HNA
     * message, is used as the current discriminator; this is what's
     * specified in the RFC.
     *
     * Because identifiers for these structures are normally passed around
     * rather than the pointers themselves, it is necessary to use a functor.
     *
     * Collation order:
     *  1. Network destination, as per IPvXNet.
     *  2. If not self originated, distance in ascending order.
     *
     * @param lhid the ID of the external route on the left-hand side.
     * @param rhid the ID of the external route on the right-hand side.
     * @return true if lhid comes before rhid.
     */
    bool operator()(const OlsrTypes::ExternalID lhid,
		    const OlsrTypes::ExternalID rhid);
};

/**
 * @short Class which manages distribution of external routes
 * throughout the OLSR domain.
 */
class ExternalRoutes {
  public:
    ExternalRoutes(Olsr& olsr, EventLoop& eventloop,
		   FaceManager& fm, Neighborhood& nh);
    ~ExternalRoutes();

    //
    // Accessors/mutators.
    //
    FaceManager& face_manager() { return _fm; }

    inline RouteManager* route_manager() { return _rm; }

    inline void set_route_manager(RouteManager* rm) { _rm = rm; }

    //
    // HNA protocol variables.
    //

    inline TimeVal get_hna_interval() const { return _hna_interval; }

    /**
     * Set the HNA send timer.
     *
     * @param hna_interval the interval between HNA advertisements.
     *
     * The timer will only be restarted if previously scheduled. If the
     * period of the HNA broadcasts is changed, a HNA broadcast MAY be
     * scheduled to take place immediately.
     */
    void set_hna_interval(const TimeVal& hna_interval);

    inline TimeVal get_hna_hold_time() const {
	return 3 * get_hna_interval();
    }


    //
    // HNA routes in.
    //

    /**
     * Update or create a route entry learned from HNA.
     *
     * If the origin of the HNA route is not reachable in the OLSR SPT,
     * the route will be rejected; see NOTES.
     * TODO: In future distance may also be treated as an advertised metric.
     *
     * @param dest The destination being updated.
     * @param lasthop The last-hop to reach the destination.
     * @param distance the number of hops to reach lasthop.
     * @param expiry_time The time at which the entry will expire.
     * @param is_created reference to a boolean which is set to true
     *                   iff a new entry was created by this method.
     *
     * @return The ID of the created or updated route entry.
     * @throw BadExternalRoute if the route could not be created.
     */
    OlsrTypes::ExternalID update_hna_route_in(const IPv4Net& dest,
					      const IPv4& lasthop,
					      const uint16_t distance,
					      const TimeVal& expiry_time,
					      bool& is_created)
	throw(BadExternalRoute);

    /**
     * Create a route entry learned from HNA.
     *
     * @param dest The destination being updated.
     * @param lasthop The last-hop to reach the destination;
     *                usually this is the origin of the route.
     * @param distance the number of hops to reach lasthop.
     * @param expiry_time The time at which the entry will expire.
     *
     * @return The ID of the created route entry.
     * @throw BadExternalRoute if the route could not be created.
     */
    OlsrTypes::ExternalID add_hna_route_in(const IPv4Net& dest,
					   const IPv4& lasthop,
					   const uint16_t distance,
					   const TimeVal& expiry_time)
	throw(BadExternalRoute);

    /**
     * Delete an HNA learned route entry given its ID.
     *
     * @param erid The ID of the external route to delete.
     * @return true if the entry was found and deleted.
     */
    bool delete_hna_route_in(OlsrTypes::ExternalID erid);

    /**
     * Clear the learned HNA routes.
     */
    void clear_hna_routes_in();

    /**
     * Look up an ExternalRoute learned from HNA, given its origin and
     * destination network prefix.
     *
     * @param dest the destination network prefix.
     * @param lasthop the origin.
     * @return pointer to the ExternalRoute.
     * @throw BadExternalRoute if the route could not be found.
     */
    const ExternalRoute* get_hna_route_in(const IPv4Net& dest,
					  const IPv4& lasthop)
	throw(BadExternalRoute);

    /**
     * Look up the ID of a learned HNA entry.
     * Both the destination and the OLSR last-hop must be specified to match.
     *
     * @param dest The destination address.
     * @param lasthop The last-hop advertised for the destination.
     *
     * @return the ID of the learned external route.
     * @throw BadExternalRoute if the route could not be found.
     */
    OlsrTypes::ExternalID get_hna_route_in_id(const IPv4Net& dest,
					      const IPv4& lasthop)
	throw(BadExternalRoute);

    /**
     * Look up a learned HNA route entry by its ID.
     *
     * @param erid the ID of the learned ExternalRoute.
     * @return the pointer to the learned ExternalRoute.
     * @throw BadExternalRoute if the route could not be found.
     */
    const ExternalRoute* get_hna_route_in_by_id(
	const OlsrTypes::ExternalID erid)
	throw(BadExternalRoute);

    /**
     * Calculate the number of unique OLSR nodes with HNA entries in this
     * node's HNA learned route database.
     *
     * Used only by the protocol simulator.
     *
     * As we don't currently maintain a list of origins for HNA, this is
     * a more computationally expensive invariant than for TC or MID.
     *
     * @return the number of unique origins in the HNA route-in map.
     */
    size_t hna_origin_count() const;

    /**
     * Calculate the number of unique HNA prefixes which have been learned.
     *
     * Used only by the protocol simulator.
     *
     * @return the number of unique destinations in the HNA route-in map.
     */
    size_t hna_dest_count() const;

    /**
     * @return the number of entries in the learned HNA route database.
     */
    inline size_t hna_entry_count() const {
	return _routes_in.size();
    }

    /**
     * Fill out a list of all the external learned route IDs.
     *
     * @param hnalist the list to fill out.
     */
    void get_hna_route_in_list(list<OlsrTypes::ExternalID>& hnalist);

    //
    // HNA routes out.
    //

    /**
     * Originate an HNA route.
     *
     * @param dest the network for which to originate HNA broadcasts.
     * @return true if the route was originated successfully.
     * @throw BadExternalRoute if the route could not be originated.
     */
    bool originate_hna_route_out(const IPv4Net& dest)
	throw(BadExternalRoute);

    /**
     * Withdraw an HNA route.
     *
     * @param dest the network to withdraw from HNA broadcasts.
     * @throw BadExternalRoute if no route to the given destination
     *                         could be found.
     */
    void withdraw_hna_route_out(const IPv4Net& dest)
	throw(BadExternalRoute);

    /**
     * Clear the advertised HNA routes.
     */
    void clear_hna_routes_out();

    /**
     * Look up the ID of an originated HNA entry.
     * Both the destination and the OLSR last-hop must be specified to match.
     *
     * @param dest The destination address.
     * @return the ID of the originated external route.
     * @throw BadExternalRoute if the route could not be found.
     */
    OlsrTypes::ExternalID get_hna_route_out_id(const IPv4Net& dest)
	throw(BadExternalRoute);

    //
    // RouteManager interaction.
    //

    /**
     * Push candidate HNA routes to the RouteManager.
     *
     * 12.6: The distance to the last-hop, as measured from the HNA
     * message, is used as the current discriminator; this is what's
     * specified in the RFC.
     *
     * TODO: Deal with the metric here rather than in the RouteManager.
     * For now, RouteManager will invent a metric before plumbing to the RIB.
     */
    void push_external_routes();

    //
    // HNA transmission timer.
    //

    void start_hna_send_timer();
    void stop_hna_send_timer();
    void restart_hna_send_timer();

    /**
     * Reschedule the HNA send timer (if the HNA interval has changed).
     */
    void reschedule_hna_send_timer();

    /**
     * Schedule the HNA send timer to fire as soon as possible.
     */
    void reschedule_immediate_hna_send_timer();

    //
    // Event handlers.
    //

    /**
     * Callback method to: service the HNA transmission timer.
     * Section 12: Non-OLSR Interfaces.
     *
     * Flood a HNA message to the rest of the OLSR domain
     * which contains this node's Host and Network Associations.
     *
     * @return true if the callback should be rescheduled, otherwise false.
     */
    bool event_send_hna();

    /**
     * Process incoming HNA message.
     * Section 12.5: HNA Message Processing.
     *
     * @param msg the message to process.
     * @param remote_addr the source address of the Packet containing msg.
     * @param local_addr the address of the OLSR interface where the
     *                   Packet containing msg was received.
     * @return true if msg was consumed by this method, otherwise false.
     */
    bool event_receive_hna(Message* msg,
			   const IPv4& remote_addr,
			   const IPv4& local_addr);

    /**
     * Callback method to: delete a learned route entry when it expires.
     *
     * @param erid The ID of the external route to delete.
     */
    void event_hna_route_in_expired(const OlsrTypes::ExternalID erid);

private:
    Olsr&		_olsr;
    EventLoop&		_eventloop;
    FaceManager&	_fm;
    Neighborhood&	_nh;
    RouteManager*	_rm;

    ExternalRouteOrderPred  _routes_in_order_pred;

    bool		_is_early_hna_enabled;

    OlsrTypes::ExternalID   _next_erid;

    /**
     * @short the interval at which HNA broadcasts are flooded
     * throughout the OLSR domain by this node, if routes are originated.
     */
    TimeVal		_hna_interval;
    XorpTimer		_hna_send_timer;

    /**
     * @short Map of learned routes by destination to route ID
     * covering that destination.
     */
    typedef multimap<IPv4Net, OlsrTypes::ExternalID>	ExternalDestInMap;
    ExternalDestInMap	_routes_in_by_dest;

    /**
     * @short HNA entries learned by this node.
     */
    typedef map<OlsrTypes::ExternalID, ExternalRoute*>	ExternalRouteMap;
    ExternalRouteMap	_routes_in;

    /**
     * @short Map of originated route IDs by destination.
     */
    typedef map<IPv4Net, OlsrTypes::ExternalID>	ExternalDestOutMap;
    ExternalDestOutMap	_routes_out_by_dest;

    /**
     * @short HNA entries originated by this node.
     */
    ExternalRouteMap	_routes_out;
};

/**
 * @short Class representing an external route entry,
 * either learned or originated in the OLSRv1 HNA sub-protocol.
 */
class ExternalRoute {
public:
    /**
     * Constructor for a external route learned from another OLSR peer.
     */
    explicit ExternalRoute(ExternalRoutes& parent,
			   EventLoop& ev,
			   const OlsrTypes::ExternalID erid,
			   const IPv4Net& dest,
			   const IPv4& lasthop,
			   const uint16_t distance,
			   const TimeVal& expiry_time)
     : _parent(parent),
       _eventloop(ev),
       _id(erid),
       _is_self_originated(false),
       _dest(dest),
       _lasthop(lasthop),
       _distance(distance)
    {
	update_timer(expiry_time);
    }

    /**
     * Constructor for a external route originated by this node.
     */
    explicit ExternalRoute(ExternalRoutes& parent,
			   EventLoop& ev,
			   const OlsrTypes::ExternalID erid,
			   const IPv4Net& dest)
     : _parent(parent),
       _eventloop(ev),
       _id(erid),
       _is_self_originated(true),
       _dest(dest),
       _distance(0)
    {
    }

    virtual ~ExternalRoute() {}

    /**
     * @return the ID of the external route.
     */
    inline OlsrTypes::ExternalID id() const { return _id; }

    /**
     * @return true if we are the node which originated this route.
     */
    inline bool is_self_originated() const { return _is_self_originated; }

    /**
     * @return the destination of this route.
     */
    inline IPv4Net dest() const { return _dest; }

    /**
     * @return the node in the OLSR topology which advertised the
     *         destination which this route points to.
     */
    inline IPv4 lasthop() const { return _lasthop; }

    /**
     * @return the number of hops between this node and the last hop.
     */
    inline uint16_t distance() const { return _distance; }

    /**
     * Update the distance in hops between this node and the last hop.
     */
    inline void set_distance(const uint16_t distance) {
	_distance = distance;
    }

    /**
     * @return the amount of time remaining until this HNA entry expires.
     */
    inline TimeVal time_remaining() const {
	TimeVal tv;
	_expiry_timer.time_remaining(tv);
	return tv;
    }

    /**
     * Schedule or re-schedule the expiry timer on this route entry.
     *
     * @param expiry_time The new expiry time of the route entry.
     */
    void update_timer(const TimeVal& expiry_time);

    /**
     * Callback method to: request that the parent delete this HNA
     * derived external route, as it has now expired.
     */
    void event_expired();

private:
    ExternalRoutes&		_parent;
    EventLoop&			_eventloop;
    OlsrTypes::ExternalID	_id;

    /**
     * true if this ExternalRoute is being originated by this
     * node and was not learned from another node, otherwise false.
     */
    bool			_is_self_originated;

    /**
     * The A_network_addr/A_netmask protocol variable.
     */
    IPv4Net			_dest;

    /**
     * The A_gateway_addr protocol variable.
     */
    IPv4			_lasthop;

    /**
     * The A_time protocol variable.
     */
    XorpTimer			_expiry_timer;

    /**
     * The number of hops recorded between this node and the lasthop,
     * as seen in the most recent HNA message containing this prefix.
     */
    uint16_t			_distance;
};

#endif // __OLSR_EXTERNAL_HH__
