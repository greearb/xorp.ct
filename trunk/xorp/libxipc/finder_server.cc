// -*- c-basic-offset: 4 -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$Id$"

#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <errno.h>
#include <fcntl.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>

#include <algorithm>

#include "finder_module.h"
#include "config.h"
#include "libxorp/eventloop.hh"
#include "finder_server.hh"
#include "libxorp/debug.h"

// ----------------------------------------------------------------------------
// Constants

static const uint32_t HELO_MAX_RETRIES		= 3;
static const int32_t  HELO_TIMEOUT_MILLISEC	= 1000;

// ----------------------------------------------------------------------------
// FinderConnectionInfo - handles connections to server

class FinderConnectionInfo {
public:
    FinderConnectionInfo(FinderServer*	      s,
			 FinderTCPIPCService* c,
			 EventLoop*	      e);

    ~FinderConnectionInfo();

    const char* lookup_service(const char *name);
    bool        add_service(const char* name, const char* value);
    bool        remove_service(const char* name);

    size_t	service_count() { return _services.size(); }

    void	send_bye();

    // Helo timing and checking methods
    inline void
    reset_helo_timer() {
	_helo_timer.schedule_after_ms(HELO_TIMEOUT_MILLISEC);
	_helo_retries = 0;
    }

    inline void increment_retries() { _helo_retries++; }
    inline uint32_t retries() { return _helo_retries; }

protected:
    FinderServer* _server;
    FinderTCPIPCService* _connection;
    EventLoop*	_e;

    XorpTimer _helo_timer;
    uint32_t _helo_retries;   // number of unanswered helo's

    map<string, string>  _services;  // service-name to service
    typedef map<string, string>::iterator service_iterator;

private:
    void receive();
    static void	receive_hook(int fd, SelectorMask m, void* thunk_connection);

    bool helo_carry_on();
    static bool periodic_helo_hook(void* thunk_connection);

    static void write_failure(FinderConnectionInfo* fci);
};

FinderConnectionInfo::FinderConnectionInfo(FinderServer *s,
					   FinderTCPIPCService* c,
					   EventLoop* e) {
    _server = s;
    _connection = c;
    _helo_retries = 0;

    _e = e;
    _helo_timer = _e->new_periodic((int)HELO_TIMEOUT_MILLISEC,
				      periodic_helo_hook,
				      reinterpret_cast<void*>(this));

    _e->add_selector(_connection->descriptor(),
		     SEL_RD, receive_hook,
		     reinterpret_cast<void*>(this));
}

FinderConnectionInfo::~FinderConnectionInfo()
{
    _server->remove_connection(this);
    _e->remove_selector(_connection->descriptor());
    delete _connection;
}

// ----------------------------------------------------------------------------
// FinderConnectionInfo service mapping methods

const char*
FinderConnectionInfo::lookup_service(const char *name)
{
    service_iterator si = _services.find(name);
    if (si != _services.end()) {
	return si->second.c_str();
    } else {
	return 0;
    }
}

bool
FinderConnectionInfo::add_service(const char* name, const char* value)
{
    if (lookup_service(name) != 0) {
	return false;
    }
    _services[name] = value;
    debug_msg("Adding (%p) \"%s\" -> \"%s\"\n",
	      reinterpret_cast<void*>(this), name, value);
    return true;
}

bool
FinderConnectionInfo::remove_service(const char* name)
{
    service_iterator si = _services.find(name);
    if (si != _services.end()) {
	debug_msg("Removing (%p): \"%s\" \"%s\"\n",
		  reinterpret_cast<void*>(this),
		  si->first.c_str(), si->second.c_str());
	_services.erase(si);
	return true;
    }
    return false;
}

// ----------------------------------------------------------------------------
// FinderConnectionInfo receive methods

void
FinderConnectionInfo::receive()
{
    FinderMessage       msg, ack, err;

    if (_connection->read_message(msg) == false) {
	debug_msg("failed to read message ?\n");
	return;
    } else if (msg.is_ack()) {
	if (msg.type() == HELLO) {
	    debug_msg("got hello ack\n");
	    reset_helo_timer();
	}
	return;
    }

    debug_msg("%s\n", msg.str().c_str());

    switch (msg.type()) {
    case HELLO:
	break;
    case BYE:
	debug_msg("Got bye\n");
	delete this;
	return;
	break;
    case REGISTER:
	if (add_service(msg.get_arg(0), msg.get_arg(1)) == false) {
	    string reason = string("Register failed:\"") +
		string(msg.get_arg(0)) +
		string("\" already registered or invalid.");
	    debug_msg(reason.c_str());

	    _connection->prepare_error(msg, err, reason.c_str());
	    if (_connection->write_message(err) <= 0) {
		write_failure(this);
		return;
	    }
	}
	break;
    case UNREGISTER:
	if (remove_service(msg.get_arg(0)) == false) {
	    string reason = string("Unregister failed:\"") +
		string(msg.get_arg(0)) +
		string("\" either not registered OR registered to a different client.");
	    debug_msg(reason.c_str());
	    _connection->prepare_error(msg, err, reason.c_str());
	    if (_connection->write_message(err) <= 0) {
		write_failure(this);
		return;
	    }
	}
	break;
    case NOTIFY:
	_connection->prepare_error(msg, err, "Protocol error: "
				      "server received NOTIFY.");
	if (_connection->write_message(err) <= 0) {
	    write_failure(this);
	    return;
	}
	break;
    case LOCATE:
	if (const char* r = _server->lookup_service(msg.get_arg(0))) {
	    FinderMessage reply;
	    _connection->prepare_message(reply, NOTIFY, msg.get_arg(0), r);
	    if (_connection->write_message(reply) <= 0) {
		write_failure(this);
		return;
	    }
	} else {
	    _connection->prepare_error(msg, err, "LOCATE failed.");
	    if (_connection->write_message(err) <= 0) {
		write_failure(this);
		return;
	    }
	}
	break;
    case ERROR:
	debug_msg("ERROR received: %s\n", msg.get_arg(0));
	return;
    }

    // Send ack
    _connection->prepare_ack(msg, ack);
    if (_connection->write_message(ack) <= 0) {
	write_failure(this);
	return;
    }
}

