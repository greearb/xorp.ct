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

#ident "$XORP: xorp/bgp/route_table_policy.cc,v 1.2 2004/09/17 20:02:26 pavlin Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME


#include "bgp_module.h"

#include "libxorp/xorp.h"

#include "route_table_policy.hh"
#include "bgp_varrw.hh"
#include "route_table_decision.hh"


template <class A>
PolicyTable<A>::PolicyTable(const string& tablename, const Safi& safi,
			    BGPRouteTable<A>* parent,
			    PolicyFilters& pfs, 
			    const filter::Filter& type)
    : BGPRouteTable<A>(tablename, safi),
      _policy_filters(pfs), _filter_type(type)
{
    this->_parent = parent;		
}

template <class A>
PolicyTable<A>::~PolicyTable()
{
}

template <class A>
const InternalMessage<A>*
PolicyTable<A>::do_filtering(const InternalMessage<A>& rtmsg, 
			     bool no_modify) const
{
    try {
	BGPVarRW<A> varrw(rtmsg, no_modify);

	ostringstream trace;

	bool accepted = true;

	debug_msg("[BGP] running filter %s on route: %s\n",
		  filter::filter2str(_filter_type).c_str(),
		  rtmsg.str().c_str());

	accepted = _policy_filters.run_filter(_filter_type, varrw, &trace);

	debug_msg("[BGP] filter trace:\n%s\nEnd of trace. Accepted: %d\n",
		  trace.str().c_str(), accepted);

	if (!accepted)
	    return NULL;

	// we only want to check if filter accepted / rejecd route
	// [for route lookups]
	if (no_modify)
	    return &rtmsg;

	if (!varrw.modified())
	    return &rtmsg;


	InternalMessage<A>* fmsg = varrw.filtered_message();

	debug_msg("[BGP] filter modified message: %s\n", fmsg->str().c_str());

	return fmsg;
    } catch(const PolicyException& e) {
	XLOG_FATAL("Policy filter error %s", e.str().c_str());
	XLOG_UNFINISHED();
    }
}

template <class A>
int
PolicyTable<A>::add_route(const InternalMessage<A> &rtmsg,
			  BGPRouteTable<A> *caller)
{
    XLOG_ASSERT(caller == this->_parent);

    BGPRouteTable<A>* next = this->_next_table;

    XLOG_ASSERT(next);

    debug_msg("[BGP] PolicyTable %s add_route: %s\n",
	      filter::filter2str(_filter_type).c_str(),
	      rtmsg.str().c_str());

    const InternalMessage<A>* fmsg = do_filtering(rtmsg, false);

    if (!fmsg)
	return ADD_FILTERED;


    int res = next->add_route(*fmsg, this);

    if (fmsg != &rtmsg)
	delete fmsg;

    return res;
}

template <class A>
int
PolicyTable<A>::replace_route(const InternalMessage<A>& old_rtmsg,
			      const InternalMessage<A>& new_rtmsg,
			      BGPRouteTable<A>* caller)
{
    XLOG_ASSERT(caller == this->_parent);


    BGPRouteTable<A>* next = this->_next_table;

    XLOG_ASSERT(next);

    debug_msg("[BGP] PolicyTable %s replace_route: %s\nWith: %s\n",
	      filter::filter2str(_filter_type).c_str(),
	      old_rtmsg.str().c_str(),
	      new_rtmsg.str().c_str());

    const InternalMessage<A>* fold = do_filtering(old_rtmsg, false);
    const InternalMessage<A>* fnew = do_filtering(new_rtmsg, false);

    // XXX: We can probably use the is_filtered flag...
    int res;
    if (fold == NULL && fnew == NULL) {
	return ADD_FILTERED;
    } else if (fold != NULL && fnew == NULL) {
	next->delete_route(*fold, this);
	res = ADD_FILTERED;
    } else if (fold == NULL && fnew != NULL) {
	res = next->add_route(*fnew, this);
    } else {
	res = next->replace_route(*fold, *fnew, this);
    }

    if (fold != &old_rtmsg)
	delete fold;
    if (fnew != &new_rtmsg)
	delete fnew;
   
    return res;
}

