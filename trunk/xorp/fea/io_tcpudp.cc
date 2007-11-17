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

#ident "$XORP: xorp/fea/io_tcpudp.cc,v 1.3 2007/08/20 19:12:14 pavlin Exp $"

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
