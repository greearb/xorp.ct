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



#ifndef __FEA_IO_TCPUDP_HH__
#define __FEA_IO_TCPUDP_HH__



#include "fea_data_plane_manager.hh"

//
// I/O TCP/UDP communication API.
//


class EventLoop;
class IfTree;
class IfTreeInterface;
class IfTreeVif;
class IoTcpUdpManager;
class IoTcpUdpReceiver;
class IPvX;


/**
 * @short A base class for I/O TCP/UDP communication.
 */
class IoTcpUdp {
public:
    /**
     * Constructor for a given address family.
     * 
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     * @param iftree the interface tree to use.
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param is_tcp if true this is TCP entry, otherwise UDP.
     */
    IoTcpUdp(FeaDataPlaneManager& fea_data_plane_manager, const IfTree& iftree,
	     int family, bool is_tcp);

    /**
     * Virtual destructor.
     */
    virtual ~IoTcpUdp();

    /**
     * Get the @ref IoTcpUdpManager instance.
     *
     * @return the @ref IoTcpUdpManager instance.
     */
    IoTcpUdpManager& io_tcpudp_manager() { return _io_tcpudp_manager; }

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
     * Test whether this is TCP entry.
     *
     * @return true if this is TCP entry, otherwise false (i.e., UDP).
     */
    virtual bool is_tcp() const { return (_is_tcp); }

    /**
     * Get the registered receiver.
     *
     * @return the registered receiver.
     */
    IoTcpUdpReceiver* io_tcpudp_receiver() { return (_io_tcpudp_receiver); }

    /**
     * Register the I/O TCP/UDP data receiver.
     *
     * @param io_tcpudp_receiver the receiver to register.
     */
    virtual void register_io_tcpudp_receiver(IoTcpUdpReceiver* io_tcpudp_receiver);

    /**
     * Unregister the I/O TCP/UDP data receiver.
     */
    virtual void unregister_io_tcpudp_receiver();

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
     * Open a TCP socket.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int tcp_open(string& error_msg) = 0;

