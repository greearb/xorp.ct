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

// $XORP: xorp/libxipc/xrlpf.hh,v 1.21 2002/12/09 18:29:09 hodson Exp $

// XRL Protocol Family Header

#ifndef __XRLPF_HH__
#define __XRLPF_HH__

#include <string>
#include "xrl.hh"
#include "xrlcmdmap.hh"
#include "xuid.hh"

#include "libxorp/eventloop.hh"
#include "libxorp/timer.hh"
#include "libxorp/selector.hh"

// Constructor exception that should be used by classes derived from
// XrlPFListener and XrlPFSender.

struct XrlPFConstructorError : public exception {
    XrlPFConstructorError(const char* reason = "Not specified") 
	: _reason(reason) {} 
    const char* what() { return _reason; }
protected:
    const char* _reason;
};

class XrlPFListener {
public:
    XrlPFListener(EventLoop& e, XrlCmdMap* m = 0) 
	: _event_loop(e), _cmd_map(m) {}
    virtual ~XrlPFListener() {}
    virtual const char*	address() const = 0; 
    virtual const char*	protocol() const = 0; 

    // set_command_map adds table of request handlers.  Fails and
    // returns false if already assigned, true otherwise.
    bool  set_command_map(XrlCmdMap* m);
    const XrlCmdMap& command_map() const { 
	if (_cmd_map == 0) abort();
	return *_cmd_map; 
    }
    EventLoop& event_loop() const { return _event_loop; }

    struct Reply {
	XUID		xuid;
	XrlError	error_code;
	XrlArgs	response;

	Reply(const XUID& id, const XrlError& error, XrlArgs r = 0)
	    : xuid(id), error_code(error), response(r) {}
	~Reply() {}
    };
protected:
    EventLoop& _event_loop;
    const XrlCmdMap* _cmd_map;
};

// ----------------------------------------------------------------------------
// XrlPFSender

class XrlPFSender {
public:
    XrlPFSender(EventLoop& e, const char* address = "") 
	: _event_loop(e), _address(address) {}
    virtual ~XrlPFSender() {}
    
    typedef void (*SendCallback)(const XrlError& e,	
				 const Xrl& xrl,
				 XrlArgs* return_values,
				 void* thunk);

    virtual void send(const Xrl& x, SendCallback cb, void* cookie) = 0;
    virtual bool sends_pending() const = 0;

    const string&	address() const { return _address; }
    EventLoop& event_loop() const { return _event_loop; }

    struct Request {
	XrlPFSender*	parent;
	XUID		xuid;		// to match requests and responses
	Xrl		xrl;		
	SendCallback	callback;
	void*		thunk;
	XorpTimer	timeout;
	Request(XrlPFSender* p, const Xrl& x, SendCallback cb, void* cookie) 
	    : parent(p), xuid(), xrl(x), callback(cb), thunk(cookie) {}
	Request() {}
	bool operator==(const XUID& x) const { return xuid == x; }
    };

protected:
    EventLoop& _event_loop;
    string _address;
};

// ----------------------------------------------------------------------------
// Inline XrlPFListener Methods

inline bool
XrlPFListener::set_command_map(XrlCmdMap *m) {
    if (_cmd_map == 0) {
	_cmd_map = m;
	return true;
    }
    return false;
}

#endif // __XRLPF_HH__
