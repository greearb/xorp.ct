// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/policy/backend/iv_exec.hh,v 1.11 2008/01/04 03:17:15 pavlin Exp $

#ifndef __POLICY_BACKEND_IV_EXEC_HH__
#define __POLICY_BACKEND_IV_EXEC_HH__

#include "libxorp/xorp.h"

#include <stack>

#include "policy/common/dispatcher.hh"
#include "policy/common/varrw.hh"
#include "policy/common/policy_exception.hh"

#include "instruction.hh"
#include "set_manager.hh"
#include "term_instr.hh"
#include "policy_instr.hh"


/**
 * @short Visitor that executes instructions
 *
 * The execution process may be optimized by not using visitors. Having
 * instructions implement a method that returns a flow action directly.
 */
class IvExec : public InstrVisitor {
public:
    /**
     * A FlowAction is what has to be done with the route. DEFAULT is the
     * default action which is normally "go to the next term", or if the last
     * term, ACCEPT.
     */
    enum FlowAction {
	ACCEPT,
	REJ,
	DEFAULT
    };

    /**
     * @short Run time errors, such as doing unsupported operations.
     *
     * The semantic check should get rid of these.
     */
    class RuntimeError : public PolicyException {
    public:
	RuntimeError(const char* file, size_t line, const string& init_why = "")
	    : PolicyException("RuntimeError", file, line, init_why) {}  
    };

    IvExec();
    ~IvExec();
   
    void set_policies(vector<PolicyInstr*>* policies);
    void set_set_manager(SetManager* sman);
   
    /**
     * Execute the policies.
     */
    FlowAction run(VarRW* varrw, ostream* os);

    /**
     * Execute a policy.
     *
     * @param pi policy to execute
     */
    FlowAction runPolicy(PolicyInstr& pi);

    /**
     * Execute a term.
     *
     * @param ti term to execute.
     */
    FlowAction runTerm(TermInstr& ti);

    /**
     * @param p push to execute.
     */
    void visit(Push& p);

    /**
     * @param ps push of a set to execute.
     */
    void visit(PushSet& ps);
    
    /**
     * @param x OnFalseExit to execute.
     */
    void visit(OnFalseExit& x);

    /**
     * @param l Load to execute.
     */
    void visit(Load& l);

    /**
     * @param s Store to execute.
     */
    void visit(Store& s);

    /**
     * @param a accept the route.
     */
    void visit(Accept& a);
    
    /**
     * @param r reject the route.
     */
    void visit(Reject& r);

    /**
     * @param nary N-ary instruction to execute.
     */
    void visit(NaryInstr& nary);

    /**
     * @return String representation of flow action.
     * @param fa Flow action to convert.
     */
    static string fa2str(const FlowAction& fa);

private:
    /**
     * Do garbage collection.
     */
    void clear_trash();

    PolicyInstr** _policies;
    unsigned _policy_count;

    const Element** _stack;
    const Element**  _stackend;
    const Element**  _stackptr;

    SetManager* _sman;
    VarRW* _varrw;
    bool _finished;
    Dispatcher _disp;
    FlowAction _fa;

    Element** _trash;
    unsigned _trashc;
    unsigned _trashs;
    ostream* _os;

    // not impelmented
    IvExec(const IvExec&);
    IvExec& operator=(const IvExec&);
};

#endif // __POLICY_BACKEND_IV_EXEC_HH__
