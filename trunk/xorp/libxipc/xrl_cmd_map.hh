// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/libxipc/xrl_cmd_map.hh,v 1.20 2008/07/23 05:10:45 pavlin Exp $

#ifndef __LIBXIPC_XRL_CMD_MAP_HH__
#define __LIBXIPC_XRL_CMD_MAP_HH__

#include <map>
#include <string>

#include "libxorp/callback.hh"
#include "xrl.hh"
#include "xrl_error.hh"

typedef
XorpCallback2<const XrlCmdError, const XrlArgs&, XrlArgs*>::RefPtr XrlRecvCallback;

struct XrlCmdEntry {
    XrlCmdEntry(const string& s, XrlRecvCallback cb) :
	_name(s), _cb(cb) {}

    const string& name() const { return _name; }

    const XrlCmdError dispatch(const XrlArgs& inputs, XrlArgs* outputs) const {
	return _cb->dispatch(inputs, outputs);
    }

protected:
    string		_name;
    XrlRecvCallback	_cb;
};

class XrlCmdMap {
public:
    typedef map<string, XrlCmdEntry> CmdMap;

public:
    XrlCmdMap(const string& name = "anonymous") : _name(name) {}
    virtual ~XrlCmdMap() {}

    const string& name() const { return _name; }

    virtual bool add_handler(const string& cmd, const XrlRecvCallback& rcb);

    virtual bool remove_handler (const string& name);

    const XrlCmdEntry* get_handler(const string& name) const;

    uint32_t count_handlers() const;

    const XrlCmdEntry* get_handler(uint32_t index) const;

    void get_command_names(list<string>& names) const;

    /**
     * Mark command map as finished.
     */
    virtual void finalize();

protected:
    bool add_handler (const XrlCmdEntry& c);

    XrlCmdMap(const XrlCmdMap&);		// not implemented
    XrlCmdMap& operator=(const XrlCmdMap&);	// not implemented

protected:
    const string _name;

    CmdMap _cmd_map;
};

#endif // __LIBXIPC_XRL_CMD_MAP_HH__
