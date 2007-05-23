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

#ident "$XORP: xorp/fea/xrl_rawsock6.cc,v 1.18 2007/05/22 22:57:20 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include "libxipc/xrl_router.hh"

#include "xrl/interfaces/fea_rawpkt6_client_xif.hh"

#include "rawsock6.hh"
#include "xrl_rawsock6.hh"

// ----------------------------------------------------------------------------
// XrlRawSocket6Manager code

XrlRawSocket6Manager::XrlRawSocket6Manager(EventLoop&		eventloop,
					   const IfTree&	iftree,
					   XrlRouter&		xrl_router)
    : RawSocket6Manager(eventloop, iftree),
      _xrl_router(xrl_router)
{
}

void
XrlRawSocket6Manager::recv_and_forward(const string& xrl_target_name,
				       const struct IPv6HeaderInfo& header,
				       const vector<uint8_t>& payload)
{
    //
    // Create the extention headers info
    //
    XLOG_ASSERT(header.ext_headers_type.size()
		== header.ext_headers_payload.size());
    XrlAtomList ext_headers_type_list, ext_headers_payload_list;
    size_t i;
    for (i = 0; i < header.ext_headers_type.size(); i++) {
	ext_headers_type_list.append(XrlAtom(static_cast<uint32_t>(header.ext_headers_type[i])));
	ext_headers_payload_list.append(XrlAtom(header.ext_headers_payload[i]));
    }

    //
    // Instantiate client sending interface
    //
    XrlRawPacket6ClientV0p1Client cl(&xrl_router());

    //
    // Send notification
    //
    cl.send_recv(xrl_target_name.c_str(),
		 header.if_name,
		 header.vif_name,
		 header.src_address,
		 header.dst_address,
		 header.ip_protocol,
		 header.ip_ttl,
		 header.ip_tos,
		 header.ip_router_alert,
		 header.ip_internet_control,
		 ext_headers_type_list,
		 ext_headers_payload_list,
		 payload,
		 callback(this,
			  &XrlRawSocket6Manager::xrl_send_recv_cb,
			  xrl_target_name));
}

void
XrlRawSocket6Manager::xrl_send_recv_cb(const XrlError& xrl_error,
				       string xrl_target_name)
{
    if (xrl_error == XrlError::OKAY())
	return;

    debug_msg("xrl_send_recv_cb: error %s\n", xrl_error.str().c_str());

    //
    // Sending Xrl generated an error.
    //
    // Remove all filters associated with Xrl Target that are tied to a raw
    // socket and then erase filters.
    //
    erase_filters_by_name(xrl_target_name);
}
