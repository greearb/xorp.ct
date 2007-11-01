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

#ident "$XORP: xorp/ospf/area_router.cc,v 1.287 2007/10/24 00:56:51 atanu Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

#include <map>
#include <list>
#include <set>
#include <deque>

#include "libproto/spt.hh"

#include "ospf.hh"
#include "delay_queue.hh"
#include "vertex.hh"
#include "area_router.hh"

template <typename A>
AreaRouter<A>::AreaRouter(Ospf<A>& ospf, OspfTypes::AreaID area,
			  OspfTypes::AreaType area_type) 
    : _ospf(ospf), _area(area), _area_type(area_type),
      _summaries(true), _stub_default_announce(false), _stub_default_cost(0),
      _external_flooding(false),
      _last_entry(0), _allocated_entries(0), _readers(0),
      _queue(ospf.get_eventloop(),
	     OspfTypes::MinLSInterval,
	     callback(this, &AreaRouter<A>::publish_all)),
      _lsid(1),
#ifdef	UNFINISHED_INCREMENTAL_UPDATE
      _TransitCapability(0),
#else
      _TransitCapability(false),
#endif
      _routing_recompute_delay(1),	// In seconds.
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

    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	// This is a router LSA so the link state ID is the Router ID.
	header.set_link_state_id(_ospf.get_router_id());
	break;
    case OspfTypes::V3:
	header.set_link_state_id(0);
	break;
    }
    
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
int
AreaRouter<A>::startup()
{
    generate_default_route();

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

    return (XORP_OK);
}

template <typename A>
int
AreaRouter<A>::shutdown()
{
    _ospf.get_routing_table().remove_area(_area);
    clear_database();

    return (XORP_OK);
}

template <typename A>
void
AreaRouter<A>::add_peer(OspfTypes::PeerID peerid)
{
    debug_msg("PeerID %u\n", peerid);
    // The peer starts in the down state.
    _peers[peerid] = PeerStateRef(new PeerState);
}

template <typename A>
void
AreaRouter<A>::delete_peer(OspfTypes::PeerID peerid)
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
AreaRouter<A>::peer_up(OspfTypes::PeerID peerid)
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
AreaRouter<A>::peer_down(OspfTypes::PeerID peerid)
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
void
AreaRouter<A>::area_border_router_transition(bool up)
{
    if (up) {
	generate_default_route();
    } else {
	withdraw_default_route();
    }
}

template <typename A>
void
AreaRouter<A>::change_area_router_type(OspfTypes::AreaType area_type)
{
    debug_msg("Type %s\n", pp_area_type(area_type).c_str());

    _area_type = area_type;

    // Remove this routers Router-LSA from the database.
    size_t index;
    if (!find_lsa(_router_lsa, index))
	XLOG_FATAL("Couldn't find this router's Router-LSA in database %s\n",
		   cstring(*_router_lsa));
    delete_lsa(_router_lsa, index, false /* Don't invalidate */);

    save_default_route();

    clear_database(true /* Preserve Link-LSAs OSPFv3 */);

    // Put the Router-LSA back.
    add_lsa(_router_lsa);

    restore_default_route();

    // Put the Summary-LSAs and (AS-External-LSAs or Type7-LSAs) back
    startup();
}

template <>
bool
AreaRouter<IPv4>::find_global_address(uint32_t, uint16_t,
				      LsaTempStore&,
				      IPv4&) const
{
    XLOG_FATAL("Only IPv4 not IPv6");

    return true;
}

template <>
bool
AreaRouter<IPv6>::find_global_address(uint32_t adv, uint16_t type,
				      LsaTempStore& lsa_temp_store,
				      IPv6& global_address) const
{
    // Find the global interface address of the neighbour that should be used.
    const list<IntraAreaPrefixLsa *>& nlsai = 
	lsa_temp_store.get_intra_area_prefix_lsas(adv);
    list<IPv6Prefix> nprefixes;
    associated_prefixesV3(type, 0, nlsai, nprefixes);
    list<IPv6Prefix>::const_iterator ni;
    for (ni = nprefixes.begin(); ni != nprefixes.end(); ni++) {
	if (ni->get_la_bit() && 
	    ni->get_network().prefix_len() == IPv6::ADDR_BITLEN) {
	    IPv6 addr = ni->get_network().masked_addr();
	    if (!addr.is_linklocal_unicast() && !addr.is_zero()) {
		global_address = addr;
		return true;
	    }
	}
    }

    return false;
}

