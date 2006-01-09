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

#ident "$XORP: xorp/ospf/area_router.cc,v 1.161 2006/01/09 10:32:39 atanu Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"
#include <map>
#include <list>
#include <set>
#include <deque>

#include "ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"

#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include "libproto/spt.hh"

#include "ospf.hh"
#include "delay_queue.hh"
#include "vertex.hh"
#include "area_router.hh"

template <typename A>
AreaRouter<A>::AreaRouter(Ospf<A>& ospf, OspfTypes::AreaID area,
			  OspfTypes::AreaType area_type) 
    : _ospf(ospf), _area(area), _area_type(area_type),
      _summaries(true), _stub_default_cost(0),
      _external_flooding(false),
      _last_entry(0), _allocated_entries(0), _readers(0),
      _queue(ospf.get_eventloop(),
	     OspfTypes::MinLSInterval,
	     callback(this, &AreaRouter<A>::publish_all)),
#ifdef	UNFINISHED_INCREMENTAL_UPDATE
      _TransitCapability(0),
#else
      _TransitCapability(false),
#endif
      _routing_recompute_delay(5),	// In seconds.
      _translator_role(OspfTypes::CANDIDATE),
      _translator_state(OspfTypes::DISABLED),
      _type7_propagate(false)	// Default from RFC 3210 Appendix A
{
    // Never need to delete this as the ref_ptr will tidy up.
    // An entry to be placed in invalid slots.
    // When a LSA that was not self originated reaches MAXAGE it must
    // be flooded but we can't mark it as invalid. In all other cases
    // just marking an entry as invalid is good enough.
    _invalid_lsa = Lsa::LsaRef(new RouterLsa(_ospf.get_version()));
    _invalid_lsa->invalidate();

    // Never need to delete this as the ref_ptr will tidy up.
    RouterLsa *rlsa = new RouterLsa(_ospf.get_version());
    rlsa->set_self_originating(true);
    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    rlsa->record_creation_time(now);

    Lsa_header& header = rlsa->get_header();
    
    // This is a router LSA so the link state ID is the Router ID.
    header.set_link_state_id(_ospf.get_router_id());
    header.set_advertising_router(_ospf.get_router_id());

    _router_lsa = Lsa::LsaRef(rlsa);
    add_lsa(_router_lsa);
//     _db.push_back(_router_lsa);
//     _last_entry = 1;

#ifdef	UNFINISHED_INCREMENTAL_UPDATE
    // Add this router to the SPT table.
    Vertex v;
    RouterVertex(v);
    _spt.add_node(v);
    _spt.set_origin(v);
#endif
}

template <typename A>
void
AreaRouter<A>::start()
{
    // Request the peer manager to send routes that are candidates
    // from summarisation. Also request an AS-External-LSAs that
    // should be announced. These requests are only relevant when
    // there are multiple areas involved although the requests are
    // made unconditionally.
    // Perform these actions last as they will invoke a callback into
    // this class.
    PeerManager<A>& pm = _ospf.get_peer_manager();
    pm.summary_push(_area);
    if (external_area_type())
	pm.external_push(_area);
}

template <typename A>
void
AreaRouter<A>::shutdown()
{
    _ospf.get_routing_table().remove_area(_area);
    shutdown_complete();
}

template <typename A>
bool
AreaRouter<A>::running()
{
    return true;
}

template <typename A>
void
AreaRouter<A>::add_peer(PeerID peerid)
{
    debug_msg("PeerID %u\n", peerid);
    // The peer starts in the down state.
    _peers[peerid] = PeerStateRef(new PeerState);
}

template <typename A>
void
AreaRouter<A>::delete_peer(PeerID peerid)
{
    debug_msg("PeerID %u\n", peerid);

    // The peer manager does not keep track of which peers belong to
    // which areas. So when a peer is deleted all areas are notified.
    if (0 == _peers.count(peerid))
	return;
    
    _peers.erase(_peers.find(peerid));
}

template <typename A>
bool
AreaRouter<A>::peer_up(PeerID peerid)
{
    debug_msg("PeerID %u\n", peerid);

    if (0 == _peers.count(peerid)) {
	XLOG_WARNING("Peer not found %u", peerid);
	return false;
    }

    // Mark the peer as UP
    typename PeerMap::iterator i = _peers.find(peerid);
    PeerStateRef psr = i->second;
    psr->_up = true;

    refresh_router_lsa();

    return true;
}

template <typename A>
bool
AreaRouter<A>::peer_down(PeerID peerid)
{
    debug_msg("PeerID %u\n", peerid);

    if (0 == _peers.count(peerid)) {
	XLOG_WARNING("Peer not found %u", peerid);
	return false;
    }

    // Mark the peer as DOWN
    typename PeerMap::iterator i = _peers.find(peerid);
    PeerStateRef psr = i->second;
    psr->_up = false;

    refresh_router_lsa();

    return true;
}

template <typename A>
bool
AreaRouter<A>::add_virtual_link(OspfTypes::RouterID rid)
{
    debug_msg("Router ID %s\n", pr_id(rid).c_str());
    XLOG_TRACE(_ospf.trace()._virtual_link,
	       "Add virtual link rid %s\n", pr_id(rid).c_str());

    switch(_area_type) {
    case OspfTypes::NORMAL:
	break;
    case OspfTypes::STUB:
    case OspfTypes::NSSA:
	XLOG_WARNING("Can't configure a virtual link through a %s area",
		     pp_area_type(_area_type).c_str());
	return false;
	break;
    }

    XLOG_ASSERT(0 == _vlinks.count(rid));
    _vlinks[rid] = false;

    routing_schedule_total_recompute();

    return true;
}

template <typename A>
bool
AreaRouter<A>::remove_virtual_link(OspfTypes::RouterID rid)
{
    debug_msg("Router ID %s\n", pr_id(rid).c_str());
    XLOG_TRACE(_ospf.trace()._virtual_link,
	       "Remove virtual link rid %s\n", pr_id(rid).c_str());

    switch(_area_type) {
    case OspfTypes::NORMAL:
	break;
    case OspfTypes::STUB:
    case OspfTypes::NSSA:
	XLOG_WARNING("Can't configure a virtual link through a %s area",
		     pp_area_type(_area_type).c_str());
	return false;
	break;
    }

    XLOG_ASSERT(0 != _vlinks.count(rid));

    _vlinks.erase(_vlinks.find(rid));

    // Note this call is async if it was sync it would cause a delete
    // link upcall to the peer_manager, possibly surprsing it.
    routing_schedule_total_recompute();

    return true;
}

template <typename A>
void
AreaRouter<A>::start_virtual_link()
{
    // Create a set of virtual links that were up. At the end of this
    // process all entries that are currently on the list should be
    // taken down as they are no longer up or they would have been
    // removed from the set.
    _tmp.clear();
    map<OspfTypes::RouterID,bool>::iterator i;
    for(i = _vlinks.begin(); i != _vlinks.end(); i++)
	if (i->second)
	    _tmp.insert(i->first);
}

template <typename A>
void
AreaRouter<A>::check_for_virtual_link(const RouteCmd<Vertex>& rc,
				      Lsa::LsaRef r)
{
    Vertex node = rc.node();
    Lsa::LsaRef lsar = node.get_lsa();
    RouterLsa *rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    OspfTypes::RouterID rid = rlsa->get_header().get_link_state_id();

    // If this router ID is in the tmp set then it is already up, just
    // remove it from the set and return;
    set<OspfTypes::RouterID>::iterator i = _tmp.find(rid);
    if (i != _tmp.end()) {
	_tmp.erase(i);
	return;
    }

    XLOG_TRACE(_ospf.trace()._virtual_link,
	       "Checking for virtual links %s\n", cstring(*rlsa));

    if (0 == _vlinks.count(rid))
	return;	// Not a candidate endpoint.

    XLOG_TRACE(_ospf.trace()._virtual_link,
	       "Found virtual link endpoint %s\n", pr_id(rid).c_str());

    // Find the interface address of the neighbour that should be used.
    A neighbour_interface_address;
    if (!find_interface_address(rc.prevhop().get_lsa(),	lsar,
				neighbour_interface_address))
	return;

    // Find this routers own interface address.
    A routers_interface_address;
    if (!find_interface_address(rc.nexthop().get_lsa(), r,
					     routers_interface_address))
	return;
    
    // Now that everything has succeeded mark the virtual link as up.
    XLOG_ASSERT(0 != _vlinks.count(rid));
    _vlinks[rid] = true;

    _ospf.get_peer_manager().up_virtual_link(rid, routers_interface_address,
					     rc.weight(),
					     neighbour_interface_address);
}

template <typename A>
void
AreaRouter<A>::end_virtual_link()
{
    set<OspfTypes::RouterID>::iterator i;
    for(i = _tmp.begin(); i != _tmp.end(); i++) {
	OspfTypes::RouterID rid = *i;
	XLOG_ASSERT(0 != _vlinks.count(rid));
	_vlinks[rid] = false;
	_ospf.get_peer_manager().down_virtual_link(rid);
    }
}

template <>
bool
AreaRouter<IPv4>::find_interface_address(Lsa::LsaRef src, Lsa::LsaRef dst,
					 IPv4& interface) const
{
    XLOG_TRACE(_ospf.trace()._find_interface_address,
	       "Find interface address \nsrc:\n%s\ndst:\n%s\n",
	       cstring(*src), cstring(*dst));

    RouterLsa *rlsa = dynamic_cast<RouterLsa *>(src.get());
    NetworkLsa *nlsa = dynamic_cast<NetworkLsa *>(src.get());

    if(0 == rlsa && 0 == nlsa) {
	XLOG_WARNING(
		     "Expecting the source to be a "
		     "Router-Lsa or a Network-LSA not %s", cstring(*src));
	return false;
    }
    
    RouterLsa *dst_rlsa = dynamic_cast<RouterLsa *>(dst.get());
    if(0 == dst_rlsa) {
	XLOG_WARNING("Expecting the source to be a Router-Lsa not %s",
		     cstring(*src));
	return false;
    }
    
    OspfTypes::RouterID srid = src->get_header().get_link_state_id();

    // Look for the corresponding link. It is not necessary to check
    // for bidirectional connectivity as this check has already been made.
    const list<RouterLink> &rlinks = dst_rlsa->get_router_links();
    list<RouterLink>::const_iterator l = rlinks.begin();
    for(; l != rlinks.end(); l++) {
	debug_msg("Does %s == %s\n",
		  pr_id(l->get_link_id()).c_str(),
		  pr_id(srid).c_str());
	if (l->get_link_id() == srid) {
	    if (rlsa) {
		if (RouterLink::p2p == l->get_type() ||
		    RouterLink::vlink == l->get_type()) {
		    interface = IPv4(htonl(l->get_link_data()));
		    return true;
		}
	    }
	    if (nlsa) {
		if (RouterLink::transit == l->get_type()) {
		    interface = IPv4(htonl(l->get_link_data()));
		    return true;
		}
	    }
	}
    }

    if (nlsa)
	return false;
    
    // There is a special case to deal with which in not part of the
    // normal adjacency check. Under normal circumstances two
    // Router-LSAs can be checked for adjacency by checking for p2p or
    // vlink. If the router link type is transit then the adjacency
    // should be Network-LSA to Router-LSA. However when introducing
    // Router-LSAs into the SPT the Router-LSA <-> Router-LSA wins
    // over Network-LSA <-> Router-LSA. If both of the LSAs are
    // Router-LSAs then check the transit links to find a common
    // router interface address.

    const list<RouterLink> &src_links = rlsa->get_router_links();
    const list<RouterLink> &dst_links = dst_rlsa->get_router_links();
    list<RouterLink>::const_iterator si = src_links.begin();
    list<RouterLink>::const_iterator di = dst_links.begin();

    for (; si != src_links.end(); si++) {
	for (; di != dst_links.end(); di++) {
	    if (si->get_type() == RouterLink::transit &&
		di->get_type() == RouterLink::transit) {
		if (si->get_link_id() == di->get_link_id()) {
		    interface = IPv4(htonl(di->get_link_data()));
		    return true;
		}
	    }
	}
    }

    return false;
}

template <>
bool
AreaRouter<IPv6>::find_interface_address(Lsa::LsaRef src, Lsa::LsaRef dst,
					 IPv6& interface) const
{
    UNUSED(src);
    UNUSED(dst);
    UNUSED(interface);

    XLOG_UNFINISHED();

    return true;
}

template <typename A>
bool
AreaRouter<A>::area_range_add(IPNet<A> net, bool advertise)
{
    debug_msg("Net %s advertise %s\n", cstring(net), pb(advertise));

    Range r;
    r._advertise = advertise;
    _area_range.insert(net, r);

    routing_schedule_total_recompute();

    return true;
}

