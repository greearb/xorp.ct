// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/rib/rt_tab_redist.cc,v 1.9 2004/03/25 01:45:09 hodson Exp $"

#include "rib_module.h"

#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/trie.hh"

#include "rt_tab_redist.hh"
#include "rt_tab_origin.hh"

/**
 * @short A network comparitor class for the purposes of ordering
 * networks in sorted containers.  We only care about uniqueness here
 * for the purposes of set membership.
 */
template <typename A>
struct NetCmp {
    typedef IPNet<A> Net;
    inline bool operator() (const Net& l, const Net& r) const
    {
	if (l.prefix_len() == r.prefix_len()) {
	    return l.masked_addr() < r.masked_addr();
	}
	return l.prefix_len() < r.prefix_len();
    }
};

/**
 * State for initial route dump from source table to redist table.
 */
template <typename A>
struct RedistDumpState {
public:
    typedef set<IPNet<A>,NetCmp<A> > RouteSet;

public:
    RouteSet	ds_nets; 		// Dump Sensitive nets - those
    					// passed to output during
    					// startup route dump.
    XorpTimer	dtimer;			// Timer for prodding dump.
    IPNet<A>	last_net;		// Last net dumped.
    uint32_t	last_n_routes;		// Last OriginTable route cnt.
    uint32_t	last_generation;	// Last OriginTable generatn.

    static const IPNet<A> NO_LAST_NET;	// Indicator value for last net invalid
};

template <typename A>
const IPNet<A> RedistDumpState<A>::NO_LAST_NET(A::ALL_ONES(), A::ADDR_BITLEN);



template<class A>
RedistTable<A>::RedistTable<A>(const string&	tablename,
			       OriginTable<A>*	from_table,
			       OriginTable<A>*	to_table,
			       EventLoop&	eventloop,
			       RedistOutputBase<A>* output)
    : RouteTable<A>(tablename),
      _e(eventloop), _from_table(from_table), _parent(from_table),
      _to_table_name(to_table->tablename()),
      _output(output), _ds(0)
{
    // Plumb ourselves into the table graph
    set_next_table(_parent->next_table());
    _parent->set_next_table(this);

    if (next_table() != 0) {
	next_table()->replumb(_parent, this);
    }

    if (_output) {
	new_dump_state();
    }
}

template<class A>
RedistTable<A>::~RedistTable<A>()
{
    // Unplumb ourselves from the table graph
    _parent->set_next_table(next_table());

    // Delete output carefully, deleting it may cause an event or two.
    if (_output) {
	RedistOutputBase<A>* rob = _output;
	_output = 0;
	delete rob;
    }
    // Delete dump state
    delete _ds;

    // Delete dead dump state
    while (_ddl.empty() == false) {
	delete _ddl.front();
	_ddl.erase(_ddl.begin());
    }
}

template <typename A>
void
RedistTable<A>::new_dump_state()
{
    XLOG_ASSERT(_ds == 0);
    _ds			= new RedistDumpState<A>();
    _ds->last_net	= RedistDumpState<A>::NO_LAST_NET;
    schedule_dump_timer();
}


template <typename A>
void
RedistTable<A>::schedule_dump_state_gc()
{
    _gctimer = _e.new_oneoff_after_ms(0,
				      callback(this,
					       &RedistTable<A>::dump_state_gc)
				      );
}

template <typename A>
void
RedistTable<A>::retire_dump_state()
{
    _ddl.push_back(_ds);
    _ds = 0;
    schedule_dump_state_gc();
}

template <typename A>
void
RedistTable<A>::dump_state_gc()
{
    if (_ddl.empty())
	return;

    RedistDumpState<A>* r = _ddl.front();
    if (r->ds_nets.empty() == true) {
	delete _ddl.front();
	_ddl.erase(_ddl.begin());
    } else {
	r->ds_nets.erase(r->ds_nets.begin());
    }
    schedule_dump_state_gc();
}

template <typename A>
bool
RedistTable<A>::dumping() const
{
    return _ds != 0;
}

template <typename A>
void
RedistTable<A>::schedule_dump_timer()
{
    XLOG_ASSERT(_ds != 0);
    _ds->dtimer = _e.new_oneoff_after_ms(0,
			 callback(this, &RedistTable<A>::dump_one_route));
}

