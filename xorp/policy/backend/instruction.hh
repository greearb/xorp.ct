// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/policy/backend/instruction.hh,v 1.14 2008/10/02 21:58:03 bms Exp $

#ifndef __POLICY_BACKEND_INSTRUCTION_HH__
#define __POLICY_BACKEND_INSTRUCTION_HH__

#include "libxorp/xorp.h"

#ifdef HAVE_REGEX_H
#  include <regex.h>
#else // ! HAVE_REGEX_H
#  ifdef HAVE_PCRE_H
#    include <pcre.h>
#  endif
#  ifdef HAVE_PCREPOSIX_H
#    include <pcreposix.h>
#  endif
#endif // ! HAVE_REGEX_H

#include "policy/common/element_base.hh"
#include "policy/common/operator_base.hh"
#include "policy/common/policy_exception.hh"
#include "policy/common/varrw.hh"

#include "instr_visitor.hh"
#include "instruction_base.hh"


/**
 * @short Push operation. Pushes a single element on the stack.
 *
 * The Push instruction owns the element.
 */
class Push :
    public NONCOPYABLE,
    public Instruction
{
public:
    /**
     * Element is owned by Push.
     * Caller must not delete element.
     *
     * @param e element associated with push.
     */
    Push(Element* e) : _elem(e) {}
    ~Push() { delete _elem; }

    // semicolon for kdoc
    INSTR_VISITABLE();

    /**
     * @return element associated with push.
     */
    const Element& elem() const { return *_elem; }

private:
    Element* _elem;
};

/**
 * @short Push a set on the stack.
 *
 * The difference with a normal push, is that the operation does does not own
 * the set element, but only contains the label of the set. The SetManager will
 * match the set label with the actual element. [It is much like containing a
 * pointer, and the SetManager deals with dereference].
 *
 * SetManager acts similar to VarRW [almost like a symbol-table].
 */
class PushSet : public Instruction {
public:
    /**
     * @param setid name of the set.
     */
    PushSet(const string& setid) : _setid(setid) {}

    INSTR_VISITABLE();

    /**
     * @return name of the set.
     */
    const string& setid() const { return _setid; }

private:
    string _setid;
};

/**
 * @short Instruction that exits the current term if top of stack is false.
 * 
 * This instruction checks if the top of the stack contains an ElemBool. If it
 * is false, the current term is exited [execution continues at next term]. If
 * not, normal execution continues.
 */
class OnFalseExit : public Instruction {
public:
    
    INSTR_VISITABLE();
};

/**
 * @short Instruction to read a variable via VarRW interface.
 */
class Load : public Instruction {
public:

    /**
     * @param var identifier of variable to load.
     */
    Load(const VarRW::Id& var) : _var(var) {}

    INSTR_VISITABLE();
    
    /**
     * @return identifier of variable to read.
     */
    const VarRW::Id& var() const { return _var; }

private:
    VarRW::Id _var;
};

/**
 * @short Instruction to write a variable via VarRW interface.
 *
 * Argument is top most element on stack.
 * 1 element popped from stack.
 */
class Store : public Instruction {
public:
    /**
     * @param var identifier of variable to store.
     */
    Store(const VarRW::Id& var) : _var(var) {}

    INSTR_VISITABLE();

    /**
     * @return identifier of variable to write.
     */
    const VarRW::Id& var() const { return _var; }

private:
    VarRW::Id _var;
};

/**
 * @short Instruction to accept a route.
 */
class Accept : public Instruction  {
public:
    INSTR_VISITABLE();
};

/**
 * @short Instruction to reject a route.
 */
class Reject : public Instruction  {
public:
    INSTR_VISITABLE();
};

class Next : public Instruction {
public:
    enum Flow {
	TERM,
	POLICY
    };

    Next(Flow f) : _flow(f) {}

    Flow flow() { return _flow; }

    INSTR_VISITABLE();

private:
    Flow    _flow;
};

class Subr : public Instruction {
public:
    Subr(string target) : _target(target) {}

    string target() { return _target; }

    INSTR_VISITABLE();

private:
    string  _target;
};

/**
 * @short An N-ary operation.
 *
 * Arguments are the N top-most elements of the stack.
 * Top-most argument is first. Thus elements have to be pushed on stack in
 * reverse order.
 *
 * Operation will pop N elements from the stack.
 */
class NaryInstr :
    public NONCOPYABLE,
    public Instruction
{
public:
    /**
     * Caller must not delete / modify operation.
     *
     * @param op operation of this instruction
     */
    NaryInstr(Oper* op) : _op(op) {}
    ~NaryInstr() { delete _op; }

    INSTR_VISITABLE();


    /**
     * @return operation associated with this instruction.
     */
    const Oper& op() const { return *_op; }

private:
    Oper* _op;
};

#endif // __POLICY_BACKEND_INSTRUCTION_HH__
