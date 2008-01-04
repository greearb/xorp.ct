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

// $XORP: xorp/rtrmgr/unexpanded_program.hh,v 1.3 2007/02/16 22:47:26 pavlin Exp $

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
