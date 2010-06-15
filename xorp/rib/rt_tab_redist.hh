// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2010 XORP, Inc and Others
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

// $XORP: xorp/rib/rt_tab_redist.hh,v 1.19 2008/10/02 21:58:13 bms Exp $

#ifndef __RIB_RT_TAB_REDIST_HH__
#define __RIB_RT_TAB_REDIST_HH__



#include "rt_tab_base.hh"

template <typename A>
class Redistributor;

template <typename A>
class RedistOutput;

template <typename A>
class RedistPolicy;

#ifndef XORP_USE_USTL
/**
 * Comparitor to allow nets to be stored in a sorted container.
 */
template <typename A>
struct RedistNetCmp {
    bool operator() (const IPNet<A>& l, const IPNet<A>& r) const;
};
#endif


/**
 * @short RouteTable used to redistribute routes.
 *
 * The RedistTable is used to redistribute routes from a routing table
 * back out to a routing protocol.  For example, when you want to
 * redistribute BGP routes into OSPF.
 *
 * For most operations the RedistTable is essentially a passthrough.
 * It keeps track of nets it hears add and deletes for so when route
 * @ref Redistributor objects are added to the RedistTable they can
 * announce the existing routes at startup.
 *
 * Design note: RedistTable uses a set of IPNet's to cache routes - this
 * should be route entry pointers with an appropriate comparitor (not pointer
 * value based).
 */
template<class A>
class RedistTable : public RouteTable<A> {
public:
#ifdef XORP_USE_USTL
    typedef set<IPNet<A> > RouteIndex;
#else
    typedef set<IPNet<A>,RedistNetCmp<A> > RouteIndex;
#endif

public:
    /**
     * Constructor.
     *
     * Plumbs RedistTable in RIB graph after from_table.
     *
     * @param from_table table to redistribute routes from.
     */
    RedistTable(const string&	tablename,
		RouteTable<A>*	from_table);

    /**
     * Destructor.
     *
     * Unplumbs table and deletes Redistributor object instances previously
     * added with add_redistributor and not previously removed with
     * remove_redistributor.
     */
    ~RedistTable();

    /**
     * Add a redistributor to announce existing routes and future updates
     * to.
     */
    void add_redistributor(Redistributor<A>* r);

    /**
     * Remove a redistributor.
     */
    void remove_redistributor(Redistributor<A>* r);

    /**
     * Find redistributor with given name attribute.
     */
    Redistributor<A>* redistributor(const string& name);

    //
    // Standard RouteTable methods
    //
    int add_route(const IPRouteEntry<A>& route, RouteTable<A>* caller);
    int delete_route(const IPRouteEntry<A>* route, RouteTable<A>* caller);
    const IPRouteEntry<A>* lookup_route(const IPNet<A>& net) const;
    const IPRouteEntry<A>* lookup_route(const A& addr) const;
    RouteRange<A>* lookup_route_range(const A& addr) const;
    TableType type() const { return REDIST_TABLE; }
    RouteTable<A>* parent() { return _parent; }
    void replumb(RouteTable<A>* old_parent, RouteTable<A>* new_parent);
    string str() const;

    /**
     * Get nets of live routes seen by RedistTable since it was
     * instantiated.
     */
    const RouteIndex& route_index() const { return _rt_index; }

protected:
    RouteTable<A>*	_parent;	// Immediately upstream table.  May
					// differ from _from_table if a
					// Deletion table or another redist
					// table has been plumbed in.
    RouteIndex		_rt_index;
    list<Redistributor<A>*> _outputs;
};



/**
 * Controller class that takes routes from RedistTable and passes them
 * on via RedistOutput.
 *
 * Instances of this class are constructed when one routing protocol
 * requests route distribution from another.  Instances walk the
 * routes available in the RedistTable route index, resolve them, and
 * announce them via the RedistOutput.  Future updates received
 * from the RedistTable are propagated via the Redistributor instances
 * associated with it.
 */
