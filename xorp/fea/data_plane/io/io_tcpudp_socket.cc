// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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



//
// I/O TCP/UDP communication support.
//
// The mechanism is UNIX TCP/UDP sockets.
//

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif

#include "libcomm/comm_api.h"
#include "fea/iftree.hh"
#include "io_tcpudp_socket.hh"


#ifdef HAVE_TCPUDP_UNIX_SOCKETS

static const string&
find_ifname_by_addr(const IfTree& iftree, const IPvX& local_addr,
		    string& error_msg)
{
    const IfTreeInterface* ifp = NULL;
    const IfTreeVif* vifp = NULL;
    static string empty;

    // Find the physical interface index
    if (iftree.find_interface_vif_by_addr(local_addr, ifp, vifp) != true) {
	error_msg = c_format("Local IP address %s was not found",
			     local_addr.str().c_str());
	return empty;
    }

    return vifp->ifname();
}

/**
 * Find the physical interface index for an interface for a given local
 * address.
 *
 * @param iftree the interface tree to use.
 * @param local_addr the local address to search for.
 * @param error_msg the error message (if error).
 * @return the physical interface index on success, otherwise 0.
 */
#ifdef HAVE_IPV6
// XXX: For now the function is used only by the IPv6 code
static uint32_t
find_pif_index_by_addr(const IfTree& iftree, const IPvX& local_addr,
		       string& error_msg)
{
    const IfTreeInterface* ifp = NULL;
    const IfTreeVif* vifp = NULL;
    uint32_t pif_index = 0;

    // Find the physical interface index
    if (iftree.find_interface_vif_by_addr(local_addr, ifp, vifp) != true) {
	error_msg = c_format("Local IP address %s was not found",
			     local_addr.str().c_str());
	return 0;
    }

    pif_index = vifp->pif_index();
    if (pif_index == 0) {
	error_msg = c_format("Could not find physical interface index for "
			     "IP address %s",
			     local_addr.str().c_str());
	return 0;
    }

    return pif_index;
}

static uint32_t
find_pif_index_by_name(const IfTree& iftree, const string& ifname, string& error_msg)
{
    const IfTreeVif* vifp = NULL;
    uint32_t pif_index = 0;

    // Find the physical interface index
    vifp = iftree.find_vif(ifname, ifname);
    if (!vifp) {
	error_msg = c_format("VIF %s was not found", ifname.c_str());
	return 0;
    }

    pif_index = vifp->pif_index();
    if (pif_index == 0) {
	error_msg = c_format("Could not find physical interface index for "
			     "dev %s",
			     ifname.c_str());
	return 0;
    }

    return pif_index;
}

int find_best_pif_idx(const IfTree& iftree, const string& ifname,
		      const IPvX& local_addr, string& error_msg, uint32_t& pif_index)
{
    // Find the physical interface index for link-local addresses
    if (ifname.size()) {
	pif_index = find_pif_index_by_name(iftree, ifname, error_msg);
    }
    if ((pif_index == 0) && local_addr.is_linklocal_unicast()) {
	pif_index = find_pif_index_by_addr(iftree, local_addr, error_msg);
	if (pif_index == 0) {
	    return XORP_ERROR;
	}
    }
    return XORP_OK;
}
#endif // HAVE_IPV6

/**
 * Extract the port number from struct sockaddr_storage.
 *
 * @return the port number (in host order).
 */
static uint16_t
get_sockadr_storage_port_number(const struct sockaddr_storage& ss)
{
    uint16_t port = 0;

    switch (ss.ss_family) {
    case AF_INET:
    {
	const struct sockaddr* sa = sockaddr_storage2sockaddr(&ss);
	const struct sockaddr_in* sin = sockaddr2sockaddr_in(sa);
	port = ntohs(sin->sin_port);
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	const struct sockaddr* sa = sockaddr_storage2sockaddr(&ss);
	const struct sockaddr_in6* sin6 = sockaddr2sockaddr_in6(sa);
	port = ntohs(sin6->sin6_port);
	break;
    }
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	return (port);
    }

    return (port);
}

IoTcpUdpSocket::IoTcpUdpSocket(FeaDataPlaneManager& fea_data_plane_manager,
			       const IfTree& iftree, int family,
			       bool is_tcp)
    : IoTcpUdp(fea_data_plane_manager, iftree, family, is_tcp),
      _peer_address(IPvX::ZERO(family)),
      _peer_port(0),
      _async_writer(NULL)
{
}

IoTcpUdpSocket::~IoTcpUdpSocket()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the I/O TCP/UDP UNIX socket mechanism: %s",
		   error_msg.c_str());
    }
}

int
IoTcpUdpSocket::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}

