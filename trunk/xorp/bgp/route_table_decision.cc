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

#ident "$XORP: xorp/bgp/route_table_decision.cc,v 1.16 2003/10/22 10:58:26 mjh Exp $"

//#define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "route_table_decision.hh"

//#define DEBUG_CODEPATH
#ifdef DEBUG_CODEPATH
#define cp(x) printf("DecisionCodePath: %2d\n", x)
#else
#define cp(x) {}
#endif

template<class A>
DecisionTable<A>::DecisionTable(string table_name, 
				NextHopResolver<A>& next_hop_resolver) 
    : BGPRouteTable<A>("DecisionTable" + table_name),
    _next_hop_resolver(next_hop_resolver)
{
    cp(1);
}

template<class A>
int
DecisionTable<A>::add_parent(BGPRouteTable<A> *new_parent,
			     PeerHandler *peer_handler) {
    debug_msg("DecisionTable<A>::add_parent: %x\n", (u_int)new_parent);
    if (_parents.find(new_parent)!=_parents.end()) {
	//the parent is already in the set
	cp(2);
	return -1;
    }
    cp(3);
    _parents.insert(make_pair(new_parent, peer_handler));
    return 0;
}

template<class A>
int
DecisionTable<A>::remove_parent(BGPRouteTable<A> *ex_parent) {
    debug_msg("DecisionTable<A>::remove_parent: %x\n", (u_int)ex_parent);
    _parents.erase(_parents.find(ex_parent));
    cp(4);
    return 0;
}


template<class A>
int
DecisionTable<A>::add_route(const InternalMessage<A> &rtmsg, 
			    BGPRouteTable<A> *caller) {
    assert(_parents.find(caller) != _parents.end());

    debug_msg("DT:add_route %s\n", rtmsg.route()->str().c_str());

    //if the nexthop isn't resolvable, don't event consider the route
    debug_msg("testing resolvability\n");
    if (!resolvable(rtmsg.route()->nexthop())) {
	cp(5);
	debug_msg("route not resolvable\n");
    	return ADD_UNUSED;
    }
    debug_msg("route resolvable\n");

    //find the alternative routes, and the old winner if there was one.
    RouteData<A> *old_winner = NULL, *old_winner_clone = NULL;
    list<RouteData<A> > alternatives;
    old_winner = find_alternative_routes(caller, rtmsg.net(), alternatives);

    //preserve old_winner because the original may be deleted by the
    //decision process
    if (old_winner != NULL) {
	old_winner_clone = new RouteData<A>(*old_winner);
    }
    
    RouteData<A> *new_winner = NULL;
    RouteData<A> new_route(rtmsg.route(), caller, rtmsg.origin_peer());
    if (!alternatives.empty()) {
	//add the new route to the pool of possible winners.
	alternatives.push_back(new_route);
	new_winner = find_winner(alternatives);
    } else {
	//the new route wins by default
	new_winner = &new_route;
    }
    assert(new_winner != NULL);

    if (old_winner_clone != NULL) {
	if (old_winner_clone->route() == new_winner->route()) {
	    //the winner didn't change.
	    assert(old_winner_clone != NULL);
	    delete old_winner_clone;
	    return ADD_UNUSED;
	}

	//the winner did change, so send a delete for the old winner
	InternalMessage<A> old_rt_msg(old_winner_clone->route(), 
				      old_winner_clone->peer_handler(), 
				      GENID_UNKNOWN);
	_next_table->delete_route(old_rt_msg, (BGPRouteTable<A>*)this);

	//the old winner is no longer the winner
	old_winner_clone->set_is_not_winner();

	//clean up temporary state
	delete old_winner_clone;
    }

    //send an add for the new winner
    new_winner->route()->set_is_winner(
        igp_distance(new_winner->route()->nexthop()));
    int result;
    if (new_winner->route() != rtmsg.route()) {
	//we have a new winner, but it isn't the route that was just added.
	//this can happen due to MED wierdness.
	InternalMessage<A> new_rt_msg(new_winner->route(), 
				      new_winner->peer_handler(), 
				      GENID_UNKNOWN);
	if (rtmsg.push())
	    new_rt_msg.set_push();
	result = _next_table->add_route(new_rt_msg, 
					(BGPRouteTable<A>*)this);
    } else {
	result = _next_table->add_route(rtmsg, 
					(BGPRouteTable<A>*)this);
    }

    if (result == ADD_UNUSED) {
	//if it got as far as the decision table, we declare it
	//used, even if it gets filtered downstream.
	result = ADD_USED;
    }
    return result;
}

