// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/plumbing.cc,v 1.41 2004/03/24 19:34:30 atanu Exp $"

// #define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "config.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/timer.hh"
#include "route_table_reader.hh"

#include "plumbing.hh"
#include "bgp.hh"

BGPPlumbing::BGPPlumbing(const Safi safi,
			 RibIpcHandler* ribhandler,
			 NextHopResolver<IPv4>& next_hop_resolver_ipv4,
			 NextHopResolver<IPv6>& next_hop_resolver_ipv6)
    : _rib_handler(ribhandler),
      _next_hop_resolver_ipv4(next_hop_resolver_ipv4),
      _next_hop_resolver_ipv6(next_hop_resolver_ipv6),
      _safi(safi),
      _plumbing_ipv4("[IPv4:" + string(pretty_string_safi(safi)) + "]", *this,
		     _next_hop_resolver_ipv4),
      _plumbing_ipv6("[IPv6:" + string(pretty_string_safi(safi)) + "]", *this, 
		     _next_hop_resolver_ipv6),
      _my_AS_number(AsNum::AS_INVALID)
{
}

void
BGPPlumbing::set_my_as_number(const AsNum &as_num) 
{
    _my_AS_number = as_num;
}

int
BGPPlumbing::add_peering(PeerHandler* peer_handler) 
{
    int result = 0;
    result |= plumbing_ipv4().add_peering(peer_handler);
    result |= plumbing_ipv6().add_peering(peer_handler);
    return result;
}

int
BGPPlumbing::stop_peering(PeerHandler* peer_handler) 
{
    debug_msg("BGPPlumbing::stop_peering\n");
    int result = 0;
    result |= plumbing_ipv4().stop_peering(peer_handler);
    result |= plumbing_ipv6().stop_peering(peer_handler);
    return result;
}

int
BGPPlumbing::peering_went_down(PeerHandler* peer_handler) 
{
    debug_msg("BGPPlumbing::peering_went_down\n");
    int result = 0;
    result |= plumbing_ipv4().peering_went_down(peer_handler);
    result |= plumbing_ipv6().peering_went_down(peer_handler);
    return result;
}

int 
BGPPlumbing::peering_came_up(PeerHandler* peer_handler) 
{
    debug_msg("BGPPlumbing::peering_came_up\n");
    int result = 0;
    result |= plumbing_ipv4().peering_came_up(peer_handler);
    result |= plumbing_ipv6().peering_came_up(peer_handler);
    return result;
}

int
BGPPlumbing::delete_peering(PeerHandler* peer_handler) 
{
    debug_msg("BGPPlumbing::delete_peering\n");
    int result = 0;
    result |= plumbing_ipv4().delete_peering(peer_handler);
    result |= plumbing_ipv6().delete_peering(peer_handler);
    return result;
}

int 
BGPPlumbing::add_route(const InternalMessage<IPv4> &rtmsg, 
		       PeerHandler* peer_handler) 
{
    debug_msg("BGPPlumbing::add_route IPv4\n");
    return plumbing_ipv4().add_route(rtmsg, peer_handler);
}

int 
BGPPlumbing::add_route(const InternalMessage<IPv6> &rtmsg, 
		       PeerHandler* peer_handler)  
{
    debug_msg("BGPPlumbing::add_route IPv6\n");
    return plumbing_ipv6().add_route(rtmsg, peer_handler);
}

int 
BGPPlumbing::delete_route(const InternalMessage<IPv4> &rtmsg, 
			  PeerHandler* peer_handler) 
{
    return plumbing_ipv4().delete_route(rtmsg, peer_handler);
}

int 
BGPPlumbing::delete_route(const InternalMessage<IPv6> &rtmsg, 
			  PeerHandler* peer_handler) 
{
    return plumbing_ipv6().delete_route(rtmsg, peer_handler);
}

int 
BGPPlumbing::delete_route(const IPNet<IPv4>& net,
			  PeerHandler* peer_handler) 
{
    return plumbing_ipv4().delete_route(net, peer_handler);
}

int 
BGPPlumbing::delete_route(const IPNet<IPv6>& net,
			  PeerHandler* peer_handler) 
{
    return plumbing_ipv6().delete_route(net, peer_handler);
}

