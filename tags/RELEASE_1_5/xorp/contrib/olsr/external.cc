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

#ident "$XORP: xorp/contrib/olsr/external.cc,v 1.1 2008/04/24 15:19:51 bms Exp $"

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

#include "olsr.hh"
#include "external.hh"

// NOTE: The OlsrTypes::ExternalID identifier space is currently shared
// between learned routes and originated routes.

// TODO: External route metrics.
//  TODO: Use metric information for collation in ExternalRouteOrderPred.

bool
ExternalRouteOrderPred::operator()(const OlsrTypes::ExternalID lhid,
				   const OlsrTypes::ExternalID rhid)
{
    try {
	// TODO Propagate exceptions if IDs cannot be retrieved,
	// rather than just catching them.
	const ExternalRoute* lhp = _ers.get_hna_route_in_by_id(lhid);
	const ExternalRoute* rhp = _ers.get_hna_route_in_by_id(rhid);

	// Invariant: Both ExternalRoutes SHOULD have the same origination
	// status, i.e. learned routes should only be compared with other
	// learned routes; originated routes with originated routes.
	XLOG_ASSERT(lhp->is_self_originated() == rhp->is_self_originated());

	if (lhp->dest() == rhp->dest()) {
	    // Invariant: Self originated routes should have a distance of 0.
	    // Learned routes should have a non-zero distance.
	    XLOG_ASSERT(lhp->is_self_originated() ?
			lhp->distance() == 0 && rhp->distance() == 0 :
			lhp->distance() != 0 && rhp->distance() != 0);

	    return lhp->distance() < rhp->distance();
	}
	return lhp->dest() < rhp->dest(); // Collation order on IPvXNet.

    } catch (...) {}

    return false;
}

/*
 * ExternalRoutes.
 */

ExternalRoutes::ExternalRoutes(Olsr& olsr, EventLoop& eventloop,
    FaceManager& fm, Neighborhood& nh)
     : _olsr(olsr),
       _eventloop(eventloop),
       _fm(fm),
       _nh(nh),
       _rm(0),
       _routes_in_order_pred(*this),
       _is_early_hna_enabled(false),
       _next_erid(1),
       _hna_interval(TimeVal(OlsrTypes::DEFAULT_HNA_INTERVAL, 0))
{
    _fm.add_message_cb(callback(this,
				&ExternalRoutes::event_receive_hna));
}

ExternalRoutes::~ExternalRoutes()
{
    _fm.delete_message_cb(callback(this,
				   &ExternalRoutes::event_receive_hna));

    clear_hna_routes_in();
    clear_hna_routes_out();
}

/*
 * Protocol variables.
 */

void
ExternalRoutes::set_hna_interval(const TimeVal& hna_interval)
{
    if (hna_interval == _hna_interval)
	return;

    debug_msg("%s setting HNA interval to %s.\n",
	      cstring(_fm.get_main_addr()),
	      cstring(hna_interval));

    _hna_interval = hna_interval;

    if (_hna_send_timer.scheduled()) {
	// Change period.
	reschedule_hna_send_timer();

	// Optionally, fire one off now.
	//reschedule_immediate_hna_send_timer();
    }
}

/*
 * HNA Routes In.
 */

