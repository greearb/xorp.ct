// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
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

#ident "$XORP: xorp/bgp/route_table_policy_sm.cc,v 1.1 2004/09/17 13:50:55 abittau Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"

#include "route_table_policy_sm.hh"
#include "route_table_decision.hh"


template <class A>
PolicyTableSourceMatch<A>::PolicyTableSourceMatch(const string& tablename, 
						  const Safi& safi,
						  BGPRouteTable<A>* parent,
						  PolicyFilters& pfs)
    : PolicyTable<A>(tablename, safi, parent, pfs, filter::EXPORT_SOURCEMATCH),
      _pushing_routes(false), _dump_iter(NULL) 

{
    this->_parent = parent;		
}


template <class A>
PolicyTableSourceMatch<A>::~PolicyTableSourceMatch()
{
    if (_dump_iter)
	delete _dump_iter;
}


template <class A>
void
PolicyTableSourceMatch<A>::push_routes(list<const PeerTableInfo<A>*>& peer_list)
{
    _pushing_routes = true;
    
    _dump_iter = new DumpIterator<A>(NULL, peer_list);

    debug_msg("[BGP] Push routes\n");

    while (_pushing_routes)
	do_next_route_dump();
}

template <class A>
void
PolicyTableSourceMatch<A>::do_next_route_dump()
{
    if (!_dump_iter->is_valid()) {
	end_route_dump();
	return;
    }

    BGPRouteTable<A>* parent = this->_parent;
    XLOG_ASSERT(parent);

    DecisionTable<A>* dt = dynamic_cast<DecisionTable<A>*>(parent);
    XLOG_ASSERT(dt != NULL);

    if (!dt->dump_next_route(*_dump_iter)) {
	if (!_dump_iter->next_peer()) {
	    end_route_dump();
	    return;
	}    
    }
}

template <class A>
void
PolicyTableSourceMatch<A>::end_route_dump()
{
    debug_msg("[BGP] End of push routes\n");
    delete _dump_iter;
    _dump_iter = NULL;
    _pushing_routes = false;
}

template class PolicyTableSourceMatch<IPv4>;
template class PolicyTableSourceMatch<IPv6>;
