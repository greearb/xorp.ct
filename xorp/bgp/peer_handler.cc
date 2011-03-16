// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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
#include "libxorp/xlog.h"
#include "peer_handler.hh"
#include "plumbing.hh"
#include "packet.hh"
#include "bgp.hh"

PeerHandler::PeerHandler(const string &init_peername,
			 BGPPeer *peer,
			 BGPPlumbing *plumbing_unicast,
			 BGPPlumbing *plumbing_multicast)
    : _plumbing_unicast(plumbing_unicast), 
      _plumbing_multicast(plumbing_multicast),
      _peername(init_peername), _peer(peer),
      _packet(NULL)
{
    debug_msg("peername: %s peer %p unicast %p multicast %p\n", 
	      _peername.c_str(), peer, _plumbing_unicast,
	      _plumbing_multicast);

    if (_plumbing_unicast != NULL) 
	_plumbing_unicast->add_peering(this);

    if (_plumbing_multicast != NULL) 
	_plumbing_multicast->add_peering(this);

    _peering_is_up = true;

    // stats for debugging only
    _nlri_total = 0;
    _packets = 0;
}

/* The typical action is only to call peering_went_down when a peering
   goes down.

   In the case of BGP itself going down, we typically call stop on all
   the peerhandlers, then call peering_went_down on all the
   peerhandlers, then wait for all the peerhandlers to indicate idle,
   then call the destructors on the peerhandlers.

   In the case of de-configuring a peering (as opposed to it going
   down temporarily, we would call peering_went_down, then wait for
   the peerhandler to indicate idle, and finally call the destructor
   on the peerhandler. */


PeerHandler::~PeerHandler()
{
    debug_msg("PeerHandler destructor called\n");

    if (_plumbing_unicast != NULL)
	_plumbing_unicast->delete_peering(this);
    if (_plumbing_multicast != NULL)
	_plumbing_multicast->delete_peering(this);
    if (_packet != NULL)
	delete _packet;
}

void
PeerHandler::stop()
{
    debug_msg("PeerHandler STOP!\n");
    _plumbing_unicast->stop_peering(this);
    _plumbing_multicast->stop_peering(this);
}

void
PeerHandler::peering_went_down()
{
    _peering_is_up = false;
    _plumbing_unicast->peering_went_down(this);
    _plumbing_multicast->peering_went_down(this);
}

void
PeerHandler::peering_came_up()
{
    _peering_is_up = true;
    _plumbing_unicast->peering_came_up(this);
    _plumbing_multicast->peering_came_up(this);
}

template <>
bool
PeerHandler::add<IPv4>(const UpdatePacket *p,
		       ref_ptr<FastPathAttributeList<IPv4> >& original_pa_list,
		       ref_ptr<FastPathAttributeList<IPv4> >& pa_list,
		       Safi safi)
{
    UNUSED(original_pa_list);
    XLOG_ASSERT(!pa_list->is_locked());
    switch(safi) {
    case SAFI_UNICAST: {
	if (p->nlri_list().empty())
	    return false;

	XLOG_ASSERT(pa_list->complete());

	BGPUpdateAttribList::const_iterator ni4;
	ni4 = p->nlri_list().begin();
	int count = p->nlri_list().size();
	while (ni4 != p->nlri_list().end()) {
	    if (!ni4->net().is_unicast()) {
		XLOG_ERROR("NLRI <%s> is not semantically correct ignoring.%s",
			   cstring(ni4->net()), cstring(*p));
		++ni4;
		continue;
	    }
	    PolicyTags policy_tags;
	    FPAList4Ref fpalist;
	    if (count == 1) {
		// no need to copy if there's only one
		fpalist = pa_list;
	    } else {
		// need to copy the others, or each one's changes will affect the rest
		fpalist = new FastPathAttributeList<IPv4>(*pa_list);
	    }
	    XLOG_ASSERT(!fpalist->is_locked());
	    _plumbing_unicast->add_route(ni4->net(), fpalist, 
					 policy_tags, this);
	    ++ni4;
	}
    }
	break;
    case SAFI_MULTICAST: {
	const MPReachNLRIAttribute<IPv4> *mpreach = pa_list->mpreach<IPv4>(safi);
	if(!mpreach)
	    return false;

	XLOG_ASSERT(pa_list->complete());

	list<IPNet<IPv4> >::const_iterator ni;
	int count = mpreach->nlri_list().size();
	ni = mpreach->nlri_list().begin();
	while (ni != mpreach->nlri_list().end()) {
	    if (!ni->is_unicast()) {
		XLOG_ERROR("NLRI <%s> is not semantically correct ignoring.%s",
			   cstring(*ni), cstring(*p));
		++ni;
		continue;
	    }
	    PolicyTags policy_tags;
	    FPAList4Ref fpalist;
	    if (count == 1) {
		// no need to copy
		fpalist = pa_list;
	    } else {
		// need to copy the others, or each one's changes will
		// affect the rest
		fpalist = new FastPathAttributeList<IPv4>(*pa_list);
	    }
	    // we're done with this attribute now.
	    fpalist->remove_attribute_by_type(MP_REACH_NLRI);
	    _plumbing_multicast->add_route(*ni, fpalist, 
					   policy_tags, this);
	    ++ni;
	}
    }
	break;
    }

    return true;
}

