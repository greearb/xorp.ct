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

#ident "$XORP: xorp/fea/io_tcpudp_manager.cc,v 1.4 2007/08/17 19:48:07 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxipc/xuid.hh"	// XXX: Needed for the purpose of using XUID()

#include "fea_node.hh"
#include "iftree.hh"
#include "io_tcpudp_manager.hh"


/* ------------------------------------------------------------------------- */
/* IoTcpUdpComm methods */

IoTcpUdpComm::IoTcpUdpComm(IoTcpUdpManager& io_tcpudp_manager,
			   const IfTree& iftree, int family, bool is_tcp,
			   const string& creator)
    : IoTcpUdpReceiver(),
      _io_tcpudp_manager(io_tcpudp_manager),
      _iftree(iftree),
      _family(family),
      _is_tcp(is_tcp),
      _creator(creator),
      // XXX: Reuse the XRL implementation for generating an unique ID
      _sockid(XUID().str()),
      _peer_host(IPvX::ZERO(family)),
      _peer_port(0)
{
}

IoTcpUdpComm::IoTcpUdpComm(IoTcpUdpManager& io_tcpudp_manager,
			   const IfTree& iftree, int family, bool is_tcp,
			   const string& creator,
			   const string& listener_sockid,
			   const IPvX& peer_host, uint16_t peer_port)
    : IoTcpUdpReceiver(),
      _io_tcpudp_manager(io_tcpudp_manager),
      _iftree(iftree),
      _family(family),
      _is_tcp(is_tcp),
      _creator(creator),
      // XXX: Reuse the XRL implementation for generating an unique ID
      _sockid(XUID().str()),
      _listener_sockid(listener_sockid),
      _peer_host(peer_host),
      _peer_port(peer_port)
{
}

IoTcpUdpComm::~IoTcpUdpComm()
{
    deallocate_io_tcpudp_plugins();
}

