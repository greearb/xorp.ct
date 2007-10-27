// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/ospf/external.hh,v 1.19 2007/08/22 01:36:16 atanu Exp $

#ifndef __OSPF_EXTERNAL_HH__
#define __OSPF_EXTERNAL_HH__

/**
 * Storage for AS-External-LSAs with efficient access.
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
    set <Lsa::LsaRef, compare> _lsas;		// Stored AS-External-LSAs.
};

/**
 * Handle AS-External-LSAs.
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
     * @param area the AS-External-LSA came from.
     *
     * @return true if this LSA should be propogated to other areas.
     */
    bool announce(OspfTypes::AreaID area, Lsa::LsaRef lsar);

    /**
     * Called to complete a series of calls to announce().
     */
    bool announce_complete(OspfTypes::AreaID area);

    /**
     * Provide this area with the stored AS-External-LSAs.
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
     * Clear the AS-External-LSA database.
     */
    bool clear_database();

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

    ASExternalDatabase _lsas;		// Stored AS-External-LSAs.
    uint32_t _originating;		// Number of AS-External-LSAs
					// that are currently being originated.
    uint32_t _lsid;			// OSPFv3 only next Link State ID.
    map<IPNet<IPv6>, uint32_t> _lsmap; 	// OSPFv3 only

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
     * Start the refresh timer.
     */
    void start_refresh_timer(Lsa::LsaRef lsar);

    /**
     * Called every LSRefreshTime seconds to refresh this LSA.
     */
    void refresh(Lsa::LsaRef lsar);

    /**
     * Pass this outbound AS-External-LSA through the policy filter.
     */
    bool do_filtering(IPNet<A>& network, A& nexthop, uint32_t& metric,
		      bool& e_bit, uint32_t& tag, bool& tag_set,
		      const PolicyTags& policytags);
};

#endif // __OSPF_EXTERNAL_HH__
