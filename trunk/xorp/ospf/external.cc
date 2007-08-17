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

#ident "$XORP: xorp/ospf/external.cc,v 1.31 2007/06/28 02:43:05 atanu Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include <list>
#include <set>

#include "libproto/spt.hh"

#include "ospf.hh"
#include "delay_queue.hh"
#include "vertex.hh"
#include "area_router.hh"
#include "external.hh"
#include "policy_varrw.hh"

template <typename A>
External<A>::External(Ospf<A>& ospf,
		      map<OspfTypes::AreaID, AreaRouter<A> *>& areas)
    : _ospf(ospf), _areas(areas), _originating(0), _lsid(1)
{
}

template <typename A>
bool
External<A>::announce(OspfTypes::AreaID area, Lsa::LsaRef lsar)
{
    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	XLOG_ASSERT(lsar->external());
	break;
    case OspfTypes::V3:
	XLOG_ASSERT(lsar->external() || (!lsar->known() && lsar->as_scope()));
	break;
    }
    XLOG_ASSERT(!lsar->get_self_originating());

    update_lsa(lsar);

    typename map<OspfTypes::AreaID, AreaRouter<A> *>::iterator i;
    for (i = _areas.begin(); i != _areas.end(); i++) {
 	if ((*i).first == area)
 	    continue;
	(*i).second->external_announce(lsar, false /* push */,
				       false /* redist */);
    }

    lsar->get_timer() = _ospf.get_eventloop().
	new_oneoff_after(TimeVal(OspfTypes::MaxAge -
				 lsar->get_header().get_ls_age(), 0),
			 callback(this, &External<A>::maxage_reached, lsar));
    
    return true;
}

template <typename A>
bool
External<A>::announce_complete(OspfTypes::AreaID area)
{
    typename map<OspfTypes::AreaID, AreaRouter<A> *>::iterator i;
    for (i = _areas.begin(); i != _areas.end(); i++) {
 	if ((*i).first == area)
 	    continue;
	(*i).second->external_announce_complete();
    }

    return true;
}

template <typename A>
void 
External<A>::push(AreaRouter<A> *area_router)
{
    XLOG_ASSERT(area_router);

    ASExternalDatabase::iterator i;
    for(i = _lsas.begin(); i != _lsas.end(); i++)
	area_router->
	    external_announce((*i), true /* push */,
			      (*i)->get_self_originating() /* redist */);
}

template <>
void 
External<IPv4>::unique_link_state_id(Lsa::LsaRef lsar)
{
    ASExternalDatabase::iterator i = _lsas.find(lsar);
    if (i == _lsas.end())
	return;
    
    Lsa::LsaRef lsar_in_db = *i;
    XLOG_ASSERT(lsar_in_db->get_self_originating());

    ASExternalLsa *aselsa = dynamic_cast<ASExternalLsa *>(lsar.get());
    XLOG_ASSERT(aselsa);
    ASExternalLsa *aselsa_in_db =
	dynamic_cast<ASExternalLsa *>(lsar_in_db.get());
    XLOG_ASSERT(aselsa_in_db);
    if (aselsa->get_network_mask() == aselsa_in_db->get_network_mask())
	return;

    IPv4 mask = IPv4(htonl(aselsa->get_network_mask()));
    IPv4 mask_in_db = IPv4(htonl(aselsa_in_db->get_network_mask()));
    XLOG_ASSERT(mask != mask_in_db);

    // Be very careful the AS-External-LSAs are stored in a set and
    // the comparator method uses the link state ID. If the link state
    // ID of the LSA in the database is going to be changed then it
    // must first be pulled out of the database and then re-inserted.

    // The simple case the new LSA is more specific so its host bits
    // can be set and we are done.
    if (mask_in_db.mask_len() < mask.mask_len()) {
	 Lsa_header& header = lsar->get_header();
	 header.set_link_state_id(set_host_bits(header.get_link_state_id(),
						ntohl(mask.addr())));
	 lsar->encode();
	 return;
    } 

    // The harder case, the LSA already in the database needs to be
    // changed. First pull it out of the database.
    delete_lsa(lsar_in_db);
    Lsa_header& header = lsar_in_db->get_header();
    header.set_link_state_id(set_host_bits(header.get_link_state_id(),
					   ntohl(mask_in_db.addr())));
    lsar_in_db->encode();
    update_lsa(lsar_in_db);
    refresh(lsar_in_db);
}

template <>
void 
External<IPv6>::unique_link_state_id(Lsa::LsaRef /*lsar*/)
{
}

template <>
ASExternalDatabase::iterator
External<IPv4>::unique_find_lsa(Lsa::LsaRef lsar, const IPNet<IPv4>& net)
{
    ASExternalDatabase::iterator i = find_lsa(lsar);
    if (i == _lsas.end())
	return i;

    Lsa::LsaRef lsar_in_db = *i;
    XLOG_ASSERT(lsar_in_db->get_self_originating());
    ASExternalLsa *aselsa_in_db =
	dynamic_cast<ASExternalLsa *>(lsar_in_db.get());
    XLOG_ASSERT(aselsa_in_db);
    IPv4 mask_in_db = IPv4(htonl(aselsa_in_db->get_network_mask()));
    // If the mask/prefix lengths match then the LSA has been found.
    if (mask_in_db.mask_len() == net.prefix_len())
	return i;

    // The incoming LSA is about to be modified.
    Lsa_header& header = lsar->get_header();
    // Set the host bits and try to find it again.
    header.set_link_state_id(set_host_bits(header.get_link_state_id(),
					   ntohl(net.netmask().addr())));

    // Recursive
    return unique_find_lsa(lsar, net);
}

