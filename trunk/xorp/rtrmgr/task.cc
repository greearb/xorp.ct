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

#ident "$XORP: xorp/rtrmgr/task.cc,v 1.11 2003/05/23 00:02:08 mjh Exp $"

#include "rtrmgr_module.h"
#include "libxorp/xlog.h"
#include "libxorp/status_codes.h"
#include "task.hh"
#include "module_manager.hh"
#include "xorp_client.hh"

#define MAX_STATUS_RETRIES 30

DelayValidation::DelayValidation(EventLoop& eventloop, uint32_t ms)
    : _eventloop(eventloop), _delay_in_ms(ms)
{
}

void DelayValidation::validate(CallBack cb) 
{
    _cb = cb;
    _timer = _eventloop.new_oneoff_after_ms(_delay_in_ms,
                 callback(this, &DelayValidation::timer_expired));
}

void DelayValidation::timer_expired() 
{
    _cb->dispatch(true);
}

StatusReadyValidation::StatusReadyValidation(const string& target, 
					     TaskManager& taskmgr)
    : _target(target), _task_manager(taskmgr), 
    _retries(0)
{
}


EventLoop&
StatusReadyValidation::eventloop()
{
    return _task_manager.eventloop();
}

void 
StatusReadyValidation::validate(CallBack cb)
{
    _cb = cb;
    if (_task_manager.do_exec()) {
	Xrl xrl(_target, "common/0.1/get_status");
	printf("XRL: >%s<\n", xrl.str().c_str());
	string response = "status:bool&reason:txt";
	_task_manager.xorp_client().
	    send_now(xrl, callback(this, &StatusReadyValidation::xrl_done),
		     response, true);
    } else {
	//when we're running with do_exec == false, we want to
	//exercise most of the same machinery, but we want to ensure
	//that the xrl_done response gets the right arguments even
	//though we're not going to call the XRL
	_retry_timer = 
	    eventloop().new_oneoff_after_ms(1000,
                callback(this, &StatusReadyValidation::dummy_response));
    }
}

void 
StatusReadyValidation::dummy_response()
{
    XrlError e = XrlError::OKAY();
    XrlArgs a;
    a.add_uint32("status", PROC_READY);
    a.add_string("reason", string(""));
    xrl_done(e, &a);
}

void
StatusReadyValidation::xrl_done(const XrlError& e, XrlArgs* xrlargs)
{
    if (e == XrlError::OKAY()) {
	try {
	    ProcessStatus status;
	    status = (ProcessStatus)(xrlargs->get_uint32("status"));
	    string reason;
	    reason = xrlargs->get_string("reason");

	    switch (status) {
	    case PROC_NULL:
	    case PROC_STARTUP:
		//these are not valid responses.
		XLOG_ERROR(("Bad status response; reason: " 
			    + reason + "\n").c_str());
		_cb->dispatch(false);
		return;
	    case PROC_FAILED:
	    case PROC_SHUTDOWN:
		_cb->dispatch(false);
		return;
	    case PROC_NOT_READY:
		//got a valid response saying we should wait.
		_retry_timer = 
		    eventloop().new_oneoff_after_ms(1000,
                         callback(this, &StatusReadyValidation::validate,_cb));
		return;
	    case PROC_READY:
		//the process is ready
		_cb->dispatch(true);
		return;
	    }
	} catch (XrlArgs::XrlAtomNotFound) {
	    //not a valid response
	    XLOG_ERROR("Bad XRL response to get_status\n");
	    _cb->dispatch(false);
	    return;
	}
    } else if (e == XrlError::NO_FINDER()) {
	//We're in trouble now! This shouldn't be able to happen.
	XLOG_UNREACHABLE();
    } else if (e == XrlError::NO_SUCH_METHOD()) {
	//The template file must have been wrong - the target doesn't
	//support the common interface
	XLOG_ERROR(("Target " + _target + 
		    " doesn't support get_status\n").c_str());
	
	//Just return true and hope everything's OK
	_cb->dispatch(true);
    } else if ((e == XrlError::RESOLVE_FAILED())
	       || (e == XrlError::SEND_FAILED()))  {
	//RESOLVE_FAILED => It's not yet registered with the finder -
	//retry after a short delay

	//SEND_FAILED => ???  We're dealing with startup conditions
	//here, so give the problem a chance to resolve itself.
	_retries++;
	if (_retries > MAX_STATUS_RETRIES) {
	    _cb->dispatch(false);
	}
	_retry_timer = 
	    eventloop().new_oneoff_after_ms(1000,
                 callback(this, &StatusReadyValidation::validate, _cb));
    } else {
	XLOG_ERROR(("Error while validating process " 
		    + _target + "\n").c_str());
	XLOG_WARNING("Continuing anyway, cross your fingers...\n");
	_cb->dispatch(true);
    }
}

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
    printf("TaskXrlItem::execute\n");
    printf("  %s\n", _unexpanded_xrl.str().c_str());
    Xrl *xrl = _unexpanded_xrl.expand();
    if (xrl == NULL) {
	errmsg = "Failed to expand XRL " + _unexpanded_xrl.str();
	return false;
    }

    string xrl_return_spec = _unexpanded_xrl.return_spec();

    //for debugging purposes, record what we were doing
    errmsg = _unexpanded_xrl.str();

    _task.xorp_client().
	send_now(*xrl, callback(this,
				&TaskXrlItem::execute_done),
		 xrl_return_spec, 
		 _task.do_exec());
    return true;
}

