// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/devnotes/template.cc,v 1.2 2003/01/16 19:08:48 mjh Exp $"

#include "task.hh"
#include "module_manager.hh"

TaskXrlItem::TaskXrlItem(const UnexpandedXrl& uxrl,
			 const XrlRouter::XrlCallback& cb, Task& task) 
    : _unexpanded_xrl(uxrl), _xrl_callback(cb), _task(task)
{
}

TaskXrlItem::TaskXrlItem(const TaskXrlItem& them)
    : _unexpanded_xrl(them._unexpanded_xrl), 
    _xrl_callback(them._xrl_callback), 
    _task(them._task)
{
}

bool 
TaskXrlItem::execute(string& errmsg)
{
    Xrl *xrl = _unexpanded_xrl.xrl();
    if (xrl == NULL) {
	errmsg = "Failed to expand XRL " + _unexpanded_xrl.str();
	return false;
    }

    string xrl_return_spec = _unexpanded_xrl.xrl_return_spec();
    return _task.xorp_client().
	send_now(*xrl, callback(this,
				&TaskXrlItem::execute_done),
		 xrl_return_spec, 
		 _task.do_exec());
}

void
TaskXrlItem::execute_done(const XrlError& err, 
			  XrlArgs* xrlargs) {

    if (err != XrlError::OKAY()) {
	//XXXX handle XRL errors here
	abort();
    }

    if (!_xrl_callback.is_empty())
	_xrl_callback->dispatch(err, xrlargs);

    bool success = true;
    string errmsg;
    if (err != XrlError::OKAY()) {
	success = false;
	errmsg = err.str();
    }
    _task.xrl_done(success, errmsg);
}




Task::Task(const string& name, XorpClient& xclient, bool do_exec) 
    : _name(name), _xclient(xclient), _do_exec(do_exec),
      _start_module(false), _stop_module(false), _mmgr(NULL)
{
}

Task::~Task()
{
    if (_validation != NULL)
	delete _validation;
}

void 
Task::add_start_module(ModuleManager& mmgr, const string& modname,
		       Validation *validation)
{
    assert(_start_module == false);
    assert(_stop_module == false);
    _start_module = true;
    _modname = modname;
    _mmgr = &mmgr;
    _validation = validation;
}

void 
Task::add_stop_module(ModuleManager& mmgr, const string& modname,
		      Validation *validation)
{
    assert(_start_module == false);
    assert(_stop_module == false);
    _start_module = true;
    _modname = modname;
    _mmgr = &mmgr;
    _validation = validation;
}

void
Task::add_xrl(const UnexpandedXrl& xrl, XrlRouter::XrlCallback& cb)
{
    _xrls.push_back(TaskXrlItem(xrl, cb, *this));
}

void
Task::run(CallBack cb)
{
    _task_complete_cb = cb;
    step1();
}

void
Task::step1() 
{
    if (_start_module) {
	_mmgr->start_module(_modname, _do_exec, 
			    callback(this, &Task::step1_done));
    } else {
	step2();
    }
}

void
Task::step1_done(bool success) 
{
    if (success)
	step2();
    else
	task_fail("Can't start process " + _modname);
}

void
Task::step2()
{
    if (_start_module && (_validation != NULL)) {
	_validation->validate(callback(this, &Task::step5_done));
    } else {
	step3();
    }
}

void
Task::step2_done(bool success) 
{
    if (success)
	step3();
    else
	task_fail("Can't validate start of process " + _modname);
}

void 
Task::step3()
{
    if (_xrls.empty()) {
	step4();
    } else {
	string errmsg;
	if (_xrls.front().execute(errmsg) == false) {
	    task_fail(errmsg);
	    return;
	}
    }
}

void
Task::xrl_done(bool success, string errmsg) 
{
    if (success) {
	_xrls.pop_front();
	step3();
    } else {
	task_fail(errmsg);
    }
}

void
Task::step4()
{
    if (_stop_module) {
	_mmgr->stop_module(_modname, callback(this, &Task::step4_done));
    } else {
	step5();
    }
}

void
Task::step4_done() {
    step5();
}

void
Task::step5() 
{
    if (_stop_module && (_validation != NULL)) {
	_validation->validate(callback(this, &Task::step5_done));
    } else {
	step6();
    }
}

void
Task::step5_done(bool success) {
    if (success)
	step6();
    else
	task_fail("Can't validate stop of process " + _modname);
}

void 
Task::step6()
{
    debug_msg("Task done\n");
    _task_complete_cb->dispatch(true);
}

void
Task::task_fail(string errmsg) 
{
    debug_msg((errmsg + "\n").c_str());
    _task_complete_cb->dispatch(false);
}