const SubnetRoute<IPv4>* 
BGPPlumbing::lookup_route(const IPNet<IPv4> &net) const 
{
    return const_cast<BGPPlumbing *>(this)->
	plumbing_ipv4().lookup_route(net);
}

const SubnetRoute<IPv6>* 
BGPPlumbing::lookup_route(const IPNet<IPv6> &net) const 
{
    return const_cast<BGPPlumbing *>(this)->
	plumbing_ipv6().lookup_route(net);
}

template<>
void
BGPPlumbing::push<IPv4>(PeerHandler* peer_handler) 
{
    debug_msg("BGPPlumbing::push\n");
    plumbing_ipv4().push(peer_handler);
}

template<>
void
BGPPlumbing::push<IPv6>(PeerHandler* peer_handler) 
{
    debug_msg("BGPPlumbing::push\n");
    plumbing_ipv6().push(peer_handler);
}

void
BGPPlumbing::output_no_longer_busy(PeerHandler *peer_handler) 
{
    plumbing_ipv4().output_no_longer_busy(peer_handler);
    plumbing_ipv6().output_no_longer_busy(peer_handler);
}

uint32_t 
BGPPlumbing::create_ipv4_route_table_reader() 
{
    return plumbing_ipv4().create_route_table_reader();
}

uint32_t 
BGPPlumbing::create_ipv6_route_table_reader()
{
    return plumbing_ipv6().create_route_table_reader();
}

bool 
BGPPlumbing::read_next_route(uint32_t token, 
			     const SubnetRoute<IPv4>*& route, 
			     IPv4& peer_id)
{
    return plumbing_ipv4().read_next_route(token, route, peer_id);
}

bool 
BGPPlumbing::read_next_route(uint32_t token, 
			     const SubnetRoute<IPv6>*& route, 
			     IPv4& peer_id)
{
    return plumbing_ipv6().read_next_route(token, route, peer_id);
}

bool
BGPPlumbing::status(string& reason) const
{
    if (const_cast<BGPPlumbing *>(this)->
	plumbing_ipv4().status(reason) == false) {
	return false;
    }
    if (const_cast<BGPPlumbing *>(this)->
	plumbing_ipv6().status(reason) == false) {
	return false;
    }
    return true;
}

/***********************************************************************/

template <class A>
BGPPlumbingAF<A>::BGPPlumbingAF<A> (const string& ribname,
				    BGPPlumbing& master,
				    NextHopResolver<A>& next_hop_resolver)
    : _ribname(ribname), _master(master), _next_hop_resolver(next_hop_resolver)
{
    debug_msg("BGPPlumbingAF constructor called for RIB %s\n", 
	      ribname.c_str());
    _awaits_push = false;

    //We want to seed the route table reader token so that if BGP
    //restarts, an old token is unlikely to be accepted.
    _max_reader_token = getpid() << 16;

    /*
     * Initial plumbing is:
     *
     *     DecisionTable -> FanoutTable -> FilterTable -> CacheTable ->..
     *        ..-> RibOutTable -> RibIpcHandler
     *
     * This is the path taken by routes that we propagate to the
     * RIB process for local use.
     *
     * All the plumbing regarding BGP Peers gets added later.
     *
     * The RibIpcHandler resides in the master plumbing class.  The
     * rest is AF specific, so resides here.
     */

    _decision_table = 
	new DecisionTable<A>(ribname + "DecisionTable",
			     _master.safi(),
			     _next_hop_resolver);
    _next_hop_resolver.add_decision(_decision_table);

    _fanout_table = 
	new FanoutTable<A>(ribname + "FanoutTable", 
			   _master.safi(),
			   _decision_table);
    _decision_table->set_next_table(_fanout_table);

    /*
     * Plumb the input branch
     */
    
    _ipc_rib_in_table =
	new RibInTable<A>(_ribname + "IpcRibInTable",
			  _master.safi(),
			  _master.rib_handler());
    _in_map[_master.rib_handler()] = _ipc_rib_in_table;

    FilterTable<A>* filter_in =
	new FilterTable<A>(_ribname + "IpcChannelInputFilter",
			   _master.safi(),
			   _ipc_rib_in_table,
			   _next_hop_resolver);
    _ipc_rib_in_table->set_next_table(filter_in);
    
    CacheTable<A>* cache_in = 
	new CacheTable<A>(_ribname + "IpcChannelInputCache",
			  _master.safi(),
			  filter_in);
    filter_in->set_next_table(cache_in);

    cache_in->set_next_table(_decision_table);
    _decision_table->add_parent(cache_in, _master.rib_handler());

    _tables.insert(filter_in);
    _tables.insert(cache_in);

    /*
     * Plumb the output branch
     */

    FilterTable<A> *filter_out =
	new FilterTable<A>(ribname + "IpcChannelOutputFilter",
			   _master.safi(),
			   _fanout_table,
			   _next_hop_resolver);
    _fanout_table->add_next_table(filter_out, _master.rib_handler());
    _tables.insert(filter_out);

    CacheTable<A> *cache_out =
	new CacheTable<A>(ribname + "IpcChannelOutputCache",
			  _master.safi(),
			  filter_out);
    filter_out->set_next_table(cache_out);
    _tables.insert(cache_out);

    _ipc_rib_out_table =
	new RibOutTable<A>(ribname + "IpcRibOutTable",
			   _master.safi(),
			   cache_out,
			   _master.rib_handler());
    _out_map[_master.rib_handler()] = _ipc_rib_out_table;
    cache_out->set_next_table(_ipc_rib_out_table);
    
    _tables.insert(filter_out);
    _tables.insert(cache_out);
}

