// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// $XORP: xorp/policy/backend/iv_exec.hh,v 1.19 2008/10/02 21:58:04 bms Exp $

#ifndef __POLICY_BACKEND_IV_EXEC_HH__
#define __POLICY_BACKEND_IV_EXEC_HH__

#include "libxorp/xorp.h"

#include "policy/common/dispatcher.hh"
#include "policy/common/varrw.hh"
#include "policy/common/policy_exception.hh"
#ifndef XORP_DISABLE_PROFILE
#include "policy_profiler.hh"
#endif

#include "instruction.hh"
#include "set_manager.hh"
#include "term_instr.hh"
#include "policy_instr.hh"
#include "policy_backend_parser.hh"

/**
 * @short Visitor that executes instructions
 *
 * The execution process may be optimized by not using visitors. Having
 * instructions implement a method that returns a flow action directly.
 */
class IvExec :
    public NONCOPYABLE,
    public InstrVisitor
{
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
    FlowAction run(VarRW* varrw);

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

    void visit(Next& next);
    void visit(Subr& sub);

    /**
     * @return String representation of flow action.
     * @param fa Flow action to convert.
     */
    static string fa2str(const FlowAction& fa);

#ifndef XORP_DISABLE_PROFILE
    void    set_profiler(PolicyProfiler*);
#endif
    string  tracelog();
    void    set_subr(SUBR* subr);

private:
    /**
     * Do garbage collection.
     */
    void clear_trash();

    PolicyInstr**   _policies;
    unsigned	    _policy_count;
    const Element** _stack_bottom;
    const Element** _stack;
    const Element** _stackend;
    const Element** _stackptr;
    SetManager*	    _sman;
    VarRW*	    _varrw;
    bool	    _finished;
    Dispatcher	    _disp;
    FlowAction	    _fa;
    Element**	    _trash;
    unsigned	    _trashc;
    unsigned	    _trashs;
    ostringstream   _os;
#ifndef XORP_DISABLE_PROFILE
    PolicyProfiler* _profiler;
#endif
    bool	    _do_trace;
    bool	    _did_trace;
    Next::Flow	    _ctr_flow;
    SUBR*	    _subr;
};

#endif // __POLICY_BACKEND_IV_EXEC_HH__
