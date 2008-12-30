// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2008 XORP, Inc.
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

#ident "$XORP: xorp/fea/xrl_io_link_manager.cc,v 1.5 2008/07/23 05:10:13 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxipc/xrl_router.hh"

#include "xrl/interfaces/fea_rawlink_client_xif.hh"

#include "xrl_io_link_manager.hh"

XrlIoLinkManager::XrlIoLinkManager(IoLinkManager&	io_link_manager,
				   XrlRouter&		xrl_router)
    : IoLinkManagerReceiver(),
      _io_link_manager(io_link_manager),
      _xrl_router(xrl_router)
{
    _io_link_manager.set_io_link_manager_receiver(this);
}

XrlIoLinkManager::~XrlIoLinkManager()
{
    _io_link_manager.set_io_link_manager_receiver(NULL);
}

void
XrlIoLinkManager::recv_event(const string& receiver_name,
			     const struct MacHeaderInfo& header,
			     const vector<uint8_t>& payload)
{
    //
    // Instantiate client sending interface
    //
    XrlRawLinkClientV0p1Client cl(&xrl_router());

    //
    // Send notification
    //
    cl.send_recv(receiver_name.c_str(),
		 header.if_name,
		 header.vif_name,
		 header.src_address,
		 header.dst_address,
		 header.ether_type,
		 payload,
		 callback(this,
			  &XrlIoLinkManager::xrl_send_recv_cb, receiver_name));
}

void
XrlIoLinkManager::xrl_send_recv_cb(const XrlError& xrl_error,
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
    _io_link_manager.instance_death(receiver_name);
}
