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

#ident "$XORP: xorp/ospf/external.cc,v 1.40 2008/10/02 21:57:47 bms Exp $"

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

    suppress_self(lsar);

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

    // Be very careful the AS-external-LSAs are stored in a set and
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
    aselsa->set_network(net);
    aselsa->set_forwarding_address(nexthop);
}

template <>
void 
External<IPv6>::set_net_nexthop_lsid(ASExternalLsa *aselsa, IPNet<IPv6> net,
				     IPv6 nexthop)
{
    aselsa->set_network(net);
    if (!nexthop.is_linklocal_unicast() && !nexthop.is_zero()) {
	aselsa->set_f_bit(true);
	aselsa->set_forwarding_address(nexthop);
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

    if (suppress_candidate(lsar, net, nexthop, metric))
	return true;

    announce_lsa(lsar);

    return true;
}

template <typename A>
void
External<A>::announce_lsa(Lsa::LsaRef lsar)
{
    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    lsar->record_creation_time(now);
    lsar->encode();

    unique_link_state_id(lsar);

    update_lsa(lsar);

    typename map<OspfTypes::AreaID, AreaRouter<A> *>::iterator i;
    for (i = _areas.begin(); i != _areas.end(); i++) {
	(*i).second->external_announce(lsar, false /* push */,
				       true /* redist */);
	(*i).second->external_announce_complete();
    }

    start_refresh_timer(lsar);
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

#ifdef	SUPPRESS_DB
    suppress_database_delete(net, true /* invalidate */);
#endif

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
bool
External<A>::clear_database()
{
    _lsas.clear();
#ifdef	SUPPRESS_DB
    _suppress_db.delete_all_nodes();
#endif
    return true;
}

template <typename A>
void
External<A>::suppress_lsas(OspfTypes::AreaID area)
{
    RoutingTable<A> &rt = _ospf.get_routing_table();
    RouteEntry<A> rte;
    list<Lsa::LsaRef>::iterator i;
    for (i = _suppress_temp.begin(); i != _suppress_temp.end(); i++) {
	ASExternalLsa *aselsa = dynamic_cast<ASExternalLsa *>((*i).get());
	XLOG_ASSERT(aselsa);
	Lsa::LsaRef olsar = aselsa->get_suppressed_lsa();
	aselsa->release_suppressed_lsa();
	if (!rt.lookup_entry_by_advertising_router(area,
						   aselsa->get_header().
						   get_advertising_router(),
						   rte))
	    continue;
	Lsa::LsaRef nlsar = clone_lsa(olsar);
#ifdef	SUPPRESS_DB
	suppress_database_add(nlsar, aselsa->get_network(A::ZERO()));
#endif
	aselsa->set_suppressed_lsa(nlsar);
	olsar->set_maxage();
	maxage_reached(olsar);
    }
    _suppress_temp.clear();
}

template <typename A>
void 
External<A>::suppress_route_announce(OspfTypes::AreaID area,
				     IPNet<A> /*net*/,
				     RouteEntry<A>& rte)
{
    switch (rte.get_destination_type()) {
    case OspfTypes::Router:
	return;
	break;
    case OspfTypes::Network:
	break;
    }

    Lsa::LsaRef lsar = rte.get_lsa();
    ASExternalLsa *aselsa = dynamic_cast<ASExternalLsa *>(lsar.get());
    if (0 == aselsa)
	return;
    // Should simplify these two methods into a single method without
    // the intermediate queueing.
    XLOG_ASSERT(_suppress_temp.empty());
    suppress_self(lsar);
    suppress_lsas(area);
}

template <typename A>
void 
External<A>::suppress_route_withdraw(OspfTypes::AreaID /*area*/,
				     IPNet<A> /*net*/,
				     RouteEntry<A>& rte)
{
    switch (rte.get_destination_type()) {
    case OspfTypes::Router:
	return;
	break;
    case OspfTypes::Network:
	break;
    }

    suppress_release_lsa(rte.get_lsa());
}

template <typename A>
Lsa::LsaRef
External<A>::clone_lsa(Lsa::LsaRef olsar)
{
    XLOG_ASSERT(olsar->get_self_originating());

    ASExternalLsa *olsa = dynamic_cast<ASExternalLsa *>(olsar.get());
    XLOG_ASSERT(olsa);

    OspfTypes::Version version = _ospf.version();
    ASExternalLsa *nlsa = new ASExternalLsa(version);
    
    switch(version) {
    case OspfTypes::V2:
	nlsa->get_header().set_options(olsa->get_header().get_options());
	nlsa->set_external_route_tag(olsa->get_external_route_tag());
	break;
    case OspfTypes::V3:
	XLOG_ASSERT(olsa->get_f_bit());
	if (olsa->get_t_bit()) {
	    nlsa->set_t_bit(true);
	    nlsa->set_external_route_tag(olsa->get_external_route_tag());
	}
	break;
    }
    
    set_net_nexthop_lsid(nlsa,
			 olsa->get_network(A::ZERO()),
			 olsa->get_forwarding_address(A::ZERO()));
    nlsa->get_header().set_advertising_router(_ospf.get_router_id());
    nlsa->set_metric(olsa->get_metric());
    nlsa->set_e_bit(olsa->get_e_bit());
    nlsa->set_self_originating(true);

    Lsa::LsaRef nlsar(nlsa);

    // Remember to call unique_link_state_id(lsar) if this LSA is ever
    // transmitted.
    
    return nlsar;
}

template <typename A>
bool 
External<A>::suppress_candidate(Lsa::LsaRef lsar, IPNet<A> net, A nexthop,
				uint32_t metric)
{
    if (A::ZERO() == nexthop)
	return false;

    RoutingTable<A> &rt = _ospf.get_routing_table();
    RouteEntry<A> rte;
    if (!rt.lookup_entry(net, rte))
	return false;

    Lsa::LsaRef rlsar = rte.get_lsa();
    
    ASExternalLsa *aselsa = dynamic_cast<ASExternalLsa *>(rlsar.get());
    if (!aselsa)
	return false;

    // Make sure the announcing router is reachable.
    if (!rt.lookup_entry_by_advertising_router(rte.get_area(),
					       aselsa->get_header().
					       get_advertising_router(),
					       rte))
	return false;
    
    OspfTypes::Version version = _ospf.version();

    switch(version) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	if (!aselsa->get_f_bit())
	    return false;
	break;
    }
    
    if (aselsa->get_forwarding_address(A::ZERO()) != nexthop)
	return false;

    if (aselsa->get_metric() != metric)
	return false;

    if (aselsa->get_header().get_advertising_router() < _ospf.get_router_id())
	return false;

    aselsa->set_suppressed_lsa(lsar);

#ifdef	SUPPRESS_DB
    suppress_database_add(lsar, net);
#endif

    return true;
}

#ifdef	SUPPRESS_DB
template <typename A>
void 
External<A>::suppress_database_add(Lsa::LsaRef lsar, const IPNet<A>& net)
{
    _suppress_db.insert(net, lsar);
}

template <typename A>
void 
External<A>::suppress_database_delete(const IPNet<A>& net, bool invalidate)
{
    typename Trie<A, Lsa::LsaRef>::iterator i = _suppress_db.lookup_node(net);
    if (_suppress_db.end() == i) {
// 	XLOG_WARNING("LSA matching %s not found", cstring(net));
	return;
    }
    if (invalidate)
	i.payload()->invalidate();
    _suppress_db.erase(i);
}
#endif

template <typename A>
void 
External<A>::suppress_self(Lsa::LsaRef lsar)
{
    ASExternalLsa *aselsa = dynamic_cast<ASExternalLsa *>(lsar.get());
    XLOG_ASSERT(aselsa);

    // This may be a refresh of previously announce AS-external-LSA.
    bool suppressed = false;
    Lsa::LsaRef nlsar;
    ASExternalDatabase::iterator i = find_lsa(lsar);
    if (_lsas.end() != i) {
	nlsar = aselsa->get_suppressed_lsa();
	if (0 != nlsar.get()) {
	    aselsa->release_suppressed_lsa();
	    if (nlsar->valid())
		suppressed = true;
	}
    }

    // If it was previously suppressed and is no longer it needs to be
    // announced.
    if (!suppress_self_check(lsar)) {
	if (suppressed) {
#ifdef	SUPPRESS_DB
	    suppress_database_delete(aselsa->get_network(A::ZERO()), false);
#endif
	    announce_lsa(nlsar);
	}
	return;
    }

    Lsa::LsaRef olsar = find_lsa_by_net(aselsa->get_network(A::ZERO()));
    XLOG_ASSERT(0 != olsar.get());

    aselsa->set_suppressed_lsa(olsar);

    // If this self orignated AS-external-LSA was already being
    // suppressed nothing more todo.
    if (suppressed)
	return;

    suppress_queue_lsa(lsar);
}

template <typename A>
bool
External<A>::suppress_self_check(Lsa::LsaRef lsar)
{
    XLOG_ASSERT(lsar->external());
    XLOG_ASSERT(!lsar->get_self_originating());

    ASExternalLsa *aselsa = dynamic_cast<ASExternalLsa *>(lsar.get());
    XLOG_ASSERT(aselsa);

    OspfTypes::Version version = _ospf.version();

    switch(version) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	if (!aselsa->get_f_bit())
	    return false;
	break;
    }

    if (aselsa->get_forwarding_address(A::ZERO()) == A::ZERO())
	return false;

    if (aselsa->get_header().get_advertising_router() < _ospf.get_router_id())
	return false;
    
    Lsa::LsaRef olsar = find_lsa_by_net(aselsa->get_network(A::ZERO()));
    if (0 == olsar.get())
	return false;

    ASExternalLsa *olsa = dynamic_cast<ASExternalLsa *>(olsar.get());
    XLOG_ASSERT(olsa);

    switch(version) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	if (!olsa->get_f_bit())
	    return false;
	break;
    }

    if (olsa->get_forwarding_address(A::ZERO()) == A::ZERO())
	return false;

    if (olsa->get_metric() != aselsa->get_metric())
	return false;

    return true;
}

