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

// $XORP: xorp/libxipc/finder_server.hh,v 1.9 2003/05/22 16:52:15 hodson Exp $

#ifndef __LIBXIPC_FINDER_SERVER_HH__
#define __LIBXIPC_FINDER_SERVER_HH__

#include "libxorp/xlog.h"

#include "finder.hh"
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
    /**
     * Constructor taking user supplied arguments for Finder request socket
     *
     * @param e EventLoop to be used by Finder components.
     * @param if_addr interface address to bind Finder request socket to.
     * @param port port to bind Finder request socket to.
     */
    FinderServer(EventLoop& e, IPv4 if_addr, uint16_t port)
	: _addr(if_addr), _port(port)
    {
	initialize(e);
    }

    /**
     * Constructor using default values for Finder request socket.
     *
     * The default interface is first useable interface on the host.
     * The default port is specified by @ref FINDER_NG_TCP_DEFAULT_PORT.
     */
    FinderServer(EventLoop& e)
    {
	_addr = IPv4(if_get_preferred());
	_port = FINDER_NG_TCP_DEFAULT_PORT;
	initialize(e);
    }

    /**
     * Destructor
     */
    ~FinderServer()
    {
	delete _fxt;
	delete _ft;
	delete _f;
    }

    /**
     * Accessor to the number of connections the Finder has.
     */
    inline uint32_t connection_count() const
    {
	return _f ? _f->messengers() : 0;
    }

    /**
     * Accessor to the interface address the Finder is bound to.
     */
    inline IPv4 addr() const		{ return _addr; }

    /**
     * Accessor to the port the Finder is bound to.
     */
    inline uint16_t port() const	{ return _port; }

protected:
    void initialize(EventLoop& e)
    {
	_f = new Finder(e);
	_ft = new FinderTcpListener(e, *_f, _f->commands(), _addr, _port);
	_fxt = new FinderXrlTarget(*_f);
	add_permitted_host(_addr);
	add_permitted_host(IPv4("127.0.0.1"));
    }
    
protected:
    Finder*		_f;
    FinderTcpListener*	_ft;
    FinderXrlTarget*	_fxt;
    IPv4		_addr;
    uint16_t		_port;
};

#endif // __LIBXIPC_FINDER_SERVER_HH__
