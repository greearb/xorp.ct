// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/route_table_decision.cc,v 1.5 2003/01/16 23:18:58 pavlin Exp $"

//#define DEBUG_LOGGING
#include "bgp_module.h"
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

/* How do we cope with the different possibilities for one route
 * replacing another one?  Assume we have two peers, A and B.  We hear
 * a route a1 from A, and it wins by default.
 * 
 * Case 1.  New route a2 comes from A.  RibIn A finds it already has
 * route a1.  RibIn sends delete_route for a1, then sends add_route
 * for a2.
 *
 * Case 2.  New worse route b1 comes from B.  RibIn B sends
 * add_route. DecisionTable looks up route in A, finds a1, and doesn't
 * process b1 further.
 *
 * Case 3.  New worse route b1 comes from B.  RibIn B sends
 * add_route. DecisionTable looks up route in A, finds a1.
 * DecisionTable sends delete_route for a1 with peer_handler set to A.
 * DecisionTable then sends add_route for b1 with peer_handler set to
 * B.
 *
 *
 * Now assume we have two routes a1 from A and b1 from B.  a1 won the
 * Decision process.
 *
 * Case 4.  New route a2 from A replaces a1, but is worse than b1.
 * RibIn A sends delete_route a1.  DecisionTable looks up route, and
 * discovers b1.  DecisionTable sends delete_route for a1 with
 * peer_handler set to A.  DecisionTable then sends add_route for b1
 * with peer_handler set to B.  Now RibIn A sends add_route for a2.
 * DecisionTable looks up route, and discovers b1.  a2 loses so no
 * further action is taken.
 *
 * Case 5. New route a2 from A replaces a1, and is better than b1.
 * RibIn A sends delete_route a1.  DecisionTable looks up route, and
 * discovers b1.  DecisionTable sends delete_route for a1 with
 * peer_handler set to A.  DecisionTable then sends add_route for b1
 * with peer_handler set to B.  Now RibIn A sends add_route for a2.
 * DecisionTable looks up route, and discovers b1.  a2 beats b1. 
 * DecisionTable sends delete_route for b1 with peer_handler set to B.
 * DecisionTable now sends add_route for a2 with peer_handler set to A.
 * At RibOut A, we see add_route b1 then delete_route b1.
 * At RibOut B, we see delete_route a1 then add_route a2.
 * At RibOut C (etc) we see delete_route a1, add_route b1,
 * delete_route b1, then add_route a2 and finally a push.
 *
 * Case 6. New route b2 replaces b1 but is worse than a1.  RibIn B
 * sends delete_route b1.  DecisionTable looks up route, discovers
 * that b1 lost to a1, and takes no further action.  RibIn B sends
 * add_route b1.  DecisionTable looks up route, discovers that b2
 * loses to a1, and takes no further action.
 *
 * Case 7. New route b2 replaces b1 and is better than a1.  RibIn B
 * sends delete_route b1.  DecisionTable looks up route, discovers
 * that b1 lost to a1, and takes no further action.  RibIn B sends
 * add_route b2.  DecisionTable looks up route, discovers that b2
 * beats a1.  DecisionTable sends delete_route a1 with peer_handler
 * set to A.  DecisionTable then sends add_route b2 with peer_handler
 * set to B.
 */