template<class A>
int
DecisionTable<A>::replace_route(const InternalMessage<A> &old_rtmsg, 
				const InternalMessage<A> &new_rtmsg, 
				BGPRouteTable<A> *caller) {
    assert(_parents.find(caller)!=_parents.end());
    assert(old_rtmsg.net()==new_rtmsg.net());

    debug_msg("DT:replace_route.\nOld route: %s\nNew Route: %s\n", old_rtmsg.route()->str().c_str(), new_rtmsg.route()->str().c_str());
    //if the new nexthop isn't resolvable, don't event consider the route
    if (!resolvable(new_rtmsg.route()->nexthop())) {
	cp(12);
	debug_msg("new route isn't resolvable\n");
    	return delete_route(old_rtmsg, caller);
    }

    list <RouteData<A> > alternatives;
    RouteData<A>* old_winner 
	= find_alternative_routes(caller, old_rtmsg.net(), alternatives);
    if (old_winner) {
	//preserve this, because the original old_winner's data will
	//be clobbered by route_wins.
	old_winner = new RouteData<A>(*old_winner);
    }
    
    bool old_won, new_won;

    old_won = old_rtmsg.route()->is_winner();

    if (route_wins(new_rtmsg.route(), new_rtmsg.origin_peer(),
		   alternatives)) {
	new_won = true;
    } else {
	new_won = false;
    }

    //keep track of changes in winner
    if (new_won) {
	//do old before new, because it's possible they're the same route
	if (old_winner != NULL) {
	    cp(18);
	    old_winner->set_is_not_winner();
	} else {
	    cp(19);
	    old_rtmsg.route()->set_is_not_winner();
	}
	new_rtmsg.route()
	    ->set_is_winner(igp_distance(new_rtmsg.route()->nexthop()));
    }

    int result = ADD_USED;
    if (old_won)
	debug_msg("old won\n");

    if (new_won)
	debug_msg("new won\n");

    if ((old_won == false) && (new_won == false)) {
	cp(20);
	debug_msg("case 1\n");
	//case 1: neither the old or new version won.
	if (old_winner)
	    delete old_winner;
	return ADD_UNUSED;
    } else if ((old_won == true) && (new_won == true)) {
	cp(21);
	debug_msg("case 2\n");
	//case 2: this peer won before, and it wins again
	result = _next_table->replace_route(old_rtmsg, new_rtmsg,
					    (BGPRouteTable<A>*)this);
    } else if ((old_won == true) && (new_won == false)) {
	cp(22);
	debug_msg("case 3\n");
	//case 3: this peer won before, but now it loses,
	_next_table->delete_route(old_rtmsg, (BGPRouteTable<A>*)this);
	find_alternative_routes(caller, old_rtmsg.net(), alternatives);
	RouteData<A>* new_winner = find_winner(alternatives);
	assert(new_winner != NULL);
	InternalMessage<A> alt_rtmsg(new_winner->route(), 
				     new_winner->peer_handler(),
				     GENID_UNKNOWN);
	result = _next_table->add_route(alt_rtmsg, 
					(BGPRouteTable<A>*)this);
	//inform the RibIn the route is now being used.
	new_winner->set_is_winner(igp_distance(new_winner->route()
					       ->nexthop()));
    } else {
	cp(23);
	//case 4: this peer lost before, but now it wins,
	assert((old_won == false) && (new_won == true));

	//there may not be an old_winner if the none of the previous
	//routes were resolvable
	if(old_winner != NULL) {
	    cp(24);
	    InternalMessage<A> alt_rtmsg(old_winner->route(), 
					 old_winner->peer_handler(), 
					 GENID_UNKNOWN);
	    _next_table->delete_route(alt_rtmsg, 
				      (BGPRouteTable<A>*)this);
	    //inform the RibIn the route is no longer being used.
	    old_winner->set_is_not_winner();
	} else {
	    cp(25);
	}
	result = _next_table->add_route(new_rtmsg, (BGPRouteTable<A>*)this);
    }

    if (old_winner)
	delete old_winner;

    if (result == ADD_USED || result == ADD_UNUSED) {
	cp(26);
	//if it got as far as the decision table, we declare it
	//used, even if it gets filtered downstream.
	return ADD_USED;
    } else {
	cp(27);
	//some sort of failure, so return the error code directly.
	return result;
    }
}

