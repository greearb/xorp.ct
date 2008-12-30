// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/rib/rt_tab_redist.cc,v 1.26 2008/07/23 05:11:32 pavlin Exp $"

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include "rt_tab_redist.hh"
#include "rt_tab_origin.hh"
#include "redist_policy.hh"


// ----------------------------------------------------------------------------
// RedistOutput<A>

template <typename A>
RedistOutput<A>::RedistOutput(Redistributor<A>* r)
    : _r(r)
{
}

template <typename A>
RedistOutput<A>::~RedistOutput()
{
}


// ----------------------------------------------------------------------------
// Redistributor<A>

template <>
const IPv4Net Redistributor<IPv4>::NO_LAST_NET(IPv4::ALL_ONES(),
					       IPv4::ADDR_BITLEN);
template <>
const IPv6Net Redistributor<IPv6>::NO_LAST_NET(IPv6::ALL_ONES(),
					       IPv6::ADDR_BITLEN);

template <typename A>
Redistributor<A>::Redistributor(EventLoop& 	e,
				const string& 	n)
    : _e(e), _name(n), _table(0), _output(0), _policy(0),
      _rei(this), _oei(this), _dumping(false), _blocked(false)
{
}

template <typename A>
Redistributor<A>::~Redistributor()
{
    delete _output;
    delete _policy;
}

template <typename A>
const string&
Redistributor<A>::name() const
{
    return _name;
}

template <typename A>
void
Redistributor<A>::set_redist_table(RedistTable<A>* rt)
{
    if (_table) {
	_table->remove_redistributor(this);
    }

    _table = rt;

    if (_table) {
	_table->add_redistributor(this);
	start_dump();
    }
}

template <typename A>
void
Redistributor<A>::set_output(RedistOutput<A>* o)
{
    if (_output)
	delete _output;

    _output = o;
    _blocked = false;
    if (_output) {
	start_dump();
    }
}

template <typename A>
void
Redistributor<A>::set_policy(RedistPolicy<A>* rpo)
{
    if (_policy)
	delete _policy;
    _policy = rpo;
}

template <typename A>
bool
Redistributor<A>::policy_accepts(const IPRouteEntry<A>& ipr) const
{
    return (_policy == 0) || _policy->accept(ipr);
}

template <typename A>
void
Redistributor<A>::start_dump()
{
    if (_output != 0 && _table != 0) {
	_dumping = true;
	_last_net = NO_LAST_NET;
	schedule_dump_timer();
	debug_msg("starting dump\n");
	_output->starting_route_dump();
    }
}

template <typename A>
void
Redistributor<A>::finish_dump()
{
    _dumping = false;
    _last_net = NO_LAST_NET;
    debug_msg("finishing dump\n");
    if (_output)
	_output->finishing_route_dump();
}

template <typename A>
void
Redistributor<A>::schedule_dump_timer()
{
    XLOG_ASSERT(_blocked == false);
    debug_msg("schedule dump timer\n");
    _dtimer = _e.new_oneoff_after_ms(0,
				     callback(this,
					      &Redistributor<A>::dump_a_route)
				     );
}

template <typename A>
void
Redistributor<A>::unschedule_dump_timer()
{
    debug_msg("unschedule dump timer\n");
    _dtimer.unschedule();
}

template <typename A>
void
Redistributor<A>::dump_a_route()
{
    XLOG_ASSERT(_dumping == true);

    typename RedistTable<A>::RouteIndex::const_iterator end;
    end = _table->route_index().end();

    typename RedistTable<A>::RouteIndex::const_iterator ci;

    // Find net associated with route to dump
    if (_last_net == NO_LAST_NET) {
	ci = _table->route_index().begin();
    } else {
	ci = _table->route_index().find(_last_net);
	XLOG_ASSERT(ci != end);
	ci++;
    }

    if (ci == end) {
	finish_dump();
	return;
    }

    // Lookup route and announce it via output
    const IPRouteEntry<A>* ipr = _table->lookup_route(*ci);
    XLOG_ASSERT(ipr != 0);
    if (policy_accepts(*ipr))
	_output->add_route(*ipr);

    // Record last net dumped and reschedule
    _last_net = *ci;

    // Check blocked as it may have been set by output's add_route()
    if (_blocked == false)
	schedule_dump_timer();
}


