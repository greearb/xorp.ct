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

#ident "$XORP: xorp/bgp/bgp_varrw.cc,v 1.34 2008/04/23 03:43:22 atanu Exp $"

#include "bgp_module.h"
#include "libxorp/xorp.h"
#include "policy/common/policy_utils.hh"
#include "peer_handler.hh"
#include "bgp_varrw.hh"
#include "policy/backend/version_filters.hh"
#include "policy/common/elem_filter.hh"
#include "policy/common/elem_bgp.hh"
#include <typeinfo>

using namespace policy_utils;

template <class A>
BGPVarRWCallbacks<A> BGPVarRW<A>::_callbacks;

template <class A>
BGPVarRW<A>::BGPVarRW(const string& name) :
    _name(name), _orig_rtmsg(NULL), _filtered_rtmsg(NULL), 
      _got_fmsg(false), _ptags(NULL), _wrote_ptags(false), 
      _palist(NULL),
      _no_modify(false),
      _modified(false), _route_modify(false)
{
    XLOG_ASSERT( unsigned(VAR_BGPMAX) <= VAR_MAX);

    for (int i = 0; i < 3; i++)
	_wrote_pfilter[i] = false;
}

template <class A>
BGPVarRW<A>::~BGPVarRW()
{
    cleanup();
}

template <class A>
void
BGPVarRW<A>::cleanup()
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

    if (_ptags)
        delete _ptags;
}

template <class A>
void
BGPVarRW<A>::attach_route(const InternalMessage<A>& rtmsg, bool no_modify)
{
    cleanup();

    _orig_rtmsg = &rtmsg;
    _filtered_rtmsg = NULL;
    _got_fmsg = false;
    _ptags = NULL;
    _wrote_ptags = false;
    _palist = NULL;
    _no_modify = no_modify;
    _modified = false;
    _route_modify = false;

    _aggr_brief_mode = rtmsg.route()->aggr_brief_mode();
    _aggr_prefix_len = rtmsg.route()->aggr_prefix_len();

    for (int i = 0; i < 3; i++) {
        if (_wrote_pfilter[i])
            _pfilter[i].release();
        _wrote_pfilter[i] = false;
    }
}

template <class A>
Element*
BGPVarRW<A>::read_policytags()
{
    return _orig_rtmsg->route()->policytags().element();
}

template <class A>
Element*
BGPVarRW<A>::read_filter_im()
{
    return new ElemFilter(_orig_rtmsg->route()->policyfilter(0));
}

template <class A>
Element*
BGPVarRW<A>::read_filter_sm()
{
    return new ElemFilter(_orig_rtmsg->route()->policyfilter(1));
}

template <class A>
Element*
BGPVarRW<A>::read_filter_ex()
{
    return new ElemFilter(_orig_rtmsg->route()->policyfilter(2));
}

template <>
Element*
BGPVarRW<IPv4>::read_network4()
{
    return new ElemIPv4Net(_orig_rtmsg->route()->net());
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
		      _orig_rtmsg->route()->net().str().c_str());
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
		      _orig_rtmsg->route()->nexthop().str().c_str());
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
    return new ElemIPv4(_orig_rtmsg->route()->nexthop());
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
    return new ElemASPath(_orig_rtmsg->route()->attributes()->aspath());
}

template <class A>
Element*
BGPVarRW<A>::read_origin()
{
    uint32_t origin = _orig_rtmsg->route()->attributes()->origin();
    return _ef.create(ElemU32::id, to_str(origin).c_str());
}

template <class A>
Element*
BGPVarRW<A>::read_localpref()
{
    const LocalPrefAttribute* lpref = _orig_rtmsg->route()->attributes()->
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
    const CommunityAttribute* ca = _orig_rtmsg->route()->attributes()->
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
    const MEDAttribute* med = _orig_rtmsg->route()->attributes()->med_att();
    if (med)
	return new ElemU32(med->med());
    else
	return NULL;
}

template <class A>
Element*
BGPVarRW<A>::read_med_remove()
{
    const MEDAttribute* med = _orig_rtmsg->route()->attributes()->med_att();
    if (med)
	return new ElemBool(false);	// XXX: default is don't remove the MED
    else
	return NULL;
}