template <typename A>
bool
AreaRouter<A>::area_range_delete(IPNet<A> net)
{
    debug_msg("Net %s\n", cstring(net));

    _area_range.erase(net);

    routing_schedule_total_recompute();

    return true;
}

template <typename A>
bool
AreaRouter<A>::area_range_change_state(IPNet<A> net, bool advertise)
{
    debug_msg("Net %s advertise %s\n", cstring(net), pb(advertise));

    typename Trie<A, Range>::iterator i = _area_range.lookup_node(net);
    if (_area_range.end() == i) {
	XLOG_WARNING("Area range %s not found", cstring(net));
	return false;
    }

    Range& r = i.payload();
    if (r._advertise == advertise)
	return true;

    r._advertise = advertise;

    routing_schedule_total_recompute();

    return true;
}

template <typename A>
bool
AreaRouter<A>::area_range_covered(IPNet<A> net, bool& advertise)
{
    debug_msg("Net %s advertise %s\n", cstring(net), pb(advertise));

    typename Trie<A, Range>::iterator i = _area_range.find(net);
    if (_area_range.end() == i)
	return false;

    advertise = i.payload()._advertise;

    return true;
}

template <typename A>
bool
AreaRouter<A>::get_lsa(const uint32_t index, bool& valid, bool& toohigh,
		       bool& self, vector<uint8_t>& lsa)
{
    debug_msg("Index %u\n", index);

    if (index >= _last_entry) {
	valid = false;
	toohigh = true;
	return true;
    } else {
	toohigh = false;
    }

    Lsa::LsaRef lsar = _db[index];

    if (!lsar->valid()) {
	valid = false;
	return true;
    }

    if (!lsar->available()) {
	valid = false;
	return true;
    }
    
    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    if (!lsar->maxage())
	lsar->update_age(now);

    self = lsar->get_self_originating();

    size_t len;
    uint8_t *ptr = lsar->lsa(len);
    lsa.resize(len);
    memcpy(&lsa[0], ptr, len);

    valid = true;

    return true;
}

/**
 * XXX - When we support this appendix; hook up the correct code for
 * the time being don't set any host bits.
 */
inline
uint32_t
appendixe(uint32_t link_state_id)
{
    return link_state_id;
}

template <>
void 
AreaRouter<IPv4>::summary_network_lsa_set_net(SummaryNetworkLsa *snlsa,
					      IPNet<IPv4> net)
{
    Lsa_header& header = snlsa->get_header();
    header.set_link_state_id(appendixe(ntohl(net.masked_addr().addr())));
    snlsa->set_network_mask(ntohl(net.netmask().addr()));
}

template <>
void 
AreaRouter<IPv6>::summary_network_lsa_set_net(SummaryNetworkLsa *snlsa,
					      IPNet<IPv6> net)
{
    snlsa->set_network(net);
}

template <typename A>
Lsa::LsaRef
AreaRouter<A>::summary_network_lsa(IPNet<A> net, RouteEntry<A>& rt)
{
    OspfTypes::Version version = _ospf.get_version();

    SummaryNetworkLsa *snlsa = new SummaryNetworkLsa(version);
    Lsa_header& header = snlsa->get_header();

    // Note for OSPFv2 the link state id is reset in here.
    summary_network_lsa_set_net(snlsa, net);
    snlsa->set_metric(rt.get_cost());

    switch (version) {
    case OspfTypes::V2:
	header.set_options(get_options());
	break;
    case OspfTypes::V3:
	// XXX - The setting of all the OSPFv3 fields needs
	// close attention.
	XLOG_WARNING("TBD: Inter-Area-Prefix-LSA set field values");
	break;
    }

    Lsa::LsaRef summary_lsa = snlsa;

    return summary_lsa;
}

template <typename A>
Lsa::LsaRef
AreaRouter<A>::summary_network_lsa_intra_area(OspfTypes::AreaID area,
					      IPNet<A> net,
					      RouteEntry<A>& rt,
					      bool& announce)
{
    debug_msg("Area %s net %s rentry %s\n", pr_id(area).c_str(),
	      cstring(net), cstring(rt));

    XLOG_ASSERT(rt.get_path_type() == RouteEntry<A>::intra_area);
    XLOG_ASSERT(rt.get_destination_type() == OspfTypes::Network);

    announce = true;

    // Unconditionally construct the Summary-LSA so it can be used to
    // remove old entries.

    Lsa::LsaRef summary_lsa = summary_network_lsa(net, rt);

    // Is this net covered by the originating areas area
    // ranges. Interestingly it doesn't matter if it should be
    // advertised or not. In both cases we shouldn't announce the LSA.
    bool advertise;
    if (!rt.get_discard() &&
	_ospf.get_peer_manager(). area_range_covered(area, net, advertise))
	announce = false;

    // If this route came from the backbone and this is a transit area
    // then no summarisation should take place.
    if (backbone(area) && get_transit_capability()) {
	if (rt.get_discard())
	    announce = false;
	else
	    announce = true;
    }
    return summary_lsa;
}

template <typename A>
Lsa::LsaRef
AreaRouter<A>::summary_build(OspfTypes::AreaID area, IPNet<A> net,
			     RouteEntry<A>& rt, bool& announce)
{
    debug_msg("Area %s net %s rentry %s\n", pr_id(area).c_str(),
	      cstring(net), cstring(rt));

    announce = true;
    Lsa::LsaRef summary_lsa;

    // Only intra-area routes are advertised into the backbone
    switch (rt.get_path_type()) {
    case RouteEntry<A>::intra_area:
	break;
    case RouteEntry<A>::inter_area:
	if (backbone())
	    return summary_lsa;
	break;
    case RouteEntry<A>::type1:
    case RouteEntry<A>::type2:
	// The peer manager should already have filtered out these two
	// types of routes.
	XLOG_UNREACHABLE();
	break;
    }

    switch(_area_type) {
    case OspfTypes::NORMAL:
	break;
    case OspfTypes::STUB:
    case OspfTypes::NSSA:
	if (!_summaries)
	    return summary_lsa;
	// RFC 2328 Section 12.4.3.1 Originating summary-LSAs into stub areas
	//  Type 4 summary-LSAs	(ASBR-summary-LSAs) are never
	//  originated into stub areas.
	if (rt.get_destination_type() && rt.get_as_boundary_router())
	    return summary_lsa;
	break;
    }
    
    // RFC 2328 Section 12.4.3. Summary-LSAs
    // All the sanity checks upto "is  this our own area" have already
    // been performed?
    
    RoutingTable<A>& routing_table = _ospf.get_routing_table();

    // If the nexthop falls into this area don't generate a summary.
    RouteEntry<A> nexthop_rt;
    if (routing_table.longest_match_entry(rt.get_nexthop(), nexthop_rt)) {
	if (nexthop_rt.get_area() == _area)
	    return summary_lsa;
    }

    //  If the routing table cost equals or exceeds the value
    //  LSInfinity, a summary-LSA cannot be generated for this route.
    if (rt.get_cost() >= OspfTypes::LSInfinity)
	return summary_lsa;

    OspfTypes::Version version = _ospf.get_version();

    switch (rt.get_destination_type()) {
    case OspfTypes::Router: {
	XLOG_ASSERT(rt.get_as_boundary_router());

	SummaryRouterLsa *srlsa = new SummaryRouterLsa(version);

	Lsa_header& header = srlsa->get_header();
	// AS  boundary router's Router ID
	header.set_link_state_id(rt.get_router_id());
// 	header.set_advertising_router(_ospf.get_router_id());

	switch (version) {
	case OspfTypes::V2:
	    srlsa->set_network_mask(0);
	    header.set_options(get_options());
	    break;
	case OspfTypes::V3:
	    // XXX - The setting of all the OSPFv3 fields need close attention.
	    XLOG_WARNING("setting destination ID "
			 "to router ID is this correct?");
	    srlsa->set_destination_id(rt.get_router_id());
	    srlsa->set_options(get_options());
	    break;
	}

	srlsa->set_metric(rt.get_cost());

	summary_lsa = Lsa::LsaRef(srlsa);
    }
	break;
    case OspfTypes::Network: {
	switch (rt.get_path_type()) {
	case RouteEntry<A>::intra_area:
	    return summary_network_lsa_intra_area(area, net, rt, announce);
	    break;
	case RouteEntry<A>::inter_area:
	    return summary_network_lsa(net, rt);
	    break;
	case RouteEntry<A>::type1:
	case RouteEntry<A>::type2:
	    // The peer manager should already have filtered out these two
	    // types of routes.
	    XLOG_UNREACHABLE();
	    break;
	}
    }
	break;
    }

    return summary_lsa;
}

template <typename A>
void
AreaRouter<A>::summary_announce(OspfTypes::AreaID area, IPNet<A> net,
				RouteEntry<A>& rt, bool push)
{
    debug_msg("Area %s net %s rentry %s push %s\n", pr_id(area).c_str(),
	      cstring(net), cstring(rt), pb(push));

    XLOG_ASSERT(area != _area);
    XLOG_ASSERT(area == rt.get_area());

    // If this is a discard route generated by an area range then
    // request a push of all the routes. We are seeing the discard
    // route either due to a reconfiguration or due to the first route
    // falling into an area range. Its unfortunate that the first
    // route to fall into a range will cause a recompute.
    if (!push && rt.get_discard()) {
	PeerManager<A>& pm = _ospf.get_peer_manager();
	pm.summary_push(_area);
	// return immediately the push will cause all the routes to be
	// pushed through again anyway.
	return;
    }

    bool announce;
    Lsa::LsaRef lsar = summary_build(area, net, rt, announce);
    if (0 == lsar.get())
	return;

    // Set the general fields.
    lsar->get_header().set_advertising_router(_ospf.get_router_id());
    lsar->set_self_originating(true);
    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    lsar->record_creation_time(now);
    lsar->encode();

    if (push) {
	// See if its already being announced.
	size_t index;
	if (find_lsa(lsar, index)) {
	    // Remove it if it should no longer be announced.
	    if(!announce) {
		lsar = _db[index];
		lsar->set_maxage();
		premature_aging(lsar, index);
	    }
	    // It is already being announced so out of here.
	    return;
	}
    }

#ifdef PARANOIA
    // Make sure its not being announced
    size_t index;
    if (find_lsa(lsar, index))
	XLOG_FATAL("LSA should not be announced \n%s", cstring(*_db[index]));
#endif
    if (!announce) {
	return;
    }
    add_lsa(lsar);
    refresh_summary_lsa(lsar);
}

template <typename A>
void
AreaRouter<A>::refresh_summary_lsa(Lsa::LsaRef lsar)
{
    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    lsar->update_age_and_seqno(now);

    lsar->get_timer() = _ospf.get_eventloop().
	new_oneoff_after(TimeVal(OspfTypes::LSRefreshTime, 0),
			 callback(this, &AreaRouter<A>::refresh_summary_lsa,
				  lsar));

    // Announce this LSA to all neighbours.
    publish_all(lsar);
}

template <typename A>
void
AreaRouter<A>::summary_withdraw(OspfTypes::AreaID area, IPNet<A> net,
				RouteEntry<A>& rt)
{
    debug_msg("Area %s net %s\n", pr_id(area).c_str(), cstring(net));

    XLOG_ASSERT(area != _area);
    XLOG_ASSERT(area == rt.get_area());

    bool announce;
    Lsa::LsaRef lsar = summary_build(area, net, rt, announce);
    if (0 == lsar.get())
	return;

    // Set the advertising router otherwise the lookup will fail.
    lsar->get_header().set_advertising_router(_ospf.get_router_id());

    if (!announce) {
#ifdef PARANOIA
	// Make sure its not being announced
	size_t index;
	if (find_lsa(lsar, index))
	    XLOG_FATAL("LSA should not be announced \n%s",
		       cstring(*_db[index]));
#endif
	return;
    }

    // Withdraw the LSA.
    size_t index;
    if (find_lsa(lsar, index)) {
	// Remove it if it should no longer be announced.
	lsar = _db[index];
	lsar->set_maxage();
	premature_aging(lsar, index);
    } else {
	XLOG_WARNING("LSA not being announced \n%s", cstring(*lsar));
    }
}

template <typename A>
bool
AreaRouter<A>::external_area_type() const
{
    bool accept = true;

    switch(_area_type) {
    case OspfTypes::NORMAL:
	accept = true;
	break;
    case OspfTypes::STUB:
	accept = false;
	break;
    case OspfTypes::NSSA:
	accept = true;
	break;
    }
    
    return accept;
}