#ifdef HAVE_IPV6

template <>
bool
PeerHandler::add<IPv6>(const UpdatePacket *p,
		       ref_ptr<FastPathAttributeList<IPv4> >& original_pa_list,
		       ref_ptr<FastPathAttributeList<IPv6> >& pa_list,
		       Safi safi)
{
    UNUSED(original_pa_list);
    UNUSED(p);

    const MPReachNLRIAttribute<IPv6> *mpreach = pa_list->mpreach<IPv6>(safi);
    if(!mpreach)
	return false;

    XLOG_ASSERT(pa_list->complete());

    list<IPNet<IPv6> >::const_iterator ni;
    int nlris_remaining = mpreach->nlri_list().size();
    ni = mpreach->nlri_list().begin();
    while (nlris_remaining > 0) {
	IPNet<IPv6> n = *ni;
	if (!n.is_unicast()) {
	    XLOG_ERROR("NLRI <%s> is not semantically correct ignoring.%s",
		       cstring(*ni), cstring(*p));
	    ++ni;
	    continue;
	}
	PolicyTags policy_tags;
	FPAList6Ref fpalist;
	if (mpreach->nlri_list().size() == 1) {
	    // no need to copy
	    fpalist = pa_list;
	} else {
	    // need to copy the others, or each one's changes will affect the rest
	    fpalist = new FastPathAttributeList<IPv6>(*pa_list);
	}
	// we're done with this attribute now.
	fpalist->remove_attribute_by_type(MP_REACH_NLRI);
	switch(safi) {
	case SAFI_UNICAST:
	    _plumbing_unicast->add_route(n, fpalist, policy_tags, this);
	    break;
	case SAFI_MULTICAST:
	    _plumbing_multicast->add_route(n, fpalist, policy_tags, this);
	    break;
	}
	nlris_remaining--;
	if (nlris_remaining > 0) {
	    // only do this if there are more nlris, because otherwise
	    // the iterator will be invalid
	    ++ni;
	}
    }

    return true;
}

template <>
bool
PeerHandler::withdraw<IPv6>(const UpdatePacket *p, 
			    ref_ptr<FastPathAttributeList<IPv4> >& original_pa_list,
			    Safi safi)
{
    UNUSED(p);
    const MPUNReachNLRIAttribute<IPv6> *mpunreach 
	= original_pa_list->mpunreach<IPv6>(safi);
    if (!mpunreach)
	return false;
    
    list<IPNet<IPv6> >::const_iterator wi;
    wi = mpunreach->wr_list().begin();
    while (wi != mpunreach->wr_list().end()) {
	switch(safi) {
	case SAFI_UNICAST:
	    _plumbing_unicast->delete_route(*wi, this);
	    break;
	case SAFI_MULTICAST:
	    _plumbing_multicast->delete_route(*wi, this);
	    break;
	}
	++wi;
    }

    return true;
}


#endif


