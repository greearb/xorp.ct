// -*- c-basic-offset: 4 -*-

// Copyright (c) 2001-2003 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the Xorp LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the Xorp LICENSE file; the license in that file is
// legally binding.

// $Id$

#ifndef __FINDER_SERVER_HH__
#define __FINDER_SERVER_HH__

#include <sys/time.h>

#include <map>
#include <list>
#include <string>

#include "config.h"

#include "finder_ipc.hh"
#include "libxorp/eventloop.hh"

class FinderConnectionInfo;

class FinderServer {
public:
    FinderServer(EventLoop& e, const char* auth_key,
		 int port = FINDER_TCP_DEFAULT_PORT)
	throw (FinderTCPServerIPCFactory::FactoryError);
    FinderServer(EventLoop& e, int port = FINDER_TCP_DEFAULT_PORT)
	throw (FinderTCPServerIPCFactory::FactoryError);
    ~FinderServer();

    const char*	set_auth_key(const char* key);
    const char*	auth_key();

    void	add_connection(FinderConnectionInfo* fci);
    void	remove_connection(const FinderConnectionInfo* fci);
    uint32_t	connection_count();

    const char*	lookup_service(const char* service);
protected:
    static void	connect_hook(FinderTCPIPCService *s, void* thunked_server);
    void	byebye_connections();

    EventLoop&			_event_loop;
    FinderTCPServerIPCFactory   _ipc_factory;
    string                      _auth_key;

    list<FinderConnectionInfo*> _connections;
    typedef list<FinderConnectionInfo*>::iterator connection_iterator;

    friend class FinderConnectionInfo;
};

// Compatibility shim

#include "libxorp/xlog.h"

#include "finder_ng.hh"
#include "finder_tcp_messenger.hh"
#include "finder_ng_xrl_target.hh"
#include "permits.hh"
#include "sockutil.hh"

class FinderNGServer {
public:
    FinderNGServer(EventLoop& e) {
	IPv4 	 bind_addr = IPv4(if_get_preferred());
	uint16_t bind_port = FINDER_NG_TCP_DEFAULT_PORT;
	_f = new FinderNG();
	_ft = new FinderNGTcpListener(e, *_f, _f->commands(),
				      bind_addr, bind_port);
	_fxt = new FinderNGXrlTarget(*_f);
	add_permitted_host(bind_addr);
	add_permitted_host(IPv4("127.0.0.1"));
    }
    ~FinderNGServer()
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
    FinderNG*		 _f;
    FinderNGTcpListener* _ft;
    FinderNGXrlTarget*	 _fxt; 
};

#endif // __FINDER_SERVER_HH__