template <class A>
int
PolicyTable<A>::delete_route(const InternalMessage<A>& rtmsg,
			     BGPRouteTable<A>* caller)
{
    XLOG_ASSERT(caller == this->_parent);

    BGPRouteTable<A>* next = this->_next_table;

    XLOG_ASSERT(next);

    debug_msg("[BGP] PolicyTable %s delete_route: %s\n",
	      filter::filter2str(_filter_type).c_str(),
	      rtmsg.str().c_str());

    const InternalMessage<A>* fmsg = do_filtering(rtmsg, false);
    if (fmsg == NULL)
	return 0;

    
    int res = next->delete_route(*fmsg, this);

    if (fmsg != &rtmsg)
	delete fmsg;

    return res;
}			     

template <class A>
int
PolicyTable<A>::route_dump(const InternalMessage<A>& rtmsg,
			   BGPRouteTable<A>* caller,
			   const PeerHandler* dump_peer)
{
    XLOG_ASSERT(caller == this->_parent);

    BGPRouteTable<A>* next = this->_next_table;

    XLOG_ASSERT(next);

    
    debug_msg("[BGP] PolicyTable %s route_dump: %s\n",
	      filter::filter2str(_filter_type).c_str(),
	      rtmsg.str().c_str());

    const InternalMessage<A>* fmsg = do_filtering(rtmsg, false);
    if (fmsg == NULL)
	return ADD_FILTERED;

    int res = next->route_dump(rtmsg, this, dump_peer);

    if (fmsg != &rtmsg)
	delete fmsg;

    return res;
}			   

template <class A>
int
PolicyTable<A>::push(BGPRouteTable<A>* caller)
{
    XLOG_ASSERT(caller == this->_parent);

    BGPRouteTable<A>* next = this->_next_table;

    XLOG_ASSERT(next);

    return next->push(this);
}

template <class A>
const SubnetRoute<A>*
PolicyTable<A>::lookup_route(const IPNet<A> &net, 
			     uint32_t& genid) const
{
    BGPRouteTable<A>* parent = this->_parent;

    XLOG_ASSERT(parent);

    const SubnetRoute<A>* found = parent->lookup_route(net, genid);

    if (!found)
	return NULL;

    InternalMessage<A> rtmsg(found, NULL, genid);

    debug_msg("[BGP] PolicyTable %s lookup_route: %s\n",
	      filter::filter2str(_filter_type).c_str(),
	      rtmsg.str().c_str());

    const InternalMessage<A>* fmsg = do_filtering(rtmsg, false);
    if (!fmsg)
	return NULL;
    
    XLOG_ASSERT(fmsg == &rtmsg);
   
    return found;
}

template <class A>
string
PolicyTable<A>::str() const
{
    return "not implemented yet";
}

template <class A>
void
PolicyTable<A>::output_state(bool busy, BGPRouteTable<A>* next_table)
{
    BGPRouteTable<A>* parent = this->_parent;

    XLOG_ASSERT(parent);
    XLOG_ASSERT(this->_next_table == next_table);

    parent->output_state(busy, this);
}

template <class A>
bool
PolicyTable<A>::get_next_message(BGPRouteTable<A>* next_table)
{
    BGPRouteTable<A>* parent = this->_parent;

    XLOG_ASSERT(parent);
    XLOG_ASSERT(this->_next_table == next_table);

    return parent->get_next_message(this);
}

template <class A>
void
PolicyTable<A>::route_used(const SubnetRoute<A>* rt, bool in_use)
{
    BGPRouteTable<A>* parent = this->_parent;

    XLOG_ASSERT(parent);

    parent->route_used(rt, in_use);
}

template class PolicyTable<IPv4>;
template class PolicyTable<IPv6>;
