// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP$"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "route_table_policy_im.hh"

template <class A>
PolicyTableImport<A>::PolicyTableImport(const string& tablename, 
						  const Safi& safi,
						  BGPRouteTable<A>* parent,
						  PolicyFilters& pfs) :

		PolicyTable<A>(tablename,safi,parent,pfs,
			       filter::IMPORT)
			       
{
    this->_parent = parent;		
}




template <class A>
int
PolicyTableImport<A>::route_dump(const InternalMessage<A>& rtmsg,
				      BGPRouteTable<A>* caller,
				      const PeerHandler* dump_peer) {
    
    // XXX: policy route dumping IS A HACK!

    // it is a "normal dump"
    if(dump_peer)
	return PolicyTable<A>::route_dump(rtmsg,caller,dump_peer);

    // it is a "policy dump"
    XLOG_ASSERT(caller == this->_parent);
    
    bool was_filtered = rtmsg.route()->is_filtered();
    
    debug_msg("[BGP] Policy route dump: %s\nWas filtered: %d\n",
	      rtmsg.str().c_str(), was_filtered);

    const InternalMessage<A>* fmsg = doFiltering(rtmsg,false);
    bool accepted = (fmsg != NULL);

    debug_msg("[BGP] Policy route dump accepted: %d\n",accepted);

    BGPRouteTable<A>* next = this->_next_table;

    XLOG_ASSERT(next);

    int res = XORP_OK;

    if(accepted) {
	if(was_filtered) {
	    res = next->add_route(*fmsg,this);
	}
	else {
	    // i think it is suicidal to replace the same route, so copy it.
	    // but we need to, so it goes down to the other filters...
	    if(fmsg == &rtmsg) {
		SubnetRoute<A>* new_route = new SubnetRoute<A>(*(rtmsg.route()));

		InternalMessage<A>* new_fmsg;
		new_fmsg = new InternalMessage<A>(new_route, rtmsg.origin_peer(),
					          rtmsg.genid());
		
	
		// propagate flags
		if(rtmsg.push())
		    new_fmsg->set_push();
		if(rtmsg.from_previous_peering())
		    new_fmsg->set_from_previous_peering();
		if(rtmsg.changed())
		    new_fmsg->set_changed();
	    
		fmsg = new_fmsg;

	    }	


	    res = next->replace_route(rtmsg,*fmsg,this);
	}
    }
    // not accepted
    else {
	if(was_filtered) {
	}
	else {
	    next->delete_route(rtmsg,this);
	}
	res = ADD_FILTERED;
    }

    if(fmsg != &rtmsg)
	delete fmsg;

    return res;
}				  

template class PolicyTableImport<IPv4>;
template class PolicyTableImport<IPv6>;