template <typename A>
void
RedistTable<A>::unschedule_dump_timer()
{
    XLOG_ASSERT(_ds != 0);
    _ds->dtimer.unschedule();
}

template <typename A>
bool
RedistTable<A>::dump_timer_scheduled() const
{
    return _ds->dtimer.scheduled();
}

template<class A>
int
RedistTable<A>::add_route(const IPRouteEntry<A>& 	route,
			  RouteTable<A>* 		caller)
{
    XLOG_ASSERT(caller == _parent);
    debug_msg("RT[%s]: Adding route %s\n",
	      tablename().c_str(), route.str().c_str());

    if (dumping()) {
	add_route_in_dump(route);
    } else {
	_output->add_route(route);
    }
    next_table()->add_route(route, this);

    return XORP_OK;
}

template<class A>
int
RedistTable<A>::delete_route(const IPRouteEntry<A>* route,
			     RouteTable<A>*	    caller)
{
    XLOG_ASSERT(caller == _parent);
    debug_msg("RT[%s]: Delete route %s\n",
	      tablename().c_str(), route->str().c_str());

    if (dumping()) {
	delete_route_in_dump(*route);
    } else {
	_output->delete_route(*route);
    }
    next_table()->delete_route(route, this);

    return XORP_OK;
}

template<class A>
const IPRouteEntry<A>*
RedistTable<A>::lookup_route(const IPNet<A>& net) const
{
    return _from_table->lookup_route(net);
}

template<class A>
const IPRouteEntry<A>*
RedistTable<A>::lookup_route(const A& addr) const
{
    return _from_table->lookup_route(addr);
}

template<class A>
void
RedistTable<A>::replumb(RouteTable<A>* old_parent,
			RouteTable<A>* new_parent)
{
    XLOG_ASSERT(_parent == old_parent);
    _parent = new_parent;
}

template<class A>
RouteRange<A>*
RedistTable<A>::lookup_route_range(const A& addr) const
{
    return _from_table->lookup_route_range(addr);
}

template <typename A>
void
RedistTable<A>::output_invalid_event(RedistOutputBase<A>* output)
{
    if (output == _output) {
	// Output failed, there's nothing left for us to do.
	delete this;
    }
}

template <typename A>
void
RedistTable<A>::output_low_water_event(RedistOutputBase<A>* output)
{
    XLOG_ASSERT(output == _output);

    // Low water mark traversed.  If appropriate, continue redist table dump
    if (dumping() && dump_timer_scheduled() == false) {
	debug_msg("Low-water crossed: resuming dump\n");
	schedule_dump_timer();
    }
}

template <typename A>
void
RedistTable<A>::output_high_water_event(RedistOutputBase<A>* output)
{
    XLOG_ASSERT(output == _output);

    // High water mark crossed.  Stop RedistTable from getting routes
    // from source and queueing them.  We still have to accept
    // upstream add and deletes, and have no way of throttling these.
    if (dumping() && dump_timer_scheduled()) {
	debug_msg("High-water crossed: ceasing dump\n");
	unschedule_dump_timer();
    }
}

template <typename A>
void
RedistTable<A>::dump_one_route()
{
    if (_ds->last_net == RedistDumpState<A>::NO_LAST_NET) {
	//
	// There's no last net so we're starting a new dump
	//
	_ds->last_n_routes	= _from_table->route_count();
	_ds->last_generation	= _from_table->generation();

	typename OriginTable<A>::RouteContainer::iterator i;
	i = _from_table->route_container().begin();
	if (i == _from_table->route_container().end()) {
	    finish_dump();
	    return;
	}

	// Flush any existing state
	_output->delete_all_routes();
	retire_dump_state();

	// Create new state and dump route
	new_dump_state();
	_output->add_route(*(i.payload()));
	_ds->last_net = i.key();
	_ds->ds_nets.insert(i.key());
    } else if (_ds->last_generation == _from_table->generation()) {
	//
	// This is a continuation of the existing dump, attempt to resume
	// where we left off or from the next appropriate point.
	//
	typename OriginTable<A>::RouteContainer::iterator i;
	i = _from_table->route_container().lower_bound(_ds->last_net);
	if (i == _from_table->route_container().end()) {
	    // No position to resume from. Stop.
	    finish_dump();
	    return;
	}

	// Skip past already announced entries.
	if (_ds->ds_nets.find(i.key()) != _ds->ds_nets.end()) {
	    ++i;
	}

	// Check if we've reached the end of the trie
	if (i == _from_table->route_container().end()) {
	    finish_dump();
	    return;
	}
	XLOG_ASSERT(_ds->ds_nets.find(i.key()) == _ds->ds_nets.end());

	_output->add_route(*(i.payload()));
	_ds->last_net = i.key();
	_ds->ds_nets.insert(i.key());
	_ds->last_n_routes = _from_table->route_count();

    } else {
	//
	// OriginTable we're dumping routes from has changed.
	// Reset and prepare to start again.
	//
	_output->delete_all_routes();
	retire_dump_state();
	new_dump_state();
    }

    schedule_dump_timer();
    return;
}

