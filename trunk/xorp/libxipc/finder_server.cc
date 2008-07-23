// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/libxipc/finder_server.cc,v 1.16 2008/01/04 03:16:22 pavlin Exp $"

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
    } catch (const std::bad_alloc&) {
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
