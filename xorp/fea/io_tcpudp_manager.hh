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

// $XORP: xorp/fea/io_tcpudp_manager.hh,v 1.13 2008/10/02 21:56:49 bms Exp $

#ifndef __FEA_IO_TCPUDP_MANAGER_HH__
#define __FEA_IO_TCPUDP_MANAGER_HH__





#include "libxorp/ipvx.hh"

#include "fea_io.hh"
#include "io_tcpudp.hh"

class FeaNode;
class FeaDataPlaneManager;


/**
 * A class that handles I/O TCP/UDP communication.
 */
class IoTcpUdpComm :
    public NONCOPYABLE,
    public IoTcpUdpReceiver
{
public:
    /**
     * Joined multicast group class.
     */
    class JoinedMulticastGroup {
    public:
	JoinedMulticastGroup(const IPvX& interface_address,
			     const IPvX& group_address)
	    : _interface_address(interface_address),
	      _group_address(group_address)
	{}
#ifdef XORP_USE_USTL
	JoinedMulticastGroup() { }
#endif
	virtual ~JoinedMulticastGroup() {}

	const IPvX& interface_address() const { return _interface_address; }
	const IPvX& group_address() const { return _group_address; }

	/**
	 * Less-Than Operator
	 *
	 * @param other the right-hand operand to compare against.
	 * @return true if the left-hand operand is numerically smaller
	 * than the right-hand operand.
	 */
	bool operator<(const JoinedMulticastGroup& other) const {
	    if (_interface_address != other._interface_address)
		return (_interface_address < other._interface_address);
	    return (_group_address < other._group_address);
	}

	/**
	 * Equality Operator
	 *
	 * @param other the right-hand operand to compare against.
	 * @return true if the left-hand operand is numerically same as the
	 * right-hand operand.
	 */
	bool operator==(const JoinedMulticastGroup& other) const {
	    return ((_interface_address == other._interface_address)
		    && (_group_address == other._group_address));
	}

	/**
	 * Add a receiver.
	 *
	 * @param receiver_name the name of the receiver to add.
	 */
	void add_receiver(const string& receiver_name) {
	    _receivers.insert(receiver_name);
	}

	/**
	 * Delete a receiver.
	 *
	 * @param receiver_name the name of the receiver to delete.
	 */
	void delete_receiver(const string& receiver_name) {
	    _receivers.erase(receiver_name);
	}

	/**
	 * @return true if there are no receivers associated with this group.
	 */
	bool empty() const { return _receivers.empty(); }

    private:
	IPvX		_interface_address;
	IPvX		_group_address;
	set<string>	_receivers;
    };

    /**
     * Constructor for IoTcpUdpComm.
     *
     * @param io_tcpudp_manager the corresponding I/O TCP/UDP manager
     * (@ref IoTcpUdpManager).
     * @param iftree the interface tree to use.
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param is_tcp if true this is TCP entry, otherwise UDP.
     * @param creator the name of the socket creator.
     */
    IoTcpUdpComm(IoTcpUdpManager& io_tcpudp_manager, const IfTree& iftree,
		 int family, bool is_tcp, const string& creator);

    /**
     * Constructor for connected IoTcpUdpComm.
     *
     * @param io_tcpudp_manager the corresponding I/O TCP/UDP manager
     * (@ref IoTcpUdpManager).
     * @param iftree the interface tree to use.
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param is_tcp if true this is TCP entry, otherwise UDP.
     * @param creator the name of the socket creator.
     * @param listener_sockid the socket ID of the listener socket.
     * @param peer_host the peer host IP address.
     * @param peer_port the peer host port number.
     */
    IoTcpUdpComm(IoTcpUdpManager& io_tcpudp_manager, const IfTree& iftree,
		 int family, bool is_tcp, const string& creator,
		 const string& listener_sockid, const IPvX& peer_host,
		 uint16_t peer_port);

    /**
     * Virtual destructor.
     */
    virtual ~IoTcpUdpComm();

    /**
     * Allocate the I/O TCP/UDP plugins (one per data plane manager).
     */
    void allocate_io_tcpudp_plugins();

    /**
     * Deallocate the I/O TCP/UDP plugins (one per data plane manager).
     */
    void deallocate_io_tcpudp_plugins();

    /**
     * Allocate an I/O TCP/UDP plugin for a given data plane manager.
     *
     * @param fea_data_plane_manager the data plane manager.
     */
    void allocate_io_tcpudp_plugin(FeaDataPlaneManager* fea_data_plane_manager);

    /**
     * Deallocate the I/O TCP/UDP plugin for a given data plane manager.
     *
     * @param fea_data_plane_manager the data plane manager.
     */
    void deallocate_io_tcpudp_plugin(FeaDataPlaneManager* fea_data_plane_manager);

    /**
     * Add a pre-allocated I/O TCP/UDP plugin.
     *
     * @param new_io_tcpudp the plugin to add.
     */
    void add_plugin(IoTcpUdp* new_io_tcpudp);

    /**
     * Start all I/O TCP/UDP plugins.
     */
    void start_io_tcpudp_plugins();

    /**
     * Stop all I/O TCP/UDP plugins.
     */
    void stop_io_tcpudp_plugins();

    /**
     * Open a TCP socket.
     *
     * @param sockid return parameter that contains unique socket ID when
     * socket instantiation is successful.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int tcp_open(string& sockid, string& error_msg);

    /**
     * Open an UDP socket.
     *
     * @param sockid return parameter that contains unique socket ID when
     * socket instantiation is successful.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int udp_open(string& sockid, string& error_msg);

    /**
     * Create a bound TCP socket.
     *
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param sockid return parameter that contains unique socket ID when
     * socket instantiation is successful.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int tcp_open_and_bind(const IPvX& local_addr, uint16_t local_port,
			  string& sockid, string& error_msg);

    /**
     * Create a bound UDP socket.
     *
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param sockid return parameter that contains unique socket ID when
     * socket instantiation is successful.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int udp_open_and_bind(const IPvX& local_addr, uint16_t local_port,
			  const string& local_dev, int reuse, string& sockid, string& error_msg);

    /**
     * Create a bound UDP multicast socket.
     *
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param mcast_addr the multicast group address to join.
     * @param ttl the TTL to use for this multicast socket.
     * @param reuse allow other sockets to bind to same multicast group.
     * @param sockid return parameter that contains unique socket ID when
     * socket instantiation is successful.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int udp_open_bind_join(const IPvX& local_addr, uint16_t local_port,
			   const IPvX& mcast_addr, uint8_t ttl, bool reuse,
			   string& sockid, string& error_msg);

    /**
     * Create a bound and connected TCP socket.
     *
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param remote_addr the address to connect to.
     * @param remote_port the remote port to connect to.
     * @param sockid return parameter that contains unique socket ID when
     * socket instantiation is successful.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int tcp_open_bind_connect(const IPvX& local_addr, uint16_t local_port,
			      const IPvX& remote_addr, uint16_t remote_port,
			      string& sockid, string& error_msg);

    /**
     * Create a bound and connected UDP socket.
     *
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param remote_addr the address to connect to.
     * @param remote_port the remote port to connect to.
     * @param sockid return parameter that contains unique socket ID when
     * socket instantiation is successful.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int udp_open_bind_connect(const IPvX& local_addr, uint16_t local_port,
			      const IPvX& remote_addr, uint16_t remote_port,
			      string& sockid, string& error_msg);

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
     * @param sockid return parameter that contains unique socket ID when
     *        socket instantiation is successful.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int udp_open_bind_broadcast(const string& ifname, const string& vifname,
			        uint16_t local_port, uint16_t remote_port,
			        bool reuse, bool limited, bool connected,
			        string& sockid, string& error_msg);

    /**
     * Bind a socket.
     *
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int bind(const IPvX& local_addr, uint16_t local_port, string& error_msg);

    /**
     * Join multicast group on already bound socket.
     *
     * @param mcast_addr group to join.
     * @param join_if_addr interface address to perform join on.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int udp_join_group(const IPvX& mcast_addr, const IPvX& join_if_addr,
		       string& error_msg);

    /**
     * Leave multicast group on already bound socket.
     *
     * @param mcast_addr group to leave.
     * @param leave_if_addr interface address to perform leave on.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int udp_leave_group(const IPvX& mcast_addr, const IPvX& leave_if_addr,
			string& error_msg);

    /**
     * Close socket.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int close(string& error_msg);

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
    int tcp_listen(uint32_t backlog, string& error_msg);

    /**
     * Enable a UDP socket for datagram reception.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int udp_enable_recv(string& error_msg);

    /**
     * Send data on socket.
     *
     * @param data block of data to be sent.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int send(const vector<uint8_t>& data, string& error_msg);

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
    int send_to(const IPvX& remote_addr, uint16_t remote_port,
		const vector<uint8_t>& data, string& error_msg);

    /**
     * Send data on socket to a given multicast group from a given interface.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param sockid unique socket ID.
     * @param group_addr destination address for data.
     * @param group_port destination port for data.
     * @param ifaddr interface address.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int send_from_multicast_if(const IPvX& group_addr, uint16_t group_port,
			       const IPvX& ifaddr, const vector<uint8_t>& data,
			       string& error_msg);

    /**
     * Set a named socket option with an integer value.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param sockid unique socket ID.
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
    int set_socket_option(const string& optname, uint32_t optval,
			  string& error_msg);

    /**
     * Set a named socket option with a string value.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param sockid unique socket ID.
     * @param optname name of option to be set. Valid values are:
     * "bindtodevice"
     * @param optval value of option to be set.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_socket_option(const string& optname, const string& optval,
			  string& error_msg);

    /**
     * Accept or reject a pending connection.
     *
     * @param is_accepted if true, the connection is accepted, otherwise is
     * rejected.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int accept_connection(bool is_accepted, string& error_msg);

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
    void recv_event(const string&		if_name,
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
    void inbound_connect_event(const IPvX&	src_host,
			       uint16_t		src_port,
			       IoTcpUdp*	new_io_tcpudp);

    /**
     * Outgoing connection request completed event.
     *
     * It applies only to TCP sockets.
     */
    void outgoing_connect_event();

    /**
     * Error occured event.
     *
     * @param error a textual description of the error.
     * @param fatal indication of whether socket is shutdown because of
     * error.
     */
    void error_event(const string&		error,
		     bool			fatal);

    /**
     * Connection closed by peer event.
     *
     * It applies only to TCP sockets.
     * This method is not called if the socket is gracefully closed
     * through close().
     */
    void disconnect_event();

    /**
     * Get the creator name.
     *
     * @return the creator name.
     */
    const string& creator() const { return (_creator); }

    /**
     * Get the socket ID.
     *
     * @return the socket ID.
     */
    const string& sockid() const { return (_sockid); }

    /**
     * Get the listener socket ID.
     *
     * @return the listener socket ID.
     */
    const string& listener_sockid() const { return (_listener_sockid); }

    /**
     * Get the peer host IP address.
     *
     * @return the peer host IP address.
     */
    const IPvX& peer_host() const { return (_peer_host); }

    /**
     * Get the peer host port number.
     *
     * @return the peer host port number.
     */
    uint16_t peer_port() const { return (_peer_port); }

private:
    IoTcpUdpComm(const IoTcpUdpComm&);			// Not implemented.
    IoTcpUdpComm& operator=(const IoTcpUdpComm&);	// Not implemented.

    IoTcpUdpManager&		_io_tcpudp_manager;
    const IfTree&		_iftree;
    const int			_family;
    const bool			_is_tcp;
    const string		_creator;
    const string		_sockid;

    // State for connected entries
    const string		_listener_sockid;
    const IPvX			_peer_host;
    const uint16_t		_peer_port;

    typedef list<pair<FeaDataPlaneManager*, IoTcpUdp*> >IoTcpUdpPlugins;
    IoTcpUdpPlugins		_io_tcpudp_plugins;

    typedef map<JoinedMulticastGroup, JoinedMulticastGroup> JoinedGroupsTable;
    JoinedGroupsTable		_joined_groups_table;
};

