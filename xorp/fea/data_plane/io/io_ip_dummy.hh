// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2011 XORP, Inc and Others
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


#ifndef __FEA_DATA_PLANE_IO_IO_IP_DUMMY_HH__
#define __FEA_DATA_PLANE_IO_IO_IP_DUMMY_HH__

//
// I/O IP raw communication support.
//
// The mechanism is Dummy (for testing purpose).
//



#include "libxorp/xorpfd.hh"

#include "fea/io_ip.hh"
#include "fea/io_ip_manager.hh"


/**
 * @short A base class for Dummy I/O IP raw communication.
 *
 * Each protocol 'registers' for I/O and gets assigned one object
 * of this class.
 */
class IoIpDummy : public IoIp {
public:
    /**
     * Constructor for a given address family and protocol.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     * @param iftree the interface tree to use.
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param ip_protocol the IP protocol number (IPPROTO_*).
     */
    IoIpDummy(FeaDataPlaneManager& fea_data_plane_manager,
	      const IfTree& iftree, int family, uint8_t ip_protocol);

    /**
     * Virtual destructor.
     */
    virtual ~IoIpDummy();

    /**
     * Start operation.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		start(string& error_msg);

    /**
     * Stop operation.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		stop(string& error_msg);

    /**
     * Set the default TTL (or hop-limit in IPv6) for the outgoing multicast
     * packets.
     *
     * @param ttl the desired IP TTL (a.k.a. hop-limit in IPv6) value.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_multicast_ttl(int ttl, string& error_msg);

    /**
     * Enable/disable multicast loopback when transmitting multicast packets.
     *
     * If the multicast loopback is enabled, a transmitted multicast packet
     * will be delivered back to this host (assuming the host is a member of
     * the same multicast group).
     *
     * @param is_enabled if true, enable the loopback, otherwise disable it.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		enable_multicast_loopback(bool is_enabled, string& error_msg);

    /**
     * Set default interface for transmitting multicast packets.
     *
     * @param if_name the name of the interface that would become the default
     * multicast interface.
     * @param vif_name the name of the vif that would become the default
     * multicast interface.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		set_default_multicast_interface(const string& if_name,
						const string& vif_name,
						string& error_msg);

    /**
     * Create input socket.
     *
     * @param if_name the name of the interface to listen on
     * @param vif_name the name of the vif to listen on
     * @error error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR
     */
    int		create_input_socket(const string& if_name,
					const string& vif_name,
					string& error_msg);

    /**
     * Join a multicast group on an interface.
     *
     * @param if_name the name of the interface to join the multicast group.
     * @param vif_name the name of the vif to join the multicast group.
     * @param group the multicast group to join.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		join_multicast_group(const string& if_name,
				     const string& vif_name,
				     const IPvX& group,
				     string& error_msg);

    /**
     * Leave a multicast group on an interface.
     *
     * @param if_name the name of the interface to leave the multicast group.
     * @param vif_name the name of the vif to leave the multicast group.
     * @param group the multicast group to leave.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		leave_multicast_group(const string& if_name,
				      const string& vif_name,
				      const IPvX& group,
				      string& error_msg);

    /**
     * Send a raw IP packet.
     *
     * @param if_name the interface to send the packet on. It is essential for
     * multicast. In the unicast case this field may be empty.
     * @param vif_name the vif to send the packet on. It is essential for
     * multicast. In the unicast case this field may be empty.
     * @param src_address the IP source address.
     * @param dst_address the IP destination address.
     * @param ip_ttl the IP TTL (hop-limit). If it has a negative value,
     * the TTL will be set internally before transmission.
     * @param ip_tos the Type Of Service (Diffserv/ECN bits for IPv4 or
     * IP traffic class for IPv6). If it has a negative value, the TOS will be
     * set internally before transmission.
     * @param ip_router_alert if true, then add the IP Router Alert option to
     * the IP packet.
     * @param ip_internet_control if true, then this is IP control traffic.
     * @param ext_headers_type a vector of integers with the types of the
     * optional IPv6 extention headers.
     * @param ext_headers_payload a vector of payload data, one for each
     * optional IPv6 extention header. The number of entries must match
     * ext_headers_type.
     * @param payload the payload, everything after the IP header and options.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		send_packet(const string&	if_name,
			    const string&	vif_name,
			    const IPvX&		src_address,
			    const IPvX&		dst_address,
			    int32_t		ip_ttl,
			    int32_t		ip_tos,
			    bool		ip_router_alert,
			    bool		ip_internet_control,
			    const vector<uint8_t>& ext_headers_type,
			    const vector<vector<uint8_t> >& ext_headers_payload,
			    const vector<uint8_t>& payload,
			    string&		error_msg);

    /**
     * Get the file descriptor for receiving protocol messages.
     *
     * @return a reference to the file descriptor for receiving protocol
     * messages.
     */
    XorpFd* findExistingInputSocketMcast(const string& _if_name, const string& _vif_name) {
	UNUSED(_if_name);
	UNUSED(_vif_name);
	return (&_dummy_protocol_fd_in);
    }

    XorpFd* findOrCreateInputSocketMcast(const string& _if_name, const string& _vif_name, string& error_msg) {
	UNUSED(error_msg);
	return findExistingInputSocketMcast(_if_name, _vif_name);
    }

private:
    // Private state
    XorpFd	_dummy_protocol_fd_in;	// Dummy protocol file descriptor
    uint8_t	_multicast_ttl;		// Default multicast TTL
    bool	_multicast_loopback;	// True if mcast loopback is enabled
    string	_default_multicast_interface;
    string	_default_multicast_vif;

    set<IoIpComm::JoinedMulticastGroup>	_joined_groups_table;
};

#endif // __FEA_DATA_PLANE_IO_IO_IP_DUMMY_HH__
