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



#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "io_tcpudp.hh"


//
// FEA I/O TCP/UDP communication base class implementation.
//

IoTcpUdp::IoTcpUdp(FeaDataPlaneManager& fea_data_plane_manager,
		   const IfTree& iftree, int family, bool is_tcp)
    : _is_running(false),
      _io_tcpudp_manager(fea_data_plane_manager.io_tcpudp_manager()),
      _fea_data_plane_manager(fea_data_plane_manager),
      _eventloop(fea_data_plane_manager.eventloop()),
      _iftree(iftree),
      _family(family),
      _is_tcp(is_tcp),
      _io_tcpudp_receiver(NULL)
{
}

IoTcpUdp::~IoTcpUdp()
{
}

void
IoTcpUdp::register_io_tcpudp_receiver(IoTcpUdpReceiver* io_tcpudp_receiver)
{
    _io_tcpudp_receiver = io_tcpudp_receiver;
}

void
IoTcpUdp::unregister_io_tcpudp_receiver()
{
    _io_tcpudp_receiver = NULL;
}

void
IoTcpUdp::recv_event(const string&		if_name,
		     const string&		vif_name,
		     const IPvX&		src_host,
		     uint16_t			src_port,
		     const vector<uint8_t>&	data)
{
    if (_io_tcpudp_receiver == NULL) {
	// XXX: should happen only during transient setup stage
	return;
    }

    _io_tcpudp_receiver->recv_event(if_name, vif_name, src_host, src_port,
				    data);
}

void
IoTcpUdp::inbound_connect_event(const IPvX&	src_host,
				uint16_t	src_port,
				IoTcpUdp*	new_io_tcpudp)
{
    if (_io_tcpudp_receiver == NULL) {
	// XXX: should happen only during transient setup stage
	return;
    }

    _io_tcpudp_receiver->inbound_connect_event(src_host, src_port,
					       new_io_tcpudp);
}

void
IoTcpUdp::outgoing_connect_event()
{
    if (_io_tcpudp_receiver == NULL) {
	// XXX: should happen only during transient setup stage
	return;
    }

    _io_tcpudp_receiver->outgoing_connect_event();
}

void
IoTcpUdp::error_event(const string&		error,
		      bool			fatal)
{
    if (_io_tcpudp_receiver == NULL) {
	// XXX: should happen only during transient setup stage
	return;
    }

    _io_tcpudp_receiver->error_event(error, fatal);
}

void
IoTcpUdp::disconnect_event()
{
    if (_io_tcpudp_receiver == NULL) {
	// XXX: should happen only during transient setup stage
	return;
    }

    _io_tcpudp_receiver->disconnect_event();

}