template<class A>
int
DecisionTable<A>::delete_route(const InternalMessage<A> &rtmsg, 
			       BGPRouteTable<A> *caller) {

    debug_msg("delete route: %s\n",
	      rtmsg.route()->str().c_str());
    assert(_parents.find(caller) != _parents.end());
    assert(_next_table != NULL);

    //find the alternative routes, and the old winner if there was one.
    RouteData<A> *old_winner = NULL, *old_winner_clone = NULL;
    list<RouteData<A> > alternatives;
    old_winner = find_alternative_routes(caller, rtmsg.net(), alternatives);

    //preserve old_winner because the original may be deleted by the
    //decision process
    if (old_winner != NULL) {
	old_winner_clone = new RouteData<A>(*old_winner);
	debug_msg("The Old winner was %s\n", 
		  old_winner->route()->str().c_str());
    } else if (rtmsg.route()->is_winner()) {
	//the route being deleted was the old winner
	old_winner_clone = new RouteData<A>(rtmsg.route(), caller,
					    rtmsg.origin_peer());
    }
    
    RouteData<A> *new_winner = NULL;
    if (!alternatives.empty()) {
	//add the new route to the pool of possible winners.
	new_winner = find_winner(alternatives);
    }

    if (old_winner_clone == NULL && new_winner == NULL) {
	//there are no resolvable routes, and there weren't before either/
	//nothing to do.
	return -1;
    }
    bool delayed_push = rtmsg.push();
    if (old_winner_clone != NULL) {
	if (new_winner != NULL
	    && old_winner_clone->route() == new_winner->route()) {
	    //the winner didn't change.
	    assert(old_winner_clone != NULL);
	    delete old_winner_clone;
	    return -1;
	}

	//the winner did change, or there's no new winner, so send a
	//delete for the old winner
	if (old_winner_clone->route() != rtmsg.route()) {
	    InternalMessage<A> old_rt_msg(old_winner_clone->route(), 
					  old_winner_clone->peer_handler(), 
					  GENID_UNKNOWN);
	    if (rtmsg.push() && new_winner == NULL)
		old_rt_msg.set_push();
	    _next_table->delete_route(old_rt_msg, (BGPRouteTable<A>*)this);
	    old_winner_clone->set_is_not_winner();
	} else {
	    if (new_winner != NULL)
		rtmsg.force_clear_push();
	    _next_table->delete_route(rtmsg, (BGPRouteTable<A>*)this);
	    rtmsg.route()->set_is_not_winner();
	}

	//clean up temporary state
	delete old_winner_clone;
    }

    if (new_winner != NULL) {
	//send an add for the new winner
	new_winner->route()->set_is_winner(
		   igp_distance(new_winner->route()->nexthop()));
	int result;
	InternalMessage<A> new_rt_msg(new_winner->route(), 
				      new_winner->peer_handler(), 
				      GENID_UNKNOWN);
	//	if (rtmsg.push())
	//	    new_rt_msg.set_push();
	result = _next_table->add_route(new_rt_msg, 
					(BGPRouteTable<A>*)this);
	if (delayed_push)
	    _next_table->push((BGPRouteTable<A>*)this);
    }

    return 0;

#if 0
    if (rtmsg.route()->is_winner() == false) {
	cp(28);
	//the route wasn't the winner before, so we don't need to do anything
	return -1;
    }

    //it's being deleted, so it's no longer the winner
    rtmsg.route()->set_is_not_winner();

    const SubnetRoute<A>* best_route;
    const PeerHandler *best_routes_peer;
    BGPRouteTable<A> *best_routes_parent;
    best_route = lookup_route(NULL, rtmsg.net(), best_routes_peer, 
			      best_routes_parent);

    if (best_route != NULL) {
	cp(29);
	//the best alternative route cannot have been the winner,
	//because we're only here if the route being deleted was the
	//winner
	assert(best_route->is_winner() == false);

	//it is now the new winner
	best_route->set_is_winner(igp_distance(best_route->nexthop()));
    }

    //propagate the delete downstream.
    int del_result = 0;
    bool delayed_push = false;
    if (best_route != NULL && rtmsg.push()) {
	cp(30);
	//we clear the push bit because we're going to have to send
	//an add_route to replace this route being deleted
	rtmsg.force_clear_push();
	delayed_push = true;
    }
    del_result = _next_table->delete_route(rtmsg, (BGPRouteTable<A>*)this);

    if (best_route != NULL) {
	cp(31);
	//We deleted a route, but there was an alternative
	//route.  We need to propagate the alternative route
	//downstream.
	PeerHandler *peer_handler;
	peer_handler = _parents[best_routes_parent];
	InternalMessage<A> alt_rtmsg(best_route, 
				     peer_handler,
				     GENID_UNKNOWN);
	int add_result;
	add_result = _next_table->add_route(alt_rtmsg, 
					    (BGPRouteTable<A>*)this);
	if (add_result == ADD_USED) {
	    cp(32);
	    //now push the information back upstream that the route
	    //has actually been used.
	    best_routes_parent->route_used(best_route, true);
	}
    }

    if (delayed_push) {
	cp(33);
	_next_table->push((BGPRouteTable<A>*)this);
    }
	
    return del_result;
#endif
}

