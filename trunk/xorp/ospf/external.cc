// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/ospf/external.cc,v 1.13 2005/11/04 07:50:22 atanu Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"
#include <list>
#include <set>

#include "ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"

#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"

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
    : _ospf(ospf), _areas(areas), _originating(0)
{
}

template <typename A>
bool
External<A>::announce(OspfTypes::AreaID area, Lsa::LsaRef lsar)
{
    XLOG_ASSERT(lsar->external());
    XLOG_ASSERT(!lsar->get_self_originating());

    update_lsa(lsar);

    typename map<OspfTypes::AreaID, AreaRouter<A> *>::iterator i;
    for (i = _areas.begin(); i != _areas.end(); i++) {
 	if ((*i).first == area)
 	    continue;
	(*i).second->external_announce(lsar, false /* push */);
    }

    lsar->get_timer() = _ospf.get_eventloop().
	new_oneoff_after(TimeVal(OspfTypes::MaxAge -
				 lsar->get_header().get_ls_age(), 0),
			 callback(this, &External<A>::maxage_reached, lsar));
    
    return true;
}

template <typename A>
bool
External<A>::shove(OspfTypes::AreaID area)
{
    typename map<OspfTypes::AreaID, AreaRouter<A> *>::iterator i;
    for (i = _areas.begin(); i != _areas.end(); i++) {
 	if ((*i).first == area)
 	    continue;
	(*i).second->external_shove();
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
	area_router->external_announce((*i), true /* push */);
}

template <>
void 
External<IPv4>::set_net_nexthop(ASExternalLsa *aselsa, IPNet<IPv4> net,
				IPv4 nexthop)
{
    Lsa_header& header = aselsa->get_header();
    header.set_link_state_id(ntohl(net.masked_addr().addr()));
    aselsa->set_network_mask(ntohl(net.netmask().addr()));
    aselsa->set_forwarding_address_ipv4(nexthop);
}

template <>
void 
External<IPv6>::set_net_nexthop(ASExternalLsa *aselsa, IPNet<IPv6> net,
				IPv6 nexthop)
{
    aselsa->set_network(net);
    aselsa->set_forwarding_address_ipv6(nexthop);
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

    bool ebit = false;
    uint32_t tag = 0;

    if (!do_filtering(net, nexthop, metric, ebit, tag, policytags))
	return true;

    OspfTypes::Version version = _ospf.version();
    // Don't worry about the memory it will be freed when the LSA goes
    // out of scope.
    ASExternalLsa *aselsa = new ASExternalLsa(version);
    Lsa_header& header = aselsa->get_header();
    
    switch(version) {
    case OspfTypes::V2:
	header.set_options(_ospf.get_peer_manager().
			   compute_options(OspfTypes::NORMAL));
	break;
    case OspfTypes::V3:
	XLOG_WARNING("TBD - AS-External-LSA set field values");
	break;
    }

    set_net_nexthop(aselsa, net, nexthop);
    header.set_advertising_router(_ospf.get_router_id());
    aselsa->set_metric(metric);
    aselsa->set_e_bit(ebit);
    aselsa->set_external_route_tag(tag);
    aselsa->set_self_originating(true);
    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    aselsa->record_creation_time(now);
    aselsa->encode();
    Lsa::LsaRef lsar = aselsa;

    update_lsa(lsar);

    typename map<OspfTypes::AreaID, AreaRouter<A> *>::iterator i;
    for (i = _areas.begin(); i != _areas.end(); i++) {
	(*i).second->external_announce(lsar, false /* push */);
	(*i).second->external_shove();
    }

    start_refresh_timer(lsar);

    return true;
}

template <typename A>
bool
External<A>::do_filtering(IPNet<A>& network, A& nexthop, uint32_t& metric,
			  bool& e_bit, uint32_t& tag,
			  const PolicyTags& policytags)
{
    try {
	OspfVarRW<A> varrw(network, nexthop, metric, e_bit, tag, policytags);
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
    _originating--;
    if (0 == _originating)
	_ospf.get_peer_manager().refresh_router_lsas();

    // Construct an LSA that will match the one in the database.
    OspfTypes::Version version = _ospf.version();
    ASExternalLsa *aselsa = new ASExternalLsa(version);
    Lsa_header& header = aselsa->get_header();

    switch(version) {
    case OspfTypes::V2:
	set_net_nexthop(aselsa, net, A::ZERO());
	break;
    case OspfTypes::V3:
	XLOG_WARNING("TBD - Set link state id");
	break;
    }

    header.set_advertising_router(_ospf.get_router_id());
    
    Lsa::LsaRef searchlsar = aselsa;

    ASExternalDatabase::iterator i = find_lsa(searchlsar);
    if (i == _lsas.end()) {
	XLOG_ERROR("Lsa not found for net %s", cstring(net));
	return false;
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