OlsrTypes::ExternalID
ExternalRoutes::update_hna_route_in(const IPv4Net& dest,
				    const IPv4& lasthop,
				    const uint16_t distance,
				    const TimeVal& expiry_time,
				    bool& is_created)
    throw(BadExternalRoute)
{
    debug_msg("Dest %s Lasthop %s Distance %u ExpiryTime %s\n",
	      cstring(dest),
	      cstring(lasthop),
	      XORP_UINT_CAST(distance),
	      cstring(expiry_time));

    // We perform the multimap search inline, to avoid doing it more than
    // once; we may need to re-insert into the multimap if the HNA
    // distance changes.

    OlsrTypes::ExternalID erid;
    ExternalRoute* er = 0;
    bool is_found = false;

    pair<ExternalDestInMap::iterator, ExternalDestInMap::iterator> rd =
	_routes_in_by_dest.equal_range(dest);
    ExternalDestInMap::iterator ii;

    for (ii = rd.first; ii != rd.second; ii++) {
	er = _routes_in[(*ii).second];
	if (er->lasthop() == lasthop) {
	    is_found = true;
	    break;
	}
    }

    if (is_found) {
	erid = er->id();

	if (er->distance() != distance) {
	    // If the distance changed, update it.
	    //  ii already points to the entry, so need only
	    //  erase and re-insert; the use of ExternalRouteOrderPred
	    //  will cause this to be an insertion sort.
	    _routes_in_by_dest.erase(ii);

	    er->set_distance(distance);
	    _routes_in_by_dest.insert(make_pair(dest, erid));
	}

	er->update_timer(expiry_time);

    } else {
	// Create a new HNA entry.
	erid = add_hna_route_in(dest, lasthop, distance, expiry_time);
    }

    is_created = !is_found;

    return erid;
}

OlsrTypes::ExternalID
ExternalRoutes::add_hna_route_in(const IPv4Net& dest,
			         const IPv4& lasthop,
			         const uint16_t distance,
			         const TimeVal& expiry_time)
    throw(BadExternalRoute)
{
    debug_msg("Dest %s Lasthop %s Distance %u ExpiryTime %s\n",
	      cstring(dest),
	      cstring(lasthop),
	      XORP_UINT_CAST(distance),
	      cstring(expiry_time));

    OlsrTypes::ExternalID erid = _next_erid++;

    if (0 != _routes_in.count(erid)) {
	xorp_throw(BadExternalRoute,
		   c_format("Mapping for ExternalID %u already exists",
		   XORP_UINT_CAST(erid)));
    }

    _routes_in[erid] = new ExternalRoute(*this, _eventloop, erid,
					 dest, lasthop, distance,
					 expiry_time);

    _routes_in_by_dest.insert(make_pair(dest, erid));

    // Route update will be scheduled further up in the call graph.

    return erid;
}

bool
ExternalRoutes::delete_hna_route_in(OlsrTypes::ExternalID erid)
{
    debug_msg("ExternalID %u\n", XORP_UINT_CAST(erid));

    ExternalRouteMap::iterator ii = _routes_in.find(erid);
    if (ii == _routes_in.end())
	return false;

    ExternalRoute* er = (*ii).second;

    // Prune from destination map.
    // Note: does not maintain invariant on ID in _routes_in_by_dest.
    pair<ExternalDestInMap::iterator,
	 ExternalDestInMap::iterator> rd =
	_routes_in_by_dest.equal_range(er->dest());
    ExternalDestInMap::iterator jj;
    for (jj = rd.first; jj != rd.second; ) {
	if ((*jj).second == erid) {
	    _routes_in_by_dest.erase(jj);
	    break;
	}
    }

    // Ensure routes are withdrawn.
    if (_rm)
	_rm->schedule_external_route_update();

    _routes_in.erase(ii);
    delete er;

    return true;
}

void
ExternalRoutes::clear_hna_routes_in()
{
    _routes_in_by_dest.clear();

    ExternalRouteMap::iterator ii, jj;
    for (ii = _routes_in.begin(); ii != _routes_in.end(); ) {
	jj = ii++;
	delete (*jj).second;
	_routes_in.erase(jj);
    }

    // Ensure routes are withdrawn.
    // Check that _rm is non-0 in case we are called from destructor.
    if (_rm)
	_rm->schedule_external_route_update();
}

const ExternalRoute*
ExternalRoutes::get_hna_route_in(const IPv4Net& dest,
				 const IPv4& lasthop)
    throw(BadExternalRoute)
{
    pair<ExternalDestInMap::const_iterator,
	 ExternalDestInMap::const_iterator> rd =
	_routes_in_by_dest.equal_range(dest);

    ExternalRoute* er = 0;
    bool is_found = false;

    ExternalDestInMap::const_iterator ii;
    for (ii = rd.first; ii != rd.second; ii++) {
	er = _routes_in[(*ii).second];
	if (er->lasthop() == lasthop) {
	    is_found = true;
	    break;
	}
    }

    if (! is_found) {
	xorp_throw(BadExternalRoute,
		   c_format("Mapping for %s:%s does not exist",
			    cstring(dest),
			    cstring(lasthop)));
    }

    return er;
}

