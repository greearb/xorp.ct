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

#ident "$XORP: xorp/bgp/bgp_varrw.cc,v 1.4 2004/09/27 10:35:15 abittau Exp $"

#include "bgp_module.h"

#include "libxorp/xorp.h"

#include "policy/common/policy_utils.hh"

#include "bgp_varrw.hh"

template <class A>
BGPVarRW<A>::BGPVarRW(const InternalMessage<A>& rtmsg, bool no_modify)
    : _orig_rtmsg(rtmsg), _filtered_rtmsg(NULL), _got_fmsg(false),
      _wrote_ptags(false), 
      _palist(*(rtmsg.route()->attributes())), // XXX: pointer deref in ctr
      _no_modify(no_modify),
      _modified(false)
{
}

template <class A>
BGPVarRW<A>::~BGPVarRW()
{
    // nobody ever obtained the filtered message but we actually created one. We
    // must delete it as no one else can, at this point.
    if(!_got_fmsg && _filtered_rtmsg)
	delete _filtered_rtmsg;
}

template <class A>
void
BGPVarRW<A>::start_read()
{
    initialize("policytags", _orig_rtmsg.route()->policytags().element());

    const SubnetRoute<A>* route = _orig_rtmsg.route();

    read_route_nexthop(*route);
    
    const PathAttributeList<A>* attr = route->attributes();

    initialize("aspath",
	       _ef.create(ElemStr::id, attr->aspath().short_str().c_str()));

    initialize("origin",
	       _ef.create(ElemU32::id,
			  policy_utils::to_str(attr->origin()).c_str()));

    const LocalPrefAttribute* lpref = attr->local_pref_att();

    if (lpref) {
	initialize("localpref",
		   _ef.create(ElemU32::id, lpref->str().c_str()));
    } else {
	initialize("localpref", NULL);
    }


    const AtomicAggAttribute* atomagg = attr->atomic_aggregate_att();
    if (atomagg) {
	initialize("atomicagg",
		   _ef.create(ElemStr::id, atomagg->str().c_str()));
    } else {
	initialize("atomicagg", NULL);
    }

    const AggregatorAttribute* aggregator = attr->aggregator_att();
    if (aggregator) {
	initialize("aggregator",
		   _ef.create(ElemStr::id, aggregator->str().c_str()));
    } else {
	initialize("aggregator", NULL);
    }
}

template<>
void
BGPVarRW<IPv4>::read_route_nexthop(const SubnetRoute<IPv4>& route)
{
    initialize("network4", _ef.create(ElemIPv4Net::id,
				      route.net().str().c_str()));

    initialize("nexthop4", _ef.create(ElemIPv4::id,
				      route.nexthop().str().c_str()));

    initialize("network6", NULL);
    initialize("nexthop6", NULL);
}

template<>
void
BGPVarRW<IPv6>::read_route_nexthop(const SubnetRoute<IPv6>& route)
{
    initialize("network6", _ef.create(ElemIPv6Net::id,
				      route.net().str().c_str()));

    initialize("nexthop6", _ef.create(ElemIPv6::id,
				      route.nexthop().str().c_str()));

    initialize("network4", NULL);
    initialize("nexthop4", NULL);
}

template <class A>
InternalMessage<A>*
BGPVarRW<A>::filtered_message()
{
    _got_fmsg = true;
    return _filtered_rtmsg;
}

template <class A>
void
BGPVarRW<A>::single_write(const string& id, const Element& e)
{
    if (_no_modify)
	return;

    if (id == "policytags") {
	_ptags = e;
	_wrote_ptags = true;
    }
    if (write_nexthop(id, e))
	return;

    const ElemU32* u32 = NULL;

    if (e.type() == ElemU32::id) {
	u32 = dynamic_cast<const ElemU32*>(&e);
	XLOG_ASSERT(u32 != NULL);
    }	

    if (id == "aspath") {
	AsPath aspath(e.str().c_str());
	_palist.replace_AS_path(aspath);
	return;
    }
    if (id == "med") {
	if (_palist.med_att())
	    _palist.remove_attribute_by_type(MED);
	
	XLOG_ASSERT(u32 != NULL);
	
	MEDAttribute med(u32->val());
        _palist.add_path_attribute(med);
	return;
    }
    if (id == "localpref") {
	if (_palist.local_pref_att())
	    _palist.remove_attribute_by_type(LOCAL_PREF);

	XLOG_ASSERT(u32 != NULL);
	
	LocalPrefAttribute lpref(u32->val());
	_palist.add_path_attribute(lpref);
	return;
    }

}

template <class A>
void
BGPVarRW<A>::end_write()
{
    // I think there should be a better way of modifying bgp routes... [a copy
    // constructor, or helper methods possibly].

    if (_no_modify)
	return;

    _palist.rehash();

    const SubnetRoute<A>* old_route = _orig_rtmsg.route();
    SubnetRoute<A>* new_route = new SubnetRoute<A>(_orig_rtmsg.net(),
						   &_palist,
						   old_route->original_route(),
						   old_route->igp_metric());
    // propagate policy tags
    if (_wrote_ptags)
	new_route->set_policytags(_ptags);
    else
	new_route->set_policytags(old_route->policytags());
   
    // propagate route flags
    new_route->set_filtered(old_route->is_filtered());
    if (old_route->is_winner())
	new_route->set_is_winner(old_route->igp_metric());

    _filtered_rtmsg = new InternalMessage<A>(new_route,
					     _orig_rtmsg.origin_peer(),
					     _orig_rtmsg.genid());
    _filtered_rtmsg->set_changed();
   
    // propagate internal message flags
    if (_orig_rtmsg.push())
	_filtered_rtmsg->set_push();
    if (_orig_rtmsg.from_previous_peering())
	_filtered_rtmsg->set_from_previous_peering();

    // delete route which may have been changed by previous filter.
    if (_orig_rtmsg.changed())
	old_route->unref();

    _modified = true;
}


template <>
bool
BGPVarRW<IPv4>::write_nexthop(const string& id, const Element& e)
{
    if (id == "nexthop4" && e.type() == ElemIPv4::id) {
	const ElemIPv4* eip = dynamic_cast<const ElemIPv4*>(&e);
	XLOG_ASSERT(eip != NULL);

	IPv4 nh(eip->val());

	_palist.replace_nexthop(nh);
	return true;
    }
    return false;
}


template <>
bool
BGPVarRW<IPv6>::write_nexthop(const string& id, const Element& e)
{
    if (id == "nexthop6" && e.type() == ElemIPv6::id) {
	const ElemIPv6* eip = dynamic_cast<const ElemIPv6*>(&e);
	XLOG_ASSERT(eip != NULL);

	IPv6 nh(eip->val());

	_palist.replace_nexthop(nh);
	return true;
    }
    return false;
}

template <class A>
bool
BGPVarRW<A>::modified()
{
    return _modified;
}

template class BGPVarRW<IPv4>;
template class BGPVarRW<IPv6>;