template <typename A>
void
RedistTable<A>::finish_dump()
{
    retire_dump_state();
}

template <typename A>
void
RedistTable<A>::add_route_in_dump(const IPRouteEntry<A>& route)
{

    XLOG_ASSERT(
		// Update is to the last route_container we used,
		(_from_table->route_count() == (_ds->last_n_routes + 1)) ||
		// or the route container has changed,
		(_from_table->generation() != _ds->last_generation) ||
		// or we're just starting out
		(_ds->last_net == RedistDumpState<A>::NO_LAST_NET)
		);

    if (_ds->last_net == RedistDumpState<A>::NO_LAST_NET) {
	// We're at the beginning of the dump.  We'll pick up the
	// contents of this add when we walk the upstream routes.
	XLOG_ASSERT(_ds->ds_nets.empty());
	return;
    }

    if (_from_table->generation() != _ds->last_generation) {
	// The route source has changed.  We'll reset the dump state
	// and pick this up when we walk the upstream routes.
	_ds->last_net = RedistDumpState<A>::NO_LAST_NET;
	return;
    }

    if (_ds->ds_nets.find(route.net()) != _ds->ds_nets.end()) {
	// Route has already been announced in dump, push this out in case
	// it's changed.
	_output->add_route(route);
    }
    _ds->last_n_routes = _from_table->route_count();
}

template <typename A>
void
RedistTable<A>::delete_route_in_dump(const IPRouteEntry<A>& route)
{
    if (_ds->last_net == RedistDumpState<A>::NO_LAST_NET) {
	// We're about to start dumping, there's nothing to do.
	XLOG_ASSERT(_ds->ds_nets.empty());
	return;
    }

    if (_ds->last_generation != _from_table->generation()) {
	// The source route container has changed, trigger restart of
	// dump process.  Rationale: expect a dump table has been
	// plumbed in.
	_ds->last_net = RedistDumpState<A>::NO_LAST_NET;
	return;
    }

    if (_from_table->route_count() == _ds->last_n_routes - 1) {
	// The delete comes directly from the route container.
	// If route for net has been dumped, propagate delete to output.

	typename RedistDumpState<A>::RouteSet::iterator i;
	i = _ds->ds_nets.find(route.net());
	if (i != _ds->ds_nets.end()) {
	    _output->delete_route(route);
	    _ds->ds_nets.erase(i);
	}
    }
    _ds->last_n_routes = _from_table->route_count();
}

template<class A>
string
RedistTable<A>::str() const
{
    string s;

    s = "-------\nRedistTable: " + tablename() + "\n";
    s += "_from_table = " + _from_table->tablename() + "\n";
    s += "_to_table = " + _to_table_name + "\n";
    if (next_table() == NULL)
	s += "no next table\n";
    else
	s += "next table = " + next_table()->tablename() + "\n";
    return s;
}



// ----------------------------------------------------------------------------
// RedistOutputBase methods

template <typename A>
void
RedistOutputBase<A>::announce_invalid()
{
    _feeder->output_invalid_event(this);
}

template <typename A>
void
RedistOutputBase<A>::announce_low_water()
{
    _feeder->output_low_water_event(this);
}

template <typename A>
void
RedistOutputBase<A>::announce_high_water()
{
    _feeder->output_high_water_event(this);
}



// ----------------------------------------------------------------------------
// Instantiations

template class RedistTable<IPv4>;
template class RedistTable<IPv6>;

template class RedistOutputBase<IPv4>;
template class RedistOutputBase<IPv6>;