template <>
ASExternalDatabase::iterator
External<IPv6>::unique_find_lsa(Lsa::LsaRef lsar, const IPNet<IPv6>& /*net*/)
{
    return find_lsa(lsar);
}

template <>
void 
External<IPv4>::set_net_nexthop_lsid(ASExternalLsa *aselsa, IPNet<IPv4> net,
				     IPv4 nexthop)
{
    aselsa->set_network_mask(ntohl(net.netmask().addr()));
    aselsa->set_forwarding_address_ipv4(nexthop);

    Lsa_header& header = aselsa->get_header();
    header.set_link_state_id(ntohl(net.masked_addr().addr()));
}

template <>
void 
External<IPv6>::set_net_nexthop_lsid(ASExternalLsa *aselsa, IPNet<IPv6> net,
				     IPv6 nexthop)
{
    IPv6Prefix prefix(_ospf.get_version());
    prefix.set_network(net);
    aselsa->set_ipv6prefix(prefix);
    if (!nexthop.is_linklocal_unicast() && !nexthop.is_zero()) {
	aselsa->set_f_bit(true);
	aselsa->set_forwarding_address_ipv6(nexthop);
    }

    // Note entries in the _lsmap are never removed, this guarantees
    // for the life of OSPF that the same network to link state ID
    // mapping exists. If this is a problem on a withdraw remove the
    // entry, will need to add another argument. Note that it is
    // possible to receive a withdraw from the policy manager with no
    // preceding announce.
    uint32_t lsid = 0;
    if (0 == _lsmap.count(net)) {
	lsid = _lsid++;
	_lsmap[net] = lsid;
    } else {
	lsid = _lsmap[net];
    }
    Lsa_header& header = aselsa->get_header();
    header.set_link_state_id(lsid);
}

template <typename A>
bool
External<A>::announce(IPNet<A> net, A nexthop, uint32_t metric,
		      const PolicyTags& policytags)
{
    debug_msg("net %s nexthop %s metric %u\n", cstring(net), cstring(nexthop),
	      metric);

    _originating++;
    if (1 == _originating)
	_ospf.get_peer_manager().refresh_router_lsas();

    bool ebit = true;
    uint32_t tag = 0;
    bool tag_set = false;

    /**
     * If the nexthop address is not configured for OSPF then it won't
     * be reachable, so set the nexthop to zero.
     */
    if (!_ospf.get_peer_manager().configured_network(nexthop))
	nexthop = A::ZERO();

    if (!do_filtering(net, nexthop, metric, ebit, tag, tag_set, policytags))
	return true;

    OspfTypes::Version version = _ospf.version();
    // Don't worry about the memory it will be freed when the LSA goes
    // out of scope.
    ASExternalLsa *aselsa = new ASExternalLsa(version);
    Lsa::LsaRef lsar(aselsa);
    
    Lsa_header& header = aselsa->get_header();
    
    switch(version) {
    case OspfTypes::V2:
	header.set_options(_ospf.get_peer_manager().
			   compute_options(OspfTypes::NORMAL));
	aselsa->set_external_route_tag(tag);
	break;
    case OspfTypes::V3:
	if (tag_set) {
	    aselsa->set_t_bit(true);
	    aselsa->set_external_route_tag(tag);
	}
	break;
    }

    set_net_nexthop_lsid(aselsa, net, nexthop);
    header.set_advertising_router(_ospf.get_router_id());
    aselsa->set_metric(metric);
    aselsa->set_e_bit(ebit);
    aselsa->set_self_originating(true);
    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    aselsa->record_creation_time(now);
    aselsa->encode();

    unique_link_state_id(lsar);

    update_lsa(lsar);

    typename map<OspfTypes::AreaID, AreaRouter<A> *>::iterator i;
    for (i = _areas.begin(); i != _areas.end(); i++) {
	(*i).second->external_announce(lsar, false /* push */,
				       true /* redist */);
	(*i).second->external_announce_complete();
    }

    start_refresh_timer(lsar);

    return true;
}

template <typename A>
void
External<A>::push_routes()
{
//     XLOG_WARNING("TBD - policy route pushing");
}

