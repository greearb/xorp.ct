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

#ident "$XORP: xorp/bgp/bgp_varrw.cc,v 1.18 2005/07/22 21:46:40 abittau Exp $"

#include "bgp_module.h"
#include "libxorp/xorp.h"
#include "policy/common/policy_utils.hh"
#include "peer_handler.hh"
#include "bgp_varrw.hh"
#include "policy/backend/version_filters.hh"
#include <typeinfo>

using namespace policy_utils;

template <class A>
BGPVarRWCallbacks<A> BGPVarRW<A>::_callbacks;

template <class A>
BGPVarRW<A>::BGPVarRW(const InternalMessage<A>& rtmsg, bool no_modify, 
		      const string& name)
    : _name(name), _orig_rtmsg(rtmsg), _filtered_rtmsg(NULL), 
      _got_fmsg(false), _wrote_ptags(false), 
      _palist(NULL),
      _no_modify(no_modify),
      _modified(false), _route_modify(false)
{
    for (int i = 0; i < 3; i++)
	_wrote_pfilter[i] = false;
}

template <class A>
BGPVarRW<A>::~BGPVarRW()
{
    // nobody ever obtained the filtered message but we actually created one. We
    // must delete it as no one else can, at this point.
    if (!_got_fmsg && _filtered_rtmsg) {
	if (_filtered_rtmsg->changed())
	    _filtered_rtmsg->route()->unref();

	delete _filtered_rtmsg;
    }

    if (_palist)
	delete _palist;
}

template <class A>
Element*
BGPVarRW<A>::read_policytags()
{
    return _orig_rtmsg.route()->policytags().element();
}

template <class A>
Element*
BGPVarRW<A>::read_filter_im()
{
    return new ElemFilter(_orig_rtmsg.route()->policyfilter(0));
}

template <class A>
Element*
BGPVarRW<A>::read_filter_sm()
{
    return new ElemFilter(_orig_rtmsg.route()->policyfilter(1));
}

template <class A>
Element*
BGPVarRW<A>::read_filter_ex()
{
    return new ElemFilter(_orig_rtmsg.route()->policyfilter(2));
}

template <>
Element*
BGPVarRW<IPv4>::read_network4()
{
    return _ef.create(ElemIPv4Net::id, 
		      _orig_rtmsg.route()->net().str().c_str());
}

template <>
Element*
BGPVarRW<IPv6>::read_network4()
{
    return NULL;
}

template <>
Element*
BGPVarRW<IPv6>::read_network6()
{
    return _ef.create(ElemIPv6Net::id, 
		      _orig_rtmsg.route()->net().str().c_str());
}

template <>
Element*
BGPVarRW<IPv4>::read_network6()
{
    return NULL;
}

template <>
Element*
BGPVarRW<IPv6>::read_nexthop6()
{
    return _ef.create(ElemIPv6::id, 
		      _orig_rtmsg.route()->nexthop().str().c_str());
}

template <>
Element*
BGPVarRW<IPv4>::read_nexthop6()
{
    return NULL;
}

template <>
Element*
BGPVarRW<IPv4>::read_nexthop4()
{
    return _ef.create(ElemIPv4::id, 
		      _orig_rtmsg.route()->nexthop().str().c_str());
}

template <>
Element*
BGPVarRW<IPv6>::read_nexthop4()
{
    return NULL;
}

template <class A>
Element*
BGPVarRW<A>::read_aspath()
{
    return _ef.create(ElemStr::id, _orig_rtmsg.route()->attributes()
				    ->aspath().short_str().c_str());
}

template <class A>
Element*
BGPVarRW<A>::read_origin()
{
    uint32_t origin = _orig_rtmsg.route()->attributes()->origin();
    return _ef.create(ElemStr::id, to_str(origin).c_str());
}

template <class A>
Element*
BGPVarRW<A>::read_localpref()
{
    const LocalPrefAttribute* lpref = _orig_rtmsg.route()->attributes()->
					local_pref_att();
    if (lpref)
	return _ef.create(ElemU32::id, to_str(lpref->localpref()).c_str());
    else
	return NULL;
}

template <class A>
Element*
BGPVarRW<A>::read_community()
{
    const CommunityAttribute* ca = _orig_rtmsg.route()->attributes()->
				    community_att();

    // no community!
    if (!ca)
	return NULL;

    ElemSetCom32* es = new ElemSetCom32;

    const set<uint32_t>& com = ca->community_set();
    for (set<uint32_t>::const_iterator i = com.begin(); i != com.end(); ++i) 
	es->insert(ElemCom32(*i));
    
    return es;
}

template <class A>
Element*
BGPVarRW<A>::read_med()
{
    const MEDAttribute* med = _orig_rtmsg.route()->attributes()->med_att();
    if (med)
	return _ef.create(ElemU32::id, to_str(med->med()).c_str());
    else
	return NULL;
}

