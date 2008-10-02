// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/ospf/external.hh,v 1.23 2008/07/23 05:11:08 pavlin Exp $

#ifndef __OSPF_EXTERNAL_HH__
#define __OSPF_EXTERNAL_HH__

#include <libxorp/trie.hh>

/**
 * Storage for AS-external-LSAs with efficient access.
 */
class ASExternalDatabase {
 public:
    struct compare {
	bool operator ()(const Lsa::LsaRef a, const Lsa::LsaRef b) const {
	    if (a->get_header().get_link_state_id() ==
		b->get_header().get_link_state_id())
		return a->get_header().get_advertising_router() <
		    b->get_header().get_advertising_router();
	    return a->get_header().get_link_state_id() <
		b->get_header().get_link_state_id();
	}
    };

    typedef set <Lsa::LsaRef, compare>::iterator iterator;

    iterator begin() { return _lsas.begin(); }
    iterator end() { return _lsas.end(); }
    void erase(iterator i) { _lsas.erase(i); }
    void insert(Lsa::LsaRef lsar) { _lsas.insert(lsar); }
    void clear();

    iterator find(Lsa::LsaRef lsar);

 private:
    set <Lsa::LsaRef, compare> _lsas;		// Stored AS-external-LSAs.
};

/**
 * Handle AS-external-LSAs.
 */
template <typename A>
class External {
 public:
    External(Ospf<A>& ospf, map<OspfTypes::AreaID, AreaRouter<A> *>& areas);

    /**
     * Candidate for announcing to other areas. Store this LSA for
     * future replay into other areas. Also arrange for the MaxAge
     * timer to start running.
     *
     * @param area the AS-external-LSA came from.
     *
     * @return true if this LSA should be propogated to other areas.
     */
    bool announce(OspfTypes::AreaID area, Lsa::LsaRef lsar);

    /**
     * Called to complete a series of calls to announce().
     */
    bool announce_complete(OspfTypes::AreaID area);

    /**
     * Provide this area with the stored AS-external-LSAs.
     */
    void push(AreaRouter<A> *area_router);

    /**
     * A true external route redistributed from the RIB (announce).
     */
    bool announce(IPNet<A> net, A nexthop, uint32_t metric,
		  const PolicyTags& policytags);

    /**
     * A true external route redistributed from the RIB (withdraw).
     */
    bool withdraw(const IPNet<A>& net);

    /**
     * Clear the AS-external-LSA database.
     */
    bool clear_database();

    /**
     * Suppress or remove AS-external-LSAs that are originated by
     * this router that should yield to externally generated
     * AS-external-LSAs as described in RFC 2328 Section 12.4.4. By
     * time this method is called the routing table should have been
     * updated so it should be clear if the other router is reachable.
     */
    void suppress_lsas(OspfTypes::AreaID area);

    /**
     * A route has just been added to the routing table.
     */
    void suppress_route_announce(OspfTypes::AreaID area, IPNet<A> net,
				 RouteEntry<A>& rt);

    /**
     * A route has just been withdrawn from the routing table.
     */
    void suppress_route_withdraw(OspfTypes::AreaID area, IPNet<A> net,
				 RouteEntry<A>& rt);

    /**
     * Is this an AS boundary router?
     */
    bool as_boundary_router_p() const {
	return _originating != 0;
    }
    
    /**
     * Re-run the policy filters on all routes.
     */
    void push_routes();

 private:
    Ospf<A>& _ospf;			// Reference to the controlling class.
    map<OspfTypes::AreaID, AreaRouter<A> *>& _areas;	// All the areas

    ASExternalDatabase _lsas;		// Stored AS-external-LSAs.
    uint32_t _originating;		// Number of AS-external-LSAs
					// that are currently being originated.
    uint32_t _lsid;			// OSPFv3 only next Link State ID.
    map<IPNet<IPv6>, uint32_t> _lsmap; 	// OSPFv3 only
    list<Lsa::LsaRef> _suppress_temp;	// LSAs that could possibly
					// suppress self originated LSAs
#ifdef SUPPRESS_DB
    Trie<A, Lsa::LsaRef> _suppress_db;	// Database of suppressed self
					// originated LSAs
#endif

