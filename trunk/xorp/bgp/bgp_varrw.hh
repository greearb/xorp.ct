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

// $XORP: xorp/bgp/bgp_varrw.hh,v 1.11 2005/09/04 18:35:49 abittau Exp $

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
    typedef Element* (BGPVarRW::*ReadCallback)();
    typedef void (BGPVarRW::*WriteCallback)(const Element& e);

    /**
     * This varrw allows for routes to remain untouched even though they are
     * filtered. This is useful in order to check if a route will be accepted
     * or rejected, without caring about its modifications.
     *
     * @param rtmsg the message to filter and possibly modify.
     * @param no_modify if true, the route will not be modified.
     * @param name the name of the filter to print in case of tracing.
     */
    BGPVarRW(const InternalMessage<A>& rtmsg, bool no_modify, const string& name);
    virtual ~BGPVarRW();

    /**
     * Caller owns the message [responsible for delete].
     * Calling multiple times will always return the same message, not a copy.
     *
     * @return the modified message. Null if no changes were made.
     */
    InternalMessage<A>* filtered_message();
    
    // SingleVarRW interface
    Element* single_read(const string& id);

    void single_write(const string& id, const Element& e);
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

    void write_filter_im(const Element& e);
    void write_filter_sm(const Element& e);
    void write_filter_ex(const Element& e);
    void write_policytags(const Element& e);

    void write_nexthop4(const Element& e);
    void write_nexthop6(const Element& e);
    void write_aspath(const Element& e);
    void write_origin(const Element& e);

    void write_localpref(const Element& e);
    void write_community(const Element& e);
    void write_med(const Element& e);

protected:
    ElementFactory		_ef;
    string			_name;

private:
    void clone_palist();

    const InternalMessage<A>&	_orig_rtmsg;
    InternalMessage<A>*		_filtered_rtmsg;
    bool			_got_fmsg;
    PolicyTags			_ptags;
    bool			_wrote_ptags;
    PathAttributeList<A>*	_palist;
    bool			_no_modify;
    bool			_modified;
    RefPf			_pfilter[3];
    bool			_wrote_pfilter[3];
    bool			_route_modify;

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
	    
    typedef map<string, RCB>  ReadMap;
    typedef map<string, WCB> WriteMap;

    BGPVarRWCallbacks();

    ReadMap  _read_map;
    WriteMap _write_map;
};

#endif // __BGP_BGP_VARRW_HH__
