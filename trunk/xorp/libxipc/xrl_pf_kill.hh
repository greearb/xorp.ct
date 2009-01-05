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

// $XORP: xorp/libxipc/xrl_pf_kill.hh,v 1.10 2008/10/02 21:57:25 bms Exp $

#ifndef __LIBXIPC_XRL_PF_KILL_HH__
#define __LIBXIPC_XRL_PF_KILL_HH__

#include "xrl_pf.hh"

class XUID;

class XrlPFKillSender : public XrlPFSender {
public:
    XrlPFKillSender(EventLoop& e, const char* pid_str) throw (XrlPFConstructorError);
    virtual ~XrlPFKillSender();

    bool send(const Xrl& 			x,
	      bool 				direct_call,
	      const XrlPFSender::SendCallback& 	cb);

    bool sends_pending() const;

    bool alive() const;

    const char* protocol() const;

    static const char* protocol_name()		{ return _protocol; }

protected:
    int _pid;
    
    static const char _protocol[];
};

#endif // __LIBXIPC_XRL_PF_KILL_HH__