    /**
     * Open an UDP socket.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int udp_open(string& error_msg) = 0;

    /**
     * Create a bound TCP socket.
     *
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int tcp_open_and_bind(const IPvX& local_addr, uint16_t local_port,
				  string& error_msg) = 0;

    /**
     * Create a bound UDP socket.
     *
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int udp_open_and_bind(const IPvX& local_addr, uint16_t local_port, const string& local_dev,
				  int reuse, string& error_msg) = 0;

    /**
     * Create a bound UDP multicast socket.
     *
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param mcast_addr the multicast group address to join.
     * @param ttl the TTL to use for this multicast socket.
     * @param reuse allow other sockets to bind to same multicast group.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int udp_open_bind_join(const IPvX& local_addr, uint16_t local_port,
				   const IPvX& mcast_addr, uint8_t ttl,
				   bool reuse,
				   string& error_msg) = 0;

    /**
     * Create a bound and connected TCP socket.
     *
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param remote_addr the address to connect to.
     * @param remote_port the remote port to connect to.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int tcp_open_bind_connect(const IPvX& local_addr,
				      uint16_t local_port,
				      const IPvX& remote_addr,
				      uint16_t remote_port,
				      string& error_msg) = 0;

    /**
     * Create a bound and connected UDP socket.
     *
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param remote_addr the address to connect to.
     * @param remote_port the remote port to connect to.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int udp_open_bind_connect(const IPvX& local_addr,
				      uint16_t local_port,
				      const IPvX& remote_addr,
				      uint16_t remote_port,
				      string& error_msg) = 0;

    /**
     * Create a bound, and optionally connected, UDP broadcast socket.
     *
     * @param ifname the interface name to bind socket to.
     * @param vifname the vif to bind socket to.
     * @param local_port the port to bind socket to.
     * @param remote_port the remote port to connect to.
     * @param reuse allow other sockets to bind to same port.
     * @param limited set the socket up for transmission to the limited
     * broadcast address 255.255.255.255.
     * @param connected connect the socket for use with send() not sendto().
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int udp_open_bind_broadcast(const string& ifname,
				        const string& vifname,
				        uint16_t local_port,
				        uint16_t remote_port,
				        bool reuse,
				        bool limited,
				        bool connected,
				        string& error_msg) = 0;

    /**
     * Bind a socket.
     *
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int bind(const IPvX& local_addr, uint16_t local_port,
		     string& error_msg) = 0;

    /**
     * Join multicast group on already bound socket.
     *
     * @param mcast_addr group to join.
     * @param join_if_addr interface address to perform join on.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int udp_join_group(const IPvX& mcast_addr,
			       const IPvX& join_if_addr,
			       string& error_msg) = 0;

    /**
     * Leave multicast group on already bound socket.
     *
     * @param mcast_addr group to leave.
     * @param leave_if_addr interface address to perform leave on.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int udp_leave_group(const IPvX& mcast_addr,
				const IPvX& leave_if_addr,
				string& error_msg) = 0;

    /**
     * Close socket.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int close(string& error_msg) = 0;

    /**
     * Listen for inbound connections on socket.
     *
     * When a connection request is received the socket creator will receive
     * notification.
     *
     * @param backlog the maximum number of pending connections.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int tcp_listen(uint32_t backlog, string& error_msg) = 0;

    /**
     * Enable a UDP socket for datagram reception.
     *
     * When a connection request is received the socket creator will receive
     * notification.
     *
     * @param backlog the maximum number of pending connections.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int udp_enable_recv(string& error_msg) = 0;

    /**
     * Send data on socket.
     *
     * @param data block of data to be sent.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int send(const vector<uint8_t>& data, string& error_msg) = 0;

    /**
     * Send data on socket to a given destination.
     *
     * The packet is not routed as the forwarding engine sending the packet
     * may not have access to the full routing table.
     *
     * @param remote_addr destination address for data.
     * @param remote_port destination port for data.
     * @param data block of data to be sent.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int send_to(const IPvX& remote_addr, uint16_t remote_port,
			const vector<uint8_t>& data, string& error_msg) = 0;

    /**
     * Send data on socket to a given multicast group from a given interface.
     *
     * @param group_addr destination address for data.
     * @param group_port destination port for data.
     * @param ifaddr interface address.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int send_from_multicast_if(const IPvX& group_addr,
				       uint16_t group_port,
				       const IPvX& ifaddr,
				       const vector<uint8_t>& data,
				       string& error_msg) = 0;

    /**
     * Set a named socket option with an integer value.
     *
     * @param optname name of option to be set. Valid values are:
     *  "onesbcast"		(IPv4 only)
     *  "receive_broadcast"	(IPv4 only)
     *  "reuseport"
     *  "send_broadcast"	(IPv4 only)
     *  "tos"			(IPv4 only)
     *  "ttl"
     *  "multicast_loopback"
     *  "multicast_ttl"
     * @param optval value of option to be set. If value is logically boolean
     * then zero represents false and any non-zero value true.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_socket_option(const string& optname, uint32_t optval,
				  string& error_msg) = 0;

    /**
     * Set a named socket option with a string value.
     *
     * @param optname name of option to be set. Valid values are:
     * "bindtodevice"
     * @param optval value of option to be set.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_socket_option(const string& optname,
                                  const string& optval,
				  string& error_msg) = 0;

    /**
     * Accept or reject a pending connection.
     *
     * @param is_accepted if true, the connection is accepted, otherwise is
     * rejected.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int accept_connection(bool is_accepted, string& error_msg) = 0;

protected:
    /**
     * Data received event.
     *
     * @param if_name the interface name the packet arrived on, if known.
     * If unknown, then it is an empty string.
     * @param vif_name the vif name the packet arrived on, if known.
     * If unknown, then it is an empty string.
     * @param src_host the originating host IP address.
     * @param src_port the originating host port number.
     * @param data the data received.
     */
    virtual void recv_event(const string&		if_name,
			    const string&		vif_name,
			    const IPvX&			src_host,
			    uint16_t			src_port,
			    const vector<uint8_t>&	data);

