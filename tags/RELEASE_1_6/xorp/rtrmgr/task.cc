// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

#ident "$XORP: xorp/rtrmgr/task.cc,v 1.64 2008/10/02 21:58:25 bms Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/utils.hh"

#include "master_conf_tree.hh"
#include "module_command.hh"
#include "module_manager.hh"
#include "task.hh"
#include "unexpanded_xrl.hh"
#include "xorp_client.hh"
#include "util.hh"


#define MAX_STATUS_RETRIES 30



// ----------------------------------------------------------------------------
// DelayValidation implementation
DelayValidation::DelayValidation(const string& module_name,
				 EventLoop& eventloop,
				 uint32_t ms,
				 bool verbose)
    : Validation(module_name, verbose),
      _eventloop(eventloop),
      _delay_in_ms(ms)
{
}

void
DelayValidation::validate(RunShellCommand::ExecId exec_id, CallBack cb)
{
    _cb = cb;
    _timer = _eventloop.new_oneoff_after_ms(_delay_in_ms,
			callback(this, &DelayValidation::timer_expired));
    UNUSED(exec_id);
}

void
DelayValidation::timer_expired()
{
    _cb->dispatch(true);
}


// ----------------------------------------------------------------------------
// XrlStatusValidation implementation

XrlStatusValidation::XrlStatusValidation(const string& module_name,
					 const XrlAction& xrl_action,
					 TaskManager& taskmgr)
    : Validation(module_name, taskmgr.verbose()),
      _xrl_action(xrl_action),
      _task_manager(taskmgr),
      _retries(0)
{
}

EventLoop&
XrlStatusValidation::eventloop()
{
    return _task_manager.eventloop();
}

