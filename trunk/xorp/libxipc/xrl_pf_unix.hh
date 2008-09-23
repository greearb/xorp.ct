// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2008 XORP, Inc.
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

// $XORP: xorp/libxipc/xrl_pf_unix.hh,v 1.2 2008/09/23 19:58:18 abittau Exp $

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