    /**
     * Find this LSA
     */
    ASExternalDatabase::iterator find_lsa(Lsa::LsaRef lsar);

    /**
     * Add this LSA to the database if it already exists replace it
     * with this entry.
     */
    void update_lsa(Lsa::LsaRef lsar);

    /**
     * Delete this LSA from the database.
     */
    void delete_lsa(Lsa::LsaRef lsar);

    /**
     * This LSA has reached MaxAge get rid of it from the database and
     * flood it out of all areas.
     */
    void maxage_reached(Lsa::LsaRef lsar);

    /**
     * Networks with same network number but different prefix lengths
     * can generate the same link state ID. When generating a new LSA
     * if a collision occurs use: 
     * RFC 2328 Appendix E. An algorithm for assigning Link State IDs
     * to resolve the clash.
     */
    void unique_link_state_id(Lsa::LsaRef lsar);

    /**
     * Networks with same network number but different prefix lengths
     * can generate the same link state ID. When looking for an LSA
     * make sure that there the lsar that matches the net is found.
     *
     * @param lsar search for this LSA in the database. WARNING this
     * LSA may have its link state ID field modifed by the search.
     * @param net that must match the LSA. 
     */
    ASExternalDatabase::iterator unique_find_lsa(Lsa::LsaRef lsar,
						 const IPNet<A>& net);

    /**
     * Set the network, nexthop and link state ID.
     */
    void set_net_nexthop_lsid(ASExternalLsa *aselsa, IPNet<A> net, A nexthop);


    /**
     * Send this self originated LSA out.
     */
    void announce_lsa(Lsa::LsaRef lsar);

    /**
     * Pass this outbound AS-external-LSA through the policy filter.
     */
    bool do_filtering(IPNet<A>& network, A& nexthop, uint32_t& metric,
		      bool& e_bit, uint32_t& tag, bool& tag_set,
		      const PolicyTags& policytags);

    /**
     * Start the refresh timer.
     */
    void start_refresh_timer(Lsa::LsaRef lsar);

    /**
     * Called every LSRefreshTime seconds to refresh this LSA.
     */
    void refresh(Lsa::LsaRef lsar);

    /**
     * Clone a self orignated LSA that is about to be removed for
     * possible later introduction.
     */
    Lsa::LsaRef clone_lsa(Lsa::LsaRef lsar);

    /**
     * Is this a self originated AS-external-LSA a candidate for suppression?
     */
    bool suppress_candidate(Lsa::LsaRef lsar, IPNet<A> net, A nexthop,
			    uint32_t metric);
      
#ifdef	SUPPRESS_DB
    /**
     * Store a self originated AS-external-LSA that has been
     * suppressed for possible future revival.
     */
    void suppress_database_add(Lsa::LsaRef lsar, const IPNet<A>& net);
    
    /**
     * Delete a self originated AS-external-LSA that is no longer
     * being redistributed.
     */
    void suppress_database_delete(const IPNet<A>& net, bool invalidate);
#endif

    /**
     * Should this AS-external-LSA cause a self originated LSA to be
     * suppressed.
     */
    void suppress_self(Lsa::LsaRef lsar);

    /**
     * Should this AS-external-LSA cause a self originated LSA to be
     * suppressed.
     */
    bool suppress_self_check(Lsa::LsaRef lsar);

    /**
     * Find a self originated AS-external-LSA by network.
     */
    Lsa::LsaRef find_lsa_by_net(IPNet<A> net);

    /**
     * This LSA if its advertising router is reachable matches a self
     * origniated LSA that will be suppressed. Store until the routing
     * computation has completed and the routing table can be checked.
     */
    void suppress_queue_lsa(Lsa::LsaRef lsar);

    /**
     * An AS-external-LSA has reached MaxAge and is being withdrawn.
     * check to see if it was suppressing a self originated LSA.
     */
    void suppress_maxage(Lsa::LsaRef lsar);

    /**
     * If this is an AS-external-LSA that is suppressing a self
     * originated route, then release it.
     */
    void suppress_release_lsa(Lsa::LsaRef lsar);
};

#endif // __OSPF_EXTERNAL_HH__