void
TaskXrlItem::execute_done(const XrlError& err, 
			  XrlArgs* xrlargs) 
{
    printf("TaskXrlItem::execute_done\n");
    if (err != XrlError::OKAY()) {
	//XXXX handle XRL errors here
	XLOG_UNFINISHED();
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
    printf("Task::run %s\n", _modname.c_str());
    _task_complete_cb = cb;
    step1();
}

void
Task::step1() 
{
    printf("step1\n");
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
    printf("step1_done\n");
    if (success)
	step2();
    else
	task_fail("Can't start process " + _modname);
}

void
Task::step2()
{
    printf("step2\n");
    if (_start_module && (_validation != NULL)) {
	_validation->validate(callback(this, &Task::step2_done));
    } else {
	step3();
    }
}

void
Task::step2_done(bool success) 
{
    printf("step2_done\n");
    if (success)
	step3();
    else
	task_fail("Can't validate start of process " + _modname);
}

void 
Task::step3()
{
    printf("step3\n");
    if (_xrls.empty()) {
	step4();
    } else {
	string errmsg;
	printf("step3: execute\n");
	if (_xrls.front().execute(errmsg) == false) {
	    XLOG_WARNING("Failed to execute XRL: %s\n", errmsg.c_str());
	    task_fail(errmsg);
	    return;
	}
    }
}

void
Task::xrl_done(bool success, string errmsg) 
{
    printf("xrl_done\n");
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
    printf("step4\n");
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
    printf("step4_done\n");
    step5();
}

void
Task::step5() 
{
    printf("step5\n");
    if (_stop_module && (_validation != NULL)) {
	_validation->validate(callback(this, &Task::step5_done));
    } else {
	step6();
    }
}

void
Task::step5_done(bool success) 
{
    printf("step5_done\n");
    if (success)
	step6();
    else
	task_fail("Can't validate stop of process " + _modname);
}

void 
Task::step6()
{
    printf("step6\n");
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
			 bool global_do_exec)
    : _module_manager(mmgr), _xorp_client(xclient), 
    _global_do_exec(global_do_exec)
{
}

void 
TaskManager::set_do_exec(bool do_exec) {
    _current_do_exec = do_exec && _global_do_exec;
}

void 
TaskManager::reset() {
    while (!_tasks.empty()) {
	delete _tasks.begin()->second;
	_tasks.erase(_tasks.begin());
    }
    while (!_tasklist.empty()) {
	_tasklist.pop_front();
    }
}

int
TaskManager::add_module(const string& modname, const string& modpath,
			Validation *validation)
{
    if (_tasks.find(modname) == _tasks.end()) {
	Task* newtask = new Task(modname, *this);
	_tasks[modname] = newtask;
	_tasklist.push_back(newtask);
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
TaskManager::add_xrl(const string& modname, const UnexpandedXrl& xrl, 
		     XrlRouter::XrlCallback& cb) 
{
    find_task(modname).add_xrl(xrl, cb);
}

void
TaskManager::run(CallBack cb) 
{
    printf("TaskManager::run, tasks: ");
    list <Task*>::const_iterator i;
    for (i = _tasklist.begin(); i != _tasklist.end(); i++) {
	printf("%s ", (*i)->name().c_str());
    }
    printf("\n");
    _completion_cb = cb;
    run_task();
}

void 
TaskManager::run_task() 
{
    printf("TaskManager::run_task()\n");
    if (_tasklist.empty()) {
	printf("No more tasks to run\n");
	_completion_cb->dispatch(true, "");
	return;
    }
    _tasklist.front()->run(callback(this, &TaskManager::task_done));
}

void 
TaskManager::task_done(bool success, string errmsg) 
{
    printf("TaskManager::task_done\n");
    if (!success) {
	printf("task failed\n");
	_completion_cb->dispatch(false, errmsg);
	reset();
	return;
    }
    _tasklist.pop_front();
    run_task();
}

void
TaskManager::fail_tasklist_initialization(const string& errmsg)
{
    XLOG_ERROR((errmsg + "\n").c_str());
    reset();
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

EventLoop&
TaskManager::eventloop() const 
{
    return _module_manager.eventloop();
}