void
FinderConnectionInfo::receive_hook(int fd, SelectorMask m, void *thunk_fci)
{
    FinderConnectionInfo* fci =
	reinterpret_cast<FinderConnectionInfo*>(thunk_fci);
    assert(fd == fci->_connection->descriptor());
    assert(m == SEL_RD);
    debug_msg("receive hook\n");
    fci->receive();
}

void
FinderConnectionInfo::send_bye()
{
    FinderMessage m;
    _connection->prepare_message(m, BYE);
    _connection->write_message(m);
}

// ----------------------------------------------------------------------------
// FinderConnectionInfo helo timeout methods

bool
FinderConnectionInfo::helo_carry_on()
{
    increment_retries();
    if (retries() < HELO_MAX_RETRIES) {
	FinderMessage m;
	_connection->prepare_message(m, HELLO);
	debug_msg("Sending hello %p\n", this);
	if (_connection->write_message(m) > 0) {
	    return true;
	}
    }
    return false;
}

bool
FinderConnectionInfo::periodic_helo_hook(void *thunk_fci)
{
    FinderConnectionInfo* fci =
	reinterpret_cast<FinderConnectionInfo*>(thunk_fci);

    bool carry_on = fci->helo_carry_on();
    if (carry_on == false)
	delete fci;
    return carry_on;
}

void
FinderConnectionInfo::write_failure(FinderConnectionInfo *fci)
{
    debug_msg("write failure on connection %p with %u services\n",
	      fci, (uint32_t)fci->service_count());
    delete fci;
}

// ----------------------------------------------------------------------------
// Server construction and destruction

FinderServer::FinderServer(EventLoop& e, int port)
    throw (FinderTCPServerIPCFactory::FactoryError)
    : _event_loop(e),
      _ipc_factory(FinderTCPServerIPCFactory(e.selector_list(),
					     connect_hook,
					     reinterpret_cast<void*>(this),
					     port))
{}

FinderServer::FinderServer(EventLoop& e, const char* key, int port)
    throw (FinderTCPServerIPCFactory::FactoryError)
    : _event_loop(e),
      _ipc_factory(FinderTCPServerIPCFactory(e.selector_list(),
					     connect_hook,
					     reinterpret_cast<void*>(this),
					     port))
{
    set_auth_key(key);
}

FinderServer::~FinderServer()
{
    byebye_connections();
}

// ----------------------------------------------------------------------------
// Authentication related methods

const char*
FinderServer::set_auth_key(const char*)
{
    debug_msg("FinderServer::set_auth_key unimplemented\n");
    return 0;
}

// ----------------------------------------------------------------------------
// Connection initialization and termination

void
FinderServer::add_connection(FinderConnectionInfo *fci)
{
    _connections.push_back(fci);
}

void
FinderServer::connect_hook(FinderTCPIPCService *c, void* thunked_server)
{
    FinderServer* server = reinterpret_cast<FinderServer*>(thunked_server);
    FinderConnectionInfo* fci = new FinderConnectionInfo(server, c,
							 &server->_event_loop);
    server->add_connection(fci);
}

void
FinderServer::remove_connection(const FinderConnectionInfo* to_be_zapped)
{
    connection_iterator ci;
    for (ci = _connections.begin(); ci != _connections.end(); ci++) {
	if (*ci == to_be_zapped) {
	    _connections.erase(ci);
	    return;
	}
    }
    assert(ci != _connections.end());
}

void
FinderServer::byebye_connections()
{
    connection_iterator ci = _connections.begin() ++;
    while (ci != _connections.end()) {
	FinderConnectionInfo *fci = *ci;
	fci->send_bye();
	delete fci;
	ci = _connections.begin() ++;
    }
}

uint32_t
FinderServer::connection_count()
{
    return (uint32_t)_connections.size();
}

// ----------------------------------------------------------------------------
// Service location

const char*
FinderServer::lookup_service(const char* name)
{
    const char*	r = NULL;

    for (connection_iterator ci = _connections.begin();
	 r == NULL && ci != _connections.end(); ci++) {
	FinderConnectionInfo *fci = *ci;
	r = fci->lookup_service(name);
    }
    debug_msg("Resolving: \"%s\" \"%s\"\n",  name, (r) ? r : "<failed>");

    return r;
}


