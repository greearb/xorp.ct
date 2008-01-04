// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2008 International Computer Science Institute
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

// $XORP: xorp/fea/io_ip.hh,v 1.2 2008/01/03 22:59:35 pavlin Exp $


#ifndef __FEA_IO_IP_HH__
#define __FEA_IO_IP_HH__

#include <vector>

#include "fea_data_plane_manager.hh"

//
// I/O IP raw communication API.
//


class EventLoop;
class IfTree;
class IfTreeInterface;
class IfTreeVif;
class IoIpManager;
class IoIpReceiver;
class IPvX;
class XorpFd;


/**
 * @short A base class for I/O IP raw communication.
 * 
 * Each protocol 'registers' for I/O and gets assigned one object
 * of this class.
 */
class IoIp {
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
    IoIp(FeaDataPlaneManager& fea_data_plane_manager, const IfTree& iftree,
	 int family, uint8_t ip_protocol);

    /**
     * Virtual destructor.
     */
    virtual ~IoIp();

    /**
     * Get the @ref IoIpManager instance.
     *
     * @return the @ref IoIpManager instance.
     */
    IoIpManager& io_ip_manager() { return _io_ip_manager; }

    /**
     * Get the @ref FeaDataPlaneManager instance.
     *
     * @return the @ref FeaDataPlaneManager instance.
     */
    FeaDataPlaneManager& fea_data_plane_manager() { return _fea_data_plane_manager; }

    /**
     * Test whether this instance is running.
     *
     * @return true if the instance is running, otherwise false.
     */
    virtual bool is_running() const { return _is_running; }

    /**
     * Get the event loop.
     * 
     * @return the event loop.
     */
    EventLoop& eventloop() { return (_eventloop); }

    /**
     * Get the interface tree.
     *
     * @return the interface tree.
     */
    const IfTree& iftree() const { return (_iftree); }

    /**
     * Get the address family.
     * 
     * @return the address family.
     */
    virtual int family() const { return (_family); }

    /**
     * Get the IP protocol number.
     * 
     * @return the IP protocol number.
     */
    virtual uint8_t ip_protocol() const { return (_ip_protocol); }

    /**
     * Get the registered receiver.
     *
     * @return the registered receiver.
     */
    IoIpReceiver* io_ip_receiver() { return (_io_ip_receiver); }

    /**
     * Register the I/O IP raw packets receiver.
     *
     * @param io_ip_receiver the receiver to register.
     */
    virtual void register_io_ip_receiver(IoIpReceiver* io_ip_receiver);

    /**
     * Unregister the I/O IP raw packets receiver.
     */
    virtual void unregister_io_ip_receiver();

    /**
     * Start operation.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg) = 0;

    /**
     * Stop operation.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg) = 0;

    /**
     * Set the default TTL (or hop-limit in IPv6) for the outgoing multicast
     * packets.
     * 
     * @param ttl the desired IP TTL (a.k.a. hop-limit in IPv6) value.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_multicast_ttl(int ttl, string& error_msg) = 0;

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
    virtual int enable_multicast_loopback(bool is_enabled,
					  string& error_msg) = 0;

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
    virtual int set_default_multicast_interface(const string& if_name,
						const string& vif_name,
						string& error_msg) = 0;

    /**
     * Join a multicast group on an interface.
     * 
     * @param if_name the name of the interface to join the multicast group.
     * @param vif_name the name of the vif to join the multicast group.
     * @param group the multicast group to join.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int join_multicast_group(const string& if_name,
				     const string& vif_name,
				     const IPvX& group,
				     string& error_msg) = 0;

    /**
     * Leave a multicast group on an interface.
     * 
     * @param if_name the name of the interface to leave the multicast group.
     * @param vif_name the name of the vif to leave the multicast group.
     * @param group the multicast group to leave.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int leave_multicast_group(const string& if_name,
				      const string& vif_name,
				      const IPvX& group,
				      string& error_msg) = 0;

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
    virtual int send_packet(const string&	if_name,
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
			    string&		error_msg) = 0;

    /**
     * Get the file descriptor for receiving protocol messages.
     *
     * @return a reference to the file descriptor for receiving protocol
     * messages.
     */
    virtual XorpFd& protocol_fd_in() = 0;

protected:
    /**
     * Received a raw IP packet.
     *
     * @param if_name the interface name the packet arrived on.
     * @param vif_name the vif name the packet arrived on.
     * @param src_address the IP source address.
     * @param dst_address the IP destination address.
     * @param ip_ttl the IP TTL (hop-limit). If it has a negative value,
     * then the received value is unknown.
     * @param ip_tos The type of service (Diffserv/ECN bits for IPv4). If it
     * has a negative value, then the received value is unknown.
     * @param ip_router_alert if true, the IP Router Alert option was
     * included in the IP packet.
     * @param ip_internet_control if true, then this is IP control traffic.
     * @param ext_headers_type a vector of integers with the types of the
     * optional IPv6 extention headers.
     * @param ext_headers_payload a vector of payload data, one for each
     * optional IPv6 extention header. The number of entries must match
     * ext_headers_type.
     * @param packet the payload, everything after the IP header and
     * options.
     */
    virtual void recv_packet(const string&	if_name,
			     const string&	vif_name,
			     const IPvX&	src_address,
			     const IPvX&	dst_address,
			     int32_t		ip_ttl,
			     int32_t		ip_tos,
			     bool		ip_router_alert,
			     bool		ip_internet_control,
			     const vector<uint8_t>& ext_headers_type,
			     const vector<vector<uint8_t> >& ext_headers_payload,
			     const vector<uint8_t>& payload);

