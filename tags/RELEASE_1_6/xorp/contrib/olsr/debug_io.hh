// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/contrib/olsr/debug_io.hh,v 1.4 2008/10/02 21:56:33 bms Exp $

#ifndef __OLSR_DEBUG_IO_HH__
#define __OLSR_DEBUG_IO_HH__

/**
 * @short The DebugIO class realizes the interface IO.
 * 
 * It is used for OLSR regression tests.
 */
class DebugIO : public IO {
  public:
    DebugIO(TestInfo& info, EventLoop& eventloop);

    virtual ~DebugIO();

    /**
     * @short Pretty print frames. Specific to DebugIo.
     */
    void pp(const string& which, int level,
	const string& interface, const string& vif,
	IPv4 dst, uint16_t dport, IPv4 src, uint16_t sport,
	uint8_t* data, uint32_t len);

    int startup();
    int shutdown();

    /**
     * @short Enable an IPv4 address and port
     * for OLSR datagram reception.
     */
    bool enable_address(const string& interface, const string& vif,
	const IPv4& address, const uint16_t& port,
	const IPv4& all_nodes_address);

    /**
     * @short Disable an IPv4 address and port for
     * OLSR datagram reception.
     */
    bool disable_address(const string& interface, const string& vif,
	const IPv4& address, const uint16_t& port);

    /**
     * Test whether this interface is enabled.
     *
     * @return true if it exists and is enabled, otherwise false.
     */
    bool is_interface_enabled(const string& interface) const;

    /**
     * Test whether this interface/vif is enabled.
     *
     * @return true if it exists and is enabled, otherwise false.
     */
    bool is_vif_enabled(const string & interface, const string & vif) const;

    /**
     * @short Return true if the given vif is broadcast capable.
     */
    bool is_vif_broadcast_capable(const string& interface, const string& vif);

    /**
     * @short Return true if the given vif is multicast capable.
     */
    bool is_vif_multicast_capable(const string& interface, const string& vif);

    /**
     * @short Return true if the given vif is a loopback vif.
     */
    bool is_vif_loopback(const string& interface, const string& vif);

    /**
     * Test whether this interface/vif/address is enabled.
     *
     * @return true if it exists and is enabled, otherwise false.
     */
    bool is_address_enabled(const string& interface, const string& vif,
	const IPv4& address) const;

    /**
     * Get all addresses associated with this interface/vif.
     *
     * @param interface the name of the interface
     * @param vif the name of the vif
     * @param addresses (out argument) list of associated addresses
     *
     * @return true if there are no errors.
     */
    bool get_addresses(const string& interface, const string& vif,
	list<IPv4>& addresses) const;

    /**
     * Get the broadcast address associated with this IPv4 address.
     *
     * @param interface the name of the interface
     * @param vif the name of the vif
     * @param address the IPv4 interface address
     * @param bcast_address (out argument) the IPv4 broadcast address
     *
     * @return true if there are no errors.
     */
    bool get_broadcast_address(const string& interface,
			       const string& vif,
			       const IPv4& address,
			       IPv4& bcast_address) const;

    /**
     * @return the interface id for this interface, as seen by libfeaclient.
     */
    bool get_interface_id(const string& interface, uint32_t& interface_id);

    /**
     * @return the mtu for this interface.
     */
    uint32_t get_mtu(const string& interface);

    /**
     * @return the number of packets which have transited this interface.
     */
    inline int packets() const { return _packets; }

    /**
     * Send a UDP datagram from src:sport to dst:dport,
     * preferably on the given link.
     */
    bool send(const string& interface, const string& vif,
	const IPv4 & src, const uint16_t & sport,
	const IPv4 & dst, const uint16_t & dport,
	uint8_t* data, const uint32_t & len);

    /**
     * Receive frames. Specific to DebugIo.
     */
    void receive(const string& interface, const string& vif,
	const IPv4 & dst, const uint16_t& dport,
	const IPv4 & src, const uint16_t& sport,
	uint8_t* data, const uint32_t & len);

    /**
     * Register where datagrams should be forwarded. Specific to DebugIo.
     */
    bool register_forward(const string& interface, const string& vif,
			  IO::ReceiveCallback cb);

    /**
     * Unregister an existing callback registered with register_forward().
     * Specific to DebugIo.
     */
    void unregister_forward(const string& interface, const string& vif);

    bool add_route(IPv4Net net, IPv4 nexthop, uint32_t nexthop_id,
        uint32_t metric, const PolicyTags& policytags);

    bool replace_route(IPv4Net net, IPv4 nexthop,
        uint32_t nexthop_id, uint32_t metric,
        const PolicyTags & policytags);

    bool delete_route(IPv4Net net);

    void routing_table_empty();

    uint32_t routing_table_size();

    /**
     * Verify that this route is in the routing table.
     */
    bool routing_table_verify(IPv4Net net, IPv4 nexthop, uint32_t metric);

    /**
     * Dump the routing table contents to the given ostream (not necessarily
     * what's in the TestInfo).
     */
    void routing_table_dump(ostream& o);

  private:
    TestInfo&		_info;
    EventLoop&		_eventloop;
    int			_packets;

    uint32_t			_next_interface_id;
    map<string, uint32_t>	_interface_ids;

    MessageDecoder	_md;

    /**
     *
     * Forwarding callback, one per emulated interface, so
     * that interfaces may be bound to separate EmulateSubnet instances.
     */
    map<pair<string, string>, IO::ReceiveCallback>  _forward_cbs;

    struct DebugRouteEntry {
        IPv4	    _nexthop;
        uint32_t    _metric;
	PolicyTags  _policytags;
    };

    map<IPv4Net, DebugRouteEntry>	_routing_table;
};

#endif // __OLSR_DEBUG_IO_HH__
