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

// $XORP: xorp/libxipc/xrl_pf_sudp.hh,v 1.1 2002/12/14 23:43:02 hodson Exp $

#ifndef __XRLPF_SUDP_HH__
#define __XRLPF_SUDP_HH__

#include "xrl_pf.hh"

// ----------------------------------------------------------------------------
// XRL Protocol Family : Simplest UDP 

class XrlPFSUDPListener : public XrlPFListener {
public:
    XrlPFSUDPListener(EventLoop& e, XrlCmdMap* m = 0) 
	throw (XrlPFConstructorError);
    ~XrlPFSUDPListener();

    const char* address() const	{ return _address_slash_port.c_str(); }
    const char* protocol() const { return _protocol; }

private:
    const XrlError dispatch_command(const char* buf, XrlArgs& response);
    void send_reply(struct sockaddr* sa, const XrlError& e, const XUID& xuid,
		    const XrlArgs* response);

    void recv(int fd, SelectorMask m);

    int	_fd;
    string _address_slash_port;
    static const char* _protocol;
};

class XrlPFSUDPSender : public XrlPFSender {
public:
    XrlPFSUDPSender(EventLoop& e, const char* address = NULL) 
	throw (XrlPFConstructorError);
    virtual ~XrlPFSUDPSender();

    void send(const Xrl& x, const XrlPFSender::SendCallback& cb);
    bool sends_pending() const { return false; }
    static const char* protocol() { return _protocol; }
private:

    sockaddr_in		_destination;

    static const char* _protocol;

    static int sender_fd;     		// shared fd between all senders

    static void recv(int fd, SelectorMask m);

    void timeout_hook(XUID x);

    static int instance_count; 	
    static map<const XUID, Request> requests_pending; // demux help
};

#endif // __XRLPF_SUDP_HH__