template <>
void
AreaRouter<IPv4>::external_copy_net_nexthop(IPv4,
					    ASExternalLsa *dst,
					    ASExternalLsa *src)
{
    dst->get_header().set_link_state_id(src->get_header().get_link_state_id());
    dst->set_network_mask(src->get_network_mask());
    dst->set_forwarding_address_ipv4(src->get_forwarding_address_ipv4());
}

template <>
void
AreaRouter<IPv6>::external_copy_net_nexthop(IPv6,
					    ASExternalLsa *dst,
					    ASExternalLsa *src)
{
    IPNet<IPv6> addr = src->get_network();
    dst->set_network(addr);
    dst->set_forwarding_address_ipv6(src->get_forwarding_address_ipv6());
}

template <typename A>
Lsa::LsaRef
AreaRouter<A>::external_generate_type7(Lsa::LsaRef lsar, bool& indb)
{
    ASExternalLsa *aselsa = dynamic_cast<ASExternalLsa *>(lsar.get());
    XLOG_ASSERT(aselsa);

    OspfTypes::Version version = _ospf.get_version();
    Type7Lsa *type7= new Type7Lsa(version);
    Lsa::LsaRef t7(type7);    
    Lsa_header& header = type7->get_header();

    switch(version) {
    case OspfTypes::V2: {
	Options options(version, aselsa->get_header().get_options());
	bool pbit = false;
	if (_type7_propagate && 
	    !_ospf.get_peer_manager().area_border_router_p())
	    pbit = true;
	options.set_p_bit(pbit);
	header.set_options(options.get_options());
    }
	break;
    case OspfTypes::V3:
	XLOG_WARNING("TBD - AS-External-LSA set field values");
	break;
    }

    external_copy_net_nexthop(A::ZERO(), type7, aselsa);
    header.
	set_advertising_router(aselsa->get_header().get_advertising_router());
    type7->set_metric(aselsa->get_metric());
    type7->set_e_bit(aselsa->get_e_bit());
    type7->set_external_route_tag(aselsa->get_external_route_tag());
    type7->set_self_originating(true);
    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    type7->record_creation_time(now);

    type7->encode();

    indb = true;

    // If this LSA already exists in the database just return it.
    size_t index;
    if (find_lsa(t7, index)) {
	return _db[index];
    }

    indb = false;

    return t7;
}

template <typename A>
Lsa::LsaRef
AreaRouter<A>::external_generate_external(Lsa::LsaRef lsar)
{
    Type7Lsa *type7 = dynamic_cast<Type7Lsa *>(lsar.get());
    XLOG_ASSERT(type7);

    OspfTypes::Version version = _ospf.get_version();
    ASExternalLsa *aselsa= new ASExternalLsa(version);
    Lsa::LsaRef a(aselsa);
    Lsa_header& header = aselsa->get_header();

    switch(version) {
    case OspfTypes::V2:
	header.set_options(get_options());
	break;
    case OspfTypes::V3:
	XLOG_WARNING("TBD - AS-External-LSA set field values");
	break;
    }

    external_copy_net_nexthop(A::ZERO(), aselsa, type7);
    header.
	set_advertising_router(type7->get_header().get_advertising_router());
    aselsa->set_metric(type7->get_metric());
    aselsa->set_e_bit(type7->get_e_bit());
    aselsa->set_external_route_tag(type7->get_external_route_tag());
    aselsa->set_self_originating(true);
    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    aselsa->record_creation_time(now);

    aselsa->encode();

    // If this LSA already exists in the database just return it.
    size_t index;
    if (find_lsa(a, index)) {
	return _db[index];
    }

    return a;
}

template <typename A>
void
AreaRouter<A>::external_announce(Lsa::LsaRef lsar, bool /*push*/)
{
    XLOG_ASSERT(lsar->external());

    switch(_area_type) {
    case OspfTypes::NORMAL:
	break;
    case OspfTypes::STUB:
	return;
	break;
    case OspfTypes::NSSA: {
	bool indb;
	lsar = external_generate_type7(lsar, indb);
	if (indb)
	    return;
    }
	break;
    }

    size_t index;
    if (find_lsa(lsar, index)) {
	XLOG_FATAL("LSA already in database: %s", cstring(*lsar));
	return;
    }
    add_lsa(lsar);
    bool multicast_on_peer;
    publish(ALLPEERS, OspfTypes::ALLNEIGHBOURS, lsar, multicast_on_peer);
}

template <typename A>
void
AreaRouter<A>::external_shove()
{
    if (!external_area_type())
	return;

    push_lsas();
}

template <typename A>
void
AreaRouter<A>::external_refresh(Lsa::LsaRef lsar)
{
    XLOG_ASSERT(lsar->external());

    TimeVal now;
    switch(_area_type) {
    case OspfTypes::NORMAL:
	break;
    case OspfTypes::STUB:
	return;
	break;
    case OspfTypes::NSSA: {
	bool indb;
	lsar = external_generate_type7(lsar, indb);
	XLOG_ASSERT(indb);
	_ospf.get_eventloop().current_time(now);
	lsar->update_age_and_seqno(now);
    }
	break;
    }

    size_t index;
    if (!find_lsa(lsar, index)) {
	XLOG_FATAL("LSA not in database: %s", cstring(*lsar));
	return;
    }
    XLOG_ASSERT(lsar == _db[index]);
    bool multicast_on_peer;
    publish(ALLPEERS, OspfTypes::ALLNEIGHBOURS, lsar, multicast_on_peer);
    push_lsas();
}

template <typename A>
void
AreaRouter<A>::external_withdraw(Lsa::LsaRef lsar)
{
    XLOG_ASSERT(lsar->external());

    switch(_area_type) {
    case OspfTypes::NORMAL:
	break;
    case OspfTypes::STUB:
	return;
	break;
    case OspfTypes::NSSA: {
	bool indb;
	lsar = external_generate_type7(lsar, indb);
	XLOG_ASSERT(indb);
	lsar->set_maxage();
    }
	break;
    }

    size_t index;
    if (!find_lsa(lsar, index)) {
	XLOG_FATAL("LSA not in database: %s", cstring(*lsar));
	return;
    }
    XLOG_ASSERT(lsar == _db[index]);
    XLOG_ASSERT(lsar->maxage());
    // XXX - Will cause a routing recomputation.
    delete_lsa(lsar, index, false /* Don't invalidate */);
    publish_all(lsar);
}

template <typename A>
bool 
AreaRouter<A>::new_router_links(PeerID peerid,
				const list<RouterLink>& router_links)
{
    if (0 == _peers.count(peerid)) {
	XLOG_WARNING("Peer not found %u", peerid);
	return false;
    }

    typename PeerMap::iterator i = _peers.find(peerid);
    PeerStateRef psr = i->second;

    psr->_router_links.clear();
    psr->_router_links.insert(psr->_router_links.begin(),
			      router_links.begin(), router_links.end());

    refresh_router_lsa();

    return true;
}

template <typename A>
bool 
AreaRouter<A>::generate_network_lsa(PeerID peerid,
				    OspfTypes::RouterID link_state_id,
				    list<OspfTypes::RouterID>& routers,
				    uint32_t network_mask)
{
    debug_msg("PeerID %u link state id %s\n", peerid,
	      pr_id(link_state_id).c_str());

    NetworkLsa *nlsa = new NetworkLsa(_ospf.get_version());
    nlsa->set_self_originating(true);
    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    nlsa->record_creation_time(now);

    Lsa_header& header = nlsa->get_header();
    header.set_link_state_id(link_state_id);
    header.set_advertising_router(_ospf.get_router_id());

#ifdef	MAX_AGE_IN_DATABASE
    size_t index;
    Lsa::LsaRef lsar = Lsa::LsaRef(nlsa);
    if (find_lsa(lsar, index)) {
	update_lsa(lsar, index);
    } else {
	add_lsa(lsar);
    }
#else
    add_lsa(Lsa::LsaRef(nlsa));
#endif

    update_network_lsa(peerid, link_state_id, routers, network_mask);
    
    return true;
}

template <typename A>
bool 
AreaRouter<A>::update_network_lsa(PeerID peerid,
				  OspfTypes::RouterID link_state_id,
				  list<OspfTypes::RouterID>& routers,
				  uint32_t network_mask)
{
    debug_msg("PeerID %u link state id %s\n", peerid,
	      pr_id(link_state_id).c_str());

    OspfTypes::Version version = _ospf.get_version();
    Ls_request lsr(version, NetworkLsa(version).get_ls_type(),
		   link_state_id, _ospf.get_router_id());

    size_t index;
    if (!find_lsa(lsr, index)) {
	XLOG_FATAL("Couldn't find Network_lsa %s in LSA database",
		   cstring(lsr));
	return false;
    }

    NetworkLsa *nlsa = dynamic_cast<NetworkLsa *>(_db[index].get());
    XLOG_ASSERT(nlsa);

    list<OspfTypes::RouterID>& attached_routers = nlsa->get_attached_routers();
    if (&routers != &attached_routers) {
	attached_routers.clear();
	attached_routers.push_back(_ospf.get_router_id()); // Add this router.
	attached_routers.insert(attached_routers.begin(),
				routers.begin(), routers.end());
    }

    switch (version) {
    case OspfTypes::V2:
	nlsa->set_network_mask(network_mask);
	nlsa->get_header().set_options(get_options());
	break;
    case OspfTypes::V3:
	nlsa->set_options(get_options());
	break;
    }

    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    nlsa->update_age_and_seqno(now);

    // Prime this Network-LSA to be refreshed.
    nlsa->get_timer() = _ospf.get_eventloop().
	new_oneoff_after(TimeVal(OspfTypes::LSRefreshTime, 0),
			 callback(this, &AreaRouter<A>::refresh_network_lsa,
				  peerid,
				  _db[index]));

    publish_all(_db[index]);

    return true;
}

template <typename A>
bool 
AreaRouter<A>::withdraw_network_lsa(PeerID peerid,
				    OspfTypes::RouterID link_state_id)
{
    debug_msg("PeerID %u link state id %s\n", peerid,
	      pr_id(link_state_id).c_str());

    OspfTypes::Version version = _ospf.get_version();
    Ls_request lsr(version, NetworkLsa(version).get_ls_type(),
		   link_state_id, _ospf.get_router_id());

    size_t index;
    if (!find_lsa(lsr, index)) {
	XLOG_FATAL("Couldn't find Network_lsa %s in LSA database",
		   cstring(lsr));
	return false;
    }

    Lsa::LsaRef lsar = _db[index];
    lsar->set_maxage();
    premature_aging(lsar, index);

    return true;
}

template <typename A>
void
AreaRouter<A>::refresh_network_lsa(PeerID peerid, Lsa::LsaRef lsar)
{
    NetworkLsa *nlsa = dynamic_cast<NetworkLsa *>(lsar.get());
    XLOG_ASSERT(nlsa);
    XLOG_ASSERT(nlsa->valid());

    uint32_t network_mask = 0;

    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	network_mask = nlsa->get_network_mask();
	break;
    case OspfTypes::V3:
	break;
    }

    update_network_lsa(peerid,
		       nlsa->get_header().get_link_state_id(),
		       nlsa->get_attached_routers(),
		       network_mask);
}

inline
string
pp_lsas(const list<Lsa::LsaRef>& lsas)
{
    string output;

    list<Lsa::LsaRef>::const_iterator i;
    for (i = lsas.begin(); i != lsas.end(); i++) {
	output += "\n\t" + (*i)->str();
    }
    
    return output;
}

