// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/policy/backend/instruction.hh,v 1.9 2007/02/16 22:46:58 pavlin Exp $

#ifndef __POLICY_BACKEND_INSTRUCTION_HH__
#define __POLICY_BACKEND_INSTRUCTION_HH__

#include "libxorp/xorp.h"

#if defined(HAVE_REGEX_H)
#include <regex.h>
#elif defined(HAVE_PCREPOSIX_H)
#include <pcre.h>
#include <pcreposix.h>
#endif

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
class Push : public Instruction {
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
    
    // not implemented
    Push(const Push&);
    Push& operator=(const Push&);
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


/**
 * @short An N-ary operation.
 *
 * Arguments are the N top-most elements of the stack.
 * Top-most argument is first. Thus elements have to be pushed on stack in
 * reverse order.
 *
 * Operation will pop N elements from the stack.
 */
class NaryInstr : public Instruction {
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

    // not impl
    NaryInstr(const NaryInstr&);
    NaryInstr& operator=(const NaryInstr&);
};

#endif // __POLICY_BACKEND_INSTRUCTION_HH__