    /**
     * Inbound connection request received event.
     *
     * It applies only to TCP sockets.
     *
     * @param src_host the originating host IP address.
     * @param src_port the originating host port number.
     * @param new_io_tcpudp the handler for the new connection.
     */
    virtual void inbound_connect_event(const IPvX&	src_host,
				       uint16_t		src_port,
				       IoTcpUdp*	new_io_tcpudp);

    /**
     * Outgoing connection request completed event.
     *
     * It applies only to TCP sockets.
     */
    virtual void outgoing_connect_event();

    /**
     * Error occured event.
     *
     * @param error a textual description of the error.
     * @param fatal indication of whether socket is shutdown because of
     * error.
     */
    virtual void error_event(const string&		error,
			     bool			fatal);

    /**
     * Connection closed by peer event.
     *
     * It applies only to TCP sockets.
     * This method is not called if the socket is gracefully closed
     * through close().
     */
    virtual void disconnect_event();

    // Misc other state
    bool	_is_running;

private:
    IoTcpUdpManager&	_io_tcpudp_manager;	// The I/O TCP/UDP manager
    FeaDataPlaneManager& _fea_data_plane_manager; // The data plane manager
    EventLoop&		_eventloop;		// The event loop to use
    const IfTree&	_iftree;		// The interface tree to use
    const int		_family;		// The address family
    const bool		_is_tcp;	// If true, this is TCP, otherwise UDP
    IoTcpUdpReceiver*	_io_tcpudp_receiver;	// The registered receiver
};

/**
 * @short A base class for I/O TCP/UDP data receiver.
 *
 * The real receiver must inherit from this class and register with the
 * corresponding IoTcpUdp entity to receive the TCP/UDP data and data-related
 * events.
 * @see IoTcpUdp.
 */
class IoTcpUdpReceiver {
public:
    /**
     * Default constructor.
     */
    IoTcpUdpReceiver() {}

    /**
     * Virtual destructor.
     */
    virtual ~IoTcpUdpReceiver() {}

    /**
     * Data received event.
     *
     * @param if_name the interface name the packet arrived on, if known.
     * If unknown, then it is an empty string.
     * @param vif_name the vif name the packet arrived on, if known.
     * If unknown, then it is an empty string.
     * @param src_host the originating host IP address.
     * @param src_port the originating host port number.
     * @param data the data received.
     */
    virtual void recv_event(const string&		if_name,
			    const string&		vif_name,
			    const IPvX&			src_host,
			    uint16_t			src_port,
			    const vector<uint8_t>&	data) = 0;

    /**
     * Inbound connection request received event.
     *
     * It applies only to TCP sockets.
     *
     * @param src_host the originating host IP address.
     * @param src_port the originating host port number.
     * @param new_io_tcpudp the handler for the new connection.
     */
    virtual void inbound_connect_event(const IPvX&	src_host,
				       uint16_t		src_port,
				       IoTcpUdp*	Xnew_io_tcpudp) = 0;

    /**
     * Outgoing connection request completed event.
     *
     * It applies only to TCP sockets.
     */
    virtual void outgoing_connect_event() = 0;

    /**
     * Error occured event.
     *
     * @param error a textual description of the error.
     * @param fatal indication of whether socket is shutdown because of
     * error.
     */
    virtual void error_event(const string&		error,
			     bool			fatal) = 0;

    /**
     * Connection closed by peer event.
     *
     * It applies only to TCP sockets.
     * This method is not called if the socket is gracefully closed
     * through close().
     */
    virtual void disconnect_event() = 0;
};

#endif // __FEA_IO_TCPUDP_HH__
