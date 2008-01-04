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

#ident "$XORP: xorp/fea/data_plane/io/io_tcpudp_dummy.cc,v 1.4 2007/08/21 00:10:37 pavlin Exp $"

//
// I/O TCP/UDP communication support.
//
// The mechanism is Dummy.
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
IoTcpUdpDummy::udp_open_and_bind(const IPvX& local_addr, uint16_t local_port,
				 string& error_msg)
{
    XLOG_ASSERT(family() == local_addr.af());

    UNUSED(local_addr);
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
IoTcpUdpDummy::accept_connection(bool is_accepted, string& error_msg)
{
    if (is_accepted) {
	// Connection accepted
	return (XORP_OK);
    }

    // Connection rejected
    return (stop(error_msg));
}