template <class A>
BGPPlumbingAF<A>::~BGPPlumbingAF<A>() 
{
    typename set <BGPRouteTable<A>*>::iterator i;
    for(i = _tables.begin(); i != _tables.end(); i++) {
	delete (*i);
    }
    delete _decision_table;
    delete _fanout_table;
    delete _ipc_rib_in_table;
    delete _ipc_rib_out_table;
}

template <class A>
int 
BGPPlumbingAF<A>::add_peering(PeerHandler* peer_handler) 
{
    /*
     * A new peer just came up.  We need to create all the RouteTables
     * to handle taking routes from this peer, and sending routes out
     * to the peer.
     *
     * New plumbing:
     *   RibInTable -> FilterTable -> CacheTable -> NhLookupTable ->
     *      DecisionTable.
     *
     *   FanoutTable -> FilterTable -> CacheTable ->..
     *        ..-> RibOutTable -> PeerHandler.
     *
     * XXX it's not clear how we configure the FilterTables yet.
     */

    string peername(peer_handler->peername());
    bool ibgp = peer_handler->ibgp();
    A my_nexthop(get_local_nexthop(peer_handler));


    /*
     * Plumb the input branch
     */
    
    RibInTable<A>* rib_in =
	new RibInTable<A>(_ribname + "RibIn" + peername,
			  _master.safi(),
			  peer_handler);
    _in_map[peer_handler] = rib_in;

    FilterTable<A>* filter_in =
	new FilterTable<A>(_ribname + "PeerInputFilter" + peername,
			   _master.safi(),
			   rib_in,
			   _next_hop_resolver);
    rib_in->set_next_table(filter_in);
    
    CacheTable<A>* cache_in = 
	new CacheTable<A>(_ribname + "PeerInputCache" + peername,
			  _master.safi(),
			  filter_in);
    filter_in->set_next_table(cache_in);

    NhLookupTable<A> *nexthop_in =
	new NhLookupTable<A>(_ribname + "NhLookup" + peername,
			     _master.safi(),
			     &_next_hop_resolver,
			     cache_in);
    cache_in->set_next_table(nexthop_in);

    nexthop_in->set_next_table(_decision_table);
    _decision_table->add_parent(nexthop_in, peer_handler);

    _tables.insert(rib_in);
    _tables.insert(filter_in);
    _tables.insert(cache_in);
    _tables.insert(nexthop_in);

    
    /*
     * Plumb the output branch
     */
    
    FilterTable<A>* filter_out =
	new FilterTable<A>(_ribname + "PeerOutputFilter" + peername,
			   _master.safi(),
			   _fanout_table,
			   _next_hop_resolver);
    _fanout_table->add_next_table(filter_out, peer_handler);
    
    CacheTable<A>* cache_out = 
	new CacheTable<A>(_ribname + "PeerOutputCache" + peername,
			  _master.safi(),
			  filter_out);
    filter_out->set_next_table(cache_out);

    RibOutTable<A>* rib_out =
	new RibOutTable<A>(_ribname + "RibOut" + peername,
			   _master.safi(),
			   cache_out,
			   peer_handler);
    cache_out->set_next_table(rib_out);
    _out_map[peer_handler] = rib_out;
    _reverse_out_map[rib_out] = peer_handler;

    _tables.insert(filter_out);
    _tables.insert(cache_out);
    _tables.insert(rib_out);

    
    /*
     * Start things up
     */

    const AsNum& his_AS_number = peer_handler->AS_number();
    const AsNum& my_AS_number = _master.my_AS_number();

    /* 1. configure the loop filters */
    filter_in->add_simple_AS_filter(my_AS_number);
    filter_out->add_simple_AS_filter(his_AS_number);

    /* 2. configure as_prepend filters for EBGP peers*/
    if (ibgp == false) {
	filter_out->add_AS_prepend_filter(my_AS_number);
    }

    /* 3. Configure MED filter.
	  Remove old MED and add new one on transmission to EBGP peers. */
    /* Note: this MUST come before the nexthop rewriter */
    if (ibgp == false) {
	filter_out->add_med_removal_filter();
	filter_out->add_med_insertion_filter();
    }

    /* 4. configure next_hop rewriter for EBGP peers*/
    if (ibgp == false) {
	filter_out->add_nexthop_rewrite_filter(my_nexthop);
    }

    /* 5. Configure local preference filter.
          Add LOCAL_PREF on receipt from EBGP peer. 
	  Remove it on transmission to EBGP peers. */
    if (ibgp == false) {
	filter_in->add_localpref_insertion_filter(
           LocalPrefAttribute::default_value() );

	filter_out->add_localpref_removal_filter();
    }

    /* 6. configure loop filter for IBGP peers */
    if (ibgp == true) {
	filter_out->add_ibgp_loop_filter();
    }

    /* 7. Process unknown attributes */
    filter_out->add_unknown_filter();

    /* 8. load up any configured filters */
    /* TBD */

    /* 9. load up damping filters */
    /* TBD */
    
    /* 10. cause the routing table to be dumped to the new peer */
    dump_entire_table(filter_out, _ribname);
    if(_awaits_push)
	push(peer_handler);

    return 0;
}