int
IoTcpUdpSocket::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (_socket_fd.is_valid()) {
	if (close(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    _is_running = false;

    return (XORP_OK);
}

int
IoTcpUdpSocket::enable_recv_pktinfo(bool is_enabled, string& error_msg)
{
    switch (family()) {
    case AF_INET:
    {
	// XXX: the setsockopt() argument must be 'int'
	int bool_flag = is_enabled;
	
	//
	// Interface index
	//
#ifdef IP_RECVIF
	// XXX: BSD
	if (setsockopt(_socket_fd, IPPROTO_IP, IP_RECVIF,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    XLOG_ERROR("setsockopt(IP_RECVIF, %u) failed: %s",
		       bool_flag, XSTRERROR);
	    return (XORP_ERROR);
	}
#endif // IP_RECVIF

#ifdef IP_PKTINFO
	// XXX: Linux
	if (setsockopt(_socket_fd, IPPROTO_IP, IP_PKTINFO,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    XLOG_ERROR("setsockopt(IP_PKTINFO, %u) failed: %s",
		       bool_flag, XSTRERROR);
	    return (XORP_ERROR);
	}
#endif // IP_PKTINFO

	UNUSED(bool_flag);
	break;
    }

#ifdef HAVE_IPV6
    case AF_INET6:
    {
	// XXX: the setsockopt() argument must be 'int'
	int bool_flag = is_enabled;
	
	//
	// Interface index and address
	//
#ifdef IPV6_RECVPKTINFO
	// The new option (applies to receiving only)
	if (setsockopt(_socket_fd, IPPROTO_IPV6, IPV6_RECVPKTINFO,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    error_msg = c_format("setsockopt(IPV6_RECVPKTINFO, %u) failed: %s",
				 bool_flag, XSTRERROR);
	    return (XORP_ERROR);
	}
#else
	// The old option (see RFC-2292)
	if (setsockopt(_socket_fd, IPPROTO_IPV6, IPV6_PKTINFO,
		       XORP_SOCKOPT_CAST(&bool_flag), sizeof(bool_flag)) < 0) {
	    error_msg = c_format("setsockopt(IPV6_PKTINFO, %u) failed: %s",
				 bool_flag, XSTRERROR);
	    return (XORP_ERROR);
	}
#endif // ! IPV6_RECVPKTINFO
	
    }
    break;
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	error_msg = c_format("Invalid address family %d", family());
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

int
IoTcpUdpSocket::tcp_open(string& error_msg)
{
    if (_socket_fd.is_valid()) {
	error_msg = c_format("The socket is already open");
	return (XORP_ERROR);
    }
	
    _socket_fd = comm_open_tcp(family(), COMM_SOCK_NONBLOCKING);
    if (! _socket_fd.is_valid()) {
	error_msg = c_format("Cannot open the socket: %s",
			     comm_get_last_error_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IoTcpUdpSocket::udp_open(string& error_msg)
{
    if (_socket_fd.is_valid()) {
	error_msg = c_format("The socket is already open");
	return (XORP_ERROR);
    }
	
    _socket_fd = comm_open_udp(family(), COMM_SOCK_NONBLOCKING);
    if (! _socket_fd.is_valid()) {
	error_msg = c_format("Cannot open the socket: %s",
			     comm_get_last_error_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IoTcpUdpSocket::tcp_open_and_bind(const IPvX& local_addr, uint16_t local_port,
				  string& error_msg)
{
    XLOG_ASSERT(family() == local_addr.af());

    if (_socket_fd.is_valid()) {
	error_msg = c_format("The socket is already open");
	return (XORP_ERROR);
    }

    string local_dev = find_ifname_by_addr(iftree(), local_addr, error_msg);
    switch (family()) {
    case AF_INET:
    {
	struct in_addr local_in_addr;

	local_addr.copy_out(local_in_addr);
	_socket_fd = comm_bind_tcp4(&local_in_addr, htons(local_port),
				    COMM_SOCK_NONBLOCKING, local_dev.c_str());
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	struct in6_addr local_in6_addr;
	uint32_t pif_index = 0;

	// Find the physical interface index for link-local addresses
	if (local_addr.is_linklocal_unicast()) {
	    pif_index = find_pif_index_by_addr(iftree(), local_addr,
					       error_msg);
	    if (pif_index == 0)
		return (XORP_ERROR);
	}

	local_addr.copy_out(local_in6_addr);
	_socket_fd = comm_bind_tcp6(&local_in6_addr, pif_index,
				    htons(local_port),
				    COMM_SOCK_NONBLOCKING, local_dev.c_str());
	break;
    }
#endif // HAVE_IPV6
    default:
	error_msg = c_format("Address family %d is not supported", family());
	return (XORP_ERROR);
    }

    if (! _socket_fd.is_valid()) {
	error_msg = c_format("Cannot open and bind the socket: %s",
			     comm_get_last_error_str());
	return (XORP_ERROR);
    }

    //
    // XXX: Don't enable receiving of data (yet).
    // This will happen after we start listen() on the socket, and
    // a new connection request is accept()-ed and allowed by the receiver.
    //

    return (XORP_OK);
}

int
IoTcpUdpSocket::udp_open_and_bind(const IPvX& local_addr, uint16_t local_port, const string& local_dev,
				  int reuse_addr, string& error_msg)
{
    XLOG_ASSERT(family() == local_addr.af());

    if (_socket_fd.is_valid()) {
	error_msg = c_format("The socket is already open");
	return (XORP_ERROR);
    }

    switch (family()) {
    case AF_INET:
    {
	struct in_addr local_in_addr;

	local_addr.copy_out(local_in_addr);
	_socket_fd = comm_bind_udp4(&local_in_addr, htons(local_port),
				    COMM_SOCK_NONBLOCKING, reuse_addr, local_dev.c_str());
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	struct in6_addr local_in6_addr;
	uint32_t pif_index = 0;

	if (find_best_pif_idx(iftree(), local_dev, local_addr, error_msg, pif_index) == XORP_ERROR)
	    return XORP_ERROR;

	local_addr.copy_out(local_in6_addr);
	_socket_fd = comm_bind_udp6(&local_in6_addr, pif_index,
				    htons(local_port),
				    COMM_SOCK_NONBLOCKING, local_dev.c_str());
	break;
    }
#endif // HAVE_IPV6
    default:
	error_msg = c_format("Address family %d is not supported", family());
	return (XORP_ERROR);
    }

    if (! _socket_fd.is_valid()) {
	error_msg = c_format("Cannot open and bind the socket: %s",
			     comm_get_last_error_str());
	return (XORP_ERROR);
    }

    return (enable_data_recv(error_msg));
}

int
IoTcpUdpSocket::udp_open_bind_join(const IPvX& local_addr, uint16_t local_port,
				   const IPvX& mcast_addr, uint8_t ttl,
				   bool reuse, string& error_msg)
{
    XLOG_ASSERT(family() == local_addr.af());
    XLOG_ASSERT(family() == mcast_addr.af());

    if (_socket_fd.is_valid()) {
	error_msg = c_format("The socket is already open");
	return (XORP_ERROR);
    }

    string local_dev = find_ifname_by_addr(iftree(), local_addr, error_msg);
    switch (family()) {
    case AF_INET:
    {
	struct in_addr local_in_addr, mcast_in_addr;

	local_addr.copy_out(local_in_addr);
	mcast_addr.copy_out(mcast_in_addr);
	_socket_fd = comm_bind_join_udp4(&mcast_in_addr, &local_in_addr,
					 htons(local_port), reuse,
					 COMM_SOCK_NONBLOCKING, local_dev.c_str());
	if (! _socket_fd.is_valid()) {
	    error_msg = c_format("Cannot open, bind and join the socket: %s",
				 comm_get_last_error_str());
	    return (XORP_ERROR);
	}

	// Set the default interface for outgoing multicast
	if (comm_set_iface4(_socket_fd, &local_in_addr) != XORP_OK) {
	    error_msg = c_format("Cannot set the default multicast interface: "
				 "%s",
				 comm_get_last_error_str());
	    comm_close(_socket_fd);
	    _socket_fd.clear();
	    return (XORP_ERROR);
	}

	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	struct in6_addr mcast_in6_addr;
	uint32_t pif_index = 0;

	// Find the physical interface index
	pif_index = find_pif_index_by_addr(iftree(), local_addr, error_msg);
	if (pif_index == 0)
	    return (XORP_ERROR);

	mcast_addr.copy_out(mcast_in6_addr);
	_socket_fd = comm_bind_join_udp6(&mcast_in6_addr, pif_index,
					 htons(local_port), reuse,
					 COMM_SOCK_NONBLOCKING, local_dev.c_str());

	if (! _socket_fd.is_valid()) {
	    error_msg = c_format("Cannot open, bind and join the socket: %s",
				 comm_get_last_error_str());
	    return (XORP_ERROR);
	}

	// Set the default interface for outgoing multicast
	if (comm_set_iface6(_socket_fd, pif_index) != XORP_OK) {
	    error_msg = c_format("Cannot set the default multicast interface: "
				 "%s",
				 comm_get_last_error_str());
	    comm_close(_socket_fd);
	    _socket_fd.clear();
	    return (XORP_ERROR);
	}

	break;
    }
#endif // HAVE_IPV6
    default:
	error_msg = c_format("Address family %d is not supported", family());
	return (XORP_ERROR);
    }

    if (comm_set_multicast_ttl(_socket_fd, ttl) != XORP_OK) {
	error_msg = c_format("Cannot set the multicast TTL: %s",
			     comm_get_last_error_str());
	comm_close(_socket_fd);
	_socket_fd.clear();
	return (XORP_ERROR);
    }
    if (comm_set_loopback(_socket_fd, 0) != XORP_OK) {
	error_msg = c_format("Cannot disable multicast loopback: %s",
			     comm_get_last_error_str());
	comm_close(_socket_fd);
	_socket_fd.clear();
	return (XORP_ERROR);
    }

    return (enable_data_recv(error_msg));
}

int
IoTcpUdpSocket::tcp_open_bind_connect(const IPvX& local_addr,
				      uint16_t local_port,
				      const IPvX& remote_addr,
				      uint16_t remote_port,
				      string& error_msg)
{
    int in_progress = 0;

    XLOG_ASSERT(family() == local_addr.af());
    XLOG_ASSERT(family() == remote_addr.af());

    if (_socket_fd.is_valid()) {
	error_msg = c_format("The socket is already open");
	return (XORP_ERROR);
    }

    string local_dev = find_ifname_by_addr(iftree(), local_addr, error_msg);
    switch (family()) {
    case AF_INET:
    {
	struct in_addr local_in_addr, remote_in_addr;

	local_addr.copy_out(local_in_addr);
	remote_addr.copy_out(remote_in_addr);
	_socket_fd = comm_bind_connect_tcp4(&local_in_addr, htons(local_port),
					    &remote_in_addr,
					    htons(remote_port),
					    COMM_SOCK_NONBLOCKING,
					    &in_progress, local_dev.c_str());
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	struct in6_addr local_in6_addr, remote_in6_addr;
	uint32_t pif_index = 0;

	// Find the physical interface index for link-local addresses
	if (local_addr.is_linklocal_unicast()) {
	    pif_index = find_pif_index_by_addr(iftree(), local_addr,
					       error_msg);
	    if (pif_index == 0)
		return (XORP_ERROR);
	}

	local_addr.copy_out(local_in6_addr);
	remote_addr.copy_out(remote_in6_addr);
	_socket_fd = comm_bind_connect_tcp6(&local_in6_addr, pif_index,
					    htons(local_port),
					    &remote_in6_addr,
					    htons(remote_port),
					    COMM_SOCK_NONBLOCKING,
					    &in_progress, local_dev.c_str());
	break;
    }
#endif // HAVE_IPV6
    default:
	error_msg = c_format("Address family %d is not supported", family());
	return (XORP_ERROR);
    }

    if (! _socket_fd.is_valid()) {
	error_msg = c_format("Cannot open, bind and connect the socket: %s",
			     comm_get_last_error_str());
	return (XORP_ERROR);
    }

    // Add the socket to the eventloop
    if (eventloop().add_ioevent_cb(_socket_fd, IOT_CONNECT,
				   callback(this, &IoTcpUdpSocket::connect_io_cb))
	!= true) {
	error_msg = c_format("Failed to add I/O callback to complete outgoing connection");
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IoTcpUdpSocket::udp_open_bind_connect(const IPvX& local_addr,
				      uint16_t local_port,
				      const IPvX& remote_addr,
				      uint16_t remote_port,
				      string& error_msg)
{
    int in_progress = 0;

    XLOG_ASSERT(family() == local_addr.af());
    XLOG_ASSERT(family() == remote_addr.af());

    if (_socket_fd.is_valid()) {
	error_msg = c_format("The socket is already open");
	return (XORP_ERROR);
    }

    string local_dev = find_ifname_by_addr(iftree(), local_addr, error_msg);
    switch (family()) {
    case AF_INET:
    {
	struct in_addr local_in_addr, remote_in_addr;

	local_addr.copy_out(local_in_addr);
	remote_addr.copy_out(remote_in_addr);
	_socket_fd = comm_bind_connect_udp4(&local_in_addr, htons(local_port),
					    &remote_in_addr,
					    htons(remote_port),
					    COMM_SOCK_NONBLOCKING,
					    &in_progress, local_dev.c_str());
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	struct in6_addr local_in6_addr, remote_in6_addr;
	uint32_t pif_index = 0;

	// Find the physical interface index for link-local addresses
	if (local_addr.is_linklocal_unicast()) {
	    pif_index = find_pif_index_by_addr(iftree(), local_addr,
					       error_msg);
	    if (pif_index == 0)
		return (XORP_ERROR);
	}

	local_addr.copy_out(local_in6_addr);
	remote_addr.copy_out(remote_in6_addr);
	_socket_fd = comm_bind_connect_udp6(&local_in6_addr, pif_index,
					    htons(local_port),
					    &remote_in6_addr,
					    htons(remote_port),
					    COMM_SOCK_NONBLOCKING,
					    &in_progress, local_dev.c_str());
	break;
    }
#endif // HAVE_IPV6
    default:
	error_msg = c_format("Address family %d is not supported", family());
	return (XORP_ERROR);
    }

    if (! _socket_fd.is_valid()) {
	error_msg = c_format("Cannot open, bind and connect the socket: %s",
			     comm_get_last_error_str());
	return (XORP_ERROR);
    }

    return (enable_data_recv(error_msg));
}

int
IoTcpUdpSocket::udp_open_bind_broadcast(const string& ifname,
				        const string& vifname,
				        uint16_t local_port,
				        uint16_t remote_port,
				        bool reuse,
				        bool limited,
                                        bool connected,
				        string& error_msg)
{
    int ret_value = XORP_OK;

    if (_socket_fd.is_valid()) {
	error_msg = c_format("The socket is already open");
	return (XORP_ERROR);
    }

    // 0. Before doing anything else, we need to look up the first
    //    configured network broadcast address on ifname/vifname.
    //    If there is no IPv4 stack configured on that node, then
    //    we need to reject this request outright.
    const IfTreeInterface* ifp;
    const IfTreeVif* vifp;

    ifp = iftree().find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("No interface %s", ifname.c_str());
	return (XORP_ERROR);
    }
    vifp = ifp->find_vif(vifname);
    if (vifp == NULL) {
	error_msg = c_format("No interface %s vif %s",
			     ifname.c_str(), vifname.c_str());
	return (XORP_ERROR);
    }
    if (! ifp->enabled()) {
	error_msg = c_format("Interface %s is down",
			     ifp->ifname().c_str());
	return (XORP_ERROR);
    }
    if (! vifp->enabled()) {
	error_msg = c_format("Interface %s vif %s is down",
			     ifp->ifname().c_str(),
			     vifp->vifname().c_str());
	return (XORP_ERROR);
    }
    if (! vifp->broadcast()) {
	error_msg = c_format("Interface %s vif %s is not broadcast capable",
			     ifp->ifname().c_str(),
			     vifp->vifname().c_str());
	return (XORP_ERROR);
    }

    // Find the first IPv4 broadcast address configured on vif.
    bool is_found = false;
    for (IfTreeVif::IPv4Map::const_iterator ai = vifp->ipv4addrs().begin();
	 ai != vifp->ipv4addrs().end();
	 ++ai) {
	IfTreeAddr4& ar = *(ai->second);
	if (ar.enabled() && ar.broadcast()) {
	    _network_broadcast_address = ar.bcast();
	    is_found = true;
	    break;
        }
    }
    if (! is_found) {
	error_msg = c_format("Interface %s vif %s has no configured "
			     "IPv4 network broadcast address",
			     ifp->ifname().c_str(),
			     vifp->vifname().c_str());
	return (XORP_ERROR);
    }

    // 1. Open a UDP socket.
    _socket_fd = comm_open_udp(family(), COMM_SOCK_NONBLOCKING);
    if (! _socket_fd.is_valid()) {
	error_msg = c_format("Cannot open the socket: %s",
			     comm_get_last_error_str());
	return (XORP_ERROR);
    }

    // 2a. On BSD derived systems, request port re-use (SO_REUSEPORT).
    //     This is a no-op on systems which do not have it,
    //     and harmless on systems which do not require it.
    if (reuse) {
        ret_value = comm_set_reuseport(_socket_fd, 1);
	if (ret_value != XORP_OK) {
	    error_msg = c_format("Cannot enable port re-use: %s",
				 comm_get_last_error_str());
	    return (XORP_ERROR);
	}
        ret_value = comm_set_reuseaddr(_socket_fd, 1);
	if (ret_value != XORP_OK) {
	    error_msg = c_format("Cannot enable address re-use: %s",
				 comm_get_last_error_str());
	    return (XORP_ERROR);
	}
    }

    // 3. Bind to address and port (bind()).
    //    On BSD derived systems, if an interface address is specified
    //    for the bind(), received broadcast datagrams will NOT be delivered
    //    to the socket. This behaviour is so old, it's taken for granted,
    //    although it could be argued it's buggy; therefore on such systems
    //    INADDR_ANY is used.
    // This also takes care of SO_BINDTODEVICE if platform supports it.
    struct in_addr local_in_addr;
    local_in_addr.s_addr = INADDR_ANY;
    ret_value = comm_sock_bind4(_socket_fd, &local_in_addr,
			        htons(local_port), vifname.c_str());
    if (ret_value != XORP_OK) {
	error_msg = c_format("Cannot bind the broadcast socket: %s",
			     comm_get_last_error_str());
	return (XORP_ERROR);
    }

    //  4. Set outgoing ttl to 1 (IP_TTL).
    //     This is the default in order to prevent packet storms.
    if (comm_unicast_ttl_present() == XORP_OK) {
	ret_value = comm_set_unicast_ttl(_socket_fd, 1);
	if (ret_value != XORP_OK) {
	    error_msg = c_format("Cannot set TTL: %s",
				 comm_get_last_error_str());
	    return (XORP_ERROR);
	}
    }

    //  5a. Enable socket for broadcast send (SO_BROADCAST).
    //      Most implementations require this.
    ret_value = comm_set_send_broadcast(_socket_fd, 1);
    if (ret_value != XORP_OK) {
	error_msg = c_format("Cannot enable broadcast sends: %s",
		             comm_get_last_error_str());
	return (XORP_ERROR);
    }

#ifdef notyet
    //  5b. On Windows Server 2003 and up, IP_RECEIVE_BROADCAST also
    //      needs to be set to receive broadcast datagrams.
    if (comm_receive_broadcast_present() == XORP_OK) {
	ret_value = comm_set_receive_broadcast(_socket_fd, 1);
	if (ret_value != XORP_OK) {
	    error_msg = c_format("Cannot enable broadcast receives: %s",
				comm_get_last_error_str());
	    return (XORP_ERROR);
	}
    }
#endif

    //  6. On BSD derived systems, if we are going to send to
    //     the limited broadcast address, request IP_ONESBCAST.
    //     This rewrites the network broadcast address to the
    //     limited broadcast address on each send.
    //
    //     However we need to record the network address in
    //     use at the time of binding. This has the unfortunate
    //     side-effect that if the configured network broadcast
    //     address on the underlying ifnet changes, the mapping
    //     will break.
    //
    //     [It also means that if we need to connect() the socket
    //      to bind its faddr tuple in the inpcb, we need to specify
    //      the network broadcast address, NOT the limited broadcast
    //      address.]
    if (limited) {
        if (comm_onesbcast_present() == XORP_OK) {
	    ret_value = comm_set_onesbcast(_socket_fd, 1);
	    if (ret_value != XORP_OK) {
		error_msg = c_format("Cannot enable IP_ONESBCAST: %s",
				    comm_get_last_error_str());
		return (XORP_ERROR);
	    }
	    _limited_broadcast_enabled = true;
	    debug_msg("enabled onesbcast on fd %s\n",
	    	      _socket_fd.str().c_str());
	    //XLOG_WARNING("enabled onesbcast on fd %s\n",
	    //	      _socket_fd.str().c_str());
        }
    }

    // Finally, if the creator requested a connected broadcast socket,
    // make sure the socket is connected to the appropriate address.
    if (connected) {
	struct in_addr remote_in_addr;
	int in_progress = 0;
        if (limited) {
            if (comm_onesbcast_present() == XORP_OK &&
                _limited_broadcast_enabled) {
		// IP_ONESBCAST platform.
		// connect() to network broadcast address, but ensure
		// translation is performed for sendto().
		_network_broadcast_address.copy_out(remote_in_addr);
            } else {
		// Not an IP_ONESBCAST platform.
		// connect() to limited broadcast address.
		// XXX This is untested on Windows.
		IPv4::ALL_ONES().copy_out(remote_in_addr);
            }
        } else {
	    // connect() to network broadcast address.
	    _network_broadcast_address.copy_out(remote_in_addr);
        }
	ret_value = comm_sock_connect4(_socket_fd,
				    &remote_in_addr,
				    htons(remote_port),
				    COMM_SOCK_NONBLOCKING,
				    &in_progress);
	if (ret_value != XORP_OK) {
	    error_msg = c_format("Cannot connect the broadcast socket: %s",
				comm_get_last_error_str());
	    return (XORP_ERROR);
	}
    }

    return (enable_data_recv(error_msg));
}

int
IoTcpUdpSocket::bind(const IPvX& local_addr, uint16_t local_port,
		     string& error_msg)
{
    int ret_value = XORP_OK;

    XLOG_ASSERT(family() == local_addr.af());

    if (! _socket_fd.is_valid()) {
	error_msg = c_format("The socket is not open");
	return (XORP_ERROR);
    }

    string local_dev = find_ifname_by_addr(iftree(), local_addr, error_msg);
    switch (family()) {
    case AF_INET:
    {
	struct in_addr local_in_addr;

	local_addr.copy_out(local_in_addr);
	ret_value = comm_sock_bind4(_socket_fd, &local_in_addr,
				    htons(local_port), local_dev.c_str());
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	struct in6_addr local_in6_addr;
	uint32_t pif_index = 0;

	// Find the physical interface index for link-local addresses
	if (local_addr.is_linklocal_unicast()) {
	    pif_index = find_pif_index_by_addr(iftree(), local_addr,
					       error_msg);
	    if (pif_index == 0)
		return (XORP_ERROR);
	}

	local_addr.copy_out(local_in6_addr);
	ret_value = comm_sock_bind6(_socket_fd, &local_in6_addr, pif_index,
				    htons(local_port), local_dev.c_str());
	break;
    }
#endif // HAVE_IPV6
    default:
	error_msg = c_format("Address family %d is not supported", family());
	return (XORP_ERROR);
    }

    if (ret_value != XORP_OK) {
	error_msg = c_format("Cannot bind the socket: %s",
			     comm_get_last_error_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IoTcpUdpSocket::udp_join_group(const IPvX& mcast_addr,
			       const IPvX& join_if_addr,
			       string& error_msg)
{
    int ret_value = XORP_OK;

    XLOG_ASSERT(family() == mcast_addr.af());
    XLOG_ASSERT(family() == join_if_addr.af());

    if (! _socket_fd.is_valid()) {
	error_msg = c_format("The socket is not open");
	return (XORP_ERROR);
    }

    switch (family()) {
    case AF_INET:
    {
	struct in_addr local_in_addr, mcast_in_addr;

	join_if_addr.copy_out(local_in_addr);
	mcast_addr.copy_out(mcast_in_addr);
	
	ret_value = comm_sock_join4(_socket_fd, &mcast_in_addr,
				    &local_in_addr);
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	struct in6_addr  mcast_in6_addr;
	uint32_t pif_index = 0;

	// Find the physical interface index
	pif_index = find_pif_index_by_addr(iftree(), join_if_addr, error_msg);
	if (pif_index == 0)
	    return (XORP_ERROR);

	mcast_addr.copy_out(mcast_in6_addr);
	ret_value = comm_sock_join6(_socket_fd, &mcast_in6_addr, pif_index);
	break;
    }
#endif // HAVE_IPV6
    default:
	error_msg = c_format("Address family %d is not supported", family());
	return (XORP_ERROR);
    }

    if (ret_value != XORP_OK) {
	error_msg = c_format("Cannot join on the socket: %s",
			     comm_get_last_error_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IoTcpUdpSocket::udp_leave_group(const IPvX& mcast_addr,
				const IPvX& leave_if_addr,
				string& error_msg)
{
    int ret_value = XORP_OK;

    XLOG_ASSERT(family() == mcast_addr.af());
    XLOG_ASSERT(family() == leave_if_addr.af());

    if (! _socket_fd.is_valid()) {
	error_msg = c_format("The socket is not open");
	return (XORP_ERROR);
    }

    switch (family()) {
    case AF_INET:
    {
	struct in_addr local_in_addr, mcast_in_addr;

	leave_if_addr.copy_out(local_in_addr);
	mcast_addr.copy_out(mcast_in_addr);
	
	ret_value = comm_sock_leave4(_socket_fd, &mcast_in_addr,
				     &local_in_addr);
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	struct in6_addr  mcast_in6_addr;
	uint32_t pif_index = 0;

	// Find the physical interface index
	pif_index = find_pif_index_by_addr(iftree(), leave_if_addr, error_msg);
	if (pif_index == 0)
	    return (XORP_ERROR);

	mcast_addr.copy_out(mcast_in6_addr);
	ret_value = comm_sock_leave6(_socket_fd, &mcast_in6_addr, pif_index);
	break;
    }
#endif // HAVE_IPV6
    default:
	error_msg = c_format("Address family %d is not supported", family());
	return (XORP_ERROR);
    }

    if (ret_value != XORP_OK) {
	error_msg = c_format("Cannot leave on the socket: %s",
			     comm_get_last_error_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IoTcpUdpSocket::close(string& error_msg)
{
    if (! _socket_fd.is_valid()) {
	error_msg = c_format("The socket is not open");
	return (XORP_ERROR);
    }

    // Remove it just in case, even though it may not be select()-ed
    eventloop().remove_ioevent_cb(_socket_fd);

    // Delete the async writer
    if (_async_writer != NULL) {
	_async_writer->stop();
	_async_writer->flush_buffers();
	delete _async_writer;
	_async_writer = NULL;
    }

    if (comm_close(_socket_fd) != XORP_OK) {
	error_msg = c_format("Cannot close the socket: %s",
			     comm_get_last_error_str());
	return (XORP_ERROR);
    }
    _socket_fd.clear();

    return (XORP_OK);
}

int
IoTcpUdpSocket::tcp_listen(uint32_t backlog, string& error_msg)
{
    if (! _socket_fd.is_valid()) {
	error_msg = c_format("The socket is not open");
	return (XORP_ERROR);
    }

    if (comm_listen(_socket_fd, backlog) != XORP_OK) {
	error_msg = c_format("Cannot listen to the socket: %s",
			     comm_get_last_error_str());
	return (XORP_ERROR);
    }

    // Add the socket to the eventloop
    if (eventloop().add_ioevent_cb(_socket_fd, IOT_ACCEPT,
				   callback(this, &IoTcpUdpSocket::accept_io_cb))
	!= true) {
	error_msg = c_format("Failed to add I/O callback to accept connections");
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IoTcpUdpSocket::udp_enable_recv(string& error_msg)
{
    return (enable_data_recv(error_msg));
}

int
IoTcpUdpSocket::send(const vector<uint8_t>& data, string& error_msg)
{
    if (! _socket_fd.is_valid()) {
	error_msg = c_format("The socket is not open");
	return (XORP_ERROR);
    }

    // Allocate the async writer
    if (_async_writer == NULL) {
	//
	// XXX: Don't coalesce the buffers.
	// Note that we shouldn't coalesce for UDP, because it might break
	// the semantics of protocol control packets that use UDP.
	// We don't coalesce for TCP as well, but this could be changed in the
	// future if it improves performance.
	//
	int coalesce_buffers_n = 1;
	_async_writer = new AsyncFileWriter(eventloop(), _socket_fd,
					    coalesce_buffers_n);
    }

    // Queue the data for transmission
    _async_writer->add_data(data, callback(this, &IoTcpUdpSocket::send_completed_cb));
    _async_writer->start();

    return (XORP_OK);
}

int
IoTcpUdpSocket::send_to(const IPvX& remote_addr, uint16_t remote_port,
			const vector<uint8_t>& data, string& error_msg)
{
    XLOG_ASSERT(family() == remote_addr.af());

    if (! _socket_fd.is_valid()) {
	error_msg = c_format("The socket is not open");
	return (XORP_ERROR);
    }

    // Allocate the async writer
    if (_async_writer == NULL) {
	//
	// XXX: Don't coalesce the buffers.
	// Note that we shouldn't coalesce for UDP, because it might break
	// the semantics of protocol control packets that use UDP.
	// We don't coalesce for TCP as well, but this could be changed in the
	// future if it improves performance.
	//
	int coalesce_buffers_n = 1;
	_async_writer = new AsyncFileWriter(eventloop(), _socket_fd,
					    coalesce_buffers_n);
    }

    // Queue the data for transmission.
    if (_limited_broadcast_enabled &&
        comm_onesbcast_present() == XORP_OK &&
        remote_addr == IPv4::ALL_ONES()) {
	// If this is an IPv4 limited broadcast socket on a platform which
	// uses the IP_ONESBCAST socket option, we must trap sends to
	// the limited broadcast address, and rewrite them to use the
	// network broadcast address.
	debug_msg("onesbcast enabled on fd %s, rewriting %s to %s.\n",
		  _socket_fd.str().c_str(),
		  remote_addr.str().c_str(),
                 _network_broadcast_address.str().c_str());
	//XLOG_WARNING("onesbcast enabled on fd %s, rewriting %s to %s.\n",
	//	  _socket_fd.str().c_str(),
	//	  remote_addr.str().c_str(),
        //          _network_broadcast_address.str().c_str());
	_async_writer->add_data_sendto(data,
                                       _network_broadcast_address,
				       remote_port,
				       callback(this, &IoTcpUdpSocket::send_completed_cb));
    } else {
	_async_writer->add_data_sendto(data,
				       remote_addr,
				       remote_port,
				       callback(this, &IoTcpUdpSocket::send_completed_cb));
    }
    _async_writer->start();

    return (XORP_OK);
}

void
IoTcpUdpSocket::send_completed_cb(AsyncFileWriter::Event	event,
				  const uint8_t*		buffer,
				  size_t			buffer_bytes,
				  size_t			offset)
{
    string error_msg;

    UNUSED(buffer);
    UNUSED(buffer_bytes);
    UNUSED(offset);

    switch (event) {
    case AsyncFileWriter::DATA:
	// I/O occured
	XLOG_ASSERT(offset <= buffer_bytes);
	break;
    case AsyncFileWriter::FLUSHING:
	// Buffer is being flushed
	break;
    case AsyncFileWriter::OS_ERROR:
	// I/O error has occured
	error_msg = c_format("Failed to send data: Unknown I/O error");
	if (io_tcpudp_receiver() != NULL)
	    io_tcpudp_receiver()->error_event(error_msg, true);
	break;
    case AsyncFileWriter::END_OF_FILE:
	// End of file reached (applies to read only)
	XLOG_UNREACHABLE();
	break;
    case AsyncFileWriter::WOULDBLOCK:
	// I/O would block the current thread
	break;
    }
}

int
IoTcpUdpSocket::send_from_multicast_if(const IPvX& group_addr,
				       uint16_t group_port,
				       const IPvX& ifaddr,
				       const vector<uint8_t>& data,
				       string& error_msg)
{
    int ret_value = XORP_OK;

    XLOG_ASSERT(family() == group_addr.af());
    XLOG_ASSERT(family() == ifaddr.af());

    if (! _socket_fd.is_valid()) {
	error_msg = c_format("The socket is not open");
	return (XORP_ERROR);
    }

    switch (family()) {
    case AF_INET:
    {
	struct in_addr ifaddr_in_addr;

	ifaddr.copy_out(ifaddr_in_addr);
	// TODO:  Not needed if we are binding to specific ports.
	ret_value = comm_set_iface4(_socket_fd, &ifaddr_in_addr);
	break;
    }
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	uint32_t pif_index = 0;

	// Find the physical interface index
	pif_index = find_pif_index_by_addr(iftree(), ifaddr, error_msg);
	if (pif_index == 0)
	    return (XORP_ERROR);

	ret_value = comm_set_iface6(_socket_fd, pif_index);
	break;
    }
#endif // HAVE_IPV6
    default:
	error_msg = c_format("Address family %d is not supported", family());
	return (XORP_ERROR);
    }

    if (ret_value != XORP_OK) {
	error_msg = c_format("Failed to set the multicast interface: %s",
			     comm_get_last_error_str());
	return (XORP_ERROR);
    }

    return (send_to(group_addr, group_port, data, error_msg));
}

int
IoTcpUdpSocket::set_socket_option(const string& optname,
				  uint32_t optval,
				  string& error_msg)
{
    int ret_value = XORP_OK;

    if (! _socket_fd.is_valid()) {
	error_msg = c_format("The socket is not open");
	return (XORP_ERROR);
    }

    do {
	if (strcasecmp(optname.c_str(), "onesbcast") == 0) {
	    ret_value = comm_set_onesbcast(_socket_fd, optval);
	    break;
	}
	if (strcasecmp(optname.c_str(), "receive_broadcast") == 0) {
	    ret_value = comm_set_receive_broadcast(_socket_fd, optval);
	    break;
	}
	if (strcasecmp(optname.c_str(), "reuseport") == 0) {
	    ret_value = comm_set_reuseport(_socket_fd, optval);
	    break;
	}
	if (strcasecmp(optname.c_str(), "send_broadcast") == 0) {
	    ret_value = comm_set_send_broadcast(_socket_fd, optval);
	    break;
	}
	if (strcasecmp(optname.c_str(), "tos") == 0) {
	    // XXX: Do not return an error if setting the
	    // type-of-service fields is not supported.
	    if (comm_tos_present() == XORP_OK) {
		ret_value = comm_set_tos(_socket_fd, optval);
	    } else {
		ret_value = XORP_OK;
	    }
	    break;
	}
	if (strcasecmp(optname.c_str(), "ttl") == 0) {
	    ret_value = comm_set_unicast_ttl(_socket_fd, optval);
	    break;
	}
	if (strcasecmp(optname.c_str(), "multicast_loopback") == 0) {
	    ret_value = comm_set_loopback(_socket_fd, optval);
	    break;
	}
	if (strcasecmp(optname.c_str(), "multicast_ttl") == 0) {
	    ret_value = comm_set_multicast_ttl(_socket_fd, optval);
	    break;
	}
	error_msg = c_format("Unknown socket option: %s", optname.c_str());
	return (XORP_ERROR);
    } while (false);

    if (ret_value != XORP_OK) {
	error_msg = c_format("Failed to set socket option %s: %s",
			     optname.c_str(), comm_get_last_error_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IoTcpUdpSocket::set_socket_option(const string& optname,
                                  const string& optval,
				  string& error_msg)
{
    int ret_value = XORP_OK;

    if (! _socket_fd.is_valid()) {
	error_msg = c_format("The socket is not open");
	return (XORP_ERROR);
    }

    do {
	if (strcasecmp(optname.c_str(), "bindtodevice") == 0) {
	    // XXX: Do not use this option for new code; see note in
	    // comm_set_bindtodevice().
	    if (comm_bindtodevice_present() == XORP_OK) {
	       ret_value = comm_set_bindtodevice(_socket_fd, optval.c_str());
	    } else {
	       ret_value = XORP_OK;
	    }
	    break;
	}
	error_msg = c_format("Unknown socket option: %s", optname.c_str());
	return (XORP_ERROR);
    } while (false);

    if (ret_value != XORP_OK) {
	error_msg = c_format("Failed to set socket option %s: %s",
			     optname.c_str(), comm_get_last_error_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IoTcpUdpSocket::accept_connection(bool is_accepted, string& error_msg)
{
    if (is_accepted) {
	// Connection accepted
	if (! is_running()) {
	    error_msg = c_format("Cannot accept connection: "
				 "the plugin is not running");
	    return (XORP_ERROR);
	}
	return (enable_data_recv(error_msg));
    }

    // Connection rejected
    return (stop(error_msg));
}

int
IoTcpUdpSocket::enable_data_recv(string& error_msg)
{
    string dummy_error_msg;

    if (! is_running()) {
	error_msg = c_format("Cannot enable receiving of data: "
			     "the plugin is not running");
	return (XORP_ERROR);
    }
    if (! _socket_fd.is_valid()) {
	error_msg = c_format("Cannot enable receiving of data: "
			     "invalid socket");
	stop(dummy_error_msg);
	return (XORP_ERROR);
    }

    // Show interest in receiving additional information
    if (enable_recv_pktinfo(true, error_msg) != XORP_OK) {
	error_msg = c_format("Cannot enable receiving of data: %s",
			     error_msg.c_str());
	stop(dummy_error_msg);
	return (XORP_ERROR);
    }

    // Get the peer address and port for TCP connection
    if (is_tcp()) {
	// Get the peer address and port
	struct sockaddr_storage ss;
	socklen_t ss_len = sizeof(ss);
	if (getpeername(_socket_fd, reinterpret_cast<struct sockaddr*>(&ss),
			&ss_len)
	    != 0) {
	    error_msg = c_format("Cannot get the peer name: %s",
				 XSTRERROR);
	    stop(dummy_error_msg);
	    return (XORP_ERROR);
	}
	XLOG_ASSERT(ss.ss_family == family());
	_peer_address.copy_in(ss);
	_peer_port = get_sockadr_storage_port_number(ss);	
    }

    if (eventloop().add_ioevent_cb(_socket_fd, IOT_READ,
				   callback(this, &IoTcpUdpSocket::data_io_cb))
	!= true) {
	error_msg = c_format("Failed to add I/O callback to receive data");
	stop(dummy_error_msg);
	return (XORP_ERROR);
    }

#ifdef HOST_OS_WINDOWS
    // XXX: IOT_DISCONNECT is available only on Windows
    if (is_tcp()) {
	if (eventloop().add_ioevent_cb(_socket_fd, IOT_DISCONNECT,
				       callback(this, &IoTcpUdpSocket::disconnect_io_cb))
	    != true) {
	    error_msg = c_format("Failed to add I/O callback to detect peer disconnect");
	    stop(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
#endif // HOST_OS_WINDOWS

    return (XORP_OK);
}

void
IoTcpUdpSocket::accept_io_cb(XorpFd fd, IoEventType io_event_type)
{
    XorpFd accept_fd;
    struct sockaddr_storage ss;
    socklen_t ss_len = sizeof(ss);
    string error_msg;

    XLOG_ASSERT(fd == _socket_fd);

    UNUSED(io_event_type);

    //
    // Test whether there is a registered receiver
    //
    if (io_tcpudp_receiver() == NULL) {
	//
	// XXX: Accept the connection and close it.
	// This might happen only during startup and should be transient.
	//
	XLOG_WARNING("Received connection request, but no receiver is "
		     "registered. Ignoring...");
	accept_fd = comm_sock_accept(_socket_fd);
	if (accept_fd.is_valid())
	    comm_close(accept_fd);
	return;
    }

    //
    // Accept the connection
    //
    accept_fd = comm_sock_accept(_socket_fd);
    if (! accept_fd.is_valid()) {
	io_tcpudp_receiver()->error_event(comm_get_last_error_str(), false);
	return;
    }

    //
    // Get the peer address and port number
    //
    if (getpeername(accept_fd, sockaddr_storage2sockaddr(&ss), &ss_len) != 0) {
	error_msg = c_format("Error getting the peer name: %s",
			     XSTRERROR);
	comm_close(accept_fd);
	io_tcpudp_receiver()->error_event(error_msg, false);
	return;
    }
    XLOG_ASSERT(ss.ss_family == family());

    //
    // Set the socket as non-blocking
    //
    if (comm_sock_set_blocking(accept_fd, COMM_SOCK_NONBLOCKING) != XORP_OK) {
	error_msg = c_format("Error setting the socket as non-blocking: %s",
			     comm_get_last_error_str());
	comm_close(accept_fd);
	io_tcpudp_receiver()->error_event(error_msg, false);
	return;
    }

    IPvX src_host(ss);
    uint16_t src_port = get_sockadr_storage_port_number(ss);

    //
    // Allocate a new handler and start it
    //
    IoTcpUdp* io_tcpudp;
    IoTcpUdpSocket* io_tcpudp_socket;
    io_tcpudp = fea_data_plane_manager().allocate_io_tcpudp(iftree(),
							    family(),
							    is_tcp());
    if (io_tcpudp == NULL) {
	XLOG_ERROR("Connection request from %s rejected: "
		   "cannot allocate I/O TCP/UDP plugin from data plane "
		   "manager %s.",
		   src_host.str().c_str(),
		   fea_data_plane_manager().manager_name().c_str());
	comm_close(accept_fd);
	return;
    }
    io_tcpudp_socket = dynamic_cast<IoTcpUdpSocket*>(io_tcpudp);
    if (io_tcpudp_socket == NULL) {
	XLOG_ERROR("Connection request from %s rejected: "
		   "unrecognized I/O TCP/UDP plugin from data plane "
		   "manager %s.",
		   src_host.str().c_str(),
		   fea_data_plane_manager().manager_name().c_str());
	fea_data_plane_manager().deallocate_io_tcpudp(io_tcpudp);
	comm_close(accept_fd);
	return;
    }
    io_tcpudp_socket->set_socket_fd(accept_fd);

    //
    // Send the event to the receiver
    //
    io_tcpudp_receiver()->inbound_connect_event(src_host, src_port, io_tcpudp);
}

void
IoTcpUdpSocket::connect_io_cb(XorpFd fd, IoEventType io_event_type)
{
    string error_msg;
    int is_connected = 0;

    XLOG_ASSERT(fd == _socket_fd);

    UNUSED(io_event_type);

    //
    // Test whether there is a registered receiver
    //
    if (io_tcpudp_receiver() == NULL) {
	//
	// This might happen only during startup and should be transient.
	//
	XLOG_WARNING("Connection opening to the peer has completed, "
		     "but no receiver is registered.");
	return;
    }

    //
    // XXX: Remove from the eventloop for connect events.
    //
    eventloop().remove_ioevent_cb(_socket_fd, IOT_CONNECT);

    // Test whether the connection succeeded
    if (comm_sock_is_connected(_socket_fd, &is_connected) != XORP_OK) {
	io_tcpudp_receiver()->error_event(comm_get_last_error_str(), true);
	return;
    }
    if (is_connected == 0) {
	// Socket is not connected
	error_msg = c_format("Socket connect failed");
	io_tcpudp_receiver()->error_event(error_msg, true);
	return;
    }

    if (enable_data_recv(error_msg) != XORP_OK) {
	io_tcpudp_receiver()->error_event(error_msg, true);
	return;
    }

    //
    // Send the event to the receiver
    //
    io_tcpudp_receiver()->outgoing_connect_event();
}

void
IoTcpUdpSocket::data_io_cb(XorpFd fd, IoEventType io_event_type)
{
    string if_name;
    string vif_name;
    IPvX src_host = IPvX::ZERO(family());
    uint16_t src_port = 0;
    vector<uint8_t> data(0xffff);	// XXX: The max. payload is 0xffff
    ssize_t bytes_recv = 0;
    uint32_t pif_index = 0;
    bool use_recvmsg = false;
    string error_msg;

    XLOG_ASSERT(fd == _socket_fd);

    UNUSED(io_event_type);

    //
    // Test whether there is a registered receiver
    //
    if (io_tcpudp_receiver() == NULL) {
	//
	// This might happen only during startup and should be transient.
	//
	XLOG_WARNING("Received data, but no receiver is registered.");
	return;
    }

    //
    // Decide whether to use recfrom(2) or recvmsg(2):
    // - If Windows, use recvfrom(2)
    // - On all other systems use recvfrom(2) for TCP, and recvmsg(2) for UDP
    //
#ifndef HOST_OS_WINDOWS
    if (! is_tcp()) {
	use_recvmsg = true;
    }
#endif

    //
    // Receive the data
    //
    if (! use_recvmsg) {
	struct sockaddr_storage ss;
	socklen_t ss_len = sizeof(ss);

	bytes_recv = recvfrom(_socket_fd, XORP_BUF_CAST(&data[0]), data.size(),
			      0, reinterpret_cast<struct sockaddr*>(&ss),
			      &ss_len);
	if (bytes_recv < 0) {
	    error_msg = c_format("Error receiving TCP/UDP data on "
				 "socket %s: %s",
				 _socket_fd.str().c_str(), XSTRERROR);
	    io_tcpudp_receiver()->error_event(error_msg, false);
	    return;
	}

	//
	// Protocol-specific processing:
	// - Get the sender's address and port
	//
	if (is_tcp()) {
	    // TCP data
	    src_host = _peer_address;
	    src_port = _peer_port;
	} else {
	    // UDP data
	    src_host.copy_in(ss);
	    src_port = get_sockadr_storage_port_number(ss);
	}
    } else {

#ifdef HOST_OS_WINDOWS
	XLOG_UNREACHABLE();
#else
	//
	// XXX: Use recvmsg(2) to receive additional information
	//
	struct msghdr rcvmh;
	struct iovec rcviov[1];
	vector<uint8_t> rcvcmsgbuf(0xffff);
	void* cmsg_data;	// XXX: CMSG_DATA() is aligned, hence void ptr

	rcviov[0].iov_base = reinterpret_cast<caddr_t>(&data[0]);
	rcviov[0].iov_len = data.size();
	rcvmh.msg_iov = rcviov;
	rcvmh.msg_iovlen = 1;
	rcvmh.msg_control = reinterpret_cast<caddr_t>(&rcvcmsgbuf[0]);
	rcvmh.msg_controllen = rcvcmsgbuf.size();

	switch (family()) {
	case AF_INET:
	{
	    struct sockaddr_in from4;

	    memset(&from4, 0, sizeof(from4));
	    rcvmh.msg_name = reinterpret_cast<caddr_t>(&from4);
	    rcvmh.msg_namelen = sizeof(from4);
	    bytes_recv = recvmsg(_socket_fd, &rcvmh, 0);
	    if (bytes_recv < 0) {
		error_msg = c_format("Error receiving TCP/UDP data on "
				     "socket %s: %s",
				     _socket_fd.str().c_str(),
				     XSTRERROR);
		io_tcpudp_receiver()->error_event(error_msg, false);
		return;
	    }
	    src_host.copy_in(from4);
	    src_port = ntohs(from4.sin_port);

	    // Get the pif_index
	    for (struct cmsghdr *cmsgp = reinterpret_cast<struct cmsghdr *>(CMSG_FIRSTHDR(&rcvmh));
		 cmsgp != NULL;
		 cmsgp = reinterpret_cast<struct cmsghdr *>(CMSG_NXTHDR(&rcvmh, cmsgp))) {
		if (cmsgp->cmsg_level != IPPROTO_IP)
		    continue;
		switch (cmsgp->cmsg_type) {
#ifdef IP_RECVIF
		case IP_RECVIF:
		{
		    struct sockaddr_dl *sdl = NULL;
		    if (cmsgp->cmsg_len < CMSG_LEN(sizeof(struct sockaddr_dl)))
			continue;
		    cmsg_data = CMSG_DATA(cmsgp);
		    sdl = reinterpret_cast<struct sockaddr_dl *>(cmsg_data);
		    pif_index = sdl->sdl_index;
		}
		break;
#endif // IP_RECVIF

#ifdef IP_PKTINFO
		case IP_PKTINFO:
		{
		    struct in_pktinfo *inp = NULL;
		    if (cmsgp->cmsg_len < CMSG_LEN(sizeof(struct in_pktinfo)))
			continue;
		    cmsg_data = CMSG_DATA(cmsgp);
		    inp = reinterpret_cast<struct in_pktinfo *>(cmsg_data);
		    pif_index = inp->ipi_ifindex;
		}
		break;
#endif // IP_PKTINFO

		default:
		    break;
		}
	    }
	}
	break;

#ifdef HAVE_IPV6
	case AF_INET6:
	{
	    struct sockaddr_in6 from6;
	    struct in6_pktinfo *pi = NULL;

	    memset(&from6, 0, sizeof(from6));
	    rcvmh.msg_name = reinterpret_cast<caddr_t>(&from6);
	    rcvmh.msg_namelen = sizeof(from6);
	    bytes_recv = recvmsg(_socket_fd, &rcvmh, 0);
	    if (bytes_recv < 0) {
		error_msg = c_format("Error receiving TCP/UDP data on "
				     "socket %s: %s",
				     _socket_fd.str().c_str(),
				     XSTRERROR);
		io_tcpudp_receiver()->error_event(error_msg, false);
		return;
	    }
	    src_host.copy_in(from6);
	    src_port = ntohs(from6.sin6_port);

	    if (rcvmh.msg_flags & MSG_CTRUNC) {
		error_msg = c_format("Error receiving TCP/UDP data on "
				     "socket %s: "
				     "RX packet from %s with size of %d "
				     "bytes is truncated",
				     _socket_fd.str().c_str(),
				     cstring(src_host),
				     XORP_UINT_CAST(bytes_recv));
		io_tcpudp_receiver()->error_event(error_msg, false);
		return;
	    }
	    size_t controllen =  static_cast<size_t>(rcvmh.msg_controllen);
	    if (controllen < sizeof(struct cmsghdr)) {
		error_msg = c_format("Error receiving TCP/UDP data on "
				     "socket %s: "
				     "RX packet from %s has too short "
				     "msg_controllen (%u instead of %u)",
				     _socket_fd.str().c_str(),
				     cstring(src_host),
				     XORP_UINT_CAST(controllen),
				     XORP_UINT_CAST(sizeof(struct cmsghdr)));
		io_tcpudp_receiver()->error_event(error_msg, false);
		return;
	    }

	    // Get the pif_index
	    for (struct cmsghdr *cmsgp = reinterpret_cast<struct cmsghdr *>(CMSG_FIRSTHDR(&rcvmh));
		 cmsgp != NULL;
		 cmsgp = reinterpret_cast<struct cmsghdr *>(CMSG_NXTHDR(&rcvmh, cmsgp))) {
		if (cmsgp->cmsg_level != IPPROTO_IPV6)
		    continue;

		switch (cmsgp->cmsg_type) {
		case IPV6_PKTINFO:
		{
		    if (cmsgp->cmsg_len < CMSG_LEN(sizeof(struct in6_pktinfo)))
			continue;
		    cmsg_data = CMSG_DATA(cmsgp);
		    pi = reinterpret_cast<struct in6_pktinfo *>(cmsg_data);
		    pif_index = pi->ipi6_ifindex;
		    // dst_address.copy_in(pi->ipi6_addr);
		}
		break;

		default:
		    break;
		}
	    }
	}
	break;
#endif // HAVE_IPV6

	default:
	    XLOG_UNREACHABLE();
	    break;
	}
#endif // ! HOST_OS_WINDOWS
    }

    data.resize(bytes_recv);

    //
    // Find the interface and the vif this message was received on.
    //
    if (pif_index != 0) {
	const IfTreeVif* vifp = iftree().find_vif(pif_index);
	if (vifp != NULL) {
	    if_name = vifp->ifname();
	    vif_name = vifp->vifname();
	}
    }

    //
    // Protocol-specific processing:
    // - If TCP, test whether the connection was closed by the remote host.
    //
    if (is_tcp()) {
	// TCP data
	if (bytes_recv == 0) {
	    //
	    // XXX: Remove from the eventloop for disconnect events.
	    //
	    // Note that IOT_DISCONNECT is available only on Windows,
	    // hence we need to use IOT_READ instead.
	    //
	    eventloop().remove_ioevent_cb(_socket_fd, IOT_READ);
	    io_tcpudp_receiver()->disconnect_event();
	    return;
	}
    } else {
	// UDP data
    }

    //
    // Send the event to the receiver
    //
    io_tcpudp_receiver()->recv_event(if_name, vif_name, src_host, src_port,
				     data);
}

void
IoTcpUdpSocket::disconnect_io_cb(XorpFd fd, IoEventType io_event_type)
{
    XLOG_ASSERT(fd == _socket_fd);

    UNUSED(io_event_type);

    //
    // Test whether there is a registered receiver
    //
    if (io_tcpudp_receiver() == NULL) {
	//
	// This might happen only during startup and should be transient.
	//
	XLOG_WARNING("Received disconnect event, but no receiver is registered.");
	return;
    }

    //
    // XXX: Remove from the eventloop for disconnect events.
    //
    eventloop().remove_ioevent_cb(_socket_fd, IOT_DISCONNECT);
    io_tcpudp_receiver()->disconnect_event();
}

#endif // HAVE_TCPUDP_UNIX_SOCKETS