template<class A>
int
DecisionTable<A>::push(BGPRouteTable<A> *caller) {
    assert(_parents.find(caller) != _parents.end());

    cp(34);
    if (_next_table != NULL)
	return _next_table->push((BGPRouteTable<A>*)this);
    return 0;
}

/**
 * This version of lookup_route finds the previous winner if there was
 * one, else it finds the best alternative.  Note that the previous
 * winner might not actually be the best current winner, but in this
 * context we need to be consistent - if it won before, then it still
 * wins until a delete_route or replace_route arrives to update our
 * idea of the winner */

template<class A>
const SubnetRoute<A>*
DecisionTable<A>::lookup_route(const BGPRouteTable<A>* ignore_parent,
			       const IPNet<A> &net,
			       const PeerHandler*& best_routes_peer,
			       BGPRouteTable<A>*& best_routes_parent) const
{
    list <RouteData<A> > alternatives;
    RouteData<A>* winner
	= find_alternative_routes(ignore_parent, net, alternatives);
    if (winner == NULL  && !alternatives.empty()) {
	winner = find_winner(alternatives);
    }
    if (winner != NULL) {
	best_routes_peer = winner->peer_handler();
	best_routes_parent = winner->parent_table();
	return winner->route();
    }
    return NULL;
}

template<class A>
const SubnetRoute<A>*
DecisionTable<A>::lookup_route(const IPNet<A> &net) const
{
    list <RouteData<A> > alternatives;
    RouteData<A>* winner = find_alternative_routes(NULL, net, alternatives);
    if (winner == NULL)
	return NULL;
    else
	return winner->route();
}


template<class A>
RouteData<A>*
DecisionTable<A>::find_alternative_routes(
    const BGPRouteTable<A> *caller,
    const IPNet<A>& net,
    list <RouteData<A> >& alternatives) const 
{
    RouteData<A>* previous_winner = NULL;
    typename map<BGPRouteTable<A>*, PeerHandler * >::const_iterator i;
    const SubnetRoute<A>* found_route;
    for (i = _parents.begin();  i != _parents.end();  i++) {
	//We don't need to lookup the route in the parent that the new
	//route came from - if this route replaced an earlier route
	//from the same parent we'd see it as a replace, not an add
 	if (i->first != caller) {
	    found_route = i->first->lookup_route(net);
	    if (found_route != NULL) {
		alternatives.push_back(RouteData<A>(found_route, 
						    i->first, i->second));
		if (found_route->is_winner()) {
		    assert(previous_winner == NULL);
		    previous_winner = &(alternatives.back());
		}
	    }
	}
    }
    return previous_winner;
}

template<class A>
uint32_t
DecisionTable<A>::local_pref(const SubnetRoute<A> *route,
			     const PeerHandler *peer) const
{
    /*
     * Local Pref should be present on all routes.  If the route comes
     * from EBGP, the incoming FilterTable should have added it.  If
     * the route comes from IBGP, it should have been present on the
     * incoming route.  
     */
    cp(55);
    const PathAttributeList<A> *attrlist = route->attributes();
    list<PathAttribute*>::const_iterator i;
    for(i = attrlist->begin(); i != attrlist->end(); i++) {
	if(LOCAL_PREF == (*i)->type()) {
	    cp(56);
	    return (dynamic_cast<const LocalPrefAttribute *>(*i))->localpref();
	} else {
	    cp(57);
	}
    }
    XLOG_WARNING("No LOCAL_PREF present %s", peer->peername().c_str());
    cp(58);

    return 0;
}

template<class A>
uint32_t
DecisionTable<A>::med(const SubnetRoute<A> *route) const
{
    const PathAttributeList<A> *attrlist = route->attributes();
    list<PathAttribute*>::const_iterator i;
    for(i = attrlist->begin(); i != attrlist->end(); i++) {
	if(MED == (*i)->type()) {
	    cp(59);
	    return (dynamic_cast<const MEDAttribute *>(*i))->
		med();
	}
    }
    cp(60);

    return 0;
}

/*
** Is this next hop resolvable. Ask the RIB via the next hop resolver.
** NOTE: If unsynchronised operation is required then this is the
** place to always return true.
*/
template<class A>
bool
DecisionTable<A>::resolvable(const A nexthop) const
{
    bool resolvable;
    uint32_t metric;

    if(!_next_hop_resolver.lookup(nexthop, resolvable, metric))
	XLOG_FATAL("This next hop must be known %s", nexthop.str().c_str());

    cp(61);
    return resolvable;
}

