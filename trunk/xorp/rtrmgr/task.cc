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

#ident "$XORP: xorp/rtrmgr/task.cc,v 1.21 2003/06/09 23:38:40 mjh Exp $"

#include "rtrmgr_module.h"
#include "libxorp/xlog.h"
#include "task.hh"
#include "module_manager.hh"
#include "module_command.hh"
#include "xorp_client.hh"

#define MAX_STATUS_RETRIES 30


// ----------------------------------------------------------------------------
// DelayValidation implementation
DelayValidation::DelayValidation(EventLoop& eventloop, uint32_t ms)
    : _eventloop(eventloop), _delay_in_ms(ms)
{
}

void
DelayValidation::validate(CallBack cb)
{
    _cb = cb;
    _timer = _eventloop.new_oneoff_after_ms(_delay_in_ms,
                 callback(this, &DelayValidation::timer_expired));
}

void
DelayValidation::timer_expired()
{
    _cb->dispatch(true);
}


// ----------------------------------------------------------------------------
// XrlStatusValidation implementation

XrlStatusValidation::XrlStatusValidation(const string& target,
					 TaskManager& taskmgr)
    : _target(target), _task_manager(taskmgr),_retries(0)
{
}

EventLoop&
XrlStatusValidation::eventloop()
{
    return _task_manager.eventloop();
}

void
XrlStatusValidation::validate(CallBack cb)
{
    _cb = cb;
    printf("validate\n");
    if (_task_manager.do_exec()) {
	Xrl xrl(_target, "common/0.1/get_status");
	printf("XRL: >%s<\n", xrl.str().c_str());
	string response = "status:bool&reason:txt";
	_task_manager.xorp_client().
	    send_now(xrl, callback(this, &XrlStatusValidation::xrl_done),
		     response, true);
    } else {
	//when we're running with do_exec == false, we want to
	//exercise most of the same machinery, but we want to ensure
	//that the xrl_done response gets the right arguments even
	//though we're not going to call the XRL
	_retry_timer =
	    eventloop().new_oneoff_after_ms(1000,
                callback(this, &XrlStatusValidation::dummy_response));
    }
}

void
XrlStatusValidation::dummy_response()
{
    XrlError e = XrlError::OKAY();
    XrlArgs a;
    a.add_uint32("status", PROC_READY);
    a.add_string("reason", string(""));
    xrl_done(e, &a);
}

