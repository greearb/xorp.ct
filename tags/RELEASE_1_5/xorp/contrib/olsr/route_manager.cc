// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/contrib/olsr/route_manager.cc,v 1.1 2008/04/24 15:19:54 bms Exp $"

#include "olsr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"

// #define DEBUG_LOGGING
// #define DEBUG_FUNCTION_NAME
// #define DETAILED_DEBUG

#include <list>

#include "libproto/spt.hh"

#include "olsr.hh"
#include "vertex.hh"
#include "route_manager.hh"

// TODO: Convert assertions to exception throws in vertex/link additions.
// TODO: Support composite metrics.
// At the moment we use a simple scalar scheme which conforms to the RFC.
// TODO: Optionally withdraw all unfiltered routes from RIB on shutdown.
// TODO: Support ETX metrics.
// TODO: Support advertised metric scheme for HNA routes.
// TODO: Support computation of external routes independently of SPT.
// Currently an entire routing calculation is performed when external
// routes change.
// TODO: Support incremental SPT re-computation.

string
RouteEntry::str()
{
    string output;

    output = c_format("RouteEntry: ");
    output += c_format("%s ", vt_to_str(destination_type()));
    output += c_format("%s", direct() ? "direct " : "");
    if (is_node_vertex(_destination_type)) {
	output += c_format("mainaddr %s ", cstring(main_address()));
    }
    output += c_format("cost %d ", XORP_UINT_CAST(cost()));
    output += c_format("nexthop %s ", cstring(nexthop()));
    output += c_format("originator %s ", cstring(originator()));

    return output;
}

RouteManager::RouteManager(Olsr& olsr, EventLoop& eventloop,
			   FaceManager* fm, Neighborhood* nh,
			   TopologyManager* tm, ExternalRoutes* er)
     : _olsr(olsr),
       _eventloop(eventloop),
       _fm(fm),
       _nh(nh),
       _tm(tm),
       _er(er),
       _spt(Spt<Vertex>(_olsr.trace()._spt)),
       _in_transaction(false),
       _current(0),
       _previous(0)
{
    //
    // Create the route computation background task but make sure it is not
    // scheduled until:
    // 1. we have either acquired artifacts which will form part of the
    //    route table (if doing incremental calculation), or:
    // 2. we are asked to by our clients.
    //
    _route_update_task = _eventloop.new_oneoff_task(
	callback(this, &RouteManager::recompute_all_routes),
	XorpTask::PRIORITY_DEFAULT,
	XorpTask::WEIGHT_DEFAULT);
    _route_update_task.unschedule();
}

RouteManager::~RouteManager()
{
    _route_update_task.unschedule();

#if 1
    // XXX Ensure that clients can't see us any more.
    if (_er)
	_er->set_route_manager(0);
    if (_tm)
	_tm->set_route_manager(0);
    if (_nh)
	_nh->set_route_manager(0);
#endif

    // Clear internal routing tables.
    delete _previous;
    delete _current;
}

void
RouteManager::schedule_route_update()
{
    _route_update_task.reschedule();
}

void
RouteManager::schedule_external_route_update()
{
    _route_update_task.reschedule();
}

void
RouteManager::begin()
{
    debug_msg("\n");
    XLOG_ASSERT(! _in_transaction);
    _in_transaction = true;

    delete _previous;
    _previous = _current;
    _current = new Trie<IPv4, RouteEntry>;

    if (0 == _previous)
	return;

    // No checks for other sub-protocols should be needed.
}