/* 
** Get the igp distance from the RIB via the next hop resolver.
*/
template<class A>
uint32_t
DecisionTable<A>::igp_distance(const A nexthop) const
{
    bool resolvable;
    uint32_t metric;

    if(!_next_hop_resolver.lookup(nexthop, resolvable, metric))
	XLOG_FATAL("This next hop must be known %s", nexthop.str().c_str());

    if (resolvable)
	debug_msg("Decision: IGP distance for %s is %d\n",
		  nexthop.str().c_str(),
		  metric);
    else
	debug_msg("Decision: IGP distance for %s is unknown\n",
		  nexthop.str().c_str());

    cp(62);
    return metric;
}

/*
** The main decision process.
**
** return true if test_route is better than current_route.
**        resolved is set to true if the winning route resolves.
*/
template<class A>
bool 
DecisionTable<A>::route_wins(const SubnetRoute<A> *test_route,
			     const PeerHandler *test_peer,
			     list<RouteData<A> >& alternatives) const
{
    typename list<RouteData<A> >::iterator i;

    /* The spec seems pretty odd.  In our architecture, it seems
       reasonable to do phase 2 before phase 1, because if a route
       isn't resolvable we don't want to consider it */

    /*
    ** Phase 2: Route Selection.  */
    /*
    ** Check if routes resolve.
    */
    bool test_resolvable = resolvable(test_route->nexthop());

    /*We should never even call route_is_better if the test_route
      isn't resolvable */
    assert(test_resolvable);

    for (i=alternatives.begin(); i!=alternatives.end();) {
	if (!resolvable(i->route()->nexthop())) {
	    i = alternatives.erase(i);
	} else {
	    i++;
	}
    }
    
    /* If there are no resolvable alternatives, the test route wins */
    if (alternatives.empty()) {
	return true;
    }

    /* By this point, all remaining routes are resolvable */

    /* 
    ** Phase 1: Calculation of degree of preference.
    */
    int test_pref = local_pref(test_route, test_peer);
    for (i=alternatives.begin(); i!=alternatives.end();) {
	int lp = local_pref(i->route(), i->peer_handler());
	assert(lp >= 0);
	//prefer higher preference
	if (lp < test_pref) {
	    i = alternatives.erase(i);
	} else if (lp > test_pref) {
	    return false;
	} else {
	    i++;
	}
    }

    if(alternatives.empty()) {
	return true;
    }

    /*
    ** If we got here the preferences for both routes are the same.
    */

    /*
    ** Here we are crappy tie breaking.
    */
    
    debug_msg("tie breaking\n");
    debug_msg("AS path test\n");
    /*
    ** Shortest AS path length.
    */
    int test_aspath_length = test_route->attributes()->
	aspath().path_length();
    debug_msg("%s length %d\n", test_route->attributes()->
	      aspath().str().c_str(), test_aspath_length);

    for (i=alternatives.begin(); i!=alternatives.end();) {
	int len = i->route()->attributes()->aspath().path_length();
	assert(len >= 0);
	//prefer shortest path
	if (len > test_aspath_length) {
	    i = alternatives.erase(i);
	} else if (len < test_aspath_length) {
	    return false;
	} else {
	    i++;
	}
    }

    
    if(alternatives.empty()) {
	cp(66);
	return true;
    }

    debug_msg("origin test\n");
    /*
    ** Lowest origin value.
    */
    int test_origin = test_route->attributes()->origin();
    for (i=alternatives.begin(); i!=alternatives.end();) {
	int origin = i->route()->attributes()->origin();
	//prefer lower origin
	if (origin > test_origin) {
	    i = alternatives.erase(i);
	} else if (origin < test_origin) {
	    return false;
	} else {
	    i++;
	}
    }

    if(alternatives.empty()) {
	return true;
    }

    debug_msg("MED test\n");
    /*
    ** Compare meds if both routes came from the same neighbour AS.
    */

    AsNum test_asnum(test_route->attributes()->aspath().first_asnum());
    int test_med = med(test_route);
    for (i=alternatives.begin(); i!=alternatives.end();) {
	AsNum asnum = (i->route()->attributes()->aspath().first_asnum());
	int alt_med = med(i->route());
	if (test_asnum == asnum) {
	    //prefer lower MED
	    if (test_med < alt_med) {
		i = alternatives.erase(i);
	    } else if (test_med > alt_med) {
		return false;
	    } else {
		i++;
	    }
	} else {
	    i++;
	}
    }

    if(alternatives.empty()) {
	return true;
    }

    //remove any other routes from consideration if they come from the
    //same AS as another route, and loses on MED to that other route.
    typename list <RouteData<A> >::iterator j;
    for (i=alternatives.begin(); i!=alternatives.end();) {
	AsNum asnum1 = (i->route()->attributes()->aspath().first_asnum());
	int med1 = med(i->route());
	bool del_i = false;
	for (j=alternatives.begin(); j!=alternatives.end();) {
	    bool del_j = false;
	    if (i != j) {
		AsNum asnum2 
		    = (j->route()->attributes()->aspath().first_asnum());
		int med2 = med(j->route());
		if (asnum1 == asnum2) {
		    if (med1 > med2) {
			i = alternatives.erase(i);
			del_i = true;
			break;
		    } else if (med1 < med2) {
			j = alternatives.erase(j);
			del_j = true;
		    }
		} 
	    }
	    //move to the next j if we didn't already do it using erase
	    if (del_j == false) {
		j++;
	    }
	}	
	//move to the next i if we didn't already do it using erase
	if (del_i == false) {
	    i++;
	}
    }

    if(alternatives.empty()) {
	//the loop above can't remove the last alternative
	XLOG_UNREACHABLE();
    }


    //Sanity checking.  If this conditions is hit, then the
    //input sanity checks haven't done their job.
    if (!test_peer->ibgp() && (test_asnum != test_peer->AS_number()))
	XLOG_FATAL("AS Path %s received from EBGP peer with AS %s\n",
		   test_route->attributes()->aspath().str().c_str(),
		   test_peer->AS_number().str().c_str());
    
    debug_msg("EBGP vs IBGP test\n");
    /*
    ** Prefer routes from external peers over internal peers.
    */
    bool test_ibgp = test_peer->ibgp();
    for (i=alternatives.begin(); i!=alternatives.end();) {
	bool ibgp = i->peer_handler()->ibgp();
	if ((!test_ibgp) && ibgp) {
	    //test route is external, alternative is internal
	    i = alternatives.erase(i);
	} else if (test_ibgp && !ibgp) {
	    //test route is internal, alternative is external
	    return false;
	} else {
	    i++;
	}
    }

    if (alternatives.empty()) {
	return true;
    }


    debug_msg("IGP distance test\n");
    /*
    ** Compare IGP distances.
    */
    int test_igp_distance = igp_distance(test_route->nexthop());
    for (i=alternatives.begin(); i!=alternatives.end(); ) {
	int igp_dist = igp_distance(i->route()->nexthop());
	//prefer lower IGP distance
	if (test_igp_distance < igp_dist) {
	    i = alternatives.erase(i);
	} else if (test_igp_distance > igp_dist) {
	    return false;
	} else {
	    i++;
	}
    }

    if (alternatives.empty()) {
	return true;
    }


    debug_msg("BGP ID test\n");
    /*
    ** Choose the route from the neighbour with the lowest BGP ID.
    */
    IPv4 test_id = test_peer->id();
    for (i=alternatives.begin(); i!=alternatives.end();) {
	IPv4 id = i->peer_handler()->id();
	//prefer lower ID distance
	if (test_id < id) {
	    i = alternatives.erase(i);
	} else if (test_id > id) {
	    return false;
	} else {
	    i++;
	}
    }

    if (alternatives.empty()) {
	return true;
    }


    debug_msg("neighbour addr test\n");
    /*
    ** Choose the route from the neighbour with the lowest neighbour
    ** address.
    */
    int test_address = test_peer->neighbour_address();
    for (i=alternatives.begin(); i!=alternatives.end();) {
	int address = i->peer_handler()->neighbour_address();
	//prefer lower address
	if (test_address < address) {
	    i = alternatives.erase(i);
	} else if (test_address > address) {
	    return false;
	} else {
	    i++;
	}
    }

    if (alternatives.empty()) {
	return true;
    }

    //We can get here if we compare two identically rated routes.  
    return false;
}

