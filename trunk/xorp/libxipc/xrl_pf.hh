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

// $XORP: xorp/libxipc/xrl_pf.hh,v 1.23 2004/06/10 22:41:11 hodson Exp $

// XRL Protocol Family Header

#ifndef __LIBXIPC_XRL_PF_HH__
#define __LIBXIPC_XRL_PF_HH__

#include <string>

#include "libxorp/eventloop.hh"
#include "libxorp/timer.hh"
#include "libxorp/selector.hh"
#include "libxorp/exceptions.hh"

class Xrl;
class XrlError;
class XrlArgs;
class XrlDispatcher;

class XrlPFConstructorError : public XorpReasonedException
{
public:
    XrlPFConstructorError(const char* file, size_t line, const string& reason)
	: XorpReasonedException("XrlPFConstructorError", file, line, reason)
    {}
};

class XrlPFListener
{
public:
    XrlPFListener(EventLoop& e, XrlDispatcher* d = 0);
    virtual ~XrlPFListener();

    virtual const char*	address() const = 0;

    virtual const char*	protocol() const = 0;

    bool set_dispatcher(const XrlDispatcher* d);

    inline const XrlDispatcher* dispatcher() const	{ return _dispatcher; }
    inline EventLoop& eventloop() const			{ return _eventloop; }

    virtual bool response_pending() const = 0;

protected:
    EventLoop& _eventloop;
    const XrlDispatcher* _dispatcher;
};


// ----------------------------------------------------------------------------
// XrlPFSender

class XrlPFSender
{
public:
    typedef
    XorpCallback2<void, const XrlError&, XrlArgs*>::RefPtr SendCallback;

public:
    XrlPFSender(EventLoop& e, const char* address);
    virtual ~XrlPFSender();

    /**
     * Send an Xrl.
     *
     * This method attempts to perform the sender side processing of an XRL.
     *
     * If a direct_call the method will return true or false to indicate
     * success to the caller.  If not a direct call, a failure will be
     * communicated via a callback since there's no way to get the information
     * directly back to the caller.
     *
     * @param xrl XRL to be sent.
     * @param direct_call indication of whether the caller is on the stack.
     * @param cb Callback to be invoked with result.
     *
     */
    virtual bool send(const Xrl& 		xrl,
		      bool 			direct_call,
		      const SendCallback& 	cb) = 0;
    virtual bool sends_pending() const = 0;
    virtual const char* protocol() const = 0;
    virtual bool alive() const = 0;

    inline const string& address() const		{ return _address; }
    inline EventLoop& eventloop() const			{ return _eventloop; }

private:
    XrlPFSender(const XrlPFSender&);			// Not implemented
    XrlPFSender& operator=(const XrlPFSender&);		// Not implemented

protected:
    EventLoop& _eventloop;
    string _address;
};

#endif // __LIBXIPC_XRL_PF_HH__
