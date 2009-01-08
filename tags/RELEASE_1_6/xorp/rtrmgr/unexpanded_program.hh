// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/rtrmgr/unexpanded_program.hh,v 1.6 2008/10/02 21:58:26 bms Exp $

#ifndef __RTRMGR_UNEXPANDED_PROGRAM_HH__
#define __RTRMGR_UNEXPANDED_PROGRAM_HH__


class MasterConfigTreeNode;
class ProgramAction;

//
// We want to build a queue of programs, but the variables needed for those
// programs may depend on the results from previous programs.  So we need to
// delay expanding them until we're actually going to send the program
// request.
//

class UnexpandedProgram {
public:
    UnexpandedProgram(const MasterConfigTreeNode& node,
		      const ProgramAction& action);
    ~UnexpandedProgram();

    /**
     * Expand the variables in the unexpanded program, and create a
     * program string that we can actually send.
     * 
     * @param errmsg the error message (if error).
     * 
     * @return the string with the program that we can execute, or an
     * empty string if error.
     */
    string expand(string& errmsg) const;

    string str() const;

private:
    const MasterConfigTreeNode& _node;
    const ProgramAction& _action;
};

#endif // __RTRMGR_UNEXPANDED_PROGRAM_HH__
