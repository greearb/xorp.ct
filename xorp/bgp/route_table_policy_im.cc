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



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "route_table_policy_im.hh"

template <class A>
PolicyTableImport<A>::PolicyTableImport(const string& tablename, 
				        const Safi& safi,
					BGPRouteTable<A>* parent,
					PolicyFilters& pfs,
					const A& peer,
					const A& self)
    : PolicyTable<A>(tablename, safi, parent, pfs, filter::IMPORT)
{
    this->_parent = parent;
    this->_varrw->set_peer(peer);
    this->_varrw->set_self(self);
}




template <class A>
int
PolicyTableImport<A>::route_dump(InternalMessage<A>& rtmsg,
				 BGPRouteTable<A>* caller,
				 const PeerHandler* dump_peer)
{
    // XXX: policy route dumping IS A HACK!

    // it is a "normal dump"
    if (dump_peer)
	return PolicyTable<A>::route_dump(rtmsg, caller, dump_peer);

    // it is a "policy dump"
    XLOG_ASSERT(caller == this->_parent);
    
    debug_msg("[BGP] Policy route dump: %s\n\n", rtmsg.str().c_str());

#if 0
    // "old" filter...
    InternalMessage<A>* fmsg = do_filtering(rtmsg, false);
    bool was_filtered = (fmsg == NULL);
    if (fmsg != NULL) {
	if (fmsg->route() == rtmsg.route()) {
	    // fmsg contains the original route, so we need to clone it,
	    // or we'll get bad interactions when we set the policyfilter
	    // on the new route below because the policyfilter is
	    // propagated back to the parent.
	    SubnetRoute<A>* copy_old_rt = new SubnetRoute<A>(*rtmsg.route());

	    // clear the parent route, so noone downstream can mess up the
	    // metadata on the route still stored in RibIn.
	    copy_old_rt->set_parent_route(NULL);

	    InternalMessage<A>* copy_fmsg
		= new InternalMessage<A>(copy_old_rt, rtmsg.origin_peer(),
					 rtmsg.genid());

	    if (rtmsg.changed())
		copy_fmsg->set_changed();

	    if (rtmsg.push())
		copy_fmsg->set_push();

	    if (rtmsg.from_previous_peering())
		copy_fmsg->set_from_previous_peering();

	    fmsg = copy_fmsg;
	    XLOG_ASSERT(fmsg->route() != rtmsg.route());
	} else {
	    // the route is already a copy, but we still need to clear the
	    // parent route to prevent anyone downstream modifying the
	    // metadata on the route still stored in RibIn
	    const_cast<SubnetRoute<A>*>(fmsg->route())->set_parent_route(NULL);
	}
    }

    // we want current filter
    rtmsg.route()->set_policyfilter(0, RefPf());

    // filter new message
    InternalMessage<A>* new_msg = do_filtering(rtmsg, false);
    
    bool accepted = (new_msg != NULL);

#endif

    // clone the PA list, so we don't process it twice
    FPAListRef old_fpa_list = new FastPathAttributeList<A>(*rtmsg.attributes());
    SubnetRoute<A>* copy_old_route = new SubnetRoute<A>(*rtmsg.route());
    copy_old_route->set_parent_route(NULL);
    InternalMessage<A> *old_rtmsg = new InternalMessage<A>(copy_old_route,
							   old_fpa_list,
							   rtmsg.origin_peer(),
							   rtmsg.genid());
    old_rtmsg->set_copied();

    // we want current filter
    rtmsg.route()->set_policyfilter(0, RefPf());

    bool old_accepted = this->do_filtering(*old_rtmsg, false);
    bool new_accepted = this->do_filtering(rtmsg, false);

    InternalMessage<A> *new_rtmsg = 0;
    SubnetRoute<A>* copy_new_route = 0;
    if (new_accepted) {
	// we don't want anyone downstream to mess with the metadata
	// on the route in RibIn
	//	copy_new_route = new SubnetRoute<A>(*rtmsg.route());
	//	copy_new_route->set_parent_route(NULL);
	//	new_rtmsg = new InternalMessage<A>(copy_new_route,
	new_rtmsg = new InternalMessage<A>(rtmsg.route(),
					   rtmsg.attributes(),
					   rtmsg.origin_peer(),
					   rtmsg.genid());
    }
    
    debug_msg("[BGP] Policy route dump accepted: %d\n", new_accepted);

    BGPRouteTable<A>* next = this->_next_table;

    XLOG_ASSERT(next);

    int res = XORP_OK;

    if (new_accepted) {
	if (!old_accepted) {
	    debug_msg("[BGP] Policy add_route [accepted, was filtered]");
	    res = next->add_route(*new_rtmsg, this);
	} else {
	    debug_msg("[BGP] Policy replace_route old=(%s) new=(%s)\n",
		      old_rtmsg->str().c_str(), new_rtmsg->str().c_str());

	    if (new_rtmsg->attributes() == old_rtmsg->attributes()) {
		// no change.
		copy_new_route->unref();
		delete new_rtmsg;
		copy_old_route->unref();
		delete old_rtmsg;
		return ADD_USED;
	    }

	    // XXX don't check return of deleteroute!
	    res = next->delete_route(*old_rtmsg, this);

	    XLOG_ASSERT(new_rtmsg->route());

	    // current filters
	    for (int i = 1; i < 3; i++)
		new_rtmsg->route()->set_policyfilter(i, RefPf());
	
	    res = next->add_route(*new_rtmsg, this);

	    // XXX this won't work.  We need to keep original filter pointers on
	    // old route and null pointers on new route.  If we clone the old
	    // route, the cache table will find it, and send out the cached old
	    // route which still has pointer to original parent, which now has
	    // null pointers, in the case we sent them in the new route which
	    // has same parent.  [it's a mess... fix soon]
//	    res = next->replace_route(*fmsg, *new_msg, this);
	}
    } else {
	// not accepted
	if (!old_accepted) {
	} else {
	    //
	    // XXX: A hack propagating the "not-winner route" flag back to
	    // the original route.
	    // The reason we need this hack here is because of an earlier
	    // hack where the fmsg parent was modified to be NULL.
	    //
	    rtmsg.route()->set_is_not_winner();
	    next->delete_route(*old_rtmsg, this);
	}
	res = ADD_FILTERED;
    }

    if (old_rtmsg)
	delete old_rtmsg;
    if (new_rtmsg)
	delete new_rtmsg;

    return res;
}

template class PolicyTableImport<IPv4>;
template class PolicyTableImport<IPv6>;