void
XrlStatusValidation::validate(RunShellCommand::ExecId exec_id, CallBack cb)
{
    debug_msg("validate\n");

    _exec_id = exec_id;
    _cb = cb;

    if (_task_manager.do_exec()) {
	string xrl_request, errmsg;
	Xrl* xrl = NULL;
	do {
	    // Try to expand using the configuration tree
	    const MasterConfigTreeNode* ctn;
	    ctn = _task_manager.config_tree().find_config_module(_module_name);
	    if (ctn != NULL) {
		xrl = _xrl_action.expand_xrl_variables(*ctn, errmsg);
		if (xrl == NULL) {
		    XLOG_FATAL("Cannot expand XRL validation action %s "
			       "for module %s: %s",
			       _xrl_action.str().c_str(),
			       _module_name.c_str(),
			       errmsg.c_str());
		}
		break;
	    }

	    // Try to expand using the template tree
	    const TemplateTreeNode& ttn = _xrl_action.template_tree_node();
	    xrl = _xrl_action.expand_xrl_variables(ttn, errmsg);
	    if (xrl == NULL) {
		XLOG_FATAL("Cannot expand XRL validation action %s "
			   "for module %s: %s",
			   _xrl_action.str().c_str(),
			   _module_name.c_str(),
			   errmsg.c_str());
	    }
	    break;
	} while (false);
	if (xrl == NULL) {
	    XLOG_FATAL("Cannot expand XRL validation action %s for module %s",
		       _xrl_action.str().c_str(), _module_name.c_str());
	}

	XLOG_TRACE(_verbose, "Validating with XRL: >%s<\n",
		   xrl->str().c_str());

	string response = _xrl_action.xrl_return_spec();
	_task_manager.xorp_client().send_now(*xrl,
			callback(this, &XrlStatusValidation::xrl_done),
			response, true);
	delete xrl;
    } else {
	//
	// When we're running with do_exec == false, we want to
	// exercise most of the same machinery, but we want to ensure
	// that the xrl_done response gets the right arguments even
	// though we're not going to call the XRL.
	//
	_retry_timer = eventloop().new_oneoff_after_ms(1000,
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
XrlStatusValidation::xrl_done(const XrlError& e, XrlArgs* xrl_args)
{
    switch (e.error_code()) {
    case OKAY:
	try {
	    ProcessStatus status;
	    status = static_cast<ProcessStatus>(xrl_args->get_uint32("status"));
	    string reason(xrl_args->get_string("reason"));
	    handle_status_response(status, reason);
	} catch (XrlArgs::XrlAtomNotFound) {
	    // Not a valid response
	    XLOG_ERROR("Bad XRL response to get_status");
	    _cb->dispatch(false);
	}
	break;

    case BAD_ARGS:
    case COMMAND_FAILED:
    case NO_SUCH_METHOD:
	//
	// The template file must have been wrong - the target doesn't
	// support the common interface.
	// Just return true and hope everything's OK
	//
	XLOG_ERROR("Module %s doesn't support status validation",
		   _module_name.c_str());
	_cb->dispatch(true);
	break;

    case RESOLVE_FAILED:
    case SEND_FAILED:
    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	//
	// REPLY_TIMED_OUT => We did not yet receive a reply.
	// RESOLVE_FAILED => It's not yet registered with the finder -
	// retry after a short delay.
	// SEND_FAILED and SEND_FAILED_TRANSIENT=> ???  We're dealing with
	// startup conditions here, so give the problem a chance to resolve
	// itself.
	//
	// TODO: Make retries and delay configurable (no magic numbers).
	//
	_retries++;
	if (_retries > MAX_STATUS_RETRIES) {
	    _cb->dispatch(false);
	}
	_retry_timer = eventloop().new_oneoff_after_ms(1000,
			callback(this, &XrlStatusValidation::validate,
				 _exec_id, _cb));
	break;

    case INTERNAL_ERROR:
	XLOG_ERROR("Error while validating module %s", _module_name.c_str());
	XLOG_WARNING("Continuing anyway, cross your fingers...");
	_cb->dispatch(true);
	break;

    case NO_FINDER:
	// We're in trouble now! This shouldn't be able to happen.
	XLOG_UNREACHABLE();
	break;
    }
}


// ----------------------------------------------------------------------------
// ProgramStatusValidation implementation

ProgramStatusValidation::ProgramStatusValidation(
    const string& module_name,
    const ProgramAction& program_action,
    TaskManager& taskmgr)
    : Validation(module_name, taskmgr.verbose()),
      _program_action(program_action),
      _task_manager(taskmgr),
      _run_command(NULL)
{
}

ProgramStatusValidation::~ProgramStatusValidation()
{
    if (_run_command != NULL) {
	delete _run_command;
	_run_command = NULL;
    }
}

EventLoop&
ProgramStatusValidation::eventloop()
{
    return _task_manager.eventloop();
}

void
ProgramStatusValidation::validate(RunShellCommand::ExecId exec_id, CallBack cb)
{
    debug_msg("validate\n");

    _cb = cb;
    if (_task_manager.do_exec()) {
	string program_request, errmsg;
	do {
	    // Try to expand using the configuration tree
	    const MasterConfigTreeNode* ctn;
	    ctn = _task_manager.config_tree().find_config_module(_module_name);
	    if (ctn != NULL) {
		if (_program_action.expand_program_variables(*ctn,
							     program_request,
							     errmsg)
		    != XORP_OK) {
		    XLOG_FATAL("Cannot expand program validation action %s "
			       "for module %s: %s",
			       _program_action.str().c_str(),
			       _module_name.c_str(),
			       errmsg.c_str());
		}
		break;
	    }

	    // Try to expand using the template tree
	    const TemplateTreeNode& ttn = _program_action.template_tree_node();
	    if (_program_action.expand_program_variables(ttn, program_request,
							 errmsg)
		!= XORP_OK) {
		XLOG_FATAL("Cannot expand program validation action %s "
			   "for module %s: %s",
			   _program_action.str().c_str(),
			   _module_name.c_str(),
			   errmsg.c_str());
	    }
	    break;
	} while (false);
	if (program_request.empty()) {
	    XLOG_FATAL("Cannot expand program validation action %s for "
		       "module %s",
		       _program_action.str().c_str(), _module_name.c_str());
	}

	// Expand the executable program name
	string executable_filename, program_arguments;
	find_executable_filename_and_arguments(program_request,
					       executable_filename,
					       program_arguments);
	program_request = executable_filename;
	if (! program_arguments.empty())
	    program_request = program_request + " " + program_arguments;
	if (executable_filename.empty()) {
	    XLOG_ERROR("Could not find program %s", program_request.c_str());
	    return;
	}

	// Run the program
	XLOG_TRACE(_verbose, "Validating with program: >%s<\n",
		   program_request.c_str());
	XLOG_ASSERT(_run_command == NULL);
	_run_command = new RunShellCommand(
	    eventloop(),
	    executable_filename,
	    program_arguments,
	    callback(this, &ProgramStatusValidation::stdout_cb),
	    callback(this, &ProgramStatusValidation::stderr_cb),
	    callback(this, &ProgramStatusValidation::done_cb));
	_run_command->set_exec_id(exec_id);
	if (_run_command->execute() != XORP_OK) {
	    delete _run_command;
	    _run_command = NULL;
	    XLOG_ERROR("Could not execute program %s",
		       program_request.c_str());
	    return;
	}
    } else {
	//
	// When we're running with do_exec == false, we want to
	// exercise most of the same machinery, hence we schedule
	// a dummy callback as if the program was called.
	//
	_delay_timer = eventloop().new_oneoff_after(
	    TimeVal(0, 0),
	    callback(this, &ProgramStatusValidation::execute_done, true));
    }
}

void
ProgramStatusValidation::stdout_cb(RunShellCommand* run_command,
				   const string& output)
{
    XLOG_ASSERT(run_command == _run_command);
    _command_stdout += output;
}

void
ProgramStatusValidation::stderr_cb(RunShellCommand* run_command,
				   const string& output)
{
    XLOG_ASSERT(run_command == _run_command);
    _command_stderr += output;
}

void
ProgramStatusValidation::done_cb(RunShellCommand* run_command,
				 bool success,
				 const string& error_msg)
{
    XLOG_ASSERT(run_command == _run_command);

    if (! success)
	_command_stderr += error_msg;

    if (_run_command != NULL) {
	delete _run_command;
	_run_command = NULL;
    }

    execute_done(success);
}

void
ProgramStatusValidation::execute_done(bool success)
{
    handle_status_response(success, _command_stdout, _command_stderr);
}


// ----------------------------------------------------------------------------
// XrlStatusStartupValidation implementation

XrlStatusStartupValidation::XrlStatusStartupValidation(
    const string& module_name,
    const XrlAction& xrl_action,
    TaskManager& taskmgr)
    : XrlStatusValidation(module_name, xrl_action, taskmgr)
{
}

void
XrlStatusStartupValidation::handle_status_response(ProcessStatus status,
						   const string& reason)
{
    switch (status) {
    case PROC_NULL:
	// This is not a valid responses.
	XLOG_ERROR("Bad status response; reason: %s", reason.c_str());
	_cb->dispatch(false);
	return;
    case PROC_FAILED:
    case PROC_SHUTDOWN:
    case PROC_DONE:
	_cb->dispatch(false);
	return;
    case PROC_STARTUP:
    case PROC_NOT_READY:
    case PROC_READY:
	// The process is ready to be activated
	_cb->dispatch(true);
	return;
    }
    XLOG_UNREACHABLE();
}


// ----------------------------------------------------------------------------
// ProgramStatusStartupValidation implementation

ProgramStatusStartupValidation::ProgramStatusStartupValidation(
    const string& module_name,
    const ProgramAction& program_action,
    TaskManager& taskmgr)
    : ProgramStatusValidation(module_name, program_action, taskmgr)
{
}

void
ProgramStatusStartupValidation::handle_status_response(
    bool success,
    const string& stdout_output,
    const string& stderr_output)
{
    if (! _cb.is_empty()) {
	_cb->dispatch(success);
    }

    UNUSED(stdout_output);
    UNUSED(stderr_output);
}


// ----------------------------------------------------------------------------
// XrlStatusReadyValidation implementation

XrlStatusReadyValidation::XrlStatusReadyValidation(
    const string& module_name,
    const XrlAction& xrl_action,
    TaskManager& taskmgr)
    : XrlStatusValidation(module_name, xrl_action, taskmgr)
{
}

void
XrlStatusReadyValidation::handle_status_response(ProcessStatus status,
						 const string& reason)
{
    switch (status) {
    case PROC_NULL:
	// This is not a valid response.
	XLOG_ERROR("Bad status response; reason: %s", reason.c_str());
	_cb->dispatch(false);
	return;
    case PROC_FAILED:
    case PROC_SHUTDOWN:
    case PROC_DONE:
	_cb->dispatch(false);
	return;
    case PROC_STARTUP:
    case PROC_NOT_READY:
	// Got a valid response saying we should wait.
	_retry_timer = eventloop().new_oneoff_after_ms(1000,
			callback((XrlStatusValidation*)this,
				 &XrlStatusValidation::validate,
				 _exec_id, _cb));
	return;
    case PROC_READY:
	// The process is ready
	_cb->dispatch(true);
	return;
    }
    XLOG_UNREACHABLE();
}


// ----------------------------------------------------------------------------
// ProgramStatusReadyValidation implementation

ProgramStatusReadyValidation::ProgramStatusReadyValidation(
    const string& module_name,
    const ProgramAction& program_action,
    TaskManager& taskmgr)
    : ProgramStatusValidation(module_name, program_action, taskmgr)
{
}

void
ProgramStatusReadyValidation::handle_status_response(
    bool success,
    const string& stdout_output,
    const string& stderr_output)
{
    if (! _cb.is_empty()) {
	_cb->dispatch(success);
    }

    UNUSED(stdout_output);
    UNUSED(stderr_output);
}


// ----------------------------------------------------------------------------
// XrlStatusConfigMeValidation implementation

XrlStatusConfigMeValidation::XrlStatusConfigMeValidation(
    const string& module_name,
    const XrlAction& xrl_action,
    TaskManager& taskmgr)
    : XrlStatusValidation(module_name, xrl_action, taskmgr)
{
}

void
XrlStatusConfigMeValidation::handle_status_response(ProcessStatus status,
						    const string& reason)
{
    switch (status) {
    case PROC_NULL:
	// This is not a valid responses.
	XLOG_ERROR("Bad status response; reason: %s", reason.c_str());
	_cb->dispatch(false);
	return;
    case PROC_FAILED:
    case PROC_SHUTDOWN:
    case PROC_DONE:
	_cb->dispatch(false);
	return;
    case PROC_STARTUP:
	// Got a valid response saying we should wait.
	_retry_timer = eventloop().new_oneoff_after_ms(1000,
			callback((XrlStatusValidation*)this,
				 &XrlStatusValidation::validate,
				 _exec_id, _cb));
	return;
    case PROC_NOT_READY:
    case PROC_READY:
	// The process is ready to be configured
	_cb->dispatch(true);
	return;
    }
    XLOG_UNREACHABLE();
}


// ----------------------------------------------------------------------------
// ProgramStatusConfigMeValidation implementation

ProgramStatusConfigMeValidation::ProgramStatusConfigMeValidation(
    const string& module_name,
    const ProgramAction& program_action,
    TaskManager& taskmgr)
    : ProgramStatusValidation(module_name, program_action, taskmgr)
{
}

void
ProgramStatusConfigMeValidation::handle_status_response(
    bool success,
    const string& stdout_output,
    const string& stderr_output)
{
    if (! _cb.is_empty()) {
	_cb->dispatch(success);
    }

    UNUSED(stdout_output);
    UNUSED(stderr_output);
}

// ----------------------------------------------------------------------------
// XrlStatusShutdownValidation implementation

XrlStatusShutdownValidation::XrlStatusShutdownValidation(
    const string& module_name,
    const XrlAction& xrl_action,
    TaskManager& taskmgr)
    : XrlStatusValidation(module_name, xrl_action, taskmgr)
{
}

void
XrlStatusShutdownValidation::handle_status_response(ProcessStatus status,
						    const string& reason)
{
    switch (status) {
    case PROC_NULL:
	// This is not a valid responses.
	XLOG_ERROR("Bad status response; reason: %s", reason.c_str());
	_cb->dispatch(false);
	return;
    case PROC_STARTUP:
    case PROC_NOT_READY:
    case PROC_READY:
    case PROC_FAILED:
	// The process should be in PROC_SHUTDOWN state, or it should not
	// be able to respond to Xrls because it's in NULL state.
	_cb->dispatch(false);
	return;
    case PROC_DONE:
	// The process has completed operation
	_cb->dispatch(true);
	return;
    case PROC_SHUTDOWN:
	// Got a valid response saying we should wait.
	_retry_timer = eventloop().new_oneoff_after_ms(1000,
			callback((XrlStatusValidation*)this,
				 &XrlStatusValidation::validate,
				 _exec_id, _cb));
	return;
    }
    XLOG_UNREACHABLE();
}

void
XrlStatusShutdownValidation::xrl_done(const XrlError& e, XrlArgs* xrl_args)
{
    switch (e.error_code()) {
    case OKAY:
	try {
	    ProcessStatus status;
	    status = static_cast<ProcessStatus>(xrl_args->get_uint32("status"));
	    string reason(xrl_args->get_string("reason"));
	    handle_status_response(status, reason);
	} catch (XrlArgs::XrlAtomNotFound) {
	    // Not a valid response
	    XLOG_ERROR("Bad XRL response to get_status");
	    _cb->dispatch(false);
	}
	break;

    case BAD_ARGS:
    case NO_SUCH_METHOD:
    case COMMAND_FAILED:
	//
	// The template file must have been wrong - the target doesn't
	// support the common interface.
	// Return false, and we can shut it down using kill.
	//
	XLOG_ERROR("Module %s doesn't support shutdown validation",
		   _module_name.c_str());
	_cb->dispatch(false);
	break;

    case RESOLVE_FAILED:
    case REPLY_TIMED_OUT:
    case SEND_FAILED:
    case SEND_FAILED_TRANSIENT:
	//
	// The process appears to have shutdown correctly or the process
	// is probably gone.
	//
	_cb->dispatch(true);
	break;

    case INTERNAL_ERROR:
	// Return false, and we can shut it down using kill.
	XLOG_ERROR("Error while shutdown validation of module %s",
		   _module_name.c_str());
	_cb->dispatch(false);
	break;

    case NO_FINDER:
	// We're in trouble now! This shouldn't be able to happen.
	XLOG_UNREACHABLE();
	break;
    }
}

// ----------------------------------------------------------------------------
// ProgramStatusShutdownValidation implementation

ProgramStatusShutdownValidation::ProgramStatusShutdownValidation(
    const string& module_name,
    const ProgramAction& program_action,
    TaskManager& taskmgr)
    : ProgramStatusValidation(module_name, program_action, taskmgr)
{
}

void
ProgramStatusShutdownValidation::handle_status_response(
    bool success,
    const string& stdout_output,
    const string& stderr_output)
{
    if (! _cb.is_empty()) {
	_cb->dispatch(success);
    }

    UNUSED(stdout_output);
    UNUSED(stderr_output);
}


// ----------------------------------------------------------------------------
// Startup implementation

Startup::Startup(const string& module_name, bool verbose)
    : _module_name(module_name),
      _verbose(verbose)
{
}


// ----------------------------------------------------------------------------
// XrlStartup implementation

XrlStartup::XrlStartup(const string& module_name,
		       const XrlAction& xrl_action,
		       TaskManager& taskmgr)
    : Startup(module_name, taskmgr.verbose()),
      _xrl_action(xrl_action),
      _task_manager(taskmgr)
{
}

EventLoop&
XrlStartup::eventloop() const
{
    return _task_manager.eventloop();
}

void
XrlStartup::startup(const RunShellCommand::ExecId& exec_id, CallBack cb)
{
    _cb = cb;
    if (_task_manager.do_exec()) {
	string xrl_request, errmsg;
	Xrl* xrl = NULL;
	do {
	    // Try to expand using the configuration tree
	    const MasterConfigTreeNode* ctn;
	    ctn = _task_manager.config_tree().find_config_module(_module_name);
	    if (ctn != NULL) {
		xrl = _xrl_action.expand_xrl_variables(*ctn, errmsg);
		if (xrl == NULL) {
		    XLOG_FATAL("Cannot expand XRL startup action %s "
			       "for module %s: %s",
			       _xrl_action.str().c_str(),
			       _module_name.c_str(),
			       errmsg.c_str());
		}
		break;
	    }

	    // Try to expand using the template tree
	    const TemplateTreeNode& ttn = _xrl_action.template_tree_node();
	    xrl = _xrl_action.expand_xrl_variables(ttn, errmsg);
	    if (xrl == NULL) {
		XLOG_FATAL("Cannot expand XRL startup action %s "
			   "for module %s: %s",
			   _xrl_action.str().c_str(),
			   _module_name.c_str(),
			   errmsg.c_str());
	    }
	    break;
	} while (false);
	if (xrl == NULL) {
	    XLOG_ERROR("Cannot expand XRL startup action %s for module %s",
		       _xrl_action.str().c_str(), _module_name.c_str());
	    return;
	}

	XLOG_TRACE(_verbose, "Startup with XRL: >%s<\n", xrl->str().c_str());

	string response = _xrl_action.xrl_return_spec();
	_task_manager.xorp_client().send_now(*xrl,
				callback(this, &XrlStartup::startup_done),
					     response, true);
	delete xrl;
    } else {
	//
	// When we're running with do_exec == false, we want to
	// exercise most of the same machinery, but we want to ensure
	// that the xrl_done response gets the right arguments even
	// though we're not going to call the XRL.
	//
	XLOG_TRACE(_verbose, "XRL: dummy call to %s\n",
		   _xrl_action.request().c_str());
	_dummy_timer = eventloop().new_oneoff_after_ms(1000,
			callback(this, &XrlStartup::dummy_response));
    }

    UNUSED(exec_id);
}

void
XrlStartup::dummy_response()
{
    XrlError e = XrlError::OKAY();
    XrlArgs a;

    startup_done(e, &a);
}

void
XrlStartup::startup_done(const XrlError& err, XrlArgs* xrl_args)
{
    UNUSED(xrl_args);

    if (err == XrlError::OKAY()) {
	// Success
	_cb->dispatch(true);
    } else {
	// Failure
	_cb->dispatch(false);
    }
}


// ----------------------------------------------------------------------------
// ProgramStartup implementation

ProgramStartup::ProgramStartup(const string& module_name,
			       const ProgramAction& program_action,
			       TaskManager& taskmgr)
    : Startup(module_name, taskmgr.verbose()),
      _program_action(program_action),
      _task_manager(taskmgr),
      _run_command(NULL)
{
}

ProgramStartup::~ProgramStartup()
{
    if (_run_command != NULL) {
	delete _run_command;
	_run_command = NULL;
    }
}

EventLoop&
ProgramStartup::eventloop() const
{
    return _task_manager.eventloop();
}

void
ProgramStartup::startup(const RunShellCommand::ExecId& exec_id, CallBack cb)
{
    _cb = cb;
    if (_task_manager.do_exec()) {
	string program_request, errmsg;
	do {
	    // Try to expand using the configuration tree
	    const MasterConfigTreeNode* ctn;
	    ctn = _task_manager.config_tree().find_config_module(_module_name);
	    if (ctn != NULL) {
		if (_program_action.expand_program_variables(*ctn,
							     program_request,
							     errmsg)
		    != XORP_OK) {
		    XLOG_FATAL("Cannot expand program startup action %s "
			       "for module %s: %s",
			       _program_action.str().c_str(),
			       _module_name.c_str(),
			       errmsg.c_str());
		}
		break;
	    }

	    // Try to expand using the template tree
	    const TemplateTreeNode& ttn = _program_action.template_tree_node();
	    if (_program_action.expand_program_variables(ttn, program_request,
							 errmsg)
		!= XORP_OK) {
		XLOG_FATAL("Cannot expand program startup action %s "
			   "for module %s: %s",
			   _program_action.str().c_str(),
			   _module_name.c_str(),
			   errmsg.c_str());
	    }
	    break;
	} while (false);
	if (program_request.empty()) {
	    XLOG_ERROR("Cannot expand program startup action %s for "
		       "module %s",
		       _program_action.str().c_str(), _module_name.c_str());
	    return;
	}

	// Expand the executable program name
	string executable_filename, program_arguments;
	find_executable_filename_and_arguments(program_request,
					       executable_filename,
					       program_arguments);
	program_request = executable_filename;
	if (! program_arguments.empty())
	    program_request = program_request + " " + program_arguments;
	if (executable_filename.empty()) {
	    XLOG_ERROR("Could not find program %s", program_request.c_str());
	    return;
	}

	// Run the program
	XLOG_TRACE(_verbose, "Startup with program: >%s<\n",
		   program_request.c_str());
	XLOG_ASSERT(_run_command == NULL);
	_run_command = new RunShellCommand(
	    eventloop(),
	    executable_filename,
	    program_arguments,
	    callback(this, &ProgramStartup::stdout_cb),
	    callback(this, &ProgramStartup::stderr_cb),
	    callback(this, &ProgramStartup::done_cb));
	_run_command->set_exec_id(exec_id);
	if (_run_command->execute() != XORP_OK) {
	    delete _run_command;
	    _run_command = NULL;
	    XLOG_ERROR("Could not execute program %s",
		       program_request.c_str());
	    return;
	}
    } else {
	//
	// When we're running with do_exec == false, we want to
	// exercise most of the same machinery, hence we schedule
	// a dummy callback as if the program was called.
	//
	XLOG_TRACE(_verbose, "Program: dummy call to %s\n",
		   _program_action.request().c_str());
	_delay_timer = eventloop().new_oneoff_after(
	    TimeVal(0, 0),
	    callback(this, &ProgramStartup::execute_done, true));
    }
}

void
ProgramStartup::stdout_cb(RunShellCommand* run_command,
			  const string& output)
{
    XLOG_ASSERT(run_command == _run_command);
    _command_stdout += output;
}

void
ProgramStartup::stderr_cb(RunShellCommand* run_command,
			  const string& output)
{
    XLOG_ASSERT(run_command == _run_command);
    _command_stderr += output;
}

void
ProgramStartup::done_cb(RunShellCommand* run_command,
			bool success,
			const string& error_msg)
{
    XLOG_ASSERT(run_command == _run_command);

    if (! success)
	_command_stderr += error_msg;

    if (_run_command != NULL) {
	delete _run_command;
	_run_command = NULL;
    }

    execute_done(success);
}

void
ProgramStartup::execute_done(bool success)
{
    if (! _cb.is_empty()) {
	_cb->dispatch(success);
    }
}

// ----------------------------------------------------------------------------
// Shutdown implementation

Shutdown::Shutdown(const string& module_name, bool verbose)
    : _module_name(module_name),
      _verbose(verbose)
{
}


// ----------------------------------------------------------------------------
// XrlShutdown implementation

XrlShutdown::XrlShutdown(const string& module_name,
			 const XrlAction& xrl_action,
			 TaskManager& taskmgr)
    : Shutdown(module_name, taskmgr.verbose()),
      _xrl_action(xrl_action),
      _task_manager(taskmgr)
{
}

EventLoop&
XrlShutdown::eventloop() const
{
    return _task_manager.eventloop();
}

void
XrlShutdown::shutdown(const RunShellCommand::ExecId& exec_id, CallBack cb)
{
    if (! _task_manager.is_verification())
	XLOG_INFO("Shutting down module: %s\n", _module_name.c_str());

    _cb = cb;
    if (_task_manager.do_exec()) {
	string xrl_request, errmsg;
	Xrl* xrl = NULL;
	do {
	    // Try to expand using the configuration tree
	    const MasterConfigTreeNode* ctn;
	    ctn = _task_manager.config_tree().find_config_module(_module_name);
	    if (ctn != NULL) {
		xrl = _xrl_action.expand_xrl_variables(*ctn, errmsg);
		if (xrl == NULL) {
		    XLOG_FATAL("Cannot expand XRL shutdown action %s "
			       "for module %s: %s",
			       _xrl_action.str().c_str(),
			       _module_name.c_str(),
			       errmsg.c_str());
		}
		break;
	    }

	    // Try to expand using the template tree
	    const TemplateTreeNode& ttn = _xrl_action.template_tree_node();
	    xrl = _xrl_action.expand_xrl_variables(ttn, errmsg);
	    if (xrl == NULL) {
		XLOG_FATAL("Cannot expand XRL shutdown action %s "
			   "for module %s: %s",
			   _xrl_action.str().c_str(),
			   _module_name.c_str(),
			   errmsg.c_str());
	    }
	    break;
	} while (false);
	if (xrl == NULL) {
	    XLOG_ERROR("Cannot expand XRL shutdown action %s for module %s",
		       _xrl_action.str().c_str(), _module_name.c_str());
	    return;
	}

	XLOG_TRACE(_verbose, "Shutdown with XRL: >%s<\n", xrl->str().c_str());

	string response = _xrl_action.xrl_return_spec();
	_task_manager.xorp_client().send_now(*xrl,
				callback(this, &XrlShutdown::shutdown_done),
					     response, true);
	delete xrl;
    } else {
	//
	// When we're running with do_exec == false, we want to
	// exercise most of the same machinery, but we want to ensure
	// that the xrl_done response gets the right arguments even
	// though we're not going to call the XRL.
	//
	XLOG_TRACE(_verbose, "XRL: dummy call to %s\n",
		   _xrl_action.request().c_str());
	_dummy_timer = eventloop().new_oneoff_after_ms(1000,
			callback(this, &XrlShutdown::dummy_response));
    }

    UNUSED(exec_id);
}

void
XrlShutdown::dummy_response()
{
    XrlError e = XrlError::OKAY();
    XrlArgs a;

    shutdown_done(e, &a);
}

void
XrlShutdown::shutdown_done(const XrlError& err, XrlArgs* xrl_args)
{
    switch (err.error_code()) {
    case OKAY:
    case RESOLVE_FAILED:
    case SEND_FAILED:
    case REPLY_TIMED_OUT:
    case SEND_FAILED_TRANSIENT:
	// Success - either it said it would shutdown, or it's already gone.
	_cb->dispatch(true);
	break;

    case BAD_ARGS:
    case COMMAND_FAILED:
    case NO_SUCH_METHOD:
    case INTERNAL_ERROR:	// TODO: should be XLOG_UNREACHABLE()?
	// We may have to kill it.
	_cb->dispatch(false);
	break;

    case NO_FINDER:
	XLOG_UNREACHABLE();
	break;
    }

    UNUSED(xrl_args);
}


// ----------------------------------------------------------------------------
// ProgramShutdown implementation

ProgramShutdown::ProgramShutdown(const string& module_name,
				 const ProgramAction& program_action,
				 TaskManager& taskmgr)
    : Shutdown(module_name, taskmgr.verbose()),
      _program_action(program_action),
      _task_manager(taskmgr),
      _run_command(NULL)
{
}

ProgramShutdown::~ProgramShutdown()
{
    if (_run_command != NULL) {
	delete _run_command;
	_run_command = NULL;
    }
}

EventLoop&
ProgramShutdown::eventloop() const
{
    return _task_manager.eventloop();
}

void
ProgramShutdown::shutdown(const RunShellCommand::ExecId& exec_id, CallBack cb)
{
    if (! _task_manager.is_verification())
	XLOG_INFO("Shutting down module: %s\n", _module_name.c_str());

    _cb = cb;
    if (_task_manager.do_exec()) {
	string program_request, errmsg;
	do {
	    // Try to expand using the configuration tree
	    const MasterConfigTreeNode* ctn;
	    ctn = _task_manager.config_tree().find_config_module(_module_name);
	    if (ctn != NULL) {
		if (_program_action.expand_program_variables(*ctn,
							     program_request,
							     errmsg)
		    != XORP_OK) {
		    XLOG_FATAL("Cannot expand program shutdown action %s "
			       "for module %s: %s",
			       _program_action.str().c_str(),
			       _module_name.c_str(),
			       errmsg.c_str());
		}
		break;
	    }

	    // Try to expand using the template tree
	    const TemplateTreeNode& ttn = _program_action.template_tree_node();
	    if (_program_action.expand_program_variables(ttn, program_request,
							 errmsg)
		!= XORP_OK) {
		XLOG_FATAL("Cannot expand program shutdown action %s "
			   "for module %s: %s",
			   _program_action.str().c_str(),
			   _module_name.c_str(),
			   errmsg.c_str());
	    }
	    break;
	} while (false);
	if (program_request.empty()) {
	    XLOG_ERROR("Cannot expand program shutdown action %s for "
		       "module %s",
		       _program_action.str().c_str(), _module_name.c_str());
	    return;
	}

	// Expand the executable program name
	string executable_filename, program_arguments;
	find_executable_filename_and_arguments(program_request,
					       executable_filename,
					       program_arguments);
	program_request = executable_filename;
	if (! program_arguments.empty())
	    program_request = program_request + " " + program_arguments;
	if (executable_filename.empty()) {
	    XLOG_ERROR("Could not find program %s", program_request.c_str());
	    return;
	}

	// Run the program
	XLOG_TRACE(_verbose, "Shutdown with program: >%s<\n",
		   program_request.c_str());
	XLOG_ASSERT(_run_command == NULL);
	_run_command = new RunShellCommand(
	    eventloop(),
	    executable_filename,
	    program_arguments,
	    callback(this, &ProgramShutdown::stdout_cb),
	    callback(this, &ProgramShutdown::stderr_cb),
	    callback(this, &ProgramShutdown::done_cb));
	_run_command->set_exec_id(exec_id);
	if (_run_command->execute() != XORP_OK) {
	    delete _run_command;
	    _run_command = NULL;
	    XLOG_ERROR("Could not execute program %s",
		       program_request.c_str());
	    return;
	}
    } else {
	//
	// When we're running with do_exec == false, we want to
	// exercise most of the same machinery, hence we schedule
	// a dummy callback as if the program was called.
	//
	XLOG_TRACE(_verbose, "Program: dummy call to %s\n",
		   _program_action.request().c_str());
	_delay_timer = eventloop().new_oneoff_after(
	    TimeVal(0, 0),
	    callback(this, &ProgramShutdown::execute_done, true));
    }
}

void
ProgramShutdown::stdout_cb(RunShellCommand* run_command,
			   const string& output)
{
    XLOG_ASSERT(run_command == _run_command);
    _command_stdout += output;
}

void
ProgramShutdown::stderr_cb(RunShellCommand* run_command,
			   const string& output)
{
    XLOG_ASSERT(run_command == _run_command);
    _command_stderr += output;
}

void
ProgramShutdown::done_cb(RunShellCommand* run_command,
			 bool success,
			 const string& error_msg)
{
    XLOG_ASSERT(run_command == _run_command);

    if (! success)
	_command_stderr += error_msg;

    if (_run_command != NULL) {
	delete _run_command;
	_run_command = NULL;
    }

    execute_done(success);
}

void
ProgramShutdown::execute_done(bool success)
{
    if (! _cb.is_empty()) {
	_cb->dispatch(success);
    }
}


// ----------------------------------------------------------------------------
// TaskXrlItem implementation

const uint32_t	TaskXrlItem::DEFAULT_RESEND_COUNT = 10;
const int	TaskXrlItem::DEFAULT_RESEND_DELAY_MS = 1000;

TaskXrlItem::TaskXrlItem(const UnexpandedXrl& uxrl,
			 const XrlRouter::XrlCallback& cb,
			 Task& task,
			 uint32_t xrl_resend_count,
			 int xrl_resend_delay_ms)
    : TaskBaseItem(task),
      _unexpanded_xrl(uxrl),
      _xrl_callback(cb),
      _xrl_resend_count_limit(xrl_resend_count),
      _xrl_resend_count(_xrl_resend_count_limit),
      _xrl_resend_delay_ms(xrl_resend_delay_ms),
      _verbose(task.verbose())
{
}

TaskXrlItem::TaskXrlItem(const TaskXrlItem& them)
    : TaskBaseItem(them),
      _unexpanded_xrl(them._unexpanded_xrl),
      _xrl_callback(them._xrl_callback),
      _xrl_resend_count_limit(them._xrl_resend_count_limit),
      _xrl_resend_count(_xrl_resend_count_limit),
      _xrl_resend_delay_ms(them._xrl_resend_delay_ms),
      _verbose(them._verbose)
{
}

bool
TaskXrlItem::execute(string& errmsg)
{
    if (task().do_exec())
	XLOG_TRACE(_verbose, "Expanding %s\n", _unexpanded_xrl.str().c_str());

    Xrl* xrl = _unexpanded_xrl.expand(errmsg);
    if (xrl == NULL) {
	errmsg = c_format("Failed to expand XRL %s: %s",
			  _unexpanded_xrl.str().c_str(), errmsg.c_str());
	return false;
    }
    if (task().do_exec())
	XLOG_TRACE(_verbose, "Executing XRL: >%s<\n", xrl->str().c_str());

    string xrl_return_spec = _unexpanded_xrl.return_spec();

    _xrl_resend_count = _xrl_resend_count_limit;
    task().xorp_client().send_now(*xrl,
				  callback(this, &TaskXrlItem::execute_done),
				  xrl_return_spec,
				  task().do_exec());
    return true;
}

void
TaskXrlItem::unschedule()
{
    XrlArgs xrl_args;	    // Empty XRL args should be OK here.

    debug_msg("TaskXrlItem::unschedule()\n");

    //
    // We need to dispatch the callbacks, or the accounting of which
    // actions were taken will be incorrect.
    //
    _xrl_callback->dispatch(XrlError::OKAY(), &xrl_args);
}

void
TaskXrlItem::resend()
{
    string errmsg;

    Xrl* xrl = _unexpanded_xrl.expand(errmsg);
    if (xrl == NULL) {
	// This can't happen in a resend, because we already succeeded
	// in the orginal send.
	XLOG_UNREACHABLE();
    }

    string xrl_return_spec = _unexpanded_xrl.return_spec();

    task().xorp_client().send_now(*xrl,
				  callback(this, &TaskXrlItem::execute_done),
				  xrl_return_spec,
				  task().do_exec());
}

void
TaskXrlItem::execute_done(const XrlError& err, XrlArgs* xrl_args)
{
    bool fatal = false;

    debug_msg("TaskXrlItem::execute_done\n");

    switch (err.error_code()) {
    case OKAY:
	break;

    case BAD_ARGS:
    case COMMAND_FAILED:
	// Non-fatal error for the target - typically this is
	// because the user entered bad information - we can't
	// change the target configuration to what the user
	// specified, but this doesn't mean the target is fatally
	// wounded.
	fatal = false;
	break;

    case NO_FINDER:
    case SEND_FAILED:
	// The error was a fatal one for the target - we now
	// consider the target to be fatally wounded.
	XLOG_ERROR("%s", err.str().c_str());
	fatal = true;
	break;

    case REPLY_TIMED_OUT:
    case RESOLVE_FAILED:
    case SEND_FAILED_TRANSIENT:
	// REPLY_TIMED_OUT shouldn't happen on a reliable
	// transport, but at this point we don't know for sure
	// which Xrls are reliable and while are unreliable, so
	// best handle this as if it were packet loss.
	//
	// RESOLVE_FAILED shouldn't happen if the startup
	// validation has done it's job correctly, but just in case
	// we'll be lenient and give it ten more seconds to be
	// functioning before we declare it dead.
	if (--_xrl_resend_count > 0) {
	    // Re-send the Xrl after a short delay.
	    _xrl_resend_timer = task().eventloop().new_oneoff_after_ms(
		_xrl_resend_delay_ms,
		callback(this, &TaskXrlItem::resend));
	    return;
	} else {
	    // Give up.
	    // The error was a fatal one for the target - we now
	    // consider the target to be fatally wounded.
	    XLOG_ERROR("%s", err.str().c_str());
	    fatal = true;
	}
	break;

    case INTERNAL_ERROR:
    case NO_SUCH_METHOD:
	// Something bad happened but it's not clear what.  Don't
	// consider these to be fatal errors, but they may well
	// prove to be so.  XXX revisit this issue when we've more
	// experience with XRL errors.
	XLOG_ERROR("%s", err.str().c_str());
	fatal = false;
	break;
    }

    if (! _xrl_callback.is_empty())
	_xrl_callback->dispatch(err, xrl_args);

    bool success = true;
    string errmsg;
    if (err != XrlError::OKAY()) {
	success = false;
	errmsg = err.str();
    }
    task().item_done(success, fatal, errmsg);
}


// ----------------------------------------------------------------------------
// TaskProgramItem implementation

TaskProgramItem::TaskProgramItem(const UnexpandedProgram&	program,
				 TaskProgramItem::ProgramCallback program_cb,
				 Task&				task)
    : TaskBaseItem(task),
      _unexpanded_program(program),
      _run_command(NULL),
      _program_cb(program_cb),
      _verbose(task.verbose())
{
}

TaskProgramItem::TaskProgramItem(const TaskProgramItem& them)
    : TaskBaseItem(them),
      _unexpanded_program(them._unexpanded_program),
      _run_command(NULL),
      _program_cb(them._program_cb),
      _verbose(them._verbose)
{
}

TaskProgramItem::~TaskProgramItem()
{
    if (_run_command != NULL) {
	delete _run_command;
	_run_command = NULL;
    }
}

bool
TaskProgramItem::execute(string& errmsg)
{
    const RunShellCommand::ExecId& exec_id = task().exec_id();

    if (task().do_exec())
	XLOG_TRACE(_verbose, "Expanding %s\n",
		   _unexpanded_program.str().c_str());

    string program_request = _unexpanded_program.expand(errmsg);
    if (program_request.empty()) {
	errmsg = c_format("Failed to expand program %s: %s",
			  _unexpanded_program.str().c_str(), errmsg.c_str());
	return false;
    }

    if (_run_command != NULL)
	return (true);		// XXX: already running

    if (task().do_exec()) {
	// Expand the executable program name
	string executable_filename, program_arguments;
	find_executable_filename_and_arguments(program_request,
					       executable_filename,
					       program_arguments);
	if (executable_filename.empty()) {
	    errmsg = c_format("Could not find program %s",
			      program_request.c_str());
	    return (false);
	}
	program_request = executable_filename;
	if (! program_arguments.empty())
	    program_request = program_request + " " + program_arguments;
	XLOG_TRACE(_verbose, "Executing program: >%s<\n",
		   program_request.c_str());
	_run_command = new RunShellCommand(
	    task().eventloop(),
	    executable_filename,
	    program_arguments,
	    callback(this, &TaskProgramItem::stdout_cb),
	    callback(this, &TaskProgramItem::stderr_cb),
	    callback(this, &TaskProgramItem::done_cb));
	_run_command->set_exec_id(exec_id);
	if (_run_command->execute() != XORP_OK) {
	    delete _run_command;
	    _run_command = NULL;
	    errmsg = c_format("Could not execute program %s",
			      program_request.c_str());
	    return (false);
	}
    } else {
	//
	// When we're running with do_exec == false, we want to
	// exercise most of the same machinery, hence we schedule
	// a dummy callback as if the program was called.
	//
	_delay_timer = task().eventloop().new_oneoff_after(
	    TimeVal(0, 0),
	    callback(this, &TaskProgramItem::execute_done, true));
    }

    return (true);
}

void
TaskProgramItem::unschedule()
{
    debug_msg("TaskProgramItem::unschedule()\n");

    if (_run_command != NULL)
	_run_command->terminate();

    //
    // We need to dispatch the callbacks, or the accounting of which
    // actions were taken will be incorrect.
    //
    string error_msg = c_format("The execution of program %s is terminated",
				_unexpanded_program.str().c_str());
    if (! _program_cb.is_empty()) {
	_program_cb->dispatch(false, _command_stdout, error_msg,
			      task().do_exec());
    }

    if (_run_command != NULL) {
	delete _run_command;
	_run_command = NULL;
    }
}

void
TaskProgramItem::stdout_cb(RunShellCommand* run_command, const string& output)
{
    debug_msg("TaskProgramItem::stdout_cb\n");

    XLOG_ASSERT(run_command == _run_command);
    _command_stdout += output;
}

void
TaskProgramItem::stderr_cb(RunShellCommand* run_command, const string& output)
{
    debug_msg("TaskProgramItem::stderr_cb\n");

    XLOG_ASSERT(run_command == _run_command);
    _command_stderr += output;
}

void
TaskProgramItem::done_cb(RunShellCommand* run_command,
			 bool success,
			 const string& error_msg)
{
    debug_msg("TaskProgramItem::done_cb\n");

    XLOG_ASSERT(run_command == _run_command);

    if (! success)
	_command_stderr += error_msg;

    if (_run_command != NULL) {
	delete _run_command;
	_run_command = NULL;
    }

    execute_done(success);
}

void
TaskProgramItem::execute_done(bool success)
{
    bool fatal = false;

    debug_msg("TaskProgramItem::execute_done\n");

    if (! _program_cb.is_empty()) {
	_program_cb->dispatch(success, _command_stdout, _command_stderr,
			      task().do_exec());
    }

    task().item_done(success, fatal, _command_stderr);
}


// ----------------------------------------------------------------------------
// Task implementation

Task::Task(const string& name, TaskManager& taskmgr)
    : _name(name),
      _taskmgr(taskmgr),
      _start_module(false),
      _stop_module(false),
      _startup_validation(NULL),
      _config_validation(NULL),
      _ready_validation(NULL),
      _shutdown_validation(NULL),
      _startup_method(NULL),
      _shutdown_method(NULL),
      _config_done(false),
      _exec_id(taskmgr.exec_id()),
      _verbose(taskmgr.verbose())
{
}

Task::~Task()
{
    if (_startup_validation != NULL)
	delete _startup_validation;
    if (_config_validation != NULL)
	delete _config_validation;
    if (_ready_validation != NULL)
	delete _ready_validation;
    if (_shutdown_validation != NULL)
	delete _shutdown_validation;
    if (_startup_method != NULL)
	delete _startup_method;
    if (_shutdown_method != NULL)
	delete _shutdown_method;

    delete_pointers_list(_task_items);
}

void
Task::start_module(const string& module_name,
		   Validation* startup_validation,
		   Validation* config_validation,
		   Startup* startup)
{
    XLOG_ASSERT(! module_name.empty());
    XLOG_ASSERT(_start_module == false);
    XLOG_ASSERT(_stop_module == false);
    XLOG_ASSERT(_startup_validation == NULL);
    XLOG_ASSERT(_config_validation == NULL);
    XLOG_ASSERT(_startup_method == NULL);

    _start_module = true;
    _module_name = module_name;
    _startup_validation = startup_validation;
    _config_validation = config_validation;
    _startup_method = startup;
}

void
Task::shutdown_module(const string& module_name, Validation* validation,
		      Shutdown* shutdown)
{
    XLOG_ASSERT(! module_name.empty());
    XLOG_ASSERT(_start_module == false);
    XLOG_ASSERT(_stop_module == false);
    XLOG_ASSERT(_shutdown_validation == NULL);
    XLOG_ASSERT(_shutdown_method == NULL);

    _stop_module = true;
    _module_name = module_name;
    _shutdown_validation = validation;
    _shutdown_method = shutdown;
}

void
Task::add_xrl(const UnexpandedXrl& xrl, XrlRouter::XrlCallback& cb)
{
    _task_items.push_back(new TaskXrlItem(xrl, cb, *this));
}

void
Task::add_program(const UnexpandedProgram&		program,
		  TaskProgramItem::ProgramCallback	program_cb)
{
    _task_items.push_back(new TaskProgramItem(program, program_cb, *this));
}

void
Task::set_ready_validation(Validation* validation)
{
    _ready_validation = validation;
}

void
Task::run(CallBack cb)
{
    debug_msg("Task::run (%s)\n", _module_name.c_str());

    _task_complete_cb = cb;
    step1_start();
}

void
Task::step1_start()
{
    debug_msg("step1 (%s)\n", _module_name.c_str());

    if (_start_module) {
	_taskmgr.module_manager().start_module(_module_name, do_exec(),
					       is_verification(),
					       callback(this, &Task::step1_done));
    } else {
	step2_wait();
    }
}

void
Task::step1_done(bool success)
{
    debug_msg("step1_done (%s)\n", _module_name.c_str());

    if (success)
	step2_wait();
    else
	task_fail("Can't start process " + _module_name, false);
}

void
Task::step2_wait()
{
    debug_msg("step2 (%s)\n", _module_name.c_str());

    if (_start_module && (_startup_validation != NULL)) {
	_startup_validation->validate(_exec_id,
				      callback(this, &Task::step2_done));
    } else {
	step2_2_wait();
    }
}

void
Task::step2_done(bool success)
{
    debug_msg("step2_done (%s)\n", _module_name.c_str());

    if (success)
	step2_2_wait();
    else
	task_fail("Can't validate start of process " + _module_name, true);
}

void
Task::step2_2_wait()
{
    debug_msg("step2_2 (%s)\n", _module_name.c_str());

    if (_start_module && (_startup_method != NULL)) {
	_startup_method->startup(_exec_id,
				 callback(this, &Task::step2_2_done));
    } else {
	step2_3_wait();
    }
}

void
Task::step2_2_done(bool success)
{
    debug_msg("step2_2_done (%s)\n", _module_name.c_str());

    if (success)
	step2_3_wait();
    else
	task_fail("Can't startup process " + _module_name, true);
}

void
Task::step2_3_wait()
{
    debug_msg("step2_3 (%s)\n", _module_name.c_str());

    if (_start_module && (_config_validation != NULL)) {
	_config_validation->validate(_exec_id,
				     callback(this, &Task::step2_3_done));
    } else {
	step3_config();
    }
}

void
Task::step2_3_done(bool success)
{
    debug_msg("step2_3_done (%s)\n", _module_name.c_str());

    if (success)
	step3_config();
    else
	task_fail("Can't validate config ready of process " + _module_name,
		  true);
}

void
Task::step3_config()
{
    debug_msg("step3 (%s)\n", _module_name.c_str());

    if (_task_items.empty()) {
	step4_wait();
    } else {
	if (_stop_module) {
	    //
	    // We don't call any task items on a module if we are going to
	    // shut it down immediately afterwards, but we do need to
	    // unschedule the task items.
	    //
	    while (! _task_items.empty()) {
		TaskBaseItem* task_base_item = _task_items.front();
		task_base_item->unschedule();
		delete task_base_item;
		_task_items.pop_front();
	    }
	    // Skip step4 and go directly to stopping the process
	    step5_stop();
	} else {
	    string errmsg;
	    debug_msg("step3: execute\n");

	    if (_task_items.front()->execute(errmsg) == false) {
		XLOG_WARNING("Failed to execute task item: %s",
			     errmsg.c_str());
		task_fail(errmsg, false);
		return;
	    }
	}
    }
}

void
Task::item_done(bool success, bool fatal, string errmsg)
{
    debug_msg("item_done (%s)\n", _module_name.c_str());

    if (success) {
	TaskBaseItem* task_base_item = _task_items.front();
	_task_items.pop_front();
	delete task_base_item;
	_config_done = true;
	step3_config();
    } else {
	task_fail(errmsg, fatal);
    }
}

void
Task::step4_wait()
{
    debug_msg("step4 (%s)\n", _module_name.c_str());

    if (_ready_validation && _config_done) {
	_ready_validation->validate(_exec_id,
				    callback(this, &Task::step4_done));
    } else {
	step5_stop();
    }
}

void
Task::step4_done(bool success)
{
    debug_msg("step4_done (%s)\n", _module_name.c_str());

    if (success) {
	step5_stop();
    } else {
	task_fail("Reconfig of process " + _module_name +
		  " caused process to fail.", true);
    }
}

void
Task::step5_stop()
{
    debug_msg("step5 (%s)\n", _module_name.c_str());

    if (_stop_module) {
	if (_shutdown_method != NULL) {
	    _shutdown_method->shutdown(_exec_id,
				       callback(this, &Task::step5_done));
	} else {
	    step6_wait();
	}
    } else {
	step8_report();
    }
}

void
Task::step5_done(bool success)
{
    debug_msg("step5_done (%s)\n", _module_name.c_str());

    if (success) {
	step6_wait();
    } else {
	XLOG_WARNING("Can't subtly stop process %s", _module_name.c_str());
	_taskmgr.module_manager().kill_module(_module_name,
					callback(this, &Task::step6_wait));
    }
}

void
Task::step6_wait()
{
    debug_msg("step6 (%s)\n", _module_name.c_str());

    if (_stop_module && (_shutdown_validation != NULL)) {
	_shutdown_validation->validate(_exec_id,
				       callback(this, &Task::step6_done));
    } else {
	step8_report();
    }
}

void
Task::step6_done(bool success)
{
    debug_msg("step6_done (%s)\n", _module_name.c_str());

    if (success) {
	step7_wait();
    } else {
	string msg = "Can't validate stop of process " + _module_name;
	XLOG_WARNING("%s", msg.c_str());
	// An error here isn't fatal - module manager will simply kill
	// the process less subtly.
	step7_kill();
    }
}

void
Task::step7_wait()
{
    _wait_timer = _taskmgr.eventloop().new_oneoff_after_ms(1000,
					callback(this, &Task::step7_kill));
}

void
Task::step7_kill()
{
    _taskmgr.module_manager().kill_module(_module_name,
					  callback(this, &Task::step8_report));
}

void
Task::step8_report()
{
    debug_msg("step8 (%s)\n", _module_name.c_str());

    debug_msg("Task done\n");

    _task_complete_cb->dispatch(true, "");
}

void
Task::task_fail(string errmsg, bool fatal)
{
    debug_msg("%s\n", errmsg.c_str());

    if (fatal && !_module_name.empty()) {
	XLOG_ERROR("Shutting down fatally wounded process (%s)",
		   _module_name.c_str());
	_taskmgr.kill_process(_module_name);
    }
    _task_complete_cb->dispatch(false, errmsg);
}

bool
Task::do_exec() const
{
    return _taskmgr.do_exec();
}

bool
Task::is_verification() const
{
    return _taskmgr.is_verification();
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

TaskManager::TaskManager(MasterConfigTree& config_tree, ModuleManager& mmgr,
			 XorpClient& xclient, bool global_do_exec,
			 bool verbose)
    : _config_tree(config_tree),
      _module_manager(mmgr),
      _xorp_client(xclient),
      _global_do_exec(global_do_exec),
      _is_verification(false),
      _verbose(verbose)
{
}

TaskManager::~TaskManager()
{
    reset();
}

void
TaskManager::set_do_exec(bool do_exec, bool is_verification)
{
    _current_do_exec = do_exec && _global_do_exec;
    _is_verification = is_verification;
}

void
TaskManager::reset()
{
    while (! _tasks.empty()) {
	delete _tasks.begin()->second;
	_tasks.erase(_tasks.begin());
    }
    _shutdown_order.clear();
    _tasklist.clear();
    _exec_id.reset();
}

int
TaskManager::add_module(const ModuleCommand& module_command, string& error_msg)
{
    string module_name = module_command.module_name();
    string module_exec_path = module_command.module_exec_path();

    if (_tasks.find(module_name) == _tasks.end()) {
	Task* newtask = new Task(module_name, *this);
	_tasks[module_name] = newtask;
	_tasklist.push_back(newtask);
    }

    if (_module_manager.module_exists(module_name)) {
	if (_module_manager.module_has_started(module_name)) {
	    return XORP_OK;
	}
    } else {
	if (_module_manager.new_module(module_name, module_exec_path,
				       error_msg)
	    != true) {
	    error_msg = c_format("Cannot create module %s: %s",
				 module_name.c_str(), error_msg.c_str());
	    fail_tasklist_initialization(error_msg);
	    return XORP_ERROR;
	}
    }

    Validation* startup_validation = module_command.startup_validation(*this);
    Validation* config_validation = module_command.config_validation(*this);
    Startup* startup_method = module_command.startup_method(*this);
    find_task(module_name).start_module(module_name, startup_validation,
					config_validation, startup_method);

    _module_commands[module_name] = &module_command;
    return XORP_OK;
}

void
TaskManager::add_xrl(const string& module_name, const UnexpandedXrl& xrl,
		     XrlRouter::XrlCallback& cb)
{
    Task& t(find_task(module_name));
    t.add_xrl(xrl, cb);

    if (t.ready_validation() != NULL)
	return;

    XLOG_ASSERT(_module_commands.find(module_name) != _module_commands.end());
    t.set_ready_validation(_module_commands[module_name]->ready_validation(*this));
}

void
TaskManager::add_program(const string&			module_name,
			 const UnexpandedProgram&	program,
			 TaskProgramItem::ProgramCallback program_cb)
{
    Task& t(find_task(module_name));
    t.add_program(program, program_cb);

    if (t.ready_validation() != NULL)
	return;

    XLOG_ASSERT(_module_commands.find(module_name) != _module_commands.end());
    t.set_ready_validation(_module_commands[module_name]->ready_validation(*this));
}

void
TaskManager::shutdown_module(const string& module_name)
{
    debug_msg("shutdown_module: %s\n", module_name.c_str());

    Task& t(find_task(module_name));

    map<string, const ModuleCommand*>::iterator iter;
    iter = _module_commands.find(module_name);
    XLOG_ASSERT(iter != _module_commands.end());
    const ModuleCommand* mc = iter->second;
    t.shutdown_module(module_name, mc->shutdown_validation(*this),
		      mc->shutdown_method(*this));
    _shutdown_order.push_front(&t);
}

void
TaskManager::run(CallBack cb)
{
    list<Task*>::iterator iter;

    debug_msg("TaskManager::run, tasks (old order): ");

    if (_verbose) {
	string debug_output;
	for (iter = _tasklist.begin(); iter != _tasklist.end(); ++iter) {
	    debug_output += c_format("%s ", (*iter)->name().c_str());
	}
	debug_output += "\n";
	debug_msg("%s", debug_output.c_str());
    }

    reorder_tasks();

    //
    // Set the execution ID of the tasks
    //
    for (iter = _tasklist.begin(); iter != _tasklist.end(); ++iter) {
	Task* task = *iter;
	task->set_exec_id(exec_id());
    }

    debug_msg("TaskManager::run, tasks: ");
    if (_verbose) {
	string debug_output;
	for (iter = _tasklist.begin(); iter != _tasklist.end(); ++iter) {
	    debug_output += c_format("%s ", (*iter)->name().c_str());
	}
	debug_output += "\n";
	debug_msg("%s", debug_output.c_str());
    }

    _completion_cb = cb;
    run_task();
}

void
TaskManager::reorder_tasks()
{
    // We re-order the task list so that process shutdowns (which are
    // irreversable) occur last.
    list<Task*> configs;
    while (! _tasklist.empty()) {
	Task* t = _tasklist.front();
	if (t->will_shutdown_module()) {
	    // We already have a list of the correct order to shutdown
	    // modules in _shutdown_order
	} else {
	    configs.push_back(t);
	}
	_tasklist.pop_front();
    }

    // Re-build the tasklist
    while (! configs.empty()) {
	_tasklist.push_back(configs.front());
	configs.pop_front();
    }

    list<Task*>::const_iterator iter;
    for (iter = _shutdown_order.begin();
	 iter != _shutdown_order.end();
	 ++iter) {
	_tasklist.push_back(*iter);
    }
}

void
TaskManager::run_task()
{
    debug_msg("TaskManager::run_task()\n");

    if (_tasklist.empty()) {
	if (! is_verification())
	    XLOG_INFO("No more tasks to run\n");
	_completion_cb->dispatch(true, "");
	return;
    }
    _tasklist.front()->run(callback(this, &TaskManager::task_done));
}

void
TaskManager::task_done(bool success, string errmsg)
{
    debug_msg("TaskManager::task_done\n");

    if (! success) {
	debug_msg("task failed\n");
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
    XLOG_ERROR("%s", errmsg.c_str());
    reset();
    return;
}

Task&
TaskManager::find_task(const string& module_name)
{
    map<string, Task*>::iterator iter;
    iter = _tasks.find(module_name);
    if (iter == _tasks.end()) {
	// The task didn't exist, so we create one.  This is only valid
	// if we've already started the module.
	XLOG_ASSERT(_module_commands.find(module_name)
		    != _module_commands.end());
	const ModuleCommand* module_command = _module_commands[module_name];
	string module_name = module_command->module_name();
	string module_exec_path = module_command->module_exec_path();
	if (_tasks.find(module_name) == _tasks.end()) {
	    Task* newtask = new Task(module_name, *this);
	    _tasks[module_name] = newtask;
	    _tasklist.push_back(newtask);
	}
	iter = _tasks.find(module_name);
    }
    XLOG_ASSERT(iter != _tasks.end());
    XLOG_ASSERT(iter->second != NULL);
    return *(iter->second);
}

void
TaskManager::kill_process(const string& module_name)
{
    // XXX We really should try to restart the failed process, but for
    // now we'll just kill it.
    _module_manager.kill_module(module_name,
				callback(this, &TaskManager::null_callback));
}

EventLoop&
TaskManager::eventloop() const
{
    return _module_manager.eventloop();
}

void
TaskManager::null_callback()
{

}