// ----------------------------------------------------------------------------
// Event Notification handling

template <typename A>
void
Redistributor<A>::RedistEventInterface::did_add(const IPRouteEntry<A>& ipr)
{
    if (_r->policy_accepts(ipr) == false) {
	return;
    }

    if (_r->dumping() == true) {
	// We're in a dump, if the route is greater than current
	// position we ignore it as it'll be picked up later,
	// otherwise we need to announce route now.
	if (_r->last_dumped_net() == Redistributor<A>::NO_LAST_NET) {
	    return;		// dump not started
	}

	const IPNet<A>& last = _r->last_dumped_net();
	const IPNet<A>& net  = ipr.net();
	if (RedistNetCmp<A>().operator() (net, last) == false){
	    return;	// route will be hit later on in dump anyway
	}
    }
    _r->output()->add_route(ipr);
}

template <typename A>
void
Redistributor<A>::RedistEventInterface::will_delete(const IPRouteEntry<A>& ipr)
{
    if (_r->policy_accepts(ipr) == false) {
	return;
    }

    // We only care that a route is about to be deleted when we are
    // doing the initial route dump.  The pending delete may affect the
    // last route dumped and as a result nobble our iteration of the valid
    // routes set.
    if (_r->dumping() == false ||
	_r->last_dumped_net() == Redistributor<A>::NO_LAST_NET) {
	return;
    }

    if (ipr.net() == _r->last_dumped_net()) {
	// The pending delete affects last announced net.
	//
	// 1. Need to step back to previous net so next announcement will work
	//    correctly.
	//
	typename RedistTable<A>::RouteIndex::const_iterator ci;
	ci = _r->redist_table()->route_index().find(_r->last_dumped_net());
	XLOG_ASSERT(ci != _r->redist_table()->route_index().end());
	if (ci == _r->redist_table()->route_index().begin()) {
	    _r->_last_net = Redistributor<A>::NO_LAST_NET;
	} else {
	    --ci;
	    _r->_last_net = *ci;
	}

	//
	// 2. Announce delete.
	//
	_r->output()->delete_route(ipr);
    }
}

template <typename A>
void
Redistributor<A>::RedistEventInterface::did_delete(const IPRouteEntry<A>& ipr)
{
    if (_r->policy_accepts(ipr) == false) {
	return;
    }

    if (_r->dumping()) {
	if (_r->last_dumped_net() == Redistributor<A>::NO_LAST_NET) {
	    debug_msg("did_delete with no last net\n");
	    return;	// Dump not started
	}

	const IPNet<A>& last = _r->last_dumped_net();
	const IPNet<A>& net  = ipr.net();
	if (RedistNetCmp<A>().operator() (net, last) == false) {
	    return;	// route has not yet been announced so ignore
	}
    }
    _r->output()->delete_route(ipr);
}

template <typename A>
void
Redistributor<A>::OutputEventInterface::low_water()
{
    debug_msg("low water\n");
    // Fallen below low water, take hint and resume dumping
    if (_r->dumping()) {
	_r->_blocked = false;
	_r->schedule_dump_timer();
    }
}

template <typename A>
void
Redistributor<A>::OutputEventInterface::high_water()
{
    // Risen above high water, take hint and stop dumping
    debug_msg("high water\n");
    if (_r->dumping()) {
	_r->unschedule_dump_timer();
	_r->_blocked = true;
    }
}

template <typename A>
void
Redistributor<A>::OutputEventInterface::fatal_error()
{
    _r->redist_table()->remove_redistributor(_r);
    delete _r;
}

// ----------------------------------------------------------------------------
// RedistTable

template <typename A>
RedistTable<A>::RedistTable(const string& tablename,
			    RouteTable<A>* parent)
    : RouteTable<A>(tablename), _parent(parent)
{
    if (_parent->next_table()) {
	set_next_table(_parent->next_table());
	this->next_table()->replumb(_parent, this);
    }
    _parent->set_next_table(this);
}

template <typename A>
RedistTable<A>::~RedistTable()
{
    while (_outputs.empty() == false) {
	delete _outputs.front();
	_outputs.pop_front();
    }
}

template <typename A>
void
RedistTable<A>::add_redistributor(Redistributor<A>* r)
{
    if (find(_outputs.begin(), _outputs.end(), r) == _outputs.end())
	_outputs.push_back(r);
}