void
RouteManager::end()
{
    debug_msg("\n");
    XLOG_ASSERT(_in_transaction);
    _in_transaction = false;

    Trie<IPv4, RouteEntry>::iterator tip;
    Trie<IPv4, RouteEntry>::iterator tic;

    // If no previous routing table, just install current one.
    if (0 == _previous) {
	for (tic = _current->begin(); tic != _current->end(); tic++) {
	    RouteEntry& rt = tic.payload();
	    if (! add_route(tic.key(), rt.nexthop(), rt.cost(), rt)) {
		XLOG_WARNING("Add of %s failed", cstring(tic.key()));
	    }
	}
	return;
    }

    // Withdraw routes which no longer exist in new table.
    for (tip = _previous->begin(); tip != _previous->end(); tip++) {
	if (_current->end() == _current->lookup_node(tip.key())) {
	    RouteEntry& rt = tip.payload();
	    if (! delete_route(tip.key(), rt)) {
		XLOG_WARNING("Delete of %s failed", cstring(tip.key()));
	    }
	}
    }

    // Add or replace routes which exist in new table.
    for (tic = _current->begin(); tic != _current->end(); tic++) {
	RouteEntry& rt = tic.payload();

	tip = _previous->lookup_node(tic.key());
	if (_previous->end() == tip) {
	    // The route is new and should be added.
	    if (! add_route(tic.key(), rt.nexthop(), rt.cost(), rt)) {
		XLOG_WARNING("Add of %s failed", cstring(tic.key()));
	    }
	} else {
	    // The route already exists and may possibly be replaced.
	    RouteEntry& rt_previous = tip.payload();

	    if (rt.nexthop() != rt_previous.nexthop() ||
		rt.cost() != rt_previous.cost()) {
		// The cost or nexthop changed; replace the route.
		if (! replace_route(tip.key(), rt.nexthop(), rt.cost(),
				    rt, rt_previous)) {
		    XLOG_WARNING("Replace of %s failed", cstring(tip.key()));
		}
	    } else {
		// No change; just update policy filtering state.
		rt.set_filtered(rt_previous.filtered());
	    }
	}
    }
}

void
RouteManager::recompute_all_routes()
{
    debug_msg("%s: recomputing all routes\n",
	      cstring(_fm->get_main_addr()));

    // Clear the Spt.
    _spt.clear();

    // Add the origin node.
    _origin = make_origin_vertex();
    _spt.add_node(_origin);
    _spt.set_origin(_origin);

    // Ask the associated classes to push their topology to us.
    // They will call us back to populate the SPT.
    _nh->push_topology();
    _tm->push_topology();

    // Perform the SPT route computation.
    list<RouteCmd<Vertex> > r;
    _spt.compute(r);

    // Begin a new routing trie transaction.
    begin();

    // Produce route entries from SPT computation result.
    // This takes care of all steps of Section 10, except for
    // 3.2 next-hop selection which needs weighting in Vertex
    // for MPRs, which we'll do with the new composite metric scheme.
    // Assumes the non-incremental version of SPT.
    list<RouteCmd<Vertex> >::const_iterator ri;
    for (ri = r.begin(); ri != r.end(); ri++) {
	const Vertex& node = ri->node();
	const Vertex& nexthop = ri->nexthop();

	RouteEntry rt;
	IPv4 addr;
	IPv4Net dest;

#ifdef DETAILED_DEBUG
	// Invariant: Destination vertex must be derived from SPT.
	XLOG_ASSERT(node.type() == OlsrTypes::VT_NEIGHBOR ||
		    node.type() == OlsrTypes::VT_TWOHOP ||
		    node.type() == OlsrTypes::VT_TOPOLOGY);

	// Invariant: Next-hop thus chosen must be a one-hop neighbor.
	XLOG_ASSERT(nexthop.type() == OlsrTypes::VT_NEIGHBOR);
#endif

	rt.set_destination_type(node.type());	    // what produced the route
	rt.set_originator(node.producer());	    // who produced the route
	rt.set_main_address(node.main_addr());	    // where it goes
	rt.set_cost(ri->weight());		    // how much the path costs

	// If the node *is* its own nexthop, it's directly connected.
	rt.set_direct(node == nexthop);

	// Links in the one-hop neighborhood must use the
	// address advertised in the HELLO link tuple as the next hop.
	if (node.type() == OlsrTypes::VT_NEIGHBOR) {
	    addr = node.link()->remote_addr();
	    rt.set_nexthop(addr);
	    rt.set_faceid(node.link()->faceid());
	} else {
	    // Links beyond the one-hop neighborhood must use the
	    // link address of the first hop as the next-hop address,
	    // as there might not be a RIB to catch our unresolved next-hops.
	    //
	    // The destination must be the main address, as we have no
	    // link specific information at this point; MID aliases will
	    // be added further below.
	    addr = node.main_addr();
	    rt.set_nexthop(nexthop.link()->remote_addr());
	    rt.set_faceid(nexthop.link()->faceid());
	}

	// OLSR routes are always host routes.
	dest = IPv4Net(addr, IPv4::ADDR_BITLEN);

	add_entry(dest, rt);

	//
	// If the route being added is to a one-hop neighbor,
	// and its link address differs from its main address,
	// make sure we also add a route to its main address,
	// as per RFC. This is separate from MID processing.
	// The nexthop should remain unchanged.
	//
	if (node.type() == OlsrTypes::VT_NEIGHBOR &&
	    node.main_addr() != node.link()->remote_addr()) {
	    IPv4Net main_dest(node.main_addr(), IPv4::ADDR_BITLEN);
	    add_entry(main_dest, rt);
	}

	// 10, 4: Add routes to each MID alias known for the node.
	vector<IPv4> aliases = _tm->get_mid_addresses(node.main_addr());
	if (! aliases.empty()) {
	    // Mark the routes learned from MID as such.
	    rt.set_destination_type(OlsrTypes::VT_MID);

	    vector<IPv4>::iterator ai;
	    for (ai = aliases.begin(); ai != aliases.end(); ai++) {
		IPv4& alias_addr = (*ai);

		// If we just added a route to a one-hop neighbor, then
		// skip the link specific address or primary address as
		// we just dealt with those.
		if (node.type() == OlsrTypes::VT_NEIGHBOR &&
		    (alias_addr == node.link()->remote_addr() ||
		     alias_addr == node.main_addr()))
		    continue;

		// Add a route to this MID alias for the node.
		IPv4Net mid_dest(alias_addr, IPv4::ADDR_BITLEN);
		add_entry(mid_dest, rt);
	    }
	}
    }

    // Ask ExternalRoutes to push its state to us.
    _er->push_external_routes();

    // Push the routes to the RIB, via policy.
    end();
}

