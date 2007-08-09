// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/xrl_io_ip_manager.cc,v 1.2 2007/05/26 02:10:26 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxipc/xrl_router.hh"

#include "xrl/interfaces/fea_rawpkt4_client_xif.hh"
#include "xrl/interfaces/fea_rawpkt6_client_xif.hh"

#include "xrl_io_ip_manager.hh"

XrlIoIpManager::XrlIoIpManager(IoIpManager&	io_ip_manager,
			       XrlRouter&	xrl_router)
    : IoIpManagerReceiver(),
      _io_ip_manager(io_ip_manager),
      _xrl_router(xrl_router)
{
    _io_ip_manager.set_io_ip_manager_receiver(this);
}

XrlIoIpManager::~XrlIoIpManager()
{
    _io_ip_manager.set_io_ip_manager_receiver(NULL);
}

void
XrlIoIpManager::recv_event(const string& receiver_name,
			   const struct IPvXHeaderInfo& header,
			   const vector<uint8_t>& payload)
{
    size_t i;

    //
    // Create the extention headers info
    //
    XLOG_ASSERT(header.ext_headers_type.size()
		== header.ext_headers_payload.size());
    XrlAtomList ext_headers_type_list, ext_headers_payload_list;
    for (i = 0; i < header.ext_headers_type.size(); i++) {
	ext_headers_type_list.append(XrlAtom(static_cast<uint32_t>(header.ext_headers_type[i])));
	ext_headers_payload_list.append(XrlAtom(header.ext_headers_payload[i]));
    }

    if (header.src_address.is_ipv4()) {
	//
	// Instantiate client sending interface
	//
	XrlRawPacket4ClientV0p1Client cl(&xrl_router());

	//
	// Send notification
	//
	cl.send_recv(receiver_name.c_str(),
		     header.if_name,
		     header.vif_name,
		     header.src_address.get_ipv4(),
		     header.dst_address.get_ipv4(),
		     header.ip_protocol,
		     header.ip_ttl,
		     header.ip_tos,
		     header.ip_router_alert,
		     header.ip_internet_control,
		     payload,
		     callback(this,
			      &XrlIoIpManager::xrl_send_recv_cb,
			      header.src_address.af(), receiver_name));
    }

    if (header.src_address.is_ipv6()) {
	//
	// Instantiate client sending interface
	//
	XrlRawPacket6ClientV0p1Client cl(&xrl_router());

	//
	// Send notification
	//
	cl.send_recv(receiver_name.c_str(),
		     header.if_name,
		     header.vif_name,
		     header.src_address.get_ipv6(),
		     header.dst_address.get_ipv6(),
		     header.ip_protocol,
		     header.ip_ttl,
		     header.ip_tos,
		     header.ip_router_alert,
		     header.ip_internet_control,
		     ext_headers_type_list,
		     ext_headers_payload_list,
		     payload,
		     callback(this,
			      &XrlIoIpManager::xrl_send_recv_cb,
			      header.src_address.af(), receiver_name));
    }
}

void
XrlIoIpManager::xrl_send_recv_cb(const XrlError& xrl_error, int family,
				 string receiver_name)
{
    if (xrl_error == XrlError::OKAY())
	return;

    debug_msg("xrl_send_recv_cb: error %s\n", xrl_error.str().c_str());

    //
    // Sending Xrl generated an error.
    //
    // Remove all filters associated with this receiver.
    //
    _io_ip_manager.erase_filters_by_receiver_name(family, receiver_name);
}