template <class A>
int 
BGPPlumbingAF<A>::stop_peering(PeerHandler* peer_handler) 
{

    /* Work our way back to the fanout table from the RibOut so we can
       find the relevant output from the fanout table.  On the way,
       flush any caches we find. */
    BGPRouteTable<A> *rt, *prevrt; 
    typename map <PeerHandler*, RibOutTable<A>*>::iterator iter;
    iter = _out_map.find(peer_handler);
    if (iter == _out_map.end()) 
	XLOG_FATAL("BGPPlumbingAF<IPv%u,%s>::stop_peering: peer %#x not found",
		   A::ip_version(),  pretty_string_safi(_master.safi()),
		   (u_int)peer_handler);
    rt = iter->second;
    prevrt = rt;
    while (rt != _fanout_table) {
	debug_msg("rt=%x (%s), _fanout_table=%x\n", 
	       (u_int)rt, rt->tablename().c_str(), (u_int)_fanout_table);
	if (rt->type() == CACHE_TABLE)
	    ((CacheTable<A>*)rt)->flush_cache();
	prevrt = rt;
	rt = rt->parent();
	if (rt == NULL) {
	    //peering was already stopped.  This can happen when we're
	    //doing an ALLSTOP.
// 	    XLOG_WARNING("BGPPlumbingAF<IPv%u>::stop_peering: "
// 			 "NULL parent table in stop_peering",
// 			 A::ip_version());
	    return 0;
	}
    }

    prevrt->set_parent(NULL);
    _fanout_table->remove_next_table(prevrt);
    return 0;
}