template <typename A>
void
AreaRouter<A>::receive_lsas(PeerID peerid,
			    OspfTypes::NeighbourID nid,
			    list<Lsa::LsaRef>& lsas, 
			    list<Lsa_header>& direct_ack,
			    list<Lsa_header>& delayed_ack,
			    bool backup, bool dr)
{
    debug_msg("PeerID %u NeighbourID %u %s\nbackup %s dr %s\n", peerid, nid,
	      pp_lsas(lsas).c_str(),
	      backup ? "true" : "false",
	      dr ? "true" : "false");

    TimeVal now;
    _ospf.get_eventloop().current_time(now);

    routing_begin();
    
    // RFC 2328 Section 13. The Flooding Procedure
    // Validate the incoming LSAs.
    
    list<Lsa::LsaRef>::const_iterator i;
    for (i = lsas.begin(); i != lsas.end(); i++) {
	// These LSAs came over the wire they must not be self originating.
	XLOG_ASSERT(!(*i)->get_self_originating());

	// Record the creation time and initial age.
	(*i)->record_creation_time(now);

	// (1) Validate the LSA's LS checksum. 
	// (2) Check that the LSA's LS type is known.
	// Both checks already performed in the packet/LSA decoding process.

	// (3) In stub areas discard AS-external-LSA's (LS type = 5, 0x4005).
	switch(_area_type) {
	case OspfTypes::NORMAL:
 	    if ((*i)->type7())
 		continue;
	    break;
	case OspfTypes::STUB:
 	    if ((*i)->type7())
 		continue;
	    /* FALLTHROUGH */
	case OspfTypes::NSSA:
 	    if ((*i)->external())
 		continue;
	    break;
	}
	const Lsa_header& lsah = (*i)->get_header();
	size_t index;
	LsaSearch search = compare_lsa(lsah, index);

#ifdef	DEBUG_LOGGING
	debug_msg("Searching for %s\n", cstring(*(*i)));
	switch(search) {
	case NOMATCH:
	    debug_msg("No match\n");
	    break;
	case NEWER:
	    debug_msg("is newer than \n%s\n", cstring(*_db[index]));
	    break;
	case OLDER:
	    debug_msg("is older than \n%s\n", cstring(*_db[index]));
	    break;
	case EQUIVALENT:
	    debug_msg("is equivalent to \n%s\n", cstring(*_db[index]));
	    break;
	}
#endif
	// (4) MaxAge
	if (OspfTypes::MaxAge == lsah.get_ls_age()) {
	    if (NOMATCH == search) {
		if (!neighbours_exchange_or_loading()) {
		    delayed_ack.push_back(lsah);
		    continue;
		}
	    }
	}
	
	switch(search) {
	case NOMATCH:
	case NEWER: {
	    // (5) New or newer LSA.
	    // (a) If this LSA is too recent drop it.
	    if (NEWER == search) {
		TimeVal then;
		_db[index]->get_creation_time(then);
		if ((now - then) < TimeVal(OspfTypes::MinLSArrival, 0)) {
		    debug_msg("Rejecting LSA last one arrived less than "
			      "%d second(s) ago\n %s\n%s",
			      OspfTypes::MinLSArrival,
			      cstring(*(*i)) , cstring(*(_db[index])));
		    XLOG_TRACE(_ospf.trace()._input_errors,
			       "Rejecting LSA last one arrived less than "
			       "%d second(s) ago\n%s\n%s",
			       OspfTypes::MinLSArrival,
			       cstring(*(*i)) , cstring(*(_db[index])));
		    continue;
		}
	    }
	    // This is out of sequence but doing it later makes no sense.
	    // (f) Self orignating LSAs 
	    // RFC 2328 Section 13.4. Receiving self-originated LSAs

	    bool match = false;
	    if (NEWER == search)
		match = _db[index]->get_self_originating();
	    if (self_originated((*i), match, index)) {
		bool multicast_on_peer;
		publish(peerid, nid, (*i), multicast_on_peer);
		continue;
	    }

	    // (b) Flood this LSA to all of our neighbours.
	    // RFC 2328 Section 13.3. Next step in the flooding procedure

	    // If this is an AS-external-LSA send it to all areas.
	    if ((*i)->external())
		external_flood_all_areas((*i));

	    if ((*i)->type7())
		external_type7_translate((*i));

	    // Set to true if the LSA was multicast out of this
	    // interface. If it was, there is no requirement to send an
	    // ACK.
	    bool multicast_on_peer;
	    publish(peerid, nid, (*i), multicast_on_peer);

	    // (c) Remove the current copy from neighbours
	    // retransmission lists. If this is a new LSA then there
	    // can be no version of this LSA on the neighbours
	    // retransmission lists. If this is a newer version of an
	    // existing LSA then the old LSA will be invalidated in
	    // update LSA, therefore the neighbours will not try and
	    // transmit it.
	    // (d) Install the new LSA.
	    if (NOMATCH == search) {
		add_lsa((*i));
	    } else {
		if ((*i)->external()) {
		    // The LSA that was matched should have been
		    // invalidated by the external code. So the new
		    // LSA just needs to added to the database.
		    XLOG_ASSERT(!_db[index]->valid());
		    add_lsa((*i));
		} else {
		    update_lsa((*i), index);
		}
	    }
	    // Start aging this LSA if its not a AS-External-LSA
	    if (!(*i)->external())
		age_lsa((*i));
	    routing_add(*i, NOMATCH != search);
	    
	    // (e) Possibly acknowledge this LSA.
	    // RFC 2328 Section 13.5 Sending Link State Acknowledgment Packets

	    if (!multicast_on_peer) {
		if ((backup && dr) || !backup)
		    delayed_ack.push_back(lsah);
	    }
	    
	    // Too late to do this now, its after (a).
	    // (f) Self orignating LSAs 
	    // RFC 2328 Section 13.4. Receiving self-originated LSAs

	}
	    break;
	case OLDER:
	    // XXX - Is this really where case (6) belongs?
	    // (6) If this LSA is on the sending neighbours request
	    // list something has gone wrong.

	    if (on_link_state_request_list(peerid, nid, (*i))) {
		event_bad_link_state_request(peerid, nid);
		goto out;
	    }

	    // (8) The database copy is more recent than the received LSA.
	    // The LSA's LS sequence number is wrapping.
	    if (_db[index]->get_header().get_ls_age() == OspfTypes::MaxAge &&
		_db[index]->get_header().get_ls_sequence_number() == 
		OspfTypes::MaxSequenceNumber)
		continue;
	    // We have a more up to date copy of this LSA than our
	    // neighbour so blast this LSA directly back to this neighbour.
	    send_lsa(peerid, nid, _db[index]);
	    break;
	case EQUIVALENT:
	    // (7) The LSAs are equivalent.
	    // (a) This might be an "implied acknowledgement".
	    if (_db[index]->exists_nack(nid)) { 

		_db[index]->remove_nack(nid);
		// (b) An "implied acknowledgement".
		if (backup && dr)
		    delayed_ack.push_back(lsah);
	    } else {
		// (b) Not an "implied acknowledgement".
		direct_ack.push_back(lsah);
	    }
	    break;
	}
    }

 out:
    push_lsas();
    external_push_all_areas();
    routing_end();
}

template <typename A>
bool
AreaRouter<A>::age_lsa(Lsa::LsaRef lsar)
{
    size_t index;

    XLOG_ASSERT(!lsar->get_self_originating());

    if (!find_lsa(lsar, index)) {
	XLOG_WARNING("LSA not in database: %s", cstring(*lsar));
	return false;
    }

    lsar->get_timer() = _ospf.get_eventloop().
	new_oneoff_after(TimeVal(OspfTypes::MaxAge -
				 lsar->get_header().get_ls_age(), 0),
			 callback(this,
				  &AreaRouter<A>::maxage_reached, lsar,index));
    return true;
}

template <typename A>
void
AreaRouter<A>::maxage_reached(Lsa::LsaRef lsar, size_t i)
{
    size_t index;

    XLOG_ASSERT(!lsar->external());

    if (!find_lsa(lsar, index))
	XLOG_FATAL("LSA not in database: %s", cstring(*lsar));

    if (i != index)
	XLOG_FATAL("Indexes don't match %u != %u %s",  XORP_UINT_CAST(i),
		   XORP_UINT_CAST(index),
		   cstring(*_db[index]));

#ifdef PARANOIA
    if (!lsar->get_self_originating()) {
	TimeVal now;
	_ospf.get_eventloop().current_time(now);
	if (!lsar->maxage())
	    lsar->update_age(now);
    }
#endif
    if (OspfTypes::MaxAge != lsar->get_header().get_ls_age())
	XLOG_FATAL("LSA is not MaxAge %s", cstring(*lsar));
    
#ifndef	MAX_AGE_IN_DATABASE
    delete_lsa(lsar, index, false /* Don't invalidate */);
#endif
    publish_all(lsar);

    // Clear the timer otherwise there is a circular dependency.
    // The LSA contains a XorpTimer that points back to the LSA.
    lsar->get_timer().clear();
}

template <typename A>
void
AreaRouter<A>::premature_aging(Lsa::LsaRef lsar, size_t i)
{
    XLOG_ASSERT(lsar->get_self_originating());
    maxage_reached(lsar, i);
}

template <typename A>
bool
AreaRouter<A>::add_lsa(Lsa::LsaRef lsar)
{
    size_t index;
    XLOG_ASSERT(!find_lsa(lsar, index));
    XLOG_ASSERT(lsar->valid());

    // If there are no readers we can put this LSA into an empty slot.
    if (0 == _readers && !_empty_slots.empty()) {
	size_t esi = _empty_slots.front();
	if (esi >= _last_entry)
	    _last_entry = esi + 1;
	_db[esi] = lsar;
	_empty_slots.pop_front();
	return true;
    }

    if (_last_entry < _allocated_entries) {
	_db[_last_entry] = lsar;
    } else {
	_db.push_back(lsar);
	_allocated_entries++;
    }
    _last_entry++;

    return true;
}

template <typename A>
bool
AreaRouter<A>::delete_lsa(Lsa::LsaRef lsar, size_t index, bool invalidate)
{
    Lsa_header& dblsah = _db[index]->get_header();
    XLOG_ASSERT(dblsah.get_ls_type() == lsar->get_header().get_ls_type());
    XLOG_ASSERT(dblsah.get_link_state_id() == 
		lsar->get_header().get_link_state_id());
    XLOG_ASSERT(dblsah.get_advertising_router() ==
		lsar->get_header().get_advertising_router());

    XLOG_ASSERT(_db[index]->valid());

    // This LSA is being deleted remove it from the routing computation.
    routing_delete(lsar);

    if (invalidate)
	_db[index]->invalidate();
    _db[index] = _invalid_lsa;
    _empty_slots.push_back(index);

    // _last_entry points one past the last entry, if the deleted LSA
    // was at the end of the array then the _last_entry pointer can be
    // decreased.
    while(0 != index && index + 1 == _last_entry && !_db[index]->valid() && 
	  0 != _last_entry) {
	_last_entry--;
	index--;
    }

    return true;
}

template <typename A>
bool
AreaRouter<A>::update_lsa(Lsa::LsaRef lsar, size_t index)
{
    Lsa_header& dblsah = _db[index]->get_header();
    XLOG_ASSERT(dblsah.get_ls_type() == lsar->get_header().get_ls_type());
    XLOG_ASSERT(dblsah.get_link_state_id() == 
		lsar->get_header().get_link_state_id());
    XLOG_ASSERT(dblsah.get_advertising_router() ==
		lsar->get_header().get_advertising_router());

    XLOG_ASSERT(_db[index]->valid());
    // A LSA arriving over the wire should never replace a
    // self originating LSA.
    XLOG_ASSERT(!_db[index]->get_self_originating());
    if (0 == _readers) {
	_db[index]->invalidate();
	_db[index] = lsar;
    } else {
	delete_lsa(lsar, index, true /* Mark the LSA as invalid */);
	add_lsa(lsar);
    }

    return true;
}

template <typename A>
bool
AreaRouter<A>::find_lsa(const Ls_request& lsr, size_t& index) const
{
    for(index = 0 ; index < _last_entry; index++) {
	if (!_db[index]->valid())
	    continue;
	Lsa_header& dblsah = _db[index]->get_header();
	if (dblsah.get_ls_type() != lsr.get_ls_type())
	    continue;

	if (dblsah.get_link_state_id() != lsr.get_link_state_id())
	    continue;

	if (dblsah.get_advertising_router() != lsr.get_advertising_router())
	    continue;

	return true;
    }

    return false;
}

template <typename A>
bool
AreaRouter<A>::find_lsa(Lsa::LsaRef lsar, size_t& index) const
{
    const Lsa_header lsah = lsar->get_header();
    Ls_request lsr(_ospf.get_version(), lsah.get_ls_type(),
		   lsah.get_link_state_id(), lsah.get_advertising_router());

    return find_lsa(lsr, index);
}

template <typename A>
bool
AreaRouter<A>::find_network_lsa(uint32_t link_state_id, size_t& index) const
{
    uint32_t ls_type = NetworkLsa(_ospf.get_version()).get_ls_type();

    for(index = 0 ; index < _last_entry; index++) {
	if (!_db[index]->valid())
	    continue;
	Lsa_header& dblsah = _db[index]->get_header();
	if (dblsah.get_ls_type() != ls_type)
	    continue;

	if (dblsah.get_link_state_id() != link_state_id)
	    continue;

	// Note we deliberately don't check for advertising router.

	return true;
    }

    return false;
}

/**
 * RFC 2328 Section 13.1 Determining which LSA is newer.
 */
