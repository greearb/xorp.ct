// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2009 XORP, Inc.
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
// The mechanism is Dummy (for testing purpose).
//

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "io_tcpudp_dummy.hh"


IoTcpUdpDummy::IoTcpUdpDummy(FeaDataPlaneManager& fea_data_plane_manager,
			     const IfTree& iftree, int family,
			     bool is_tcp)
    : IoTcpUdp(fea_data_plane_manager, iftree, family, is_tcp)
{
}

IoTcpUdpDummy::~IoTcpUdpDummy()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the I/O TCP/UDP Dummy mechanism: %s",
		   error_msg.c_str());
    }
}

int
IoTcpUdpDummy::start(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running)
	return (XORP_OK);

    _is_running = true;

    return (XORP_OK);
}

int
IoTcpUdpDummy::stop(string& error_msg)
{
    UNUSED(error_msg);

    if (! _is_running)
	return (XORP_OK);

    _is_running = false;

    return (XORP_OK);
}

int
IoTcpUdpDummy::tcp_open(string& error_msg)
{
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IoTcpUdpDummy::udp_open(string& error_msg)
{
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IoTcpUdpDummy::tcp_open_and_bind(const IPvX& local_addr, uint16_t local_port,
				 string& error_msg)
{
    XLOG_ASSERT(family() == local_addr.af());

    UNUSED(local_addr);
    UNUSED(local_port);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IoTcpUdpDummy::udp_open_and_bind(const IPvX& local_addr, uint16_t local_port, const string& local_dev,
				 int reuse, string& error_msg)
{
    XLOG_ASSERT(family() == local_addr.af());

    UNUSED(local_addr);
    UNUSED(local_dev);
    UNUSED(reuse);
    UNUSED(local_port);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IoTcpUdpDummy::udp_open_bind_join(const IPvX& local_addr, uint16_t local_port,
				  const IPvX& mcast_addr, uint8_t ttl,
				  bool reuse, string& error_msg)
{
    XLOG_ASSERT(family() == local_addr.af());
    XLOG_ASSERT(family() == mcast_addr.af());

    UNUSED(local_addr);
    UNUSED(local_port);
    UNUSED(mcast_addr);
    UNUSED(ttl);
    UNUSED(reuse);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IoTcpUdpDummy::tcp_open_bind_connect(const IPvX& local_addr,
				     uint16_t local_port,
				     const IPvX& remote_addr,
				     uint16_t remote_port,
				     string& error_msg)
{
    XLOG_ASSERT(family() == local_addr.af());
    XLOG_ASSERT(family() == remote_addr.af());

    UNUSED(local_addr);
    UNUSED(local_port);
    UNUSED(remote_addr);
    UNUSED(remote_port);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IoTcpUdpDummy::udp_open_bind_connect(const IPvX& local_addr,
				     uint16_t local_port,
				     const IPvX& remote_addr,
				     uint16_t remote_port,
				     string& error_msg)
{
    XLOG_ASSERT(family() == local_addr.af());
    XLOG_ASSERT(family() == remote_addr.af());

    UNUSED(local_addr);
    UNUSED(local_port);
    UNUSED(remote_addr);
    UNUSED(remote_port);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IoTcpUdpDummy::udp_open_bind_broadcast(const string& ifname,
				       const string& vifname,
				       uint16_t local_port,
				       uint16_t remote_port,
				       bool reuse,
				       bool limited,
				       bool connected,
				       string& error_msg)
{
    UNUSED(ifname);
    UNUSED(vifname);
    UNUSED(local_port);
    UNUSED(remote_port);
    UNUSED(reuse);
    UNUSED(limited);
    UNUSED(connected);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IoTcpUdpDummy::bind(const IPvX& local_addr, uint16_t local_port,
		    string& error_msg)
{
    XLOG_ASSERT(family() == local_addr.af());

    UNUSED(local_addr);
    UNUSED(local_port);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IoTcpUdpDummy::udp_join_group(const IPvX& mcast_addr,
			      const IPvX& join_if_addr,
			      string& error_msg)
{
    XLOG_ASSERT(family() == mcast_addr.af());
    XLOG_ASSERT(family() == join_if_addr.af());

    UNUSED(mcast_addr);
    UNUSED(join_if_addr);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IoTcpUdpDummy::udp_leave_group(const IPvX& mcast_addr,
			       const IPvX& leave_if_addr,
			       string& error_msg)
{
    XLOG_ASSERT(family() == mcast_addr.af());
    XLOG_ASSERT(family() == leave_if_addr.af());

    UNUSED(mcast_addr);
    UNUSED(leave_if_addr);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IoTcpUdpDummy::close(string& error_msg)
{
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IoTcpUdpDummy::tcp_listen(uint32_t backlog, string& error_msg)
{
    UNUSED(backlog);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IoTcpUdpDummy::udp_enable_recv(string& error_msg)
{
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IoTcpUdpDummy::send(const vector<uint8_t>& data, string& error_msg)
{
    UNUSED(data);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IoTcpUdpDummy::send_to(const IPvX& remote_addr, uint16_t remote_port,
		       const vector<uint8_t>& data, string& error_msg)
{
    XLOG_ASSERT(family() == remote_addr.af());

    UNUSED(remote_addr);
    UNUSED(remote_port);
    UNUSED(data);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IoTcpUdpDummy::send_from_multicast_if(const IPvX& group_addr,
				      uint16_t group_port,
				      const IPvX& ifaddr,
				      const vector<uint8_t>& data,
				      string& error_msg)
{
    XLOG_ASSERT(family() == group_addr.af());
    XLOG_ASSERT(family() == ifaddr.af());

    UNUSED(group_addr);
    UNUSED(group_port);
    UNUSED(ifaddr);
    UNUSED(data);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IoTcpUdpDummy::set_socket_option(const string& optname, uint32_t optval,
				 string& error_msg)
{
    UNUSED(optname);
    UNUSED(optval);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IoTcpUdpDummy::set_socket_option(const string& optname,
				 const string& optval,
				 string& error_msg)
{
    UNUSED(optname);
    UNUSED(optval);
    UNUSED(error_msg);

    return (XORP_OK);
}

int
IoTcpUdpDummy::accept_connection(bool is_accepted, string& error_msg)
{
    if (is_accepted) {
	// Connection accepted
	return (XORP_OK);
    }

    // Connection rejected
    return (stop(error_msg));
}
