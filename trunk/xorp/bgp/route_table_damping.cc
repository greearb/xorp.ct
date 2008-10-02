// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "libxorp/xlog.h"
#include "bgp.hh"
#include "route_table_damping.hh"

template<class A>
DampingTable<A>::DampingTable(string tablename, Safi safi,
			      BGPRouteTable<A>* parent,
			      const PeerHandler *peer,
			      Damping& damping)
    : BGPRouteTable<A>(tablename, safi), _peer(peer), _damping(damping),
      _damp_count(0)
{
    this->_parent = parent;
}

template<class A>
DampingTable<A>::~DampingTable()
{
}

template<class A>
int
DampingTable<A>::add_route(const InternalMessage<A> &rtmsg,
			   BGPRouteTable<A> *caller)
{
    debug_msg("\n         %s\n caller: %s\n rtmsg: %p route: %p\n%s\n",
	      this->tablename().c_str(),
	      caller ? caller->tablename().c_str() : "NULL",
	      &rtmsg,
	      rtmsg.route(),
	      rtmsg.str().c_str());

    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(this->_next_table != NULL);

    if (!damping())
	return this->_next_table->
	    add_route(rtmsg, static_cast<BGPRouteTable<A>*>(this));

    if (!damping_global())
	return this->_next_table->
	    add_route(rtmsg, static_cast<BGPRouteTable<A>*>(this));

    // The first route for this network.
    typename Trie<A, Damp>::iterator i = _damp.lookup_node(rtmsg.net());
    if (i == _damp.end()) {
	Damp damp(_damping.get_tick(), _damping.get_merit());
	_damp.insert(rtmsg.net(), damp);
	return this->_next_table->
	    add_route(rtmsg, static_cast<BGPRouteTable<A>*>(this));
    }

    // Update the figure of merit and damp if necessary.
    Damp& damp = i.payload();
    if (update_figure_of_merit(damp, rtmsg))
	return ADD_UNUSED;

    return this->_next_table->
	add_route(rtmsg, static_cast<BGPRouteTable<A>*>(this));
}

template<class A>
int
DampingTable<A>::replace_route(const InternalMessage<A> &old_rtmsg, 
			       const InternalMessage<A> &new_rtmsg, 
			       BGPRouteTable<A> *caller)
{
    debug_msg("\n         %s\n"
	      "caller: %s\n"
	      "old rtmsg: %p new rtmsg: %p "
	      "old route: %p"
	      "new route: %p"
	      "old: %s\n new: %s\n",
	      this->tablename().c_str(),
	      caller ? caller->tablename().c_str() : "NULL",
	      &old_rtmsg,
	      &new_rtmsg,
	      old_rtmsg.route(),
	      new_rtmsg.route(),
	      old_rtmsg.str().c_str(),
	      new_rtmsg.str().c_str());

    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(this->_next_table != NULL);

    IPNet<A> net = old_rtmsg.net();
    XLOG_ASSERT(net == new_rtmsg.net());

    if (!damping())
	return this->_next_table->
	    replace_route(old_rtmsg, new_rtmsg,
			  static_cast<BGPRouteTable<A>*>(this));

    // Find the record for this route which must exist. If the route
    // was being damped continue to damp.
    typename Trie<A, Damp>::iterator i = _damp.lookup_node(old_rtmsg.net());
    // An entry should be found, but if damping was enabled after the
    // original route passed through here it won't be found.
    if (i == _damp.end()) {
	return this->_next_table->
	    replace_route(old_rtmsg, new_rtmsg,
			  static_cast<BGPRouteTable<A>*>(this));
    }
    Damp& damp = i.payload();
    if (damp._damped) {
	typename RefTrie<A, DampRoute<A> >::iterator r;
	r = _damped.lookup_node(old_rtmsg.net());
	XLOG_ASSERT(r != _damped.end());
	TimeVal exp;
	if (!r.payload().timer().time_remaining(exp))
	    XLOG_FATAL("Route is being damped but no timer is scheduled");
	r.payload().timer().unschedule();
	_damped.erase(r);
	if (damping_global()) {
		DampRoute<A> damproute(new_rtmsg.route(), new_rtmsg.genid());
		damproute.timer() = eventloop().
		    new_oneoff_after(exp,
				     callback(this,
					      &DampingTable<A>::undamp,
					      new_rtmsg.net()));
		_damped.insert(new_rtmsg.net(), damproute);
	    return ADD_UNUSED;
	}
	
	// Routes are no longer being damped, but they were previously
	// send the new route through as an add.
	damp._damped = false;
	_damp_count--;
	return this->_next_table->
	    add_route(new_rtmsg, static_cast<BGPRouteTable<A>*>(this));
    }

    if (update_figure_of_merit(damp, new_rtmsg)) {
	this->_next_table->
	    delete_route(old_rtmsg, static_cast<BGPRouteTable<A>*>(this));
	return ADD_UNUSED;
    }

    return this->_next_table->
	replace_route(old_rtmsg, new_rtmsg,
		      static_cast<BGPRouteTable<A>*>(this));
}