/*
** The main decision process.
**
** return true if test_route is better than current_route.
**        resolved is set to true if the winning route resolves.
*/
template<class A>
RouteData<A>*
DecisionTable<A>::find_winner(list<RouteData<A> >& alternatives) const
{
    typename list<RouteData<A> >::iterator i;

    /* The spec seems pretty odd.  In our architecture, it seems
       reasonable to do phase 2 before phase 1, because if a route
       isn't resolvable we don't want to consider it */

    /*
    ** Phase 2: Route Selection.  */
    /*
    ** Check if routes resolve.
    */
    for (i=alternatives.begin(); i!=alternatives.end();) {
	if (!resolvable(i->route()->nexthop())) {
	    i = alternatives.erase(i);
	} else {
	    i++;
	}
    }
    
    /* If there are no resolvable alternatives, no-one wins */
    if (alternatives.empty()) {
	return NULL;
    }
    if (alternatives.size()==1) {
	return &(alternatives.front());
    }

    /* By this point, all remaining routes are resolvable */

    /* 
    ** Phase 1: Calculation of degree of preference.
    */
    int test_pref = local_pref(alternatives.front().route(), 
			       alternatives.front().peer_handler());
    i = alternatives.begin(); i++;
    while(i!=alternatives.end()) {
	int lp = local_pref(i->route(), i->peer_handler());
	assert(lp >= 0);
	//prefer higher preference
	if (lp < test_pref) {
	    i = alternatives.erase(i);
	} else if (lp > test_pref) {
	    test_pref = lp;
	    alternatives.pop_front();
	    i++;
	} else {
	    i++;
	}
    }

    if (alternatives.size()==1) {
	return &(alternatives.front());
    }
	  

    /*
    ** If we got here the preferences for both routes are the same.
    */

    /*
    ** Here we are crappy tie breaking.
    */
    
    debug_msg("tie breaking\n");
    debug_msg("AS path test\n");
    /*
    ** Shortest AS path length.
    */
    int test_aspath_length = alternatives.front().route()->attributes()->
	aspath().path_length();

    i = alternatives.begin(); i++;
    while(i!=alternatives.end()) {
	int len = i->route()->attributes()->aspath().path_length();
	assert(len >= 0);
	//prefer shortest path
	if (len > test_aspath_length) {
	    i = alternatives.erase(i);
	} else if (len < test_aspath_length) {
	    test_aspath_length = len;
	    alternatives.pop_front();
	} else {
	    i++;
	}
    }

    
    if (alternatives.size()==1) {
	return &(alternatives.front());
    }

    debug_msg("origin test\n");
    /*
    ** Lowest origin value.
    */
    int test_origin = alternatives.front().route()->attributes()->origin();
    i = alternatives.begin(); i++;
    while(i!=alternatives.end()) {
	int origin = i->route()->attributes()->origin();
	//prefer lower origin
	if (origin > test_origin) {
	    i = alternatives.erase(i);
	} else if (origin < test_origin) {
	    test_origin = origin;
	    alternatives.pop_front();
	    i++;
	} else {
	    i++;
	}
    }

    if (alternatives.size()==1) {
	return &(alternatives.front());
    }

    debug_msg("MED test\n");
    /*
    ** Compare meds if both routes came from the same neighbour AS.
    */
    typename list <RouteData<A> >::iterator j;
    for (i=alternatives.begin(); i!=alternatives.end();) {
	AsNum asnum1 = (i->route()->attributes()->aspath().first_asnum());
	int med1 = med(i->route());
	bool del_i = false;
	for (j=alternatives.begin(); j!=alternatives.end();) {
	    bool del_j = false;
	    if (i != j) {
		AsNum asnum2 
		    = (j->route()->attributes()->aspath().first_asnum());
		int med2 = med(j->route());
		if (asnum1 == asnum2) {
		    if (med1 > med2) {
			i = alternatives.erase(i);
			del_i = true;
			break;
		    } else if (med1 < med2) {
			j = alternatives.erase(j);
			del_j = true;
		    }
		} 
	    }
	    //move to the next j if we didn't already do it using erase
	    if (del_j == false) {
		j++;
	    }
	}	
	//move to the next i if we didn't already do it using erase
	if (del_i == false) {
	    i++;
	}
    }

    if (alternatives.size()==1) {
	return &(alternatives.front());
    }


    debug_msg("EBGP vs IBGP test\n");
    /*
    ** Prefer routes from external peers over internal peers.
    */
    bool test_ibgp = alternatives.front().peer_handler()->ibgp();
    i = alternatives.begin(); i++;
    while(i!=alternatives.end()) {
	bool ibgp = i->peer_handler()->ibgp();
	if ((!test_ibgp) && ibgp) {
	    //test route is external, alternative is internal
	    i = alternatives.erase(i);
	} else if (test_ibgp && !ibgp) {
	    //test route is internal, alternative is external
	    alternatives.pop_front();
	    test_ibgp = ibgp;
	    i++;
	} else {
	    i++;
	}
    }

    if (alternatives.size()==1) {
	return &(alternatives.front());
    }


    debug_msg("IGP distance test\n");
    /*
    ** Compare IGP distances.
    */
    int test_igp_distance = igp_distance(alternatives.front().route()->nexthop());
    i = alternatives.begin(); i++;
    while(i!=alternatives.end()) {
	int igp_dist = igp_distance(i->route()->nexthop());
	//prefer lower IGP distance
	if (test_igp_distance < igp_dist) {
	    i = alternatives.erase(i);
	} else if (test_igp_distance > igp_dist) {
	    alternatives.pop_front();
	    test_igp_distance = igp_dist;
	    i++;
	} else {
	    i++;
	}
    }

    if (alternatives.size()==1) {
	return &(alternatives.front());
    }


    debug_msg("BGP ID test\n");
    /*
    ** Choose the route from the neighbour with the lowest BGP ID.
    */
    IPv4 test_id = alternatives.front().peer_handler()->id();
    i = alternatives.begin(); i++;
    while(i!=alternatives.end()) {
	IPv4 id = i->peer_handler()->id();
	//prefer lower ID distance
	if (test_id < id) {
	    i = alternatives.erase(i);
	} else if (test_id > id) {
	    alternatives.pop_front();
	    test_id = id;
	    i++;
	} else {
	    i++;
	}
    }

    if (alternatives.size()==1) {
	return &(alternatives.front());
    }


    debug_msg("neighbour addr test\n");
    /*
    ** Choose the route from the neighbour with the lowest neighbour
    ** address.
    */
    int test_address = alternatives.front().peer_handler()
	->neighbour_address();
    i = alternatives.begin(); i++;
    while(i!=alternatives.end()) {
	int address = i->peer_handler()->neighbour_address();
	//prefer lower address
	if (test_address < address) {
	    i = alternatives.erase(i);
	} else if (test_address > address) {
	    alternatives.pop_front();
	    test_address = address;
	    i++;
	} else {
	    i++;
	}
    }

    if (alternatives.empty()) {
	XLOG_UNREACHABLE();
    }
    
    //We can get here with more than one route if we compare two
    //identically rated routes.  Just choose one.
    return &(alternatives.front());
}