template <typename A>
typename AreaRouter<A>::LsaSearch
AreaRouter<A>::compare_lsa(const Lsa_header& candidate,
			   const Lsa_header& current) const
{
    debug_msg("router id: %s\n", pr_id(_ospf.get_router_id()).c_str());
    debug_msg("\ncandidate: %s\n  current: %s\n", cstring(candidate),
	      cstring(current));

    const int32_t candidate_seqno = candidate.get_ls_sequence_number();
    const int32_t current_seqno = current.get_ls_sequence_number();

    if (current_seqno != candidate_seqno) {
	if (current_seqno > candidate_seqno)
	    return OLDER;
	if (current_seqno < candidate_seqno)
	    return NEWER;
    }

    if (current.get_ls_checksum() > candidate.get_ls_checksum())
	return OLDER;
    if (current.get_ls_checksum() < candidate.get_ls_checksum())
	return NEWER;

    if (current.get_ls_age() == candidate.get_ls_age())
	return EQUIVALENT;

    if (current.get_ls_age() == OspfTypes::MaxAge)
	return OLDER;
    if (candidate.get_ls_age() == OspfTypes::MaxAge)
	return NEWER;
	
    if(abs(current.get_ls_age() - candidate.get_ls_age()) > 
       OspfTypes::MaxAgeDiff) {
	return candidate.get_ls_age() < current.get_ls_age() ? NEWER : OLDER;
    }
	
    // These two LSAs are identical.
    return EQUIVALENT;
}

template <typename A>
typename AreaRouter<A>::LsaSearch
AreaRouter<A>::compare_lsa(const Lsa_header& lsah, size_t& index) const
{
    Ls_request lsr(_ospf.get_version(), lsah.get_ls_type(),
		   lsah.get_link_state_id(), lsah.get_advertising_router());

    if (find_lsa(lsr, index)) {
	if (!_db[index]->maxage()) {
	    // Update the age before checking this field.
	    TimeVal now;
	    _ospf.get_eventloop().current_time(now);
	    _db[index]->update_age(now);
	}
	return compare_lsa(lsah, _db[index]->get_header());
    }

    return NOMATCH;
}

template <typename A>
typename AreaRouter<A>::LsaSearch
AreaRouter<A>::compare_lsa(const Lsa_header& lsah) const
{
    size_t index;
    return compare_lsa(lsah, index);
}

template <typename A>
bool
AreaRouter<A>::newer_lsa(const Lsa_header& lsah) const
{
    switch(compare_lsa(lsah)) {
    case NOMATCH:
    case NEWER:
	return true;
    case EQUIVALENT:
    case OLDER:
	return false;
    }

    XLOG_UNREACHABLE();
    return true;
}

template <typename A>
bool
AreaRouter<A>::get_lsas(const list<Ls_request>& reqs,
			list<Lsa::LsaRef>& lsas)
{
    TimeVal now;
    _ospf.get_eventloop().current_time(now);

    list<Ls_request>::const_iterator i;
    for(i = reqs.begin(); i != reqs.end(); i++) {
	size_t index;
	if (!find_lsa(*i, index)) {
	    XLOG_WARNING("Unable to find %s", cstring(*i));
	    return false;
	}
	Lsa::LsaRef lsar = _db[index];
	// Start the delay timer so we won't transmit any more self
	// originating LSAs until the appropriate time has passed.
	if (lsar->get_self_originating())
	    _queue.fire();
	if (!lsar->maxage())
	    lsar->update_age(now);
	lsas.push_back(lsar);
    }
    
    return true;
}

template <typename A>
DataBaseHandle
AreaRouter<A>::open_database(bool& empty)
{
    // While there are readers no entries can be add to the database
    // before the _last_entry.

    _readers++;

    // Snapshot the current last entry position. While the database is
    // open (_readers > 0) new entries will be added past _last_entry.

    DataBaseHandle dbh = DataBaseHandle(true, _last_entry);

    empty = !subsequent(dbh);

    return dbh;
}

template <typename A>
bool
AreaRouter<A>::valid_entry_database(size_t index)
{
    Lsa::LsaRef lsar = _db[index];

    // This LSA is not valid.
    if (!lsar->valid())
	return false;

    if (!lsar->maxage()) {
	TimeVal now;
	_ospf.get_eventloop().current_time(now);
	lsar->update_age(now);
    }

    if (lsar->maxage()) {
#ifdef	MAX_AGE_IN_DATABASE
	// XXX - This is the only way that a MaxAge LSA gets removed
	// from the database. When this code is removed make
	// valid_entry and subsequent const again.
	// If we find a LSA with MaxAge in the database and the nack
	// list is empty it can be removed from the database.
	if (lsar->empty_nack())
	    delete_lsa(lsar, index, true /* Invalidate */);
#endif	
	return false;
    }

    // There is no wire format for this LSA.
    if (!lsar->available())
	return false;

    return true;
}

template <typename A>
bool
AreaRouter<A>::subsequent(DataBaseHandle& dbh)
{
    bool another = false;
    for (size_t index = dbh.position(); index < dbh.last(); index++) {
	if (!valid_entry_database(index))
	    continue;
	another = true;
	break;
    }

    return another;
}

template <typename A>
Lsa::LsaRef
AreaRouter<A>::get_entry_database(DataBaseHandle& dbh, bool& last)
{
    XLOG_ASSERT(dbh.valid());

    uint32_t position;

    do {
	position = dbh.position();

	if (position >= _db.size())
	    XLOG_FATAL("Index too far %d length %d", position,
		       XORP_INT_CAST(_db.size()));

	dbh.advance(last);
    } while(!valid_entry_database(position));

    // If this is not the last entry make sure there is a subsequent
    // valid entry.
    if (!last)
	last = !subsequent(dbh);

    return _db[position];
}

template <typename A>
void
AreaRouter<A>::close_database(DataBaseHandle& dbh)
{
    XLOG_ASSERT(dbh.valid());
    XLOG_ASSERT(0 != _readers);
    _readers--;

    if (subsequent(dbh))
	XLOG_WARNING("Database closed with entries remaining");

    dbh.invalidate();
}

template <typename A>
void
AreaRouter<A>::testing_print_link_state_database() const
{
    fprintf(stderr, "****** DATABASE START ******\n");
    for (size_t index = 0 ; index < _last_entry; index++) {
	Lsa::LsaRef lsar = _db[index];
	if (!lsar->valid())
	    continue;
	// Please leave this as a fprintf its for debugging only.
	fprintf(stderr, "%s\n", cstring(*lsar));
    }
    fprintf(stderr, "****** DATABASE END ********\n");
}

template <typename A>
bool
AreaRouter<A>::update_router_links()
{
    RouterLsa *router_lsa = dynamic_cast<RouterLsa *>(_router_lsa.get());
    XLOG_ASSERT(router_lsa);
    bool empty = router_lsa->get_router_links().empty();
    router_lsa->get_router_links().clear();
    typename PeerMap::iterator i;
    for(i = _peers.begin(); i != _peers.end(); i++) {
	PeerStateRef temp_psr = i->second;
	if (temp_psr->_up) {
	    typename list<RouterLink>::iterator j = 
		temp_psr->_router_links.begin();
	    for(; j != temp_psr->_router_links.end(); j++)
		router_lsa->get_router_links().push_back(*j);
	}
    }

    // If we weren't advertising and we still aren't return.
    if (empty && router_lsa->get_router_links().empty())
	return false;

    PeerManager<A>& pm = _ospf.get_peer_manager();
    router_lsa->set_v_bit(pm.virtual_link_endpoint(_area));
    router_lsa->set_e_bit(pm.as_boundary_router_p());
    router_lsa->set_b_bit(pm.area_border_router_p());

    switch (_ospf.get_version()) {
    case OspfTypes::V2:
	router_lsa->get_header().set_options(get_options());
	break;
    case OspfTypes::V3:
	router_lsa->set_options(get_options());
	break;
    }

    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    router_lsa->update_age_and_seqno(now);

    // Prime this Router-LSA to be refreshed.
    router_lsa->get_timer() = _ospf.get_eventloop().
	new_oneoff_after(TimeVal(OspfTypes::LSRefreshTime, 0),
			 callback(this, &AreaRouter<A>::refresh_router_lsa));

    return true;
}

template <typename A>
void
AreaRouter<A>::refresh_router_lsa()
{
    if (update_router_links()) {
	// publish the router LSA.
	_queue.add(_router_lsa);
    }
}

template <typename A>
void
AreaRouter<A>::publish(const PeerID peerid, const OspfTypes::NeighbourID nid,
		       Lsa::LsaRef lsar, bool &multicast_on_peer) const
{
    debug_msg("Publish: %s\n", cstring(*lsar));

    TimeVal now;
    _ospf.get_eventloop().current_time(now);

    // Update the age field unless its self originating.
    if (lsar->get_self_originating()) {
	if (OspfTypes::MaxSequenceNumber == lsar->get_ls_sequence_number())
	    XLOG_FATAL("TBD: Flush this LSA and generate a new LSA");
    } else {
	if (!lsar->maxage())
	    lsar->update_age(now);
    }

    typename PeerMap::const_iterator i;
    for(i = _peers.begin(); i != _peers.end(); i++) {
	PeerStateRef temp_psr = i->second;
	if (temp_psr->_up) {
	    bool multicast;
	    if (!_ospf.get_peer_manager().
		queue_lsa(i->first, peerid, nid, lsar, multicast))
		XLOG_FATAL("Unable to queue LSA");
	    // Did this LSA get broadcast/multicast on the
	    // peer/interface that it came in on.
	    if (peerid == i->first)
		multicast_on_peer = multicast;
	}
    }
}

template <typename A>
void
AreaRouter<A>::publish_all(Lsa::LsaRef lsar)
{
    debug_msg("Publish: %s\n", cstring(*lsar));
    bool multicast_on_peer;
    publish(ALLPEERS, OspfTypes::ALLNEIGHBOURS, lsar, multicast_on_peer);

    push_lsas();	// NOTE: a push after every LSA.
}

template <typename A>
void
AreaRouter<A>::push_lsas()
{
    typename PeerMap::iterator i;
    for(i = _peers.begin(); i != _peers.end(); i++) {
	PeerStateRef temp_psr = i->second;
	if (temp_psr->_up) {
	    if (!_ospf.get_peer_manager().push_lsas(i->first))
		XLOG_FATAL("Unable to push LSAs");
	}
    }
}

template <typename A>
void
AreaRouter<A>::external_type7_translate(Lsa::LsaRef lsar)
{
    Type7Lsa *t7 = dynamic_cast<Type7Lsa *>(lsar.get());
    XLOG_ASSERT(t7);

    switch (_ospf.get_version()) {
    case OspfTypes::V2:
	if (t7->get_forwarding_address_ipv4() == IPv4::ZERO())
	    return;
	break;
    case OspfTypes::V3:
	if (!t7->get_f_bit())
	    return;
	break;
    }

    // If the propogate bit isn't set there is nothing todo.
    if (!external_propagate_bit(lsar))
	return;

    switch(_translator_state) {
    case OspfTypes::ENABLED:
    case OspfTypes::ELECTED:
	break;
    case OspfTypes::DISABLED:
	return;
	break;
    }

    _external_flooding = true;

    // Convert this Type-7-LSA into an AS-External-LSA and flood.
    external_flood_all_areas(external_generate_external(lsar));
}

template <typename A>
void
AreaRouter<A>::external_flood_all_areas(Lsa::LsaRef lsar)
{
    debug_msg("Flood all areas %s\n", cstring(*lsar));

    _external_flooding = true;
    PeerManager<A>& pm = _ospf.get_peer_manager();
    pm.external_announce(_area, lsar);
}

template <typename A>
void
AreaRouter<A>::external_push_all_areas()
{
    if (_external_flooding) {
	PeerManager<A>& pm = _ospf.get_peer_manager();
	pm.external_shove(_area);
	_external_flooding = false;
    }
}

template <typename A>
bool
AreaRouter<A>::neighbours_exchange_or_loading() const
{
    typename PeerMap::const_iterator i;
    for(i = _peers.begin(); i != _peers.end(); i++) {
	if (_ospf.get_peer_manager().
	    neighbours_exchange_or_loading(i->first, _area))
	    return true;
    }

    return false;
}

template <typename A>
bool
AreaRouter<A>::on_link_state_request_list(const PeerID peerid,
					  const OspfTypes::NeighbourID nid,
					  Lsa::LsaRef lsar) const
{
    return _ospf.get_peer_manager().
	on_link_state_request_list(peerid, _area, nid, lsar);
}

template <typename A>
bool
AreaRouter<A>::event_bad_link_state_request(const PeerID peerid,
				    const OspfTypes::NeighbourID nid) const
{
    return _ospf.get_peer_manager().
	event_bad_link_state_request(peerid, _area, nid);
}

template <typename A>
bool
AreaRouter<A>::send_lsa(const PeerID peerid,
			const OspfTypes::NeighbourID nid,
			Lsa::LsaRef lsar) const
{
    return _ospf.get_peer_manager().send_lsa(peerid, _area, nid, lsar);
}

