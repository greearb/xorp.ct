// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/bgp/bgp_varrw.hh,v 1.2 2004/09/17 23:55:22 pavlin Exp $

#ifndef __BGP_BGP_VARRW_HH__
#define __BGP_BGP_VARRW_HH__

#include "policy/backend/single_varrw.hh"
#include "policy/common/element_factory.hh"

#include "internal_message.hh"

/**
 * @short Allows reading an modifying a BGP route.
 *
 * If the route is modified, the user is responsible for retrieving the
 * filtered message and deleting it.
 */
template <class A>
class BGPVarRW : public SingleVarRW {
public:
    /**
     * This varrw allows for routes to remain untouched even though they are
     * filtered. This is useful in order to check if a route will be accepted
     * or rejected, without caring about its modifications.
     *
     * @param rtmsg the message to filter and possibly modify.
     * @param no_modify if true, the route will not be modified.
     */
     
    BGPVarRW(const InternalMessage<A>& rtmsg, bool no_modify);

    /**
     * Caller owns the message [responsible for delete].
     *
     * @return the modified message. Null if no changes were made.
     */
    InternalMessage<A>* filtered_message();
    
    // SingleVarRW interface
    void single_start();
    void single_write(const string& id, const Element& e);
    void single_end();

    /**
     * Users should always test value of modified. If it is true they must
     * delete/use filtererd_message.
     *
     * @return true if route was modified. False otherwise.
     */
    bool modified();

private:
    /**
     * Specialized template which reads the correct address family of nexthop
     * and network addresses.
     *
     * @param route route to read addressed from.
     */
    void read_route_nexthop(const SubnetRoute<A>& route);

    /**
     * Attempts to write the nexthop address.
     * @param id variable to write.
     * @param e value to write.
     * @return true if value was written. False otherwise.
     */
    bool write_nexthop(const string& id, const Element& e);

    const InternalMessage<A>&	_orig_rtmsg;

    ElementFactory _ef;

    InternalMessage<A>*	_filtered_rtmsg;

    PolicyTags _ptags;
    bool _wrote_ptags;

    PathAttributeList<A> _palist;

    bool _no_modify;

    bool _modified;

    // not impl
    BGPVarRW(const BGPVarRW&);
    BGPVarRW& operator=(const BGPVarRW&);
};


#endif // __BGP_BGP_VARRW_HH__
