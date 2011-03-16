// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "finder_module.h"
#include "finder_server.hh"

FinderServer::FinderServer(EventLoop& e,
			   IPv4	      default_interface,
			   uint16_t   default_port)
    throw (InvalidAddress, InvalidPort)
    : _e(e), _f(e), _fxt(_f)
{
    char* value;
    IPv4 finder_addr = default_interface;
    uint16_t finder_port = default_port;

    // Set the finder server address from the environment variable if it is set
    value = getenv("XORP_FINDER_SERVER_ADDRESS");
    if (value != NULL) {
	try {
	    IPv4 ipv4(value);
	    if (! ipv4.is_unicast()) {
		XLOG_ERROR("Failed to change the Finder server address to %s",
			   ipv4.str().c_str());
	    } else {
		finder_addr = ipv4;
	    }
	} catch (const InvalidString& e) {
	    UNUSED(e);
	    XLOG_ERROR("Invalid \"XORP_FINDER_SERVER_ADDRESS\": %s",
		       e.str().c_str());
	}
    }

    // Set the finder server port from the environment variable if it is set
    value = getenv("XORP_FINDER_SERVER_PORT");
    if (value != NULL) {
	int port = atoi(value);
	if (port <= 0 || port > 65535) {
	    XLOG_ERROR("Invalid \"XORP_FINDER_SERVER_PORT\": %s", value);
	} else {
	    finder_port = port;
	}
    }

    add_binding(finder_addr, finder_port);

    //
    // XXX: if we want to bind to the command-line specified (or the default)
    // values as well, then uncomment the following.
    //
    // if ((finder_addr != default_interface) || (finder_port != default_port))
    //	  add_binding(default_interface, default_port);

    // Permit connections from local addresses
    vector<IPv4> addrs;
    get_active_ipv4_addrs(addrs);
    vector<IPv4>::const_iterator i;
    for (i = addrs.begin(); i != addrs.end(); i++) {
	add_permitted_host(*i);
    }
}

FinderServer::~FinderServer()
{
    while (_listeners.empty() == false) {
	delete _listeners.front();
	_listeners.pop_front();
    }
}

bool
FinderServer::add_binding(IPv4 addr, uint16_t port)
    throw (InvalidAddress, InvalidPort)
{
    Listeners::const_iterator i = _listeners.begin();
    while (i != _listeners.end()) {
	FinderTcpListener* pl = *i;
	if (pl->address() == addr && pl->port() == port)
	    return false;
	i++;
    }
    try {
        _listeners.push_back(
	    new FinderTcpListener(_e, _f, _f.commands(), addr, port)
	    );
    } catch (const bad_alloc&) {
	return false;
    }
    // XXX we'd probably be better to leave permits alone here
    add_permitted_host(addr);
    return true;
}

bool
FinderServer::remove_binding(IPv4 addr, uint16_t port)
{
    Listeners::iterator i = _listeners.begin();
    while (i != _listeners.end()) {
	FinderTcpListener* pl = *i;
	if (pl->address() == addr && pl->port() == port) {
	    delete *i;
	    _listeners.erase(i);
	    return true;
	}
    }

    return false;
}
