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

// $XORP: xorp/rib/rt_tab_redist.hh,v 1.7 2004/04/16 02:24:14 hodson Exp $

#ifndef __RIB_RT_TAB_REDIST_HH__
#define __RIB_RT_TAB_REDIST_HH__

#include "rt_tab_base.hh"

template <typename A>
class Redistributor;

template <typename A>
class RedistributorOutput;

/**
 * Comparitor to allow nets to be stored in a sorted container.
 */
template <typename A>
struct RedistNetCmp {
    inline bool operator() (const IPNet<A>& l, const IPNet<A>& r) const;
};


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
 */
template<class A>
class RedistTable : public RouteTable<A> {
public:
    typedef set<IPNet<A>,RedistNetCmp<A> > RouteIndex;

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
    inline const RouteIndex& route_index() const { return _rt_index; }

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
 * on via RedistributorOutput.
 *
 * Instances of this class are constructed when one routing protocol
 * requests route distribution from another.  Instances walk the
 * routes available in the RedistTable route index, resolve them, and
 * announce them via the RedistributorOutput.  Future updates received
 * from the RedistTable are propagated via the Redistributor instances
 * associated with it.
 */
template <typename A>
class Redistributor
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
	// Methods only available to RedistributorOutput.  These are
	// events it can tell us about.
	void low_water();
	void high_water();
	void fatal_error();

	friend class RedistributorOutput<A>;
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

    void set_output(RedistributorOutput<A>* output);

    /**
     * Method available to instances of RedistTable to announce events
     * to the Redistributor instance.
     */
    inline RedistEventInterface& redist_event()		{ return _rei; }

    /**
     * Method available to instances of RedistributorOutput to
     * announce transport events to the Redistributor instance.
     */
    inline OutputEventInterface& output_event()		{ return _oei; }

    /**
     * Indicate dump status.  When Redistributor is first connected it dumps
     * existing routes to it's RedistributorOutput.
     *
     * @return true if route dump is in process, false if route dump is
     * either not started or finished.
     */
    inline bool dumping() const				{ return _dumping; }

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

    inline const IPNet<A>& last_dumped_net() const	{ return _last_net; }
    inline RedistTable<A>* redist_table()		{ return _table; }
    inline RedistributorOutput<A>* output()		{ return _output; }

private:
    // The following are not implemented
    Redistributor(const Redistributor<A>&);
    Redistributor<A>& operator=(const Redistributor<A>&);

    // These are nested classes and need to be friends to invoke methods in
    // enclosing class.
    friend class RedistEventInterface;
    friend class OutputEventInterface;

private:
    EventLoop&			_e;
    string			_name;
    RedistTable<A>*		_table;
    RedistributorOutput<A>*	_output;

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
class RedistributorOutput
{
public:
    RedistributorOutput(Redistributor<A>* r);
    virtual ~RedistributorOutput();

    virtual void add_route(const IPRouteEntry<A>& ipr)		= 0;
    virtual void delete_route(const IPRouteEntry<A>& ipr)	= 0;

protected:
    inline void announce_low_water()	{ _r->output_event().low_water(); }
    inline void announce_high_water()	{ _r->output_event().high_water(); }
    inline void announce_fatal_error()	{ _r->output_event().fatal_error(); }

private:
    // The following are not implemented
    RedistributorOutput(const RedistributorOutput<A>&);
    RedistributorOutput<A>& operator=(const RedistributorOutput<A>&);

private:
    Redistributor<A>* _r;
};


// ----------------------------------------------------------------------------
// Inline RedistTable methods

template <typename A>
inline bool
RedistNetCmp<A>::operator() (const IPNet<A>& l, const IPNet<A>& r) const
{
    if (l.prefix_len() != r.prefix_len())
	return l.prefix_len() < r.prefix_len();
    return l.masked_addr() < r.masked_addr();
}

#endif // __RIB_RT_TAB_REDIST_HH__