template <>
bool
PeerHandler::withdraw<IPv4>(const UpdatePacket *p, 
			    ref_ptr<FastPathAttributeList<IPv4> >& original_pa_list,
			    Safi safi)
{
    switch(safi) {
    case SAFI_UNICAST: {
	if (p->wr_list().empty())
	    return false;

	BGPUpdateAttribList::const_iterator wi;
	wi = p->wr_list().begin();
	while (wi != p->wr_list().end()) {
	    _plumbing_unicast->delete_route(wi->net(), this);
	    ++wi;
	}
    }
	break;
    case SAFI_MULTICAST: {
	const MPUNReachNLRIAttribute<IPv4> *mpunreach 
	    = original_pa_list->mpunreach<IPv4>(safi);
	if (!mpunreach)
	    return false;
    
	list<IPNet<IPv4> >::const_iterator wi;
	wi = mpunreach->wr_list().begin();
	while (wi != mpunreach->wr_list().end()) {
	    _plumbing_multicast->delete_route(*wi, this);
	    ++wi;
	}
    }
	break;
    }

    return true;
}

inline
void
set_if_true(bool& orig, bool now)
{
    if (now)
	orig = true;
}

/*
 * process_update_packet is called when we've received an Update
 * message from the peer.  We break down the update into its pieces,
 * and pass the information into the plumbing to be stored in the
 * RibIn and distributed as required.
 */

