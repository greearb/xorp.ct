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

#ident "$XORP: xorp/libxipc/finder_client.cc,v 1.1.1.1 2002/12/11 23:56:03 hodson Exp $"

#include <sys/types.h>
#include <unistd.h>

#include <algorithm>
#include <functional>

#include "finder_module.h"
#include "config.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "finder_client.hh"

// ----------------------------------------------------------------------------
// FinderRegistration - container for pending registration result

struct FinderRegistration {
    FinderRegistration(FinderClientCallback cb, void* userdata, 
		       const char *name, const char* value = "") :
	_name(name), _value(value), _callback(cb), _userdata(userdata) {}
    FinderRegistration() {}
    const char*			name() const { return _name.c_str(); }
    const char* 		value() const { return _value.c_str(); }
    FinderClientCallback	callback() const { return _callback; }
    void* 			userdata() const { return _userdata; }
    uint16_t 			seqno() const { return _seqno; }
    uint16_t			set_seqno(uint16_t s) { 
	gettimeofday(&_issued, NULL);
	return _seqno = s; 
    }
    const timeval& 		issued() const { return _issued; }
private:
    string			_name;
    string			_value;
    FinderClientCallback	_callback;
    void*			_userdata;
    uint16_t			_seqno;
    timeval			_issued;
};

// ----------------------------------------------------------------------------
// Constants

static const int CONNECT_RETRY_MILLISEC = 250;
static const int CONNECT_REAP_MILLISEC  = 2500;

// ----------------------------------------------------------------------------
// Authentication 

const char*
FinderClient::set_auth_key(const char* key) {
    return _connection->set_auth_key(key);
}

const char*
FinderClient::auth_key() {
    return _connection->auth_key();
}

// ----------------------------------------------------------------------------
// Protocol Related

void
FinderClient::send_register(FinderRegistration& r) {
    FinderMessage m;

    _connection->prepare_message(m, REGISTER, r.name(), r.value());
    if (_connection->write_message(m) <= 0) {
	restart_connection();
	return;
    }

    r.set_seqno(m.seqno());
    _registers.push_back(r);
    debug_msg(m.str().c_str());
}

void
FinderClient::send_resolve(FinderRegistration& r) {
    FinderMessage m;

    _connection->prepare_message(m, LOCATE, r.name());
    if (_connection->write_message(m) <= 0) {
	restart_connection();
	return;
    }

    r.set_seqno(m.seqno());
    _queries.push_back(r);

    debug_msg(m.str().c_str());
}

void 
FinderClient::send_unregister(FinderRegistration& r) {
    FinderMessage m;

    _connection->prepare_message(m, UNREGISTER, r.name());
    if (_connection->write_message(m) <= 0) {
	restart_connection();
	return;
    }
    r.set_seqno(m.seqno());
    _unregisters.push_back(r);
}

void
FinderClient::send_bye() {
    FinderMessage m;
    _connection->prepare_message(m, BYE);
    _connection->write_message(m);
}

void
FinderClient::hello_handler(const FinderMessage& m) {
    struct timeval now;
    gettimeofday(&now, NULL);
    debug_msg("got hello (seqno %d) %ld.%06ld\n", m.seqno(), 
	      now.tv_sec, now.tv_usec);
    UNUSED(m);
}

void
FinderClient::bye_handler(const FinderMessage& msg) {
    if (msg.is_ack() == false) {
	FinderMessage ack;
	_connection->prepare_ack(msg, ack);
	_connection->write_message(ack);
	restart_connection();
    }
}

void
FinderClient::register_handler(const FinderMessage& msg) {
    debug_msg(msg.str().c_str());
    if (msg.is_ack() == false) {
	debug_msg("Protocol error! - client received REGISTER.\n");
	return;
    }

    for(RLI r = _registers.begin(); r != _registers.end(); r++) {
	debug_msg("acked something %d %d\n", r->seqno(), msg.ackno());
	if (r->seqno() == msg.ackno()) {
	    _resolved[r->name()] = r->value();
	    if (r->callback())
		r->callback()(FC_OKAY, r->name(), r->value(), r->userdata());
	    _registers.erase(r);
	    break;
	}
    }
}

