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

#ident "$XORP: xorp/policy/backend/iv_exec.cc,v 1.18 2008/08/06 08:05:45 abittau Exp $"

#include "policy/policy_module.h"
#include "libxorp/xorp.h"
#include "policy/common/policy_utils.hh"
#include "policy/common/elem_null.hh"
#include "policy/common/element.hh"
#include "iv_exec.hh"

IvExec::IvExec() : 
	       _policies(NULL), _policy_count(0), _sman(NULL), _varrw(NULL),
	       _finished(false), _fa(DEFAULT), _trash(NULL), _trashc(0),
	       _trashs(2000), _os(NULL), _profiler(NULL)
{
    unsigned ss = 128;
    _trash = new Element*[_trashs];

    _stack = new const Element*[ss];
    _stackptr = &_stack[0];
    _stackptr--;
    _stackend = &_stack[ss];
}

IvExec::~IvExec()
{
    if (_policies)
	delete [] _policies;
    
    clear_trash();
    delete [] _trash;
    delete [] _stack;
}

IvExec::FlowAction 
IvExec::run(VarRW* varrw, ostream* os)
{
    _varrw     = varrw;
    _os	       = os;
    _did_trace = false;

    XLOG_ASSERT(_policies);
    XLOG_ASSERT(_sman);
    XLOG_ASSERT(_varrw);

    FlowAction ret = DEFAULT;

    // execute all policies
    for (unsigned i = 0; i < _policy_count; ++i) {
	FlowAction fa = runPolicy(*_policies[i]);

	// if a policy rejected/accepted a route then terminate.
	if (fa != DEFAULT) {
	    ret = fa;
	    break;
	}
    }

    if (_did_trace)
	*_os << "Outcome of whole filter: " << fa2str(ret) << endl;

    // important because varrw may hold pointers to trash elements
    _varrw->sync();
    clear_trash();

    return ret;
}

IvExec::FlowAction 
IvExec::runPolicy(PolicyInstr& pi)
{
    TermInstr** terms  = pi.terms();
    int termc	       = pi.termc();
    FlowAction outcome = DEFAULT;

    _do_trace = pi.trace() && _os;

    _varrw->enable_trace(_do_trace);

    if (_do_trace)
	_did_trace = true;

    if (_do_trace)
	*_os << "Running policy: " << pi.name() << endl;

    // run all terms
    for (int i = 0; i < termc ; ++i) {
	FlowAction fa = runTerm(*terms[i]);

	// if term accepted/rejected route, then terminate.
	if (fa != DEFAULT) {
	    outcome = fa;
	    break;
	}    
    }

    if (_do_trace)
	*_os << "Outcome of policy: " << fa2str(outcome) << endl;

    return outcome;
}

IvExec::FlowAction 
IvExec::runTerm(TermInstr& ti)
{

    // we just started
    _finished = false;
    _fa = DEFAULT;

    // clear stack
    _stackptr = &_stack[0];
    _stackptr--;

    int instrc = ti.instrc();
    Instruction** instr = ti.instructions();

    if (_do_trace)
	*_os << "Running term: " << ti.name() << endl;

    // run all instructions
    for (int i = 0; i < instrc; ++i) {
	if (_profiler)
	    _profiler->start();

	(instr[i])->accept(*this);

	if (_profiler)
	    _profiler->stop();

	// a flow action occured [accept/reject/default -- exit]
	if (_finished)
	    break;
    }

    if (_do_trace)
	*_os << "Outcome of term: " << fa2str(_fa) << endl;

    return _fa;
}

void 
IvExec::visit(Push& p)
{
    const Element& e = p.elem();
    // node owns element [no need to trash]
    _stackptr++;
    XLOG_ASSERT(_stackptr < _stackend);
    *_stackptr = &e;
    
    if(_do_trace)
	*_os << "PUSH " << e.type() << " " << e.str() << endl;
}

void 
IvExec::visit(PushSet& ps)
{
    string name = ps.setid();
    const Element& s = _sman->getSet(name);

    // set manager owns set [no need to trash]
    _stackptr++;
    XLOG_ASSERT(_stackptr < _stackend);
    *_stackptr = &s;

    if(_do_trace)
	*_os << "PUSH_SET " << s.type() << " " << name
	     << ": " << s.str() << endl;
}