template <class A>
Element*
BGPVarRW<A>::read_aggregate_prefix_len()
{
    // No-op. Should never be called.
    return new ElemU32(_aggr_prefix_len);
}

template <class A>
Element*
BGPVarRW<A>::read_aggregate_brief_mode()
{
    // No-op. Should never be called.
    return new ElemU32(_aggr_brief_mode);
}

template <class A>
Element*
BGPVarRW<A>::read_was_aggregated()
{
    if (_aggr_prefix_len == SR_AGGR_EBGP_WAS_AGGREGATED)
	return new ElemBool(true);
    else
	return new ElemBool(false);
}

template <class A>
Element*
BGPVarRW<A>::single_read(const Id& id)
{
    ReadCallback cb = _callbacks._read_map[id];
    XLOG_ASSERT(cb);
    return (this->*cb)();
}

template <class A>
void
BGPVarRW<A>::clone_palist()
{
    _palist = new PathAttributeList<A>(*_orig_rtmsg->route()->attributes());
}

template <class A>
void
BGPVarRW<A>::write_community(const Element& e)
{
    _route_modify = true;

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
    const PeerHandler* ph = _orig_rtmsg->origin_peer();
    if (ph != NULL && !ph->originate_route_handler()) {
	e = _ef.create(ElemIPv4::id, ph->get_peer_addr().c_str());
    }
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
    XLOG_ASSERT(!_ptags);

    _ptags = new PolicyTags(e);
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

    const ElemASPath& aspath = dynamic_cast<const ElemASPath&>(e);

    if (!_palist)
	clone_palist();
    _palist->replace_AS_path(aspath.val());
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
BGPVarRW<A>::write_med_remove(const Element& e)
{
    const ElemBool& med_remove = dynamic_cast<const ElemBool&>(e);

    if (! med_remove.val())
	return;			// Don't remove the MED

    if (!_palist)
	clone_palist();
    if (_palist->med_att())
	_palist->remove_attribute_by_type(MED);

    _route_modify = true;
}

template <class A>
void
BGPVarRW<A>::write_aggregate_prefix_len(const Element& e)
{
    // We should not set the aggr_pref_len if already set by someone else!
    if (_aggr_prefix_len != SR_AGGR_IGNORE)
	return;

    const ElemU32& u32 = dynamic_cast<const ElemU32&>(e);
    if (u32.val() <= 128) {
	// Only accept valid prefix_len values, since out of range values
	// might get interpreted later as special markers
	_aggr_prefix_len = u32.val();
	_route_modify = true;
    }
}

template <class A>
void
BGPVarRW<A>::write_aggregate_brief_mode(const Element& e)
{
    const ElemBool& brief_mode = dynamic_cast<const ElemBool&>(e);

    if (!brief_mode.val())
	return;	

    _aggr_brief_mode = true;

    _route_modify = true;
}

template <class A>
void
BGPVarRW<A>::write_was_aggregated(const Element& e)
{
    // No-op. Should never be called.
    UNUSED(e);
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
BGPVarRW<A>::single_write(const Id& id, const Element& e)
{
    if (_no_modify)
	return;
    
    WriteCallback cb = _callbacks._write_map[id];
    XLOG_ASSERT(cb);
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
		_orig_rtmsg->route()->set_policyfilter(i, _pfilter[i]);
	}
	return;
    }

    if (_palist)
        _palist->rehash();

    const SubnetRoute<A>* old_route = _orig_rtmsg->route();
    SubnetRoute<A>* new_route = new SubnetRoute<A>(_orig_rtmsg->net(),
						   _palist ? _palist :
						   old_route->attributes(),
						   old_route->original_route(),
						   old_route->igp_metric());
    // propagate policy tags
    if (_wrote_ptags)
	new_route->set_policytags(*_ptags);
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
    new_route->set_aggr_prefix_len(_aggr_prefix_len);
    if (_aggr_brief_mode)
	new_route->set_aggr_brief_mode();
    else
	new_route->clear_aggr_brief_mode();

    _filtered_rtmsg = new InternalMessage<A>(new_route,
					     _orig_rtmsg->origin_peer(),
					     _orig_rtmsg->genid());

    _filtered_rtmsg->set_changed();
   
    // propagate internal message flags
    if (_orig_rtmsg->push())
	_filtered_rtmsg->set_push();
    if (_orig_rtmsg->from_previous_peering())
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
				     *_orig_rtmsg);
    
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
    init_rw(VarRW::VAR_POLICYTAGS, 
	    &BGPVarRW<A>::read_policytags, &BGPVarRW<A>::write_policytags);
    
    init_rw(VarRW::VAR_FILTER_IM, 
	    &BGPVarRW<A>::read_filter_im, &BGPVarRW<A>::write_filter_im);
    
    init_rw(VarRW::VAR_FILTER_SM, 
	    &BGPVarRW<A>::read_filter_sm, &BGPVarRW<A>::write_filter_sm);
    
    init_rw(VarRW::VAR_FILTER_EX, 
	    &BGPVarRW<A>::read_filter_ex, &BGPVarRW<A>::write_filter_ex);

    init_rw(BGPVarRW<A>::VAR_NETWORK4, &BGPVarRW<A>::read_network4, NULL);
    
    init_rw(BGPVarRW<A>::VAR_NEXTHOP4, 
	    &BGPVarRW<A>::read_nexthop4, &BGPVarRW<A>::write_nexthop4);

    init_rw(BGPVarRW<A>::VAR_NETWORK6, &BGPVarRW<A>::read_network6, NULL);
    
    init_rw(BGPVarRW<A>::VAR_NEXTHOP6, 
	    &BGPVarRW<A>::read_nexthop6, &BGPVarRW<A>::write_nexthop6);

    init_rw(BGPVarRW<A>::VAR_ASPATH, 
	    &BGPVarRW<A>::read_aspath, &BGPVarRW<A>::write_aspath);

    init_rw(BGPVarRW<A>::VAR_ORIGIN, 
	    &BGPVarRW<A>::read_origin, &BGPVarRW<A>::write_origin);

    init_rw(BGPVarRW<A>::VAR_NEIGHBOR, &BGPVarRW<A>::read_neighbor_base_cb, NULL);

    init_rw(BGPVarRW<A>::VAR_LOCALPREF, 
	    &BGPVarRW<A>::read_localpref, &BGPVarRW<A>::write_localpref);

    init_rw(BGPVarRW<A>::VAR_COMMUNITY, 
	    &BGPVarRW<A>::read_community, &BGPVarRW<A>::write_community);

    init_rw(BGPVarRW<A>::VAR_MED, 
	    &BGPVarRW<A>::read_med, &BGPVarRW<A>::write_med);

    init_rw(BGPVarRW<A>::VAR_MED_REMOVE, 
	    &BGPVarRW<A>::read_med_remove, &BGPVarRW<A>::write_med_remove);

    init_rw(BGPVarRW<A>::VAR_AGGREGATE_PREFIX_LEN, 
	    &BGPVarRW<A>::read_aggregate_prefix_len,
	    &BGPVarRW<A>::write_aggregate_prefix_len);

    init_rw(BGPVarRW<A>::VAR_AGGREGATE_BRIEF_MODE, 
	    &BGPVarRW<A>::read_aggregate_brief_mode,
	    &BGPVarRW<A>::write_aggregate_brief_mode);

    init_rw(BGPVarRW<A>::VAR_WAS_AGGREGATED, 
	    &BGPVarRW<A>::read_was_aggregated,
	    &BGPVarRW<A>::write_was_aggregated);
}

template <class A>
void
BGPVarRWCallbacks<A>::init_rw(const VarRW::Id& id, RCB rcb, WCB wcb)
{
    if (rcb)
	_read_map[id] = rcb;
    if (wcb)
	_write_map[id] = wcb;
}

template class BGPVarRW<IPv4>;
template class BGPVarRW<IPv6>;

template class BGPVarRWCallbacks<IPv4>;
template class BGPVarRWCallbacks<IPv6>;