void
FinderClient::unregister_handler(const FinderMessage& msg) {
    debug_msg(msg.str().c_str());
    if (msg.is_ack() == false) {
	debug_msg("Protocol error! - client received UNREGISTER.\n");
	return;
    }

    for(RLI r = _unregisters.begin(); r != _unregisters.end(); r++) {
	debug_msg("acked something %d %d", r->seqno(), msg.ackno());
	if (r->seqno() == msg.ackno()) {
	    if (r->callback())
		r->callback()(FC_OKAY, r->name(), r->value(), r->userdata());
	    _unregisters.erase(r);
	    break;
	}
    }
}

void
FinderClient::notify_handler(const FinderMessage& msg) {
    if (msg.is_ack() == true) {
	debug_msg("Protocol error! - client received NOTIFY_ACK.\n");
	return;
    }

    const char* name  = msg.get_arg(0);
    const char* value = msg.get_arg(1);
    _resolved[name] = value;

    for (RLI q = _queries.begin(); q != _queries.end(); q++) {
	if (q->name() == string(name)) {
	    if (q->callback()) 
		q->callback()(FC_OKAY, name, value, q->userdata());
	    _queries.erase(q);
	    q--;
	}
    }
}

void
FinderClient::locate_handler(const FinderMessage& msg) {
    if (msg.is_ack() != true) {
	debug_msg("Protocol error! - client receive LOCATE.\n");
    }
    // Nothing to do
}

void
FinderClient::error_handler(const FinderMessage& msg) {
    debug_msg("ERROR %s\n", msg.str().c_str());
    if (msg.is_ack() == true) {
	return;
    }

    uint16_t	seqno = (uint16_t)atoi(msg.get_arg(0));
    // Check if message relates to attempted registration...
    for (RLI r = _registers.begin(); r != _registers.end(); r++) {
	if (seqno == r->seqno()) {
	    if (r->callback())
		r->callback()(FC_ADD_FAILED, r->name(), NULL, r->userdata());
	    debug_msg("Register failed (\"%s\", \"%s\"):\n%s\n",
		      r->name(), r->value(), msg.get_arg(1));
	    _registers.erase(r);
	    return;
	}
    }

    // Check if message relates to lookup 
    for (RLI q = _queries.begin(); q != _queries.end(); q++) {
	if (seqno == q->seqno()) {
	    if (q->callback())
		q->callback()(FC_LOOKUP_FAILED, q->name(), NULL, 
			      q->userdata());
	    debug_msg("Look up failed (\"%s\"): %s\n", 
		      q->name(), msg.get_arg(1));
	    _queries.erase(q);
	    return;
	}
    }
}

void
FinderClient::receive_hook(int fd, SelectorMask sm, void* thunked_client) {
    FinderClient* c = reinterpret_cast<FinderClient*>(thunked_client);

    assert(c->_connection->descriptor() == fd);
    assert(sm == SEL_RD);

    c->_reaper_timer.schedule_after_ms(CONNECT_REAP_MILLISEC);

    FinderMessage msg;
    c->_connection->read_message(msg);

    switch (msg.type()) {
    case HELLO:
	c->hello_handler(msg); 
	break;
    case BYE:
	c->bye_handler(msg);
	return;
    case REGISTER:
	c->register_handler(msg);
	break;
    case UNREGISTER:
	c->unregister_handler(msg);
	break;
    case NOTIFY:
	c->notify_handler(msg);
	break;
    case LOCATE:
	c->locate_handler(msg);
	break;
    case ERROR:
	c->error_handler(msg);
	break;
    }

    if (msg.is_ack() == false && c->_connection != 0) {
	FinderMessage ack;

	c->_connection->prepare_ack(msg, ack);
	if (c->_connection->write_message(ack) <= 0) {
	    c->restart_connection();
	    return;
	}
    }
}

// ----------------------------------------------------------------------------
// Registration Related

void 
FinderClient::add(const char* name, const char* value, 
		  FinderClientCallback cb, void* userdata) {
    if (_connection) {
	FinderRegistration r(cb, userdata, name, value);
	string s(name);
	_registered[s] = r;
	send_register(r);
    } else {
	cb(FC_NO_SERVER, name, value, userdata);
    }
}

