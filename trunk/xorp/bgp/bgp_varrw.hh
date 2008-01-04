// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/bgp/bgp_varrw.hh,v 1.20 2007/02/16 22:45:11 pavlin Exp $

#ifndef __BGP_BGP_VARRW_HH__
#define __BGP_BGP_VARRW_HH__

#include "policy/backend/single_varrw.hh"
#include "policy/common/element_factory.hh"
#include "internal_message.hh"

template <class A>
class BGPVarRWCallbacks;

/**
 * @short Allows reading an modifying a BGP route.
 *
 * If the route is modified, the user is responsible for retrieving the
 * filtered message and deleting it.
 */
template <class A>
class BGPVarRW : public SingleVarRW {
public:
    enum {
	VAR_NETWORK4 = VAR_PROTOCOL,
        VAR_NEXTHOP4,
        VAR_NETWORK6,
        VAR_NEXTHOP6,
        VAR_ASPATH,
        VAR_ORIGIN,
        VAR_NEIGHBOR,
        VAR_LOCALPREF,
        VAR_COMMUNITY,
        VAR_MED,
        VAR_MED_REMOVE,
        VAR_AGGREGATE_PREFIX_LEN,
        VAR_AGGREGATE_BRIEF_MODE,
        VAR_WAS_AGGREGATED,

	VAR_BGPMAX // must be last
    };

    typedef Element* (BGPVarRW::*ReadCallback)();
    typedef void (BGPVarRW::*WriteCallback)(const Element& e);

    /**
     * This varrw allows for routes to remain untouched even though they are
     * filtered. This is useful in order to check if a route will be accepted
     * or rejected, without caring about its modifications.
     *
     * @param name the name of the filter to print in case of tracing.
     */
    BGPVarRW(const string& name);
    virtual ~BGPVarRW();

    /**
     * Attach a route to the varrw.
     *
     * @param rtmsg the message to filter and possibly modify.
     * @param no_modify if true, the route will not be modified.
     */
    void attach_route(const InternalMessage<A>& rtmsg, bool no_modify);

    /**
     * Caller owns the message [responsible for delete].
     * Calling multiple times will always return the same message, not a copy.
     *
     * @return the modified message. Null if no changes were made.
     */
    InternalMessage<A>* filtered_message();
    
    // SingleVarRW interface
    Element* single_read(const Id& id);

    void single_write(const Id& id, const Element& e);
    void end_write();

    /**
     * If a route is modified, the caller may obtain it via the filtered_message
     * call.
     *
     * @return true if route was modified. False otherwise.
     */
    bool modified();

    /**
     * Output basic BGP specific information.
     *
     * @return BGP trace based on verbosity level returned from trace().
     */
    virtual string more_tracelog();

    /**
     * Reads the neighbor variable.  This is different on input/output branch.
     *
     * @return the neighbor variable.
     */
    virtual Element* read_neighbor();

    /**
     * Callback wrapper used to call the virtual @ref read_neighbor() method.
     *
     * @return the neighbor variable.
     */
    Element* read_neighbor_base_cb()	{ return read_neighbor(); }

    Element* read_policytags();
    Element* read_filter_im();
    Element* read_filter_sm();
    Element* read_filter_ex();

    Element* read_network4();
    Element* read_network6();

    Element* read_nexthop4();
    Element* read_nexthop6();
    Element* read_aspath();
    Element* read_origin();

    Element* read_localpref();
    Element* read_community();
    Element* read_med();
    Element* read_med_remove();

    Element* read_aggregate_prefix_len();
    Element* read_aggregate_brief_mode();
    Element* read_was_aggregated();

    void write_filter_im(const Element& e);
    void write_filter_sm(const Element& e);
    void write_filter_ex(const Element& e);
    void write_policytags(const Element& e);

    void write_nexthop4(const Element& e);
    void write_nexthop6(const Element& e);
    void write_aspath(const Element& e);
    void write_origin(const Element& e);

    void write_aggregate_prefix_len(const Element& e);
    void write_aggregate_brief_mode(const Element& e);
    void write_was_aggregated(const Element& e);

    void write_localpref(const Element& e);
    void write_community(const Element& e);
    void write_med(const Element& e);
    void write_med_remove(const Element& e);

protected:
    ElementFactory		_ef;
    string			_name;

private:
    void clone_palist();
    void cleanup();

    const InternalMessage<A>*	_orig_rtmsg;
    InternalMessage<A>*		_filtered_rtmsg;
    bool			_got_fmsg;
    PolicyTags*			_ptags;
    bool			_wrote_ptags;
    PathAttributeList<A>*	_palist;
    bool			_no_modify;
    bool			_modified;
    RefPf			_pfilter[3];
    bool			_wrote_pfilter[3];
    bool			_route_modify;

    // Aggregation -> we cannot write those directly into the subnet
    // route so must provide local volatile copies to be operated on
    uint32_t			_aggr_prefix_len;
    bool			_aggr_brief_mode;

    // not impl
    BGPVarRW(const BGPVarRW&);
    BGPVarRW& operator=(const BGPVarRW&);

    static BGPVarRWCallbacks<A> _callbacks;
};

template <class A>
class BGPVarRWCallbacks {
public:
    // XXX don't know how to refer to BGPVarRW<A>::ReadCallback in gcc 2.95
    typedef Element* (BGPVarRW<A>::*RCB)();
    typedef void (BGPVarRW<A>::*WCB)(const Element&);

    void init_rw(const VarRW::Id&, RCB, WCB);

    BGPVarRWCallbacks();

    RCB _read_map[BGPVarRW<A>::VAR_BGPMAX];
    WCB _write_map[BGPVarRW<A>::VAR_BGPMAX];
};

#endif // __BGP_BGP_VARRW_HH__
