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

#ident "$XORP: xorp/libxipc/xrl_cmd_map.cc,v 1.6 2003/03/05 19:35:58 hodson Exp $"

#include "xrl_module.h"
#include "config.h"
#include "libxorp/debug.h"
#include "xrl_cmd_map.hh"

bool
XrlCmdMap::add_handler(const XrlCmdEntry& cmd)
{
    if (get_handler(cmd.name.c_str())) {
	debug_msg("XrlCmdMap::add_handler for \"%s\" failed"
		  ": a handler already exists\n", cmd.name.c_str());
	return false;
    }
    _cmd_map.insert(CmdMap::value_type(cmd.name, cmd));
    return true;
}

bool
XrlCmdMap::add_handler(const string& cmd, const XrlRecvCallback& rcb)
{
    return add_handler(XrlCmdEntry(cmd, rcb));
}

const XrlCmdEntry*
XrlCmdMap::get_handler(const string& name) const
{
    CmdMap::const_iterator c = _cmd_map.find(name);
    if (c == _cmd_map.end())
	return 0;
    return &c->second;
}

bool
XrlCmdMap::remove_handler(const string& name)
{
    CmdMap::iterator c = _cmd_map.find(name);
    if (c == _cmd_map.end())
	return false;
    _cmd_map.erase(c);
    return true;
}

const XrlCmdEntry*
XrlCmdMap::get_handler(uint32_t index) const
{
    CmdMap::const_iterator c;
    for (c = _cmd_map.begin(); c != _cmd_map.end(); c++, index--) {
	if (index == 0) {
	    return &c->second;
	}
    }
    return NULL;
}

uint32_t
XrlCmdMap::count_handlers() const
{
    return _cmd_map.size();
}

void
XrlCmdMap::get_command_names(list<string>& out) const
{
    CmdMap::const_iterator ci;
    for (ci = _cmd_map.begin(); ci != _cmd_map.end(); ++ci) {
	out.push_back(ci->first);
    }
}

void
XrlCmdMap::finalize()
{
}