OlsrTypes::ExternalID
ExternalRoutes::get_hna_route_in_id(const IPv4Net& dest,
				    const IPv4& lasthop)
    throw(BadExternalRoute)
{
    const ExternalRoute* er = get_hna_route_in(dest, lasthop);

    return er->id();
}

const ExternalRoute*
ExternalRoutes::get_hna_route_in_by_id(const OlsrTypes::ExternalID erid)
    throw(BadExternalRoute)
{
    ExternalRouteMap::iterator ii = _routes_in.find(erid);
    if (ii ==  _routes_in.end()) {
	xorp_throw(BadExternalRoute,
		   c_format("Mapping for %u does not exist",
			    XORP_UINT_CAST(erid)));
    }

    return (*ii).second;
}

size_t
ExternalRoutes::hna_origin_count() const
{
    set<IPv4> origins;

    // Count the number of unique origins in _routes_in.
    ExternalRouteMap::const_iterator ii;
    for (ii = _routes_in.begin(); ii != _routes_in.end(); ii++) {
	ExternalRoute* er = (*ii).second;
	XLOG_ASSERT(! er->is_self_originated());
	// Requires that origins is a model of UniqueAssociativeContainer.
	origins.insert(er->lasthop());
    }

    return origins.size();
}

size_t
ExternalRoutes::hna_dest_count() const
{
    size_t unique_key_count = 0;

    ExternalDestInMap::const_iterator ii;
    for (ii = _routes_in_by_dest.begin();
	 ii != _routes_in_by_dest.end();
	 ii = _routes_in_by_dest.upper_bound((*ii).first))
    {
	unique_key_count++;
    }

    return unique_key_count;
}

void
ExternalRoutes::get_hna_route_in_list(list<OlsrTypes::ExternalID>& hnalist)
{
    ExternalRouteMap::const_iterator ii;
    for (ii = _routes_in.begin(); ii != _routes_in.end(); ii++)
	hnalist.push_back((*ii).first);
}


/*
 * HNA Routes Out [Redistribution].
 */

bool
ExternalRoutes::originate_hna_route_out(const IPv4Net& dest)
    throw(BadExternalRoute)
{
    debug_msg("MyMainAddr %s Dest %s\n",
	      cstring(_fm.get_main_addr()),
	      cstring(dest));

    bool is_first_route = _routes_out.empty();

    if (0 != _routes_out_by_dest.count(dest)) {
	debug_msg("Already originating %s\n", cstring(dest));
	return false;
    }

    OlsrTypes::ExternalID erid = _next_erid++;

    if (0 != _routes_out.count(erid)) {
	xorp_throw(BadExternalRoute,
		   c_format("Mapping for ExternalID %u already exists",
		   XORP_UINT_CAST(erid)));
    }

    _routes_out[erid] = new ExternalRoute(*this, _eventloop, erid, dest);

    _routes_out_by_dest.insert(make_pair(dest, erid));

    if (is_first_route)
	start_hna_send_timer();

    // If configured to send HNA advertisements immediately when
    // routes are originated, do so.
    if (_is_early_hna_enabled)
	reschedule_immediate_hna_send_timer();

    return true;
}

