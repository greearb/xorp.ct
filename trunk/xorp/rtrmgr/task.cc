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

#ident "$XORP: xorp/rtrmgr/task.cc,v 1.3 2003/05/02 04:53:34 mjh Exp $"

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
			  XrlArgs* xrlargs) 
{

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




Task::Task(const string& name, TaskManager& taskmgr) 
    : _name(name), _taskmgr(taskmgr),
      _start_module(false), _stop_module(false)
{
}

Task::~Task()
{
    if (_validation != NULL)
	delete _validation;
}

void 
Task::add_start_module(const string& modname,
		       Validation *validation)
{
    assert(_start_module == false);
    assert(_stop_module == false);
    _start_module = true;
    _modname = modname;
    _validation = validation;
}

void 
Task::add_stop_module(const string& modname,
		      Validation *validation)
{
    assert(_start_module == false);
    assert(_stop_module == false);
    _start_module = true;
    _modname = modname;
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
	_taskmgr.module_manager()
	    .start_module(_modname, do_exec(), 
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
	_taskmgr.module_manager()
	    .stop_module(_modname, callback(this, &Task::step4_done));
    } else {
	step5();
    }
}

void
Task::step4_done() 
{
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
Task::step5_done(bool success) 
{
    if (success)
	step6();
    else
	task_fail("Can't validate stop of process " + _modname);
}

void 
Task::step6()
{
    debug_msg("Task done\n");
    _task_complete_cb->dispatch(true, "");
}

void
Task::task_fail(string errmsg) 
{
    debug_msg((errmsg + "\n").c_str());
    _task_complete_cb->dispatch(false, errmsg);
}

bool 
Task::do_exec() const 
{
    return _taskmgr.do_exec();
}

XorpClient& 
Task::xorp_client() const 
{
    return _taskmgr.xorp_client();
}

TaskManager::TaskManager(ModuleManager &mmgr, XorpClient& xclient, 
			 bool do_exec)
    : _module_manager(mmgr), _xorp_client(xclient), _do_exec(do_exec)
{
}

int
TaskManager::add_module(const string& modname, const string& modpath,
			Validation *validation)
{
    if (_tasks.find(modname) == _tasks.end()) {
	_tasks[modname] = new Task(modname, *this);
    }

    if (_module_manager.module_exists(modname)) {
	if (_module_manager.module_has_started(modname)) {
	    return XORP_OK;
	}
    } else {
	if (!_module_manager.new_module(modname, modpath)) {
	    fail_tasklist_initialization("Can't create module");
	    return XORP_ERROR;
	}
    }

    find_task(modname).add_start_module(modname, validation);
    return XORP_OK;
}

void
TaskManager::run(CallBack cb) 
{
    _completion_cb = cb;
    run_task();
}

void 
TaskManager::run_task() 
{
    if (_tasks.empty()) {
	_completion_cb->dispatch(true, "");
	return;
    }
    _tasks.begin()->second->run(callback(this, &TaskManager::task_done));
}

void 
TaskManager::task_done(bool success, string errmsg) 
{
    if (!success) {
	_completion_cb->dispatch(false, errmsg);
	while (!_tasks.empty()) {
	    delete _tasks.begin()->second;
	    _tasks.erase(_tasks.begin());
	}
	return;
    }
    delete _tasks.begin()->second;
    _tasks.erase(_tasks.begin());
    run_task();
}

void
TaskManager::fail_tasklist_initialization(const string& errmsg)
{
    XLOG_ERROR((errmsg + "\n").c_str());
    while (!_tasks.empty()) {
	delete _tasks.begin()->second;
	_tasks.erase(_tasks.begin());
    }
    return;
}

Task& 
TaskManager::find_task(const string& modname)
{
    map<string, Task*>::iterator i;
    i = _tasks.find(modname);
    assert(i != _tasks.end());
    assert(i->second != NULL);
    return *(i->second);
}
