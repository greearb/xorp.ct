// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007 International Computer Science Institute
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


#ifndef __FEA_DATA_PLANE_IO_IO_TCPUDP_DUMMY_HH__
#define __FEA_DATA_PLANE_IO_IO_TCPUDP_DUMMY_HH__


//
// I/O TCP/UDP Dummy support.
//

#include "fea/io_tcpudp.hh"


/**
 * @short A base class for I/O TCP/UDP Dummy communication.
 */
class IoTcpUdpDummy : public IoTcpUdp {
public:
    /**
     * Constructor for a given address family.
     * 
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@see FeaDataPlaneManager).
     * @param iftree the interface tree to use.
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     */
    IoTcpUdpDummy(FeaDataPlaneManager& fea_data_plane_manager,
		  const IfTree& iftree, int family);

    /**
     * Virtual destructor.
     */
    virtual ~IoTcpUdpDummy();

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
     * Open a TCP socket.
     *
     * @param is_blocking if true then the socket will be blocking, otherwise
     * non-blocking.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int tcp_open(bool is_blocking, string& error_msg);

    /**
     * Open an UDP socket.
     *
     * @param is_blocking if true then the socket will be blocking, otherwise
     * non-blocking.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int udp_open(bool is_blocking, string& error_msg);

    /**
     * Create a bound TCP socket.
     *
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param is_blocking if true then the socket will be blocking, otherwise
     * non-blocking.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int tcp_open_and_bind(const IPvX& local_addr, uint16_t local_port,
			  bool is_blocking, string& error_msg);

    /**
     * Create a bound UDP socket.
     *
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param is_blocking if true then the socket will be blocking, otherwise
     * non-blocking.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int udp_open_and_bind(const IPvX& local_addr, uint16_t local_port,
			  bool is_blocking, string& error_msg);

    /**
     * Create a bound UDP multicast socket.
     *
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param mcast_addr the multicast group address to join.
     * @param ttl the TTL to use for this multicast socket.
     * @param reuse allow other sockets to bind to same multicast group.
     * @param is_blocking if true then the socket will be blocking, otherwise
     * non-blocking.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int udp_open_bind_join(const IPvX& local_addr, uint16_t local_port,
			   const IPvX& mcast_addr, uint8_t ttl, bool reuse,
			   bool is_blocking, string& error_msg);

    /**
     * Create a bound and connected TCP socket.
     *
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param remote_addr the address to connect to.
     * @param remote_port the remote port to connect to.
     * @param is_blocking if true then the socket will be blocking, otherwise
     * non-blocking.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int tcp_open_bind_connect(const IPvX& local_addr, uint16_t local_port,
			      const IPvX& remote_addr, uint16_t remote_port,
			      bool is_blocking, string& error_msg);

    /**
     * Create a bound and connected UDP socket.
     *
     * @param local_addr the interface address to bind socket to.
     * @param local_port the port to bind socket to.
     * @param remote_addr the address to connect to.
     * @param remote_port the remote port to connect to.
     * @param is_blocking if true then the socket will be blocking, otherwise
     * non-blocking.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int udp_open_bind_connect(const IPvX& local_addr, uint16_t local_port,
			      const IPvX& remote_addr, uint16_t remote_port,
			      bool is_blocking, string& error_msg);

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
     * Send data on socket.
     *
     * @param data block of data to be sent.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int send(const vector<uint8_t>& data, string& error_msg);

    /**
     * Send data on socket with optional flags.
     *
     * These flags provide hints to the forwarding engine on how to send the
     * packets, they are not guaranteed to work.
     *
     * Note: There is no flag for "do not route" as this is always true since
     * the particular forwarding engine sending the data may not have access
     * to the full routing table.
     *
     * @param data block of data to be sent.
     * @param out_of_band mark data as out of band.
     * @param end_of_record data completes record.
     * @param end_of_file data completes file.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int send_with_flags(const vector<uint8_t>& data, bool out_of_band,
			bool end_of_record, bool end_of_file,
			string& error_msg);

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
     * Send data on socket to a given destination.
     *
     * The packet is not routed as the forwarding engine sending the packet
     * may not have access to the full routing table.
     *
     * @param remote_addr destination address for data.
     * @param remote_port destination port for data.
     * @param data block of data to be sent.
     * @param out_of_band mark data as out of band.
     * @param end_of_record data completes record.
     * @param end_of_file data completes file.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int send_to_with_flags(const IPvX& remote_addr, uint16_t remote_port,
			   const vector<uint8_t>& data, bool out_of_band,
			   bool end_of_record, bool end_of_file,
			   string& error_msg);

    /**
     * Send data on socket to a given multicast group from a given interface.
     *
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
     * Set a named socket option.
     *
     * @param optname name of option to be set. Valid values are:
     * "multicast_loopback" "multicast_ttl"
     * @param optval value of option to be set. If value is logically boolean
     * then zero represents false and any non-zero value true.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_socket_option(const string& optname, uint32_t optval,
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

private:
};

#endif // __FEA_DATA_PLANE_IO_IO_TCPUDP_DUMMY_HH__