int
IoTcpUdpComm::tcp_open(bool is_blocking, string& sockid, string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_tcpudp_plugins.empty()) {
	error_msg = c_format("No I/O TCP/UDP plugin to open TCP socket");
	return (XORP_ERROR);
    }

    IoTcpUdpPlugins::iterator iter;
    for (iter = _io_tcpudp_plugins.begin();
	 iter != _io_tcpudp_plugins.end();
	 ++iter) {
	IoTcpUdp* io_tcpudp = iter->second;
	if (io_tcpudp->tcp_open(is_blocking, error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    if (ret_value == XORP_OK)
	sockid = _sockid;

    return (ret_value);
}

int
IoTcpUdpComm::udp_open(bool is_blocking, string& sockid, string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_tcpudp_plugins.empty()) {
	error_msg = c_format("No I/O TCP/UDP plugin to open UDP socket");
	return (XORP_ERROR);
    }

    IoTcpUdpPlugins::iterator iter;
    for (iter = _io_tcpudp_plugins.begin();
	 iter != _io_tcpudp_plugins.end();
	 ++iter) {
	IoTcpUdp* io_tcpudp = iter->second;
	if (io_tcpudp->udp_open(is_blocking, error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    if (ret_value == XORP_OK)
	sockid = _sockid;

    return (ret_value);
}

int
IoTcpUdpComm::tcp_open_and_bind(const IPvX& local_addr, uint16_t local_port,
				bool is_blocking, string& sockid,
				string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_tcpudp_plugins.empty()) {
	error_msg = c_format("No I/O TCP/UDP plugin to open and bind "
			     "TCP socket with address %s and port %u",
			     local_addr.str().c_str(), local_port);
	return (XORP_ERROR);
    }

    IoTcpUdpPlugins::iterator iter;
    for (iter = _io_tcpudp_plugins.begin();
	 iter != _io_tcpudp_plugins.end();
	 ++iter) {
	IoTcpUdp* io_tcpudp = iter->second;
	if (io_tcpudp->tcp_open_and_bind(local_addr, local_port, is_blocking,
					 error_msg2)
	    != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    if (ret_value == XORP_OK)
	sockid = _sockid;

    return (ret_value);
}

int
IoTcpUdpComm::udp_open_and_bind(const IPvX& local_addr, uint16_t local_port,
				bool is_blocking, string& sockid,
				string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_tcpudp_plugins.empty()) {
	error_msg = c_format("No I/O TCP/UDP plugin to open and bind "
			     "UDP socket with address %s and port %u",
			     local_addr.str().c_str(), local_port);
	return (XORP_ERROR);
    }

    IoTcpUdpPlugins::iterator iter;
    for (iter = _io_tcpudp_plugins.begin();
	 iter != _io_tcpudp_plugins.end();
	 ++iter) {
	IoTcpUdp* io_tcpudp = iter->second;
	if (io_tcpudp->udp_open_and_bind(local_addr, local_port, is_blocking,
					 error_msg2)
	    != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    if (ret_value == XORP_OK)
	sockid = _sockid;

    return (ret_value);
}

int
IoTcpUdpComm::udp_open_bind_join(const IPvX& local_addr, uint16_t local_port,
				 const IPvX& mcast_addr, uint8_t ttl,
				 bool reuse, bool is_blocking, string& sockid,
				 string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_tcpudp_plugins.empty()) {
	error_msg = c_format("No I/O TCP/UDP plugin to open, bind and join "
			     "UDP socket with address %s and port %u "
			     "with group %s",
			     local_addr.str().c_str(), local_port,
			     mcast_addr.str().c_str());
	return (XORP_ERROR);
    }

    //
    // Add the record for the joined multicast group
    //
    JoinedGroupsTable::iterator joined_iter;
    JoinedMulticastGroup init_jmg(local_addr, mcast_addr);
    joined_iter = _joined_groups_table.find(init_jmg);
    if (joined_iter == _joined_groups_table.end()) {
	//
	// First receiver, hence join the multicast group first to check
	// for errors.
	//
	IoTcpUdpPlugins::iterator plugin_iter;
	for (plugin_iter = _io_tcpudp_plugins.begin();
	     plugin_iter != _io_tcpudp_plugins.end();
	     ++plugin_iter) {
	    IoTcpUdp* io_tcpudp = plugin_iter->second;
	    if (io_tcpudp->udp_open_bind_join(local_addr, local_port,
					      mcast_addr, ttl, reuse,
					      is_blocking, error_msg2)
		!= XORP_OK) {
		ret_value = XORP_ERROR;
		if (! error_msg.empty())
		    error_msg += " ";
		error_msg += error_msg2;
	    }
	}
	_joined_groups_table.insert(make_pair(init_jmg, init_jmg));
	joined_iter = _joined_groups_table.find(init_jmg);
    }
    XLOG_ASSERT(joined_iter != _joined_groups_table.end());
    //
    // TODO: If we add support for multiple receivers per socket, then
    // enable the following code to add a new receiver name.
    //
    // JoinedMulticastGroup& jmg = joined_iter->second;
    // jmg.add_receiver(receiver_name);

    if (ret_value == XORP_OK)
	sockid = _sockid;

    return (ret_value);
}

int
IoTcpUdpComm::tcp_open_bind_connect(const IPvX& local_addr,
				    uint16_t local_port,
				    const IPvX& remote_addr,
				    uint16_t remote_port,
				    bool is_blocking, string& sockid,
				    string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_tcpudp_plugins.empty()) {
	error_msg = c_format("No I/O TCP/UDP plugin to open, bind and connect "
			     "TCP socket with address %s and port %u "
			     "with remote address %s and port %u",
			     local_addr.str().c_str(), local_port,
			     remote_addr.str().c_str(), remote_port);
	return (XORP_ERROR);
    }

    IoTcpUdpPlugins::iterator iter;
    for (iter = _io_tcpudp_plugins.begin();
	 iter != _io_tcpudp_plugins.end();
	 ++iter) {
	IoTcpUdp* io_tcpudp = iter->second;
	if (io_tcpudp->tcp_open_bind_connect(local_addr, local_port,
					     remote_addr, remote_port,
					     is_blocking, error_msg2)
	    != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    if (ret_value == XORP_OK)
	sockid = _sockid;

    return (ret_value);
}

int
IoTcpUdpComm::udp_open_bind_connect(const IPvX& local_addr,
				    uint16_t local_port,
				    const IPvX& remote_addr,
				    uint16_t remote_port,
				    bool is_blocking, string& sockid,
				    string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_tcpudp_plugins.empty()) {
	error_msg = c_format("No I/O TCP/UDP plugin to open, bind and connect "
			     "UDP socket with address %s and port %u "
			     "with remote address %s and port %u",
			     local_addr.str().c_str(), local_port,
			     remote_addr.str().c_str(), remote_port);
	return (XORP_ERROR);
    }

    IoTcpUdpPlugins::iterator iter;
    for (iter = _io_tcpudp_plugins.begin();
	 iter != _io_tcpudp_plugins.end();
	 ++iter) {
	IoTcpUdp* io_tcpudp = iter->second;
	if (io_tcpudp->udp_open_bind_connect(local_addr, local_port,
					     remote_addr, remote_port,
					     is_blocking, error_msg2)
	    != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    if (ret_value == XORP_OK)
	sockid = _sockid;

    return (ret_value);
}

int
IoTcpUdpComm::bind(const IPvX& local_addr, uint16_t local_port,
		   string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_tcpudp_plugins.empty()) {
	error_msg = c_format("No I/O TCP/UDP plugin to bind "
			     "socket with address %s and port %u",
			     local_addr.str().c_str(), local_port);
	return (XORP_ERROR);
    }

    IoTcpUdpPlugins::iterator iter;
    for (iter = _io_tcpudp_plugins.begin();
	 iter != _io_tcpudp_plugins.end();
	 ++iter) {
	IoTcpUdp* io_tcpudp = iter->second;
	if (io_tcpudp->bind(local_addr, local_port, error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    return (ret_value);
}

int
IoTcpUdpComm::udp_join_group(const IPvX& mcast_addr, const IPvX& join_if_addr,
			     string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_tcpudp_plugins.empty()) {
	error_msg = c_format("No I/O TCP/UDP plugin to join "
			     "UDP socket on group %s and interface address %s",
			     mcast_addr.str().c_str(),
			     join_if_addr.str().c_str());
	return (XORP_ERROR);
    }

    //
    // Add the record for the joined multicast group
    //
    JoinedGroupsTable::iterator joined_iter;
    JoinedMulticastGroup init_jmg(join_if_addr, mcast_addr);
    joined_iter = _joined_groups_table.find(init_jmg);
    if (joined_iter == _joined_groups_table.end()) {
	//
	// First receiver, hence join the multicast group first to check
	// for errors.
	//
	IoTcpUdpPlugins::iterator plugin_iter;
	for (plugin_iter = _io_tcpudp_plugins.begin();
	     plugin_iter != _io_tcpudp_plugins.end();
	     ++plugin_iter) {
	    IoTcpUdp* io_tcpudp = plugin_iter->second;
	    if (io_tcpudp->udp_join_group(mcast_addr, join_if_addr, error_msg2)
		!= XORP_OK) {
		ret_value = XORP_ERROR;
		if (! error_msg.empty())
		    error_msg += " ";
		error_msg += error_msg2;
	    }
	}
	_joined_groups_table.insert(make_pair(init_jmg, init_jmg));
	joined_iter = _joined_groups_table.find(init_jmg);
    }
    XLOG_ASSERT(joined_iter != _joined_groups_table.end());
    //
    // TODO: If we add support for multiple receivers per socket, then
    // enable the following code to add a new receiver name.
    //
    // JoinedMulticastGroup& jmg = joined_iter->second;
    // jmg.add_receiver(receiver_name);

    return (ret_value);
}

int
IoTcpUdpComm::udp_leave_group(const IPvX& mcast_addr,
			      const IPvX& leave_if_addr,
			      string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_tcpudp_plugins.empty()) {
	error_msg = c_format("No I/O TCP/UDP plugin to leave "
			     "UDP socket on group %s and interface address %s",
			     mcast_addr.str().c_str(),
			     leave_if_addr.str().c_str());
	return (XORP_ERROR);
    }

    //
    // Delete the record for the joined multicast group
    //
    JoinedGroupsTable::iterator joined_iter;
    JoinedMulticastGroup init_jmg(leave_if_addr, mcast_addr);
    joined_iter = _joined_groups_table.find(init_jmg);
    if (joined_iter == _joined_groups_table.end()) {
	error_msg = c_format("Cannot leave group %s on interface address %s: "
			     "the group was not joined",
			     mcast_addr.str().c_str(),
			     leave_if_addr.str().c_str());
	return (XORP_ERROR);
    }

    JoinedMulticastGroup& jmg = joined_iter->second;
    //
    // TODO: If we add support for multiple receivers per socket, then
    // enable the following code to delete the receiver name.
    //
    // jmg.delete_receiver(receiver_name);
    if (jmg.empty()) {
	//
	// The last receiver, hence leave the group
	//
	_joined_groups_table.erase(joined_iter);

	IoTcpUdpPlugins::iterator plugin_iter;
	for (plugin_iter = _io_tcpudp_plugins.begin();
	     plugin_iter != _io_tcpudp_plugins.end();
	     ++plugin_iter) {
	    IoTcpUdp* io_tcpudp = plugin_iter->second;
	    if (io_tcpudp->udp_leave_group(mcast_addr, leave_if_addr,
					   error_msg2)
		!= XORP_OK) {
		ret_value = XORP_ERROR;
		if (! error_msg.empty())
		    error_msg += " ";
		error_msg += error_msg2;
	    }
	}
    }

    return (ret_value);
}

int
IoTcpUdpComm::close(string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_tcpudp_plugins.empty()) {
	error_msg = c_format("No I/O TCP/UDP plugin to close socket");
	return (XORP_ERROR);
    }

    //
    // XXX: We assume that closing a socket automatically leaves all
    // previously joined multicast groups on that socket, therefore we
    // just clear the set of joined multicast groups.
    //
    _joined_groups_table.clear();

    IoTcpUdpPlugins::iterator iter;
    for (iter = _io_tcpudp_plugins.begin();
	 iter != _io_tcpudp_plugins.end();
	 ++iter) {
	IoTcpUdp* io_tcpudp = iter->second;
	if (io_tcpudp->close(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    return (ret_value);
}

int
IoTcpUdpComm::tcp_listen(uint32_t backlog, string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_tcpudp_plugins.empty()) {
	error_msg = c_format("No I/O TCP/UDP plugin to listen to TCP socket");
	return (XORP_ERROR);
    }

    IoTcpUdpPlugins::iterator iter;
    for (iter = _io_tcpudp_plugins.begin();
	 iter != _io_tcpudp_plugins.end();
	 ++iter) {
	IoTcpUdp* io_tcpudp = iter->second;
	if (io_tcpudp->tcp_listen(backlog, error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    return (ret_value);
}

int
IoTcpUdpComm::send(const vector<uint8_t>& data, string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_tcpudp_plugins.empty()) {
	error_msg = c_format("No I/O TCP/UDP plugin to send data on socket");
	return (XORP_ERROR);
    }

    IoTcpUdpPlugins::iterator iter;
    for (iter = _io_tcpudp_plugins.begin();
	 iter != _io_tcpudp_plugins.end();
	 ++iter) {
	IoTcpUdp* io_tcpudp = iter->second;
	if (io_tcpudp->send(data, error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    return (ret_value);
}

int
IoTcpUdpComm::send_to(const IPvX& remote_addr, uint16_t remote_port,
		      const vector<uint8_t>& data, string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_tcpudp_plugins.empty()) {
	error_msg = c_format("No I/O TCP/UDP plugin to send data on "
			     "socket to remote address %s and port %u",
			     remote_addr.str().c_str(), remote_port);
	return (XORP_ERROR);
    }

    IoTcpUdpPlugins::iterator iter;
    for (iter = _io_tcpudp_plugins.begin();
	 iter != _io_tcpudp_plugins.end();
	 ++iter) {
	IoTcpUdp* io_tcpudp = iter->second;
	if (io_tcpudp->send_to(remote_addr, remote_port, data, error_msg2)
	    != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    return (ret_value);
}

int
IoTcpUdpComm::send_from_multicast_if(const IPvX& group_addr,
				     uint16_t group_port,
				     const IPvX& ifaddr,
				     const vector<uint8_t>& data,
				     string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_tcpudp_plugins.empty()) {
	error_msg = c_format("No I/O TCP/UDP plugin to send data from "
			     "multicast interface with address %s on "
			     "socket to group %s and port %u from ",
			     ifaddr.str().c_str(), group_addr.str().c_str(),
			     group_port);
	return (XORP_ERROR);
    }

    IoTcpUdpPlugins::iterator iter;
    for (iter = _io_tcpudp_plugins.begin();
	 iter != _io_tcpudp_plugins.end();
	 ++iter) {
	IoTcpUdp* io_tcpudp = iter->second;
	if (io_tcpudp->send_from_multicast_if(group_addr, group_port, ifaddr,
					      data, error_msg2)
	    != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    return (ret_value);
}

int
IoTcpUdpComm::set_socket_option(const string& optname, uint32_t optval,
				string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_tcpudp_plugins.empty()) {
	error_msg = c_format("No I/O TCP/UDP plugin to set %s socket option",
			     optname.c_str());
	return (XORP_ERROR);
    }

    IoTcpUdpPlugins::iterator iter;
    for (iter = _io_tcpudp_plugins.begin();
	 iter != _io_tcpudp_plugins.end();
	 ++iter) {
	IoTcpUdp* io_tcpudp = iter->second;
	if (io_tcpudp->set_socket_option(optname, optval, error_msg2)
	    != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    return (ret_value);
}

int
IoTcpUdpComm::accept_connection(bool is_accepted, string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (_io_tcpudp_plugins.empty()) {
	error_msg = c_format("No I/O TCP/UDP plugin to %s a connection",
			     (is_accepted)? "accept" : "reject");
	return (XORP_ERROR);
    }

    IoTcpUdpPlugins::iterator iter;
    for (iter = _io_tcpudp_plugins.begin();
	 iter != _io_tcpudp_plugins.end();
	 ++iter) {
	IoTcpUdp* io_tcpudp = iter->second;
	if (io_tcpudp->accept_connection(is_accepted, error_msg2)
	    != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    return (ret_value);
}

void
IoTcpUdpComm::recv_event(const IPvX&		src_host,
			 uint16_t		src_port,
			 const vector<uint8_t>&	data)
{
    _io_tcpudp_manager.recv_event(_creator, sockid(), src_host, src_port,
				  data);
}

void
IoTcpUdpComm::inbound_connect_event(const IPvX&		src_host,
				    uint16_t		src_port,
				    IoTcpUdp*		new_io_tcpudp)
{
    IoTcpUdpComm* new_io_tcpudp_comm;

    new_io_tcpudp_comm = _io_tcpudp_manager.connect_io_tcpudp_comm(_family,
								   _is_tcp,
								   _creator,
								   sockid(),
								   src_host,
								   src_port,
								   new_io_tcpudp);

    _io_tcpudp_manager.inbound_connect_event(_creator, sockid(),
					     src_host, src_port,
					     new_io_tcpudp_comm->sockid());
}

void
IoTcpUdpComm::outgoing_connect_event()
{
    _io_tcpudp_manager.outgoing_connect_event(_family, _creator, sockid());
}

void
IoTcpUdpComm::error_event(const string&		error,
			  bool			fatal)
{
    _io_tcpudp_manager.error_event(_family, _creator, sockid(), error, fatal);
}

void
IoTcpUdpComm::disconnect_event()
{
    _io_tcpudp_manager.disconnect_event(_family, _creator, sockid());
}

void
IoTcpUdpComm::allocate_io_tcpudp_plugins()
{
    list<FeaDataPlaneManager *>::iterator iter;

    for (iter = _io_tcpudp_manager.fea_data_plane_managers().begin();
	 iter != _io_tcpudp_manager.fea_data_plane_managers().end();
	 ++iter) {
	FeaDataPlaneManager* fea_data_plane_manager = *iter;
	allocate_io_tcpudp_plugin(fea_data_plane_manager);
    }
}

void
IoTcpUdpComm::deallocate_io_tcpudp_plugins()
{
    while (! _io_tcpudp_plugins.empty()) {
	IoTcpUdpPlugins::iterator iter = _io_tcpudp_plugins.begin();
	FeaDataPlaneManager* fea_data_plane_manager = iter->first;
	deallocate_io_tcpudp_plugin(fea_data_plane_manager);
    }
}

void
IoTcpUdpComm::allocate_io_tcpudp_plugin(FeaDataPlaneManager* fea_data_plane_manager)
{
    IoTcpUdpPlugins::iterator iter;

    XLOG_ASSERT(fea_data_plane_manager != NULL);

    for (iter = _io_tcpudp_plugins.begin();
	 iter != _io_tcpudp_plugins.end();
	 ++iter) {
	if (iter->first == fea_data_plane_manager)
	    break;
    }
    if (iter != _io_tcpudp_plugins.end()) {
	return;	// XXX: the plugin was already allocated
    }

    IoTcpUdp* io_tcpudp = fea_data_plane_manager->allocate_io_tcpudp(
	_iftree, _family, _is_tcp);
    if (io_tcpudp == NULL) {
	XLOG_ERROR("Couldn't allocate plugin for I/O TCP/UDP "
		   "communications for data plane manager %s",
		   fea_data_plane_manager->manager_name().c_str());
	return;
    }

    _io_tcpudp_plugins.push_back(make_pair(fea_data_plane_manager, io_tcpudp));
}

void
IoTcpUdpComm::deallocate_io_tcpudp_plugin(FeaDataPlaneManager* fea_data_plane_manager)
{
    IoTcpUdpPlugins::iterator iter;

    XLOG_ASSERT(fea_data_plane_manager != NULL);

    for (iter = _io_tcpudp_plugins.begin();
	 iter != _io_tcpudp_plugins.end();
	 ++iter) {
	if (iter->first == fea_data_plane_manager)
	    break;
    }
    if (iter == _io_tcpudp_plugins.end()) {
	XLOG_ERROR("Couldn't deallocate plugin for I/O TCP/UDP "
		   "communications for data plane manager %s: plugin not found",
		   fea_data_plane_manager->manager_name().c_str());
	return;
    }

    IoTcpUdp* io_tcpudp = iter->second;
    fea_data_plane_manager->deallocate_io_tcpudp(io_tcpudp);
    _io_tcpudp_plugins.erase(iter);
}

void
IoTcpUdpComm::add_plugin(IoTcpUdp* new_io_tcpudp)
{
    IoTcpUdpPlugins::iterator iter;

    XLOG_ASSERT(new_io_tcpudp != NULL);

    for (iter = _io_tcpudp_plugins.begin();
	 iter != _io_tcpudp_plugins.end();
	 ++iter) {
	if (iter->second == new_io_tcpudp)
	    break;
    }
    if (iter != _io_tcpudp_plugins.end()) {
	return;	// XXX: the plugin was already added
    }

    _io_tcpudp_plugins.push_back(
	make_pair(&new_io_tcpudp->fea_data_plane_manager(), new_io_tcpudp));
}

void
IoTcpUdpComm::start_io_tcpudp_plugins()
{
    IoTcpUdpPlugins::iterator iter;
    string error_msg;

    for (iter = _io_tcpudp_plugins.begin();
	 iter != _io_tcpudp_plugins.end();
	 ++iter) {
	IoTcpUdp* io_tcpudp = iter->second;
	if (io_tcpudp->is_running())
	    continue;
	io_tcpudp->register_io_tcpudp_receiver(this);
	if (io_tcpudp->start(error_msg) != XORP_OK) {
	    XLOG_ERROR("%s", error_msg.c_str());
	    continue;
	}

	//
	// Push all multicast joins into the new plugin
	//
	JoinedGroupsTable::iterator join_iter;
	for (join_iter = _joined_groups_table.begin();
	     join_iter != _joined_groups_table.end();
	     ++join_iter) {
	    JoinedMulticastGroup& joined_multicast_group = join_iter->second;
	    if (io_tcpudp->udp_join_group(
		    joined_multicast_group.group_address(),
		    joined_multicast_group.interface_address(),
		    error_msg)
		!= XORP_OK) {
		XLOG_ERROR("%s", error_msg.c_str());
	    }
	}
    }
}

void
IoTcpUdpComm::stop_io_tcpudp_plugins()
{
    string error_msg;
    IoTcpUdpPlugins::iterator iter;

    for (iter = _io_tcpudp_plugins.begin();
	 iter != _io_tcpudp_plugins.end();
	 ++iter) {
	IoTcpUdp* io_tcpudp = iter->second;
	io_tcpudp->unregister_io_tcpudp_receiver();
	if (io_tcpudp->stop(error_msg) != XORP_OK) {
	    XLOG_ERROR("%s", error_msg.c_str());
	}
    }
}

// ----------------------------------------------------------------------------
// IoTcpUdpManager code

IoTcpUdpManager::IoTcpUdpManager(FeaNode&	fea_node,
				 const IfTree&	iftree)
    : IoTcpUdpManagerReceiver(),
      _fea_node(fea_node),
      _eventloop(fea_node.eventloop()),
      _iftree(iftree),
      _io_tcpudp_manager_receiver(NULL)
{
}

IoTcpUdpManager::~IoTcpUdpManager()
{
    CommTable::iterator iter;

    //
    // Delete all IoTcpUdpComm entries
    //
    while (! _comm_table4.empty()) {
	iter = _comm_table4.begin();
	delete iter->second;
	_comm_table4.erase(iter);
    }
    while (! _comm_table6.empty()) {
	iter = _comm_table6.begin();
	delete iter->second;
	_comm_table6.erase(iter);
    }
}

IoTcpUdpManager::CommTable&
IoTcpUdpManager::comm_table_by_family(int family)
{
    if (family == IPv4::af())
	return (_comm_table4);
    if (family == IPv6::af())
	return (_comm_table6);

    XLOG_FATAL("Invalid address family: %d", family);
    return (_comm_table4);
}

void
IoTcpUdpManager::erase_comm_handlers_by_creator(int family,
						const string& creator)
{
    CommTable& comm_table = comm_table_by_family(family);
    CommTable::iterator iter;

    // Delete all entries for a given creator name
    for (iter = comm_table.begin(); iter != comm_table.end(); ) {
	IoTcpUdpComm* io_tcpudp_comm = iter->second;
	CommTable::iterator tmp_iter = iter++;

	if (io_tcpudp_comm->creator() == creator) {
	    comm_table.erase(tmp_iter);
	    delete io_tcpudp_comm;
	}
    }
}

bool
IoTcpUdpManager::has_comm_handler_by_creator(const string& creator) const
{
    CommTable::const_iterator iter;

    // There whether there is an entry for a given creator name
    for (iter = _comm_table4.begin(); iter != _comm_table4.end(); ++iter) {
	const IoTcpUdpComm* io_tcpudp_comm = iter->second;
	if (io_tcpudp_comm->creator() == creator)
	    return (true);
    }
    for (iter = _comm_table6.begin(); iter != _comm_table6.end(); ++iter) {
	const IoTcpUdpComm* io_tcpudp_comm = iter->second;
	if (io_tcpudp_comm->creator() == creator)
	    return (true);
    }

    return (false);
}

int
IoTcpUdpManager::register_data_plane_manager(
    FeaDataPlaneManager* fea_data_plane_manager,
    bool is_exclusive)
{
    if (is_exclusive) {
	// Unregister all registered data plane managers
	while (! _fea_data_plane_managers.empty()) {
	    unregister_data_plane_manager(_fea_data_plane_managers.front());
	}
    }

    if (fea_data_plane_manager == NULL) {
	// XXX: exclusive NULL is used to unregister all data plane managers
	return (XORP_OK);
    }

    if (find(_fea_data_plane_managers.begin(),
	     _fea_data_plane_managers.end(),
	     fea_data_plane_manager)
	!= _fea_data_plane_managers.end()) {
	// XXX: already registered
	return (XORP_OK);
    }

    _fea_data_plane_managers.push_back(fea_data_plane_manager);

    //
    // Allocate all I/O TCP/UDP plugins for the new data plane manager
    //
    CommTable::iterator iter;
    for (iter = _comm_table4.begin(); iter != _comm_table4.end(); ++iter) {
	IoTcpUdpComm* io_tcpudp_comm = iter->second;
	io_tcpudp_comm->allocate_io_tcpudp_plugin(fea_data_plane_manager);
	io_tcpudp_comm->start_io_tcpudp_plugins();
    }
    for (iter = _comm_table6.begin(); iter != _comm_table6.end(); ++iter) {
	IoTcpUdpComm* io_tcpudp_comm = iter->second;
	io_tcpudp_comm->allocate_io_tcpudp_plugin(fea_data_plane_manager);
	io_tcpudp_comm->start_io_tcpudp_plugins();
    }

    return (XORP_OK);
}

int
IoTcpUdpManager::unregister_data_plane_manager(
    FeaDataPlaneManager* fea_data_plane_manager)
{
    if (fea_data_plane_manager == NULL)
	return (XORP_ERROR);

    list<FeaDataPlaneManager*>::iterator data_plane_manager_iter;
    data_plane_manager_iter = find(_fea_data_plane_managers.begin(),
				   _fea_data_plane_managers.end(),
				   fea_data_plane_manager);
    if (data_plane_manager_iter == _fea_data_plane_managers.end())
	return (XORP_ERROR);

    //
    // Dealocate all I/O TCP/UDP plugins for the unregistered data plane
    // manager
    //
    CommTable::iterator iter;
    for (iter = _comm_table4.begin(); iter != _comm_table4.end(); ++iter) {
	IoTcpUdpComm* io_tcpudp_comm = iter->second;
	io_tcpudp_comm->deallocate_io_tcpudp_plugin(fea_data_plane_manager);
    }
    for (iter = _comm_table6.begin(); iter != _comm_table6.end(); ++iter) {
	IoTcpUdpComm* io_tcpudp_comm = iter->second;
	io_tcpudp_comm->deallocate_io_tcpudp_plugin(fea_data_plane_manager);
    }

    _fea_data_plane_managers.erase(data_plane_manager_iter);

    return (XORP_OK);
}

int
IoTcpUdpManager::tcp_open(int family, const string& creator, bool is_blocking,
			  string& sockid, string& error_msg)
{
    IoTcpUdpComm* io_tcpudp_comm;

    io_tcpudp_comm = open_io_tcpudp_comm(family, true, creator);
    XLOG_ASSERT(io_tcpudp_comm != NULL);

    if (io_tcpudp_comm->tcp_open(is_blocking, sockid, error_msg) != XORP_OK) {
	delete_io_tcpudp_comm(family, io_tcpudp_comm->sockid());
	return (XORP_ERROR);
    }

    if (_fea_node.fea_io().add_instance_watch(creator, this, error_msg)
	!= XORP_OK) {
	delete_io_tcpudp_comm(family, io_tcpudp_comm->sockid());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IoTcpUdpManager::udp_open(int family, const string& creator, bool is_blocking,
			  string& sockid, string& error_msg)
{
    IoTcpUdpComm* io_tcpudp_comm;

    io_tcpudp_comm = open_io_tcpudp_comm(family, false, creator);
    XLOG_ASSERT(io_tcpudp_comm != NULL);

    if (io_tcpudp_comm->udp_open(is_blocking, sockid, error_msg) != XORP_OK) {
	delete_io_tcpudp_comm(family, io_tcpudp_comm->sockid());
	return (XORP_ERROR);
    }

    if (_fea_node.fea_io().add_instance_watch(creator, this, error_msg)
	!= XORP_OK) {
	delete_io_tcpudp_comm(family, io_tcpudp_comm->sockid());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IoTcpUdpManager::tcp_open_and_bind(int family, const string& creator,
				   const IPvX& local_addr, uint16_t local_port,
				   bool is_blocking, string& sockid,
				   string& error_msg)
{
    IoTcpUdpComm* io_tcpudp_comm;

    //
    // If "local_addr" is not zero, then it must belong to a local interface
    //
    if (! local_addr.is_zero()) {
	if (! is_my_address(local_addr)) {
	    error_msg = c_format("Cannot open and bind a TCP socket "
				 "to address %s: address not found",
				 local_addr.str().c_str());
	    return (XORP_ERROR);
	}
    }

    io_tcpudp_comm = open_io_tcpudp_comm(family, true, creator);
    XLOG_ASSERT(io_tcpudp_comm != NULL);

    if (io_tcpudp_comm->tcp_open_and_bind(local_addr, local_port, is_blocking,
					  sockid, error_msg)
	!= XORP_OK) {
	delete_io_tcpudp_comm(family, io_tcpudp_comm->sockid());
	return (XORP_ERROR);
    }

    if (_fea_node.fea_io().add_instance_watch(creator, this, error_msg)
	!= XORP_OK) {
	delete_io_tcpudp_comm(family, io_tcpudp_comm->sockid());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IoTcpUdpManager::udp_open_and_bind(int family, const string& creator,
				   const IPvX& local_addr, uint16_t local_port,
				   bool is_blocking, string& sockid,
				   string& error_msg)
{
    IoTcpUdpComm* io_tcpudp_comm;

    //
    // If "local_addr" is not zero, then it must belong to a local interface
    //
    if (! local_addr.is_zero()) {
	if (! is_my_address(local_addr)) {
	    error_msg = c_format("Cannot open and bind an UDP socket "
				 "to address %s: address not found",
				 local_addr.str().c_str());
	    return (XORP_ERROR);
	}
    }

    io_tcpudp_comm = open_io_tcpudp_comm(family, false, creator);
    XLOG_ASSERT(io_tcpudp_comm != NULL);

    if (io_tcpudp_comm->udp_open_and_bind(local_addr, local_port, is_blocking,
					  sockid, error_msg)
	!= XORP_OK) {
	delete_io_tcpudp_comm(family, io_tcpudp_comm->sockid());
	return (XORP_ERROR);
    }

    if (_fea_node.fea_io().add_instance_watch(creator, this, error_msg)
	!= XORP_OK) {
	delete_io_tcpudp_comm(family, io_tcpudp_comm->sockid());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IoTcpUdpManager::udp_open_bind_join(int family, const string& creator,
				    const IPvX& local_addr,
				    uint16_t local_port,
				    const IPvX& mcast_addr,
				    uint8_t ttl, bool reuse,
				    bool is_blocking, string& sockid,
				    string& error_msg)
{
    IoTcpUdpComm* io_tcpudp_comm;

    //
    // The "local_addr" must not zero, and must belong to a local interface
    //
    if (local_addr.is_zero()) {
	error_msg = c_format("Cannot open, bind and join an UDP socket "
			     "to address ZERO: the address must belong to "
			     "a local interface");
	return (XORP_ERROR);
    } else {
	if (! is_my_address(local_addr)) {
	    error_msg = c_format("Cannot open, bind and join an UDP socket "
				 "to address %s: address not found",
				 local_addr.str().c_str());
	    return (XORP_ERROR);
	}
    }

    io_tcpudp_comm = open_io_tcpudp_comm(family, false, creator);
    XLOG_ASSERT(io_tcpudp_comm != NULL);

    if (io_tcpudp_comm->udp_open_bind_join(local_addr, local_port, mcast_addr,
					   ttl, reuse, is_blocking, sockid,
					   error_msg)
	!= XORP_OK) {
	delete_io_tcpudp_comm(family, io_tcpudp_comm->sockid());
	return (XORP_ERROR);
    }

    if (_fea_node.fea_io().add_instance_watch(creator, this, error_msg)
	!= XORP_OK) {
	delete_io_tcpudp_comm(family, io_tcpudp_comm->sockid());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IoTcpUdpManager::tcp_open_bind_connect(int family, const string& creator,
				       const IPvX& local_addr,
				       uint16_t local_port,
				       const IPvX& remote_addr,
				       uint16_t remote_port,
				       bool is_blocking, string& sockid,
				       string& error_msg)
{
    IoTcpUdpComm* io_tcpudp_comm;

    //
    // If "local_addr" is not zero, then it must belong to a local interface
    //
    if (! local_addr.is_zero()) {
	if (! is_my_address(local_addr)) {
	    error_msg = c_format("Cannot open, bind and connect a TCP socket "
				 "to address %s: address not found",
				 local_addr.str().c_str());
	    return (XORP_ERROR);
	}
    }

    io_tcpudp_comm = open_io_tcpudp_comm(family, true, creator);
    XLOG_ASSERT(io_tcpudp_comm != NULL);

    if (io_tcpudp_comm->tcp_open_bind_connect(local_addr, local_port,
					      remote_addr, remote_port,
					      is_blocking, sockid, error_msg)
	!= XORP_OK) {
	delete_io_tcpudp_comm(family, io_tcpudp_comm->sockid());
	return (XORP_ERROR);
    }

    if (_fea_node.fea_io().add_instance_watch(creator, this, error_msg)
	!= XORP_OK) {
	delete_io_tcpudp_comm(family, io_tcpudp_comm->sockid());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IoTcpUdpManager::udp_open_bind_connect(int family, const string& creator,
				       const IPvX& local_addr,
				       uint16_t local_port,
				       const IPvX& remote_addr,
				       uint16_t remote_port,
				       bool is_blocking, string& sockid,
				       string& error_msg)
{
    IoTcpUdpComm* io_tcpudp_comm;

    //
    // If "local_addr" is not zero, then it must belong to a local interface
    //
    if (! local_addr.is_zero()) {
	if (! is_my_address(local_addr)) {
	    error_msg = c_format("Cannot open, bind and connect an UDP socket "
				 "to address %s: address not found",
				 local_addr.str().c_str());
	    return (XORP_ERROR);
	}
    }

    io_tcpudp_comm = open_io_tcpudp_comm(family, false, creator);
    XLOG_ASSERT(io_tcpudp_comm != NULL);

    if (io_tcpudp_comm->udp_open_bind_connect(local_addr, local_port,
					      remote_addr, remote_port,
					      is_blocking, sockid, error_msg)
	!= XORP_OK) {
	delete_io_tcpudp_comm(family, io_tcpudp_comm->sockid());
	return (XORP_ERROR);
    }

    if (_fea_node.fea_io().add_instance_watch(creator, this, error_msg)
	!= XORP_OK) {
	delete_io_tcpudp_comm(family, io_tcpudp_comm->sockid());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IoTcpUdpManager::bind(int family, const string& sockid, const IPvX& local_addr,
		      uint16_t local_port, string& error_msg)
{
    IoTcpUdpComm* io_tcpudp_comm;

    //
    // If "local_addr" is not zero, then it must belong to a local interface
    //
    if (! local_addr.is_zero()) {
	if (! is_my_address(local_addr)) {
	    error_msg = c_format("Cannot bind a socket "
				 "to address %s: address not found",
				 local_addr.str().c_str());
	    return (XORP_ERROR);
	}
    }

    io_tcpudp_comm = find_io_tcpudp_comm(family, sockid, error_msg);
    if (io_tcpudp_comm == NULL)
	return (XORP_ERROR);

    return (io_tcpudp_comm->bind(local_addr, local_port, error_msg));
}

int
IoTcpUdpManager::udp_join_group(int family, const string& sockid,
				const IPvX& mcast_addr,
				const IPvX& join_if_addr,
				string& error_msg)
{
    IoTcpUdpComm* io_tcpudp_comm;

    //
    // The "join_if_addr" must not zero, and must belong to a local interface
    //
    if (join_if_addr.is_zero()) {
	error_msg = c_format("Cannot join an UDP socket "
			     "to address ZERO: the address must belong to "
			     "a local interface");
	return (XORP_ERROR);
    } else {
	if (! is_my_address(join_if_addr)) {
	    error_msg = c_format("Cannot join an UDP socket "
				 "to address %s: address not found",
				 join_if_addr.str().c_str());
	    return (XORP_ERROR);
	}
    }

    io_tcpudp_comm = find_io_tcpudp_comm(family, sockid, error_msg);
    if (io_tcpudp_comm == NULL)
	return (XORP_ERROR);

    return (io_tcpudp_comm->udp_join_group(mcast_addr, join_if_addr,
					   error_msg));
}

int
IoTcpUdpManager::udp_leave_group(int family, const string& sockid,
				 const IPvX& mcast_addr,
				 const IPvX& leave_if_addr,
				 string& error_msg)
{
    IoTcpUdpComm* io_tcpudp_comm;

    //
    // The "leave_if_addr" must not zero, and must belong to a local interface
    //
    if (leave_if_addr.is_zero()) {
	error_msg = c_format("Cannot leave an UDP socket "
			     "on address ZERO: the address must belong to "
			     "a local interface");
	return (XORP_ERROR);
    } else {
	if (! is_my_address(leave_if_addr)) {
	    error_msg = c_format("Cannot leave an UDP socket "
				 "on address %s: address not found",
				 leave_if_addr.str().c_str());
	    return (XORP_ERROR);
	}
    }

    io_tcpudp_comm = find_io_tcpudp_comm(family, sockid, error_msg);
    if (io_tcpudp_comm == NULL)
	return (XORP_ERROR);

    return (io_tcpudp_comm->udp_leave_group(mcast_addr, leave_if_addr,
					    error_msg));
}

int
IoTcpUdpManager::close(int family, const string& sockid, string& error_msg)
{
    IoTcpUdpComm* io_tcpudp_comm;
    int ret_value;
    string creator;

    io_tcpudp_comm = find_io_tcpudp_comm(family, sockid, error_msg);
    if (io_tcpudp_comm == NULL)
	return (XORP_ERROR);

    creator = io_tcpudp_comm->creator();

    ret_value = io_tcpudp_comm->close(error_msg);
    delete_io_tcpudp_comm(family, io_tcpudp_comm->sockid());

    // Deregister interest in watching the creator
    if (! has_comm_handler_by_creator(creator)) {
	string dummy_error_msg;
	_fea_node.fea_io().delete_instance_watch(creator, this,
						 dummy_error_msg);
    }

    return (ret_value);
}

int
IoTcpUdpManager::tcp_listen(int family, const string& sockid, uint32_t backlog,
			    string& error_msg)
{
    IoTcpUdpComm* io_tcpudp_comm;

    io_tcpudp_comm = find_io_tcpudp_comm(family, sockid, error_msg);
    if (io_tcpudp_comm == NULL)
	return (XORP_ERROR);

    return (io_tcpudp_comm->tcp_listen(backlog, error_msg));
}

int
IoTcpUdpManager::send(int family, const string& sockid,
		      const vector<uint8_t>& data, string& error_msg)
{
    IoTcpUdpComm* io_tcpudp_comm;

    io_tcpudp_comm = find_io_tcpudp_comm(family, sockid, error_msg);
    if (io_tcpudp_comm == NULL)
	return (XORP_ERROR);

    return (io_tcpudp_comm->send(data, error_msg));
}

int
IoTcpUdpManager::send_to(int family, const string& sockid,
			 const IPvX& remote_addr, uint16_t remote_port,
			 const vector<uint8_t>& data, string& error_msg)
{
    IoTcpUdpComm* io_tcpudp_comm;

    io_tcpudp_comm = find_io_tcpudp_comm(family, sockid, error_msg);
    if (io_tcpudp_comm == NULL)
	return (XORP_ERROR);

    return (io_tcpudp_comm->send_to(remote_addr, remote_port, data,
				    error_msg));
}

int
IoTcpUdpManager::send_from_multicast_if(int family, const string& sockid,
					const IPvX& group_addr,
					uint16_t group_port,
					const IPvX& ifaddr,
					const vector<uint8_t>& data,
					string& error_msg)
{
    IoTcpUdpComm* io_tcpudp_comm;

    //
    // The "ifaddr" must not zero, and must belong to a local interface
    //
    if (ifaddr.is_zero()) {
	error_msg = c_format("Cannot send on an UDP socket "
			     "from address ZERO: the address must belong to "
			     "a local interface");
	return (XORP_ERROR);
    } else {
	if (! is_my_address(ifaddr)) {
	    error_msg = c_format("Cannot send on an UDP socket "
				 "from address %s: address not found",
				 ifaddr.str().c_str());
	    return (XORP_ERROR);
	}
    }

    io_tcpudp_comm = find_io_tcpudp_comm(family, sockid, error_msg);
    if (io_tcpudp_comm == NULL)
	return (XORP_ERROR);

    return (io_tcpudp_comm->send_from_multicast_if(group_addr, group_port,
						   ifaddr, data, error_msg));
}

int
IoTcpUdpManager::set_socket_option(int family, const string& sockid,
				   const string& optname, uint32_t optval,
				   string& error_msg)
{
    IoTcpUdpComm* io_tcpudp_comm;

    io_tcpudp_comm = find_io_tcpudp_comm(family, sockid, error_msg);
    if (io_tcpudp_comm == NULL)
	return (XORP_ERROR);

    return (io_tcpudp_comm->set_socket_option(optname, optval, error_msg));
}

int
IoTcpUdpManager::accept_connection(int family, const string& sockid,
				   bool is_accepted, string& error_msg)
{
    IoTcpUdpComm* io_tcpudp_comm;
    int ret_value = XORP_OK;

    io_tcpudp_comm = find_io_tcpudp_comm(family, sockid, error_msg);
    if (io_tcpudp_comm == NULL)
	return (XORP_ERROR);

    ret_value = io_tcpudp_comm->accept_connection(is_accepted, error_msg);

    if (! is_accepted) {
	//
	// Connection rejected, therefore close and delete the socket.
	//
	string dummy_error_msg;
	close(family, sockid, dummy_error_msg);
    }

    return (ret_value);
}

void
IoTcpUdpManager::recv_event(const string&		receiver_name,
			    const string&		sockid,
			    const IPvX&			src_host,
			    uint16_t			src_port,
			    const vector<uint8_t>&	data)
{
    if (_io_tcpudp_manager_receiver != NULL)
	_io_tcpudp_manager_receiver->recv_event(receiver_name, sockid,
						src_host, src_port, data);
}

void
IoTcpUdpManager::inbound_connect_event(const string&	receiver_name,
				       const string&	sockid,
				       const IPvX&	src_host,
				       uint16_t		src_port,
				       const string&	new_sockid)
{
    if (_io_tcpudp_manager_receiver != NULL)
	_io_tcpudp_manager_receiver->inbound_connect_event(receiver_name,
							   sockid,
							   src_host, src_port,
							   new_sockid);
}

void
IoTcpUdpManager::outgoing_connect_event(int		family,
					const string&	receiver_name,
					const string&	sockid)
{
    if (_io_tcpudp_manager_receiver != NULL)
	_io_tcpudp_manager_receiver->outgoing_connect_event(family,
							    receiver_name,
							    sockid);
}

void
IoTcpUdpManager::error_event(int		family,
			     const string&	receiver_name,
			     const string&	sockid,
			     const string&	error,
			     bool		fatal)
{
    if (_io_tcpudp_manager_receiver != NULL)
	_io_tcpudp_manager_receiver->error_event(family, receiver_name, sockid,
						 error, fatal);

    if (fatal) {
	//
	// Fatal error, therefore close and delete the socket.
	//
	string dummy_error_msg;
	close(family, sockid, dummy_error_msg);
    }
}

void
IoTcpUdpManager::disconnect_event(int		family,
			  const string&		receiver_name,
			  const string&		sockid)
{
    if (_io_tcpudp_manager_receiver != NULL)
	_io_tcpudp_manager_receiver->disconnect_event(family, receiver_name,
						      sockid);
}

void
IoTcpUdpManager::instance_birth(const string& instance_name)
{
    // XXX: Nothing to do
    UNUSED(instance_name);
}

void
IoTcpUdpManager::instance_death(const string& instance_name)
{
    string dummy_error_msg;

    _fea_node.fea_io().delete_instance_watch(instance_name, this,
					     dummy_error_msg);

    erase_comm_handlers_by_creator(AF_INET, instance_name);
#ifdef HAVE_IPV6
    erase_comm_handlers_by_creator(AF_INET6, instance_name);
#endif
}

IoTcpUdpComm*
IoTcpUdpManager::connect_io_tcpudp_comm(int family,
					bool is_tcp,
					const string& creator,
					const string& listener_sockid,
					const IPvX& peer_host,
					uint16_t peer_port,
					IoTcpUdp* new_io_tcpudp)
{
    CommTable& comm_table = comm_table_by_family(family);
    CommTable::iterator iter;
    IoTcpUdpComm* io_tcpudp_comm = NULL;

    //
    // Find a matching IoTcpUdpComm entry that was created by the
    // connect event from the first plugin.
    //
    for (iter = comm_table.begin(); iter != comm_table.end(); ++iter) {
	io_tcpudp_comm = iter->second;
	if ((io_tcpudp_comm->listener_sockid() == listener_sockid)
	    && (io_tcpudp_comm->peer_host() == peer_host)
	    && (io_tcpudp_comm->peer_port() == peer_port)) {
	    break;
	}
	io_tcpudp_comm = NULL;
    }

    if (io_tcpudp_comm == NULL) {
	//
	// Entry not found, therefore create it. However, it shouldn't
	// contain any plugins.
	//
	io_tcpudp_comm = open_io_tcpudp_comm(family, is_tcp, creator, false);
	XLOG_ASSERT(io_tcpudp_comm != NULL);
    }

    //
    // Add the new plugin
    //
    io_tcpudp_comm->add_plugin(new_io_tcpudp);
    io_tcpudp_comm->start_io_tcpudp_plugins();

    return (io_tcpudp_comm);
}

IoTcpUdpComm*
IoTcpUdpManager::find_io_tcpudp_comm(int family, const string& sockid,
				     string& error_msg)
{
    CommTable& comm_table = comm_table_by_family(family);
    CommTable::iterator iter;

    iter = comm_table.find(sockid);
    if (iter == comm_table.end()) {
	error_msg = c_format("Socket not found");
	return (NULL);
    }

    return (iter->second);
}

IoTcpUdpComm*
IoTcpUdpManager::open_io_tcpudp_comm(int family, bool is_tcp,
				     const string& creator,
				     bool allocate_plugins)
{
    CommTable& comm_table = comm_table_by_family(family);
    IoTcpUdpComm* io_tcpudp_comm;

    io_tcpudp_comm = new IoTcpUdpComm(*this, iftree(), family, is_tcp,
				      creator);
    comm_table[io_tcpudp_comm->sockid()] = io_tcpudp_comm;

    //
    // Allocate and start the IoTcpUdp plugins: one per data plane manager.
    //
    if (allocate_plugins) {
	io_tcpudp_comm->allocate_io_tcpudp_plugins();
	io_tcpudp_comm->start_io_tcpudp_plugins();
    }

    return (io_tcpudp_comm);
}

void
IoTcpUdpManager::delete_io_tcpudp_comm(int family, const string& sockid)
{
    CommTable& comm_table = comm_table_by_family(family);
    CommTable::iterator iter;

    iter = comm_table.find(sockid);
    if (iter == comm_table.end())
	return;

    // Delete the entry
    IoTcpUdpComm* io_tcpudp_comm = iter->second;
    comm_table.erase(iter);
    delete io_tcpudp_comm;
}

bool
IoTcpUdpManager::is_my_address(const IPvX& local_addr) const
{
    const IfTreeInterface* ifp = NULL;
    const IfTreeVif* vifp = NULL;
    return (_iftree.find_interface_vif_by_addr(local_addr, ifp, vifp) == true);
}