int
PeerHandler::process_update_packet(UpdatePacket *p)
{
    debug_msg("Processing packet\n %s\n", p->str().c_str());

    FPAList4Ref pa_list = p->pa_list();

    FPAList4Ref pa_ipv4_unicast = new FastPathAttributeList<IPv4>();
    FPAList4Ref pa_ipv4_multicast = new FastPathAttributeList<IPv4>();
#ifdef HAVE_IPV6
    FPAList6Ref pa_ipv6_unicast	= new FastPathAttributeList<IPv6>();
    FPAList6Ref pa_ipv6_multicast = new FastPathAttributeList<IPv6>();
    bool ipv6_unicast = false;
    bool ipv6_multicast = false;
#endif
    XLOG_ASSERT(!pa_ipv4_unicast->is_locked());
    bool ipv4_unicast = false;
    bool ipv4_multicast = false;
    
    const PathAttribute* pa;

    // Store a reference to the ASPath here temporarily, as we may
    // need to mess with it before passing it to the final PA lists.
    // It's safe to mess with the ASPath in place, as we won't need
    // the original after this.
    ASPath* as_path = 0;
    if (!pa_list->is_empty()) {
	if (pa_list->aspath_att())
	    as_path = const_cast<ASPath*>(&(pa_list->aspath()));

	for (int i = 0; i < pa_list->max_att(); i++) {
	    pa = pa_list->find_attribute_by_type((PathAttType)i);
	    if (pa) {
		switch((PathAttType)i) {
		case AS_PATH:
		    // add this later.
		    continue;
		case AS4_PATH:
		    if (_peer->use_4byte_asnums()) {
			// we got an AS4_PATH from a peer that shouldn't send
			// us one because it speaks native 4-byte AS numbers.
			// The spec says to silently discard the attribute.
			continue;
		    } else if (_peer->localdata()->use_4byte_asnums()) {
			// We got an AS4_PATH, but we're configured to use
			// 4-byte AS numbers.  We need to merge this into the
			// AS_PATH we've already decoded, then discard the
			// AS4_PATH as we don't need both.
			const AS4PathAttribute* as4attr = 
			    (const AS4PathAttribute*)(pa_list->as4path_att());
			XLOG_ASSERT(as_path);
			as_path->merge_as4_path(as4attr->as4_path());

			/* don't store the AS4path in the PA list */
			continue;
		    } else {
			// We got an AS4_PATH, but we're not configured to use
			// 4-byte AS numbers.  We just propagate this
			// unchanged, so fall through and add it to the
			// pa_list in the normal way.
		    }
		    break;

		case MP_REACH_NLRI:
#ifdef HAVE_IPV6
		    // we've got a multiprotocol NLRI to split out
		    if (dynamic_cast<const MPReachNLRIAttribute<IPv6>*>(pa)) {
			// it's IPv6
			const MPReachNLRIAttribute<IPv6>* mpreach =
			    dynamic_cast<const MPReachNLRIAttribute<IPv6>*>(pa);
			switch(mpreach->safi()) {
			case SAFI_UNICAST: {
			    IPv6NextHopAttribute nha(mpreach->nexthop());
			    pa_ipv6_unicast->add_path_attribute(nha);
			    pa_ipv6_unicast->add_path_attribute(*pa);
			    break;
			}
			case SAFI_MULTICAST: {
			    IPv6NextHopAttribute nha(mpreach->nexthop());
			    pa_ipv6_multicast->add_path_attribute(nha);
			    pa_ipv6_multicast->add_path_attribute(*pa);
			    break;
			}
			}
		    }
#endif

		    if (dynamic_cast<const MPReachNLRIAttribute<IPv4>*>(pa)) {
			// it's IPv4
			const MPReachNLRIAttribute<IPv4>* mpreach =
			    dynamic_cast<const MPReachNLRIAttribute<IPv4>*>(pa);
			switch(mpreach->safi()) {
			case SAFI_UNICAST:
			    // XXXX what should we do here?
			    XLOG_WARNING("AFI == IPv4 && SAFI == UNICAST???");
			    //pa_ipv4_unicast.
			    //add_path_attribute(IPv4NextHopAttribute(mpreach->nexthop()));
			    break;
			case SAFI_MULTICAST: {
			    IPv4NextHopAttribute nha(mpreach->nexthop());
			    pa_ipv4_multicast->add_path_attribute(nha);
			    pa_ipv4_multicast->add_path_attribute(*pa);
			    break;
			}
			}
		    }
		    continue;

		case MP_UNREACH_NLRI:
#ifdef HAVE_IPV6
		    if (dynamic_cast<const MPUNReachNLRIAttribute<IPv6>*>(pa)) {
			/* don't store this in the PA list */
			continue;
		    }
#endif

		    if (dynamic_cast<const MPUNReachNLRIAttribute<IPv4>*>(pa)) {
			/* don't store this in the PA list */
			continue;
		    }
		default:
		    /* the remaining attributes are handled normally below */
		    {}
		} /* end of switch */

		pa_ipv4_unicast->add_path_attribute(*pa);

		/*
		** The nexthop path attribute applies only to IPv4 Unicast case.
		*/
		if (NEXT_HOP != pa->type()) {
		    pa_ipv4_multicast->add_path_attribute(*pa);
#ifdef HAVE_IPV6
		    pa_ipv6_unicast->add_path_attribute(*pa);
		    pa_ipv6_multicast->add_path_attribute(*pa);
#endif
		}
	    } /* end of if */
	} /* end of for loop */
    } /* end of if pa_list */

    /* finally store the ASPath attribute, now we know we're done messing with it */
    if (as_path) {
	ASPathAttribute as_path_attr(*as_path);
	pa_ipv4_unicast->add_path_attribute(as_path_attr);
	pa_ipv4_multicast->add_path_attribute(as_path_attr);
#ifdef HAVE_IPV6
	pa_ipv6_unicast->add_path_attribute(as_path_attr);
	pa_ipv6_multicast->add_path_attribute(as_path_attr);
#endif
    }


    // IPv4 Unicast Withdraws
    set_if_true(ipv4_unicast, withdraw<IPv4>(p, pa_list, SAFI_UNICAST));

    // IPv4 Multicast Withdraws
    set_if_true(ipv4_multicast, withdraw<IPv4>(p, pa_list, SAFI_MULTICAST));

#ifdef HAVE_IPV6
    // IPv6 Unicast Withdraws
    set_if_true(ipv6_unicast, withdraw<IPv6>(p, pa_list, SAFI_UNICAST));

    // IPv6 Multicast Withdraws
    set_if_true(ipv6_multicast, withdraw<IPv6>(p, pa_list, SAFI_MULTICAST));
#endif

    // IPv4 Unicast Route add.
    XLOG_ASSERT(!pa_ipv4_unicast->is_locked());
    set_if_true(ipv4_unicast, add<IPv4>(p, pa_list, 
					pa_ipv4_unicast, SAFI_UNICAST));

    // IPv4 Multicast Route add.
    set_if_true(ipv4_multicast, add<IPv4>(p, pa_list, 
					  pa_ipv4_multicast,SAFI_MULTICAST));

#ifdef HAVE_IPV6
    // IPv6 Unicast Route add.
    set_if_true(ipv6_unicast, add<IPv6>(p, pa_list,
					pa_ipv6_unicast, SAFI_UNICAST));

    // IPv6 Multicast Route add.
    set_if_true(ipv6_multicast, add<IPv6>(p, pa_list,
					  pa_ipv6_multicast,SAFI_MULTICAST));
#endif

    if (ipv4_unicast)
	_plumbing_unicast->push<IPv4>(this);
    if (ipv4_multicast)
	_plumbing_multicast->push<IPv4>(this);
#ifdef HAVE_IPV6
    if (ipv6_unicast)
	_plumbing_unicast->push<IPv6>(this);
    if (ipv6_multicast)
	_plumbing_multicast->push<IPv6>(this);
#endif
    return 0;
}