void
ExternalRoutes::withdraw_hna_route_out(const IPv4Net& dest)
    throw(BadExternalRoute)
{
    debug_msg("MyMainAddr %s Dest %s\n",
	      cstring(_fm.get_main_addr()),
	      cstring(dest));

    ExternalDestOutMap::iterator ii = _routes_out_by_dest.find(dest);
    if (ii == _routes_out_by_dest.end()) {
	xorp_throw(BadExternalRoute,
		   c_format("%s is not originated by this node",
			cstring(dest)));
    }

    ExternalRouteMap::iterator jj = _routes_out.find((*ii).second);
    if (jj == _routes_out.end()) {
	XLOG_UNREACHABLE();
	xorp_throw(BadExternalRoute,
		   c_format("Mapping for %s does not exist",
			cstring(dest)));
    }

    ExternalRoute* er = (*jj).second;
    XLOG_ASSERT(er != 0);

    if (! er->is_self_originated()) {
	XLOG_UNREACHABLE();
	xorp_throw(BadExternalRoute,
		   c_format("%s is not a self-originated prefix",
			cstring(dest)));
    }

    _routes_out.erase(jj);
    _routes_out_by_dest.erase(ii);

    delete er;

    // If the last originated route has been withdrawn,
    // stop the HNA transmission timer.
    // [No point in scheduling an early HNA broadcast, as
    //  HNA routes are expired only when their life timer expires.]
    if (_routes_out.empty()) {
	debug_msg("%s: stopping HNA timer as last external route is now"
		  "withdrawn.\n",
		  cstring(_fm.get_main_addr()));
	stop_hna_send_timer();
    }
}

void
ExternalRoutes::clear_hna_routes_out()
{
    ExternalRouteMap::iterator ii, jj;
    for (ii = _routes_out.begin(); ii != _routes_out.end(); ) {
	jj = ii++;
	delete (*jj).second;
	_routes_out.erase(jj);
    }
}

OlsrTypes::ExternalID
ExternalRoutes::get_hna_route_out_id(const IPv4Net& dest)
    throw(BadExternalRoute)
{
    ExternalDestOutMap::const_iterator ii = _routes_out_by_dest.find(dest);
    if (ii == _routes_out_by_dest.end()) {
	xorp_throw(BadExternalRoute,
		   c_format("Mapping for %s does not exist",
			    cstring(dest)));
    }

    return (*ii).second;
}

/*
 * RouteManager interaction.
 */

void
ExternalRoutes::push_external_routes()
{
    XLOG_ASSERT(_rm != 0);

    size_t pushed_route_count = 0;

    // For each destination (key) in the HNA "routes in" container, pick
    // the route with the shortest distance.
    // [An insertion sort was performed by update_hna_route_in(), so
    //  the first route for each destination has the shortest distance.]
    // Recursive resolution of the next hop is the RIB's problem.
    for (ExternalDestInMap::iterator ii = _routes_in_by_dest.begin();
	 ii != _routes_in_by_dest.end();
	 ii = _routes_in_by_dest.upper_bound((*ii).first))
    {
	ExternalRoute* er = _routes_in[(*ii).second];

	bool is_route_added = _rm->add_hna_route(er->dest(),
						 er->lasthop(),
						 er->distance());
	if (is_route_added)
	    ++pushed_route_count;
    }

    debug_msg("%s: pushed %u HNA routes to RouteManager.\n",
	      cstring(_fm.get_main_addr()),
	      XORP_UINT_CAST(pushed_route_count));
}

/*
 * Timer manipulation.
 */

void
ExternalRoutes::start_hna_send_timer()
{
    debug_msg("%s -> HNA_RUNNING\n", cstring(_fm.get_main_addr()));

    _hna_send_timer = _eventloop.
	new_periodic(get_hna_interval(),
		     callback(this, &ExternalRoutes::event_send_hna));
}

void
ExternalRoutes::stop_hna_send_timer()
{
    debug_msg("%s -> HNA_STOPPED\n", cstring(_fm.get_main_addr()));

    _hna_send_timer.clear();
}

void
ExternalRoutes::restart_hna_send_timer()
{
    reschedule_hna_send_timer();
}

void
ExternalRoutes::reschedule_hna_send_timer()
{
    _hna_send_timer.reschedule_after(get_hna_interval());

}

void
ExternalRoutes::reschedule_immediate_hna_send_timer()
{
    _hna_send_timer.schedule_now();
}

/*
 * Event handlers.
 */