template <>
bool
AreaRouter<IPv4>::self_originated_by_interface(Lsa::LsaRef lsar, IPv4) const
{
    if (0 == dynamic_cast<NetworkLsa *>(lsar.get()))
	return false;

    IPv4 address(htonl(lsar->get_header().get_link_state_id()));
    return _ospf.get_peer_manager().known_interface_address(address);
}

template <>
bool
AreaRouter<IPv6>::self_originated_by_interface(Lsa::LsaRef, IPv6) const
{
    XLOG_FATAL("Not required for OSPFv3");
    return false;
}

template <typename A>
bool
AreaRouter<A>::self_originated(Lsa::LsaRef lsar, bool lsa_exists, size_t index)
{
    // RFC 2328 Section 13.4. Receiving self-originated LSAs

    debug_msg("lsar: %s\nexists: %s index: %u\n", cstring((*lsar)),
	      lsa_exists ? "true" : "false", XORP_UINT_CAST(index));
    if (lsa_exists)
	debug_msg("database copy: %s\n", cstring((*_db[index])));

    bool originated = lsa_exists;
    if (!originated) {
	if (lsar->get_header().get_advertising_router() ==
	    _ospf.get_router_id()) {
	    originated = true;
	} else {
	    switch (_ospf.get_version()) {
	    case OspfTypes::V2:
		if (self_originated_by_interface(lsar))
		    originated = true;
		break;
	    case OspfTypes::V3:
		break;
	    }
	}
    }

    if (!originated)
	return false;

    debug_msg("router id: %s\n", pr_id(_ospf.get_router_id()).c_str());
    debug_msg("self originated: %s\n", cstring((*lsar)));

    // If we got this far this is a self-originated LSA that needs to
    // be removed from the wild.

    // A database copy of this LSA exists. The new LSA that arrived is
    // newer than the database copy. Copy the sequence number from the
    // new LSA into the database LSA increment it and flood.
    if (lsa_exists) {
	_db[index]->set_ls_sequence_number(lsar->get_ls_sequence_number());
	lsar = _db[index];
	lsar->increment_sequence_number();
	lsar->encode();
	return true;
    }

    // This is a spurious LSA that was probably originated by us in
    // the past just get rid of it by setting it to MaxAge.
    if (!lsar->maxage())
	lsar->set_maxage();
#ifdef	MAX_AGE_IN_DATABASE
    debug_msg("Adding MaxAge lsa to database\n%s\n", cstring(*lsar));
    add_lsa(lsar);
#endif

    return true;
}

template <typename A>
void
AreaRouter<A>::RouterVertex(Vertex& v)
{
    v.set_version(_ospf.get_version());
    v.set_type(OspfTypes::Router);
    v.set_nodeid(_ospf.get_router_id());
    v.set_origin(true);
    v.set_lsa(_router_lsa);
}

template <typename A>
void
AreaRouter<A>::routing_begin()
{
}

template <typename A>
void
AreaRouter<A>::routing_add(Lsa::LsaRef lsar, bool known)
{
    debug_msg("%s known %s\n", cstring(*lsar), known ? "true" : "false");
}

template <typename A>
void
AreaRouter<A>::routing_delete(Lsa::LsaRef lsar)
{
    debug_msg("%s\n", cstring(*lsar));

    routing_schedule_total_recompute();
}

template <typename A>
void
AreaRouter<A>::routing_end()
{
    routing_schedule_total_recompute();
}

template <typename A>
void 
AreaRouter<A>::routing_schedule_total_recompute()
{
    if (_routing_recompute_timer.scheduled())
	return;

    _routing_recompute_timer = _ospf.get_eventloop().
	new_oneoff_after(TimeVal(_routing_recompute_delay, 0),
			 callback(this, &AreaRouter<A>::routing_timer));
    
}

template <typename A>
void 
AreaRouter<A>::routing_timer()
{
    routing_total_recompute();
}

template <typename A>
void 
AreaRouter<A>::routing_total_recompute()
{
    switch (_ospf.get_version()) {
    case OspfTypes::V2:
	routing_total_recomputeV2();
	break;
    case OspfTypes::V3:
	routing_total_recomputeV3();
	break;
    }
}

template <> void AreaRouter<IPv4>::
routing_area_rangesV2(list<RouteCmd<Vertex> >& r);
template <> void AreaRouter<IPv4>::routing_inter_areaV2();
template <> void AreaRouter<IPv4>::routing_transit_areaV2();
template <> void AreaRouter<IPv4>::routing_as_externalV2();

template <>
void 
AreaRouter<IPv4>::routing_total_recomputeV2()
{
#ifdef	DEBUG_LOGGING
    testing_print_link_state_database();
#endif

    // RFC 2328 16.1.  Calculating the shortest-path tree for an area

    Spt<Vertex> spt(_ospf.trace()._spt);
    bool transit_capability = false;

    // Add this router to the SPT table.
    Vertex rv;
    RouterVertex(rv);
    spt.add_node(rv);
    spt.set_origin(rv);

    for (size_t index = 0 ; index < _last_entry; index++) {
	Lsa::LsaRef lsar = _db[index];
	if (!lsar->valid() || lsar->maxage())
	    continue;

	RouterLsa *rlsa;
	
	if (0 != (rlsa = dynamic_cast<RouterLsa *>(lsar.get()))) {

	    if (rlsa->get_v_bit())
		transit_capability = true;
	    
	    Vertex v;

	    v.set_version(_ospf.get_version());
	    v.set_type(OspfTypes::Router);
	    v.set_nodeid(rlsa->get_header().get_link_state_id());
  	    v.set_lsa(lsar);

	    // Don't add this router back again.
	    if (spt.exists_node(v)) {
		debug_msg("%s Router %s\n",
		       pr_id(_ospf.get_router_id()).c_str(),
		       cstring(v));
		if (rv == v)
		    v.set_origin(rv.get_origin());
	    } else {
		debug_msg("%s Add %s\n",
		       pr_id(_ospf.get_router_id()).c_str(),
		       cstring(v));
		spt.add_node(v);
	    }

	    debug_msg("%s Router-Lsa %s\n",
		   pr_id(_ospf.get_router_id()).c_str(),
		   cstring(*rlsa));

	    switch(_ospf.get_version()) {
	    case OspfTypes::V2:
		routing_router_lsaV2(spt, v, rlsa);
		break;
	    case OspfTypes::V3:
		routing_router_lsaV3(spt, v, rlsa);
		break;
	    }
	}
    }

    set_transit_capability(transit_capability);

    RoutingTable<IPv4>& routing_table = _ospf.get_routing_table();
    routing_table.begin(_area);

    // Compute the SPT.
    list<RouteCmd<Vertex> > r;
    spt.compute(r);

    // Compute the area range summaries.
    routing_area_rangesV2(r);

    start_virtual_link();

    list<RouteCmd<Vertex> >::const_iterator ri;
    for(ri = r.begin(); ri != r.end(); ri++) {
	debug_msg("Add route: Node: %s -> Nexthop %s\n",
		  cstring(ri->node()), cstring(ri->nexthop()));
	
	Vertex node = ri->node();

	Lsa::LsaRef lsar = node.get_lsa();
	RouterLsa *rlsa;
	NetworkLsa *nlsa;
	RouteEntry<IPv4> route_entry;
	IPNet<IPv4> net;
	route_entry.set_destination_type(node.get_type());
	if (OspfTypes::Router == node.get_type()) {
	    rlsa = dynamic_cast<RouterLsa *>(lsar.get());
	    XLOG_ASSERT(rlsa);
	    check_for_virtual_link((*ri), _router_lsa);
	    if (!(rlsa->get_e_bit() || rlsa->get_b_bit()))
		continue;
	    // Originating routers Router ID.
	    route_entry.set_router_id(rlsa->get_header().get_link_state_id());
	    IPv4 addr;
	    XLOG_ASSERT(find_interface_address(ri->prevhop().get_lsa(), lsar,
					       addr));
	    net = IPNet<IPv4>(addr, IPv4::ADDR_BITLEN);
	    route_entry.set_area_border_router(rlsa->get_b_bit());
	    route_entry.set_as_boundary_router(rlsa->get_e_bit());
	} else {
	    nlsa = dynamic_cast<NetworkLsa *>(lsar.get());
	    XLOG_ASSERT(nlsa);
// 	    route_entry.set_router_id(nlsa->get_header().
// 				      get_advertising_router());
	    route_entry.set_address(nlsa->get_header().get_link_state_id());
	    IPv4 addr = IPv4(htonl(route_entry.get_address()));
	    IPv4 mask = IPv4(htonl(nlsa->get_network_mask()));
	    net = IPNet<IPv4>(addr, mask.mask_len());
	}
	// If nexthop point back to the node itself then it it
	// directly connected.
	route_entry.set_directly_connected(ri->node() == ri->nexthop());
	route_entry.set_path_type(RouteEntry<IPv4>::intra_area);
	route_entry.set_cost(ri->weight());
	route_entry.set_type_2_cost(0);

	IPv4 nexthop = IPv4(htonl(ri->nexthop().get_address()));
	route_entry.set_nexthop(nexthop);

	route_entry.set_advertising_router(lsar->get_header().
					   get_advertising_router());
	route_entry.set_area(_area);
	route_entry.set_lsa(lsar);

	routing_table.add_entry(_area, net, route_entry);
    }

    end_virtual_link();

    // RFC 2328 Section 16.2.  Calculating the inter-area routes
    if (_ospf.get_peer_manager().internal_router_p() ||
	(backbone() && _ospf.get_peer_manager().area_border_router_p()))
	routing_inter_areaV2();

    // RFC 2328 Section 16.3.  Examining transit areas' summary-LSAs
    if (transit_capability &&
	_ospf.get_peer_manager().area_border_router_p())
	routing_transit_areaV2();

    // RFC 2328 Section 16.4.  Calculating AS external routes

    routing_as_externalV2();

    routing_table.end();
}

template <>
void 
AreaRouter<IPv6>::routing_total_recomputeV2()
{
    XLOG_FATAL("OSPFv2 with IPv6 not valid");
}

template <typename A>
void 
AreaRouter<A>::routing_total_recomputeV3()
{
    XLOG_WARNING("TBD - routing computation for V3");
}

template <>
void 
AreaRouter<IPv4>::routing_area_rangesV2(list<RouteCmd<Vertex> >& r)
{
    // If there is only one area there are no other areas for which to
    // compute summaries.
    if (_ospf.get_peer_manager().internal_router_p())
 	return;

    // Do any of our intra area path fall within the summary range and
    // if they do is it an advertised range. If a network falls into
    // an advertised range then use the largest cost of the covered
    // networks.

    map<IPNet<IPv4>, RouteEntry<IPv4> > ranges;

    list<RouteCmd<Vertex> >::const_iterator ri;
    for(ri = r.begin(); ri != r.end(); ri++) {
	if (ri->node() == ri->nexthop())
	    continue;
	if (ri->node().get_type() == OspfTypes::Router)
	    continue;
	Vertex node = ri->node();
	Lsa::LsaRef lsar = node.get_lsa();
	RouteEntry<IPv4> route_entry;
	route_entry.set_destination_type(OspfTypes::Network);
	IPNet<IPv4> net;
	NetworkLsa *nlsa = dynamic_cast<NetworkLsa *>(lsar.get());
	XLOG_ASSERT(nlsa);
	route_entry.set_address(nlsa->get_header().get_link_state_id());
	IPv4 addr = IPv4(htonl(route_entry.get_address()));
	IPv4 mask = IPv4(htonl(nlsa->get_network_mask()));
	net = IPNet<IPv4>(addr, mask.mask_len());
	
	// Does this network fall into an area range?
	bool advertise;
	if (!area_range_covered(net, advertise))
	    continue;
	if (!advertise)
	    continue;
	route_entry.set_path_type(RouteEntry<IPv4>::intra_area);
	route_entry.set_cost(ri->weight());
	route_entry.set_type_2_cost(0);

	route_entry.set_nexthop(IPv4(htonl(ri->nexthop().get_address())));

	route_entry.set_advertising_router(lsar->get_header().
					   get_advertising_router());
	
	// Mark this as a discard route.
	route_entry.set_discard(true);
	if (0 != ranges.count(net)) {
	    RouteEntry<IPv4> r = ranges[net];
	    if (route_entry.get_cost() < r.get_cost())
		continue;
	} 
	ranges[net] = route_entry;
    }
 
    // Send in the discard routes.
    RoutingTable<IPv4>& routing_table = _ospf.get_routing_table();
    map<IPNet<IPv4>, RouteEntry<IPv4> >::const_iterator i;
    for (i = ranges.begin(); i != ranges.end(); i++) {
	IPNet<IPv4> net = i->first;
	RouteEntry<IPv4> route_entry = i->second;
	routing_table.add_entry(_area, net, route_entry);
    }
}