template <>
bool 
PeerHandler::multiprotocol<IPv4>(Safi safi, BGPPeerData::Direction d) const
{
    return _peer->peerdata()->multiprotocol<IPv4>(safi, d);
}

int
PeerHandler::start_packet()
{
    XLOG_ASSERT(_packet == NULL);
    _packet = new UpdatePacket();
    return 0;
}

int
PeerHandler::add_route(const SubnetRoute<IPv4> &rt, 
		       ref_ptr<FastPathAttributeList<IPv4> >& pa_list,
		       bool /*ibgp*/, Safi safi)
{
    debug_msg("PeerHandler::add_route(IPv4) %p\n", &rt);
    XLOG_ASSERT(_packet != NULL);
    // if a route came from IBGP, it shouldn't go to IBGP (unless
    // we're a route reflector)
//     if (ibgp)
// 	XLOG_ASSERT(!_peer->ibgp());

    // Check this peer wants this NLRI
    if (!multiprotocol<IPv4>(safi, BGPPeerData::NEGOTIATED))
	return 0;

    if (_packet->big_enough()) {
	push_packet();
	start_packet();
    }

    // did we already add the packet attribute list?
    if (_packet->pa_list()->is_empty()) {
	debug_msg("First add on this packet\n");
	debug_msg("SubnetRoute is %s\n", rt.str().c_str());
	// no, so add all the path attributes
	_packet->replace_pathattribute_list(pa_list);
#if 0
	PathAttributeList<IPv4>::const_iterator pai;
	pai = rt.attributes()->begin();
	while (pai != rt.attributes()->end()) {
	    debug_msg("Adding attribute %s\n", (*pai)->str().c_str());
	    /*
	    ** Don't put a multicast nexthop in the parameter list.
	    */
	    if (!(NEXT_HOP == (*pai)->type() && safi == SAFI_MULTICAST))
		_packet.add_pathatt(**pai);
	    ++pai;
	}
#endif
	if (SAFI_MULTICAST == safi) {
	    // Multicast SAFI packets don't have a regular nexthop,
	    // but have on in the MultiProtocol attribute instead.
	    _packet->pa_list()->remove_attribute_by_type(NEXT_HOP);
	    MPReachNLRIAttribute<IPv4> mp(safi);
	    mp.set_nexthop(pa_list->nexthop());
	    _packet->add_pathatt(mp);
	}
    }

    // add the NLRI information.
    switch(safi) {
    case SAFI_UNICAST: {
	BGPUpdateAttrib nlri(rt.net());
 	XLOG_ASSERT(_packet->pa_list()->nexthop() == pa_list->nexthop());
	_packet->add_nlri(nlri);
    }
	break;
    case SAFI_MULTICAST: {
	XLOG_ASSERT(pa_list->mpreach<IPv4>(SAFI_MULTICAST));
	XLOG_ASSERT(pa_list->mpreach<IPv4>(SAFI_MULTICAST)->nexthop() == 
		    pa_list->nexthop());
	MPReachNLRIAttribute<IPv4>* mpreach_att =
	    _packet->pa_list()->mpreach<IPv4>(SAFI_MULTICAST);
	XLOG_ASSERT(mpreach_att);
	mpreach_att->add_nlri(rt.net());
    }
	break;
    }

    return 0;
}