bool
ExternalRoutes::event_send_hna()
{
    debug_msg("MyMainAddr %s event_send_hna \n",
	      cstring(_fm.get_main_addr()));

    XLOG_ASSERT(! _routes_out.empty());

    HnaMessage* hna = new HnaMessage();

    // 12.2: TTL is set to max, expiry time is set to HNA_HOLD_TIME.
    hna->set_expiry_time(get_hna_hold_time());
    hna->set_origin(_fm.get_main_addr());
    hna->set_ttl(OlsrTypes::MAX_TTL);
    hna->set_hop_count(0);
    hna->set_seqno(_fm.get_msg_seqno());

    // Populate the message with the routes which this node advertises.
    ExternalRouteMap::const_iterator ii;
    for (ii = _routes_out.begin(); ii != _routes_out.end(); ii++) {
	ExternalRoute* er = (*ii).second;
	hna->add_network(er->dest());
    }

    // Flood the message to the rest of the OLSR topology.
    bool is_flooded = _fm.flood_message(hna);	// consumes hna
    UNUSED(is_flooded);

    // XXX refcounting has been removed
    delete hna;

    // Reschedule the timer for next time.
    return true;
}

bool
ExternalRoutes::event_receive_hna(
    Message* msg,
    const IPv4& remote_addr,
    const IPv4& local_addr)
{
    HnaMessage* hna = dynamic_cast<HnaMessage *>(msg);
    if (0 == hna)
	return false;	// not for me

    // Put this after the cast to skip false positives.
    debug_msg("[HNA] MyMainAddr %s Src %s RecvIf %s\n",
	      cstring(_fm.get_main_addr()),
	      cstring(remote_addr),
	      cstring(local_addr));

    // 12.5, 1: Sender must be in symmetric 1-hop neighborhood.
    if (! _nh.is_sym_neighbor_addr(remote_addr)) {
	debug_msg("Rejecting TC message from %s via non-neighbor %s\n",
		  cstring(msg->origin()),
		  cstring(remote_addr));
	XLOG_TRACE(_olsr.trace()._input_errors,
		   "Rejecting TC message from %s via non-neighbor %s",
		   cstring(msg->origin()),
		   cstring(remote_addr));
	return true; // consumed but invalid.
    }

    // Invariant: I should not see my own HNA route advertisements.
    XLOG_ASSERT(hna->origin() != _fm.get_main_addr());

    TimeVal now;
    _eventloop.current_time(now);

    // 12.5, 2: For each address/mask pair in the message,
    // create or update an existing entry.
    size_t updated_hna_count = 0;
    try {
	bool is_hna_created = false;

	// Account for hop count not being incremented before forwarding.
	const vector<IPv4Net>& nets = hna->networks();
	const uint16_t distance = hna->hops() + 1;

	vector<IPv4Net>::const_iterator ii;
	for (ii = nets.begin(); ii != nets.end(); ii++) {
	    update_hna_route_in((*ii), hna->origin(), distance,
				hna->expiry_time() + now,
				is_hna_created);
	    updated_hna_count++;
	    UNUSED(is_hna_created);
	}
    } catch (...) {
	// If an exception is thrown whilst processing the HNA
	// message, disregard the rest of the message.
    }

    if (updated_hna_count > 0)
	_rm->schedule_external_route_update();

    _fm.forward_message(remote_addr, msg);

    return true; // consumed
    UNUSED(local_addr);
}

void
ExternalRoutes::event_hna_route_in_expired(const OlsrTypes::ExternalID erid)
{
    delete_hna_route_in(erid);
}

void
ExternalRoute::update_timer(const TimeVal& expiry_time)
{
    XLOG_ASSERT(! _is_self_originated);

    if (_expiry_timer.scheduled())
	_expiry_timer.clear();

    _expiry_timer = _eventloop.
	new_oneoff_at(expiry_time,
		      callback(this, &ExternalRoute::event_expired));
}

void
ExternalRoute::event_expired()
{
    XLOG_ASSERT(! _is_self_originated);

    _parent.event_hna_route_in_expired(id());
}
