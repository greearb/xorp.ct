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

// $XORP: xorp/rib/rt_tab_redist.hh,v 1.6 2004/04/16 02:21:58 hodson Exp $

#ifndef __RIB_RT_TAB_REDIST_HH__
#define __RIB_RT_TAB_REDIST_HH__

#include "rt_tab_base.hh"

class EventLoop;

template <typename A>
class RedistOutputFeeder;

template <typename A>
struct RedistDumpState;

template <typename A>
class OriginTable;

/**
 * @short class used by @ref RedistTable for outputting route redistribution
 * data.
 *
 * This class performs route redistribution for the RedistTable.  It
 * buffers add_route and delete_route requests and implementations
 * pass these on the appropriate place in a synchronous (no-delay) or
 * asynchronous (delay) manner.  The RedistTable is derived from
 * RedistOutputFeeder.  The RedistOutputFeeder is an abstract class
 * that defines events that the RedistOutputBase implementations
 * signal.
 *
 * The number of buffered add and delete operations at any given time
 * can be determined with the @ref backlog() method.  In addition,
 * implementations have a low and high water mark for their backlogs
 * and events occur when these levels are crossed, which should be
 * used as hints to resume or stop adding buffered add and delete
 * operations.
 */
template <typename A>
class RedistOutputBase {
public:
    RedistOutputBase(RedistOutputFeeder<A>* feeder);
    virtual ~RedistOutputBase() { announce_invalid(); }

    virtual void add_route(const IPRouteEntry<A>& route)	= 0;
    virtual void delete_route(const IPRouteEntry<A>& route)	= 0;
    virtual void delete_all_routes()				= 0;

    virtual uint32_t backlog() const				= 0;
    virtual uint32_t low_water_backlog() const			= 0;
    virtual uint32_t high_water_backlog() const			= 0;

protected:
    /**
     * Announce to feeder that instance is no longer valid.
     * Invokes @ref RedistOutputFeeder::output_invalid_event().
     */
    void announce_invalid();

    /**
     * Announce backlog has fallen below low water mark.
     * Invokes @ref RedistOutputFeeder::output_low_water_event().
     */
    void announce_low_water();

    /**
     * Announce backlog has risen above high water mark.
     * Invokes @ref RedistOutputFeeder::output_high_water_event().
     */
    void announce_high_water();

private:
    RedistOutputFeeder<A>* _feeder;
};

/**
 * @short Base class for recipient of RedistOutputBase events.
 *
 * This class defines events that may be called from descendants of
 * RedistOutputBase.
 */
template <typename A>
class RedistOutputFeeder {
public:
    virtual ~RedistOutputFeeder() {};
    /**
     * Called when output is no longer valid.  Note: output will never
     * become valid again.
     */
    virtual void output_invalid_event(RedistOutputBase<A>* output)	= 0;

    /**
     * Called when output's buffer falls below low water mark level.
     */
    virtual void output_low_water_event(RedistOutputBase<A>* output)	= 0;

    /**
     * Called when output's buffer rises above high water mark level.
     */
    virtual void output_high_water_event(RedistOutputBase<A>* output)	= 0;
};


/**
 * @short RouteTable used to redistribute routes (not yet working)
 *
 * RedistTable is used to redistribute routes from a routing table
 * back out to a routing protocol.  For example, when you want to
 * redistribute BGP routes into OSPF.
 *
 * A RedistTable acts as a pass-thorough and until it's output target
 * has been set with @ref set_redist_target.  Once it has a target, it
 * must first take routes from the upstream table and advertise those
 * to it's output.  Should any add's or delete's occur while this is
 * happening, they need to be examined to see if they overlap with
 * what's already been output.  If so, then the update needs to be
 * forwarded to the output immediately.  If not, then the update will
 * become apparent when the output iterator reaches the affected
 * region in the upstream route source's trie.
 *
 * Theres some complexity to this code, because the routing table
 * we're redisting from can be shutdown.  If this happens a
 * DeletionTable is plumbed in between the upstream table and the
 * redist instance.  The DeletionTable takes possession of the
 * protocols trie and tears out the routes.  If the redist table
 * instance is still dumping routes, this needs to be accounted for.
 *
 * RedistTable is a work-in-progress.
 */
template<class A>
class RedistTable : public RouteTable<A>, public RedistOutputFeeder<A> {
public:
    /**
     * Constructor
     *
     * Plumbs RedistTable in RIB graph after from_table.
     *
     * @param from_table table to redistribute routes from.
     *
     * @param to_table table to redistribute routes to.
     *
     * @param eventloop the EventLoop to be used for timers and file
     *		 	descriptor events.
     */
    RedistTable(const string&	tablename,
		OriginTable<A>*	from_table,
		OriginTable<A>*	to_table,
		EventLoop&	eventloop);

    /**
     * Constructor
     *
     * Plumbs RedistTable in RIB graph after from_table.
     *
     * @param from_table table to redistribute routes from.
     *
     * @param to_tablename name of table to redistribute routes to.
     *
     * @param eventloop the EventLoop to be used for timers and file
     *		 	descriptor events.
     */
    RedistTable(const string&	tablename,
		OriginTable<A>*	from_table,
		const string&	to_tablename,
		EventLoop&	eventloop);

    /**
     * Set output to receive redistributed routes.  The output is
     * deleted automatically when the RedistTable Instance is
     * destructed.
     */
    void set_output(RedistOutputBase<A>* output);

    ~RedistTable();

    int add_route(const IPRouteEntry<A>& route, RouteTable<A>* caller);
    int delete_route(const IPRouteEntry<A>* route, RouteTable<A>* caller);
    const IPRouteEntry<A>* lookup_route(const IPNet<A>& net) const;
    const IPRouteEntry<A>* lookup_route(const A& addr) const;
    RouteRange<A>* lookup_route_range(const A& addr) const;
    TableType type() const { return REDIST_TABLE; }
    RouteTable<A>* parent() { return _from_table; }
    void replumb(RouteTable<A>* old_parent, RouteTable<A>* new_parent);
    string str() const;

protected:
    void output_invalid_event(RedistOutputBase<A>* output);
    void output_low_water_event(RedistOutputBase<A>* output);
    void output_high_water_event(RedistOutputBase<A>* output);

protected:
    inline bool dumping() const;
    inline bool dump_timer_scheduled() const;

    void schedule_dump_timer();
    void unschedule_dump_timer();
    void dump_one_route();

    void new_dump_state();
    void finish_dump();
    void retire_dump_state();

    void add_route_in_dump(const IPRouteEntry<A>&	route);
    void delete_route_in_dump(const IPRouteEntry<A>&	route);

    inline void schedule_dump_state_gc();
    void dump_state_gc();

private:
    EventLoop&		_e;

    OriginTable<A>*	_from_table;	// Table that routes are redisted from

    RouteTable<A>*	_parent;	// Immediately upstream table.  May
					// differ from _from_table if a
					// Deletion table or another redist
					// table has been plumbed in.

    string		_to_tablename;	// Protocol routes are being dumped to

    RedistOutputBase<A>* _output;
    RedistDumpState<A>*	 _ds;
    list<RedistDumpState<A>*> _ddl;	// Dead dump state to be garbage coll.
    XorpTimer		_gctimer;	// Timer for garbage collecting _dds
};

#endif // __RIB_RT_TAB_REDIST_HH__
