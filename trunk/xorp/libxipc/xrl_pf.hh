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

// $XORP: xorp/libxipc/xrl_pf.hh,v 1.7 2003/02/25 19:52:01 hodson Exp $

// XRL Protocol Family Header

#ifndef __XRLPF_HH__
#define __XRLPF_HH__

#include <string>
#include "xrl.hh"
#include "xuid.hh"

#include "libxorp/eventloop.hh"
#include "libxorp/timer.hh"
#include "libxorp/selector.hh"
#include "libxorp/exceptions.hh"

class XrlPFConstructorError : public XorpReasonedException {
public:
    XrlPFConstructorError(const char* file, size_t line, const string& reason)
	: XorpReasonedException("XrlPFConstructorError", file, line, reason)
    {}
};

class XrlCmdDispatcher;

class XrlPFListener {
public:
    XrlPFListener(EventLoop& e, XrlCmdDispatcher* d = 0)
	: _event_loop(e), _dispatcher(d) {}
    virtual ~XrlPFListener() {}
    virtual const char*	address() const = 0;
    virtual const char*	protocol() const = 0;

    inline bool set_dispatcher(XrlCmdDispatcher* d);
    inline const XrlCmdDispatcher* dispatcher() const { return _dispatcher; }

    EventLoop& event_loop() const { return _event_loop; }

    struct Reply {
	XUID		xuid;
	XrlError	error_code;
	XrlArgs	response;

	Reply(const XUID& id, const XrlError& error, XrlArgs r)
	    : xuid(id), error_code(error), response(r) {}
	~Reply() {}
    };
protected:
    EventLoop& _event_loop;
    const XrlCmdDispatcher* _dispatcher;
};

// ----------------------------------------------------------------------------
// XrlPFSender

class XrlPFSender {
public:
    XrlPFSender(EventLoop& e, const char* address = "")
	: _event_loop(e), _address(address) {}

    virtual ~XrlPFSender() {}

    typedef
    XorpCallback3<void, const XrlError&, const Xrl&, XrlArgs*>::RefPtr
    SendCallback;

    virtual void send(const Xrl& x, const SendCallback& cb) = 0;
    virtual bool sends_pending() const = 0;

    const string& address() const { return _address; }
    EventLoop& event_loop() const { return _event_loop; }

    struct Request {
	XrlPFSender*	parent;
	XUID		xuid;		// to match requests and responses
	Xrl		xrl;
	SendCallback	callback;
	XorpTimer	timeout;
	Request(XrlPFSender* p, const Xrl& x, const SendCallback& cb)
	    : parent(p), xuid(), xrl(x), callback(cb) {}
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
XrlPFListener::set_dispatcher(XrlCmdDispatcher* d)
{
    if (_dispatcher == 0) {
	_dispatcher = d;
	return true;
    }
    return false;
}

#endif // __XRLPF_HH__
