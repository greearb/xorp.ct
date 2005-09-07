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

// $XORP: xorp/ospf/xrl_io.hh,v 1.6 2005/08/31 23:38:47 atanu Exp $

#ifndef __OSPF_XRL_IO_HH__
#define __OSPF_XRL_IO_HH__

#include "io.hh"

class EventLoop;

/**
 * Concrete implementation of IO using XRLs.
 */
template <typename A>
class XrlIO : public IO<A> {
 public:
    XrlIO(EventLoop& eventloop, string ribname)
	: _eventloop(eventloop), _ribname(ribname)
    {}


    /**
     * Send Raw frames.
     */
    bool send(const string& interface, const string& vif, 
	      A dst, A src,
	      uint8_t* data, uint32_t len);

    /**
     * Register for receiving raw frames.
     */
    bool register_receive(typename IO<A>::ReceiveCallback cb);

    /**
     * Enable the interface/vif to receive frames.
     */
    bool enable_interface_vif(const string& interface, const string& vif);

    /**
     * Disable this interface/vif from receiving frames.
     */
    bool disable_interface_vif(const string& interface, const string& vif);

    /**
     * Is this interface/vif/address enabled?
     *
     * @return true if it is.
     */
    bool enabled(const string& interface, const string& vif, A address);

    /**
     * @return prefix length for this address.
     */
    uint32_t get_prefix_length(const string& interface, const string& vif,
			       A address);

    /**
     * @return the mtu for this interface.
     */
    uint32_t get_mtu(const string& interface);

    /**
     * On the interface/vif join this multicast group.
     */
    bool join_multicast_group(const string& interface, const string& vif,
			      A mcast);
    

    /**
     * On the interface/vif leave this multicast group.
     */
    bool leave_multicast_group(const string& interface, const string& vif,
			       A mcast);

    /**
     * Add route to RIB.
     *
     * @param net network
     * @param nexthop
     * @param metric to network
     * @param equal true if this in another route to the same destination.
     * @param discard true if this is a discard route.
     */
    bool add_route(IPNet<A> net,
		   A nexthop,
		   uint32_t metric,
		   bool equal,
		   bool discard);

    /**
     * Replace route in RIB.
     *
     * @param net network
     * @param nexthop
     * @param metric to network
     * @param equal true if this in another route to the same destination.
     * @param discard true if this is a discard route.
     */
    bool replace_route(IPNet<A> net,
		       A nexthop,
		       uint32_t metric,
		       bool equal,
		       bool discard);

    /**
     * Delete route from RIB.
     */
    bool delete_route(IPNet<A> net);

 private:
    EventLoop& _eventloop;
    string _ribname;

    typename IO<A>::ReceiveCallback _cb;
};
#endif // __OSPF_XRL_IO_HH__