void
XrlStatusValidation::xrl_done(const XrlError& e, XrlArgs* xrlargs)
{
    if (e == XrlError::OKAY()) {
	try {
	    ProcessStatus status;
	    status = static_cast<ProcessStatus>(xrlargs->get_uint32("status"));
	    string reason(xrlargs->get_string("reason"));
	    handle_status_response(status, reason);
	    return;
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


// ----------------------------------------------------------------------------
// StatusReadyValidation implementation

StatusReadyValidation::StatusReadyValidation(const string& target,
					     TaskManager& taskmgr)
    : XrlStatusValidation(target, taskmgr)
{
}

void
StatusReadyValidation::handle_status_response(ProcessStatus status,
					      const string& reason)
{
    switch (status) {
    case PROC_NULL:
	//this is not a valid responses.
	XLOG_ERROR(("Bad status response; reason: "
		    + reason + "\n").c_str());
	_cb->dispatch(false);
	return;
    case PROC_FAILED:
    case PROC_SHUTDOWN:
	_cb->dispatch(false);
	return;
    case PROC_STARTUP:
    case PROC_NOT_READY:
	//got a valid response saying we should wait.
	_retry_timer =
	    eventloop().new_oneoff_after_ms(1000,
                callback((XrlStatusValidation*)this,
			 &XrlStatusValidation::validate,_cb));
	return;
    case PROC_READY:
	//the process is ready
	_cb->dispatch(true);
	return;
    }
    XLOG_UNREACHABLE();
}


// ----------------------------------------------------------------------------
// StatusConfigMeValidation implementation

StatusConfigMeValidation::StatusConfigMeValidation(const string& target,
						   TaskManager& taskmgr)
    : XrlStatusValidation(target, taskmgr)
{
}

void
StatusConfigMeValidation::handle_status_response(ProcessStatus status,
						 const string& reason)
{
    switch (status) {
    case PROC_NULL:
	//this is not a valid responses.
	XLOG_ERROR(("Bad status response; reason: "
		    + reason + "\n").c_str());
	_cb->dispatch(false);
	return;
    case PROC_FAILED:
    case PROC_SHUTDOWN:
	_cb->dispatch(false);
	return;
    case PROC_STARTUP:
	//got a valid response saying we should wait.
	_retry_timer =
	    eventloop().new_oneoff_after_ms(1000,
                callback((XrlStatusValidation*)this,
			 &XrlStatusValidation::validate,_cb));
	return;
    case PROC_NOT_READY:
    case PROC_READY:
	//the process is ready to be configured
	_cb->dispatch(true);
	return;
    }
    XLOG_UNREACHABLE();
}

// ----------------------------------------------------------------------------
// StatusShutdownValidation implementation

StatusShutdownValidation::StatusShutdownValidation(const string& target,
						   TaskManager& taskmgr)
    : XrlStatusValidation(target, taskmgr)
{
}

void
StatusShutdownValidation::handle_status_response(ProcessStatus status,
						 const string& reason)
{
    switch (status) {
    case PROC_NULL:
	//this is not a valid responses.
	XLOG_ERROR(("Bad status response; reason: "
		    + reason + "\n").c_str());
	_cb->dispatch(false);
	return;
    case PROC_STARTUP:
    case PROC_NOT_READY:
    case PROC_READY:
    case PROC_FAILED:
	//the process should be in SHUTDOWN state, or it should not
	//be able to respond to Xrls because it's in NULL state.
	_cb->dispatch(false);
	return;
    case PROC_SHUTDOWN:
	//got a valid response saying we should wait.
	_retry_timer =
	    eventloop().new_oneoff_after_ms(1000,
                callback((XrlStatusValidation*)this,
			 &XrlStatusValidation::validate,_cb));
	return;
    }
    XLOG_UNREACHABLE();
}

void
StatusShutdownValidation::xrl_done(const XrlError& e, XrlArgs* xrlargs)
{
    if (e == XrlError::OKAY()) {
	try {
	    ProcessStatus status;
	    status = static_cast<ProcessStatus>(xrlargs->get_uint32("status"));
	    string reason(xrlargs->get_string("reason"));
	    handle_status_response(status, reason);
	    return;
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
	//return false, and we can shut it down using kill
	_cb->dispatch(false);
    } else if (e == XrlError::RESOLVE_FAILED()) {
	//The process appears to have shutdown correctly.
	_cb->dispatch(true);
    } else if (e == XrlError::SEND_FAILED())  {
	//something bad happened.
	//return false, and we can shut it down using kill
	_cb->dispatch(false);
    } else {
	XLOG_ERROR(("Error while validating process "
		    + _target + "\n").c_str());
	//return false, and we can shut it down using kill
	_cb->dispatch(false);
    }
}


// ----------------------------------------------------------------------------
// Shutdown implementation

Shutdown::Shutdown(const string& modname, TaskManager& taskmgr)
    : _modname(modname), _task_manager(taskmgr)
{
}

EventLoop&
Shutdown::eventloop() const
{
    return _task_manager.eventloop();
}


// ----------------------------------------------------------------------------
// XrlShutdown implementation

XrlShutdown::XrlShutdown(const string& modname, TaskManager& taskmgr)
    : Shutdown(modname, taskmgr)
{
}

void
XrlShutdown::shutdown(CallBack cb)
{
    _cb = cb;
    if (_task_manager.do_exec()) {
	Xrl xrl(_modname, "common/0.1/shutdown");
	printf("XRL: >%s<\n", xrl.str().c_str());
	string response = "";
	_task_manager.xorp_client().
	    send_now(xrl, callback(this, &XrlShutdown::shutdown_done),
		     response, true);
    } else {
	//when we're running with do_exec == false, we want to
	//exercise most of the same machinery, but we want to ensure
	//that the xrl_done response gets the right arguments even
	//though we're not going to call the XRL
	printf("XRL: dummy call to %s common/0.1/shutdown\n",
	       _modname.c_str());
	_dummy_timer =
	    eventloop().new_oneoff_after_ms(1000,
                callback(this, &XrlShutdown::dummy_response));
    }
}

void
XrlShutdown::dummy_response()
{
    XrlError e = XrlError::OKAY();
    XrlArgs a;
    shutdown_done(e, &a);
}

void
XrlShutdown::shutdown_done(const XrlError& err, XrlArgs* xrlargs)
{
    UNUSED(xrlargs);
    if ((err == XrlError::OKAY())
	|| (err == XrlError::RESOLVE_FAILED())) {
	//success - either it said it would shutdown, or it's already gone.
	_cb->dispatch(true);
    } else {
	//we may have to kill it.
	_cb->dispatch(false);
    }
}


// ----------------------------------------------------------------------------
// TaskXrlItem implementation

TaskXrlItem::TaskXrlItem(const UnexpandedXrl& uxrl,
			 const XrlRouter::XrlCallback& cb, Task& task)
    : _unexpanded_xrl(uxrl), _xrl_callback(cb), _task(task), _resend_counter(0)
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
    Xrl* xrl = _unexpanded_xrl.expand();
    if (xrl == NULL) {
	errmsg = "Failed to expand XRL " + _unexpanded_xrl.str();
	return false;
    }

    string xrl_return_spec = _unexpanded_xrl.return_spec();

    //for debugging purposes, record what we were doing
    errmsg = _unexpanded_xrl.str();

    _resend_counter = 0;
    _task.xorp_client().
	send_now(*xrl, callback(this,
				&TaskXrlItem::execute_done),
		 xrl_return_spec,
		 _task.do_exec());
    return true;
}

void
TaskXrlItem::unschedule()
{
    printf("TaskXrlItem::unschedule()\n");
    //empty args should be OK here.
    XrlArgs args;

    //we need to dispatch the callbacks, or the accounting of which
    //actions were taken will be incorrect.
    _xrl_callback->dispatch(XrlError::OKAY(), &args);
}

void
TaskXrlItem::resend()
{
    Xrl* xrl = _unexpanded_xrl.expand();
    if (xrl == NULL) {
	//this can't happen in a resend, because we already succeeded
	//in the orginal send
	XLOG_UNREACHABLE();
    }

    string xrl_return_spec = _unexpanded_xrl.return_spec();

    _task.xorp_client().
	send_now(*xrl, callback(this,
				&TaskXrlItem::execute_done),
		 xrl_return_spec,
		 _task.do_exec());
}

void
TaskXrlItem::execute_done(const XrlError& err,
			  XrlArgs* xrlargs)
{
    bool fatal = false;
    printf("TaskXrlItem::execute_done\n");
    if (err != XrlError::OKAY()) {
	//XXXX handle XRL errors here
	if ((err == XrlError::NO_FINDER())
	    || (err == XrlError::SEND_FAILED())
	    || (err == XrlError::NO_SUCH_METHOD())) {
	    //The error was a fatal one for the target - we now
	    //consider the target to be fatally wounded.
	    XLOG_ERROR(err.str().c_str());
	    fatal = true;
	} else if ((err == XrlError::COMMAND_FAILED())
		   || (err == XrlError::BAD_ARGS())) {
	    //Non-fatal error for the target - typically this is
	    //because the user entered bad information - we can't
	    //change the target configuration to what the user
	    //specified, but this doesn't mean the target is fatally
	    //wounded.
	    fatal = false;
	} else if ((err == XrlError::REPLY_TIMED_OUT())
		   || (err == XrlError::RESOLVE_FAILED())) {
	    //REPLY_TIMED_OUT shouldn't happen on a reliable
	    //transport, but at this point we don't know for sure
	    //which Xrls are reliable and while are unreliable, so
	    //best handle this as if it were packet loss.
	    //
	    //RESOLVE_FAILED shouldn't happen if the startup
	    //validation has done it's job correctly, but just in case
	    //we'll be lenient and give it ten more seconds to be
	    //functioning before we declare it dead.
	    if (_resend_counter > 10) {
		//Give up.
		//The error was a fatal one for the target - we now
		//consider the target to be fatally wounded.
		XLOG_ERROR(err.str().c_str());
		fatal = true;
	    } else {
		//Re-send the Xrl after a short delay.
		_resend_counter++;
		_resend_timer = _task.eventloop().new_oneoff_after_ms(1000,
                                     callback(this, &TaskXrlItem::resend));
		return;
	    }
	} else if (err == XrlError::INTERNAL_ERROR()) {
	    //Something bad happened but it's not clear what.  Don't
	    //consider these to be fatal errors, but they may well
	    //prove to be so.  XXX revisit this issue when we've more
	    //experience with XRL errors.
	    XLOG_ERROR(err.str().c_str());
	    fatal = false;
	} else {
	    //we intended to explicitly handle all errors above, so if
	    //we got here there's an error type we didn't know about.
	    XLOG_UNREACHABLE();
	}
    }

    if (!_xrl_callback.is_empty())
	_xrl_callback->dispatch(err, xrlargs);

    bool success = true;
    string errmsg;
    if (err != XrlError::OKAY()) {
	success = false;
	errmsg = err.str();
    }
    _task.xrl_done(success, fatal, errmsg);
}


// ----------------------------------------------------------------------------
// Task implementation

Task::Task(const string& name, TaskManager& taskmgr)
    : _name(name), _taskmgr(taskmgr),
      _start_module(false), _stop_module(false),
      _start_validation(NULL), _ready_validation(NULL),
      _shutdown_validation(NULL), _shutdown_method(NULL),
      _config_done(false)
{
}

Task::~Task()
{
    if (_start_validation != NULL)
	delete _start_validation;
    if (_ready_validation != NULL)
	delete _ready_validation;
    if (_shutdown_validation != NULL)
	delete _shutdown_validation;
    if (_shutdown_method != NULL)
	delete _shutdown_method;
}

void
Task::start_module(const string& modname,
		   Validation* validation)
{
    assert(_start_module == false);
    assert(_stop_module == false);
    _start_module = true;
    _modname = modname;
    _start_validation = validation;
}

void
Task::shutdown_module(const string& modname,
		      Validation* validation,
		      Shutdown* shutdown)
{
    assert(_start_module == false);
    assert(_stop_module == false);
    assert(_shutdown_validation == NULL);
    assert(_shutdown_method == NULL);
    _stop_module = true;
    _modname = modname;
    _shutdown_validation = validation;
    _shutdown_method = shutdown;
}

void
Task::add_xrl(const UnexpandedXrl& xrl, XrlRouter::XrlCallback& cb)
{
    _xrls.push_back(TaskXrlItem(xrl, cb, *this));
}

void
Task::set_ready_validation(Validation* validation)
{
    _ready_validation = validation;
}

void
Task::run(CallBack cb)
{
    printf("Task::run %s\n", _modname.c_str());
    _task_complete_cb = cb;
    step1_start();
}

void
Task::step1_start()
{
    printf("step1 (%s)\n", _modname.c_str());
    if (_start_module) {
	_taskmgr.module_manager()
	    .start_module(_modname, do_exec(),
			  callback(this, &Task::step1_done));
    } else {
	step2_wait();
    }
}

void
Task::step1_done(bool success)
{
    printf("step1_done (%s)\n", _modname.c_str());
    if (success)
	step2_wait();
    else
	task_fail("Can't start process " + _modname, false);
}

void
Task::step2_wait()
{
    printf("step2 (%s)\n", _modname.c_str());
    if (_start_module && (_start_validation != NULL)) {
	_start_validation->validate(callback(this, &Task::step2_done));
    } else {
	step3_config();
    }
}

void
Task::step2_done(bool success)
{
    printf("step2_done (%s)\n", _modname.c_str());
    if (success)
	step3_config();
    else
	task_fail("Can't validate start of process " + _modname, true);
}

void
Task::step3_config()
{
    printf("step3 (%s)\n", _modname.c_str());
    if (_xrls.empty()) {
	step4_wait();
    } else {
	if (_stop_module) {
	    //we don't call any XRLs on a module if we are going to
	    //shut it down immediately afterwards, but we do need to
	    //unschedule the XRLs.
	    while (!_xrls.empty()) {
		_xrls.front().unschedule();
		_xrls.pop_front();
	    }
	    //skip step4 and go directly to stopping the process
	    step5_stop();
	} else {
	    string errmsg;
	    printf("step3: execute\n");
	    if (_xrls.front().execute(errmsg) == false) {
		XLOG_WARNING("Failed to execute XRL: %s\n", errmsg.c_str());
		task_fail(errmsg, false);
		return;
	    }
	}

    }
}

void
Task::xrl_done(bool success, bool fatal, string errmsg)
{
    printf("xrl_done (%s)\n", _modname.c_str());
    if (success) {
	_xrls.pop_front();
	_config_done = true;
	step3_config();
    } else {
	task_fail(errmsg, fatal);
    }
}

void
Task::step4_wait()
{
    printf("step4 (%s)\n", _modname.c_str());
    if (_ready_validation && _config_done) {
	_ready_validation->validate(callback(this, &Task::step4_done));
    } else {
	step5_stop();
    }
}

void
Task::step4_done(bool success)
{
    if (success) {
	step5_stop();
    } else {
	task_fail("Reconfig of process " + _modname +
		  " caused process to fail.", true);
    }
}

void
Task::step5_stop()
{
    printf("step5 (%s)\n", _modname.c_str());
    if (_stop_module) {
	if (_shutdown_method != NULL) {
	    _shutdown_method->shutdown(callback(this, &Task::step5_done));
	} else {
	    step6_wait();
	}
    } else {
	step7_report();
    }
}

void
Task::step5_done(bool success)
{
    printf("step5_done (%s)\n", _modname.c_str());
    if (success) {
	step6_wait();
    } else {
	XLOG_WARNING(("Can't subtly stop process " + _modname).c_str());
	_taskmgr.module_manager()
	    .kill_module(_modname, callback(this, &Task::step6_wait));
    }
}

void
Task::step6_wait()
{
    printf("step6 (%s)\n", _modname.c_str());
    if (_stop_module && (_shutdown_validation != NULL)) {
	_shutdown_validation->validate(callback(this, &Task::step6_done));
    } else {
	step7_report();
    }
}

void
Task::step6_done(bool success)
{
    printf("step6_done (%s)\n", _modname.c_str());
    if (success) {
	step7_report();
    } else {
	XLOG_WARNING(("Can't validate stop of process " + _modname).c_str());
	//An error here isn't fatal - module manager will simply kill
	//the process less subtly.
	_taskmgr.module_manager()
	    .kill_module(_modname, callback(this, &Task::step7_report));
    }
}

void
Task::step7_report()
{
    printf("step7 (%s)\n", _modname.c_str());
    debug_msg("Task done\n");
    _task_complete_cb->dispatch(true, "");
}

void
Task::task_fail(string errmsg, bool fatal)
{
    debug_msg((errmsg + "\n").c_str());
    if (fatal) {
	XLOG_ERROR(("Shutting down fatally wounded process " + _modname)
		   .c_str());
	_taskmgr.kill_process(_modname);
    }
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

EventLoop&
Task::eventloop() const
{
    return _taskmgr.eventloop();
}


// ----------------------------------------------------------------------------
// TaskManager implementation

TaskManager::TaskManager(ModuleManager& mmgr, XorpClient& xclient,
			 bool global_do_exec)
    : _module_manager(mmgr), _xorp_client(xclient),
      _global_do_exec(global_do_exec)
{
}

TaskManager::~TaskManager()
{
    reset();
}

void
TaskManager::set_do_exec(bool do_exec)
{
    _current_do_exec = do_exec && _global_do_exec;
}

void
TaskManager::reset()
{
    while (!_tasks.empty()) {
	delete _tasks.begin()->second;
	_tasks.erase(_tasks.begin());
    }
    while (!_tasklist.empty()) {
	_tasklist.pop_front();
    }
}

int
TaskManager::add_module(const ModuleCommand& module_command)
{
    string modname = module_command.name();
    string modpath = module_command.path();
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

    Validation* validation = module_command.startup_validation(*this);
    find_task(modname).start_module(modname, validation);

    _module_commands[modname] = &module_command;
    return XORP_OK;
}

void
TaskManager::add_xrl(const string& modname, const UnexpandedXrl& xrl,
		     XrlRouter::XrlCallback& cb)
{
    Task& t(find_task(modname));
    t.add_xrl(xrl, cb);

    if (t.ready_validation() != NULL)
	return;

    XLOG_ASSERT(_module_commands.find(modname) != _module_commands.end());
    t.set_ready_validation(_module_commands[modname]->ready_validation(*this));
}

void
TaskManager::shutdown_module(const string& modname)
{
    Task& t(find_task(modname));
    XLOG_ASSERT(_module_commands.find(modname) != _module_commands.end());
    t.shutdown_module(modname,
		      _module_commands[modname]->shutdown_validation(*this),
		      _module_commands[modname]->shutdown_method(*this));
    _shutdown_order.push_front(&t);
}

void
TaskManager::run(CallBack cb)
{
    printf("TaskManager::run, tasks (old order): ");
    list <Task*>::const_iterator i;
    for (i = _tasklist.begin(); i != _tasklist.end(); i++) {
	printf("%s ", (*i)->name().c_str());
    }
    printf("\n");
    reorder_tasks();
    printf("TaskManager::run, tasks: ");
    for (i = _tasklist.begin(); i != _tasklist.end(); i++) {
	printf("%s ", (*i)->name().c_str());
    }
    printf("\n");
    _completion_cb = cb;
    run_task();
}

void
TaskManager::reorder_tasks()
{
    //we re-order the task list so that process shutdowns (which are
    //irreversable) occur last.
    list <Task*> configs;
    while (!_tasklist.empty()) {
	Task* t = _tasklist.front();
	if (t->will_shutdown_module()) {
	    //we already have a list of the correct order to shutdown
	    //modules in _shutdown_order
	} else {
	    configs.push_back(t);
	}
	_tasklist.pop_front();
    }

    //re-build the tasklist
    while (!configs.empty()) {
	_tasklist.push_back(configs.front());
	configs.pop_front();
    }
    list <Task*>::const_iterator i;
    for (i = _shutdown_order.begin();
	 i != _shutdown_order.end();
	 i++) {
	_tasklist.push_back(*i);
    }
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
    if (i == _tasks.end()) {
	//The task didn't exist, so we create one.  This is only valid
	//if we've already started the module.
	XLOG_ASSERT(_module_commands.find(modname) != _module_commands.end());
	const ModuleCommand* module_command = _module_commands[modname];
	string modname = module_command->name();
	string modpath = module_command->path();
	if (_tasks.find(modname) == _tasks.end()) {
	    Task* newtask = new Task(modname, *this);
	    _tasks[modname] = newtask;
	    _tasklist.push_back(newtask);
	}
	i = _tasks.find(modname);
    }
    assert(i != _tasks.end());
    assert(i->second != NULL);
    return *(i->second);
}

void
TaskManager::kill_process(const string& modname)
{
    //XXX We really should try to restart the failed process, but for
    //now we'll just kill it.
    XorpCallback0<void>::RefPtr null_cb;
    _module_manager.kill_module(modname, null_cb);
}

EventLoop&
TaskManager::eventloop() const
{
    return _module_manager.eventloop();
}
