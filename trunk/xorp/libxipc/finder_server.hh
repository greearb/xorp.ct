// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/libxipc/finder_server.hh,v 1.11 2003/06/01 21:37:27 hodson Exp $

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
    FinderServer(EventLoop& e,
		 uint16_t   default_port = FINDER_DEFAULT_PORT,
		 IPv4	    default_interface = FINDER_DEFAULT_HOST)
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
    inline uint32_t connection_count() const	{ return _f.messengers(); }

    IPv4 addr() const { return _listeners.front()->address(); }
    uint16_t port() const { return _listeners.front()->port(); }

protected:
    EventLoop&		_e;
    Finder		_f;
    FinderXrlTarget	_fxt;
    Listeners		_listeners;
};

#endif // __LIBXIPC_FINDER_SERVER_HH__