template <class A>
int 
BGPPlumbingAF<A>::peering_went_down(PeerHandler* peer_handler) 
{
    typename map <PeerHandler*, RibInTable<A>* >::iterator iter;
    iter = _in_map.find(peer_handler);
    if (iter == _in_map.end())
	XLOG_FATAL("BGPPlumbingAF<A>::peering_went_down: peer %p not found",
		   peer_handler);

    RibInTable<A> *rib_in;
    rib_in = iter->second;
    //peering went down will be propagated downstream by the RIB-In.
    rib_in->ribin_peering_went_down();

    //stop_peering shuts down and disconnects the output branch for this peer
    stop_peering(peer_handler);

    /* we don't flush the input caches - lookup requests should still
       be answered until the DeletionTable gets round to telling the
       downstream tables that the route has been deleted */

    return 0;
}

template <class A>
int 
BGPPlumbingAF<A>::peering_came_up(PeerHandler* peer_handler) 
{

    //plumb the output branch back into the fanout table
    BGPRouteTable<A> *rt, *prevrt;
    typename map <PeerHandler*, RibOutTable<A>*>::iterator iter;
    iter = _out_map.find(peer_handler);
    if (iter == _out_map.end()) 
	XLOG_FATAL("BGPPlumbingAF<A>::peering_came_up: peer %#x not found",
		(u_int)peer_handler);
    rt = iter->second;
    prevrt = rt;
    while (rt != NULL) {
	debug_msg("rt=%x (%s), _fanout_table=%x\n", 
	       (u_int)rt, rt->tablename().c_str(), (u_int)_fanout_table);
	prevrt = rt;
	rt = rt->parent();
    }

    debug_msg("type = %d", prevrt->type());
    FilterTable<A> *filter_out = dynamic_cast<FilterTable<A> *>(prevrt);
    XLOG_ASSERT(filter_out != NULL);

    _fanout_table->add_next_table(filter_out, peer_handler);
    filter_out->set_parent(_fanout_table);

    //bring the RibIn back up
    typename map <PeerHandler*, RibInTable<A>* >::iterator iter2;
    iter2 = _in_map.find(peer_handler);
    if (iter2 == _in_map.end())
	XLOG_FATAL("BGPPlumbingAF<A>::peering_went_down: peer %p not found",
		   peer_handler);
    RibInTable<A> *rib_in;
    rib_in = iter2->second;
    rib_in->ribin_peering_came_up();

    dump_entire_table(filter_out, _ribname);

    if(_awaits_push)
	push(peer_handler);
    return 0;
}

template <class A>
int 
BGPPlumbingAF<A>::delete_peering(PeerHandler* peer_handler) 
{
    debug_msg("BGPPlumbingAF<A>::drop_peering\n");

    BGPRouteTable<A> *rt, *parent, *child;

    /*
     * Steps:
     *  1. remove the relevant next_table link from the fanout table.
     *  2. delete add the routes in the RIB_In.
     *  3. remove the relevant parent table link from the decision table.
     *  4. tear down the state
     */

    /*
     * Step 1 - remove the relevant next_table link from the fanout table.
     * This stops us being able to send any updates to this peer.
     * We find the relevant link by tracking back from the RibOut.
     */

    stop_peering(peer_handler);

    /*
     * Step 2 - delete all the affected routes
     */
    
    peering_went_down(peer_handler);

    /*
     * Step 3 - remove the relevant parent link from the decision table
     */

    typename map <PeerHandler*, RibInTable<A>* >::iterator iter2;
    iter2 = _in_map.find(peer_handler);
    child = iter2->second;
    rt = child;
    while(child != _decision_table) {
	rt = child;
	child = rt->next_table();
    }
    _decision_table->remove_parent(rt);

    /* Step 4 - tear down all the tables for this peer */

    rt = iter2->second;
    while(rt != _decision_table) {
	child = rt->next_table();
	_tables.erase(rt);
	delete rt;
	rt = child;
    }

    typename map <PeerHandler*, RibOutTable<A>*>::iterator iter;
    iter = _out_map.find(peer_handler);
    if (iter == _out_map.end())
	XLOG_FATAL("BGPPlumbingAF<A>::drop_peering: peer %#x not found",
		(u_int)peer_handler);

    iter = _out_map.find(peer_handler);
    rt = iter->second;
    while(rt != NULL) {
	parent = rt->parent();
	if (rt->type() == CACHE_TABLE)
	    ((CacheTable<A>*)rt)->flush_cache();
	_tables.erase(rt);
	delete rt;
	rt = parent;
    }
    return 0;
}

