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

// $XORP: xorp/fea/xrl_rawsock6.hh,v 1.9 2007/05/19 01:52:42 pavlin Exp $

#ifndef __FEA_XRL_RAWSOCK6_HH__
#define __FEA_XRL_RAWSOCK6_HH__

#include <map>
#include "libxorp/ref_ptr.hh"
#include "libxipc/xrl_router.hh"

#include "rawsock6.hh"

class IfTree;
class XrlRouter;
class XrlFilterRawSocket6;

/**
 * @short A class that manages raw sockets as used by the XORP Xrl Interface.
 *
 * The XrlRawSocket6Manager has two containers: a container for raw
 * sockets indexed by the protocol associated with the raw socket, and
 * a container for the filters associated with each xrl_target.  When
 * an Xrl Target registers for interest in a particular type of raw
 * packet a raw socket (FilterRawSocket6) is created if necessary,
 * then the relevent filter is created and associated with the
 * RawSocket.
 */
class XrlRawSocket6Manager {
public:
    /**
     * Constructor for XrlRawSocket6Manager instances.
     */
    XrlRawSocket6Manager(EventLoop& eventloop, const IfTree& iftree,
			 XrlRouter& xr);

    ~XrlRawSocket6Manager();

    /**
     * Send an IPv6 packet on a raw socket.
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
     * @param ip_tos the Type Of Service (IP traffic class for IPv6). If it
     * has a negative value, the TOS will be set internally before
     * transmission.
     * @param ip_router_alert if true, then add the IP Router Alert option to
     * the IP packet.
     * @param ip_internet_control if true, then this is IP control traffic.
     * @param ext_headers_type a vector of integers with the types of the
     * optional extention headers.
     * @param ext_headers_payload a vector of payload data, one for each
     * optional extention header. The number of entries must match
     * ext_headers_type.
     * @param payload the payload, everything after the IP header and options.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int send(const string&	if_name,
	     const string&	vif_name,
	     const IPv6&	src_address,
	     const IPv6&	dst_address,
	     uint8_t		ip_protocol,
	     int32_t		ip_ttl,
	     int32_t		ip_tos,
	     bool		ip_router_alert,
	     bool		ip_internet_control,
	     const vector<uint8_t>& ext_headers_type,
	     const vector<vector<uint8_t> >& ext_headers_payload,
	     const vector<uint8_t>&	payload,
	     string&		error_msg);

    /**
     * Register to receive IPv6 packets. The receiver is expected to support
     * raw_packet6_client/0.1 interface.
     *
     * @param xrl_target_name the receiver's XRL target name.
     * @param if_name the interface through which packets should be accepted.
     * @param vif_name the vif through which packets should be accepted.
     * @param ip_protocol the IP protocol number that the receiver is
     * interested in. It must be between 0 and 255. A protocol number of 0 is
     * used to specify all protocols.
     * @param enable_multicast_loopback if true then enable delivering of
     * multicast datagrams back to this host (assuming the host is a member of
     * the same multicast group.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_receiver(const string&	xrl_target_name,
			  const string&	if_name,
			  const string&	vif_name,
			  uint8_t	ip_protocol,
			  bool		enable_multicast_loopback,
			  string&	error_msg);
    
    /**
     * Unregister to receive IPv6 packets.
     *
     * @param xrl_target_name the receiver's XRL target name.
     * @param if_name the interface through which packets should not be
     * accepted.
     * @param vif_name the vif through which packets should not be accepted.
     * @param ip_protocol the IP Protocol number that the receiver is not
     * interested in anymore. It must be between 0 and 255. A protocol number
     * of 0 is used to specify all protocols.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_receiver(const string&	xrl_target_name,
			    const string&	if_name,
			    const string&	vif_name,
			    uint8_t		ip_protocol,
			    string&		error_msg);

    /**
     * Join an IPv6 multicast group.
     *
     * @param xrl_target_name the receiver's XRL target name.
     * @param if_name the interface through which packets should be accepted.
     * @param vif_name the vif through which packets should be accepted.
     * @param ip_protocol the IP protocol number that the receiver is
     * interested in. It must be between 0 and 255. A protocol number of 0 is
     * used to specify all protocols.
     * @param group_address the multicast group address to join.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int join_multicast_group(const string&	xrl_target_name,
			     const string&	if_name,
			     const string&	vif_name,
			     uint8_t		ip_protocol,
			     const IPv6&	group_address,
			     string&		error_msg);

    /**
     * Leave an IPv6 multicast group.
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
    int leave_multicast_group(const string&	xrl_target_name,
			      const string&	if_name,
			      const string&	vif_name,
			      uint8_t		ip_protocol,
			      const IPv6&	group_address,
			      string&		error_msg);

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

    // Collection of IPv6 raw sockets keyed by protocol.
    typedef map<uint8_t, FilterRawSocket6*> SocketTable6;
    SocketTable6	_sockets;

    // Collection of RawSocketFilters created by XrlRawSocketManager
    typedef multimap<string, XrlFilterRawSocket6*> FilterBag6;
    FilterBag6		_filters;

protected:
    void erase_filters(const FilterBag6::iterator& begin,
		       const FilterBag6::iterator& end);

};

#endif // __FEA_XRL_RAWSOCK6_HH__