int
PeerHandler::replace_route(const SubnetRoute<IPv4> &old_rt,
			   bool /*old_ibgp*/, 
			   const SubnetRoute<IPv4> &new_rt, 
			   bool new_ibgp, 
			   FPAList4Ref& pa_list, Safi safi)
{
    // in the basic PeerHandler, we can ignore the old route, because
    // to change a route, the BGP protocol just sends the new
    // replacement route
    UNUSED(old_rt);
    return add_route(new_rt, pa_list, new_ibgp, safi);
}

int
PeerHandler::delete_route(const SubnetRoute<IPv4> &rt, 
			  FPAList4Ref& /*pa_list*/,
			  bool /*ibgp*/, 
			  Safi safi)
{
    debug_msg("PeerHandler::delete_route(IPv4) %p\n", &rt);
    XLOG_ASSERT(_packet != NULL);

    // Check this peer wants this NLRI
    if (!multiprotocol<IPv4>(safi, BGPPeerData::NEGOTIATED))
	return 0;

    if (_packet->big_enough()) {
	push_packet();
	start_packet();
    }

    if (SAFI_MULTICAST == safi && 0 == _packet->pa_list()->mpunreach<IPv4>(safi)) {
	MPUNReachNLRIAttribute<IPv4>* mp = new MPUNReachNLRIAttribute<IPv4>(safi);
	_packet->pa_list()->add_path_attribute(mp);
    }
    
    switch(safi) {
    case SAFI_UNICAST: {
	BGPUpdateAttrib wdr(rt.net());
	_packet->add_withdrawn(wdr);
    }
	break;
    case SAFI_MULTICAST: {
	XLOG_ASSERT(_packet->pa_list()->mpunreach<IPv4>(SAFI_MULTICAST));
	_packet->pa_list()->mpunreach<IPv4>(SAFI_MULTICAST)->add_withdrawn(rt.net());
    }
	break;
    }

    return 0;
}

PeerOutputState
PeerHandler::push_packet()
{
    debug_msg("PeerHandler::push_packet - sending packet:\n %s\n",
	      _packet->str().c_str());

    // do some sanity checking
    XLOG_ASSERT(_packet);
    int wdr = _packet->wr_list().size();
    int nlri = _packet->nlri_list().size();

    if(_packet->pa_list()->mpreach<IPv4>(SAFI_MULTICAST))
	nlri += _packet->pa_list()->mpreach<IPv4>(SAFI_MULTICAST)->nlri_list().size();
    if(_packet->pa_list()->mpunreach<IPv4>(SAFI_MULTICAST))
	wdr += _packet->pa_list()->mpunreach<IPv4>(SAFI_MULTICAST)->wr_list().size();

#ifdef HAVE_IPV6
    // Account for IPv6
    if(_packet->pa_list()->mpreach<IPv6>(SAFI_UNICAST))
	nlri += _packet->pa_list()->mpreach<IPv6>(SAFI_UNICAST)->nlri_list().size();
    if(_packet->pa_list()->mpunreach<IPv6>(SAFI_UNICAST))
	wdr += _packet->pa_list()->mpunreach<IPv6>(SAFI_UNICAST)->wr_list().size();
    if(_packet->pa_list()->mpreach<IPv6>(SAFI_MULTICAST))
	nlri += _packet->pa_list()->mpreach<IPv6>(SAFI_MULTICAST)->nlri_list().size();
    if(_packet->pa_list()->mpunreach<IPv6>(SAFI_MULTICAST))
	wdr += _packet->pa_list()->mpunreach<IPv6>(SAFI_MULTICAST)->wr_list().size();
#endif

//     XLOG_ASSERT( (wdr+nlri) > 0);
    // If we get here and wdr+nlri equals zero then we were trying to
    // push an AFI/SAFI that our peer did not request so just drop
    // the packet and return.
    if (0 == (wdr+nlri)) {
	delete _packet;
	_packet = NULL;
	return PEER_OUTPUT_OK;
    }
    
    if (nlri > 0)
	XLOG_ASSERT(!_packet->pa_list()->is_empty());

    _nlri_total += nlri;
    _packets++;
    debug_msg("Mean packet has %f nlri's\n", ((float)_nlri_total)/_packets);

    PeerOutputState result;
    result = _peer->send_update_message(*_packet);
    delete _packet;
    _packet = NULL;
    return result;
}

