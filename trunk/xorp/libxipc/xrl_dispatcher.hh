// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/libxipc/xrl_dispatcher.hh,v 1.11 2008/09/23 19:56:30 abittau Exp $

#ifndef __LIBXIPC_XRL_DISPATCHER_HH__
#define __LIBXIPC_XRL_DISPATCHER_HH__

#include "xrl_cmd_map.hh"

class XrlDispatcher : public XrlCmdMap {
public:
    struct XI {
	XI(const XrlCmdEntry* c) : _cmd(c), _new(true) {}

	Xrl		    _xrl;
	const XrlCmdEntry*  _cmd;
	bool		    _new;
    };

    XrlDispatcher(const char* class_name)
	: XrlCmdMap(class_name)
    {}
    virtual ~XrlDispatcher() {}

    virtual XI*	       lookup_xrl(const string& name) const;
    virtual XrlError   dispatch_xrl(const string& method_name,
				    const XrlArgs& in,
				    XrlArgs& out) const;
    XrlError	       dispatch_xrl_fast(const XI& xi, XrlArgs& out) const;
};

#endif // __LIBXIPC_XRL_DISPATCHER_HH__
