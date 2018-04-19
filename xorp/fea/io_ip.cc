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

#include "io_ip.hh"
#include "libxorp/ipv4.hh"


//
// FEA I/O IP raw communication base class implementation.
//

IoIp::IoIp(FeaDataPlaneManager& fea_data_plane_manager, const IfTree& iftree,
	   int family, uint8_t ip_protocol)
    : _is_running(false),
      _io_ip_manager(fea_data_plane_manager.io_ip_manager()),
      _fea_data_plane_manager(fea_data_plane_manager),
      _eventloop(fea_data_plane_manager.eventloop()),
      _iftree(iftree),
      _family(family),
      _ip_protocol(ip_protocol),
      _io_ip_receiver(NULL)
{
}

IoIp::~IoIp()
{
}

void
IoIp::register_io_ip_receiver(IoIpReceiver* io_ip_receiver)
{
    _io_ip_receiver = io_ip_receiver;
}

void
IoIp::unregister_io_ip_receiver()
{
    _io_ip_receiver = NULL;
}

void
IoIp::recv_packet(const string&	if_name,
		  const string&	vif_name,
		  const IPvX&	src_address,
		  const IPvX&	dst_address,
		  int32_t	ip_ttl,
		  int32_t	ip_tos,
		  bool		ip_router_alert,
		  bool		ip_internet_control,
		  const vector<uint8_t>& ext_headers_type,
		  const vector<vector<uint8_t> >& ext_headers_payload,
		  const vector<uint8_t>& payload)
{
    //XLOG_INFO("recv-packet, io-ip-rcvr: %p protocol: %d(%s)\n",
    //          _io_ip_receiver, ip_protocol(), ip_proto_str(ip_protocol()));
    if (_io_ip_receiver == NULL) {
	// XXX: should happen only during transient setup stage
	return;
    }

    _io_ip_receiver->recv_packet(if_name, vif_name, src_address, dst_address,
				 ip_ttl, ip_tos, ip_router_alert,
				 ip_internet_control, ext_headers_type,
				 ext_headers_payload, payload);
}

void
IoIp::recv_system_multicast_upcall(const vector<uint8_t>& payload)
{
    //XLOG_INFO("recv-mcast-upcall, io-ip-rcvr: %p\n",
    //      _io_ip_receiver);

    if (_io_ip_receiver == NULL) {
	// XXX: should happen only during transient setup stage
	return;
    }

    _io_ip_receiver->recv_system_multicast_upcall(payload);
}