template <>
void 
AreaRouter<IPv4>::routing_inter_areaV2()
{
    // RFC 2328 Section 16.2.  Calculating the inter-area routes
    for (size_t index = 0 ; index < _last_entry; index++) {
	Lsa::LsaRef lsar = _db[index];
	if (!lsar->valid() || lsar->maxage())
	    continue;

	SummaryNetworkLsa *snlsa;	// Type 3
	SummaryRouterLsa *srlsa;	// Type 4
	uint32_t metric = 0;
	IPv4 mask;

	if (0 != (snlsa = dynamic_cast<SummaryNetworkLsa *>(lsar.get()))) {
	    metric = snlsa->get_metric();
	    mask = IPv4(htonl(snlsa->get_network_mask()));
	}
	if (0 != (srlsa = dynamic_cast<SummaryRouterLsa *>(lsar.get()))) {
	    metric = srlsa->get_metric();
	    mask = IPv4::ALL_ONES();
	}
	if (0 == snlsa && 0 == srlsa)
	    continue;
	if (OspfTypes::LSInfinity == metric)
	    continue;

	// (2)
	if (lsar->get_self_originating())
	    continue;

	uint32_t lsid = lsar->get_header().get_link_state_id();
	IPNet<IPv4> n = IPNet<IPv4>(IPv4(htonl(lsid)), mask.mask_len());

	// (3) 
	if (snlsa) {
	    bool active;
	    if (area_range_covered(n, active)) {
		if (active)
		    continue;
	    }
	}

	// (4)
	uint32_t adv = lsar->get_header().get_advertising_router();
	IPv4 br(htonl(adv));
	RoutingTable<IPv4>& routing_table = _ospf.get_routing_table();
	RouteEntry<IPv4> rt;
	if (!routing_table.lookup_entry(br, rt))
	    continue;

	if (rt.get_advertising_router() != adv || rt.get_area() != _area)
	    continue;

	uint32_t iac = rt.get_cost() + metric;

	// (5)
	bool add_entry = false;
	bool replace_entry = false;
	RouteEntry<IPv4> rtnet;
	if (routing_table.lookup_entry(n, rtnet)) {
	    switch(rtnet.get_path_type()) {
	    case RouteEntry<IPv4>::intra_area:
		break;
	    case RouteEntry<IPv4>::inter_area:
		// XXX - Should be dealing with equal cost here.
		if (iac < rtnet.get_cost())
		    replace_entry = true;
		break;
	    case RouteEntry<IPv4>::type1:
	    case RouteEntry<IPv4>::type2:
		replace_entry = true;
		break;
	    }
	} else {
	    add_entry = true;
	}
	if (!add_entry && !replace_entry)
	    continue;

	RouteEntry<IPv4> rtentry;
	rtentry.set_destination_type(OspfTypes::Network);
	rtentry.set_address(lsid);
	rtentry.set_area(_area);
	rtentry.set_path_type(RouteEntry<IPv4>::inter_area);
	rtentry.set_cost(iac);
	rtentry.set_nexthop(rt.get_nexthop());
	rtentry.set_advertising_router(rt.get_advertising_router());

	if (add_entry)
	    routing_table.add_entry(_area, n, rtentry);
	if (replace_entry)
	    routing_table.replace_entry(_area, n, rtentry);
    }
}

template <>
void 
AreaRouter<IPv4>::routing_transit_areaV2()
{
    XLOG_WARNING("TBD");
}

template <>
void 
AreaRouter<IPv4>::routing_as_externalV2()
{
    // RFC 2328 Section 16.4.  Calculating AS external routes
    // RFC 3101 Section 2.5.   Calculating Type-7 AS external routes
    for (size_t index = 0 ; index < _last_entry; index++) {
	Lsa::LsaRef lsar = _db[index];
	if (!lsar->valid() || lsar->maxage() || lsar->get_self_originating())
	    continue;

	// Note that Type7Lsa is derived from ASExternalLsa so will
	// pass this test.
	ASExternalLsa *aselsa;
	if (0 == (aselsa = dynamic_cast<ASExternalLsa *>(lsar.get()))) {
	    continue;
	}

	if (OspfTypes::LSInfinity == aselsa->get_metric())
	    continue;
	
	IPv4 mask = IPv4(htonl(aselsa->get_network_mask()));
	uint32_t lsid = lsar->get_header().get_link_state_id();
	uint32_t adv = lsar->get_header().get_advertising_router();
	IPNet<IPv4> n = IPNet<IPv4>(IPv4(htonl(lsid)), mask.mask_len());
	
	// (3)
	RoutingTable<IPv4>& routing_table = _ospf.get_routing_table();
	RouteEntry<IPv4> rt;
 	if (!routing_table.lookup_entry_by_advertising_router(_area, adv, rt))
 	    continue;

	if (!rt.get_as_boundary_router())
	    return;

	// If a routing entry has been found it must be from this area.
	XLOG_ASSERT(rt.get_area() == _area);

	if (aselsa->type7() && 0 == n.prefix_len()) {
	    if (_ospf.get_peer_manager().area_border_router_p()) {
		if (!external_propagate_bit(lsar))
		    continue;
		if (!_summaries)
		    continue;
	    }
	}

	IPv4 forwarding = aselsa->get_forwarding_address_ipv4();
	if (IPv4(static_cast<uint32_t>(0)) == forwarding) {
	    forwarding = rt.get_nexthop();
	}

	RouteEntry<IPv4> rtf;
	if (!routing_table.longest_match_entry(forwarding, rtf))
	    continue;
	if (!rtf.get_directly_connected())
	    forwarding = rtf.get_nexthop();
	if (aselsa->external()) {
	    if (RouteEntry<IPv4>::intra_area != rtf.get_path_type() &&
		RouteEntry<IPv4>::inter_area != rtf.get_path_type())
		continue;
	}
	if (aselsa->type7()) {
	    if (RouteEntry<IPv4>::intra_area != rtf.get_path_type())
		continue;
	}
	// (4)
	uint32_t x = rtf.get_cost();	// Cost specified by
					// ASBR/forwarding address
	uint32_t y = aselsa->get_metric();

	// (5)
	RouteEntry<IPv4> rtentry;
	if (!aselsa->get_e_bit()) {	// Type 1
	    rtentry.set_path_type(RouteEntry<IPv4>::type1);
	    rtentry.set_cost(x + y);
	} else {			// Type 2
	    rtentry.set_path_type(RouteEntry<IPv4>::type2);
	    rtentry.set_cost(x);
	    rtentry.set_type_2_cost(y);
	}

	// (6)
	bool add_entry = false;
	bool replace_entry = false;
	bool identical = false;
	RouteEntry<IPv4> rtnet;
	if (routing_table.lookup_entry(n, rtnet)) {
	    switch(rtnet.get_path_type()) {
	    case RouteEntry<IPv4>::intra_area:
		break;
	    case RouteEntry<IPv4>::inter_area:
		break;
	    case RouteEntry<IPv4>::type1:
		if (RouteEntry<IPv4>::type2 == rtentry.get_path_type()) {
		    break;
		}
		if (rtentry.get_cost() < rtnet.get_cost()) {
		    replace_entry = true;
		    break;
		}
		if (rtentry.get_cost() == rtnet.get_cost())
		    identical = true;
		break;
	    case RouteEntry<IPv4>::type2:
		if (RouteEntry<IPv4>::type1 == rtentry.get_path_type()) {
		    replace_entry = true;
		    break;
		}
		if (rtentry.get_type_2_cost() < rtnet.get_type_2_cost()) {
		    replace_entry = true;
		    break;
		}
		if (rtentry.get_type_2_cost() == rtnet.get_type_2_cost())
		    identical = true;
		break;
	    }
	    // (e)
	    if (identical) {
		replace_entry = routing_compare_externals(rtnet.get_lsa(),
							  lsar);
	    }
	} else {
	    add_entry = true;
	}
	if (!add_entry && !replace_entry)
	    continue;

	rtentry.set_lsa(lsar);
	rtentry.set_destination_type(OspfTypes::Network);
	rtentry.set_address(lsid);
	rtentry.set_area(_area);
	rtentry.set_nexthop(forwarding);
	rtentry.set_advertising_router(aselsa->get_header().
				       get_advertising_router());

	if (add_entry)
	    routing_table.add_entry(_area, n, rtentry);
	if (replace_entry)
	    routing_table.replace_entry(_area, n, rtentry);
	
    }
}

template <typename A>
bool
AreaRouter<A>::routing_compare_externals(Lsa::LsaRef current,
					 Lsa::LsaRef candidate) const
{
    // RFC 3101 Section 2.5. (6) (e) Calculating Type-7 AS external routes.

    bool current_type7 = current->type7();
    bool candidate_type7 = candidate->type7();

    if (current_type7)
	current_type7 = external_propagate_bit(current);

    if (candidate_type7)
	candidate_type7 = external_propagate_bit(candidate);

    if (current_type7 == candidate_type7) {
	return candidate->get_header().get_advertising_router() >
	    current->get_header().get_advertising_router();
    }

    if (candidate_type7)
	return true;

    return false;
}

template <typename A>
bool 
AreaRouter<A>::bidirectional(RouterLink::Type rl_type,
			     const uint32_t link_state_id, 
			     const RouterLink& rl, RouterLsa *rlsa,
			     uint16_t& metric,
			     uint32_t& interface_address)
{
    XLOG_ASSERT(0 != rlsa);
    XLOG_ASSERT(rl_type == RouterLink::p2p || rl_type == RouterLink::vlink);
    XLOG_ASSERT(rl.get_type() == rl_type);

    // This is the edge from the Router-LSA to the Router-LSA.
    XLOG_ASSERT(rl.get_link_id() == rlsa->get_header().get_link_state_id());
    XLOG_ASSERT(rl.get_link_id() == 
		rlsa->get_header().get_advertising_router());

    const list<RouterLink> &rlinks = rlsa->get_router_links();
    list<RouterLink>::const_iterator l = rlinks.begin();
    for(; l != rlinks.end(); l++) {
	if (l->get_link_id() == link_state_id &&
	    l->get_type() == rl_type) {
	    metric = l->get_metric();
	    interface_address = l->get_link_data();
	    return true;
	}
    }

    return false;
}

template <typename A>
bool 
AreaRouter<A>::bidirectional(const uint32_t link_state_id, 
			     const RouterLink& rl, NetworkLsa *nlsa)
    const
{
    XLOG_ASSERT(0 != nlsa);
    XLOG_ASSERT(rl.get_type() == RouterLink::transit);

    // This is the edge from the Router-LSA to the Network-LSA.
    XLOG_ASSERT(rl.get_link_id() == nlsa->get_header().get_link_state_id());

    // Does the Network-LSA know about the Router-LSA.
    list<OspfTypes::RouterID>& routers = nlsa->get_attached_routers();
    list<OspfTypes::RouterID>::const_iterator i;
    for (i = routers.begin(); i != routers.end(); i++)
	if (link_state_id == *i)
	    return true;

    return false;
}

template <typename A>
bool 
AreaRouter<A>::bidirectional(RouterLsa *rlsa,
			     NetworkLsa *nlsa,
			     uint32_t& interface_address)
{
    XLOG_ASSERT(rlsa);
    XLOG_ASSERT(nlsa);

    const uint32_t link_state_id = nlsa->get_header().get_link_state_id();
    
    const list<RouterLink> &rlinks = rlsa->get_router_links();
    list<RouterLink>::const_iterator l = rlinks.begin();
    for(; l != rlinks.end(); l++) {
	if (l->get_link_id() == link_state_id &&
	    l->get_type() == RouterLink::transit) {
	    interface_address = l->get_link_data();
	    return true;
	}
    }

    return false;
}

/**
 * XXX
 * This is temporary until SPT supports a single call to update an
 * edge adding the edge if necessary.
 *
 */
template <typename A>
inline
void
update_edge(Spt<A>& spt, const Vertex& src, int metric, const Vertex& dst)
{
    debug_msg("src %s metric %d dst %s\n", cstring(src), metric, cstring(dst));

    if (!spt.add_edge(src, metric, dst)) {
	// XXX
	// This warning should not appear during absolute calculation.
	// It may be normal for it to appear when doing incremental
	// updates when it should be removed.
	XLOG_WARNING("Edge exists between %s and %s", cstring(src),
		     cstring(dst));
	if (!spt.update_edge_weight(src, metric, dst))
	    XLOG_FATAL("Couldn't update edge between %s and %s",
		       cstring(src), cstring(dst));
    }
}

