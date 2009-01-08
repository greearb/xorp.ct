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

// $XORP: xorp/libxipc/xrl_pf_inproc.hh,v 1.22 2008/10/02 21:57:25 bms Exp $

#ifndef __LIBXIPC_XRL_PF_INPROC_HH__
#define __LIBXIPC_XRL_PF_INPROC_HH__

#include "xrl_pf.hh"

// ----------------------------------------------------------------------------
// XRL Protocol Family : Intra Process communication

class XrlPFInProcListener : public XrlPFListener {
public:
    XrlPFInProcListener(EventLoop& e, XrlDispatcher* xr = 0)
	throw (XrlPFConstructorError);
    ~XrlPFInProcListener();

    const char* address() const { return _address.c_str(); }
    const char* protocol() const { return _protocol; }

    bool response_pending() const;

private:
    string		_address;
    uint32_t		_instance_no;

    static const char*	_protocol;
    static uint32_t	_next_instance_no;
};

class XrlPFInProcSender : public XrlPFSender {
public:
    XrlPFInProcSender(EventLoop& e, const char* address = NULL)
	throw (XrlPFConstructorError);

    ~XrlPFInProcSender();

    bool send(const Xrl& 			x,
	      bool 				direct_call,
	      const XrlPFSender::SendCallback& 	cb);

    bool sends_pending() const			{ return false; }

    const char* protocol() const;

    static const char* protocol_name()		{ return _protocol; }

    bool alive() const;

private:
    static const char*	_protocol;
    uint32_t		_listener_no;
    ref_ptr<uint32_t>	_depth;
};

#endif // __LIBXIPC_XRL_PF_INPROC_HH__
