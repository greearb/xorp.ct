// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/dummy_main.cc,v 1.22 2007/12/10 23:26:25 mjh Exp $"

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
