// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP$

#ifndef __POLICY_BACKEND_INSTRUCTION_BASE_HH__
#define __POLICY_BACKEND_INSTRUCTION_BASE_HH__

#include "instr_visitor.hh"

/**
 * @short Base class for an instruction.
 *
 * An instruction is an operation a policy filter may execute. Such as pushing
 * an element on the stack.
 */
class Instruction {
public:
    virtual ~Instruction() {}

    /**
     * Pass the current instruction to the visitor.
     *
     * @param v visitor to use on instruction.
     */
    virtual void accept(InstrVisitor& v) = 0;
};

// macro ugliness to make instruction visitable [usable by visitor].
#define INSTR_VISITABLE() \
void accept(InstrVisitor& v) { \
    v.visit(*this); \
}

#endif // __POLICY_BACKEND_INSTRUCTION_BASE_HH__
