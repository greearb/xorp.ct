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

#ident "$XORP$"

#include "config.h"
#include "iv_exec.hh"
#include "policy/common/policy_utils.hh"
#include "policy/common/elem_null.hh"
#include "policy/common/element.hh"

IvExec::IvExec(vector<PolicyInstr*>& policies, SetManager& sman, VarRW& varrw,
	       ostream* os) : 
	       _policies(policies), _sman(sman), _varrw(varrw),
	       _finished(false), _fa(DEFAULT), _os(os) {}

IvExec::~IvExec() {
    clear_trash();
}

IvExec::FlowAction 
IvExec::run() {
    vector<PolicyInstr*>::iterator i;
    
    FlowAction ret = DEFAULT;

    // execute all policies
    for(i = _policies.begin(); i != _policies.end(); ++i) {
	FlowAction fa = runPolicy(*(*i));

	// if a policy rejected/accepted a route then terminate.
	if(fa != DEFAULT) {
	    ret = fa;
	    break;
	}
    }
    
    if(_os)
	*_os << "Outcome of whole filter: " << fa2str(ret) << endl;

    // important because varrw may hold pointers to trash elements
    _varrw.sync();
    clear_trash();

    
    return ret;
}

IvExec::FlowAction 
IvExec::runPolicy(PolicyInstr& pi) {
    vector<TermInstr*>& terms = pi.terms();
    
    vector<TermInstr*>::iterator i;

    FlowAction outcome = DEFAULT;

    if(_os)
	*_os << "Running policy: " << pi.name() << endl;

    // run all terms
    for(i = terms.begin(); i != terms.end(); ++i) {
	FlowAction fa = runTerm( *(*i));

	// if term accepted/rejected route, then terminate.
	if(fa != DEFAULT) {
	    outcome = fa;
	    break;
	}    
    }

    if(_os)
	*_os << "Outcome of policy: " << fa2str(outcome) << endl;

    return outcome;
}

IvExec::FlowAction 
IvExec::runTerm(TermInstr& ti) {

    // we just started
    _finished = false;
    _fa = DEFAULT;

    // clear stack
    while(!_stack.empty())
	_stack.pop();

    vector<Instruction*>& instr = ti.instructions();
    vector<Instruction*>::iterator i;

    if(_os)
	*_os << "Running term: " << ti.name() << endl;

    // run all instructions
    for(i = instr.begin(); i != instr.end(); ++i) {
	(*i)->accept(*this);

	// a flow action occured [accept/reject/default -- exit]
	if(_finished)
	    break;
    }

    if(_os)
	*_os << "Outcome of term: " << fa2str(_fa) << endl;

    return _fa;
}

void 
IvExec::visit(Push& p) {
    // node owns element [no need to trash]
    _stack.push(&(p.elem()));
    
    if(_os)
	*_os << "PUSH " << p.elem().str() << endl;
}

void 
IvExec::visit(PushSet& ps) {
    // set manager owns set [no need to trash]
    _stack.push(&_sman.getSet(ps.setid()));

    if(_os)
	*_os << "PUSH_SET " << ps.setid() << endl;
}

void 
IvExec::visit(OnFalseExit& /* x */) {
    if(_stack.empty())
	throw RuntimeError("Got empty stack on ON_FALSE_EXIT");

    // we expect a bool at the top.
    const ElemBool* t = dynamic_cast<const ElemBool*>(_stack.top());
    if(!t) {
	// but maybe it is a ElemNull... in which case its a NOP
	const Element* e = _stack.top();
	if(e->type() == ElemNull::id) {
	    if(_os)
		*_os << "GOT NULL ON TOP OF STACK, GOING TO NEXT TERM" << endl;
	    _finished = true;
        }

	// if it is anything else, its an error
        else {
	   throw RuntimeError("Expected bool on top of stack instead: " +
			      e->type());
	}
	    
    }

    // we do not pop the element!!!
    // The reason is, that maybe we want to stick ONFALSE_EXIT's here and there
    // for optimizations to peek on the stack. Consider a giant AND, we may
    // stick an ON_FALSEEXIT after earch clause of the and. In that case we do
    // not wish to pop the element from the stack, as if it is true, we want to
    // continue computing the AND.

    // it is false, so lets go to next term
    if(!t->val())
	_finished = true;

    if(_os)
	*_os << "ONFALSE_EXIT: " << t->str() << endl;

	
}

void 
IvExec::visit(Regex& re) {
    if(_stack.empty())
	throw RuntimeError("Got empty stack on REGEX");

    const Element* arg = _stack.top();
    _stack.pop();

    // XXX: regex errors
    bool res = !regexec(&re.reg(),arg->str().c_str(),0,0,0);
    
    if(_os)
	*_os << "REGEX " << re.rexp() << endl;
	
    ElemBool* r = new ElemBool(res);
    // we just created element, so trash it.
    _trash.insert(r);
    _stack.push(r);
}

void 
IvExec::visit(Load& l) {
    const Element& x = _varrw.read(l.var());

    if(_os)
	*_os << "LOAD " << l.var() << endl;
    // varrw owns element [do not trash]
    _stack.push(&x);
}

void 
IvExec::visit(Store& s) {
    if(_stack.empty())
	throw RuntimeError("Stack empty on assign of " + s.var());

    const Element* arg = _stack.top();
    _stack.pop();

    if(arg->type() == ElemNull::id) {
	if(_os)
	    *_os << "STORE NULL [treated as NOP]" << endl;
	return;
    }

    // we still own the element.
    // if it had to be trashed, it would have been trashed on creation, so do
    // NOT trash now. And yes, it likely is an element we do not have to
    // trash anyway.
    _varrw.write(s.var(),*arg);
    if(_os)
	*_os << "STORE " << s.var() << endl;
}

void 
IvExec::visit(Accept& /* a */) {
    // ok we like the route, so exit all execution
    _finished = true;
    _fa = ACCEPT;
    if(_os)
	*_os << "ACCEPT" << endl;
}
    
void 
IvExec::visit(Reject& /* r */) {
    // we don't like it, get out of here.
    _finished = true;
    _fa = REJ;

    if(_os)
	*_os << "REJECT" << endl;
}

void
IvExec::visit(NaryInstr& nary) {
    unsigned size = _stack.size();
    unsigned arity = nary.op().arity();

    // check if we have enough elements on stack
    if(size < arity) {
	ostringstream oss;

	oss << "Stack size " << size << " on operation " 
	    << nary.op().str() << " with arity " << arity;

	throw RuntimeError(oss.str());
    }

    // prepare arguments
    Dispatcher::ArgList args;

    for(unsigned i = 0; i < arity; ++i) {
	args.push_back(_stack.top());
	_stack.pop();
    }

    // execute the operation
    Element* r = _disp.run(nary.op(),args);

    // trash the result for deletion on completion
    _trash.insert(r);

    // store result on stack
    _stack.push(r);

    // output trace
    if(_os)
	*_os << nary.op().str() << endl;
}

void
IvExec::clear_trash() {
    policy_utils::clear_container(_trash);
}


string
IvExec::fa2str(const FlowAction& fa) {
    switch(fa) {
	case ACCEPT:
	    return "Accept";
	
	case REJ:
	    return "Reject";
	
	case DEFAULT:
	    return "Default action";
    }

    return "Unknown";
}
