// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/devnotes/template.cc,v 1.2 2003/01/16 19:08:48 mjh Exp $"

#include "finder_module.h"
#include "finder_server.hh"

FinderServer::FinderServer(EventLoop& e,
			   uint16_t   default_port,
			   IPv4	      default_interface)
    throw (InvalidAddress, InvalidPort)
    : _e(e), _f(e), _fxt(_f)
{
    add_binding(default_interface, default_port);
    uint32_t n = if_count();
    for (uint32_t i = 1; i <= n; i++) {
	string name;
	in_addr ia;
	uint16_t flags;
	if (if_probe(i, name, ia, flags)) {
	    add_permitted_host(IPv4(ia));
	}
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
