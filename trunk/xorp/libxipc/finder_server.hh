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

// $XORP: xorp/libxipc/finder_server.hh,v 1.7 2003/04/23 20:50:48 hodson Exp $

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
    FinderServer(EventLoop& e)
    {
	IPv4 	 bind_addr = IPv4(if_get_preferred());
	uint16_t bind_port = FINDER_NG_TCP_DEFAULT_PORT;
	_f = new Finder(e);
	_ft = new FinderTcpListener(e, *_f, _f->commands(),
				      bind_addr, bind_port);
	_fxt = new FinderXrlTarget(*_f);
	add_permitted_host(bind_addr);
	add_permitted_host(IPv4("127.0.0.1"));
    }
    ~FinderServer()
    {
	delete _fxt;
	delete _ft;
	delete _f;
    }
    inline uint32_t connection_count() const
    {
	return _f ? _f->messengers() : 0;
    }
protected:
    Finder*	       _f;
    FinderTcpListener* _ft;
    FinderXrlTarget*   _fxt; 
};

#endif // __LIBXIPC_FINDER_SERVER_HH__
