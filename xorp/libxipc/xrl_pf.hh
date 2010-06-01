// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2010 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/libxipc/xrl_pf.hh,v 1.34 2008/10/02 21:57:24 bms Exp $

// XRL Protocol Family Header

#ifndef __LIBXIPC_XRL_PF_HH__
#define __LIBXIPC_XRL_PF_HH__

#include <string>

#include "libxorp/eventloop.hh"
#include "libxorp/timer.hh"
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

    const XrlDispatcher* dispatcher() const	{ return _dispatcher; }
    EventLoop& eventloop() const		{ return _eventloop; }

    virtual bool response_pending() const = 0;

protected:
    EventLoop& _eventloop;
    const XrlDispatcher* _dispatcher;
};


// ----------------------------------------------------------------------------
// XrlPFSender

class XrlPFSender
    : public NONCOPYABLE
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

    virtual bool	sends_pending() const = 0;
    virtual const char*	protocol() const = 0;

    /**
     * Determine if the underlying transport is still open.
     *
     * @return true if the transport is alive.
     */
    virtual bool	alive() const = 0;

    // XXX Unfinished support for XRL batching.
    virtual void	batch_start() {}
    virtual void	batch_stop() {}

    const string& address() const		{ return _address; }
    EventLoop& eventloop() const		{ return _eventloop; }
    virtual void set_address(const char* a) { _address = a; }

protected:
    EventLoop& _eventloop;
    string _address;
};

#endif // __LIBXIPC_XRL_PF_HH__