bool
RouteManager::add_onehop_link(const LogicalLink* l, const Neighbor* n)
{
    // Invariant: RouteManager should never see a Neighbor with WILL_NEVER.
    XLOG_ASSERT(n->willingness() != OlsrTypes::WILL_NEVER);

    // Create the neighbor vertex.
    Vertex v_n(*n);
    v_n.set_faceid(l->faceid());

    // Asssociate l with v_n, for doing the MID routes correctly later on;
    // we must choose a single link for Dijkstra calculation, and there
    // is no way of associating attributes with edges just now.
    v_n.set_link(l);

    bool is_n_added = _spt.add_node(v_n);
    XLOG_ASSERT(true == is_n_added);

    // 10, 3.2: When h=1, nodes with highest willingness and MPR selectors
    // are preferred as next hop. Break ties using a simple scalar scheme.
    int cost;
    _fm->get_interface_cost(l->faceid(), cost);
    cost += OlsrTypes::WILL_MAX - n->willingness();
    if (! n->is_mpr_selector())
	cost++;

    // Add the link from myself to v_n.
    bool is_link_added = _spt.add_edge(_origin, cost, v_n);
    XLOG_ASSERT(true == is_link_added);

    return is_link_added;
}

bool
RouteManager::add_twohop_link(const Neighbor* n, const TwoHopLink* l2,
			      const TwoHopNeighbor* n2)
{
    debug_msg("adding n2 %s via n1 %s\n",
	      cstring(n2->main_addr()),
	      cstring(n->main_addr()));

    // Assert that the Neighbor exists.
    // TODO: Throw an exception, we should never see this.
    Vertex v_n(*n);
    if (! _spt.exists_node(v_n)) {
	debug_msg("N1 %s does not exist.\n", cstring(n->main_addr()));
	return false;
    }

    // Make a Vertex from TwoHopNeighbor.
    Vertex v_n2(*n2);

    //  We need to know the origin of the vertex. In the case
    //  of a two-hop neighbor, this is the neighbor which advertises
    //  the link to the two-hop neighbor, N.
    v_n2.set_producer(n->main_addr());

    //  Associate l2 with v_n, for doing the MID routes correctly later on;
    //  we must choose a single link for Dijkstra calculation, and there
    //  is no way of associating attributes with edges just now.
    v_n2.set_twohop_link(l2);

    bool is_n2_added = _spt.add_node(v_n2);
    XLOG_ASSERT(true == is_n2_added);

    // Add the link from v_n to v_n2.
    bool is_link_added = _spt.add_edge(v_n, 1, v_n2);
    XLOG_ASSERT(true == is_link_added);

    return is_link_added;
    UNUSED(l2);
}

