// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

// $XORP: xorp/fea/xrl_rawsock4.hh,v 1.13 2007/05/08 19:23:15 pavlin Exp $

#ifndef __FEA_XRL_RAWSOCK4_HH__
#define __FEA_XRL_RAWSOCK4_HH__

#include <map>
#include "libxorp/ref_ptr.hh"
#include "libxipc/xrl_router.hh"

#include "rawsock4.hh"

class IfTree;
class XrlRouter;
class XrlFilterRawSocket4;

/**
 * @short A class that manages raw sockets as used by the XORP Xrl Interface.
 *
 * The XrlRawSocket4Manager has two containers: a container for raw
 * sockets indexed by the protocol associated with the raw socket, and
 * a container for the filters associated with each xrl_target.  When
 * an Xrl Target registers for interest in a particular type of raw
 * packet a raw socket (FilterRawSocket4) is created if necessary,
 * then the relevent filter is created and associated with the
 * RawSocket.
 */
class XrlRawSocket4Manager {
public:
    /**
     * Constructor for XrlRawSocket4Manager instances.
     */
    XrlRawSocket4Manager(EventLoop& eventloop, const IfTree& iftree,
			 XrlRouter& xr);

    ~XrlRawSocket4Manager();

    /**
     * Send an IPv4 packet on a raw socket.
     *
     * @param if_name the interface to send the packet on. It is essential for
     * multicast. In the unicast case this field may be empty.
     * @param vif_name the vif to send the packet on. It is essential for
     * multicast. In the unicast case this field may be empty.
     * @param src_address the IP source address.
     * @param dst_address the IP destination address.
     * @param ip_protocol the IP protocol number. It must be between 1 and
     * 255.
     * @param ip_ttl the IP TTL (hop-limit). If it has a negative value, the
     * TTL will be set internally before transmission.
     * @param ip_tos the Type Of Service (Diffserv/ECN bits for IPv4). If it
     * has a negative value, the TOS will be set internally before
     * transmission.
     * @param ip_router_alert if true, then add the IP Router Alert option to
     * the IP packet.
     * @param ip_internet_control if true, then this is IP control traffic.
     * @param payload the payload, everything after the IP header and options.
     */
    XrlCmdError send(
	const string&	if_name,
	const string&	vif_name,
	const IPv4&	src_address,
	const IPv4&	dst_address,
	uint8_t		ip_protocol,
	int32_t		ip_ttl,
	int32_t		ip_tos,
	bool		ip_router_alert,
	bool		ip_internet_control,
	const vector<uint8_t>&	payload);

    /**
     * Register to receive IPv4 packets. The receiver is expected to support
     * raw_packet4_client/0.1 interface.
     *
     * @param xrl_target_name the receiver's XRL target name.
     * @param if_name the interface through which packets should be accepted.
     * @param vif_name the vif through which packets should be accepted.
     * @param ip_protocol the IP protocol number that the receiver is
     * interested in. It must be between 0 and 255. A protocol number of 0 is
     * used to specify all protocols.
     *
     *  @param enable_multicast_loopback if true then enable delivering of
     *  multicast datagrams back to this host (assuming the host is a member of
     *  the same multicast group.
     */
    XrlCmdError register_receiver(
	const string&	xrl_target_name,
	const string&	if_name,
	const string&	vif_name,
	uint8_t		ip_protocol,
	bool		enable_multicast_loopback);

    /**
     * Unregister to receive IPv4 packets.
     *
     * @param xrl_target_name the receiver's XRL target name.
     * @param if_name the interface through which packets should not be
     * accepted.
     * @param vif_name the vif through which packets should not be accepted.
     * @param ip_protocol the IP Protocol number that the receiver is not
     * interested in anymore. It must be between 0 and 255. A protocol number
     * of 0 is used to specify all protocols.
     */
    XrlCmdError unregister_receiver(
	const string&	xrl_target_name,
	const string&	if_name,
	const string&	vif_name,
	uint8_t		ip_protocol);

    /**
     * Join an IPv4 multicast group.
     *
     * @param xrl_target_name the receiver's XRL target name.
     * @param if_name the interface through which packets should be accepted.
     * @param vif_name the vif through which packets should be accepted.
     * @param ip_protocol the IP protocol number that the receiver is
     * interested in. It must be between 0 and 255. A protocol number of 0 is
     * used to specify all protocols.
     * @param group_address the multicast group address to join.
     */
    XrlCmdError join_multicast_group(
	const string&	xrl_target_name,
	const string&	if_name,
	const string&	vif_name,
	uint8_t		ip_protocol,
	const IPv4&	group_address);

    /**
     * Leave an IPv4 multicast group.
     *
     * @param xrl_target_name the receiver's XRL target name.
     * @param if_name the interface through which packets should not be
     * accepted.
     * @param vif_name the vif through which packets should not be accepted.
     * @param ip_protocol the IP protocol number that the receiver is not
     * interested in anymore. It must be between 0 and 255. A protocol number
     * of 0 is used to specify all protocols.
     * @param group_address the multicast group address to leave.
     */
    XrlCmdError leave_multicast_group(
	const string&	xrl_target_name,
	const string&	if_name,
	const string&	vif_name,
	uint8_t		ip_protocol,
	const IPv4&	group_address);

    XrlRouter&		router() { return _xrlrouter; }
    const IfTree&	iftree() const { return _iftree; }

    /**
     * Method to be called by Xrl sending filter invoker
     */
    void xrl_send_recv_cb(const XrlError& e, string xrl_target_name);

protected:
    EventLoop&		_eventloop;
    const IfTree&	_iftree;
    XrlRouter&		_xrlrouter;

    // Collection of IPv4 raw sockets keyed by protocol
    typedef map<uint8_t, FilterRawSocket4*> SocketTable4;
    SocketTable4	_sockets;

    // Collection of RawSocketFilters created by XrlRawSocketManager
    typedef multimap<string, XrlFilterRawSocket4*> FilterBag4;
    FilterBag4		_filters;

protected:
    void erase_filters(const FilterBag4::iterator& begin,
		       const FilterBag4::iterator& end);

};

#endif // __FEA_XRL_RAWSOCK4_HH__