template <typename A>
class Redistributor :
    public NONCOPYABLE
{
public:
    class RedistEventInterface {
	// Methods only available to RedistTable.
	void did_add(const IPRouteEntry<A>& ipr);
	void will_delete(const IPRouteEntry<A>& ipr);
	void did_delete(const IPRouteEntry<A>& ipr);

	friend class RedistTable<A>;
	friend class Redistributor<A>;

    public:
	RedistEventInterface(Redistributor<A>* r) : _r(r) {}

    private:
	Redistributor<A>* _r;
    };

    class OutputEventInterface {
	// Methods only available to RedistOutput.  These are
	// events it can tell us about.
	void low_water();
	void high_water();
	void fatal_error();

	friend class RedistOutput<A>;
	friend class Redistributor<A>;

    public:
	OutputEventInterface(Redistributor<A>* r) : _r(r) {}

    private:
	Redistributor<A>* _r;
    };

public:
    Redistributor(EventLoop& e, const string& name);
    virtual ~Redistributor();

    const string& name() const;

    void set_redist_table(RedistTable<A>* rt);

    /**
     * Bind RedistOutput to Redistributor instance.  The output
     * should be dynamically allocated with new.  When a new
     * redistributor output is set, the existing output is removed via
     * delete.  The RedistOutput is deleted by the
     * Redistributor when the Redistributor is destructed.
     */
    void set_output(RedistOutput<A>* output);

    /**
     * Bind policy object to Redistributor instance.  The policy
     * should be dynamically allocated with new.  When a new policy is
     * set, the existing policy is removed via delete.  The policy is
     * deleted by the Redistributor when the Redistributor is
     * destructed.
     */
    void set_policy(RedistPolicy<A>* policy);

    /**
     * Determine if policy accepts updates to route.
     *
     * @return true if associated property accepts update to route or
     * if no policy is enforced, false otherwise.
     */
    bool policy_accepts(const IPRouteEntry<A>& ipr) const;

    /**
     * Method available to instances of RedistTable to announce events
     * to the Redistributor instance.
     */
    RedistEventInterface& redist_event()		{ return _rei; }

    /**
     * Method available to instances of RedistOutput to
     * announce transport events to the Redistributor instance.
     */
    OutputEventInterface& output_event()		{ return _oei; }

    /**
     * Indicate dump status.  When Redistributor is first connected it dumps
     * existing routes to it's RedistOutput.
     *
     * @return true if route dump is in process, false if route dump is
     * either not started or finished.
     */
    bool dumping() const				{ return _dumping; }

private:
    /**
     * Start initial route dump when a RedistTable is associated with instance
     * through set_redist_table().
     */
    void start_dump();
    void finish_dump();

    void schedule_dump_timer();
    void unschedule_dump_timer();
    void dump_a_route();

    const IPNet<A>& last_dumped_net() const	{ return _last_net; }
    RedistTable<A>* redist_table()		{ return _table; }
    RedistOutput<A>* output()			{ return _output; }

private:
    // These are nested classes and need to be friends to invoke methods in
    // enclosing class.
    friend class RedistEventInterface;
    friend class OutputEventInterface;

private:
    EventLoop&			_e;
    string			_name;
    RedistTable<A>*		_table;
    RedistOutput<A>*		_output;
    RedistPolicy<A>*		_policy;

    RedistEventInterface	_rei;
    OutputEventInterface	_oei;

    bool 			_dumping;	// Announcing existing routes
    bool			_blocked;	// Output above high water
    IPNet<A>			_last_net;	// Last net announced
    XorpTimer			_dtimer;

    static const IPNet<A> NO_LAST_NET;		// Indicator for last net inval
};



/**
 * Base class for propagaing output of route add and delete messages.
 */
template <typename A>
class RedistOutput :
    public NONCOPYABLE
{
public:
    RedistOutput(Redistributor<A>* r);
    virtual ~RedistOutput();

    virtual void add_route(const IPRouteEntry<A>& ipr)		= 0;
    virtual void delete_route(const IPRouteEntry<A>& ipr)	= 0;

    /**
     * Method called by Redistributor to indicate start of initial
     * route dump.  This occurs when an output is first attached to
     * the redistributor to announce the existing routes.
     */
    virtual void starting_route_dump()				= 0;

    /**
     * Method called by Redistributor to indicate end of initial
     * route dump.  This occurs when an output is first attached to
     * the redistributor to announce the existing routes.
     */
    virtual void finishing_route_dump()				= 0;

protected:
    void announce_low_water()		{ _r->output_event().low_water(); }
    void announce_high_water()		{ _r->output_event().high_water(); }
    void announce_fatal_error()		{ _r->output_event().fatal_error(); }

private:
    Redistributor<A>* _r;
};


// ----------------------------------------------------------------------------
// Inline RedistTable methods

#ifndef XORP_USE_USTL
template <typename A>
inline bool
RedistNetCmp<A>::operator() (const IPNet<A>& l, const IPNet<A>& r) const
{
    if (l.prefix_len() != r.prefix_len())
	return l.prefix_len() < r.prefix_len();
    return l.masked_addr() < r.masked_addr();
}
#endif

#endif // __RIB_RT_TAB_REDIST_HH__