template <typename A>
void
AreaRouter<A>::routing_router_lsaV2(Spt<Vertex>& spt, const Vertex& src,
				    RouterLsa *rlsa)
{
    debug_msg("Vertex %s \n%s\n", cstring(src), cstring(*rlsa));

    const list<RouterLink> &rl = rlsa->get_router_links();
    list<RouterLink>::const_iterator l = rl.begin();
    for(; l != rl.end(); l++) {
// 	fprintf(stderr, "RouterLink %s\n", cstring(*l));
	switch(l->get_type()) {
	case RouterLink::p2p:
	case RouterLink::vlink:
	    routing_router_link_p2p_vlinkV2(spt, src, rlsa, *l);
	    break;
	case RouterLink::transit:
	    routing_router_link_transitV2(spt, src, rlsa, *l);
	    break;
	case RouterLink::stub:
	    routing_router_link_stubV2(spt, src, rlsa, *l);
	    break;
	}
    }
}

template <typename A>
void
AreaRouter<A>::routing_router_link_p2p_vlinkV2(Spt<Vertex>& spt,
					       const Vertex& src,
					       RouterLsa *rlsa, RouterLink rl)
{
    // Try and find the router link that this one points at.
    Ls_request lsr(_ospf.get_version(),
		   RouterLsa(_ospf.get_version()).get_header().get_ls_type(),
		   rl.get_link_id(),
		   rl.get_link_id());

    size_t index;
    if (find_lsa(lsr, index)) {
	Lsa::LsaRef lsapeer = _db[index];
	// This can probably never happen
	if (lsapeer->maxage()) {
	    XLOG_WARNING("LSA in database MaxAge\n%s",
			 cstring(*lsapeer));
	    return;
	}
	// Check that this Router-LSA points back to the
	// original.
	uint16_t metric;
	uint32_t interface_address;
	if (!bidirectional(rl.get_type(),
			   rlsa->get_header().get_link_state_id(),
			   rl,
			   dynamic_cast<RouterLsa *>(lsapeer.get()),
			   metric,
			   interface_address)) {
	    return;
	}
	// The destination node may not exist if it doesn't
	// create it. Half the time it will exist
	Vertex dst;
	dst.set_version(_ospf.get_version());
	dst.set_type(OspfTypes::Router);
	dst.set_nodeid(lsapeer->get_header().get_link_state_id());
	dst.set_lsa(lsapeer);
	// If the src is the origin then set the address of the
	// dest. This is the nexthop address from the origin.
	if (src.get_origin()) {
	    dst.set_address(interface_address);
	}
	if (!spt.exists_node(dst)) {
	    spt.add_node(dst);
	}
	update_edge(spt, src, rl.get_metric(), dst);
	update_edge(spt, dst, metric, src);
    }
}

template <typename A>
void
AreaRouter<A>::routing_router_link_transitV2(Spt<Vertex>& spt,
					     const Vertex& src,
					     RouterLsa *rlsa,
					     RouterLink rl)
{
    size_t index;
    if (!find_network_lsa(rl.get_link_id(), index)) {
	return;
    }

    Lsa::LsaRef lsan = _db[index];
    // This can probably never happen
    if (lsan->maxage()) {
	XLOG_WARNING("LSA in database MaxAge\n%s",
		     cstring(*lsan));
	return;
    }
    // Both nodes exist check for
    // bi-directional connectivity.
    NetworkLsa *nlsa = dynamic_cast<NetworkLsa *>(lsan.get());
    XLOG_ASSERT(nlsa);
    if (!bidirectional(rlsa->get_header().get_link_state_id(), rl, nlsa)) {
	return;
    }

    uint32_t nlsid = lsan->get_header().get_link_state_id();

    // Put both links back. If the network
    // vertex is not in the SPT put it in.
    // Create a destination node.
    Vertex dst;
    dst.set_version(_ospf.get_version());
    dst.set_type(OspfTypes::Network);
    dst.set_nodeid(nlsid);
    dst.set_lsa(lsan);
		
    // If the src is the origin then set the address of the
    // dest. This is the nexthop address from the origin.
    if (src.get_origin()) {
 	dst.set_address(nlsid);
    }

    if (!spt.exists_node(dst)) {
	spt.add_node(dst);
    }

    uint32_t rlsid = rlsa->get_header().get_link_state_id();
    bool dr = rlsid == nlsa->get_header().get_advertising_router();

    update_edge(spt, src, rl.get_metric(), dst);
    // Reverse edge
    update_edge(spt, dst, 0, src);

    if (!src.get_origin() || !dr) {
	return;
    }

    // If we reached here then this router originated the Network-LSA
    // and is hence the designated router.

    // Make an edge to all the members of the Network-LSA that are
    // bidirectionally connected. If these extra edges are not made
    // all nexthops will be the Network-LSA which in the end is the
    // router itself.

    // Even though the extra edges have been made an edge to the
    // Network-LSA was also made. If this edge did not exist it would
    // appear that the network represented by the Network-LSA was not
    // directly connected because the route to it would be via one of
    // the neighbours. Therefore the original edge is left in, if for
    // some reason the edge to the Network-LSA starts to win over the
    // direct edge to a neighbour, then don't put an edge pointing to
    // the Network-LSA and put a marker into the vertex itself
    // stopping any nodes with the marker being installed in the
    // routing table. The comparator in the vertex is biased towards
    // router vertexs over network vertexs, so there should not be a
    // problem

    XLOG_ASSERT(src.get_origin());
    XLOG_ASSERT(dr);

    const uint16_t router_ls_type = 
	RouterLsa(_ospf.get_version()).get_header().get_ls_type();
    list<OspfTypes::RouterID>& attached_routers = nlsa->get_attached_routers();
    list<OspfTypes::RouterID>::iterator i;
    for (i = attached_routers.begin(); i != attached_routers.end(); i++) {
	// Don't make an edge back to this router.
	if (*i == rlsid)
	    continue;
	// Find the Router-LSA that points back to this Network-LSA.
	Ls_request lsr(_ospf.get_version(), router_ls_type, *i, *i);
	size_t index;
	if (find_lsa(lsr, index)) {
	    Lsa::LsaRef lsapeer = _db[index];
	    // This can probably never happen
	    if (lsapeer->maxage()) {
		XLOG_WARNING("LSA in database MaxAge\n%s", cstring(*lsapeer));
		return;
	    }

	    uint32_t interface_address;
	    if (!bidirectional(dynamic_cast<RouterLsa *>(lsapeer.get()), nlsa,
			       interface_address))
		continue;

	    // Router-LSA <=> Network-LSA <=> Router-LSA.
	    // There is bidirectional connectivity from the original
	    // Router-LSA through to this one found in the
	    // Network-LSA, make an edge.
	    
	    // The destination node may not exist if it doesn't
	    // create it. Half the time it will exist
	    Vertex dst;
	    dst.set_version(_ospf.get_version());
	    dst.set_type(OspfTypes::Router);
	    dst.set_nodeid(lsapeer->get_header().get_link_state_id());
	    dst.set_lsa(lsapeer);
	    // If the src is the origin then set the address of the
	    // dest. This is the nexthop address from the origin.
	    if (src.get_origin()) {
		dst.set_address(interface_address);
	    }
	    if (!spt.exists_node(dst)) {
		spt.add_node(dst);
	    }
	    update_edge(spt, src, rl.get_metric(), dst);
	}
    }
}

template <typename A>
void
AreaRouter<A>::routing_router_link_stubV2(Spt<Vertex>& spt,
					   const Vertex& src,
					   RouterLsa *rlsa,
					   RouterLink rl)
{
    Vertex dst;
    dst.set_version(_ospf.get_version());
    dst.set_type(OspfTypes::Network);
    dst.set_nodeid(rl.get_link_id());
    // XXX Temporarily
    // Create a Network LSA to satisfy the routing calculation
    NetworkLsa *nlsa = new NetworkLsa(_ospf.get_version());
    nlsa->get_header().set_link_state_id(rl.get_link_id());
    nlsa->get_header().set_advertising_router(rlsa->get_header().
					      get_link_state_id());
    nlsa->set_network_mask(rl.get_link_data());
    Lsa::LsaRef lsan = Lsa::LsaRef(nlsa);
    // 
    dst.set_lsa(lsan);
    //
    if (!spt.exists_node(dst)) {
	spt.add_node(dst);
    }
    spt.add_edge(src, rl.get_metric(), dst);
}

template <typename A>
void
AreaRouter<A>::routing_router_lsaV3(Spt<Vertex>& spt, const Vertex& v,
				    RouterLsa *rlsa)
{
    debug_msg("Spt %s Vertex %s \n%s\n", cstring(spt), cstring(v),
	      cstring(*rlsa));

    XLOG_WARNING("TBD - add to SPT");
}
/*************************************************************************/

#ifdef	UNFINISHED_INCREMENTAL_UPDATE
template <typename A>
void
AreaRouter<A>::routing_begin()
{
    XLOG_ASSERT(_new_lsas.empty());

#ifdef  PARANOIA
    list<RouteCmd<Vertex> > r;
    _spt.compute(r);
    XLOG_ASSERT(r.empty());
#endif
    // Put back this routers interfaces.
    routing_add(_router_lsa, true);
}

template <typename A>
void
AreaRouter<A>::routing_add(Lsa::LsaRef lsar, bool known)
{
    debug_msg("%s\n", cstring(*lsar));

    // XXX - This lookup is currently expensive after TODO 28 it will
    // be fine.
    size_t index;
    if (!find_lsa(lsar, index))
	XLOG_FATAL("This LSA must be in the database\n%s\n", cstring(*lsar));

    _new_lsas.push_back(Bucket(index, known));
}

template <typename A>
void
AreaRouter<A>::routing_delete(Lsa::LsaRef lsar)
{
    debug_msg("%s\n", cstring(*lsar));

    RouterLsa *rlsa;
    if (0 != (rlsa = dynamic_cast<RouterLsa *>(lsar.get()))) {
	if (rlsa->get_v_bit())
	    _TransitCapability--;
    }

    XLOG_WARNING("TBD \n%s", cstring(*lsar));
}

template <typename A>
void
AreaRouter<A>::routing_end()
{
    typename list<Bucket>::iterator i;

    RouterLsa *rlsa;
    NetworkLsa *nlsa;

    // First part of the routing computation only consider Router-LSAs
    i = _new_lsas.begin();
    while (i != _new_lsas.end()) {
	Lsa::LsaRef lsar = _db[i->_index];
	if (0 != (rlsa = dynamic_cast<RouterLsa *>(lsar.get()))) {

	    if (rlsa->get_v_bit())
		_TransitCapability++;
	    
	    Vertex v;

	    v.set_version(_ospf.get_version());
	    v.set_type(Vertex::Router);
	    v.set_nodeid(rlsa->get_header().get_link_state_id());

	    // XXX 
	    // Really want to remove all the links but removing a node
	    // and putting it back has the same effect.
	    if (i->_known && _spt.exists_node(v)) {
		printf("%s Remove %s\n",
		       pr_id(_ospf.get_router_id()).c_str(),
		       cstring(v));
		_spt.remove_node(v);
	    } else {
		printf("%s New %s\n",
		       pr_id(_ospf.get_router_id()).c_str(),
		       cstring(v));
	    }

	    printf("%s Add %s\n",
		   pr_id(_ospf.get_router_id()).c_str(),
		   cstring(v));
	    _spt.add_node(v);

	    switch(_ospf.get_version()) {
		case OspfTypes::V2:
		    routing_router_lsaV2(v, rlsa);
		    break;
		case OspfTypes::V3:
		    routing_router_lsaV3(v, rlsa);
		    break;
	    }

	    _new_lsas.erase(i++);
	}  else if (0 != (nlsa = dynamic_cast<NetworkLsa *>(lsar.get()))) {
	    
	    printf("%s %s\n", pr_id(_ospf.get_router_id()).c_str(),
		   cstring(*nlsa));
	    i++;
	} else {
	    i++;
	}
    }

    i = _new_lsas.begin();
    while (i != _new_lsas.end()) {
	_new_lsas.erase(i++);
    }

    list<RouteCmd<Vertex> > r;
    _spt.compute(r);

    list<RouteCmd<Vertex> >::const_iterator ri;
    for(ri = r.begin(); ri != r.end(); ri++)
	XLOG_WARNING("TBD: Add route:\n%s %s",
		     pr_id(_ospf.get_router_id()).c_str(),
		     ri->str().c_str());
}

#endif

template class AreaRouter<IPv4>;
template class AreaRouter<IPv6>;
