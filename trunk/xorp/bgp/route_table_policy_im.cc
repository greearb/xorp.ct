// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/bgp/route_table_policy_im.cc,v 1.18 2008/07/23 05:09:37 pavlin Exp $"

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
PolicyTableImport<A>::route_dump(const InternalMessage<A>& rtmsg,
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

    // "old" filter...
    const InternalMessage<A>* fmsg = do_filtering(rtmsg, false);
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
    const InternalMessage<A>* new_msg = do_filtering(rtmsg, false);
    
    bool accepted = (new_msg != NULL);

    debug_msg("[BGP] Policy route dump accepted: %d\n", accepted);

    BGPRouteTable<A>* next = this->_next_table;

    XLOG_ASSERT(next);

    int res = XORP_OK;

    if (accepted) {
	if (was_filtered) {
	    debug_msg("[BGP] Policy add_route [accepted, was filtered]");
	    res = next->add_route(*new_msg, this);
	} else {
	    XLOG_ASSERT(fmsg != NULL);
	    debug_msg("[BGP] Policy replace_route old=(%s) new=(%s)\n",
		      fmsg->str().c_str(), new_msg->str().c_str());

	    XLOG_ASSERT(fmsg->route() != new_msg->route());
#if 0
	    // we will delete and add the same subnetroute!
	    // make new internal message to preserve route.
	    if (rtmsg.changed() && fmsg->route() == new_msg->route()) {
		SubnetRoute<A>* copy_rt = new SubnetRoute<A>(*new_msg->route());
		InternalMessage<A>* copy_msg = 
		    new InternalMessage<A>(copy_rt, new_msg->origin_peer(),
					   new_msg->genid());
		
		XLOG_ASSERT(new_msg->changed());
		
		if (new_msg->changed())
		    copy_msg->set_changed();
	
		if (new_msg->push())
		    copy_msg->set_push();

		if (new_msg->from_previous_peering())
		    copy_msg->set_from_previous_peering();

		if (new_msg != &rtmsg)
		    delete new_msg;

		new_msg = copy_msg;
	    
		XLOG_ASSERT(fmsg != new_msg);
	    }
#endif

	    // XXX don't check return of deleteroute!
	    res = next->delete_route(*fmsg, this);

	    XLOG_ASSERT(new_msg->route());

	    // current filters
	    for (int i = 1; i < 3; i++)
		new_msg->route()->set_policyfilter(i, RefPf());
	
	    res = next->add_route(*new_msg, this);

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
	if (was_filtered) {
	} else {
	    XLOG_ASSERT(fmsg != NULL);
	    //
	    // XXX: A hack propagating the "not-winner route" flag back to
	    // the original route.
	    // The reason we need this hack here is because of an earlier
	    // hack where the fmsg parent was modified to be NULL.
	    //
	    rtmsg.route()->set_is_not_winner();
	    next->delete_route(*fmsg, this);
	}
	res = ADD_FILTERED;
    }

    //
    // Check if route was modified by our filters.  If it was, and it was
    // modified by the static filters, then we need to free the new subnet
    // route allocated by static filters.
    //
    if (rtmsg.changed() && fmsg != &rtmsg && new_msg != &rtmsg) {
	rtmsg.route()->unref();
    }

    if (fmsg != &rtmsg) {
	if (fmsg != NULL)
	    delete fmsg;
    }

    if (new_msg != &rtmsg) {
	if (new_msg != NULL)
	    delete new_msg;
    }

    return res;
}

template class PolicyTableImport<IPv4>;
template class PolicyTableImport<IPv6>;