template<class A>
int
DampingTable<A>::delete_route(const InternalMessage<A> &rtmsg, 
			    BGPRouteTable<A> *caller)
{
    debug_msg("\n         %s\n caller: %s\n rtmsg: %p route: %p\n%s\n",
	      this->tablename().c_str(),
	      caller ? caller->tablename().c_str() : "NULL",
	      &rtmsg,
	      rtmsg.route(),
	      rtmsg.str().c_str());

    XLOG_ASSERT(caller == this->_parent);
    XLOG_ASSERT(this->_next_table != NULL);

    if (!damping())
	return this->_next_table->
	    delete_route(rtmsg, static_cast<BGPRouteTable<A>*>(this));


    // Don't update the figure of merit just remove the route if it
    // was being damped.
    typename Trie<A, Damp>::iterator i = _damp.lookup_node(rtmsg.net());
    // An entry should be found, but if damping was enabled after the
    // original route passed through here it won't be found.
    if (i == _damp.end()) {
	return this->_next_table->
	    delete_route(rtmsg, static_cast<BGPRouteTable<A>*>(this));
    }
    Damp& damp = i.payload();
    if (damp._damped) {
	typename RefTrie<A, DampRoute<A> >::iterator r;
	r = _damped.lookup_node(rtmsg.net());
	XLOG_ASSERT(r != _damped.end());
	r.payload().timer().unschedule();	// Not strictly necessary.
	_damped.erase(r);

	damp._damped = false;
	_damp_count--;

	return 0;
    }

    return this->_next_table->
	delete_route(rtmsg, static_cast<BGPRouteTable<A>*>(this));
}

template<class A>
int
DampingTable<A>::push(BGPRouteTable<A> *caller)
{
    XLOG_ASSERT(caller == this->_parent);
    return this->_next_table->push((BGPRouteTable<A>*)this);
}

template<class A>
int 
DampingTable<A>::route_dump(const InternalMessage<A> &rtmsg,
			    BGPRouteTable<A> *caller,
			    const PeerHandler *dump_peer)
{
    XLOG_ASSERT(caller == this->_parent);

    if (!damping())
	return this->_next_table->
	    route_dump(rtmsg, static_cast<BGPRouteTable<A>*>(this), dump_peer);

    if (is_this_route_damped(rtmsg.net()))
	return ADD_UNUSED;
    
    return this->_next_table->
	route_dump(rtmsg, static_cast<BGPRouteTable<A>*>(this), dump_peer);
}

template<class A>
const SubnetRoute<A>*
DampingTable<A>::lookup_route(const IPNet<A> &net,
			      uint32_t& genid) const
{
    if (!damping())
	return this->_parent->lookup_route(net, genid);

    if (is_this_route_damped(net))
	return 0;

    return this->_parent->lookup_route(net, genid);
}

template<class A>
void
DampingTable<A>::route_used(const SubnetRoute<A>* rt, bool in_use)
{
    if (!damping())
	this->_parent->route_used(rt, in_use);

    if (is_this_route_damped(rt->net()))
	XLOG_FATAL("A damped route can't be used");

    this->_parent->route_used(rt, in_use);
}

template<class A>
bool
DampingTable<A>::update_figure_of_merit(Damp& damp,
					const InternalMessage<A> &rtmsg)
{
    // If damping has been disabled but some routes are still being
    // damped just return false.
    if (!_damping.get_damping())
	return false;

    damp._merit = _damping.compute_merit(damp._time, damp._merit);
    damp._time = _damping.get_tick();

    debug_msg("\n         %s\n rtmsg: %p route: %p\n%s\n",
	      this->tablename().c_str(),
	      &rtmsg,
	      rtmsg.route(),
	      rtmsg.str().c_str());
    debug_msg("Merit %d\n", damp._merit);

    // The figure of merit is above the cutoff threshold damp the route.
    if (_damping.cutoff(damp._merit)) {
	debug_msg("Damped\n");
	damp._damped = true;
	_damp_count++;
	DampRoute<A> damproute(rtmsg.route(), rtmsg.genid());
	damproute.timer() = eventloop().
	    new_oneoff_after(TimeVal(_damping.get_reuse_time(damp._merit), 0),
			     callback(this,
				      &DampingTable<A>::undamp,
				      rtmsg.net()));
	_damped.insert(rtmsg.net(), damproute);

	return true;
    }

    return false;
}

template<class A>
bool
DampingTable<A>::is_this_route_damped(const IPNet<A> &net) const
{
    typename Trie<A, Damp>::iterator i = _damp.lookup_node(net);
    if (i == _damp.end())
	return false;

    if (i.payload()._damped)
	return true;

    return false;
}

template<class A>
void
DampingTable<A>::undamp(IPNet<A> net)
{
    debug_msg("Released net %s\n", cstring(net));

    typename Trie<A, Damp>::iterator i = _damp.lookup_node(net);
    XLOG_ASSERT(i != _damp.end());
    Damp& damp = i.payload();
    XLOG_ASSERT(damp._damped);
    
    typename RefTrie<A, DampRoute<A> >::iterator r;
    r = _damped.lookup_node(net);
    XLOG_ASSERT(r != _damped.end());
    InternalMessage<A> rtmsg(r.payload().route(), _peer, r.payload().genid());
    _damped.erase(r);
    damp._damped = false;
    _damp_count--;

    this->_next_table->add_route(rtmsg,
				 static_cast<BGPRouteTable<A>*>(this));
    this->_next_table->push(static_cast<BGPRouteTable<A>*>(this));
}

template<class A>
EventLoop& 
DampingTable<A>::eventloop() const 
{
    return _peer->eventloop();
}


template<class A>
string
DampingTable<A>::str() const
{
    string s = "DampingTable<A>" + this->tablename();
    return s;
}

template class DampingTable<IPv4>;
template class DampingTable<IPv6>;
