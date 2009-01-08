// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/libxipc/xrl_pf_sudp.hh,v 1.26 2008/10/02 21:57:25 bms Exp $

#ifndef __LIBXIPC_XRL_PF_SUDP_HH__
#define __LIBXIPC_XRL_PF_SUDP_HH__

#include "xrl_pf.hh"

class XUID;

// ----------------------------------------------------------------------------
// XRL Protocol Family : Simplest UDP

class XrlPFSUDPListener : public XrlPFListener {
public:
    XrlPFSUDPListener(EventLoop& e, XrlDispatcher* xr = 0)
	throw (XrlPFConstructorError);
    ~XrlPFSUDPListener();

    const char* address() const			{ return _addr.c_str(); }
    const char* protocol() const		{ return _protocol; }

    bool response_pending() const;

private:
    const XrlError dispatch_command(const char* buf, XrlArgs& response);

    void send_reply(struct sockaddr_storage*	ss,
		    socklen_t			ss_len,
		    const XrlError&		e,
		    const XUID&			xuid,
		    const XrlArgs*		response);

    void recv(XorpFd fd, IoEventType type);

    XorpFd	_sock;
    string _addr;
    static const char* _protocol;
};

class XrlPFSUDPSender : public XrlPFSender {
public:
    XrlPFSUDPSender(EventLoop& e, const char* address = NULL)
	throw (XrlPFConstructorError);
    virtual ~XrlPFSUDPSender();

    bool send(const Xrl& 			x,
	      bool 				direct_call,
	      const XrlPFSender::SendCallback& 	cb);

    bool sends_pending() const;

    bool alive() const;

    const char* protocol() const;

    static const char* protocol_name()		{ return _protocol; }

protected:
    static void recv(XorpFd fd, IoEventType type);
    void timeout_hook(XUID x);

private:
    struct sockaddr_in		_destination;

    static const char* _protocol;
    static XorpFd sender_sock;  	// shared fd between all senders
    static int instance_count;
};

#endif // __LIBXIPC_XRL_PF_SUDP_HH__
