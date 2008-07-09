// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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

// $XORP$

#ifndef __OLSR_IO_HH__
#define __OLSR_IO_HH__

/**
 * @short Abstract interface to low level IO operations.
 *
 * An abstract class that defines packet reception and
 * transmission. The details of how packets are received or transmitted
 * are therefore hidden from the internals of the OLSR code.
 */
class IO : public ServiceBase {
  public:
    IO() {}
    virtual ~IO() {}

    /**
     * Enable an IPv4 address and port for OLSR datagram reception and
     * transmission.
     *
     * @param interface the interface to enable.
     * @param vif the vif to enable.
     * @param address the address to enable.
     * @param port the port to enable.
     * @param all_nodes_address the all-nodes address to enable.
     * @return true if the address was enabled, otherwise false.
     */
    virtual bool enable_address(const string& interface, const string& vif,
	const IPv4& address, const uint16_t& port,
	const IPv4& all_nodes_address) = 0;

    /**
     * Disable an IPv4 address and port for OLSR datagram reception.
     *
     * @param interface the interface to disable.
     * @param vif the vif to disable.
     * @param address the address to disable.
     * @param port the port to disable.
     * @return true if the address was disabled, otherwise false.
     */
    virtual bool disable_address(const string& interface, const string& vif,
	const IPv4& address, const uint16_t& port) = 0;

    /**
     * Test whether this interface is enabled.
     *
     * @param interface the interface to test.
     * @return true if it exists and is enabled, otherwise false.
     */
    virtual bool is_interface_enabled(const string& interface) const = 0;

    /**
     * Test whether this interface/vif is enabled.
     *
     * @param interface the interface to test.
     * @param vif the vif to test.
     * @return true if it exists and is enabled, otherwise false.
     */
    virtual bool is_vif_enabled(const string& interface,
	const string& vif) const = 0;

    /**
     * Test whether this interface/vif is broadcast capable.
     *
     * @param interface the interface to test.
     * @param vif the vif to test.
     * @return true if it is broadcast capable, otherwise false.
     */
    virtual bool is_vif_broadcast_capable(const string& interface,
	const string& vif) = 0;

    /**
     * Test whether this interface/vif is multicast capable.
     *
     * @param interface the interface to test.
     * @param vif the vif to test.
     * @return true if it is multicast capable, otherwise false.
     */
    virtual bool is_vif_multicast_capable(const string& interface,
	const string& vif) = 0;

    /**
     * Test whether this interface/vif is a loopback interface.
     *
     * @param interface the interface to test.
     * @param vif the vif to test.
     * @return true if it is a loopback interface, otherwise false.
     */
    virtual bool is_vif_loopback(const string& interface,
	const string& vif) = 0;

    /**
     * Test whether this interface/vif/address is enabled.
     *
     * @param interface the interface to test.
     * @param vif the vif to test.
     * @param address the address to test.
     * @return true if it exists and is enabled, otherwise false.
     */
    virtual bool is_address_enabled(const string& interface,
	const string& vif, const IPv4& address) const = 0;

    /**
     * Callback for interface status from the FEA.
     */
    typedef  XorpCallback2<void, const string&,
				   bool>::RefPtr InterfaceStatusCb;

    /**
     * Callback for vif status from the FEA.
     */
    typedef  XorpCallback3<void, const string&, const string&,
				   bool>::RefPtr VifStatusCb;

    /**
     * Callback for address status from the FEA.
     */
    typedef  XorpCallback4<void, const string&, const string&, IPv4,
				   bool>::RefPtr AddressStatusCb;

    /**
     * Callback for packet reception from the FEA.
     */
    typedef XorpCallback8<void, const string&, const string&,
				   IPv4, uint16_t, IPv4, uint16_t,
				   uint8_t*,
				   uint32_t>::RefPtr ReceiveCallback;

    /**
     * Get all addresses associated with this interface/vif.
     *
     * @param interface the name of the interface
     * @param vif the name of the vif
     * @param addresses (out argument) list of associated addresses
     * @return true if there are no errors.
     */
    virtual bool get_addresses(const string& interface, const string& vif,
	list<IPv4>& addresses) const = 0;

    /**
     * Get the broadcast address associated with this interface/vif/address.
     *
     * @param interface the name of the interface
     * @param vif the name of the vif
     * @param address IPv4 binding address
     * @param bcast_address (out argument) primary broadcast address
     * @return true if there are no errors.
     */
    virtual bool get_broadcast_address(const string& interface,
				       const string& vif,
				       const IPv4& address,
				       IPv4& bcast_address) const = 0;

    /**
     * Get the ID of the interface, as seen by libfeaclient.
     *
     * @param interface the name of the interface.
     * @param interface_id (out argument) interface ID.
     * @return the interface id for this interface.
     */
    virtual bool get_interface_id(const string& interface,
	uint32_t& interface_id) = 0;

