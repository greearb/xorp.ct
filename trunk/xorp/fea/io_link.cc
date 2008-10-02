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

#ident "$XORP: xorp/fea/io_link.cc,v 1.3 2008/07/23 05:10:10 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/mac.hh"

#include "io_link.hh"


//
// FEA I/O link raw communication base class implementation.
//

IoLink::IoLink(FeaDataPlaneManager& fea_data_plane_manager,
	       const IfTree& iftree, const string& if_name,
	       const string& vif_name, uint16_t ether_type,
	       const string& filter_program)
    : _is_running(false),
      _io_link_manager(fea_data_plane_manager.io_link_manager()),
      _fea_data_plane_manager(fea_data_plane_manager),
      _eventloop(fea_data_plane_manager.eventloop()),
      _iftree(iftree),
      _if_name(if_name),
      _vif_name(vif_name),
      _ether_type(ether_type),
      _filter_program(filter_program),
      _io_link_receiver(NULL),
      _is_log_trace(false)
{
}

IoLink::~IoLink()
{
}

void
IoLink::register_io_link_receiver(IoLinkReceiver* io_link_receiver)
{
    _io_link_receiver = io_link_receiver;
}

void
IoLink::unregister_io_link_receiver()
{
    _io_link_receiver = NULL;
}

void
IoLink::recv_packet(const Mac&		src_address,
		    const Mac&		dst_address,
		    uint16_t		ether_type,
		    const vector<uint8_t>& payload)
{
    if (_io_link_receiver == NULL) {
	// XXX: should happen only during transient setup stage
	return;
    }

    _io_link_receiver->recv_packet(src_address, dst_address, ether_type,
				   payload);
}
