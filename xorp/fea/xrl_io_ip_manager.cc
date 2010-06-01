// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2010 XORP, Inc and Others
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

#include "libxipc/xrl_router.hh"

#include "xrl/interfaces/fea_rawpkt4_client_xif.hh"
#ifdef HAVE_IPV6
#include "xrl/interfaces/fea_rawpkt6_client_xif.hh"
#endif

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

#ifdef HAVE_IPV6
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
#endif
}

void
XrlIoIpManager::xrl_send_recv_cb(const XrlError& xrl_error, int family,
				 string receiver_name)
{
    UNUSED(family);

    if (xrl_error == XrlError::OKAY())
	return;

    debug_msg("xrl_send_recv_cb: error %s\n", xrl_error.str().c_str());

    //
    // Sending Xrl generated an error.
    //
    // Remove all filters associated with this receiver.
    //
    _io_ip_manager.instance_death(receiver_name);
}
