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

#ident "$XORP: xorp/fea/xrl_io_link_manager.cc,v 1.3 2007/08/15 18:55:17 pavlin Exp $"

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