template <typename A>
void
RedistTable<A>::remove_redistributor(Redistributor<A>* r)
{
    typename list<Redistributor<A>*>::iterator i =
	find(_outputs.begin(), _outputs.end(), r);

    if (i != _outputs.end()) {
	_outputs.erase(i);
    }
}

template <typename A>
Redistributor<A>*
RedistTable<A>::redistributor(const string& name)
{
    typename list<Redistributor<A>*>::iterator i = _outputs.begin();
    while (i != _outputs.end()) {
	Redistributor<A>* r = *i;
	if (r->name() == name)
	    return r;
	++i;
    }
    return 0;
}

template <typename A>
int
RedistTable<A>::add_route(const IPRouteEntry<A>& route, RouteTable<A>* caller)
{
    XLOG_ASSERT(caller == _parent);

    typename RouteIndex::iterator rci = _rt_index.find(route.net());
    XLOG_ASSERT(rci == _rt_index.end());

    _rt_index.insert(route.net());

    typename list<Redistributor<A>*>::iterator i = _outputs.begin();
    while (i != _outputs.end()) {
	Redistributor<A>* r = *i;
	i++;	// XXX for safety increment iterator before prodding output
	r->redist_event().did_add(route);
    }

    if (this->next_table())
	return this->next_table()->add_route(route, this);
    return XORP_OK;
}

template <typename A>
int
RedistTable<A>::delete_route(const IPRouteEntry<A>* r,
			     RouteTable<A>* caller)
{
    XLOG_ASSERT(caller == _parent);

    const IPRouteEntry<A>& route = *r;

    debug_msg("delete_route for %s\n", route.net().str().c_str());

    typename RouteIndex::iterator rci = _rt_index.find(route.net());
    XLOG_ASSERT(rci != _rt_index.end());

    typename list<Redistributor<A>*>::iterator i;

    // Announce intent to delete route
    i = _outputs.begin();
    while (i != _outputs.end()) {
	Redistributor<A>* r = *i;
	i++;	// XXX for safety increment iterator before prodding output
	r->redist_event().will_delete(route);
    }

    _rt_index.erase(rci);

    // Announce delete as fait accompli
    i = _outputs.begin();
    while (i != _outputs.end()) {
	Redistributor<A>* r = *i;
	i++;	// XXX for safety increment iterator before prodding output
	r->redist_event().did_delete(route);
    }

    if (this->next_table())
	return this->next_table()->delete_route(r, this);

    return XORP_OK;
}


// ----------------------------------------------------------------------------
// Standard RouteTable methods, RedistTable punts everything to parent.

template <typename A>
const IPRouteEntry<A>*
RedistTable<A>::lookup_route(const IPNet<A>& net) const
{
    return _parent->lookup_route(net);
}

template <typename A>
const IPRouteEntry<A>*
RedistTable<A>::lookup_route(const A& addr) const
{
    return _parent->lookup_route(addr);
}

template <typename A>
RouteRange<A>*
RedistTable<A>::lookup_route_range(const A& addr) const
{
    return _parent->lookup_route_range(addr);
}

template <typename A>
void
RedistTable<A>::replumb(RouteTable<A>* old_parent, RouteTable<A>* new_parent)
{
    XLOG_ASSERT(old_parent == _parent);
    _parent = new_parent;
}

template <typename A>
string
RedistTable<A>::str() const
{
    string s;
    s = "-------\nRedistTable: " + this->tablename() + "\n";

    if (_outputs.empty() == false) {
	s += "outputs:\n";
	typename list<Redistributor<A>*>::const_iterator i = _outputs.begin();
	while (i != _outputs.end()) {
	    const Redistributor<A>* r = *i;
	    s += "\t" + r->name() + "\n";
	    ++i;
	}
    }

    if (this->next_table() == NULL) {
	s += "no next table\n";
    } else {
	s += "next table = " + this->next_table()->tablename() + "\n";
    }

    return s;
}

// ----------------------------------------------------------------------------
// Instantiations

template class RedistOutput<IPv4>;
template class RedistOutput<IPv6>;

template class Redistributor<IPv4>;
template class Redistributor<IPv6>;

template class RedistTable<IPv4>;
template class RedistTable<IPv6>;