/**
 * @short Class that implements the API for sending TCP/UDP packets
 * and related events to a receiver.
 */
class IoTcpUdpManagerReceiver {
public:
    /**
     * Virtual destructor.
     */
    virtual ~IoTcpUdpManagerReceiver() {}

    /**
     * Data received event.
     *
     * @param receiver_name the name of the receiver to send the data to.
     * @param sockid unique socket ID.
     * @param if_name the interface name the packet arrived on, if known.
     * If unknown, then it is an empty string.
     * @param vif_name the vif name the packet arrived on, if known.
     * If unknown, then it is an empty string.
     * @param src_host the originating host IP address.
     * @param src_port the originating host port number.
     * @param data the data received.
     */
    virtual void recv_event(const string&		receiver_name,
			    const string&		sockid,
			    const string&		if_name,
			    const string&		vif_name,
			    const IPvX&			src_host,
			    uint16_t			src_port,
			    const vector<uint8_t>&	data) = 0;

    /**
     * Inbound connection request received event.
     *
     * It applies only to TCP sockets.
     *
     * @param receiver_name the name of the receiver to send the event to.
     * @param sockid unique socket ID.
     * @param src_host the originating host IP address.
     * @param src_port the originating host port number.
     * @param new_sockid the new socket ID.
     */
    virtual void inbound_connect_event(const string&	receiver_name,
				       const string&	sockid,
				       const IPvX&	src_host,
				       uint16_t		src_port,
				       const string&	new_sockid) = 0;