template<class A>
bool
DecisionTable<A>::dump_next_route(DumpIterator<A>& dump_iter) {
    const PeerHandler* peer = dump_iter.current_peer();
    typename map<BGPRouteTable<A>*, PeerHandler* >::const_iterator i;
    for (i = _parents.begin(); i != _parents.end(); i++) {
	if (i->second == peer)
	    return i->first->dump_next_route(dump_iter);
    }

    //we found no matching peer
    //this shouldn't happen because when a peer goes down, the dump
    //iterator should move on to other peers
    XLOG_UNREACHABLE();
}

template<class A>
int
DecisionTable<A>::route_dump(const InternalMessage<A> &rtmsg, 
			     BGPRouteTable<A> */*caller*/,
			     const PeerHandler *peer) {
    XLOG_ASSERT(_next_table != NULL);
    return _next_table->route_dump(rtmsg, (BGPRouteTable<A>*)this, peer);
}

template<class A>
void
DecisionTable<A>::igp_nexthop_changed(const A& bgp_nexthop)
{
    typename map<BGPRouteTable<A>*, PeerHandler* >::const_iterator i;
    for (i = _parents.begin(); i != _parents.end(); i++) {
	i->first->igp_nexthop_changed(bgp_nexthop);
    }
}

template<class A>
void
DecisionTable<A>::peering_went_down(const PeerHandler *peer, uint32_t genid,
				    BGPRouteTable<A> *caller) {
    XLOG_ASSERT(_next_table != NULL);
    typename map <BGPRouteTable<A>*, PeerHandler*>::const_iterator i;
    i = _parents.find(caller);
    XLOG_ASSERT(i !=_parents.end());
    XLOG_ASSERT(i->second == peer);

    _next_table->peering_went_down(peer, genid, this);
}

template<class A>
void
DecisionTable<A>::peering_down_complete(const PeerHandler *peer, 
					uint32_t genid,
					BGPRouteTable<A> *caller) {
    XLOG_ASSERT(_next_table != NULL);
    typename map <BGPRouteTable<A>*, PeerHandler*>::const_iterator i;
    i = _parents.find(caller);
    XLOG_ASSERT(i !=_parents.end());
    XLOG_ASSERT(i->second == peer);

    _next_table->peering_down_complete(peer, genid, this);
}

template<class A>
string
DecisionTable<A>::str() const {
    string s = "DecisionTable<A>" + tablename();
    return s;
}

template class DecisionTable<IPv4>;
template class DecisionTable<IPv6>;