void
FinderClient::lookup(const char*          name, 
		     FinderClientCallback cb, 
		     void*                userdata) {
    debug_msg("lookup %s\n", name);
    RMI i = _resolved.find(name);
    if (i != _resolved.end()) {
	const string& v = i->second;
	if (cb != NULL) 
	    cb(FC_OKAY, name, v.c_str(), userdata);
	return;
    }

    if (connected()) {
	FinderRegistration r(cb, userdata, name);
	send_resolve(r);
    } else {
	cb(FC_NO_SERVER, name, 0, userdata);
    }
}

void
FinderClient::remove(const char*          name, 
		     FinderClientCallback cb, 
		     void*                userdata) {
    RI i = _registered.find(name);
    if (i != _registered.end()) {
	_registered.erase(i);
    }

    RMI r = _resolved.find(name);
    if (r != _resolved.end()) {
	_resolved.erase(r);
    }

    if (_connection) {
	FinderRegistration r(cb, userdata, name);
	send_unregister(r);
    } else if (cb != NULL) {
	cb(FC_OKAY, name, NULL, userdata);
    }
}

void
FinderClient::invalidate(const string& name)
{
    /* klduge */
    map<string,string>::iterator i = _resolved.find(name);
    if (i != _resolved.end()) {
	    _resolved.erase(i);
	    debug_msg("Invalidating %s\n", name.c_str());
    }
}

// ----------------------------------------------------------------------------
// Connection related

void
FinderClient::initiate_hook(void *thunked_client) {
    FinderClient *c = reinterpret_cast<FinderClient*>(thunked_client);
    
    assert(c->_connection == NULL);
    c->_connection = 
	FinderTCPClientIPCFactory::create(c->_finder_host.c_str(), 
					  c->_finder_port);
    if (c->_connection) {
	struct timeval now;
	gettimeofday(&now, NULL);
	debug_msg("Connected to Finder. Re-registering...%ld.%06ld\n",
		  now.tv_sec, now.tv_usec);

	// Connect succeed
	c->_event_loop.add_selector(c->_connection->descriptor(),
				     SEL_RD, receive_hook, thunked_client);

	// Send everything that has been registered with client
	for (RI r = c->_registered.begin(); r != c->_registered.end(); r++) {
	    c->send_register(r->second);
	}

	// Start _reaper_timer as one way to spot when server has died
	c->_reaper_timer = c->_event_loop.new_oneoff_after_ms
	    (CONNECT_REAP_MILLISEC, reap_hook, thunked_client);
	c->_connect_timer.clear(); // timer free'd when this fn returns
    } else {
	debug_msg("Failed to connect to Finder. Retrying...\n");
	// Connect failed try again 
	c->_connect_timer = c->_event_loop.new_oneoff_after_ms
	    (CONNECT_RETRY_MILLISEC, initiate_hook, thunked_client);
    }
}

void
FinderClient::reap_hook(void* thunked_client)
{
    FinderClient* c = reinterpret_cast<FinderClient*>(thunked_client);
    if (c->_connection == 0) {
	initiate_hook(thunked_client);
    } else {
	c->restart_connection();
    }
}

void 
FinderClient::connect() {
    if (!connected()) {
	start_connection();
    }
}

void
FinderClient::start_connection() {
    assert(_connection == 0);
    initiate_hook(reinterpret_cast<void*>(this));
}

void
FinderClient::restart_connection() {
    assert(_connection != 0);
    debug_msg("Restarting connection %p\n", this);
    terminate_connection();
    start_connection();
}

void
FinderClient::terminate_connection() {
    assert(_connection != 0);
    _event_loop.remove_selector(_connection->descriptor());
    delete _connection;
    _connection = 0;

    // Flush resolved entries
    _resolved.clear();

    // Flush pending registration state
    _registers.clear();

    // Flush unanswered unregistration state
    _unregisters.clear();

    // Announce lookup failures for pending queries
    while (_queries.empty() == false) {
	FinderRegistration& q = _queries.front();
	q.callback()(FC_NO_SERVER, q.name(), NULL, q.userdata());
	_queries.erase(_queries.begin());
    }
}

// ----------------------------------------------------------------------------
// Constructor/Destructor

FinderClient::FinderClient(EventLoop& e, const char* addr, uint16_t port) 
    : _finder_host(addr), _finder_port(port), _connection(NULL),
      _event_loop(e)
{
    connect();
}

FinderClient::~FinderClient() {
    if (connected()) {
	send_bye();
	_event_loop.remove_selector(_connection->descriptor());
	delete _connection;
    }
}
