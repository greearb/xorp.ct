// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/dummy_main.cc,v 1.4 2002/12/09 18:28:41 hodson Exp $"

#include <sys/time.h>

#include "bgp_module.h"
#include "config.h"

#include "libxorp/selector.hh"
#include "libxorp/xlog.h"

#include "main.hh"
#include "path_attribute_list.hh"
#include "iptuple.hh"

EventLoop BGPMain::_eventloop;

BGPMain::BGPMain()
{
    _xrl_router = NULL;
}

BGPMain::~BGPMain() {
    /*
    ** Any xrl events added by the destruction of the rib_ipc_hander
    ** are soaked up before destroying the xrl_router.
    */
    if (_xrl_router != NULL)
	while(_xrl_router->pending())
	    get_eventloop()->run();
    
}

void
BGPMain::main_loop()
{
}	

void 
BGPMain::local_config(const uint32_t&, const IPv4&)
{
}

/*
** Callback registered with the asyncio code.
*/
void 
BGPMain::connect_attempt(int, SelectorMask,
			 struct in_addr, uint16_t)
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

void
BGPMain::accept_connection_from(BGPPeerData *)
{
}

int
BGPMain::create_listener(const Iptuple&)
{
    return 0;
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

void
BGPMain::add_update(BGPPeerData* , UpdatePacket* )
{
}

bool 
BGPMain::add_route(const OriginType,  const AsNum&,
		   const IPv4&, const IPv4Net&)
{
    return false;
}

bool 
BGPMain::delete_route(const IPv4Net&)
{
    return false;
}