template <class A>
void
BGPPlumbingAF<A>::dump_entire_table(FilterTable<A> *filter_out, string ribname)
{
    debug_msg("BGPPlumbingAF<IPv%u:%s>::dump_entire_table\n",
	      A::ip_version(), pretty_string_safi(_master.safi()));

    _fanout_table->dump_entire_table(filter_out, _master.safi(), ribname);

    DumpTable<A> *dump_table =
	dynamic_cast<DumpTable<A> *>(filter_out->parent());
    XLOG_ASSERT(dump_table);

    /*
    ** It is possible that another peer was in the middle of going
    ** down when this peering came up. So sweep through the peers and
    ** look for the deletion tables. Deletion tables can be
    ** nested. Treat them as if they have just sent a
    ** peering_went_down.
    */

    typename map <PeerHandler*, RibInTable<A>* >::iterator iter2;
    for (iter2 = _in_map.begin(); iter2 != _in_map.end(); iter2++) {
	RibInTable<A> *rib_in = iter2->second;
	debug_msg("<%s>\n", rib_in->next_table()->str().c_str());
	DeletionTable<A> *deletion_table =
	    dynamic_cast<DeletionTable<A> *>(rib_in->next_table());
	while (0 != deletion_table) {
	    debug_msg("Found a deletion table\n");
	    dump_table->peering_is_down(iter2->first, deletion_table->genid());

	    deletion_table = 
		dynamic_cast<DeletionTable<A> *>(deletion_table->next_table());
	}
    }
}

template <class A>
int 
BGPPlumbingAF<A>::add_route(const InternalMessage<A> &rtmsg, 
			    PeerHandler* peer_handler) 
{
    debug_msg("BGPPlumbingAF<IPv%u:%s>::add_route\n", A::ip_version(),
	      pretty_string_safi(_master.safi()));

    int result = 0;
    RibInTable<A> *rib_in;
    typename map <PeerHandler*, RibInTable<A>* >::iterator iter;
    iter = _in_map.find(peer_handler);
    if (iter == _in_map.end())
	XLOG_FATAL("BGPPlumbingAF<IPv%u:%s>: "
		   "add_route called for a PeerHandler "
		   "that has no associated RibIn", A::ip_version(),
		   pretty_string_safi(_master.safi()));

    rib_in = iter->second;

    result = rib_in->add_route(rtmsg, NULL);

    if (rtmsg.push() == false) {
	if (result == ADD_USED || result == ADD_UNUSED) {
	    _awaits_push = true;
	} else {
	    //XXX the add_route returned an error.
	    //Do we want to proactively send a push now?
	}
    }
    return result;
}

template <class A>
int 
BGPPlumbingAF<A>::delete_route(const InternalMessage<A> &rtmsg, 
			    PeerHandler* peer_handler) 
{
    int result = 0;
    RibInTable<A> *rib_in;
    typename map <PeerHandler*, RibInTable<A>* >::iterator iter;
    iter = _in_map.find(peer_handler);
    if (iter == _in_map.end())
	XLOG_FATAL("BGPPlumbingAF: delete_route called for a \
PeerHandler that has no associated RibIn");

    rib_in = iter->second;

    result = rib_in->delete_route(rtmsg, NULL);

    if (rtmsg.push() == false) {
	if (result == 0) {
	    _awaits_push = true;
	} else {
	    //XXX the delete_route returned an error.
	    //Do we want to proactively send a push now?
	}
    }
    return result;
}

template <class A>
int 
BGPPlumbingAF<A>::delete_route(const IPNet<A>& net,
			       PeerHandler* peer_handler) 
{
    int result = 0;
    RibInTable<A> *rib_in;
    typename map <PeerHandler*, RibInTable<A>* >::iterator iter;
    iter = _in_map.find(peer_handler);
    if (iter == _in_map.end())
	XLOG_FATAL("BGPPlumbingAF: delete_route called for a \
PeerHandler that has no associated RibIn");

    rib_in = iter->second;

    const SubnetRoute<A> *found_route = rib_in->lookup_route(net);
    if (found_route == NULL) {
	XLOG_WARNING("Attempt to delete non existent route %s",
		     net.str().c_str());
	return result;
    }
    InternalMessage<A> rtmsg(found_route, peer_handler, GENID_UNKNOWN);
    result = rib_in->delete_route(rtmsg, NULL);
    return result;
}