void 
IvExec::visit(OnFalseExit& /* x */)
{
    if (_stackptr < _stack)
	xorp_throw(RuntimeError, "Got empty stack on ON_FALSE_EXIT");

    // we expect a bool at the top.
    const ElemBool* t = dynamic_cast<const ElemBool*>(*_stackptr);
    if(!t) {
	// but maybe it is a ElemNull... in which case its a NOP
	const Element* e = *_stackptr;
	if(e->hash() == ElemNull::_hash) {
	    if(_do_trace)
		*_os << "GOT NULL ON TOP OF STACK, GOING TO NEXT TERM" << endl;
	    _finished = true;
	    return;
        }

	// if it is anything else, its an error
        else {
	   xorp_throw(RuntimeError, "Expected bool on top of stack instead: ");
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

    if(_do_trace)
	*_os << "ONFALSE_EXIT: " << t->str() << endl;
}

void 
IvExec::visit(Load& l)
{
    const Element& x = _varrw->read_trace(l.var());

    if(_do_trace)
	*_os << "LOAD " << l.var() << ": " << x.str() << endl;
    // varrw owns element [do not trash]
    _stackptr++;
    XLOG_ASSERT(_stackptr < _stackend);
    *_stackptr = &x;
}

void 
IvExec::visit(Store& s)
{
    if (_stackptr < _stack)
	xorp_throw(RuntimeError, "Stack empty on assign of " + s.var());

    const Element* arg = *_stackptr;
    _stackptr--;
    XLOG_ASSERT( _stackptr >= (_stack-1));

    if(arg->hash() == ElemNull::_hash) {
	if(_do_trace)
	    *_os << "STORE NULL [treated as NOP]" << endl;
	return;
    }

    // we still own the element.
    // if it had to be trashed, it would have been trashed on creation, so do
    // NOT trash now. And yes, it likely is an element we do not have to
    // trash anyway.
    _varrw->write_trace(s.var(),*arg);
    if(_do_trace)
	*_os << "STORE " << s.var() << ": " << arg->str() << endl;
}

void 
IvExec::visit(Accept& /* a */)
{
    // ok we like the route, so exit all execution
    _finished = true;
    _fa = ACCEPT;
    if(_do_trace)
	*_os << "ACCEPT" << endl;
}
    
void 
IvExec::visit(Reject& /* r */)
{
    // we don't like it, get out of here.
    _finished = true;
    _fa = REJ;

    if(_do_trace)
	*_os << "REJECT" << endl;
}

void
IvExec::visit(NaryInstr& nary)
{
    unsigned arity = nary.op().arity();

    XLOG_ASSERT(_stackptr-arity+1 >= _stack);

    // execute the operation
    Element* r = _disp.run(nary.op(), arity, _stackptr-arity+1);
    if (arity)
	_stackptr -= arity -1;
    else
	_stackptr++;

    // trash the result for deletion on completion
    _trash[_trashc] = r;
    _trashc++;

    XLOG_ASSERT(_trashc < _trashs);

    // store result on stack
    XLOG_ASSERT(_stackptr < _stackend && _stackptr >= _stack);
    *_stackptr = r;

    // output trace
    if(_do_trace)
	*_os << nary.op().str() << endl;
}

void
IvExec::clear_trash()
{
    for (unsigned i = 0; i< _trashc; i++)
	delete _trash[i];

    _trashc = 0;
}

string
IvExec::fa2str(const FlowAction& fa)
{
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

void
IvExec::set_policies(vector<PolicyInstr*>* policies)
{
    if (_policies) {
	delete [] _policies;
	_policies = NULL;
    }

    // resetting...
    if (!policies) {
	_policy_count = 0;
	return;
    }

    _policy_count = policies->size();

    _policies = new PolicyInstr*[_policy_count];

    vector<PolicyInstr*>::iterator iter;

    unsigned i = 0;
    for (iter = policies->begin(); iter != policies->end(); ++iter) {
	_policies[i] = *iter;
	i++;
    }
}

void
IvExec::set_set_manager(SetManager* sman)
{
    _sman = sman;
}

void
IvExec::set_profiler(PolicyProfiler* pp)
{
    _profiler = pp;
}