template <class A>
Element*
BGPVarRW<A>::single_read(const string& id)
{
    typename BGPVarRWCallbacks<A>::ReadMap::iterator i = 
		_callbacks._read_map.find(id);

    XLOG_ASSERT (i != _callbacks._read_map.end());

    ReadCallback cb = i->second;
    return (this->*cb)();
}

template <class A>
void
BGPVarRW<A>::clone_palist()
{
    _palist = new PathAttributeList<A>(*_orig_rtmsg.route()->attributes());
}

template <class A>
void
BGPVarRW<A>::write_community(const Element& e)
{
    XLOG_ASSERT(e.type() == ElemSetCom32::id);

    const ElemSetCom32& es = dynamic_cast<const ElemSetCom32&>(e);

    if (!_palist)
	clone_palist();

    if (_palist->community_att())
	_palist->remove_attribute_by_type(COMMUNITY);
	
    CommunityAttribute ca;
   
    for (typename ElemSetCom32::const_iterator i = es.begin(); i != es.end(); 
	 ++i) {
	ca.add_community( (*i).val());
    }	
    
    _palist->add_path_attribute(ca);
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

template <class A>
InternalMessage<A>*
BGPVarRW<A>::filtered_message()
{
    XLOG_ASSERT(_modified && _filtered_rtmsg);
    _got_fmsg = true;
    return _filtered_rtmsg;
}

template <class A>
void
BGPVarRW<A>::write_filter_im(const Element& e)
{
    const ElemFilter& ef = dynamic_cast<const ElemFilter&>(e);
    _pfilter[0] = ef.val();
    _wrote_pfilter[0] = true;
}

template <class A>
void
BGPVarRW<A>::write_filter_sm(const Element& e)
{
    const ElemFilter& ef = dynamic_cast<const ElemFilter&>(e);
    _pfilter[1] = ef.val();
    _wrote_pfilter[1] = true;
}

template <class A>
void
BGPVarRW<A>::write_filter_ex(const Element& e)
{
    const ElemFilter& ef = dynamic_cast<const ElemFilter&>(e);
    _pfilter[2] = ef.val();
    _wrote_pfilter[2] = true;
}

template <class A>
void
BGPVarRW<A>::write_policytags(const Element& e)
{
    _ptags = e;
    _wrote_ptags = true;
    
    // XXX: maybe we should make policytags be like filter pointers... i.e. meta
    // information, rather than real route information...
    _route_modify = true;
}

template <>
void
BGPVarRW<IPv4>::write_nexthop4(const Element& e)
{
    _route_modify = true;
    const ElemIPv4* eip = dynamic_cast<const ElemIPv4*>(&e);
    XLOG_ASSERT(eip != NULL);

    IPv4 nh(eip->val());

    if (!_palist)
	clone_palist();
    _palist->replace_nexthop(nh);
}

template <>
void
BGPVarRW<IPv6>::write_nexthop4(const Element& /* e */)
{
}

template <>
void
BGPVarRW<IPv6>::write_nexthop6(const Element& e)
{
    _route_modify = true;
    const ElemIPv6* eip = dynamic_cast<const ElemIPv6*>(&e);
    XLOG_ASSERT(eip != NULL);

    IPv6 nh(eip->val());

    if (!_palist)
	clone_palist();
    _palist->replace_nexthop(nh);
}

template <>
void
BGPVarRW<IPv4>::write_nexthop6(const Element& /* e */)
{
}

template <class A>
void
BGPVarRW<A>::write_aspath(const Element& e)
{
    _route_modify = true;

    AsPath aspath(e.str().c_str());

    if (!_palist)
	clone_palist();
    _palist->replace_AS_path(aspath);
}

template <class A>
void
BGPVarRW<A>::write_med(const Element& e)
{
    _route_modify = true;

    if (!_palist)
	clone_palist();
    if (_palist->med_att())
	_palist->remove_attribute_by_type(MED);
	
    const ElemU32& u32 = dynamic_cast<const ElemU32&>(e);	
    MEDAttribute med(u32.val());
    _palist->add_path_attribute(med);
}

template <class A>
void
BGPVarRW<A>::write_localpref(const Element& e)
{
    _route_modify = true;

    if (!_palist)
	clone_palist();
    if (_palist->local_pref_att())
	_palist->remove_attribute_by_type(LOCAL_PREF);
	
    const ElemU32& u32 = dynamic_cast<const ElemU32&>(e);	
    LocalPrefAttribute lpref(u32.val());
    _palist->add_path_attribute(lpref);
}

template <class A>
void
BGPVarRW<A>::write_origin(const Element& e)
{
    _route_modify = true;

    const ElemU32& u32 = dynamic_cast<const ElemU32&>(e);	
    OriginType origin = INCOMPLETE;

    if (u32.val() > INCOMPLETE)
	XLOG_FATAL("Unknown origin: %d\n", u32.val());
	
    origin = static_cast<OriginType>(u32.val());
    
    if (!_palist)
	clone_palist();
    _palist->replace_origin(origin);
}

template <class A>
void
BGPVarRW<A>::single_write(const string& id, const Element& e)
{
    if (_no_modify)
	return;
    
    typename BGPVarRWCallbacks<A>::WriteMap::iterator i = 
		_callbacks._write_map.find(id);

    XLOG_ASSERT (i != _callbacks._write_map.end());

    WriteCallback cb = i->second;
    (this->*cb)(e);
}

template <class A>
void
BGPVarRW<A>::end_write()
{
    // I think there should be a better way of modifying bgp routes... [a copy
    // constructor, or helper methods possibly].

    // OK.  The real problem is that you can't clone a subnet route, and assign
    // path attributes to it!

    if (_no_modify)
	return;

    // only meta routing stuff changed [i.e. policy filter pointers... so we can
    // get away without creating a new subnet route, etc]
    if (!_route_modify) {
	for (int i = 0; i < 3; i++) {
	    if (_wrote_pfilter[i])
		_orig_rtmsg.route()->set_policyfilter(i, _pfilter[i]);
	}
	return;
    }

    if (_palist)
        _palist->rehash();

    const SubnetRoute<A>* old_route = _orig_rtmsg.route();
    SubnetRoute<A>* new_route = new SubnetRoute<A>(_orig_rtmsg.net(),
						   _palist ? _palist :
						   old_route->attributes(),
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
    new_route->set_in_use(old_route->in_use());
    new_route->set_nexthop_resolved(old_route->nexthop_resolved());

    _filtered_rtmsg = new InternalMessage<A>(new_route,
					     _orig_rtmsg.origin_peer(),
					     _orig_rtmsg.genid());

    _filtered_rtmsg->set_changed();
   
    // propagate internal message flags
    if (_orig_rtmsg.push())
	_filtered_rtmsg->set_push();
    if (_orig_rtmsg.from_previous_peering())
	_filtered_rtmsg->set_from_previous_peering();

    _modified = true;
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

template <class A>
BGPVarRWCallbacks<A>::BGPVarRWCallbacks()
{
    _read_map["neighbor"] = &BGPVarRW<A>::read_neighbor;

    _read_map["policytags"] = &BGPVarRW<A>::read_policytags;
    _read_map[VersionFilters::filter_import] = &BGPVarRW<A>::read_filter_im;
    _read_map[VersionFilters::filter_sm] = &BGPVarRW<A>::read_filter_sm;
    _read_map[VersionFilters::filter_ex] = &BGPVarRW<A>::read_filter_ex;
    
    _read_map["network4"] = &BGPVarRW<A>::read_network4;
    _read_map["network6"] = &BGPVarRW<A>::read_network6;

    _read_map["nexthop4"] = &BGPVarRW<A>::read_nexthop4;
    _read_map["nexthop6"] = &BGPVarRW<A>::read_nexthop6;
    _read_map["aspath"] = &BGPVarRW<A>::read_aspath;
    _read_map["origin"] = &BGPVarRW<A>::read_origin;
    
    _read_map["localpref"] = &BGPVarRW<A>::read_localpref;
    _read_map["community"] = &BGPVarRW<A>::read_community;
    _read_map["med"] = &BGPVarRW<A>::read_med;
    
    _write_map["policytags"] = &BGPVarRW<A>::write_policytags;
    _write_map[VersionFilters::filter_import] = &BGPVarRW<A>::write_filter_im;
    _write_map[VersionFilters::filter_sm] = &BGPVarRW<A>::write_filter_sm;
    _write_map[VersionFilters::filter_ex] = &BGPVarRW<A>::write_filter_ex;
    
    _write_map["nexthop4"] = &BGPVarRW<A>::write_nexthop4;
    _write_map["nexthop6"] = &BGPVarRW<A>::write_nexthop6;
    _write_map["aspath"] = &BGPVarRW<A>::write_aspath;
    _write_map["origin"] = &BGPVarRW<A>::write_origin;
    
    _write_map["localpref"] = &BGPVarRW<A>::write_localpref;
    _write_map["community"] = &BGPVarRW<A>::write_community;
    _write_map["med"] = &BGPVarRW<A>::write_med;
}

template class BGPVarRW<IPv4>;
template class BGPVarRW<IPv6>;

template class BGPVarRWCallbacks<IPv4>;
template class BGPVarRWCallbacks<IPv6>;
