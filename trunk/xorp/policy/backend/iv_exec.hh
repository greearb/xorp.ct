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

#ifndef __POLICY_BACKEND_IV_EXEC_HH__
#define __POLICY_BACKEND_IV_EXEC_HH__

#include "policy/common/dispatcher.hh"
#include "policy/common/varrw.hh"
#include "policy/common/policy_exception.hh"
#include "instruction.hh"
#include "set_manager.hh"
#include "regex.h"
#include "term_instr.hh"
#include "policy_instr.hh"
#include <stack>


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
	RuntimeError(const string& err) : PolicyException(err) {}
    };


    /**
     * Execute the give policies with the given varrw [i.e. route].
     *
     * @param policies policies to execute.
     * @param sman set manager to use for loading sets.
     * @param varrw interface to read/write route variables.
     * @param os if not null, an execution trace will be output to stream.
     */
    IvExec(vector<PolicyInstr*>& policies, SetManager& sman, 
	   VarRW& varrw,
	   ostream* os);
    ~IvExec();
    
    /**
     * Execute the policies.
     */
    FlowAction run();

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
     * @param re Regex to execute.
     */
    void visit(Regex& re);

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

    vector<PolicyInstr*>& _policies;
    stack<const Element*> _stack;
    SetManager& _sman;
    VarRW& _varrw;

    bool _finished;

    Dispatcher _disp;

    FlowAction _fa;


    set<Element*> _trash;

    ostream* _os;

    // not impelmented
    IvExec(const IvExec&);
    IvExec& operator=(const IvExec&);
};


#endif // __POLICY_BACKEND_IV_EXEC_HH__
