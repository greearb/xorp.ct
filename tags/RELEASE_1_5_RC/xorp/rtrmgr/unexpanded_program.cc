// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/rtrmgr/unexpanded_program.cc,v 1.3 2007/02/16 22:47:26 pavlin Exp $"


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