    /**
     * Get the MTU for an interface.
     *
     * @param interface the name of the interface.
     * @return the mtu for this interface.
     */
    virtual uint32_t get_mtu(const string& interface) = 0;

    /**
     * Add a callback for tracking the interface status.
     *
     * The callback will be invoked whenever the status of the interface
     * is changed from disabled to enabled or vice-versa.
     *
     * @param cb the callback to register.
     */
    inline void register_interface_status(InterfaceStatusCb cb) {
	_interface_status_cb = cb;
    };

    /**
     * Add a callback for tracking the interface/vif status.
     *
     * The callback will be invoked whenever the status of the interface/vif
     * is changed from disabled to enabled or vice-versa.
     *
     * @param cb the callback to register.
     */
    inline void register_vif_status(VifStatusCb cb) {
	_vif_status_cb = cb;
    };

    /**
     * Add a callback for tracking the interface/vif/address status.
     *
     * The callback will be invoked whenever the status of the tuple
     * (interface, vif, address) is changed from disabled to enabled
     * or vice-versa.
     *
     * @param cb the callback to register.
     */
    inline void register_address_status(AddressStatusCb cb) {
	_address_status_cb = cb;
    };

    /**
     * Register for receiving datagrams.
     *
     * @param cb the callback to register.
     */
    inline void register_receive(ReceiveCallback cb) { _receive_cb = cb; };

  protected:
    ReceiveCallback	_receive_cb;
    InterfaceStatusCb	_interface_status_cb;
    VifStatusCb		_vif_status_cb;
    AddressStatusCb	_address_status_cb;

  public:
    struct interface_vif {
        string _interface_name;
        string _vif_name;
    };

    /**
     * Send a UDP datagram from src:sport to dst:dport, on
     * the given interface, if possible.
     *
     * @param interface the interface to transmit from.
     * @param vif the vif to transmit from.
     * @param src the IPv4 source address to transmit from.
     * @param sport the UDP source port to transmit from.
     * @param dst the IPv4 destination address to send to.
     * @param dport the UDP destination port to send to.
     * @param data the datagram to transmit.
     * @param len the length of the datagram to transmit.
     * @return true if the datagram was sent OK, otherwise false.
     */
    virtual bool send(const string& interface, const string& vif,
	const IPv4& src, const uint16_t& sport,
	const IPv4& dst, const uint16_t& dport,
	uint8_t* data, const uint32_t& len) = 0;

    /**
     * Add route.
     *
     * @param net network
     * @param nexthop
     * @param faceid interface ID towards the nexthop
     * @param metric to network
     * @param policytags policy info to the RIB.
     * @return true if the route was added OK, otherwise false.
     */
    virtual bool add_route(IPv4Net net, IPv4 nexthop, uint32_t faceid,
			   uint32_t metric,
			   const PolicyTags& policytags) = 0;

    /**
     * Replace route.
     *
     * @param net network
     * @param nexthop
     * @param faceid interface ID towards the nexthop
     * @param metric to network
     * @param policytags policy info to the RIB.
     * @return true if the route was replaced OK, otherwise false.
     */
    virtual bool replace_route(IPv4Net net, IPv4 nexthop, uint32_t faceid,
			       uint32_t metric,
			       const PolicyTags& policytags) = 0;

    /**
     * Delete route.
     *
     * @param net network
     * @return true if the route was deleted OK, otherwise false.
     */
    virtual bool delete_route(IPv4Net net) = 0;

    /**
     * Store a mapping of the OLSR internal interface ID to
     * interface/vif. This will be required by when installing a route.
     *
     * @param interface_id the ID of the interface, as seen by libfeaclient.
     * @param interface the name of the interface mapped to interface_id.
     * @param vif the name of vif mapped to interface_id.
     */
    inline void set_interface_mapping(uint32_t interface_id,
	const string& interface, const string& vif) {
	interface_vif iv;

	iv._interface_name = interface;
	iv._vif_name = vif;

	_interface_vif[interface_id] = iv;
    };

    /**
     * Given an OLSR interface ID, return the interface/vif.
     *
     * @param interface_id the ID of the interface, as seen by libfeaclient.
     * @param interface the name of the interface mapped to interface_id.
     * @param vif the name of vif mapped to interface_id.
     * @return true if interface_id was found, otherwise false.
     */
    inline bool get_interface_vif_by_interface_id(uint32_t interface_id,
	string& interface, string& vif) {
	if (0 == _interface_vif.count(interface_id))
	    return false;

	interface_vif iv = _interface_vif[interface_id];
	interface = iv._interface_name;
	vif = iv._vif_name;
	return true;
    };

  protected:
    map<uint32_t, interface_vif> _interface_vif;
};

#endif // __OLSR_IO_HH__