template <typename A>
Lsa::LsaRef
External<A>::find_lsa_by_net(IPNet<A> net)
{ 
    ASExternalLsa *tlsa = new ASExternalLsa(_ospf.get_version());
    Lsa::LsaRef tlsar(tlsa);
    tlsa->get_header().set_advertising_router(_ospf.get_router_id());
    set_net_nexthop_lsid(tlsa, net, A::ZERO());

    Lsa::LsaRef lsar;

    ASExternalDatabase::iterator i = find_lsa(tlsar);
    if (i != _lsas.end())
	lsar = *i;
   
    return lsar;
}

template <typename A>
void 
External<A>::suppress_queue_lsa(Lsa::LsaRef lsar)
{
    _suppress_temp.push_back(lsar);
}

template <typename A>
void 
External<A>::suppress_maxage(Lsa::LsaRef lsar)
{
    XLOG_ASSERT(lsar->external());
    XLOG_ASSERT(lsar->maxage());

    if (lsar->get_self_originating())
	return;

    suppress_release_lsa(lsar);
}

template <typename A>
void 
External<A>::suppress_release_lsa(Lsa::LsaRef lsar)
{
    // If this LSA was suppressing a self originated LSA then announce
    // it now.
    ASExternalLsa *aselsa = dynamic_cast<ASExternalLsa *>(lsar.get());
    if (0 == aselsa)
	return;
    Lsa::LsaRef nlsar = aselsa->get_suppressed_lsa();
    if (0 == nlsar.get())
	return;
    aselsa->release_suppressed_lsa();
    if (!nlsar->valid())
	return;
#ifdef	SUPPRESS_DB
    suppress_database_delete(aselsa->get_network(A::ZERO()), false);
#endif
    
    announce_lsa(nlsar);
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
    
    suppress_maxage(lsar);

    delete_lsa(lsar);

    typename map<OspfTypes::AreaID, AreaRouter<A> *>::iterator ia;
    for (ia = _areas.begin(); ia != _areas.end(); ia++) {
	(*ia).second->external_withdraw(lsar);
    }

    // Clear the timer otherwise there is a circular dependency.
    // The LSA contains a XorpTimer that points back to the LSA.
    lsar->get_timer().clear();
}

void
ASExternalDatabase::clear()
{
    set <Lsa::LsaRef, compare>::iterator i;
    for(i = _lsas.begin(); i != _lsas.end(); i++)
	(*i)->invalidate();

    _lsas.clear();
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
