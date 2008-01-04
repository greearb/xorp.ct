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

#ident "$XORP: xorp/fea/io_ip.cc,v 1.1 2007/07/26 01:18:39 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "io_ip.hh"


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
    if (_io_ip_receiver == NULL) {
	// XXX: should happen only during transient setup stage
	return;
    }

    _io_ip_receiver->recv_system_multicast_upcall(payload);
}
