// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ident "$XORP: xorp/rtrmgr/unexpanded_program.cc,v 1.5 2008/07/23 05:11:45 pavlin Exp $"


#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"

#include "template_commands.hh"
#include "unexpanded_program.hh"


UnexpandedProgram::UnexpandedProgram(const MasterConfigTreeNode& node,
				     const ProgramAction& action) 
    : _node(node),
      _action(action)
{
}

UnexpandedProgram::~UnexpandedProgram()
{
}

/**
 * Expand expands the variables in the unexpanded program, and create a
 * program name that we can actually execute.
 */
string
UnexpandedProgram::expand(string& errmsg) const
{
    string request;

    if (_action.expand_program_variables(_node, request, errmsg) != XORP_OK) {
	debug_msg("Failed to expand program variables: %s\n", errmsg.c_str());
	return string("");
    }
    debug_msg("Program expanded to %s\n", request.c_str());

    return (request);
}

string
UnexpandedProgram::str() const
{
    return _action.str();
}