template<class A>
int
DecisionTable<A>::add_route(const InternalMessage<A> &rtmsg, 
			    BGPRouteTable<A> *caller) {
    if (_parents.find(caller)==_parents.end())
	abort();

    debug_msg("DT:add_route %s\n", rtmsg.route()->str().c_str());

    //if the nexthop isn't resolvable, don't event consider the route
    debug_msg("testing resolvability\n");
    if (!resolvable(rtmsg.route()->nexthop())) {
	cp(5);
	debug_msg("route not resolvable\n");
    	return ADD_UNUSED;
    }
    debug_msg("route resolvable\n");

    const SubnetRoute<A> *old_winner;
    const PeerHandler* old_winners_peer;
    BGPRouteTable<A>* old_winners_parent;
    
    find_previous_winner(caller, rtmsg.net(), old_winner, old_winners_peer, 
			 old_winners_parent);

    bool new_is_best = false;
    if (old_winner == NULL) {
	cp(6);
	new_is_best = true;
    } else if (route_is_better(old_winner, old_winners_peer,
			       rtmsg.route(), rtmsg.origin_peer())) {
	cp(7);
	new_is_best = true;
	//propagate a delete_route to clear out the old state.
	InternalMessage<A> old_rt_msg(old_winner, old_winners_peer, 
				      GENID_UNKNOWN);
	_next_table->delete_route(old_rt_msg, (BGPRouteTable<A>*)this);

	//inform the RibIn the route is no longer being used.
	old_winners_parent->route_used(old_winner, false);

	//the old winner is no longer the winner
	old_winner->set_is_not_winner();
    }
    if (new_is_best) {
	cp(8);
	rtmsg.route()->set_is_winner(igp_distance(rtmsg.route()->nexthop()));

	int result;
	result = _next_table->add_route(rtmsg, (BGPRouteTable<A>*)this);

	if (result == ADD_USED || result == ADD_UNUSED) {
	    cp(9);
	    //if it got as far as the decision table, we declare it
	    //used, even if it gets filtered downstream.
	    return ADD_USED;
	} else {
	    cp(10);
	    //some sort of failure, so return the error code directly.
	    return result;
	}
    }
    cp(11);
    return ADD_UNUSED;
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

    const SubnetRoute<A>* old_winner;
    const PeerHandler* old_winners_peer;
    BGPRouteTable<A>* old_winners_parent;

    find_previous_winner(caller, new_rtmsg.net(), old_winner, 
			 old_winners_peer, old_winners_parent);
    
    bool old_won, new_won;

    old_won = old_rtmsg.route()->is_winner();
    if (old_won) {
	//we're replacing the previous winner
	assert(old_winner == NULL);
	if (route_is_better(old_rtmsg.route(), old_rtmsg.origin_peer(),
			    new_rtmsg.route(), new_rtmsg.origin_peer())) {
	    cp(13);
	    // The route is better, so no need to look for
	    // any alternative route.
	} else {
	    //The route is the same or worse
	    if (route_is_better(new_rtmsg.route(), new_rtmsg.origin_peer(),
				old_rtmsg.route(), old_rtmsg.origin_peer())) {
		//The route got worse.  We need to see if anyone else can win.
		cp(140);
		old_winner = lookup_route(caller, new_rtmsg.net(), 
					  old_winners_peer, 
					  old_winners_parent);
	    } else {
		cp(141);
		//It's neither better or worse.
		//No need to look for any alternative.
	    }
	}
    } else {
	cp(15);
    }

    if (old_winner == NULL) {
	cp(16);
	new_won = true;
    } else {
	cp(17);
	new_won = route_is_better(old_winner, old_winners_peer,
				  new_rtmsg.route(), new_rtmsg.origin_peer());
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
	
	assert(old_winner != NULL);
	InternalMessage<A> alt_rtmsg(old_winner, old_winners_peer,
				     GENID_UNKNOWN);
	result = _next_table->add_route(alt_rtmsg, 
					(BGPRouteTable<A>*)this);
	//inform the RibIn the route is now being used.
	old_winner->set_is_winner(igp_distance(old_winner->nexthop()));
	old_winners_parent->route_used(old_winner, true);
    } else {
	cp(23);
	//case 4: this peer lost before, but now it wins,
	assert((old_won == false) && (new_won == true));

	//there may not be an old_winner if the none of the previous
	//routes were resolvable
	if(old_winner != NULL) {
	    cp(24);
	    InternalMessage<A> alt_rtmsg(old_winner, old_winners_peer, 
					 GENID_UNKNOWN);
	    _next_table->delete_route(alt_rtmsg, 
				      (BGPRouteTable<A>*)this);
	    //inform the RibIn the route is no longer being used.
	    old_winners_parent->route_used(old_winner, false);
	} else {
	    cp(25);
	}
	result = _next_table->add_route(new_rtmsg, (BGPRouteTable<A>*)this);
    }

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
    if (_parents.find(caller)==_parents.end()) abort();
    if (_next_table == NULL) abort();

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
}

