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

// $XORP: xorp/libxipc/finder_client.hh,v 1.1 2002/12/14 23:42:54 hodson Exp $

#ifndef __FINDER_CLIENT_HH__
#define __FINDER_CLIENT_HH__

#include <map>
#include <string>

#include "libxorp/timer.hh"
#include "libxorp/callback.hh"
#include "finder_ipc.hh"

class EventLoop;

struct FinderRegistration;

class FinderClient {
public:
    enum Error {
	FC_OKAY,
	FC_LOOKUP_FAILED,
	FC_ADD_FAILED, 
	FC_REMOVE_FAILED,
	FC_NO_SERVER
    };

    typedef
    XorpCallback3<void,Error,const char*,const char*>::RefPtr
    Callback;

public:
    
    FinderClient(EventLoop& e,
		 const char*	ipaddr = "localhost", 
		 uint16_t	port   = FINDER_TCP_DEFAULT_PORT);
    ~FinderClient();

    inline bool connected() const { return _connection != NULL; }
    void connect();

    void add(const char* name, const char* value, const Callback& cb);
    void lookup(const char* name, const Callback& cb);
    void remove(const char* name, const Callback& callback);

    void invalidate(const string& value);

    const char* set_auth_key(const char*);
    const char* auth_key();

    struct ClientConnectionError : public exception {
	ClientConnectionError(const char* reason = "Not specified") 
	    : _reason(reason) {}
        const char* what() { return _reason; }
    protected:
        const char* _reason;
    };
private:
    map<string, FinderRegistration> _registered;
    typedef map<string, FinderRegistration>::iterator RI;

    map<string, string> _resolved;
    typedef map<string,string>::iterator RMI;

    // Pending transactions
    list<FinderRegistration>	_queries;	// unresolved queries
    list<FinderRegistration>	_registers;	// unack'ed registrations
    list<FinderRegistration>	_unregisters;	// unack'ed unregister ops
    typedef list<FinderRegistration>::iterator RLI;

    string	 _finder_host;
    uint16_t	 _finder_port;

    FinderTCPIPCService* _connection;
    bool _got_hello;

    EventLoop& _event_loop;

    void send_register(FinderRegistration& r);
    void send_unregister(FinderRegistration& r);
    void send_resolve(FinderRegistration& r);
    void send_bye();

    void hello_handler(const FinderMessage& msg);
    void bye_handler(const FinderMessage& msg);
    void register_handler(const FinderMessage& msg);
    void unregister_handler(const FinderMessage& msg);
    void notify_handler(const FinderMessage& msg);
    void locate_handler(const FinderMessage& msg);
    void error_handler(const FinderMessage& msg);

    static void receive_hook(int fd, SelectorMask m, void *thunked_client);

    // Connection related methods, hooks, and timers
    void start_connection();
    void terminate_connection();
    void restart_connection(); 

    static void initiate_hook(void* thunked_client);
    XorpTimer _connect_timer; // Connection retry timer.

    static void reap_hook(void* thunked_client);
    XorpTimer _reaper_timer;  // If connection quiet too long, reaper kills it.
};

#endif // __FINDER_CLIENT_H__

