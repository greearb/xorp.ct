// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2008 XORP, Inc.
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

#ident "$XORP: xorp/fea/io_link.cc,v 1.2 2008/01/04 03:15:47 pavlin Exp $"

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