template <typename A>
bool
AreaRouter<A>::configured_virtual_link() const
{
    return !_vlinks.empty();
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
    // link upcall to the peer_manager, possibly surprising it.
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
AreaRouter<A>::check_for_virtual_linkV2(const RouteCmd<Vertex>& rc,
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

template <>
void
AreaRouter<IPv4>::check_for_virtual_linkV3(const RouteCmd<Vertex>&,
					   Lsa::LsaRef,
					   LsaTempStore&)
{
}

template <>
void
AreaRouter<IPv6>::check_for_virtual_linkV3(const RouteCmd<Vertex>& rc,
					   Lsa::LsaRef r,
					   LsaTempStore& lsa_temp_store)
{
    Vertex node = rc.node();
    list<Lsa::LsaRef>& lsars = node.get_lsas();
    list<Lsa::LsaRef>::iterator l = lsars.begin();
    XLOG_ASSERT(l != lsars.end());
    Lsa::LsaRef lsar = *l++;
    RouterLsa *rlsa = dynamic_cast<RouterLsa *>(lsar.get());
    XLOG_ASSERT(rlsa);
    OspfTypes::RouterID rid = rlsa->get_header().get_advertising_router();

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

    // Find the global interface address of the neighbour that should be used.
    IPv6 neighbour_interface_address;
    if (!find_global_address(rid, rlsa->get_ls_type(), lsa_temp_store,
			     neighbour_interface_address)) {
	XLOG_TRACE(_ospf.trace()._virtual_link,
		   "No global address for virtual link endpoint %s\n",
		   pr_id(rid).c_str());
	return;
    }

    // Find this routers own interface address, use the LSA database
    // because if the global address is not being advertised there is
    // no point in trying to bring up the virtual link.
    IPv6 routers_interface_address;
    if (!find_global_address(r->get_header().get_advertising_router(),
			     rlsa->get_ls_type(), lsa_temp_store,
			     routers_interface_address)) {
	XLOG_TRACE(_ospf.trace()._virtual_link,
		   "No global address for this router\n");
	return;
    }

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
    
    // There is a special case to deal with which is not part of the
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

    for (; si != src_links.end(); si++) {
	list<RouterLink>::const_iterator di = dst_links.begin();
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
AreaRouter<IPv6>::find_interface_address(Lsa::LsaRef, Lsa::LsaRef, IPv6&) const
{
    XLOG_FATAL("Only IPv4 not IPv6");
    return false;
}

template <>
bool
AreaRouter<IPv4>::find_interface_address(OspfTypes::RouterID, uint32_t, IPv4&)
{
    XLOG_FATAL("Only IPv6 not IPv4");
    return false;
}

template <>
bool
AreaRouter<IPv6>::find_interface_address(OspfTypes::RouterID rid,
					 uint32_t interface_id,
					 IPv6& interface)
{
    XLOG_ASSERT(OspfTypes::V3 == _ospf.get_version());

    // Try and find the router link that this one points at.
    Ls_request lsr(_ospf.get_version(),
		   LinkLsa(_ospf.get_version()).get_header().get_ls_type(),
		   interface_id,
		   rid);

    size_t index;
    if (find_lsa(lsr, index)) {
	Lsa::LsaRef lsa = _db[index];
	// This can probably never happen
	if (lsa->maxage()) {
	    XLOG_WARNING("LSA in database MaxAge\n%s",
			 cstring(*lsa));
	    return false;
	}
	LinkLsa *llsa = dynamic_cast<LinkLsa *>(lsa.get());
	XLOG_ASSERT(llsa);
	interface = llsa->get_link_local_address();
	return true;
    }

    if (get_neighbour_address(rid, interface_id, interface))
	 return true;

    return false;
}

template <typename A>
bool
AreaRouter<A>::area_range_add(IPNet<A> net, bool advertise)
{
    debug_msg("Net %s advertise %s\n", cstring(net), bool_c_str(advertise));

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
    debug_msg("Net %s advertise %s\n", cstring(net), bool_c_str(advertise));

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
    debug_msg("Net %s advertise %s\n", cstring(net), bool_c_str(advertise));

    typename Trie<A, Range>::iterator i = _area_range.find(net);
    if (_area_range.end() == i)
	return false;

    advertise = i.payload()._advertise;

    return true;
}

template <typename A>
bool
AreaRouter<A>::area_range_covering(IPNet<A> net, IPNet<A>& sumnet)
{
    debug_msg("Net %s\n", cstring(net));

    typename Trie<A, Range>::iterator i = _area_range.find(net);
    if (_area_range.end() == i) {
	XLOG_WARNING("Net %s not covered", cstring(net));
	return false;
    }

    sumnet = i.key();

    return true;
}

template <typename A>
bool
AreaRouter<A>::area_range_configured()
{
    return 0 != _area_range.route_count();
}

template <typename A>
bool
AreaRouter<A>::originate_default_route(bool enable)
{
    if (_stub_default_announce == enable)
	return true;

    _stub_default_announce = enable;

    switch(_area_type) {
    case OspfTypes::NORMAL:
	return true;
	break;
    case OspfTypes::STUB:
    case OspfTypes::NSSA:
	break;
    }

    if (_stub_default_announce)
	generate_default_route();
    else
	withdraw_default_route();

    return true;
}

template <typename A>
bool
AreaRouter<A>::stub_default_cost(uint32_t cost)
{
    if (_stub_default_cost == cost)
	return true;

    _stub_default_cost = cost;

    switch(_area_type) {
    case OspfTypes::NORMAL:
	return true;
	break;
    case OspfTypes::STUB:
    case OspfTypes::NSSA:
	break;
    }

    if (_stub_default_announce)
	refresh_default_route();

    return true;
}

template <typename A>
bool
AreaRouter<A>::summaries(bool enable)
{
    if (_summaries == enable)
	return true;

    _summaries = enable;

    switch(_area_type) {
    case OspfTypes::NORMAL:
	return true;
	break;
    case OspfTypes::STUB:
    case OspfTypes::NSSA:
	break;
    }

    if (_summaries) {
	_ospf.get_peer_manager().summary_push(_area);
	return true;
    }

    save_default_route();

    // Remove all the Summary-LSAs from the database by MaxAging them.
    OspfTypes::Version version = _ospf.get_version();
    maxage_type_database(SummaryNetworkLsa(version).get_ls_type());
    maxage_type_database(SummaryRouterLsa(version).get_ls_type());

    restore_default_route();

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

template <>
void 
AreaRouter<IPv4>::unique_link_state_id(Lsa::LsaRef lsar)
{
    SummaryNetworkLsa *snlsa = dynamic_cast<SummaryNetworkLsa *>(lsar.get());
    if (0 == snlsa)	// Must be a type 4 lsa.
	return;

    size_t index;
    if (!find_lsa(lsar, index))
	return;

    Lsa::LsaRef lsar_in_db = _db[index];
    XLOG_ASSERT(lsar_in_db->get_self_originating());
    SummaryNetworkLsa *snlsa_in_db =
	dynamic_cast<SummaryNetworkLsa *>(lsar_in_db.get());
    if (0 == snlsa)
	return;
    if (snlsa->get_network_mask() == snlsa_in_db->get_network_mask())
	return;

    IPv4 mask = IPv4(htonl(snlsa->get_network_mask()));
    IPv4 mask_in_db = IPv4(htonl(snlsa_in_db->get_network_mask()));
    XLOG_ASSERT(mask != mask_in_db);

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
    // changed. Its not strictly necessary to remove the LSA from the
    // database but if we ever use maps to speed up access to the
    // database the deletion will be required as the link state ID
    // will be one of the keys.
    delete_lsa(lsar_in_db, index, false /* invalidate */);
    Lsa_header& header = lsar_in_db->get_header();
    header.set_link_state_id(set_host_bits(header.get_link_state_id(),
					   ntohl(mask_in_db.addr())));
    lsar_in_db->encode();
    add_lsa(lsar_in_db);
    refresh_summary_lsa(lsar_in_db);
}

template <>
void 
AreaRouter<IPv6>::unique_link_state_id(Lsa::LsaRef /*lsar*/)
{
}

template <>
bool
AreaRouter<IPv4>::unique_find_lsa(Lsa::LsaRef lsar, const IPNet<IPv4>& net,
				  size_t& index)
{
    if (!find_lsa(lsar, index))
	return false;

    Lsa::LsaRef lsar_in_db = _db[index];
    XLOG_ASSERT(lsar_in_db->get_self_originating());
    SummaryNetworkLsa *snlsa_in_db =
	dynamic_cast<SummaryNetworkLsa *>(lsar_in_db.get());
    if (0 == snlsa_in_db)
	return true;
    IPv4 mask_in_db = IPv4(htonl(snlsa_in_db->get_network_mask()));
    if (mask_in_db.mask_len() == net.prefix_len())
	return true;

    // The incoming LSA can't be modified so create a new LSA to
    // continue the search.
    // Don't worry about the memory it will be freed when the LSA goes
    // out of scope.
    SummaryNetworkLsa *search = new SummaryNetworkLsa(_ospf.version());
    Lsa::LsaRef searchlsar(search);
    
    // Copy across the header fields.
    searchlsar->get_header() = lsar->get_header();

    Lsa_header& header = searchlsar->get_header();
    // Set the host bits and try to find it again.
    header.set_link_state_id(set_host_bits(header.get_link_state_id(),
					   ntohl(net.netmask().addr())));

    // Recursive
    return unique_find_lsa(searchlsar, net, index);
}

template <>
bool
AreaRouter<IPv6>::unique_find_lsa(Lsa::LsaRef lsar, const IPNet<IPv6>& /*net*/,
				  size_t& index)
{
    return find_lsa(lsar, index);
}

template <>
void 
AreaRouter<IPv4>::summary_network_lsa_set_net_lsid(SummaryNetworkLsa *snlsa,
						   IPNet<IPv4> net)
{
    snlsa->set_network_mask(ntohl(net.netmask().addr()));

    Lsa_header& header = snlsa->get_header();
    header.set_link_state_id(ntohl(net.masked_addr().addr()));
}

template <>
void 
AreaRouter<IPv6>::summary_network_lsa_set_net_lsid(SummaryNetworkLsa *snlsa,
						   IPNet<IPv6> net)
{
    IPv6Prefix prefix(_ospf.get_version());
    prefix.set_network(net);
    snlsa->set_ipv6prefix(prefix);

    // Note entries in the _lsmap are never removed, this guarantees
    // for the life of OSPF that the same network to link state ID
    // mapping exists. If this is a problem on a withdraw remove the
    // entry, will need to add another argument.
    uint32_t lsid = 0;
    if (0 == _lsmap.count(net)) {
	lsid = _lsid++;
	_lsmap[net] = lsid;
    } else {
	lsid = _lsmap[net];
    }
    Lsa_header& header = snlsa->get_header();
    header.set_link_state_id(lsid);
}

template <typename A>
Lsa::LsaRef
AreaRouter<A>::summary_network_lsa(IPNet<A> net, RouteEntry<A>& rt)
{
    OspfTypes::Version version = _ospf.get_version();

    SummaryNetworkLsa *snlsa = new SummaryNetworkLsa(version);
    Lsa_header& header = snlsa->get_header();

    summary_network_lsa_set_net_lsid(snlsa, net);
    snlsa->set_metric(rt.get_cost());

    switch (version) {
    case OspfTypes::V2:
	header.set_options(get_options());
	break;
    case OspfTypes::V3:
	if (net.masked_addr().is_linklocal_unicast())
	    XLOG_WARNING("Advertising a Link-local address in %s",
			 cstring(*snlsa));
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
	_ospf.get_peer_manager().area_range_covered(area, net, advertise))
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
	if (OspfTypes::Router == rt.get_destination_type() &&
	    rt.get_as_boundary_router())
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
	// AS  boundary router's Router ID, for OSPFv3 we can choose
	// any unique value so lets leave it as the the Router ID.
	header.set_link_state_id(rt.get_router_id());
// 	header.set_advertising_router(_ospf.get_router_id());

	switch (version) {
	case OspfTypes::V2:
	    srlsa->set_network_mask(0);
	    header.set_options(get_options());
	    break;
	case OspfTypes::V3:
	    srlsa->set_destination_id(rt.get_router_id());
	    RouterLsa *rlsa = dynamic_cast<RouterLsa *>(rt.get_lsa().get());
	    SummaryRouterLsa *sr = 
		dynamic_cast<SummaryRouterLsa *>(rt.get_lsa().get());
	    uint32_t options = 0;
	    if (rlsa) {
		options = rlsa->get_options();
	    } else if(sr) {
		options = sr->get_options();
	    } else {
		XLOG_WARNING("Unexpected LSA can't get options %s",
			     cstring(rt));
	    }
	    // In OSPFv3 the options field is set to the options from
	    // the original Router-LSA in OSPFv2 as far as I can tell
	    // it should be this routers options.
	    srlsa->set_options(options);
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
	      cstring(net), cstring(rt), bool_c_str(push));

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
	if (unique_find_lsa(lsar, net, index)) {
	    // Remove it if it should no longer be announced.
	    if(!announce) {
		lsar = _db[index];
		premature_aging(lsar, index);
	    }
	    // It is already being announced so out of here.
	    return;
	}
    }

    // Check to see if its already being announced, another LSA may
    // already have caused this summary to have been announced. Not
    // absolutely clear how.
    size_t index;
    if (unique_find_lsa(lsar, net, index)) {
	XLOG_WARNING("LSA already being announced \n%s", cstring(*_db[index]));
	return;
    }

    if (!announce) {
	return;
    }

    unique_link_state_id(lsar);

    add_lsa(lsar);
    refresh_summary_lsa(lsar);
}

template <typename A>
void
AreaRouter<A>::refresh_summary_lsa(Lsa::LsaRef lsar)
{
    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    update_age_and_seqno(lsar, now);

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

    // It would be useful to rely on the announce variable to
    // determine if an LSA was previously being announced and should
    // now be withdrawn. Under normal circumstances it works fine,
    // however a reconfiguration of area ranges may solicit the
    // messages.

    // Withdraw the LSA.
    size_t index;
    if (unique_find_lsa(lsar, net, index)) {
	if (!announce)
	    XLOG_WARNING("LSA probably should not have been announced! "
			 "Area range change?\n%s", cstring(*lsar));
	// Remove it if it should no longer be announced.
	lsar = _db[index];
	premature_aging(lsar, index);
    } else {
	if (announce)
	    XLOG_WARNING("LSA not being announced! Area range change?\n%s",
			 cstring(*lsar));
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

template <typename A>
void
AreaRouter<A>::external_copy_net_nexthop(A,
					 ASExternalLsa *dst,
					 ASExternalLsa *src)
{
    dst->set_network(src->get_network(A::ZERO()));

    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	dst->set_forwarding_address(src->get_forwarding_address(A::ZERO()));
	break;
    case OspfTypes::V3:
	if (src->get_f_bit())
	    dst->
		set_forwarding_address(src->get_forwarding_address(A::ZERO()));
	break;
    }
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
	type7->set_external_route_tag(aselsa->get_external_route_tag());
    }
	break;
    case OspfTypes::V3:
	type7->set_f_bit(aselsa->get_f_bit());
	if (type7->get_f_bit())
	    type7->set_forwarding_address_ipv6(aselsa->
					       get_forwarding_address_ipv6());
	type7->set_t_bit(aselsa->get_t_bit());
	if (type7->get_t_bit())
	    type7->set_external_route_tag(aselsa->get_external_route_tag());
	break;
    }

    external_copy_net_nexthop(A::ZERO(), type7, aselsa);
    header.
	set_advertising_router(aselsa->get_header().get_advertising_router());
    type7->set_e_bit(aselsa->get_e_bit());
    type7->set_metric(aselsa->get_metric());
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
	aselsa->set_external_route_tag(type7->get_external_route_tag());
	break;
    case OspfTypes::V3:
	aselsa->set_f_bit(type7->get_f_bit());
	if (aselsa->get_f_bit())
	    aselsa->set_forwarding_address_ipv6(type7->
						get_forwarding_address_ipv6());
	aselsa->set_t_bit(type7->get_t_bit());
	if (aselsa->get_t_bit())
	    aselsa->set_external_route_tag(type7->get_external_route_tag());
	break;
    }

    external_copy_net_nexthop(A::ZERO(), aselsa, type7);
    header.
	set_advertising_router(type7->get_header().get_advertising_router());
    aselsa->set_metric(type7->get_metric());
    aselsa->set_e_bit(type7->get_e_bit());
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
AreaRouter<A>::external_announce(Lsa::LsaRef lsar, bool /*push*/, bool redist)
{
    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	XLOG_ASSERT(lsar->external());
	break;
    case OspfTypes::V3:
	XLOG_ASSERT(lsar->external() || (!lsar->known() && lsar->as_scope()));
	break;
    }

    switch(_area_type) {
    case OspfTypes::NORMAL:
	break;
    case OspfTypes::STUB:
	return;
	break;
    case OspfTypes::NSSA: {
	switch(_ospf.get_version()) {
	case OspfTypes::V2:
	    break;
	case OspfTypes::V3:
	    if (!lsar->known())
		return;
	    break;
	}
	if (!redist)
	    return;
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
    publish(OspfTypes::ALLPEERS, OspfTypes::ALLNEIGHBOURS, lsar,
	    multicast_on_peer);
}

template <typename A>
void
AreaRouter<A>::external_announce_complete()
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
	update_age_and_seqno(lsar, now);
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
    publish(OspfTypes::ALLPEERS, OspfTypes::ALLNEIGHBOURS, lsar,
	    multicast_on_peer);
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
	if (!lsar->maxage())
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
#ifndef	MAX_AGE_IN_DATABASE
    delete_lsa(lsar, index, false /* Don't invalidate */);
#endif
    publish_all(lsar);
}

template <typename A>
bool 
AreaRouter<A>::new_router_links(OspfTypes::PeerID peerid,
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
AreaRouter<A>::add_link_lsa(OspfTypes::PeerID peerid, Lsa::LsaRef lsar)
{
    debug_msg("PeerID %u %s\n", peerid, cstring(*lsar));
    XLOG_ASSERT(lsar->get_peerid() == peerid);

    add_lsa(lsar);

    update_link_lsa(peerid, lsar);

    return true;
}

template <typename A>
bool 
AreaRouter<A>::update_link_lsa(OspfTypes::PeerID peerid, Lsa::LsaRef lsar)
{
    debug_msg("PeerID %u %s\n", peerid, cstring(*lsar));
    XLOG_ASSERT(lsar->get_peerid() == peerid);

    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    update_age_and_seqno(lsar, now);
    
    lsar->get_timer() = _ospf.get_eventloop().
	new_oneoff_after(TimeVal(OspfTypes::LSRefreshTime, 0),
			 callback(this, &AreaRouter<A>::refresh_link_lsa,
				  peerid,
				  lsar));

    publish_all(lsar);

    return true;
}

template <typename A>
bool 
AreaRouter<A>::withdraw_link_lsa(OspfTypes::PeerID peerid, Lsa::LsaRef lsar)
{
    debug_msg("PeerID %u %s\n", peerid, cstring(*lsar));
    XLOG_ASSERT(lsar->get_peerid() == peerid);

    // Clear the timer, don't want this LSA coming back.
    lsar->get_timer().clear();

    size_t index;
    if (find_lsa(lsar, index))
	delete_lsa(lsar, index, false /* Don't invalidate */);
    else
	XLOG_WARNING("Link-LSA not found in database %s", cstring(*lsar));

    return true;
}

template <typename A>
void
AreaRouter<A>::refresh_link_lsa(OspfTypes::PeerID peerid, Lsa::LsaRef lsar)
{
    debug_msg("PeerID %u %s\n", peerid, cstring(*lsar));
    XLOG_ASSERT(lsar->get_peerid() == peerid);

    update_link_lsa(peerid, lsar);
}

template <typename A>
bool 
AreaRouter<A>::generate_network_lsa(OspfTypes::PeerID peerid,
				    OspfTypes::RouterID link_state_id,
				    list<RouterInfo>& routers,
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
    Lsa::LsaRef lsar = Lsa::LsaRef(nlsa);
    add_lsa(lsar);
#endif

    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	generate_intra_area_prefix_lsa(peerid, lsar, link_state_id);
	break;
    }

    update_network_lsa(peerid, link_state_id, routers, network_mask);
    
    return true;
}

template <typename A>
bool 
AreaRouter<A>::update_network_lsa(OspfTypes::PeerID peerid,
				  OspfTypes::RouterID link_state_id,
				  list<RouterInfo>& routers,
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

    // If routers is empty this is a refresh.
    if (!routers.empty()) {
	list<OspfTypes::RouterID>& attached_routers = 
	    nlsa->get_attached_routers();
	attached_routers.clear();
	attached_routers.push_back(_ospf.get_router_id());// Add this router
	list<RouterInfo>::iterator i;
	for (i = routers.begin(); i != routers.end(); i++)
	    attached_routers.push_back(i->_router_id);
    }

    switch (version) {
    case OspfTypes::V2:
	nlsa->set_network_mask(network_mask);
	nlsa->get_header().set_options(get_options());
	break;
    case OspfTypes::V3:
	uint32_t options =
	    update_intra_area_prefix_lsa(peerid,_db[index]->get_ls_type(),
					 link_state_id, routers); 
	nlsa->set_options(options);
	break;
    }

    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    update_age_and_seqno(_db[index], now);

    // Prime this Network-LSA to be refreshed.
    nlsa->get_timer() = _ospf.get_eventloop().
	new_oneoff_after(TimeVal(OspfTypes::LSRefreshTime, 0),
			 callback(this, &AreaRouter<A>::refresh_network_lsa,
				  peerid,
				  _db[index],
				  true /* timer */));

    publish_all(_db[index]);

    return true;
}

template <typename A>
bool 
AreaRouter<A>::withdraw_network_lsa(OspfTypes::PeerID peerid,
				    OspfTypes::RouterID link_state_id)
{
    debug_msg("PeerID %u link state id %s\n", peerid,
	      pr_id(link_state_id).c_str());

    OspfTypes::Version version = _ospf.get_version();
    Ls_request lsr(version, NetworkLsa(version).get_ls_type(),
		   link_state_id, _ospf.get_router_id());

    size_t index;
    if (!find_lsa(lsr, index)) {
	XLOG_WARNING("Couldn't find Network_lsa %s in LSA database"
		     " Did the Router ID change?",
		     cstring(lsr));
	return false;
    }

    Lsa::LsaRef lsar = _db[index];
    premature_aging(lsar, index);

    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	withdraw_intra_area_prefix_lsa(peerid, lsar->get_ls_type(),
				       link_state_id);
	break;
    }

    return true;
}

template <typename A>
void
AreaRouter<A>::refresh_network_lsa(OspfTypes::PeerID peerid, Lsa::LsaRef lsar,
				   bool timer)
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

    list<RouterInfo> routers;

    update_network_lsa(peerid,
		       nlsa->get_header().get_link_state_id(),
		       routers,
		       network_mask);

    if (!timer)
	routing_schedule_total_recompute();
}

inline
bool
operator==(const IPv6Prefix& lhs, const IPv6Prefix& rhs)
{
    // We could check for lhs == rhs this will be such a rare
    // occurence why bother.
    if (lhs.use_metric() != rhs.use_metric())
	return false;

    if (lhs.get_network() != rhs.get_network())
	return false;

    if (lhs.get_prefix_options() != rhs.get_prefix_options())
	return false;

    if (lhs.use_metric() && lhs.get_metric() != rhs.get_metric())
	return false;

    return true;
}

inline
bool 
operator<(const IPv6Prefix& lhs, const IPv6Prefix& rhs)
{
    if (lhs.get_network() < rhs.get_network())
	return true;

    if (lhs.get_prefix_options() < rhs.get_prefix_options())
	return true;

    if (lhs.use_metric() && lhs.get_metric() < rhs.get_metric())
	return true;

    return false;
}

inline
bool
operator==(const LinkLsa& lhs, const LinkLsa& rhs)
{
    set<IPv6Prefix> lhs_set, rhs_set;
    list<IPv6Prefix>::const_iterator i;

    const list<IPv6Prefix>& lhs_prefixes = lhs.get_prefixes();
    for (i = lhs_prefixes.begin(); i != lhs_prefixes.end(); i++)
	lhs_set.insert(*i);
    
    const list<IPv6Prefix>& rhs_prefixes = rhs.get_prefixes();
    for (i = rhs_prefixes.begin(); i != rhs_prefixes.end(); i++)
	rhs_set.insert(*i);

    return lhs_set == rhs_set;
}

template <typename A>
bool
AreaRouter<A>::check_link_lsa(LinkLsa *nllsa, LinkLsa *ollsa)
{
    XLOG_ASSERT(nllsa);

    // There is an existing Link-LSA if it is the same as the new
    // Link-LSA then there is nothing that needs to be done.
    if (ollsa) {
	if (*nllsa == *ollsa)
	    return false;
    }

    return true;
}

template <typename A>
void
AreaRouter<A>::update_intra_area_prefix_lsa(OspfTypes::PeerID peerid)
{
    PeerManager<A>& pm = _ospf.get_peer_manager();
    uint32_t interface_id = pm.get_interface_id(peerid);
    list<RouterInfo> routers;
    if (!pm.get_attached_routers(peerid, _area, routers))
	XLOG_WARNING("Unable to get attached routers");

    // Test to see that a Network-LSA and Intra-Area-Prefix-LSA have
    // already been generated. In particular during the database
    // exchange the router maybe the designated router for a peer but
    // no full adjacencies have yet been formed.
    if (routers.empty())
	return;

    // An updated Network-LSA and Intra-Area-Prefix-LSA will be sent,
    // note if the options have not changed then it is not actually
    // necessary to send a new Network-LSA.
    update_network_lsa(peerid, interface_id, routers, 0);
}

template <typename A>
bool
AreaRouter<A>::generate_intra_area_prefix_lsa(OspfTypes::PeerID peerid,
					      Lsa::LsaRef lsar,
					      uint32_t interface_id)
{
    debug_msg("PeerID %u interface id %d\n", peerid, interface_id);

    IntraAreaPrefixLsa *iaplsa = new IntraAreaPrefixLsa(_ospf.get_version());
    iaplsa->set_self_originating(true);

    Lsa_header& header = iaplsa->get_header();
    header.set_link_state_id(iaplsa->create_link_state_id(lsar->get_ls_type(),
							  interface_id));
    header.set_advertising_router(_ospf.get_router_id());

	iaplsa->set_referenced_ls_type(lsar->get_ls_type());

    OspfTypes::Version version = _ospf.get_version();
    if (RouterLsa(version).get_ls_type() == lsar->get_ls_type()) {
	iaplsa->set_referenced_link_state_id(0);
    } else if (NetworkLsa(version).get_ls_type() == lsar->get_ls_type()) {
	iaplsa->set_referenced_link_state_id(lsar->get_header().
					     get_link_state_id());
    } else {
	XLOG_FATAL("Unknown LS Type %#x %s\n", lsar->get_ls_type(),
		   cstring(*lsar));
    }

    iaplsa->set_referenced_advertising_router(lsar->get_header().
					      get_advertising_router());
    
    add_lsa(Lsa::LsaRef(iaplsa));

    return true;
}

template <typename A>
uint32_t
AreaRouter<A>::populate_prefix(OspfTypes::PeerID peerid,
			       uint32_t interface_id,
			       OspfTypes::RouterID router_id,
			       list<IPv6Prefix>& prefixes)
{
    debug_msg("PeerID %u interface id %d\n", peerid, interface_id);

    OspfTypes::Version version = _ospf.get_version();
    Ls_request lsr(version, LinkLsa(version).get_ls_type(), interface_id,
		   router_id);
    uint32_t options = 0;
    size_t index;
    if (find_lsa(lsr, index)) {
	LinkLsa *llsa = dynamic_cast<LinkLsa *>(_db[index].get());
	XLOG_ASSERT(llsa);
	options = llsa->get_options();
	const list<IPv6Prefix>& link_prefixes = llsa->get_prefixes();
	list<IPv6Prefix>::const_iterator i;
	for (i = link_prefixes.begin(); i != link_prefixes.end(); i++) {
	    IPv6Prefix prefix(version, true);
	    prefix = *i;
	    if (prefix.get_nu_bit() || prefix.get_la_bit())
		continue;
	    if (prefix.get_network().masked_addr().is_linklocal_unicast())
		continue;
	    prefix.set_metric(0);
	    list<IPv6Prefix>::iterator p;
	    for (p = prefixes.begin(); p != prefixes.end(); p++) {
		if (p->get_network() == prefix.get_network())
		    break;
	    }
	    if (p == prefixes.end()) {
		prefixes.push_back(prefix);
	    } else {
		p->set_prefix_options(p->get_prefix_options() | 
				      prefix.get_prefix_options());
	    }
	}
    }

    return options;
}

template <typename A>
uint32_t
AreaRouter<A>::update_intra_area_prefix_lsa(OspfTypes::PeerID peerid,
					    uint16_t referenced_ls_type,
					    uint32_t interface_id,
					    const list<RouterInfo>& 
					    attached_routers)
{
    debug_msg("PeerID %u interface id %d\n", peerid, interface_id);

    OspfTypes::Version version = _ospf.get_version();
    Ls_request lsr(version,
		   IntraAreaPrefixLsa(version).get_ls_type(),
		   IntraAreaPrefixLsa(version).
		   create_link_state_id(referenced_ls_type, interface_id),
		   _ospf.get_router_id());

    uint32_t options = 0;
    size_t index;
    if (!find_lsa(lsr, index)) {
	XLOG_FATAL("Couldn't find Intra-Area-Prefix-LSA %s in LSA database",
		   cstring(lsr));
	return options;
    }

    IntraAreaPrefixLsa *iaplsa =
	dynamic_cast<IntraAreaPrefixLsa *>(_db[index].get());
    XLOG_ASSERT(iaplsa);
    
    // If attached_routers is empty this is a refresh.
    if (!attached_routers.empty()) {
	list<IPv6Prefix>& prefixes = iaplsa->get_prefixes();
	prefixes.clear();
	// Find our own Link-LSA for this interface and add the addresses.
	options = populate_prefix(peerid, interface_id, _ospf.get_router_id(),
				  prefixes);
	list<RouterInfo>::const_iterator i;
	for (i = attached_routers.begin(); i!= attached_routers.end(); i++)
	    options |= populate_prefix(peerid, i->_interface_id, i->_router_id,
				       prefixes);
    }
	
    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    update_age_and_seqno(_db[index], now);

    publish_all(_db[index]);
    
    return options;
}

template <typename A>
bool 
AreaRouter<A>::withdraw_intra_area_prefix_lsa(OspfTypes::PeerID peerid,
					      uint16_t referenced_ls_type,
					      uint32_t interface_id)
{
    debug_msg("PeerID %u interface id %d\n", peerid, interface_id);

    OspfTypes::Version version = _ospf.get_version();
    Ls_request lsr(version,
		   IntraAreaPrefixLsa(version).get_ls_type(),
		   IntraAreaPrefixLsa(version).
		   create_link_state_id(referenced_ls_type, interface_id),
		   _ospf.get_router_id());

    size_t index;
    if (!find_lsa(lsr, index)) {
	XLOG_WARNING("Couldn't find Intra-Area-Prefix-LSA %s in LSA database",
		     cstring(lsr));
	return false;
    }

    Lsa::LsaRef lsar = _db[index];
    premature_aging(lsar, index);

    return true;
}

template <typename A>
void
AreaRouter<A>::refresh_intra_area_prefix_lsa(OspfTypes::PeerID /*peerid*/,
					     uint16_t /*referenced_ls_type*/,
					     uint32_t /*interface_id*/)
{
    // Not required as the Intra-Area-Prefix-LSAs totally fate share
    // with the associated Router-LSA or Network-LSA.
    XLOG_UNFINISHED();
}

template <typename A>
void
AreaRouter<A>::generate_default_route()
{
    switch(_area_type) {
    case OspfTypes::NORMAL:
	return;
	break;
    case OspfTypes::STUB:
    case OspfTypes::NSSA:
	break;
    }

    if (!_stub_default_announce)
	return;

    if (!_ospf.get_peer_manager().area_border_router_p())
	return;

    size_t index;
    if (find_default_route(index))
	return;

    SummaryNetworkLsa *snlsa = new SummaryNetworkLsa(_ospf.get_version());

    snlsa->set_self_originating(true);
    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    snlsa->record_creation_time(now);

    Lsa_header& header = snlsa->get_header();
    header.set_link_state_id(OspfTypes::DefaultDestination);
    header.set_advertising_router(_ospf.get_router_id());

    switch (_ospf.get_version()) {
    case OspfTypes::V2:
	snlsa->set_network_mask(0);
	break;
    case OspfTypes::V3:
	// The IPv6Prefix will have the default route by default.
	XLOG_ASSERT(0 == snlsa->get_ipv6prefix().get_network().prefix_len());
	break;
    }

#ifdef	MAX_AGE_IN_DATABASE
    XLOG_UNFINISHED();
#else
    add_lsa(Lsa::LsaRef(snlsa));
#endif

    // No need to set the cost it will be done in refresh_default_route().

    refresh_default_route();
}

template <typename A>
bool
AreaRouter<A>::find_default_route(size_t& index)
{
    OspfTypes::Version version = _ospf.get_version();
    Ls_request lsr(version, SummaryNetworkLsa(version).get_ls_type(),
		   OspfTypes::DefaultDestination, _ospf.get_router_id());

    if (!find_lsa(lsr, index)) {
	return false;
    }

    return true;
}

template <typename A>
void
AreaRouter<A>::save_default_route()
{
    _saved_default_route = _invalid_lsa;

    switch(_area_type) {
    case OspfTypes::NORMAL:
	return;
	break;
    case OspfTypes::STUB:
    case OspfTypes::NSSA:
	break;
    }

    if (!_stub_default_announce)
	return;

    size_t index;
    if (!find_default_route(index)) {
	return;
    }

    _saved_default_route = _db[index];
    delete_lsa(_saved_default_route, index, false /* Don't invalidate */);
}

template <typename A>
void
AreaRouter<A>::restore_default_route()
{
    switch(_area_type) {
    case OspfTypes::NORMAL:
	return;
	break;
    case OspfTypes::STUB:
    case OspfTypes::NSSA:
	break;
    }

    if (!_stub_default_announce)
	return;

    // No LSA was saved so generate a new one.
    if (!_saved_default_route->valid()) {
	generate_default_route();
	return;
    }

    // Restore the saved LSA.
    add_lsa(_saved_default_route);
    refresh_default_route();
}

template <typename A>
void
AreaRouter<A>::withdraw_default_route()
{
    size_t index;
    if (!find_default_route(index))
	return;

    premature_aging(_db[index], index);
}

template <typename A>
void
AreaRouter<A>::refresh_default_route()
{
    size_t index;
    if (!find_default_route(index)) {
	XLOG_WARNING("Couldn't find default route");
	return;
    }

    SummaryNetworkLsa *snlsa =
	dynamic_cast<SummaryNetworkLsa *>(_db[index].get());
    XLOG_ASSERT(snlsa);

    switch (_ospf.get_version()) {
    case OspfTypes::V2:
	snlsa->get_header().set_options(get_options());
	break;
    case OspfTypes::V3:
	break;
    }
    
    snlsa->set_metric(_stub_default_cost);

    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    update_age_and_seqno(_db[index], now);

    snlsa->get_timer() = _ospf.get_eventloop().
	new_oneoff_after(TimeVal(OspfTypes::LSRefreshTime, 0),
			 callback(this,
				  &AreaRouter<A>::refresh_default_route));

    publish_all(_db[index]);
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
AreaRouter<A>::receive_lsas(OspfTypes::PeerID peerid,
			    OspfTypes::NeighbourID nid,
			    list<Lsa::LsaRef>& lsas, 
			    list<Lsa_header>& direct_ack,
			    list<Lsa_header>& delayed_ack,
			    bool is_router_dr, bool is_router_bdr,
			    bool is_neighbour_dr)
{
    debug_msg("PeerID %u NeighbourID %u %s\nbackup %s dr %s\n", peerid, nid,
	      pp_lsas(lsas).c_str(),
	      bool_c_str(is_router_bdr), bool_c_str(is_neighbour_dr));

    OspfTypes::Version version = _ospf.get_version();

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

	// For OSPFv3 LSAs with Link-local scope store the incoming PeerID.
	switch(version) {
	case OspfTypes::V2:
	    break;
	case OspfTypes::V3:
	    if ((*i)->link_local_scope())
		(*i)->set_peerid(peerid);
	    break;
	}
		   
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
	    switch(_ospf.get_version()) {
	    case OspfTypes::V2:
		break;
	    case OspfTypes::V3:
		// Note unknown LSAs are captured in this test.
		if (!(*i)->link_local_scope() && !(*i)->area_scope())
		    continue;
#if	0
		if (!(*i)->known()) {
		    XLOG_INFO("%s", cstring(*(*i)));
		    (*i)->set_tracing(true);
		}
#endif
		break;
	    }
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

	// OSPFv3 only.
	// If this router is the designated router for this peer and
	// this is a Link-LSA it may be necessary for the router to
	// generate a new Intra-Area-Prefix-LSA.
	bool invoke_update_intra_area_prefix_lsa = false;
	switch(_ospf.get_version()) {
	case OspfTypes::V2:
	    break;
	case OspfTypes::V3:
	    if (!is_router_dr)
		break;
	    LinkLsa *ollsa = 0;
	    LinkLsa *nllsa = 0;
	    switch(search) {
	    case NEWER:
		ollsa = dynamic_cast<LinkLsa *>(_db[index].get());
		if (0 == ollsa)
		    break;
		/*FALLTHROUGH*/
	    case NOMATCH:
 		nllsa = dynamic_cast<LinkLsa *>((*i).get());
		if (0 == nllsa)
		    break;
		invoke_update_intra_area_prefix_lsa =
		    check_link_lsa(nllsa, ollsa);
		break;
	    case OLDER:
	    case EQUIVALENT:
		break;
	    }
	    break;
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
		if (NOMATCH == search)
		    publish_all((*i));
		else
		    publish_all(_db[index]);
		continue;
	    }

	    // (b) Flood this LSA to all of our neighbours.
	    // RFC 2328 Section 13.3. Next step in the flooding procedure

	    // If this is an AS-external-LSA send it to all areas.
	    if ((*i)->external())
		external_flood_all_areas((*i));

	    if ((*i)->type7())
		external_type7_translate((*i));

	    switch(_ospf.get_version()) {
	    case OspfTypes::V2:
		break;
	    case OspfTypes::V3:
		if (!(*i)->known() && (*i)->as_scope())
		    external_flood_all_areas((*i));
		break;
	    }

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

	    debug_msg("LSA multicast out on peer: %s "
		      "LSA from DR: %s interface in state backup: %s\n",
		      bool_c_str(multicast_on_peer),
		      bool_c_str(is_neighbour_dr),
		      bool_c_str(is_router_bdr));
	    if (!multicast_on_peer) {
		if ((is_router_bdr && is_neighbour_dr) || !is_router_bdr)
		    delayed_ack.push_back(lsah);
	    }
	    
	    // Too late to do this now, its after (a).
	    // (f) Self orignating LSAs 
	    // RFC 2328 Section 13.4. Receiving self-originated LSAs

	    // OSPFv3 only.
	    // If this router is the designated router for this peer and
	    // this is a Link-LSA it may be necessary for the router to
	    // generate a new Intra-Area-Prefix-LSA.
	    switch(_ospf.get_version()) {
	    case OspfTypes::V2:
		break;
	    case OspfTypes::V3:
		if (invoke_update_intra_area_prefix_lsa)
		    update_intra_area_prefix_lsa(peerid);
		break;
	    }
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
		if (is_router_bdr && is_neighbour_dr)
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
    if (!lsar->maxage())
	lsar->set_maxage();
    maxage_reached(lsar, i);
}

template <typename A>
void
AreaRouter<A>::increment_sequence_number(Lsa::LsaRef lsar)
{
    XLOG_ASSERT(lsar->get_self_originating());

    if (lsar->max_sequence_number()) {
	max_sequence_number_reached(lsar);
	return;
    }

    lsar->increment_sequence_number();
}

template <typename A>
void
AreaRouter<A>::update_age_and_seqno(Lsa::LsaRef lsar, const TimeVal& now)
{
    XLOG_ASSERT(lsar->get_self_originating());

    if (lsar->max_sequence_number()) {
	max_sequence_number_reached(lsar);
	return;
    }

    lsar->update_age_and_seqno(now);
}

template <typename A>
void
AreaRouter<A>::max_sequence_number_reached(Lsa::LsaRef lsar)
{
    XLOG_ASSERT(lsar->get_self_originating());

    // Under normal circumstances this code path will be reached
    // every 680 years.
    XLOG_INFO("LSA reached MaxSequenceNumber %s", cstring(*lsar));

    if (!lsar->maxage())
	lsar->set_maxage();

    if (_reincarnate.empty())
	_reincarnate_timer = _ospf.get_eventloop().
	    new_periodic_ms(1000, callback(this, &AreaRouter<A>::reincarnate));
	
    _reincarnate.push_back(lsar);
}

template <typename A>
bool
AreaRouter<A>::reincarnate()
{
    list<Lsa::LsaRef>::iterator i = _reincarnate.begin();
    while(i != _reincarnate.end()) {
	XLOG_ASSERT((*i)->valid());
	XLOG_ASSERT((*i)->maxage());
	XLOG_ASSERT((*i)->max_sequence_number());
	if ((*i)->empty_nack()) {
	    TimeVal now;
	    _ospf.get_eventloop().current_time(now);
	    (*i)->revive(now);
	    XLOG_INFO("Reviving an LSA that reached MaxSequenceNumber %s",
		      cstring(*(*i)));
	    publish_all((*i));
	    _reincarnate.erase(i++);
	} else {
	    i++;
	}
    }

    if (_reincarnate.empty())
	return false;

    return true;
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

template <typename A>
bool
AreaRouter<A>::find_router_lsa(uint32_t advertising_router,
			       size_t& index) const
{
    XLOG_ASSERT(OspfTypes::V3 == _ospf.get_version());

    uint32_t ls_type = RouterLsa(_ospf.get_version()).get_ls_type();

    // The index is set by the caller.
    for(; index < _last_entry; index++) {
	if (!_db[index]->valid())
	    continue;
	Lsa_header& dblsah = _db[index]->get_header();
	if (dblsah.get_ls_type() != ls_type)
	    continue;

	if (dblsah.get_advertising_router() != advertising_router)
	    continue;

	// Note we deliberately don't check for the Link State ID.

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
AreaRouter<A>::open_database(OspfTypes::PeerID peerid, bool& empty)
{
    // While there are readers no entries can be add to the database
    // before the _last_entry.

    _readers++;

    // Snapshot the current last entry position. While the database is
    // open (_readers > 0) new entries will be added past _last_entry.

    DataBaseHandle dbh = DataBaseHandle(true, _last_entry, peerid);

    empty = !subsequent(dbh);

    return dbh;
}

template <typename A>
bool
AreaRouter<A>::valid_entry_database(OspfTypes::PeerID peerid, size_t index)
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

    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	if (lsar->link_local_scope() && lsar->get_peerid() != peerid)
	    return false;
	break;
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
	if (!valid_entry_database(dbh.get_peerid(), index))
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
    } while(!valid_entry_database(dbh.get_peerid(), position));

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
AreaRouter<A>::clear_database(bool preserve_link_lsas)
{
    for (size_t index = 0 ; index < _last_entry; index++) {
	if (!_db[index]->valid())
	    continue;
	if (_db[index]->external()) {
	    _db[index] = _invalid_lsa;
	    continue;
	}
	switch (_ospf.get_version()) {
	case OspfTypes::V2:
	    break;
	case OspfTypes::V3:
	    if (preserve_link_lsas && _db[index]->get_self_originating() &&
		dynamic_cast<LinkLsa *>(_db[index].get()))
		continue;
	    break;
	}
	_db[index]->invalidate();
    }
}

template <typename A>
void
AreaRouter<A>::maxage_type_database(uint16_t type)
{
    for (size_t index = 0 ; index < _last_entry; index++) {
	if (!_db[index]->valid())
	    continue;
	if (!_db[index]->get_self_originating())
	    continue;
	if (_db[index]->get_ls_type() != type)
	    continue;
	premature_aging(_db[index], index);
    }
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
    switch(_area_type) {
    case OspfTypes::NORMAL:
	router_lsa->set_e_bit(pm.as_boundary_router_p());
	break;
    case OspfTypes::STUB:
    case OspfTypes::NSSA:
	router_lsa->set_e_bit(false);
	break;
    }
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
    update_age_and_seqno(_router_lsa, now);

    // Prime this Router-LSA to be refreshed.
    router_lsa->get_timer() = _ospf.get_eventloop().
	new_oneoff_after(TimeVal(OspfTypes::LSRefreshTime, 0),
			 callback(this, &AreaRouter<A>::refresh_router_lsa,
				  /* timer */true));

    return true;
}

template <typename A>
void
AreaRouter<A>::refresh_router_lsa(bool timer)
{
    if (update_router_links()) {
	// publish the router LSA.
	_queue.add(_router_lsa);

	switch(_ospf.get_version()) {
	case OspfTypes::V2:
	    break;
	case OspfTypes::V3:
	    stub_networksV3(timer);
	    break;
	}

	// This new Router-LSA is being announced, hence something has
	// changed in a link or a transit capability has
	// changed. Therefore the routing table needs to be recomputed.
	if (!timer)
	    routing_schedule_total_recompute();
    }
}

template <typename A>
void
AreaRouter<A>::stub_networksV3(bool timer)
{
    debug_msg("timer %s\n", bool_c_str(timer));

    OspfTypes::Version version = _ospf.get_version();
    Ls_request lsr(version,
		   IntraAreaPrefixLsa(version).get_ls_type(),
		   IntraAreaPrefixLsa(version).
		   create_link_state_id(_router_lsa->get_ls_type(), 0),
		   _ospf.get_router_id());

    // If the timer fired find that LSA update it and retransmit.
    if (timer) {
	size_t index;
	if (!find_lsa(lsr, index))
	    return;

	TimeVal now;
	_ospf.get_eventloop().current_time(now);
	update_age_and_seqno(_db[index], now);

	_queue.add(_db[index]);

	return;
    }

    // Generate the list of prefixes that should be sent.
    list<IPv6Prefix> prefixes;
    PeerManager<A>& pm = _ospf.get_peer_manager();
    typename PeerMap::iterator i;
    for(i = _peers.begin(); i != _peers.end(); i++) {
	PeerStateRef temp_psr = i->second;
	if (temp_psr->_up) {
	    if (temp_psr->_router_links.empty()) {
		uint32_t interface_id = pm.get_interface_id(i->first);
		Ls_request lsr(version, LinkLsa(version).get_ls_type(),
			       interface_id, _ospf.get_router_id());
		size_t index;
		if (find_lsa(lsr, index)) {
		    LinkLsa *llsa = dynamic_cast<LinkLsa *>(_db[index].get());
		    XLOG_ASSERT(llsa);
		    debug_msg("%s\n", cstring(*llsa));
		    const list<IPv6Prefix>& link_prefixes =
			llsa->get_prefixes();
		    list<IPv6Prefix>::const_iterator i;
		    for (i = link_prefixes.begin(); i != link_prefixes.end(); 
			 i++){
			IPv6Prefix prefix(version, true);
			prefix = *i;
			if (prefix.get_nu_bit() /*|| prefix.get_la_bit()*/)
			    continue;
			if (prefix.get_network().masked_addr().
			    is_linklocal_unicast())
			    continue;
			prefix.set_metric(0);
			prefixes.push_back(prefix);
		    }
		}
	    } else if (configured_virtual_link()) {
		uint32_t interface_id = pm.get_interface_id(i->first);
		Ls_request lsr(version, LinkLsa(version).get_ls_type(),
			       interface_id, _ospf.get_router_id());
		size_t index;
		if (find_lsa(lsr, index)) {
		    LinkLsa *llsa = dynamic_cast<LinkLsa *>(_db[index].get());
		    XLOG_ASSERT(llsa);
		    debug_msg("%s\n", cstring(*llsa));
		    const list<IPv6Prefix>& link_prefixes =
			llsa->get_prefixes();
		    list<IPv6Prefix>::const_iterator i;
		    for (i = link_prefixes.begin(); i != link_prefixes.end(); 
			 i++){
			IPv6Prefix prefix(version, true);
			prefix = *i;
			if (!prefix.get_la_bit())
			    continue;
			if (prefix.get_network().masked_addr().
			    is_linklocal_unicast())
			    continue;
			prefix.set_metric(0);
			prefixes.push_back(prefix);
		    }
		}
	    }
	}
    }

    // If there are no prefixes to send then remove the LSA if it was
    // previously sending anything.
    if (prefixes.empty()) {
	debug_msg("No prefixes computed\n");
	size_t index;
	if (!find_lsa(lsr, index))
	    return;

	premature_aging(_db[index], index);

	return;
    }

    // If there is no Intra-Area-Prefix-LSA then generate one.
    size_t index;
    if (!find_lsa(lsr, index)) {

	if (!generate_intra_area_prefix_lsa(OspfTypes::ALLPEERS, _router_lsa,
					    0)) {
	    XLOG_WARNING("Unable to generate an Intra-Area-Prefix-LSA");
	    return;
	}

	if (!find_lsa(lsr, index))
	    XLOG_FATAL("Unable to find %s", cstring(lsr));

	Lsa::LsaRef lsar = _db[index];

	IntraAreaPrefixLsa *iaplsa = 
	    dynamic_cast<IntraAreaPrefixLsa *>(lsar.get());
	XLOG_ASSERT(iaplsa);

	list<IPv6Prefix>& nprefixes = iaplsa->get_prefixes();
	nprefixes.insert(nprefixes.begin(), prefixes.begin(), prefixes.end());

	TimeVal now;
	_ospf.get_eventloop().current_time(now);
	update_age_and_seqno(lsar, now);

	_queue.add(lsar);

	return;
    }

    // An Intra-Area-Prefix-LSA already exists if it is different to
    // the newly computed one then update the old one and publish.
    Lsa::LsaRef lsar = _db[index];
    IntraAreaPrefixLsa *iaplsa = 
	dynamic_cast<IntraAreaPrefixLsa *>(lsar.get());
    XLOG_ASSERT(iaplsa);

    list<IPv6Prefix>& oprefixes = iaplsa->get_prefixes();
    list<IPv6Prefix>::iterator j, k;
    bool found = false;
    for (j = oprefixes.begin(); j != oprefixes.end(); j++) {
	for (k = prefixes.begin(); k != prefixes.end(); k++) {
	    if (*j == *k) {
		found = true;
		break;
	    }
	}
	if (!found)
	    break;
    }
	
    // They are identical nothing to do.
    if (found)
	return;

    oprefixes.clear();
    oprefixes.insert(oprefixes.begin(), prefixes.begin(), prefixes.end());

    TimeVal now;
    _ospf.get_eventloop().current_time(now);
    update_age_and_seqno(lsar, now);

    _queue.add(lsar);
}

template <typename A>
void
AreaRouter<A>::publish(const OspfTypes::PeerID peerid,
		       const OspfTypes::NeighbourID nid,
		       Lsa::LsaRef lsar, bool &multicast_on_peer) const
{
    debug_msg("Publish: %s\n", cstring(*lsar));

    TimeVal now;
    _ospf.get_eventloop().current_time(now);

    // Update the age field unless its self originating.
    if (lsar->get_self_originating()) {
// 	if (OspfTypes::MaxSequenceNumber == lsar->get_ls_sequence_number())
// 	    XLOG_FATAL("TBD: Flush this LSA and generate a new LSA");
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
    publish(OspfTypes::ALLPEERS, OspfTypes::ALLNEIGHBOURS, lsar,
	    multicast_on_peer);

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
	pm.external_announce_complete(_area);
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
AreaRouter<A>::neighbour_at_least_two_way(OspfTypes::RouterID rid) const
{
    if (_ospf.get_testing())
	return true;

    typename PeerMap::const_iterator i;
    for(i = _peers.begin(); i != _peers.end(); i++) {
	bool twoway;
	if (_ospf.get_peer_manager().
	    neighbour_at_least_two_way(i->first, _area, rid, twoway))
	    return twoway;
    }

    return false;
}

template <typename A>
bool
AreaRouter<A>::get_neighbour_address(OspfTypes::RouterID rid,
				     uint32_t interface_id,
				     A& neighbour_address) const
{
    typename PeerMap::const_iterator i;
    for(i = _peers.begin(); i != _peers.end(); i++) {
	if (_ospf.get_peer_manager().get_neighbour_address(i->first, _area,
							   rid,
							   interface_id,
							   neighbour_address))
	    return true;
    }

    return false;
}

template <typename A>
bool
AreaRouter<A>::on_link_state_request_list(const OspfTypes::PeerID peerid,
					  const OspfTypes::NeighbourID nid,
					  Lsa::LsaRef lsar) const
{
    return _ospf.get_peer_manager().
	on_link_state_request_list(peerid, _area, nid, lsar);
}

template <typename A>
bool
AreaRouter<A>::event_bad_link_state_request(const OspfTypes::PeerID peerid,
				    const OspfTypes::NeighbourID nid) const
{
    return _ospf.get_peer_manager().
	event_bad_link_state_request(peerid, _area, nid);
}

template <typename A>
bool
AreaRouter<A>::send_lsa(const OspfTypes::PeerID peerid,
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
    XLOG_FATAL("Only IPv4 not IPv6");
    return false;
}

template <typename A>
bool
AreaRouter<A>::self_originated(Lsa::LsaRef lsar, bool lsa_exists, size_t index)
{
    // RFC 2328 Section 13.4. Receiving self-originated LSAs

    debug_msg("lsar: %s\nexists: %s index: %u\n", cstring((*lsar)),
	      bool_c_str(lsa_exists), XORP_UINT_CAST(index));
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
	increment_sequence_number(lsar);
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
    switch (_ospf.get_version()) {
    case OspfTypes::V2:
	v.set_lsa(_router_lsa);
	break;
    case OspfTypes::V3:
	v.get_lsas().push_back(_router_lsa);
	break;
    }
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
    debug_msg("%s known %s\n", cstring(*lsar), bool_c_str(known));
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
routing_area_rangesV2(const list<RouteCmd<Vertex> >& r);
template <> void AreaRouter<IPv4>::routing_inter_areaV2();
template <> void AreaRouter<IPv4>::routing_transit_areaV2();
template <> void AreaRouter<IPv4>::routing_as_externalV2();

template <> void AreaRouter<IPv6>::
routing_area_rangesV3(const list<RouteCmd<Vertex> >& r,
		      LsaTempStore& lsa_temp_store);
template <> void AreaRouter<IPv6>::routing_inter_areaV3();
template <> void AreaRouter<IPv6>::routing_transit_areaV3();
template <> void AreaRouter<IPv6>::routing_as_externalV3();

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

    // If the backbone area is configured to generate summaries and
    // the transit capability of this area just changed then all the
    // candidate summary routes need to be pushed through this area again.
    if (get_transit_capability() != transit_capability) {
	set_transit_capability(transit_capability);
	PeerManager<IPv4>& pm = _ospf.get_peer_manager();
	if (pm.area_range_configured(OspfTypes::BACKBONE))
	    pm.summary_push(_area);
    }

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
	    check_for_virtual_linkV2((*ri), _router_lsa);
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

	route_entry.set_nexthop(ri->nexthop().get_address_ipv4());

	route_entry.set_advertising_router(lsar->get_header().
					   get_advertising_router());
	route_entry.set_area(_area);
	route_entry.set_lsa(lsar);

	routing_table_add_entry(routing_table, net, route_entry);
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

    if (backbone())
	_ospf.get_peer_manager().routing_recompute_all_transit_areas();
}

template <>
void 
AreaRouter<IPv6>::routing_total_recomputeV2()
{
    XLOG_FATAL("OSPFv2 with IPv6 not valid");
}

template <>
void 
AreaRouter<IPv4>::routing_total_recomputeV3()
{
    XLOG_FATAL("OSPFv3 with IPv4 not valid");
}

/**
 * Given a list of LSAs return the one with the lowest link state ID.
 */
inline
Lsa::LsaRef
get_router_lsa_lowest(const list<Lsa::LsaRef>& lsars)
{
    // A router can have multiple Router-LSAs associated with it,
    // the options fields in all the Router-LSAs should all be the
    // same, however if they aren't the LSA with the lowest Link
    // State ID takes precedence. Unconditionally find and use the
    // Router-LSA with the lowest Link State ID.

    list<Lsa::LsaRef>::const_iterator i = lsars.begin();
    XLOG_ASSERT(i != lsars.end());
    Lsa::LsaRef lsar = *i++;
    for (; i != lsars.end(); i++)
	if ((*i)->get_header().get_link_state_id() <
	    lsar->get_header().get_link_state_id())
	    lsar = *i;

    return lsar;
}

template <>
void 
AreaRouter<IPv6>::routing_total_recomputeV3()
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

    LsaTempStore lsa_temp_store;

    for (size_t index = 0 ; index < _last_entry; index++) {
	Lsa::LsaRef lsar = _db[index];
	if (!lsar->valid() || lsar->maxage())
	    continue;

	RouterLsa *rlsa;
	
	if (0 != (rlsa = dynamic_cast<RouterLsa *>(lsar.get()))) {
 	    lsa_temp_store.add_router_lsa(lsar);

	    if (rlsa->get_v_bit())
		transit_capability = true;
	    
	    Vertex v;

	    v.set_version(_ospf.get_version());
	    v.set_type(OspfTypes::Router);
	    // In OSPFv3 a router may generate multiple Router-LSAs,
	    // use the router ID as the nodeid.
	    v.set_nodeid(rlsa->get_header().get_advertising_router());
 	    v.get_lsas().push_back(lsar);

	    // Don't add this router back again.
	    if (spt.exists_node(v)) {
		debug_msg("%s Router %s\n",
		       pr_id(_ospf.get_router_id()).c_str(),
		       cstring(v));
		if (rv == v) {
		    v.set_origin(rv.get_origin());
		} else {
//		    XLOG_WARNING("TBD: Add LSA to vertex");
// 		    v = spt.get_node(v);
// 		    v.get_lsas().push_back(lsar);
		}
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
	} else {
	    IntraAreaPrefixLsa *iaplsa;
	    if (0 != (iaplsa = dynamic_cast<IntraAreaPrefixLsa *>(lsar.get())))
		lsa_temp_store.add_intra_area_prefix_lsa(iaplsa);
	}
    }

    // If the backbone area is configured to generate summaries and
    // the transit capability of this area just changed then all the
    // candidate summary routes need to be pushed through this area again.
    if (get_transit_capability() != transit_capability) {
	set_transit_capability(transit_capability);
	PeerManager<IPv6>& pm = _ospf.get_peer_manager();
	if (pm.area_range_configured(OspfTypes::BACKBONE))
	    pm.summary_push(_area);
    }

    RoutingTable<IPv6>& routing_table = _ospf.get_routing_table();
    routing_table.begin(_area);

    // Compute the SPT.
    list<RouteCmd<Vertex> > r;
    spt.compute(r);

    // Compute the area range summaries.
    routing_area_rangesV3(r, lsa_temp_store);

    start_virtual_link();

    list<RouteCmd<Vertex> >::const_iterator ri;
    for(ri = r.begin(); ri != r.end(); ri++) {
	debug_msg("Add route: Node: %s -> Nexthop %s\n",
		  cstring(ri->node()), cstring(ri->nexthop()));
	
	Vertex node = ri->node();

	list<Lsa::LsaRef>& lsars = node.get_lsas();
	list<Lsa::LsaRef>::iterator i = lsars.begin();
	XLOG_ASSERT(i != lsars.end());
	Lsa::LsaRef lsar = *i++;
#if	0
	if (OspfTypes::Router == node.get_type()) {
	    for (; i != lsars.end(); i++)
		if ((*i)->get_header().get_link_state_id() <
		    lsar->get_header().get_link_state_id())
		    lsar = *i;
	} else {
	    XLOG_ASSERT(1 == lsars.size());
	}
#endif

	if (OspfTypes::Router == node.get_type()) {
	    lsar = get_router_lsa_lowest(lsa_temp_store.
					 get_router_lsas(node.get_nodeid()));
	    RouterLsa *rlsa = dynamic_cast<RouterLsa *>(lsar.get());
	    XLOG_ASSERT(rlsa);
 	    check_for_virtual_linkV3((*ri), _router_lsa, lsa_temp_store);
	    const list<IntraAreaPrefixLsa *>& lsai = 
		lsa_temp_store.get_intra_area_prefix_lsas(node.get_nodeid());
	    if (!lsai.empty()) {
		RouteEntry<IPv6> route_entry;
		route_entry.set_destination_type(OspfTypes::Network);
// 		route_entry.set_router_id(rlsa->get_header().
// 					  get_advertising_router());
// 		route_entry.set_directly_connected(ri->node() == 
// 						   ri->nexthop());
		route_entry.set_directly_connected(false);
		route_entry.set_path_type(RouteEntry<IPv6>::intra_area);
// 		route_entry.set_cost(ri->weight());
		route_entry.set_type_2_cost(0);

		if (IPv6::ZERO() == ri->nexthop().get_address_ipv6()) {
		    IPv6 global_address;
		    if (!find_global_address(rlsa->get_header().
					     get_advertising_router(),
					     rlsa->get_ls_type(),
					     lsa_temp_store,
					     global_address))
			continue;
		    route_entry.set_nexthop(global_address);
		    route_entry.set_nexthop_id(OspfTypes::UNUSED_INTERFACE_ID);
		} else {
		    route_entry.set_nexthop(ri->nexthop().get_address_ipv6());
		    route_entry.set_nexthop_id(ri->nexthop().get_nexthop_id());
		}
		route_entry.set_advertising_router(lsar->get_header().
						   get_advertising_router());
		route_entry.set_area(_area);
		route_entry.set_lsa(lsar);

		list<IPv6Prefix> prefixes;
		associated_prefixesV3(rlsa->get_ls_type(), 0, lsai, prefixes);
		list<IPv6Prefix>::iterator j;
		for (j = prefixes.begin(); j != prefixes.end(); j++) {
		    if (j->get_nu_bit())
			continue;
		    if (j->get_network().contains(route_entry.get_nexthop()))
			continue;
		    route_entry.set_cost(ri->weight() + j->get_metric());
		    routing_table_add_entry(routing_table, j->get_network(),
					    route_entry);
		}
	    }
	    if (rlsa->get_e_bit() || rlsa->get_b_bit()) {
		RouteEntry<IPv6> route_entry;
		route_entry.set_destination_type(OspfTypes::Router);
 		route_entry.set_router_id(rlsa->get_header().
 					  get_advertising_router());
 		route_entry.set_directly_connected(ri->node() == 
 						   ri->nexthop());
		route_entry.set_path_type(RouteEntry<IPv6>::intra_area);
 		route_entry.set_cost(ri->weight());
		route_entry.set_type_2_cost(0);

		// If this is a virtual link use the global address if
		// available.
		IPv6 router_address;
		if (IPv6::ZERO() == ri->nexthop().get_address_ipv6()) {
		    if (!find_global_address(rlsa->get_header().
					     get_advertising_router(),
					     rlsa->get_ls_type(),
					     lsa_temp_store,
					     router_address))
			continue;
		} else {
		    route_entry.set_nexthop(ri->nexthop().get_address_ipv6());
		    route_entry.set_nexthop_id(ri->nexthop().get_nexthop_id());
		    router_address = ri->nexthop().get_address_ipv6();
		}
		route_entry.set_advertising_router(lsar->get_header().
						   get_advertising_router());
		route_entry.set_area(_area);
		route_entry.set_lsa(lsar);

		route_entry.set_area_border_router(rlsa->get_b_bit());
		route_entry.set_as_boundary_router(rlsa->get_e_bit());
 		IPNet<IPv6> net(router_address,	IPv6::ADDR_BITLEN);
 		routing_table_add_entry(routing_table, net, route_entry);
// 		routing_table_add_entry(routing_table, IPNet<IPv6>(),
// 					route_entry);
	    }
	} else {
	    NetworkLsa *nlsa = dynamic_cast<NetworkLsa *>(lsar.get());
	    XLOG_ASSERT(nlsa);
	    const list<IntraAreaPrefixLsa *>& lsai = 
		lsa_temp_store.get_intra_area_prefix_lsas(node.get_nodeid());
	    if (!lsai.empty()) {
		RouteEntry<IPv6> route_entry;
		route_entry.set_destination_type(OspfTypes::Network);
// 		route_entry.set_router_id(rlsa->get_header().
// 					  get_advertising_router());
 		route_entry.set_directly_connected(ri->node() == 
						   ri->nexthop());
		route_entry.set_path_type(RouteEntry<IPv6>::intra_area);
// 		route_entry.set_cost(ri->weight());
		route_entry.set_type_2_cost(0);

		route_entry.set_nexthop(ri->nexthop().get_address_ipv6());
		route_entry.set_nexthop_id(ri->nexthop().get_nexthop_id());
		route_entry.set_advertising_router(lsar->get_header().
						   get_advertising_router());
		route_entry.set_area(_area);
		route_entry.set_lsa(lsar);

		list<IPv6Prefix> prefixes;
		associated_prefixesV3(nlsa->get_ls_type(),
				      nlsa->get_header().get_link_state_id(),
				      lsai, prefixes);
		list<IPv6Prefix>::iterator j;
		for (j = prefixes.begin(); j != prefixes.end(); j++) {
		    if (j->get_nu_bit())
			continue;
		    route_entry.set_cost(ri->weight() + j->get_metric());
		    routing_table_add_entry(routing_table, j->get_network(),
					    route_entry);
		}
	    }
	}

#if	0
	RouterLsa *rlsa;
	NetworkLsa *nlsa;

	RouteEntry<IPv6> route_entry;
	IPNet<IPv6> net;
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
	route_entry.set_path_type(RouteEntry<IPv6>::intra_area);
	route_entry.set_cost(ri->weight());
	route_entry.set_type_2_cost(0);

	route_entry.set_nexthop(ri->nexthop().get_address_ipv6());
	route_entry.set_advertising_router(lsar->get_header().
					   get_advertising_router());
	route_entry.set_area(_area);
	route_entry.set_lsa(lsar);

	routing_table_add_entry(routing_table, net, route_entry);
#endif
    }

    end_virtual_link();

    // RFC 2328 Section 16.2.  Calculating the inter-area routes
    if (_ospf.get_peer_manager().internal_router_p() ||
	(backbone() && _ospf.get_peer_manager().area_border_router_p()))
	routing_inter_areaV3();

    // RFC 2328 Section 16.3.  Examining transit areas' summary-LSAs
    if (transit_capability &&
	_ospf.get_peer_manager().area_border_router_p())
	routing_transit_areaV3();

    // RFC 2328 Section 16.4.  Calculating AS external routes

    routing_as_externalV3();

    routing_table.end();

    if (backbone())
	_ospf.get_peer_manager().routing_recompute_all_transit_areas();
}

template <typename A>
void 
AreaRouter<A>::routing_table_add_entry(RoutingTable<A>& routing_table,
				       IPNet<A> net,
				       RouteEntry<A>& route_entry)
{
    // Verify that a route is not already in the table. In the OSPFv2
    // case stub links are processed in the main processing loop
    // rather than later as the RFC suggests. Either due to
    // misconfiguration or races a Network-LSA and a stub link in
    // a Router-LSA can point to the same network. Therefore it is
    // necessary to check that a route is not already in the table.
    RouteEntry<A> current_route_entry;
    if (routing_table.lookup_entry(_area, net, current_route_entry)) {
	if (current_route_entry.get_cost() > route_entry.get_cost()) {
	    routing_table.replace_entry(_area, net, route_entry);
	} else if (current_route_entry.get_cost() ==
		   route_entry.get_cost()) {
	    if (route_entry.get_advertising_router() <
		current_route_entry.get_advertising_router())
		routing_table.replace_entry(_area, net, route_entry);
	}
    } else {
	routing_table.add_entry(_area, net, route_entry);
    }
}

template <>
void 
AreaRouter<IPv4>::routing_area_rangesV2(const list<RouteCmd<Vertex> >& r)
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
// 	if (ri->node() == ri->nexthop())
// 	    continue;
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
	IPNet<IPv4> sumnet;
	if (!area_range_covering(net, sumnet)) {
	    XLOG_FATAL("Net %s does not have a covering net", cstring(net));
	    continue;
	}
	route_entry.set_directly_connected(ri->node() == ri->nexthop());
	route_entry.set_path_type(RouteEntry<IPv4>::intra_area);
	route_entry.set_cost(ri->weight());
	route_entry.set_type_2_cost(0);

	route_entry.set_nexthop(ri->nexthop().get_address_ipv4());

	route_entry.set_advertising_router(lsar->get_header().
					   get_advertising_router());
	route_entry.set_area(_area);
	route_entry.set_lsa(lsar);

	// Mark this as a discard route.
	route_entry.set_discard(true);
	if (0 != ranges.count(sumnet)) {
	    RouteEntry<IPv4> r = ranges[sumnet];
	    if (route_entry.get_cost() < r.get_cost())
		continue;
	} 
	ranges[sumnet] = route_entry;
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
AreaRouter<IPv6>::routing_area_rangesV3(const list<RouteCmd<Vertex> >& r,
					LsaTempStore& lsa_temp_store)
{
    // If there is only one area there are no other areas for which to
    // compute summaries.
    if (_ospf.get_peer_manager().internal_router_p())
 	return;

    // Do any of our intra area path fall within the summary range and
    // if they do is it an advertised range. If a network falls into
    // an advertised range then use the largest cost of the covered
    // networks.

    map<IPNet<IPv6>, RouteEntry<IPv6> > ranges;

    list<RouteCmd<Vertex> >::const_iterator ri;
    for(ri = r.begin(); ri != r.end(); ri++) {
// 	if (ri->node() == ri->nexthop())
// 	    continue;
	if (ri->node().get_type() == OspfTypes::Router)
	    continue;
	Vertex node = ri->node();
	list<Lsa::LsaRef>& lsars = node.get_lsas();
	list<Lsa::LsaRef>::iterator l = lsars.begin();
	XLOG_ASSERT(l != lsars.end());
	Lsa::LsaRef lsar = *l++;
	RouteEntry<IPv6> route_entry;
	route_entry.set_destination_type(OspfTypes::Network);
	IPNet<IPv6> net;
	NetworkLsa *nlsa = dynamic_cast<NetworkLsa *>(lsar.get());
	XLOG_ASSERT(nlsa);
#if	0
	route_entry.set_address(nlsa->get_header().get_link_state_id());
	IPv6 addr = IPv6(htonl(route_entry.get_address()));
	IPv6 mask = IPv6(htonl(nlsa->get_network_mask()));
	net = IPNet<IPv6>(addr, mask.mask_len());
#endif
	list<IntraAreaPrefixLsa *>& lsai = lsa_temp_store.
	    get_intra_area_prefix_lsas(node.get_nodeid());
	list<IPv6Prefix> prefixes;
	associated_prefixesV3(nlsa->get_ls_type(),
			      nlsa->get_header().get_link_state_id(), lsai,
			      prefixes);
	list<IPv6Prefix>::const_iterator i;
	for (i = prefixes.begin(); i != prefixes.end(); i++) {
	    if (i->get_nu_bit())
		continue;
	    net = i->get_network();
	    // Does this network fall into an area range?
	    bool advertise;
	    if (!area_range_covered(net, advertise))
		continue;
	    if (!advertise)
		continue;
	    IPNet<IPv6> sumnet;
	    if (!area_range_covering(net, sumnet)) {
		XLOG_FATAL("Net %s does not have a covering net",
			   cstring(net));
		continue;
	    }
	    route_entry.set_directly_connected(ri->node() == ri->nexthop());
	    route_entry.set_path_type(RouteEntry<IPv6>::intra_area);
	    route_entry.set_cost(ri->weight() + i->get_metric());
	    route_entry.set_type_2_cost(0);

	    route_entry.set_nexthop(ri->nexthop().get_address_ipv6());
	    route_entry.set_nexthop_id(ri->nexthop().get_nexthop_id());

	    route_entry.set_advertising_router(lsar->get_header().
					       get_advertising_router());
	    route_entry.set_area(_area);
	    route_entry.set_lsa(lsar);

	    // Mark this as a discard route.
	    route_entry.set_discard(true);
	    if (0 != ranges.count(sumnet)) {
		RouteEntry<IPv6> r = ranges[sumnet];
		if (route_entry.get_cost() < r.get_cost())
		    continue;
	    } 
	    ranges[sumnet] = route_entry;
	}
    }
 
    // Send in the discard routes.
    RoutingTable<IPv6>& routing_table = _ospf.get_routing_table();
    map<IPNet<IPv6>, RouteEntry<IPv6> >::const_iterator i;
    for (i = ranges.begin(); i != ranges.end(); i++) {
	IPNet<IPv6> net = i->first;
	RouteEntry<IPv6> route_entry = i->second;
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
	RoutingTable<IPv4>& routing_table = _ospf.get_routing_table();
	RouteEntry<IPv4> rt;
	if (!routing_table.lookup_entry_by_advertising_router(_area, adv, rt))
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
	if (snlsa) {
	    rtentry.set_destination_type(OspfTypes::Network);
	    rtentry.set_address(lsid);
	} else if (srlsa) {
	    rtentry.set_destination_type(OspfTypes::Router);
	    rtentry.set_router_id(lsid);
	    rtentry.set_as_boundary_router(true);
	} else
	    XLOG_UNREACHABLE();
	rtentry.set_area(_area);
//	rtentry.set_directly_connected(rt.get_directly_connected());
	rtentry.set_directly_connected(false);
	rtentry.set_path_type(RouteEntry<IPv4>::inter_area);
	rtentry.set_cost(iac);
	rtentry.set_nexthop(rt.get_nexthop());
	rtentry.set_advertising_router(rt.get_advertising_router());
	rtentry.set_lsa(lsar);

	if (add_entry)
	    routing_table.add_entry(_area, n, rtentry);
	if (replace_entry)
	    routing_table.replace_entry(_area, n, rtentry);
    }
}

template <>
void 
AreaRouter<IPv6>::routing_inter_areaV3()
{
    // RFC 2328 Section 16.2.  Calculating the inter-area routes
    for (size_t index = 0 ; index < _last_entry; index++) {
	Lsa::LsaRef lsar = _db[index];
	if (!lsar->valid() || lsar->maxage())
	    continue;

	SummaryNetworkLsa *snlsa;	// Type 3
	SummaryRouterLsa *srlsa;	// Type 4
	uint32_t metric = 0;
	IPNet<IPv6> n;

	if (0 != (snlsa = dynamic_cast<SummaryNetworkLsa *>(lsar.get()))) {
	    if (snlsa->get_ipv6prefix().get_nu_bit())
		continue;
	    if (snlsa->get_ipv6prefix().get_network().masked_addr().
		is_linklocal_unicast())
		continue;
	    metric = snlsa->get_metric();
	    n = snlsa->get_ipv6prefix().get_network();
	}
	if (0 != (srlsa = dynamic_cast<SummaryRouterLsa *>(lsar.get()))) {
	    metric = srlsa->get_metric();
	}
	if (0 == snlsa && 0 == srlsa)
	    continue;
	if (OspfTypes::LSInfinity == metric)
	    continue;

	// (2)
	if (lsar->get_self_originating())
	    continue;

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
	RoutingTable<IPv6>& routing_table = _ospf.get_routing_table();
	RouteEntry<IPv6> rt;
	if (!routing_table.lookup_entry_by_advertising_router(_area, adv, rt))
	    continue;

	if (rt.get_advertising_router() != adv || rt.get_area() != _area)
	    continue;

	uint32_t iac = rt.get_cost() + metric;

	// (5)
	bool add_entry = false;
	bool replace_entry = false;
	RouteEntry<IPv6> rtnet;
	bool found = false;
	OspfTypes::RouterID dest = 0;
	if (snlsa) {
	    if (routing_table.lookup_entry(n, rtnet))
		found = true;
	} else if (srlsa) {
	    dest = srlsa->get_destination_id();
	    if (routing_table.lookup_entry_by_advertising_router(_area,
								 dest,
								 rtnet))
		found = true;
	} else
	    XLOG_UNREACHABLE();

	if (found) {
	    switch(rtnet.get_path_type()) {
	    case RouteEntry<IPv6>::intra_area:
		break;
	    case RouteEntry<IPv6>::inter_area:
		// XXX - Should be dealing with equal cost here.
		if (iac < rtnet.get_cost())
		    replace_entry = true;
		break;
	    case RouteEntry<IPv6>::type1:
	    case RouteEntry<IPv6>::type2:
		replace_entry = true;
		break;
	    }
	} else {
	    add_entry = true;
	}
	if (!add_entry && !replace_entry)
	    continue;

	RouteEntry<IPv6> rtentry;
	if (snlsa) {
	    rtentry.set_destination_type(OspfTypes::Network);
// 	    rtentry.set_address(lsid);
	} else if (srlsa) {
	    rtentry.set_destination_type(OspfTypes::Router);
	    rtentry.set_router_id(dest);
	    rtentry.set_as_boundary_router(true);
	} else
	    XLOG_UNREACHABLE();
	rtentry.set_area(_area);
//	rtentry.set_directly_connected(rt.get_directly_connected());
	rtentry.set_directly_connected(false);
	rtentry.set_path_type(RouteEntry<IPv6>::inter_area);
	rtentry.set_cost(iac);
	rtentry.set_nexthop(rt.get_nexthop());
	rtentry.set_nexthop_id(rt.get_nexthop_id());
	rtentry.set_advertising_router(rt.get_advertising_router());
	rtentry.set_lsa(lsar);

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
    // RFC 2328 Section 16.3.  Examining transit areas' summary-LSAs
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
	// Lookup this route first 
	RoutingTable<IPv4>& routing_table = _ospf.get_routing_table();
	RouteEntry<IPv4> rtnet;
	if (!routing_table.lookup_entry(n, rtnet))
	    continue;

	if (!backbone(rtnet.get_area()))
	    continue;

	bool match = true;
	switch(rtnet.get_path_type()) {
	case RouteEntry<IPv4>::intra_area:
	case RouteEntry<IPv4>::inter_area:
	    break;
	case RouteEntry<IPv4>::type1:
	case RouteEntry<IPv4>::type2:
	    match = false;
	    break;
	}

	if (!match)
	    continue;

	// (4)
	// Is the BR reachable?
	uint32_t adv = lsar->get_header().get_advertising_router();
	RouteEntry<IPv4> rtrtr;
	if (!routing_table.
	    lookup_entry_by_advertising_router(rtnet.get_area(), adv, rtrtr))
	    continue;

	uint32_t iac = rtrtr.get_cost() + metric;

	// (5)
	if (rtnet.get_cost() <= iac)
	    continue;

// 	rtnet.set_area(_area);
// 	rtnet.set_path_type(RouteEntry<IPv4>::inter_area);
	rtnet.set_cost(iac);
	rtnet.set_nexthop(rtrtr.get_nexthop());
	rtnet.set_advertising_router(rtrtr.get_advertising_router());
	rtnet.set_lsa(lsar);

	// Change the existing route, so if the original route goes
	// away the route will be removed.
	routing_table.replace_entry(rtnet.get_area(), n, rtnet);
    }
}

template <>
void 
AreaRouter<IPv6>::routing_transit_areaV3()
{
    // RFC 2328 Section 16.3.  Examining transit areas' summary-LSAs
    for (size_t index = 0 ; index < _last_entry; index++) {
	Lsa::LsaRef lsar = _db[index];
	if (!lsar->valid() || lsar->maxage())
	    continue;

	SummaryNetworkLsa *snlsa;	// Type 3
	SummaryRouterLsa *srlsa;	// Type 4
	uint32_t metric = 0;
	IPNet<IPv6> n;

	if (0 != (snlsa = dynamic_cast<SummaryNetworkLsa *>(lsar.get()))) {
	    metric = snlsa->get_metric();
	    n = snlsa->get_ipv6prefix().get_network();
	}
	if (0 != (srlsa = dynamic_cast<SummaryRouterLsa *>(lsar.get()))) {
	    metric = srlsa->get_metric();
	}
	if (0 == snlsa && 0 == srlsa)
	    continue;
	if (OspfTypes::LSInfinity == metric)
	    continue;

	// (2)
	if (lsar->get_self_originating())
	    continue;

// 	uint32_t lsid = lsar->get_header().get_link_state_id();
// 	IPNet<IPv4> n = IPNet<IPv4>(IPv4(htonl(lsid)), mask.mask_len());

	// (3)
	// Lookup this route first 
	RoutingTable<IPv6>& routing_table = _ospf.get_routing_table();
	RouteEntry<IPv6> rtnet;
	OspfTypes::RouterID dest;
	if (snlsa) {
	    if (!routing_table.lookup_entry(n, rtnet))
		continue;
	} else if (srlsa) {
	    dest = srlsa->get_destination_id();
	    if (!routing_table.lookup_entry_by_advertising_router(_area,
								 dest,
								 rtnet))
		continue;
	} else
	    XLOG_UNREACHABLE();

	if (!backbone(rtnet.get_area()))
	    continue;

	bool match = true;
	switch(rtnet.get_path_type()) {
	case RouteEntry<IPv6>::intra_area:
	case RouteEntry<IPv6>::inter_area:
	    break;
	case RouteEntry<IPv6>::type1:
	case RouteEntry<IPv6>::type2:
	    match = false;
	    break;
	}

	if (!match)
	    continue;

	// (4)
	// Is the BR reachable?
	uint32_t adv = lsar->get_header().get_advertising_router();
	RouteEntry<IPv6> rtrtr;
	if (!routing_table.
	    lookup_entry_by_advertising_router(rtnet.get_area(), adv, rtrtr))
	    continue;

	uint32_t iac = rtrtr.get_cost() + metric;

	// (5)
	if (rtnet.get_cost() <= iac)
	    continue;

// 	rtnet.set_area(_area);
// 	rtnet.set_path_type(RouteEntry<IPv6>::inter_area);
	rtnet.set_cost(iac);
	rtnet.set_nexthop(rtrtr.get_nexthop());
	rtnet.set_nexthop_id(rtrtr.get_nexthop_id());
	rtnet.set_advertising_router(rtrtr.get_advertising_router());
	rtnet.set_lsa(lsar);

	// Change the existing route, so if the original route goes
	// away the route will be removed.
	routing_table.replace_entry(rtnet.get_area(), n, rtnet);
    }
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
	
// 	IPv4 mask = IPv4(htonl(aselsa->get_network_mask()));
 	uint32_t lsid = lsar->get_header().get_link_state_id();
	uint32_t adv = lsar->get_header().get_advertising_router();
// 	IPNet<IPv4> n = IPNet<IPv4>(IPv4(htonl(lsid)), mask.mask_len());
	IPNet<IPv4> n = aselsa->get_network<IPv4>(IPv4::ZERO());
	
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
		if (!_ospf.get_rfc1583_compatibility()) {
		    if (RouteEntry<IPv4>::intra_area == rtf.get_path_type()) {
			if (!backbone(rtf.get_area()) &&
			    backbone(rtnet.get_area())) {
				replace_entry = true;
			    }
		    }
		}
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

template <>
void 
AreaRouter<IPv6>::routing_as_externalV3()
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

	if (aselsa->get_ipv6prefix().get_nu_bit())
	    continue;
	
	if (aselsa->get_ipv6prefix().get_network().masked_addr().
	    is_linklocal_unicast())
	    continue;

// 	IPv4 mask = IPv4(htonl(aselsa->get_network_mask()));
// 	uint32_t lsid = lsar->get_header().get_link_state_id();
	uint32_t adv = lsar->get_header().get_advertising_router();
// 	IPNet<IPv4> n = IPNet<IPv4>(IPv4(htonl(lsid)), mask.mask_len());
	IPNet<IPv6> n = aselsa->get_network<IPv6>(IPv6::ZERO());
	
	// (3)
	RoutingTable<IPv6>& routing_table = _ospf.get_routing_table();
	RouteEntry<IPv6> rt;
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

	IPv6 forwarding;
	uint32_t forwarding_id = OspfTypes::UNUSED_INTERFACE_ID;
	RouteEntry<IPv6> rtf;
	if (aselsa->get_f_bit()) {
	    forwarding = aselsa->get_forwarding_address_ipv6();
	    if (!routing_table.longest_match_entry(forwarding, rtf))
		continue;
	    if (!rtf.get_directly_connected()) {
		forwarding = rtf.get_nexthop();
		forwarding_id = rtf.get_nexthop_id();
	    }
	    if (aselsa->external()) {
		if (RouteEntry<IPv6>::intra_area != rtf.get_path_type() &&
		    RouteEntry<IPv6>::inter_area != rtf.get_path_type())
		    continue;
	    }
	    if (aselsa->type7()) {
		if (RouteEntry<IPv6>::intra_area != rtf.get_path_type())
		    continue;
	    }
	} else {
	    forwarding = rt.get_nexthop();
	    forwarding_id = rt.get_nexthop_id();
	    rtf = rt;
	}

	// (4)
	uint32_t x = rtf.get_cost();	// Cost specified by
					// ASBR/forwarding address
	uint32_t y = aselsa->get_metric();

	// (5)
	RouteEntry<IPv6> rtentry;
	if (!aselsa->get_e_bit()) {	// Type 1
	    rtentry.set_path_type(RouteEntry<IPv6>::type1);
	    rtentry.set_cost(x + y);
	} else {			// Type 2
	    rtentry.set_path_type(RouteEntry<IPv6>::type2);
	    rtentry.set_cost(x);
	    rtentry.set_type_2_cost(y);
	}

	// (6)
	bool add_entry = false;
	bool replace_entry = false;
	bool identical = false;
	RouteEntry<IPv6> rtnet;
	if (routing_table.lookup_entry(n, rtnet)) {
	    switch(rtnet.get_path_type()) {
	    case RouteEntry<IPv6>::intra_area:
#if	0
		if (!_ospf.get_rfc1583_compatibility()) {
		    if (RouteEntry<IPv6>::intra_area == rtf.get_path_type()) {
			if (!backbone(rtf.get_area()) &&
			    backbone(rtnet.get_area())) {
				replace_entry = true;
			    }
		    }
		}
#endif
		break;
	    case RouteEntry<IPv6>::inter_area:
		break;
	    case RouteEntry<IPv6>::type1:
		if (RouteEntry<IPv6>::type2 == rtentry.get_path_type()) {
		    break;
		}
		if (rtentry.get_cost() < rtnet.get_cost()) {
		    replace_entry = true;
		    break;
		}
		if (rtentry.get_cost() == rtnet.get_cost())
		    identical = true;
		break;
	    case RouteEntry<IPv6>::type2:
		if (RouteEntry<IPv6>::type1 == rtentry.get_path_type()) {
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
// 	rtentry.set_address(lsid);
	rtentry.set_area(_area);
	rtentry.set_nexthop(forwarding);
	rtentry.set_nexthop_id(forwarding_id);
	rtentry.set_advertising_router(aselsa->get_header().
				       get_advertising_router());

	if (add_entry)
	    routing_table.add_entry(_area, n, rtentry);
	if (replace_entry)
	    routing_table.replace_entry(_area, n, rtentry);
	
    }
}

#if	0
template <typename A>
bool 
AreaRouter<A>::associated_prefixesV3(const RouterLsa *rlsa,
				     const list<IntraAreaPrefixLsa *>& lsai,
				     list<IPv6Prefix>& prefixes)
{
    list<IntraAreaPrefixLsa *>::const_iterator i;
    for (i = lsai.begin(); i != lsai.end(); i++) {
	if ((*i)->get_referenced_ls_type() != rlsa->get_ls_type())
	    continue;
	if ((*i)->get_referenced_link_state_id() != 0) {
	    XLOG_WARNING("Referenced Link State ID "
			 "should be zero %s", cstring(*(*i)));
	    continue;
	}
	if ((*i)->get_referenced_advertising_router() != 
	    (*i)->get_header().get_advertising_router()) {
	    XLOG_WARNING("Advertising router and Referenced "
			 "Advertising router don't match %s",
			 cstring(*(*i)));
	    continue;
	}
	list<IPv6Prefix>& p = (*i)->get_prefixes();
	list<IPv6Prefix>::const_iterator j;
	for (j = p.begin(); j != p.end(); j++) {
	    prefixes.push_back(*j);
	}
    }

    return true;
}

template <typename A>
bool
AreaRouter<A>::associated_prefixesV3(NetworkLsa *nlsa,
				     const list<IntraAreaPrefixLsa *>& lsai,
				     list<IPv6Prefix>& prefixes)
{
    list<IntraAreaPrefixLsa *>::const_iterator i;
    for (i = lsai.begin(); i != lsai.end(); i++) {
	if ((*i)->get_referenced_ls_type() != nlsa->get_ls_type())
	    continue;
	if ((*i)->get_referenced_link_state_id() != 
	    nlsa->get_header().get_link_state_id()) {
	    continue;
	}
	if ((*i)->get_referenced_advertising_router() != 
	    (*i)->get_header().get_advertising_router()) {
	    XLOG_WARNING("Advertising router and Referenced "
			 "Advertising router don't match %s",
			 cstring(*(*i)));
	    continue;
	}
	list<IPv6Prefix>& p = (*i)->get_prefixes();
	list<IPv6Prefix>::const_iterator j;
	for (j = p.begin(); j != p.end(); j++) {
	    prefixes.push_back(*j);
	}
    }

    return true;
}
#endif

template <typename A>
bool
AreaRouter<A>::associated_prefixesV3(uint16_t ls_type,
				     uint32_t referenced_link_state_id,
				     const list<IntraAreaPrefixLsa *>& lsai,
				     list<IPv6Prefix>& prefixes) const
{
    list<IntraAreaPrefixLsa *>::const_iterator i;
    for (i = lsai.begin(); i != lsai.end(); i++) {
	if ((*i)->get_referenced_ls_type() != ls_type)
	    continue;
	if ((*i)->get_referenced_link_state_id() != referenced_link_state_id) {
	    if (RouterLsa(_ospf.get_version()).get_ls_type() == ls_type) {
		XLOG_ASSERT(0 == referenced_link_state_id);
		XLOG_WARNING("Referenced Link State ID "
			     "should be zero %s", cstring(*(*i)));
	    }
	    continue;
	}
	if ((*i)->get_referenced_advertising_router() != 
	    (*i)->get_header().get_advertising_router()) {
	    XLOG_WARNING("Advertising router and Referenced "
			 "Advertising router don't match %s",
			 cstring(*(*i)));
	    continue;
	}
	list<IPv6Prefix>& p = (*i)->get_prefixes();
	list<IPv6Prefix>::const_iterator j;
	for (j = p.begin(); j != p.end(); j++) {
	    prefixes.push_back(*j);
	}
    }

    return true;
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
AreaRouter<A>::bidirectionalV2(RouterLink::Type rl_type,
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
AreaRouter<A>::bidirectional(const uint32_t link_state_id_or_adv, 
			     const RouterLink& rl, NetworkLsa *nlsa)
    const
{
    XLOG_ASSERT(0 != nlsa);
    XLOG_ASSERT(rl.get_type() == RouterLink::transit);

    // This is the edge from the Router-LSA to the Network-LSA.
    switch(_ospf.get_version()) {
    case OspfTypes::V2:
	
	XLOG_ASSERT(rl.get_link_id() ==
		    nlsa->get_header().get_link_state_id());
	break;
    case OspfTypes::V3:
	XLOG_ASSERT(rl.get_neighbour_interface_id() ==
		    nlsa->get_header().get_link_state_id());
	XLOG_ASSERT(rl.get_neighbour_router_id() ==
		    nlsa->get_header().get_advertising_router());
	break;
    }

    // Does the Network-LSA know about the Router-LSA.
    list<OspfTypes::RouterID>& routers = nlsa->get_attached_routers();
    list<OspfTypes::RouterID>::const_iterator i;
    for (i = routers.begin(); i != routers.end(); i++)
	if (link_state_id_or_adv == *i)
	    return true;

    return false;
}

template <typename A>
bool 
AreaRouter<A>::bidirectionalV2(RouterLsa *rlsa,
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

template <typename A>
bool 
AreaRouter<A>::bidirectionalV3(RouterLsa *rlsa, NetworkLsa *nlsa,
			       uint32_t& interface_id)
{
    XLOG_ASSERT(rlsa);
    XLOG_ASSERT(nlsa);

    const uint32_t link_state_id = nlsa->get_header().get_link_state_id();
    const uint32_t adv = nlsa->get_header().get_advertising_router();
    
    const list<RouterLink> &rlinks = rlsa->get_router_links();
    list<RouterLink>::const_iterator l = rlinks.begin();
    for(; l != rlinks.end(); l++) {
	if (l->get_neighbour_interface_id() == link_state_id &&
	    l->get_neighbour_router_id() == adv &&
	    l->get_type() == RouterLink::transit) {
	    interface_id = l->get_interface_id();
	    return true;
	}
    }

    return false;
}

template <typename A>
bool 
AreaRouter<A>::bidirectionalV3(RouterLink::Type rl_type,
			     const uint32_t advertising_router, 
			     RouterLsa *rlsa,
			     uint16_t& metric)
{
    XLOG_ASSERT(rlsa);
    XLOG_ASSERT(rl_type == RouterLink::p2p || rl_type == RouterLink::vlink);

    const list<RouterLink> &rlinks = rlsa->get_router_links();
    list<RouterLink>::const_iterator l = rlinks.begin();
    for(; l != rlinks.end(); l++) {
	if (l->get_neighbour_router_id() == advertising_router &&
	    l->get_type() == rl_type) {
	    metric = l->get_metric();
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
	if (!bidirectionalV2(rl.get_type(),
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
	    dst.set_address(IPv4(htonl(interface_address)));
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
 	dst.set_address(IPv4(htonl(nlsid)));
    }

    if (!spt.exists_node(dst)) {
	spt.add_node(dst);
    }

    uint32_t rlsid = rlsa->get_header().get_link_state_id();
    bool dr = rlsid == nlsa->get_header().get_advertising_router();

    update_edge(spt, src, rl.get_metric(), dst);
    // Reverse edge
    update_edge(spt, dst, 0, src);

    if (!src.get_origin()) {
	return;
    }

    // We are here because this Network-LSA was either generated by
    // this router, or was generated by a router on a directly
    // connected interface.

    // It is necessary to make an edge to all the bidirectionally
    // connected routers. If we don't do this then if this router is
    // the designated router all the next hops will incorrectly point
    // at this router itself. If this is not the designated router and
    // hence did not generate the Network-LSA *all* next hops will go
    // through the designated router which is strictly correct but
    // does add an extra hop, if the designated router is not on the
    // direct path. If this is the designated router then it is safe
    // to make an edge after making the bidirectional check as this
    // router by definition has a full adjacency to all the
    // routers. If this is not the designated router then it is
    // necessary to make the 2-Way check.

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

    const uint16_t router_ls_type = 
	RouterLsa(_ospf.get_version()).get_header().get_ls_type();
    list<OspfTypes::RouterID>& attached_routers = nlsa->get_attached_routers();
    list<OspfTypes::RouterID>::iterator i;
    for (i = attached_routers.begin(); i != attached_routers.end(); i++) {
	// Don't make an edge back to this router.
	if (*i == rlsid)
	    continue;

	// If this is the designated router then we know have a full
	// adjacency with all the attached routers as this router
	// created this list. If we are not the designated router then
	// we need to check that we are at least 2-Way.
	if (!dr) {
	    if (!neighbour_at_least_two_way(*i))
		continue;
	}

	// Find the Router-LSA that points back to this Network-LSA.
	Ls_request lsr(_ospf.get_version(), router_ls_type, *i, *i);
	size_t index;
	if (find_lsa(lsr, index)) {
	    Lsa::LsaRef lsapeer = _db[index];
	    // This can probably never happen
	    if (lsapeer->maxage()) {
		XLOG_WARNING("LSA in database MaxAge\n%s", cstring(*lsapeer));
		continue;
	    }

	    uint32_t interface_address;
	    if (!bidirectionalV2(dynamic_cast<RouterLsa *>(lsapeer.get()),
				 nlsa,
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
		dst.set_address(IPv4(htonl(interface_address)));
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
    // Set the host bits to generate a unique nodeid.
    dst.set_nodeid(rl.get_link_id() | ~rl.get_link_data());
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
AreaRouter<A>::routing_router_lsaV3(Spt<Vertex>& spt, const Vertex& src,
				    RouterLsa *rlsa)
{
    debug_msg("Spt %s Vertex %s \n%s\n", cstring(spt), cstring(src),
	      cstring(*rlsa));

    const list<RouterLink> &rl = rlsa->get_router_links();
    list<RouterLink>::const_iterator l = rl.begin();
    for(; l != rl.end(); l++) {
//  	fprintf(stderr, "RouterLink %s\n", cstring(*l));
	switch(l->get_type()) {
	case RouterLink::p2p:
	case RouterLink::vlink:
 	    routing_router_link_p2p_vlinkV3(spt, src, rlsa, *l);
	    break;
	case RouterLink::transit:
 	    routing_router_link_transitV3(spt, src, rlsa, *l);
	    break;
	case RouterLink::stub:
	    XLOG_FATAL("OSPFv3 does not support type stub");
	    break;
	}
    }
}

template <typename A>
void
AreaRouter<A>::routing_router_link_p2p_vlinkV3(Spt<Vertex>& spt,
					       const Vertex& src,
					       RouterLsa *rlsa,
					       RouterLink rl)
{
    // Find the Router-LSA that is pointed at by this router link.
    // Remember that an OSPSv3 router can generate multiple
    // Router-LSAs so it is necessary to search all of them for the
    // corresponding router link.
    bool found = false;
    Lsa::LsaRef lsapeer;    
    RouterLsa *rlsapeer = 0;
    uint16_t metric;
    for(size_t index = 0;; index++) {
	if (!find_router_lsa(rl.get_neighbour_router_id(), index))
	    return;
	lsapeer = _db[index];
	// This can probably never happen
	if (lsapeer->maxage()) {
	    XLOG_WARNING("LSA in database MaxAge\n%s", cstring(*lsapeer));
	    continue;
	}
// 	fprintf(stderr, "peer %s\n", cstring(*lsapeer));
	// Check that this Router-LSA points back to the original.
	rlsapeer = dynamic_cast<RouterLsa *>(lsapeer.get());
	XLOG_ASSERT(0 != rlsapeer);
	if (bidirectionalV3(rl.get_type(),
			     rlsa->get_header().get_advertising_router(),
			     rlsapeer,
			     metric)) {
	    found = true;
	    break;
	}
    }

    if (!found)
	return;

    // The coressponding router link has been found but should it be used?
    Options options(_ospf.get_version(), rlsapeer->get_options());
    if (!options.get_v6_bit())
	return;
    if (!options.get_r_bit())
	return;

    // The destination node may not exist if it doesn't
    // create it. Half the time it will exist
    Vertex dst;
    dst.set_version(_ospf.get_version());
    dst.set_type(OspfTypes::Router);
    dst.set_nodeid(lsapeer->get_header().get_advertising_router());
    dst.get_lsas().push_back(lsapeer);
    // If the src is the origin then set the address of the
    // dest. This is the nexthop address from the origin.
    if (src.get_origin()) {
	if (RouterLink::p2p == rl.get_type()) {
	    // Find the nexthop address from the router's Link-LSA. If the
	    // nexthop can't be found then there is no point putting
	    // this router into the graph. If this is a directly adjacent
	    // Virtual link then still use the Link-local address.
	    A interface_address;
	    if (!find_interface_address(rl.get_neighbour_router_id(),
					rl.get_neighbour_interface_id(),
					interface_address))
		return;
	    dst.set_address(interface_address);
	    dst.set_nexthop_id(rl.get_interface_id());
	} else if (RouterLink::vlink == rl.get_type()) {
	    dst.set_address(IPv6::ZERO());
	    dst.set_nexthop_id(OspfTypes::UNUSED_INTERFACE_ID);
	} else {
	    XLOG_FATAL("Unexpected router link %s", cstring(rl));
	}
    }
    if (!spt.exists_node(dst)) {
	spt.add_node(dst);
    }
    update_edge(spt, src, rl.get_metric(), dst);
    update_edge(spt, dst, metric, src);
}

template <typename A>
void
AreaRouter<A>::routing_router_link_transitV3(Spt<Vertex>& spt,
					     const Vertex& src,
					     RouterLsa *rlsa,
					     RouterLink rl)
{
    OspfTypes::Version version = _ospf.get_version();

    // Find the Network-LSA that this router link points at.
    Ls_request lsr(version,
		   NetworkLsa(version).get_header().get_ls_type(),
		   rl.get_neighbour_interface_id(),
		   rl.get_neighbour_router_id());
    size_t index;
    if (!find_lsa(lsr, index)) {
	return;
    }

    Lsa::LsaRef lsan = _db[index];
    // This can probably never happen
    if (lsan->maxage()) {
	XLOG_WARNING("LSA in database MaxAge\n%s", cstring(*lsan));
	return;
    }

    // Both nodes exist check for
    // bi-directional connectivity.
    NetworkLsa *nlsa = dynamic_cast<NetworkLsa *>(lsan.get());
    XLOG_ASSERT(nlsa);
    if (!bidirectional(rlsa->get_header().get_advertising_router(), rl,
		       nlsa)) {
	return;
    }

    // Put both links back. If the network
    // vertex is not in the SPT put it in.
    // Create a destination node.
    Vertex dst;
    dst.set_version(_ospf.get_version());
    dst.set_type(OspfTypes::Network);
    dst.set_nodeid(lsan->get_header().get_advertising_router());
    dst.set_interface_id(lsan->get_header().get_link_state_id());
    dst.get_lsas().push_back(lsan);

    // If the src is the origin then set the address of the
    // dest. This is the nexthop address from the origin.
    if (src.get_origin()) {
	// Find the nexthop address from the router's Link-LSA. If the
	// nexthop can't be found then there is no point putting
	// this router into the graph.
	A interface_address;
	if (!find_interface_address(rl.get_neighbour_router_id(),
				    rl.get_neighbour_interface_id(),
				    interface_address))
	    return;
  	dst.set_address(interface_address);
	dst.set_nexthop_id(rl.get_interface_id());
    }

    if (!spt.exists_node(dst)) {
	spt.add_node(dst);
    }

    uint32_t rladv = rlsa->get_header().get_advertising_router();
    bool dr = rladv == nlsa->get_header().get_advertising_router();

    update_edge(spt, src, rl.get_metric(), dst);
    // Reverse edge
    update_edge(spt, dst, 0, src);

    if (!src.get_origin()) {
	return;
    }

    // We are here because this Network-LSA was either generated by
    // this router, or was generated by a router on a directly
    // connected interface.

    // It is necessary to make an edge to all the bidirectionally
    // connected routers. If we don't do this then if this router is
    // the designated router all the next hops will incorrectly point
    // at this router itself. If this is not the designated router and
    // hence did not generate the Network-LSA *all* next hops will go
    // through the designated router which is strictly correct but
    // does add an extra hop, if the designated router is not on the
    // direct path. If this is the designated router then it is safe
    // to make an edge after making the bidirectional check as this
    // router by definition has a full adjacency to all the
    // routers. If this is not the designated router then it is
    // necessary to make the 2-Way check.

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

    list<OspfTypes::RouterID>& attached_routers = nlsa->get_attached_routers();
    list<OspfTypes::RouterID>::iterator i;
    for (i = attached_routers.begin(); i != attached_routers.end(); i++) {
	// Don't make an edge back to this router.
	if (*i == rladv)
	    continue;

	// If this is the designated router then we know have a full
	// adjacency with all the attached routers as this router
	// created this list. If we are not the designated router then
	// we need to check that we are at least 2-Way.
	if (!dr) {
	    if (!neighbour_at_least_two_way(*i))
		continue;
	}

	// Find the Router-LSA that points back to this Network-LSA.
	// Remember that an OSPSv3 router can generate multiple
	// Router-LSAs so it is necessary to search all of them for the
	// corresponding router link.
	bool found = false;
	Lsa::LsaRef lsapeer;    
	RouterLsa *rlsapeer = 0;
	uint32_t interface_id;
	for(size_t index = 0;; index++) {
	    if (!find_router_lsa(*i, index))
		break;
	    lsapeer = _db[index];
	    // This can probably never happen
	    if (lsapeer->maxage()) {
		XLOG_WARNING("LSA in database MaxAge\n%s", cstring(*lsapeer));
		continue;
	    }
// 	    fprintf(stderr, "peer %s\n", cstring(*lsapeer));

	    rlsapeer = dynamic_cast<RouterLsa *>(lsapeer.get());
	    XLOG_ASSERT(0 != rlsapeer);
	    if (bidirectionalV3(rlsapeer, nlsa, interface_id)) {
		found = true;
		break;
	    }
	}

	if (!found)
	    continue;

	// The coressponding router link has been found but should it
	// be used?
	Options options(_ospf.get_version(), rlsapeer->get_options());
	if (!options.get_v6_bit())
	    continue;
	if (!options.get_r_bit())
	    continue;

	uint32_t adv = lsapeer->get_header().get_advertising_router();

	// Router-LSA <=> Network-LSA <=> Router-LSA.
	// There is bidirectional connectivity from the original
	// Router-LSA through to this one found in the
	// Network-LSA, make an edge.
	    
	// The destination node may not exist if it doesn't
	// create it. Half the time it will exist
	Vertex dst;
	dst.set_version(_ospf.get_version());
	dst.set_type(OspfTypes::Router);
	dst.set_nodeid(adv);
	dst.get_lsas().push_back(lsapeer);
	// If the src is the origin then set the address of the
	// dest. This is the nexthop address from the origin.
	if (src.get_origin()) {
	    // Find the nexthop address from the router's Link-LSA. If the
	    // nexthop can't be found then there is no point putting
	    // this router into the graph.
	    A interface_address;
	    if (!find_interface_address(adv, interface_id,
					interface_address))
		continue;
	    dst.set_address(interface_address);
	    dst.set_nexthop_id(rl.get_interface_id());
	}
	if (!spt.exists_node(dst)) {
	    spt.add_node(dst);
	}
	update_edge(spt, src, rl.get_metric(), dst);
    }
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