bool
RouteManager::add_tc_link(const TopologyEntry* tc)
{
    // Check that the near endpoint (last hop) exists.
    // It should be a two-hop neighbor when tc->distance() is 2,
    // and a TC node in all other cases, however this is a pedantic invariant.
    //
    // It's problematic that we can't separate the semantics of search
    // in Spt from storage without considerably changing the class.
    //
    // Note: The TC input code can only measure the distance from the
    // node to the TC message's origin.

    Vertex v_lh(tc->lasthop());
    if (! _spt.exists_node(v_lh)) {
	debug_msg("Ignoring TC %p as last-hop to %s not in SPT, v_lh:\n%s\n",
		  tc, cstring(tc->destination()), cstring(v_lh));
	return false;
    }

    // Add the vertex for the remote endpoint learned from TC.
    // A trace statement may be printed if the node already exists.
    Vertex v_tc(*tc);
    bool is_tc_added = _spt.add_node(v_tc);
    UNUSED(is_tc_added);

    // Add the link from v_lh to v_tc.
    // For a non-LQ link the cost is always 1.
    // A trace statement may be printed if the link already exists.
    bool is_link_added = _spt.add_edge(v_lh, 1, v_tc);
    UNUSED(is_link_added);

    return true;
}

bool
RouteManager::add_hna_route(const IPv4Net& dest,
			    const IPv4& lasthop,
			    const uint16_t distance)
{
    // We need to look up the route to the node which advertises the
    // prefix 'dest' to know what the next-hop should be.
    Trie<IPv4, RouteEntry>::iterator tic =
	_current->lookup_node(IPv4Net(lasthop, IPv4::ADDR_BITLEN));
    if (tic == _current->end()) {
	debug_msg("%s: skipping HNA %s, no route to origin %s\n",
		   cstring(_fm->get_main_addr()),
		   cstring(dest),
		   cstring(lasthop));
	return false;
    }

    RouteEntry& ort = tic.payload();	// The route to the origin.
    RouteEntry rt;			// The route we are about to add.

    rt.set_destination_type(OlsrTypes::VT_HNA);	// what produced the route
    rt.set_originator(lasthop);			// who produced the route
    rt.set_nexthop(ort.nexthop());		// the next hop

    // 12.6: The cost of an RFC compliant HNA route is that of
    // the path used to reach the OLSR node which advertises it.
    rt.set_cost(ort.cost());

    add_entry(dest, rt);

    return true;

    // Currently the distance measured from the HNA messages is unused;
    // this parameter may change in future.
    UNUSED(distance);
}

bool
RouteManager::add_entry(const IPv4Net& net, const RouteEntry& rt)
{
    debug_msg("add %s %s\n",
	      cstring(net),
	      cstring(const_cast<RouteEntry&>(rt)));

    XLOG_ASSERT(_in_transaction);
    XLOG_ASSERT(rt.direct() || rt.nexthop() != IPv4::ZERO());

    bool result = true;

    Trie<IPv4, RouteEntry>::iterator tic;
    tic = _current->lookup_node(net);
    if (_current->end() == tic) {
	_current->insert(net, rt);
    }

    return result;
}

bool
RouteManager::delete_entry(const IPv4Net& net, const RouteEntry& rt)
{
    debug_msg("delete %s %s\n",
	      cstring(net),
	      cstring(const_cast<RouteEntry&>(rt)));

    Trie<IPv4, RouteEntry>::iterator tic;
    tic = _current->lookup_node(net);
    if (_current->end() != tic) {
	_current->erase(tic);
    }

    return false;
    UNUSED(rt);
}

bool
RouteManager::replace_entry(const IPv4Net& net,
			    const RouteEntry& rt,
			    const RouteEntry& previous_rt)
{
    debug_msg("replace %s %s\n",
	      cstring(net),
	      cstring(const_cast<RouteEntry&>(previous_rt)));

    delete_entry(net, previous_rt);
    bool result = add_entry(net, rt);

    return result;
}