    /**
     * Outgoing connection request completed event.
     *
     * It applies only to TCP sockets.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param receiver_name the name of the receiver to send the event to.
     * @param sockid unique socket ID.
     */
    virtual void outgoing_connect_event(int		family,
					const string&	receiver_name,
					const string&	sockid) = 0;

    /**
     * Error occured event.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param receiver_name the name of the receiver to send the event to.
     * @param sockid unique socket ID.
     * @param error a textual description of the error.
     * @param fatal indication of whether socket is shutdown because of
     * error.
     */
    virtual void error_event(int			family,
			     const string&		receiver_name,
			     const string&		sockid,
			     const string&		error,
			     bool			fatal) = 0;

    /**
     * Connection closed by peer event.
     *
     * It applies only to TCP sockets.
     * This method is not called if the socket is gracefully closed
     * through close().
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param receiver_name the name of the receiver to send the event to.
     * @param sockid unique socket ID.
     */
    virtual void disconnect_event(int			family,
				  const string&		receiver_name,
				  const string&		sockid) = 0;
};

/**
 * @short A class that manages I/O TCP/UDP communications.
 */
class IoTcpUdpManager : public IoTcpUdpManagerReceiver,
			public InstanceWatcher {
public:
    /**
     * Constructor for IoTcpUdpManager.
     */
    IoTcpUdpManager(FeaNode& fea_node, const IfTree& iftree);

    /**
     * Virtual destructor.
     */
    virtual ~IoTcpUdpManager();

    /**
     * Open a TCP socket.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param creator the name of the socket creator.
     * @param sockid return parameter that contains unique socket ID when
     * socket instantiation is successful.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int tcp_open(int family, const string& creator, string& sockid,
		 string& error_msg);

    /**
     * Open an UDP socket.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param creator the name of the socket creator.
     * @param sockid return parameter that contains unique socket ID when
     * socket instantiation is successful.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int udp_open(int family, const string& creator, string& sockid,
		 string& error_msg);

    /**
     * Create a bound TCP socket.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param creator the name of the socket creator.
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param sockid return parameter that contains unique socket ID when
     * socket instantiation is successful.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int tcp_open_and_bind(int family, const string& creator,
			  const IPvX& local_addr, uint16_t local_port,
			  string& sockid, string& error_msg);

    /**
     * Create a bound UDP socket.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param creator the name of the socket creator.
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param sockid return parameter that contains unique socket ID when
     * socket instantiation is successful.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int udp_open_and_bind(int family, const string& creator,
			  const IPvX& local_addr, uint16_t local_port,
			  const string& local_dev, int reuse,
			  string& sockid, string& error_msg);

    /**
     * Create a bound UDP multicast socket.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param creator the name of the socket creator.
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param mcast_addr the multicast group address to join.
     * @param ttl the TTL to use for this multicast socket.
     * @param reuse allow other sockets to bind to same multicast group.
     * @param sockid return parameter that contains unique socket ID when
     * socket instantiation is successful.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int udp_open_bind_join(int family, const string& creator,
			   const IPvX& local_addr, uint16_t local_port,
			   const IPvX& mcast_addr, uint8_t ttl, bool reuse,
			   string& sockid, string& error_msg);

    /**
     * Create a bound and connected TCP socket.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param creator the name of the socket creator.
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param remote_addr the address to connect to.
     * @param remote_port the remote port to connect to.
     * @param sockid return parameter that contains unique socket ID when
     * socket instantiation is successful.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int tcp_open_bind_connect(int family, const string& creator,
			      const IPvX& local_addr, uint16_t local_port,
			      const IPvX& remote_addr, uint16_t remote_port,
			      string& sockid, string& error_msg);

    /**
     * Create a bound and connected UDP socket.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param creator the name of the socket creator.
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param remote_addr the address to connect to.
     * @param remote_port the remote port to connect to.
     * @param sockid return parameter that contains unique socket ID when
     * socket instantiation is successful.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int udp_open_bind_connect(int family, const string& creator,
			      const IPvX& local_addr, uint16_t local_port,
			      const IPvX& remote_addr, uint16_t remote_port,
			      string& sockid, string& error_msg);

    /**
     * Create a bound and connected UDP broadcast socket.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param creator the name of the socket creator.
     * @param ifname the interface name to bind socket to.
     * @param vifname the vif to bind socket to.
     * @param local_port the port to bind socket to.
     * @param remote_port the remote port to connect to.
     * @param reuse allow other sockets to bind to same port.
     * @param limited set the socket up for transmission to the limited
     * broadcast address 255.255.255.255.
     * @param connected connect the socket for use with send() not sendto().
     * @param sockid return parameter that contains unique socket ID when
     * socket instantiation is successful.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int udp_open_bind_broadcast(int family, const string& creator,
			        const string& ifname, const string& vifname,
			        uint16_t local_port, uint16_t remote_port,
			        bool reuse, bool limited, bool connected,
			        string& sockid, string& error_msg);

    /**
     * Bind a socket.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param sockid the socket ID of the socket to bind.
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int bind(int family, const string& sockid, const IPvX& local_addr,
	     uint16_t local_port, string& error_msg);

    /**
     * Join multicast group on already bound socket.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param sockid unique socket ID.
     * @param mcast_addr group to join.
     * @param join_if_addr interface address to perform join on.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int udp_join_group(int family, const string& sockid,
		       const IPvX& mcast_addr, const IPvX& join_if_addr,
		       string& error_msg);

    /**
     * Leave multicast group on already bound socket.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param sockid unique socket ID.
     * @param mcast_addr group to leave.
     * @param leave_if_addr interface address to perform leave on.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int udp_leave_group(int family, const string& sockid,
			const IPvX& mcast_addr, const IPvX& leave_if_addr,
			string& error_msg);

    /**
     * Close socket.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param sockid unique socket ID of socket to be closed.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int close(int family, const string& sockid, string& error_msg);

    /**
     * Listen for inbound connections on socket.
     *
     * When a connection request is received the socket creator will receive
     * notification.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param sockid the unique socket ID of the socket to perform listen.
     * @param backlog the maximum number of pending connections.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int tcp_listen(int family, const string& sockid, uint32_t backlog,
		   string& error_msg);

    /**
     * Enable a UDP socket for datagram reception.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param sockid the unique socket ID of the socket to enable
     * datagram reception for.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int udp_enable_recv(int family, const string& sockid, string& error_msg);

    /**
     * Send data on socket.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param sockid unique socket ID.
     * @param data block of data to be sent.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int send(int family, const string& sockid, const vector<uint8_t>& data,
	     string& error_msg);

    /**
     * Send data on socket to a given destination.
     *
     * The packet is not routed as the forwarding engine sending the packet
     * may not have access to the full routing table.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param sockid unique socket ID.
     * @param remote_addr destination address for data.
     * @param remote_port destination port for data.
     * @param data block of data to be sent.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int send_to(int family, const string& sockid, const IPvX& remote_addr,
		uint16_t remote_port, const vector<uint8_t>& data,
		string& error_msg);

    /**
     * Send data on socket to a given multicast group from a given interface.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param sockid unique socket ID.
     * @param group_addr destination address for data.
     * @param group_port destination port for data.
     * @param ifaddr interface address.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int send_from_multicast_if(int family, const string& sockid,
			       const IPvX& group_addr, uint16_t group_port,
			       const IPvX& ifaddr, const vector<uint8_t>& data,
			       string& error_msg);

    /**
     * Set a named socket option with an integer value.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param sockid unique socket ID.
     * @param optname name of option to be set. Valid values are:
     *  "onesbcast"
     *  "receive_broadcast"
     *  "reuseport"
     *  "send_broadcast"
     *  "tos"
     *  "ttl"
     *  "multicast_loopback"
     *  "multicast_ttl"
     * @param optval value of option to be set. If value is logically boolean
     * then zero represents false and any non-zero value true.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_socket_option(int family, const string& sockid,
			  const string& optname, uint32_t optval,
			  string& error_msg);

    /**
     * Set a named socket option with a string value.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param sockid unique socket ID.
     * @param optname name of option to be set. Valid values are:
     * "bindtodevice"
     * @param optval value of option to be set.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_socket_option(int family, const string& sockid,
			  const string& optname, const string& optval,
			  string& error_msg);

    /**
     * Accept or reject a pending connection.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param sockid unique socket ID.
     * @param is_accepted if true, the connection is accepted, otherwise is
     * rejected.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int accept_connection(int family, const string& sockid, bool is_accepted,
			  string& error_msg);

    /**
     * Data received event.
     *
     * @param receiver_name the name of the receiver to send the data to.
     * @param sockid unique socket ID.
     * @param if_name the interface name the packet arrived on, if known.
     * If unknown, then it is an empty string.
     * @param vif_name the vif name the packet arrived on, if known.
     * If unknown, then it is an empty string.
     * @param src_host the originating host IP address.
     * @param src_port the originating host port number.
     * @param data the data received.
     */
    void recv_event(const string&		receiver_name,
		    const string&		sockid,
		    const string&		if_name,
		    const string&		vif_name,
		    const IPvX&			src_host,
		    uint16_t			src_port,
		    const vector<uint8_t>&	data);

    /**
     * Inbound connection request received event.
     *
     * It applies only to TCP sockets.
     *
     * @param receiver_name the name of the receiver to send the event to.
     * @param sockid unique socket ID.
     * @param src_host the originating host IP address.
     * @param src_port the originating host port number.
     * @param new_sockid the new socket ID.
     */
    void inbound_connect_event(const string&	receiver_name,
			       const string&	sockid,
			       const IPvX&	src_host,
			       uint16_t		src_port,
			       const string&	new_sockid);

    /**
     * Outgoing connection request completed event.
     *
     * It applies only to TCP sockets.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param receiver_name the name of the receiver to send the event to.
     * @param sockid unique socket ID.
     */
    void outgoing_connect_event(int		family,
				const string&	receiver_name,
				const string&	sockid);

    /**
     * Error occured event.
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param receiver_name the name of the receiver to send the event to.
     * @param sockid unique socket ID.
     * @param error a textual description of the error.
     * @param fatal indication of whether socket is shutdown because of
     * error.
     */
    void error_event(int			family,
		     const string&		receiver_name,
		     const string&		sockid,
		     const string&		error,
		     bool			fatal);

    /**
     * Connection closed by peer event.
     *
     * It applies only to TCP sockets.
     * This method is not called if the socket is gracefully closed
     * through close().
     *
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param receiver_name the name of the receiver to send the event to.
     * @param sockid unique socket ID.
     */
    void disconnect_event(int			family,
			  const string&		receiver_name,
			  const string&		sockid);

    /**
     * Inform the watcher that a component instance is alive.
     *
     * @param instance_name the name of the instance that is alive.
     */
    void instance_birth(const string& instance_name);

    /**
     * Inform the watcher that a component instance is dead.
     *
     * @param instance_name the name of the instance that is dead.
     */
    void instance_death(const string& instance_name);

    /**
     * Set the instance that is responsible for sending IP packets
     * to a receiver.
     */
    void set_io_tcpudp_manager_receiver(IoTcpUdpManagerReceiver* v) {
	_io_tcpudp_manager_receiver = v;
    }

    /**
     * Get a reference to the interface tree.
     *
     * @return a reference to the interface tree (@ref IfTree).
     */
    const IfTree&	iftree() const { return _iftree; }

    /**
     * Register @ref FeaDataPlaneManager data plane manager.
     *
     * @param fea_data_plane_manager the data plane manager to register.
     * @param is_exclusive if true, the manager is registered as the
     * exclusive manager, otherwise is added to the list of managers.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_data_plane_manager(FeaDataPlaneManager* fea_data_plane_manager,
				    bool is_exclusive);

    /**
     * Unregister @ref FeaDataPlaneManager data plane manager.
     *
     * @param fea_data_plane_manager the data plane manager to unregister.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_data_plane_manager(FeaDataPlaneManager* fea_data_plane_manager);

    /**
     * Get the list of registered data plane managers.
     *
     * @return the list of registered data plane managers.
     */
    list<FeaDataPlaneManager*>& fea_data_plane_managers() {
	return _fea_data_plane_managers;
    }

    /**
     * Connect IoTcpUdpComm entry to a new plugin.
     *
     * @param family the address family.
     * @param is_tcp if true this is TCP entry, otherwise UDP.
     * @param creator the name of the creator.
     * @param listener_sockid the socket ID of the listener socket.
     * @param peer_host the peer host IP address.
     * @param peer_port the peer host port number.
     * @param new_io_tcpudp the handler for the new connection.
     * @return an entry (@ref IoTcpUdpComm) to handle the connection
     * from the new plugin.
     */
    IoTcpUdpComm* connect_io_tcpudp_comm(int family,
					 bool is_tcp,
					 const string& creator,
					 const string& listener_sockid,
					 const IPvX& peer_host,
					 uint16_t peer_port,
					 IoTcpUdp* new_io_tcpudp);

private:
    typedef map<string, IoTcpUdpComm*> CommTable;

    /**
     * Find an IoTcpUdpComm entry.
     *
     * @param family the address family.
     * @param sockid the socket ID to search for.
     * @raturn the IoTcpUdpComm entry if found, otherwise NULL.
     */
    IoTcpUdpComm*	find_io_tcpudp_comm(int family, const string& sockid,
					    string& error_msg);
    /**
     * Create and open IoTcpUdpComm entry.
     *
     * @param family the address family.
     * @param is_tcp if true this is TCP entry, otherwise UDP.
     * @param creator the name of the creator.
     * @param allocate_plugins if true, then allocate the plugin handler(s)
     * internally, otherwise they will be explicitly added externally.
     */
    IoTcpUdpComm*	open_io_tcpudp_comm(int family, bool is_tcp,
					    const string& creator,
					    bool allocate_plugins = true);

    /**
     * Delete an existing IoTcpUdoComm entry.
     *
     * @param family the address family.
     * @param sockid the socket ID of the entry to delete.
     */
    void		delete_io_tcpudp_comm(int family,
					      const string& sockid);

    /**
     * Get the CommTable for an address family.
     *
     * @param family the address family.
     * @return a reference to the CommTable for the address family.
     */
    CommTable&		comm_table_by_family(int family);

    /**
     * Erase CommTable handlers for a given creator name.
     *
     * @param family the address family.
     * @param creator the name of the creator.
     */
    void erase_comm_handlers_by_creator(int family, const string& creator);

    /**
     * Test whether there is a CommTable handler for a given creator name.
     *
     * @param creator the name of the creator.
     * @return true if there is a CommTable handler for the given creator name,
     * otherwise false.
     */
    bool has_comm_handler_by_creator(const string& creator) const;

    /**
     * Test whether an address belongs to this host.
     *
     * @param local_addr the address to test.
     * @return true if the address belongs to this host, otherwise false.
     */
    bool		is_my_address(const IPvX& local_addr) const;

    FeaNode&		_fea_node;
    EventLoop&		_eventloop;
    const IfTree&	_iftree;

    // Collection of IP communication handlers keyed by sockid.
    CommTable		_comm_table4;
    CommTable		_comm_table6;

    IoTcpUdpManagerReceiver* _io_tcpudp_manager_receiver;

    list<FeaDataPlaneManager*> _fea_data_plane_managers;
};

#endif // __FEA_IO_TCPUDP_MANAGER_HH__