template<class A>
int
DecisionTable<A>::push(BGPRouteTable<A> *caller) {
    if (_parents.find(caller)==_parents.end())
	abort();
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
    const SubnetRoute<A>* best_route, *found_route;
    best_routes_peer = NULL;
    best_route = NULL;
    typename map<BGPRouteTable<A>*, PeerHandler * >::const_iterator i;
    best_routes_parent = NULL;
    for (i = _parents.begin(); i!=_parents.end(); i++) {
	cp(35);
	if (i->first == ignore_parent) {
	    cp(36);
	    continue;
	}
	found_route = i->first->lookup_route(net);
	if (found_route != NULL) {
	    cp(37);
	    if (best_route == NULL) {
		cp(38);
		if (found_route->is_winner() || 
		    resolvable(found_route->nexthop())) {
		    cp(39);
		    best_route = found_route;
		    best_routes_peer = i->second;
		    best_routes_parent = i->first;
		}
	    } else {
		cp(40);
		//note that it is possible that
		//found_route->is_winner() is true, but the route
		//would not win the route_is_better test because the
		//IGP metrics have changed, but the change has not yet
		//been propagated.  In this case we must go with the
		//route that has is_winner()==true, because as far as
		//decision is concerned, the IGP metric change hasn't
		//occured yet.
		if (found_route->is_winner()) {
		    cp(41);
		    best_route = found_route;
		    best_routes_peer = i->second;
		    best_routes_parent = i->first;
		} else {
		    cp(42);
		    if (resolvable(found_route->nexthop())) {
			cp(43);
			if (route_is_better(best_route, best_routes_peer,
					    found_route, i->second)) {
			    cp(44);
			    best_route = found_route;
			    best_routes_peer = i->second;
			    best_routes_parent = i->first;
			}
		    }
		}
		//if the route has the winner bit set, we don't need to go further
		if (best_route->is_winner())
		    cp(45);
		return best_route;
	    }
	}
    }
    cp(46);
    return best_route;
}

template<class A>
const SubnetRoute<A>*
DecisionTable<A>::lookup_route(const IPNet<A> &net) const
{
    const SubnetRoute<A>* winner;
    const PeerHandler* winners_peer;
    BGPRouteTable<A>* winners_parent;
    find_previous_winner(NULL, net, winner, winners_peer, winners_parent);
    return winner;
}

