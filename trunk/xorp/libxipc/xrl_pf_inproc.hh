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

// $XORP: xorp/libxipc/xrl_pf_inproc.hh,v 1.2 2002/12/18 22:54:30 hodson Exp $

#ifndef __XRLPF_INPROC_HH__
#define __XRLPF_INPROC_HH__

#include "xrl_pf.hh"

// ----------------------------------------------------------------------------
// XRL Protocol Family : Intra Process communication

class XrlPFInProcListener : public XrlPFListener {
public:
    XrlPFInProcListener(EventLoop& e, XrlCmdMap* m = 0)
	throw (XrlPFConstructorError);
    ~XrlPFInProcListener();

    const char* address() const { return _address.c_str(); }
    const char* protocol() const { return _protocol; }

    const XrlError dispatch(const Xrl& request, XrlArgs& reply);

private:
    string _address;
    uint32_t _instance_no;

    static const char* _protocol;
    static uint32_t _next_instance_no;
};

class XrlPFInProcSender : public XrlPFSender {
public:
    XrlPFInProcSender(EventLoop& e, const char* address = NULL)
	throw (XrlPFConstructorError);
    ~XrlPFInProcSender();

    void send(const Xrl& x, const XrlPFSender::SendCallback& cb);
    bool sends_pending() const { return false; }
    static const char* protocol() { return _protocol; }

private:
    static const char* _protocol;
    uint32_t _listener_no;
};

#endif // __XRLPF_INPROC_HH__
