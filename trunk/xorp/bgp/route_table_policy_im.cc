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

#ident "$XORP: xorp/bgp/route_table_policy_im.cc,v 1.4 2005/03/25 02:52:47 pavlin Exp $"

 #define DEBUG_LOGGING
 #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"

#include "route_table_policy_im.hh"

template <class A>
PolicyTableImport<A>::PolicyTableImport(const string& tablename, 
						  const Safi& safi,
						  BGPRouteTable<A>* parent,
						  PolicyFilters& pfs)
    : PolicyTable<A>(tablename, safi, parent, pfs, filter::IMPORT)
{
    this->_parent = parent;		
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
    
    bool was_filtered = rtmsg.route()->is_filtered();
    
    debug_msg("[BGP] Policy route dump: %s\nWas filtered: %d\n",
	      rtmsg.str().c_str(), was_filtered);

    // "old" filter...
    const InternalMessage<A>* fmsg = do_filtering(rtmsg, false);
    was_filtered = (fmsg == NULL);

    // new route
    SubnetRoute<A>* cur_rt = new SubnetRoute<A>(*rtmsg.route());

    // we want current filter
    for (int i = 0; i < 3; i++)
        cur_rt->set_policyfilter(i, RefPf());

    InternalMessage<A>* tmp_im = new InternalMessage<A>(cur_rt,
						        rtmsg.origin_peer(),
						        rtmsg.genid());
    if(rtmsg.changed())
	tmp_im->set_changed();
    if(rtmsg.push())
	tmp_im->set_push();
    if(rtmsg.from_previous_peering())
	tmp_im->set_from_previous_peering();

    // filter new message
    const InternalMessage<A>* new_msg = do_filtering(*tmp_im, false);
    if (tmp_im != new_msg)
        delete tmp_im;

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
#if 0	
	    // I think it is suicidal to replace the same route, so copy it.
	    // but we need to, so it goes down to the other filters...
	    if (fmsg == &rtmsg) {
		SubnetRoute<A>* new_route = new SubnetRoute<A>(*(rtmsg.route()));

		InternalMessage<A>* new_fmsg;
		new_fmsg = new InternalMessage<A>(new_route,
						  rtmsg.origin_peer(),
					          rtmsg.genid());
	
		// propagate flags
		if (rtmsg.push())
		    new_fmsg->set_push();
		if (rtmsg.from_previous_peering())
		    new_fmsg->set_from_previous_peering();
		if (rtmsg.changed())
		    new_fmsg->set_changed();
	    
		fmsg = new_fmsg;
	    }	
#endif
	    debug_msg("[BGP] Policy replace_route old=(%s) new=(%s)\n",
		      fmsg->str().c_str(), new_msg->str().c_str());
	    res = next->replace_route(*fmsg, *new_msg, this);
	}
    } else {
	// not accepted
	if (was_filtered) {
	} else {
	    next->delete_route(*fmsg, this);
	}
	res = ADD_FILTERED;
    }

    if (fmsg != &rtmsg)
	delete fmsg;

    delete new_msg;

    return res;
}

template class PolicyTableImport<IPv4>;
template class PolicyTableImport<IPv6>;
