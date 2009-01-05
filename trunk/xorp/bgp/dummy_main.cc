// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

#ident "$XORP: xorp/bgp/dummy_main.cc,v 1.25 2008/10/02 21:56:15 bms Exp $"

#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xorpfd.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"

#include "bgp.hh"
#include "path_attribute.hh"
#include "iptuple.hh"


BGPMain::BGPMain(EventLoop& eventloop)
    : _eventloop(eventloop)
{
    _local_data = new LocalData(_eventloop);
    _xrl_router = NULL;
}

BGPMain::~BGPMain() {
    /*
    ** Any xrl events added by the destruction of the rib_ipc_hander
    ** are soaked up before destroying the xrl_router.
    */
    if (_xrl_router != NULL)
	while(_xrl_router->pending())
	    eventloop().run();
    delete _local_data;
}

int
BGPMain::startup()
{
    return (XORP_OK);
}

int
BGPMain::shutdown()
{
    return (XORP_OK);
}

void
BGPMain::status_change(ServiceBase*	service,
		       ServiceStatus	old_status,
		       ServiceStatus	new_status)
{
    UNUSED(service);
    UNUSED(old_status);
    UNUSED(new_status);
}

void
BGPMain::tree_complete()
{
}

void
BGPMain::updates_made()
{
}

bool
BGPMain::interface_address4(const IPv4& /*address*/) const
{
    return false;
}

bool
BGPMain::interface_address6(const IPv6& /*address*/) const
{
    return false;
}

bool
BGPMain::interface_address_prefix_len4(const IPv4& /*address*/,
				       uint32_t& /*prefix_len*/) const
{
    return false;
}

bool
BGPMain::interface_address_prefix_len6(const IPv6& /*address*/,
				       uint32_t& /*prefix_len*/) const
{
    return false;
}

void
BGPMain::main_loop()
{
}	

void 
BGPMain::local_config(const uint32_t&, const IPv4&, bool)
{
}

/*
** Callback registered with the asyncio code.
*/
void 
BGPMain::connect_attempt(XorpFd, IoEventType, string, uint16_t)
{
}

void
BGPMain::attach_peer(BGPPeer*)
{
}

void
BGPMain::detach_peer(BGPPeer*)
{
}	

/*
** Find this peer if it exists.
*/
BGPPeer *
BGPMain::find_peer(const Iptuple&)
{
    return 0;
}

bool
BGPMain::create_peer(BGPPeerData *)
{
    return false;
}

bool
BGPMain::delete_peer(const Iptuple&)
{
    return false;
}

bool
BGPMain::enable_peer(const Iptuple&)
{
    return false;
}

bool
BGPMain::disable_peer(const Iptuple&)
{
    return false;
}

bool
BGPMain::register_ribname(const string&)
{
    return false;
}

XorpFd
BGPMain::create_listener(const Iptuple&)
{
    XorpFd tmpfd;

    return (tmpfd);
}

LocalData*
BGPMain::get_local_data()
{
    return 0;
}

void
BGPMain::start_server(const Iptuple&)
{
}

void
BGPMain::stop_server(const Iptuple&)
{
}

void
BGPMain::stop_all_servers()
{
}