bool
RouteManager::add_route(IPv4Net net, IPv4 nexthop,
			uint32_t metric, RouteEntry& rt)
{
   debug_msg("ADD ROUTE net %s nexthop %s metric %u\n",
	     cstring(net), cstring(nexthop), metric);

    bool result = true;

    PolicyTags policytags;
    bool accepted = do_filtering(net, nexthop, metric, rt, policytags);

    rt.set_filtered(!accepted);
    if (accepted) {
	result = _olsr.add_route(net, nexthop, rt.faceid(), metric,
				 policytags);
    }

    return result;
}

bool
RouteManager::delete_route(const IPv4Net& net, const RouteEntry& rt)
{
    debug_msg("DELETE ROUTE %s filtered %s\n",
	      cstring(net), bool_c_str(rt.filtered()));

    bool result = false;
    if (! rt.filtered()) {
	result = _olsr.delete_route(net);
    } else {
	result = true;
    }

    return result;
}

bool
RouteManager::replace_route(IPv4Net net, IPv4 nexthop,
			    uint32_t metric,
			    RouteEntry& rt,
			    RouteEntry& previous_rt)
{
    debug_msg("REPLACE ROUTE %s\n", cstring(net));

    bool result = delete_route(net, previous_rt);
    if (!result)
	XLOG_WARNING("Failed to delete: %s", cstring(net));

    result = add_route(net, nexthop, metric, rt);

    return result;
}

void
RouteManager::push_routes()
{
    Trie<IPv4, RouteEntry>::iterator tic;

    if (0 == _current)
	return;

    for (tic = _current->begin(); tic != _current->end(); tic++) {
	RouteEntry& rt = tic.payload();

	PolicyTags policytags;
	IPv4Net net = tic.key();
	IPv4 nexthop = rt.nexthop();
	uint32_t faceid = rt.faceid();
	uint32_t metric = rt.cost();
	bool accepted = do_filtering(net, nexthop, metric, rt, policytags);

	if (accepted) {
	    if (! rt.filtered()) {
		_olsr.replace_route(net, nexthop, faceid, metric,
				    policytags);
				    
	    } else {
		_olsr.add_route(net, nexthop, faceid, metric,
				policytags);
	    }
	} else {
	    if (! rt.filtered()) {
		_olsr.delete_route(net);
	    }
	}

	rt.set_filtered(!accepted);
    }
}

bool
RouteManager::do_filtering(IPv4Net& net, IPv4& nexthop,
			   uint32_t& metric, RouteEntry& rt,
			   PolicyTags& policytags)
{
#ifdef notyet
    // Unlike OSPF, OLSR is expected to send host routes to the
    // RIB which may be directly connected.
    if (rt.direct())
	return false;
#endif

    try {
	IPv4 originator = rt.originator();
	IPv4 main_addr = rt.main_address();
	uint32_t type = rt.destination_type();

	// Import filtering.
	OlsrVarRW varrw(net, nexthop, metric, originator, main_addr, type,
			policytags);

	bool accepted = false;

	debug_msg("[OLSR] Running filter: %s on route: %s\n",
		  filter::filter2str(filter::IMPORT).c_str(), cstring(net));
	XLOG_TRACE(_olsr.trace()._import_policy,
		  "[OSPF] Running filter: %s on route: %s\n",
		  filter::filter2str(filter::IMPORT).c_str(), cstring(net));

	accepted = _olsr.get_policy_filters().
		       run_filter(filter::IMPORT, varrw);

	if (!accepted)
	    return accepted;

	// Export source-match filtering.
	OlsrVarRW varrw2(net, nexthop, metric, originator, main_addr, type,
			 policytags);

	debug_msg("[OLSR] Running filter: %s on route: %s\n",
		  filter::filter2str(filter::EXPORT_SOURCEMATCH).c_str(),
		  cstring(net));
	XLOG_TRACE(_olsr.trace()._import_policy,
		   "[OLSR] Running filter: %s on route: %s\n",
		   filter::filter2str(filter::EXPORT_SOURCEMATCH).c_str(),
		  cstring(net));

	_olsr.get_policy_filters().
	    run_filter(filter::EXPORT_SOURCEMATCH, varrw2);

	return accepted;

    } catch(const PolicyException& e) {
	XLOG_WARNING("PolicyException: %s", cstring(e));
	return false;
    }

    return true;
}
