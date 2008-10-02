// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008 XORP, Inc.
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

// $XORP: xorp/libxipc/xrl_pf_unix.hh,v 1.3 2008/09/23 20:48:33 abittau Exp $

#ifndef __LIBXIPC_XRL_PF_UNIX_HH__
#define __LIBXIPC_XRL_PF_UNIX_HH__

#include "xrl_pf_stcp.hh"

class XrlPFUNIXListener : public XrlPFSTCPListener {
public:
    XrlPFUNIXListener(EventLoop& e, XrlDispatcher* xr = 0);
    ~XrlPFUNIXListener();

    const char* protocol() const;
    static void encode_address(string& address);
    static void decode_address(string& address);

    static const char*	_protocol;

private:
    string get_sock_path();
};

class XrlPFUNIXSender : public XrlPFSTCPSender {
public:
    XrlPFUNIXSender(EventLoop& e, const char* address = 0);

    const char* protocol() const;
    static const char* protocol_name();
};

#endif // __LIBXIPC_XRL_PF_UNIX_HH__