template <typename A>
bool
External<A>::do_filtering(IPNet<A>& network, A& nexthop, uint32_t& metric,
			  bool& e_bit, uint32_t& tag, bool& tag_set,
			  const PolicyTags& policytags)
{
    try {
	PolicyTags ptags = policytags;
	OspfVarRW<A> varrw(network, nexthop, metric, e_bit, tag,tag_set,ptags);
	XLOG_TRACE(_ospf.trace()._export_policy,
		   "[OSPF] Running filter: %s on route: %s\n",
		   filter::filter2str(filter::EXPORT).c_str(),
		   cstring(network));
	bool accepted = _ospf.get_policy_filters().
	    run_filter(filter::EXPORT, varrw);
	
	if (!accepted)
	    return accepted;

	// XXX - Do I need to do any matching here.

    } catch(const PolicyException& e) {
	XLOG_WARNING("PolicyException: %s", e.str().c_str());
	return false;
    }

    return true;
}

template <typename A>
void
External<A>::start_refresh_timer(Lsa::LsaRef lsar)
{
    lsar->get_timer() = _ospf.get_eventloop().
	new_oneoff_after(TimeVal(OspfTypes::LSRefreshTime, 0),
			 callback(this, &External<A>::refresh, lsar));
}

template <typename A>
void
External<A>::refresh(Lsa::LsaRef lsar)
{
    XLOG_ASSERT(lsar->valid());

    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    lsar->update_age_and_seqno(now);

    typename map<OspfTypes::AreaID, AreaRouter<A> *>::iterator i;
    for (i = _areas.begin(); i != _areas.end(); i++) {
	(*i).second->external_refresh(lsar);
    }

    start_refresh_timer(lsar);
}

template <typename A>
bool
External<A>::withdraw(const IPNet<A>& net)
{
    debug_msg("net %s\n", cstring(net));

    _originating--;
    if (0 == _originating)
	_ospf.get_peer_manager().refresh_router_lsas();

    // Construct an LSA that will match the one in the database.
    OspfTypes::Version version = _ospf.version();
    ASExternalLsa *aselsa = new ASExternalLsa(version);
    Lsa_header& header = aselsa->get_header();

    set_net_nexthop_lsid(aselsa, net, A::ZERO());
    header.set_advertising_router(_ospf.get_router_id());
    
    Lsa::LsaRef searchlsar = aselsa;

    ASExternalDatabase::iterator i = unique_find_lsa(searchlsar, net);
    if (i == _lsas.end()) {
	// The LSA may not have been found because it has been filtered.
 	debug_msg("Lsa not found for net %s", cstring(net));
	return true;
// 	XLOG_ERROR("Lsa not found for net %s", cstring(net));
// 	return false;
    }

    Lsa::LsaRef lsar = *i;

    if (!lsar->get_self_originating()) {
	XLOG_FATAL("Matching LSA is not self originated %s", cstring(*lsar));
	return false;
    }

    lsar->set_maxage();
    maxage_reached(lsar);

    return true;
}

template <typename A>
ASExternalDatabase::iterator
External<A>::find_lsa(Lsa::LsaRef lsar)
{
    return _lsas.find(lsar);
}

template <typename A>
void
External<A>::update_lsa(Lsa::LsaRef lsar)
{
    ASExternalDatabase::iterator i = _lsas.find(lsar);
    if (i != _lsas.end()) {
	(*i)->invalidate();
	_lsas.erase(i);
    }
    _lsas.insert(lsar);
}

template <typename A>
void
External<A>::delete_lsa(Lsa::LsaRef lsar)
{
    ASExternalDatabase::iterator i = find_lsa(lsar);
    XLOG_ASSERT(i != _lsas.end());
    _lsas.erase(i);
}

template <typename A>
void
External<A>::maxage_reached(Lsa::LsaRef lsar)
{
    XLOG_ASSERT(lsar->external());
//     XLOG_ASSERT(!lsar->get_self_originating());

    ASExternalDatabase::iterator i = find_lsa(lsar);
    if (i == _lsas.end())
	XLOG_FATAL("LSA not in database: %s", cstring(*lsar));

    if (!lsar->maxage()) {
	TimeVal now;
	_ospf.get_eventloop().current_time(now);
	lsar->update_age(now);
    }

    if (!lsar->maxage())
	XLOG_FATAL("LSA is not MaxAge %s", cstring(*lsar));
    
    delete_lsa(lsar);

    typename map<OspfTypes::AreaID, AreaRouter<A> *>::iterator ia;
    for (ia = _areas.begin(); ia != _areas.end(); ia++) {
	(*ia).second->external_withdraw(lsar);
    }

    // Clear the timer otherwise there is a circular dependency.
    // The LSA contains a XorpTimer that points back to the LSA.
    lsar->get_timer().clear();
}

ASExternalDatabase::iterator
ASExternalDatabase::find(Lsa::LsaRef lsar)
{
    return _lsas.find(lsar);
#if	0
    Lsa_header& header = lsar->get_header();
    uint32_t link_state_id = header.get_link_state_id();
    uint32_t advertising_router = header.get_advertising_router();
    list <Lsa::LsaRef>::iterator i;
    for(i = _lsas.begin(); i != _lsas.end(); i++) {
	if ((*i)->get_header().get_link_state_id() == link_state_id &&
	    (*i)->get_header().get_advertising_router() == advertising_router){
	    return i;
	}
    }	

    return _lsas.end();
#endif
}

template class External<IPv4>;
template class External<IPv6>;
