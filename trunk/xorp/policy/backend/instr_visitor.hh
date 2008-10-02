// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/policy/backend/instr_visitor.hh,v 1.9 2008/08/06 08:22:20 abittau Exp $

#ifndef __POLICY_BACKEND_INSTR_VISITOR_HH__
#define __POLICY_BACKEND_INSTR_VISITOR_HH__

// XXX: not acyclic! [but better for compiletime "safety"].
class Push;
class PushSet;
class OnFalseExit;
class Load;
class Store;
class Accept;
class Reject;
class NaryInstr;
class Next;
class Subr;

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
    virtual void visit(Load&) = 0;
    virtual void visit(Store&) = 0;
    virtual void visit(Accept&) = 0;
    virtual void visit(Reject&) = 0;
    virtual void visit(NaryInstr&) = 0;
    virtual void visit(Next&) = 0;
    virtual void visit(Subr&) = 0;
};

#endif // __POLICY_BACKEND_INSTR_VISITOR_HH__
