// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/libxipc/finder_server.hh,v 1.19 2008/07/23 05:10:42 pavlin Exp $

#ifndef __LIBXIPC_FINDER_SERVER_HH__
#define __LIBXIPC_FINDER_SERVER_HH__

#include <list>

#include "libxorp/xlog.h"

#include "finder.hh"
#include "finder_constants.hh"
#include "finder_tcp_messenger.hh"
#include "finder_xrl_target.hh"
#include "permits.hh"
#include "sockutil.hh"

/**
 * A wrapper class for the components within a Finder.
 *
 * Instantiates a Finder object and IPC infrastructure for Finder to accept
 * accept incoming connections.
 */
class FinderServer {
public:
    typedef list<FinderTcpListener*> Listeners;

public:

    /**
     * Constructor
     */
    FinderServer(EventLoop& e, IPv4 default_interface, uint16_t default_port)
	throw (InvalidAddress, InvalidPort);

    /**
     * Destructor
     */
    ~FinderServer();

    /**
     * Add an additional interface and port to accept connections on.
     *
     * @return true on success, false if binding already exists or cannot be
     * instantiated.
     */
    bool add_binding(IPv4 addr, uint16_t port)
	throw (InvalidAddress, InvalidPort);

    /**
     * Remove an interface binding that was added by calling add_binding.
     * @return true on success, false if binding does not exist or was
     * not added by add_binding.
     */
    bool remove_binding(IPv4 addr, uint16_t port);

    /**
     * Accessor to the number of connections the Finder has.
     */
    uint32_t connection_count() const	{ return _f.messengers(); }

    IPv4 addr() const { return _listeners.front()->address(); }
    uint16_t port() const { return _listeners.front()->port(); }

protected:
    EventLoop&		_e;
    Finder		_f;
    FinderXrlTarget	_fxt;
    Listeners		_listeners;
};

#endif // __LIBXIPC_FINDER_SERVER_HH__