    /**
     * Received a multicast forwarding related upcall from the system.
     *
     * Examples of such upcalls are: "nocache", "wrongiif", "wholepkt",
     * "bw_upcall".
     *
     * @param payload the payload data for the upcall.
     */
    virtual void recv_system_multicast_upcall(const vector<uint8_t>& payload);

    // Misc other state
    bool	_is_running;

private:
    IoIpManager&	_io_ip_manager;		// The I/O IP manager
    FeaDataPlaneManager& _fea_data_plane_manager; // The data plane manager
    EventLoop&		_eventloop;		// The event loop to use
    const IfTree&	_iftree;		// The interface tree to use
    int			_family;		// The address family
    uint8_t		_ip_protocol;	     // The protocol number (IPPROTO_*)
    IoIpReceiver*	_io_ip_receiver;	// The registered receiver
};

/**
 * @short A base class for I/O IP raw packets receiver.
 *
 * The real receiver must inherit from this class and register with the
 * corresponding IoIp entity to receive the raw IP packets.
 * @see IoIp.
 */
class IoIpReceiver {
public:
    /**
     * Default constructor.
     */
    IoIpReceiver() {}

    /**
     * Virtual destructor.
     */
    virtual ~IoIpReceiver() {}

    /**
     * Received a raw IP packet.
     *
     * @param if_name the interface name the packet arrived on.
     * @param vif_name the vif name the packet arrived on.
     * @param src_address the IP source address.
     * @param dst_address the IP destination address.
     * @param ip_ttl the IP TTL (hop-limit). If it has a negative value,
     * then the received value is unknown.
     * @param ip_tos The type of service (Diffserv/ECN bits for IPv4). If it
     * has a negative value, then the received value is unknown.
     * @param ip_router_alert if true, the IP Router Alert option was
     * included in the IP packet.
     * @param ip_internet_control if true, then this is IP control traffic.
     * @param ext_headers_type a vector of integers with the types of the
     * optional IPv6 extention headers.
     * @param ext_headers_payload a vector of payload data, one for each
     * optional IPv6 extention header. The number of entries must match
     * ext_headers_type.
     * @param packet the payload, everything after the IP header and
     * options.
     */
    virtual void recv_packet(const string&	if_name,
			     const string&	vif_name,
			     const IPvX&	src_address,
			     const IPvX&	dst_address,
			     int32_t		ip_ttl,
			     int32_t		ip_tos,
			     bool		ip_router_alert,
			     bool		ip_internet_control,
			     const vector<uint8_t>& ext_headers_type,
			     const vector<vector<uint8_t> >& ext_headers_payload,
			     const vector<uint8_t>& payload) = 0;

    /**
     * Received a multicast forwarding related upcall from the system.
     *
     * Examples of such upcalls are: "nocache", "wrongiif", "wholepkt",
     * "bw_upcall".
     *
     * @param payload the payload data for the upcall.
     */
    virtual void recv_system_multicast_upcall(const vector<uint8_t>& payload) = 0;
};

#endif // __FEA_IO_IP_HH__