template <class A>
void
BGPPlumbingAF<A>::push(PeerHandler* peer_handler) 
{
    debug_msg("BGPPlumbingAF<IPv%u:%s>::push\n", A::ip_version(),
	      pretty_string_safi(_master.safi()));
    if (_awaits_push == false) {
	XLOG_WARNING("push <IPv%u:%s> when none needed",
		     A::ip_version(),
		     pretty_string_safi(_master.safi()));
	return;
    }
    RibInTable<A> *rib_in;
    typename map <PeerHandler*, RibInTable<A>* >::iterator iter;
    iter = _in_map.find(peer_handler);
    if (iter == _in_map.end())
	XLOG_FATAL("BGPPlumbingAF: Push called for a PeerHandler \
that has no associated RibIn");

    rib_in = iter->second;

    rib_in->push(NULL);
}

template <class A>
void
BGPPlumbingAF<A>::output_no_longer_busy(PeerHandler *peer_handler) 
{
    RibOutTable<A> *rib_out;
    typename map <PeerHandler*, RibOutTable<A>* >::iterator iter;
    iter = _out_map.find(peer_handler);
    if (iter == _out_map.end())
	XLOG_FATAL("BGPPlumbingAF: output_no_longer_busy called for a \
PeerHandler that has no associated RibOut");
    rib_out = iter->second;
    rib_out->output_no_longer_busy();
}

template <class A>
const SubnetRoute<A>* 
BGPPlumbingAF<A>::lookup_route(const IPNet<A> &net) const 
{
    //lookup_route returns the route currently being told to the RIB.
    //It's possible this differs from the route we tell a peer,
    //because of output filters that may modify attributes.
    return _ipc_rib_out_table->lookup_route(net);
}

const IPv4& 
BGPPlumbingAF<IPv4>::get_local_nexthop(const PeerHandler *peerhandler) const 
{
    return peerhandler->my_v4_nexthop();
}

const IPv6& 
BGPPlumbingAF<IPv6>::get_local_nexthop(const PeerHandler *peerhandler) const 
{
    return peerhandler->my_v6_nexthop();
}


template <class A>
list <RibInTable<A>*>
BGPPlumbingAF<A>::ribin_list() const 
{
    list <RibInTable<A>*> _ribin_list;
    typename map <PeerHandler*, RibInTable<A>* >::const_iterator i;
    for (i = _in_map.begin(); i != _in_map.end(); i++) {
	_ribin_list.push_back(i->second);
    }
    return _ribin_list;
}

template <class A>
uint32_t 
BGPPlumbingAF<A>::create_route_table_reader()
{
    //Generate a new token that can't clash with any in use, even if
    //the space wraps.
    _max_reader_token++;
    while (_route_table_readers.find(_max_reader_token) 
	   != _route_table_readers.end()) {
	_max_reader_token++;
    }

    RouteTableReader<A> *new_reader = new RouteTableReader<A>(ribin_list());
    _route_table_readers[_max_reader_token] = new_reader;
    return _max_reader_token;
}

template <class A>
bool 
BGPPlumbingAF<A>::read_next_route(uint32_t token, 
				  const SubnetRoute<A>*& route, 
				  IPv4& peer_id) 
{
    typename map <uint32_t, RouteTableReader<A>*>::iterator i;
    i = _route_table_readers.find(token);
    if (i == _route_table_readers.end())
	return false;
    RouteTableReader<A> *_reader = i->second;
    bool result = _reader->get_next(route, peer_id);
    if (result == false) {
	//we've finished reading the routing table.
	_route_table_readers.erase(i);
	delete _reader;
    }
    return result;
}

template <class A>
bool
BGPPlumbingAF<A>::status(string&) const
{
    return true;
}

template class BGPPlumbingAF<IPv4>;
template class BGPPlumbingAF<IPv6>;
