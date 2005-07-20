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

#ident "$XORP: xorp/bgp/bgp_varrw.cc,v 1.12 2005/07/15 23:37:30 abittau Exp $"

#include "bgp_module.h"
#include "libxorp/xorp.h"
#include "policy/common/policy_utils.hh"
#include "peer_handler.hh"
#include "bgp_varrw.hh"
#include "policy/backend/version_filters.hh"

using namespace policy_utils;

template <class A>
BGPVarRW<A>::BGPVarRW(const InternalMessage<A>& rtmsg, bool no_modify, 
		      const string& name)
    : _name(name), _orig_rtmsg(rtmsg), _filtered_rtmsg(NULL), 
      _got_fmsg(false), _wrote_ptags(false), 
      _palist(*(rtmsg.route()->attributes())), // XXX: pointer deref in ctr
      _no_modify(no_modify),
      _modified(false)
{
    for (int i = 0; i < 3; i++)
	_wrote_pfilter[i] = false;
}

template <class A>
BGPVarRW<A>::~BGPVarRW()
{
    // nobody ever obtained the filtered message but we actually created one. We
    // must delete it as no one else can, at this point.
    if (!_got_fmsg && _filtered_rtmsg)
	delete _filtered_rtmsg;
}

template <class A>
void
BGPVarRW<A>::start_read()
{
    // intialize policy stuff
    initialize("policytags", _orig_rtmsg.route()->policytags().element());

    initialize(VersionFilters::filter_import,
	       new ElemFilter(_orig_rtmsg.route()->policyfilter(0)));
    initialize(VersionFilters::filter_sm,
	       new ElemFilter(_orig_rtmsg.route()->policyfilter(1)));
    initialize(VersionFilters::filter_ex,
	       new ElemFilter(_orig_rtmsg.route()->policyfilter(2)));

    // initialize BGP vars
    const SubnetRoute<A>* route = _orig_rtmsg.route();

    read_route_nexthop(*route);
    
    const PathAttributeList<A>* attr = route->attributes();

    initialize("aspath",
	       _ef.create(ElemStr::id, attr->aspath().short_str().c_str()));
    
    uint32_t origin = attr->origin();
    initialize("origin", _ef.create(ElemU32::id, to_str(origin).c_str()));

    const LocalPrefAttribute* lpref = attr->local_pref_att();
    Element* e = NULL;
    if (lpref)
	e = _ef.create(ElemU32::id, to_str(lpref->localpref()).c_str());
    initialize("localpref", e);

    // XXX don't support community yet
    initialize("community", read_community(*attr));

    initialize("neighbor", read_neighbor());

    e = NULL;
    const MEDAttribute* med = attr->med_att();
    if(med) 
	e = _ef.create(ElemU32::id, to_str(med->med()).c_str());
    initialize("med", e);
}

template <class A>
Element*
BGPVarRW<A>::read_community(const PathAttributeList<A>& attr)
{
    const CommunityAttribute* ca = attr.community_att();

    // no community!
    if (!ca)
	return NULL;

    ElemSetU32* es = new ElemSetU32;

    const set<uint32_t>& com = ca->community_set();
    for (set<uint32_t>::const_iterator i = com.begin(); i != com.end(); ++i) 
	es->insert(ElemU32(*i));
    
    return es;
}

template <class A>
void
BGPVarRW<A>::write_community(const Element& e)
{
    XLOG_ASSERT(e.type() == ElemSetU32::id);

    const ElemSetU32& es = dynamic_cast<const ElemSetU32&>(e);

    if (_palist.community_att())
	_palist.remove_attribute_by_type(COMMUNITY);
	
    CommunityAttribute ca;
   
    for (typename ElemSetU32::const_iterator i = es.begin(); i != es.end(); 
	 ++i) {
	ca.add_community( (*i).val());
    }	
    
    _palist.add_path_attribute(ca);
}

template <class A>
Element*
BGPVarRW<A>::read_neighbor()
{
    Element* e = NULL;
    const PeerHandler* ph = _orig_rtmsg.origin_peer();
    if(ph)
	e = _ef.create(ElemIPv4::id, ph->id().str().c_str());
    return e;
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
	return;
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
    if (id == "community") {
	write_community(e);
	return;
    }
    if (id == "origin") {
	XLOG_ASSERT(u32 != NULL);

	OriginType origin = INCOMPLETE;

	if (u32->val() > INCOMPLETE)
		XLOG_FATAL("Unknown origin: %d\n", u32->val());
	
	origin = static_cast<OriginType>(u32->val());
    
	_palist.replace_origin(origin);
	return;
    }
    
    // XXX need to tidy up this whole class...
try {    
    if (id == VersionFilters::filter_import) {
	const ElemFilter& ef = dynamic_cast<const ElemFilter&>(e);

	_pfilter[0] = ef.val();
	_wrote_pfilter[0] = true;
	return;
    }

    if (id == VersionFilters::filter_sm) {
	const ElemFilter& ef = dynamic_cast<const ElemFilter&>(e);

	_pfilter[1] = ef.val();
	_wrote_pfilter[1] = true;
	return;
    }
    
    if (id == VersionFilters::filter_ex) {
	const ElemFilter& ef = dynamic_cast<const ElemFilter&>(e);

	_pfilter[2] = ef.val();
	_wrote_pfilter[2] = true;
	return;
    }
} catch(const bad_cast& ex) {
    XLOG_FATAL("Bad cast when writing ElemFilter %s (Type=%s) (val=%s)", 
	       id.c_str(), e.type().c_str(), e.str().c_str());
}


    // XXX: writing a variable we don't know about!
    XLOG_FATAL("Don't know how to write: %s\n", id.c_str());
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

    for (int i = 0; i < 3; i++) {
	if (_wrote_pfilter[i]) {
	    // XXX we need to change the policyfilter in the ORIGINAL ROUTE too.
	    // i.e. the version should be kept in the ribin subnetroute copy.
	    // If we just store it in the copy we send down stream, next time we
	    // get a push, we will get the wrong filter pointer [it will always
	    // be the original one which was in ribin,.. as we never update it].
	    old_route->set_policyfilter(i, _pfilter[i]);
	    new_route->set_policyfilter(i, _pfilter[i]);
	}    
	else
	    new_route->set_policyfilter(i, old_route->policyfilter(i));
    }

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
    // XXX I don't know if this is correct or not!!!!
//    if (_orig_rtmsg.changed())
//	old_route->unref();

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

template <class A>
string
BGPVarRW<A>::more_tracelog()
{
    string x = "BGP " + _name + " route: ";
    uint32_t level = trace();

    const InternalMessage<A>& msg = ( modified() ? 
				     (*_filtered_rtmsg) :
				     _orig_rtmsg);
    
    if (level > 0)
	x += msg.net().str();
    if (level > 1) {
	x += " Full route: ";
	x += msg.str();
    }

    return x;
}

template class BGPVarRW<IPv4>;
template class BGPVarRW<IPv6>;