template<class A>
bool
DecisionTable<A>::find_previous_winner(BGPRouteTable<A> *caller,
				       const IPNet<A>& net,
				       const SubnetRoute<A>*& winner,
				       const PeerHandler*& winners_peer,
				       BGPRouteTable<A>*& winners_parent
				       ) const {
    typename map<BGPRouteTable<A>*, PeerHandler * >::const_iterator i;
    winner = NULL;
    winners_peer = NULL;
    winners_parent = NULL;
    bool found_any_route = false;
    const SubnetRoute<A>* found_route;
    for (i = _parents.begin();  i != _parents.end();  i++) {
	//We don't need to lookup the route in the parent that the new
	//route came from - if this route replaced an earlier route
	//from the same parent we'd see it as a replace, not an add
	cp(47);
 	if (i->first != caller) {
	    cp(48);
	    found_route = i->first->lookup_route(net);
	    if (found_route != NULL) {
		cp(49);
		found_any_route = true;
		if (found_route->is_winner()) {
		    cp(50);
		    winner = found_route;
		    winners_parent = i->first;
		    winners_peer = i->second;
		    break;
		} else {
		    cp(51);
		}
	    } else {
		cp(52);
	    }
 	} else {
	    cp(53);
	}
    }
    cp(54);
    return found_any_route;
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
    list<PathAttribute*> l = attrlist->att_list();
    list<PathAttribute*>::const_iterator i;
    for(i = l.begin(); i != l.end(); i++) {
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
    list<PathAttribute*> l = attrlist->att_list();
    list<PathAttribute*>::const_iterator i;
    for(i = l.begin(); i != l.end(); i++) {
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
	debug_msg("Decision: IGP distance for %s is %d", nexthop.str().c_str(),
		  metric);
    else
	debug_msg("Decision: IGP distance for %s is unknown", nexthop.str().c_str());

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
DecisionTable<A>::route_is_better(const SubnetRoute<A> *our_route,
				  const PeerHandler *our_peer,
				  const SubnetRoute<A> *test_route,
				  const PeerHandler *test_peer) const
{
    debug_msg("in route_is_better\n");

    /* The spec seems pretty odd.  In our architecture, it seems
       reasonable to do phase 2 before phase 1, because if a route
       isn't resolvable we don't want to consider it */

    /*
    ** Phase 2: Route Selection.  */
    /*
    ** Check if routes resolve.
    */
    bool test_resolvable = resolvable(test_route->nexthop());
    bool our_resolvable = resolvable(our_route->nexthop());

    /*We should never even call route_is_better if the test_route
      isn't resolvable */
    assert(test_resolvable);

    /* If our route isn't resolvable, the test route wins */
    if (!our_resolvable) {
	cp(63);
	return true;
    }
    /* By this point, both routes are resolvable */
    assert(our_resolvable && test_resolvable);

    /* 
    ** Phase 1: Calculation of degree of preference.
    */
    int our_pref = local_pref(our_route, our_peer);
    int test_pref = local_pref(test_route, test_peer);

    if(test_pref > our_pref) {
	cp(64);
	return true;
    }
    if(test_pref < our_pref) {
	debug_msg("%d vs %d\n", test_pref, our_pref);
	debug_msg("test route: %s\n", 
		  test_route->str().c_str());
	cp(65);
	return false;
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
	aspath().get_path_length();
    int our_aspath_length = our_route->attributes()->
	aspath().get_path_length();
    debug_msg("%s length %d\n", our_route->attributes()->
	      aspath().str().c_str(), our_aspath_length);
    debug_msg("%s length %d\n", test_route->attributes()->
	      aspath().str().c_str(), test_aspath_length);
    
    if(test_aspath_length < our_aspath_length) {
	cp(66);
	return true;
    }

    if(test_aspath_length > our_aspath_length) {
	cp(67);
	return false;
    }

    debug_msg("origin test\n");
    /*
    ** Lowest origin value.
    */
    int test_origin = test_route->attributes()->origin();
    int our_origin = our_route->attributes()->origin();

    if(test_origin < our_origin) {
	cp(68);
	return true;
    }

    if(test_origin > our_origin) {
	cp(69);
	return false;
    }

    debug_msg("MED test\n");
    /*
    ** Compare meds if both routes came from the same neighbour AS.
    */
    AsNum test_asnum(test_route->attributes()->aspath().first_asnum());
    AsNum our_asnum(our_route->attributes()->aspath().first_asnum());

    //Sanity checking.  If either of these conditions is hit, then the
    //input sanity checks haven't done their job.
    if (!test_peer->ibgp() && (test_asnum != test_peer->AS_number()))
	XLOG_FATAL("AS Path %s received from EBGP peer with AS %s\n",
		   test_route->attributes()->aspath().str().c_str(),
		   test_peer->AS_number().str().c_str());
    if (!our_peer->ibgp() && (our_asnum != our_peer->AS_number()))
	XLOG_FATAL("AS Path %s received from EBGP peer with AS %s\n",
		   our_route->attributes()->aspath().str().c_str(),
		   our_peer->AS_number().str().c_str());

    if(test_asnum == our_asnum) {
	int test_med = med(test_route);
	int our_med = med(our_route);

	if(test_med < our_med) {
	    cp(70);
	    return true;
	}

	if(test_med > our_med) {
	    cp(71);
	    return false;
	}
	cp(72);
    } else {
	cp(73);
    }
    
    debug_msg("EBGP vs IBGP test\n");
    /*
    ** Prefer routes from external peers over internal peers.
    */
    if(test_peer->ibgp() != our_peer->ibgp()) {
	cp(74);
	if(!test_peer->ibgp()) {
	    cp(75);
	    return true;
	} else {
	    cp(76);
	    return false;
	}
    } else {
	cp(77);
    }

    debug_msg("IGP distance test\n");
    /*
    ** Compare IGP distances.
    */
    int test_igp_distance = igp_distance(test_route->nexthop());
    int our_igp_distance = igp_distance(our_route->nexthop());
    debug_msg("%d vs %d\n", our_igp_distance, test_igp_distance);
    if(test_igp_distance < our_igp_distance) {
	cp(78);
	return true;
    }
    if(our_igp_distance < test_igp_distance) {
	cp(79);
	return false;
    }

    debug_msg("BGP ID test\n");
    /*
    ** Choose the route from the neighbour with the lowest BGP ID.
    */
    int test_id = test_peer->id();
    int our_id = our_peer->id();

    debug_msg("testing BGP ID: %d vs %d\n", test_id, our_id);

    if(test_id < our_id) {
	cp(80);
	return true;
    }

    if(test_id > our_id) {
	cp(81);
	return false;
    }

    debug_msg("neighbour addr test\n");
    /*
    ** Choose the route from the neighbour with the lowest neighbour
    ** address.
    */
    int test_address = test_peer->neighbour_address();
    int our_address = our_peer->neighbour_address();

    if(test_address < our_address) {
	cp(82);
	return true;
    }

    if(test_address > our_address) {
	cp(83);
	return false;
    }

    //We can get here if we compare two identically rated routes.  
    return false;
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
    abort();
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