void
PeerHandler::output_no_longer_busy()
{
    if (_peering_is_up) {
	_plumbing_unicast->output_no_longer_busy(this);
	_plumbing_multicast->output_no_longer_busy(this);
    }
}

uint32_t
PeerHandler::get_prefix_count() const
{
    return _plumbing_unicast->get_prefix_count(this) +
	_plumbing_multicast->get_prefix_count(this);
}

EventLoop&
PeerHandler::eventloop() const
{
    return _peer->main()->eventloop();
}


/** IPv6 stuff */
#ifdef HAVE_IPV6

template <>
bool
PeerHandler::multiprotocol<IPv6>(Safi safi, BGPPeerData::Direction d) const
{
    return _peer->peerdata()->multiprotocol<IPv6>(safi, d);
}


int
PeerHandler::add_route(const SubnetRoute<IPv6> &rt, 
		       ref_ptr<FastPathAttributeList<IPv6> >& pa_list,
		       bool /*ibgp*/, Safi safi)
{
    debug_msg("PeerHandler::add_route(IPv6) %p\n", &rt);
    XLOG_ASSERT(_packet != NULL);
    // if a route came from IBGP, it shouldn't go to IBGP (unless
    // we're a route reflector)
//     if (ibgp)
// 	XLOG_ASSERT(!_peer->ibgp());

    // Check this peer wants this NLRI
    if (!multiprotocol<IPv6>(safi, BGPPeerData::NEGOTIATED))
	return 0;

    if (_packet->big_enough()) {
	push_packet();
	start_packet();
    }

    // did we already add the packet attribute list?
    if (_packet->pa_list()->is_empty() && !pa_list->is_empty()) {
	// no, so add all the path attributes
	for (int i = 0; i < MAX_ATTRIBUTE; i++) {
	    const PathAttribute* pa;
	    pa = pa_list->find_attribute_by_type((PathAttType)i);
	    if (pa && i != NEXT_HOP) {
		/*
		** Don't put an IPv6 next hop in the IPv4 path attribute list.
		*/
		_packet->add_pathatt(*pa);
	    }
	}
	MPReachNLRIAttribute<IPv6> mp(safi);
	mp.set_nexthop(pa_list->nexthop());
	_packet->add_pathatt(mp);
    }

    MPReachNLRIAttribute<IPv6>* mpreach_att =
	_packet->pa_list()->mpreach<IPv6>(safi);
    XLOG_ASSERT(mpreach_att);
    XLOG_ASSERT(mpreach_att->nexthop() == pa_list->nexthop());
    mpreach_att->add_nlri(rt.net());

    return 0;
}


int
PeerHandler::replace_route(const SubnetRoute<IPv6> &old_rt,
			   bool /*old_ibgp*/, 
			   const SubnetRoute<IPv6> &new_rt, 
			   bool new_ibgp, 
			   FPAList6Ref& pa_list, Safi safi)
{
    // in the basic PeerHandler, we can ignore the old route, because
    // to change a route, the BGP protocol just sends the new
    // replacement route
    UNUSED(old_rt);
    return add_route(new_rt, pa_list, new_ibgp, safi);
}



int
PeerHandler::delete_route(const SubnetRoute<IPv6>& rt, 
			  FPAList6Ref& /*pa_list*/,
			  bool /*ibgp*/, 
			  Safi safi)
{
    debug_msg("PeerHandler::delete_route(IPv6) %p\n", &rt);
    XLOG_ASSERT(_packet != NULL);

    // Check this peer wants this NLRI
    if (!multiprotocol<IPv6>(safi, BGPPeerData::NEGOTIATED))
	return 0;

    if (_packet->big_enough()) {
	push_packet();
	start_packet();
    }

    if (0 == _packet->pa_list()->mpunreach<IPv6>(safi)) {
	MPUNReachNLRIAttribute<IPv6>* mp = new MPUNReachNLRIAttribute<IPv6>(safi);
	_packet->pa_list()->add_path_attribute(mp);
    }

    XLOG_ASSERT(_packet->pa_list()->mpunreach<IPv6>(safi));
    _packet->pa_list()->mpunreach<IPv6>(safi)->add_withdrawn(rt.net());

    return 0;
}

#endif // ipv6
