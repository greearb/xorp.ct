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

#ifndef __POLICY_BACKEND_INSTR_VISITOR_HH__
#define __POLICY_BACKEND_INSTR_VISITOR_HH__


// XXX: not acyclic! [but better for compiletime "safety"].
class Push;
class PushSet;
class OnFalseExit;
class Regex;
class Load;
class Store;
class Accept;
class Reject;

class NaryInstr;

/**
 * @short Visitor pattern to traverse a structure of instructions.
 *
 * Inspired by Alexandrescu [Modern C++ Design].
 */
class InstrVisitor {
public:
    virtual ~InstrVisitor() {}

    virtual void visit(Push&) = 0;
    virtual void visit(PushSet&) = 0;

    virtual void visit(OnFalseExit&) = 0;

    virtual void visit(Regex&) = 0;

    virtual void visit(Load&) = 0;
    virtual void visit(Store&) = 0;

    virtual void visit(Accept&) = 0;
    virtual void visit(Reject&) = 0;

    virtual void visit(NaryInstr&) = 0;
};

#endif // __POLICY_BACKEND_INSTR_VISITOR_HH__
