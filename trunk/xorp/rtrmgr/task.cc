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

#ident "$XORP: xorp/rtrmgr/task.cc,v 1.35 2004/05/18 00:05:12 pavlin Exp $"

#include "rtrmgr_module.h"
#include "libxorp/xlog.h"
#include "task.hh"
#include "conf_tree.hh"
#include "module_manager.hh"
#include "module_command.hh"
#include "xorp_client.hh"
#include "unexpanded_xrl.hh"

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
XrlStatusValidation::validate(CallBack cb)
{
    debug_msg("validate\n");

    _cb = cb;
    if (_task_manager.do_exec()) {
	string xrl_request, errmsg;
	Xrl* xrl = NULL;
	do {
	    // Try to expand using the configuration tree
	    const ConfigTreeNode* ctn;
	    ctn = _task_manager.config_tree().find_config_module(_module_name);
	    if (ctn != NULL) {
		if (_xrl_action.expand_xrl_variables(*ctn, xrl_request,
						     errmsg)
		    != XORP_OK) {
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
	    if (_xrl_action.expand_xrl_variables(ttn, xrl_request, errmsg)
		!= XORP_OK) {
		XLOG_FATAL("Cannot expand XRL validation action %s "
			   "for module %s: %s",
			   _xrl_action.str().c_str(),
			   _module_name.c_str(),
			   errmsg.c_str());
	    }
	    break;
	} while (false);
	if (xrl_request.empty()) {
	    XLOG_FATAL("Cannot expand XRL validation action %s for module %s",
		       _xrl_action.str().c_str(), _module_name.c_str());
	}

	// Create the XRL
	try {
	    xrl = new Xrl(xrl_request.c_str());
	} catch (const InvalidString& e) {
	    XLOG_ERROR("Invalid XRL validation action %s for module %s",
		       xrl_request.c_str(), _module_name.c_str());
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
    if (e == XrlError::OKAY()) {
	try {
	    ProcessStatus status;
	    status = static_cast<ProcessStatus>(xrl_args->get_uint32("status"));
	    string reason(xrl_args->get_string("reason"));
	    handle_status_response(status, reason);
	    return;
	} catch (XrlArgs::XrlAtomNotFound) {
	    // Not a valid response
	    XLOG_ERROR("Bad XRL response to get_status");
	    _cb->dispatch(false);
	    return;
	}
    } else if (e == XrlError::NO_FINDER()) {
	// We're in trouble now! This shouldn't be able to happen.
	XLOG_UNREACHABLE();
    } else if (e == XrlError::NO_SUCH_METHOD()) {
	// The template file must have been wrong - the target doesn't
	// support the common interface.
	XLOG_ERROR("Module %s doesn't support status validation",
		   _module_name.c_str());

	// Just return true and hope everything's OK
	_cb->dispatch(true);
    } else if ((e == XrlError::RESOLVE_FAILED())
	       || (e == XrlError::SEND_FAILED()))  {
	//
	// RESOLVE_FAILED => It's not yet registered with the finder -
	// retry after a short delay.
	//

	//
	// SEND_FAILED => ???  We're dealing with startup conditions
	// here, so give the problem a chance to resolve itself.
	//
	_retries++;
	if (_retries > MAX_STATUS_RETRIES) {
	    _cb->dispatch(false);
	}
	_retry_timer = eventloop().new_oneoff_after_ms(1000,
			callback(this, &XrlStatusValidation::validate, _cb));
    } else {
	XLOG_ERROR("Error while validating module %s", _module_name.c_str());
	XLOG_WARNING("Continuing anyway, cross your fingers...");
	_cb->dispatch(true);
    }
}


// ----------------------------------------------------------------------------
// StatusStartupValidation implementation

StatusStartupValidation::StatusStartupValidation(const string& module_name,
						 const XrlAction& xrl_action,
						 TaskManager& taskmgr)
    : XrlStatusValidation(module_name, xrl_action, taskmgr)
{
}

void
StatusStartupValidation::handle_status_response(ProcessStatus status,
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
// StatusReadyValidation implementation

StatusReadyValidation::StatusReadyValidation(const string& module_name,
					     const XrlAction& xrl_action,
					     TaskManager& taskmgr)
    : XrlStatusValidation(module_name, xrl_action, taskmgr)
{
}

void
StatusReadyValidation::handle_status_response(ProcessStatus status,
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
				 &XrlStatusValidation::validate, _cb));
	return;
    case PROC_READY:
	// The process is ready
	_cb->dispatch(true);
	return;
    }
    XLOG_UNREACHABLE();
}


// ----------------------------------------------------------------------------
// StatusConfigMeValidation implementation

StatusConfigMeValidation::StatusConfigMeValidation(const string& module_name,
						   const XrlAction& xrl_action,
						   TaskManager& taskmgr)
    : XrlStatusValidation(module_name, xrl_action, taskmgr)
{
}

void
StatusConfigMeValidation::handle_status_response(ProcessStatus status,
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
				 &XrlStatusValidation::validate, _cb));
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
// StatusShutdownValidation implementation

StatusShutdownValidation::StatusShutdownValidation(const string& module_name,
						   const XrlAction& xrl_action,
						   TaskManager& taskmgr)
    : XrlStatusValidation(module_name, xrl_action, taskmgr)
{
}

void
StatusShutdownValidation::handle_status_response(ProcessStatus status,
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
	// The process should be in SHUTDOWN state, or it should not
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
				 &XrlStatusValidation::validate, _cb));
	return;
    }
    XLOG_UNREACHABLE();
}

void
StatusShutdownValidation::xrl_done(const XrlError& e, XrlArgs* xrl_args)
{
    if (e == XrlError::OKAY()) {
	try {
	    ProcessStatus status;
	    status = static_cast<ProcessStatus>(xrl_args->get_uint32("status"));
	    string reason(xrl_args->get_string("reason"));
	    handle_status_response(status, reason);
	    return;
	} catch (XrlArgs::XrlAtomNotFound) {
	    // Not a valid response
	    XLOG_ERROR("Bad XRL response to get_status");
	    _cb->dispatch(false);
	    return;
	}
    } else if (e == XrlError::NO_FINDER()) {
	// We're in trouble now! This shouldn't be able to happen.
	XLOG_UNREACHABLE();
    } else if (e == XrlError::NO_SUCH_METHOD()) {
	// The template file must have been wrong - the target doesn't
	// support the common interface.
	XLOG_ERROR("Module %s doesn't support shutdown validation",
		   _module_name.c_str());
	// Return false, and we can shut it down using kill.
	_cb->dispatch(false);
    } else if (e == XrlError::RESOLVE_FAILED()) {
	// The process appears to have shutdown correctly.
	_cb->dispatch(true);
    } else if (e == XrlError::SEND_FAILED())  {
	// The process is probably gone.
	_cb->dispatch(true);
    } else {
	XLOG_ERROR("Error while shutdown validation of module %s",
		   _module_name.c_str());
	// Return false, and we can shut it down using kill.
	_cb->dispatch(false);
    }
}


// ----------------------------------------------------------------------------
// Startup implementation

Startup::Startup(const string& module_name, bool verbose)
    : _module_name(module_name, verbose)
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
XrlStartup::startup(CallBack cb)
{
    _cb = cb;
    if (_task_manager.do_exec()) {
	string xrl_request, errmsg;
	Xrl* xrl = NULL;
	do {
	    // Try to expand using the configuration tree
	    const ConfigTreeNode* ctn;
	    ctn = _task_manager.config_tree().find_config_module(_module_name);
	    if (ctn != NULL) {
		if (_xrl_action.expand_xrl_variables(*ctn, xrl_request,
						     errmsg)
		    != XORP_OK) {
		    XLOG_FATAL("Cannot expand XRL startup action %s "
			       "for module %s: %s",
			       _xrl_action.str().c_str(),
			       _module_name.c_str(),
			       errmsg.c_str());
		}
	    }

	    // Try to expand using the template tree
	    const TemplateTreeNode& ttn = _xrl_action.template_tree_node();
	    if (_xrl_action.expand_xrl_variables(ttn, xrl_request, errmsg)
		!= XORP_OK) {
		XLOG_FATAL("Cannot expand XRL startup action %s "
			   "for module %s: %s",
			   _xrl_action.str().c_str(),
			   _module_name.c_str(),
			   errmsg.c_str());
	    }
	    break;
	} while (false);
	if (xrl_request.empty()) {
	    XLOG_ERROR("Cannot expand XRL startup action %s for module %s",
		       _xrl_action.str().c_str(), _module_name.c_str());
	    return;
	}

	// Create the XRL
	try {
	    xrl = new Xrl(xrl_request.c_str());
	} catch (const InvalidString& e) {
	    XLOG_ERROR("Invalid XRL startup action %s for module %s",
		       xrl_request.c_str(), _module_name.c_str());
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
XrlShutdown::shutdown(CallBack cb)
{
    XLOG_INFO("Shutting down module: %s\n", _module_name.c_str());

    _cb = cb;
    if (_task_manager.do_exec()) {
	string xrl_request, errmsg;
	Xrl* xrl = NULL;
	do {
	    // Try to expand using the configuration tree
	    const ConfigTreeNode* ctn;
	    ctn = _task_manager.config_tree().find_config_module(_module_name);
	    if (ctn != NULL) {
		if (_xrl_action.expand_xrl_variables(*ctn, xrl_request,
						     errmsg)
		    != XORP_OK) {
		    XLOG_FATAL("Cannot expand XRL shutdown action %s "
			       "for module %s: %s",
			       _xrl_action.str().c_str(),
			       _module_name.c_str(),
			       errmsg.c_str());
		}
	    }

	    // Try to expand using the template tree
	    const TemplateTreeNode& ttn = _xrl_action.template_tree_node();
	    if (_xrl_action.expand_xrl_variables(ttn, xrl_request, errmsg)
		!= XORP_OK) {
		XLOG_FATAL("Cannot expand XRL shutdown action %s "
			   "for module %s: %s",
			   _xrl_action.str().c_str(),
			   _module_name.c_str(),
			   errmsg.c_str());
	    }
	    break;
	} while (false);
	if (xrl_request.empty()) {
	    XLOG_ERROR("Cannot expand XRL shutdown action %s for module %s",
		       _xrl_action.str().c_str(), _module_name.c_str());
	    return;
	}

	// Create the XRL
	try {
	    xrl = new Xrl(xrl_request.c_str());
	} catch (const InvalidString& e) {
	    XLOG_ERROR("Invalid XRL shutdown action %s for module %s",
		       xrl_request.c_str(), _module_name.c_str());
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
    UNUSED(xrl_args);

    if ((err == XrlError::OKAY())
	|| (err == XrlError::RESOLVE_FAILED())
	|| (err == XrlError::SEND_FAILED())) {
	// Success - either it said it would shutdown, or it's already gone.
	_cb->dispatch(true);
    } else {
	// We may have to kill it.
	_cb->dispatch(false);
    }
}


// ----------------------------------------------------------------------------
// TaskXrlItem implementation

TaskXrlItem::TaskXrlItem(const UnexpandedXrl& uxrl,
			 const XrlRouter::XrlCallback& cb, Task& task)
    : _unexpanded_xrl(uxrl),
      _xrl_callback(cb),
      _task(task),
      _resend_counter(0),
      _verbose(task.verbose())
{
}

TaskXrlItem::TaskXrlItem(const TaskXrlItem& them)
    : _unexpanded_xrl(them._unexpanded_xrl),
      _xrl_callback(them._xrl_callback),
      _task(them._task),
      _resend_counter(0),
      _verbose(them._verbose)
{
}

bool
TaskXrlItem::execute(string& errmsg)
{
    XLOG_TRACE(_verbose, "Expanding %s\n", _unexpanded_xrl.str().c_str());

    Xrl* xrl = _unexpanded_xrl.expand(errmsg);
    if (xrl == NULL) {
	errmsg = c_format("Failed to expand XRL %s: %s",
			  _unexpanded_xrl.str().c_str(), errmsg.c_str());
	return false;
    }
    XLOG_TRACE(_verbose, "Executing XRL: >%s<\n", xrl->str().c_str());

    string xrl_return_spec = _unexpanded_xrl.return_spec();

    // For debugging purposes, record what we were doing
    errmsg = _unexpanded_xrl.str();

    _resend_counter = 0;
    _task.xorp_client().send_now(*xrl,
				 callback(this, &TaskXrlItem::execute_done),
				 xrl_return_spec,
				 _task.do_exec());
    return true;
}

void
TaskXrlItem::unschedule()
{
    XrlArgs xrl_args;	    // Empty XRL args should be OK here.

    debug_msg("TaskXrlItem::unschedule()\n");

    // We need to dispatch the callbacks, or the accounting of which
    // actions were taken will be incorrect.
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

    _task.xorp_client().send_now(*xrl,
				 callback(this, &TaskXrlItem::execute_done),
				 xrl_return_spec,
				 _task.do_exec());
}

void
TaskXrlItem::execute_done(const XrlError& err, XrlArgs* xrl_args)
{
    bool fatal = false;

    debug_msg("TaskXrlItem::execute_done\n");

    if (err != XrlError::OKAY()) {
	// XXX: handle XRL errors here
	if ((err == XrlError::NO_FINDER())
	    || (err == XrlError::SEND_FAILED())
	    || (err == XrlError::NO_SUCH_METHOD())) {
	    //
	    // The error was a fatal one for the target - we now
	    // consider the target to be fatally wounded.
	    //
	    XLOG_ERROR("%s", err.str().c_str());
	    fatal = true;
	} else if ((err == XrlError::COMMAND_FAILED())
		   || (err == XrlError::BAD_ARGS())) {
	    //
	    // Non-fatal error for the target - typically this is
	    // because the user entered bad information - we can't
	    // change the target configuration to what the user
	    // specified, but this doesn't mean the target is fatally
	    // wounded.
	    //
	    fatal = false;
	} else if ((err == XrlError::REPLY_TIMED_OUT())
		   || (err == XrlError::RESOLVE_FAILED())) {
	    //
	    // REPLY_TIMED_OUT shouldn't happen on a reliable
	    // transport, but at this point we don't know for sure
	    // which Xrls are reliable and while are unreliable, so
	    // best handle this as if it were packet loss.
	    //
	    // RESOLVE_FAILED shouldn't happen if the startup
	    // validation has done it's job correctly, but just in case
	    // we'll be lenient and give it ten more seconds to be
	    // functioning before we declare it dead.
	    //
	    // TODO: get rid of the hard-coded value of "10" below.
	    if (_resend_counter > 10) {
		// Give up.
		// The error was a fatal one for the target - we now
		// consider the target to be fatally wounded.
		XLOG_ERROR("%s", err.str().c_str());
		fatal = true;
	    } else {
		// Re-send the Xrl after a short delay.
		_resend_counter++;
		// TODO: get rid of the hard-coded value of "1000" below.
		_resend_timer = _task.eventloop().new_oneoff_after_ms(1000,
					callback(this, &TaskXrlItem::resend));
		return;
	    }
	} else if (err == XrlError::INTERNAL_ERROR()) {
	    //
	    // Something bad happened but it's not clear what.  Don't
	    // consider these to be fatal errors, but they may well
	    // prove to be so.  XXX revisit this issue when we've more
	    // experience with XRL errors.
	    //
	    XLOG_ERROR("%s", err.str().c_str());
	    fatal = false;
	} else {
	    // We intended to explicitly handle all errors above, so if
	    // we got here there's an error type we didn't know about.
	    XLOG_UNREACHABLE();
	}
    }

    if (!_xrl_callback.is_empty())
	_xrl_callback->dispatch(err, xrl_args);

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
}

void
Task::start_module(const string& module_name,
		   Validation* startup_validation,
		   Validation* config_validation,
		   Startup* startup)
{
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
    debug_msg("Task::run %s\n", _module_name.c_str());

    _task_complete_cb = cb;
    step1_start();
}

void
Task::step1_start()
{
    debug_msg("step1 (%s)\n", _module_name.c_str());

    if (_start_module) {
	_taskmgr.module_manager().start_module(_module_name, do_exec(),
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
	_startup_validation->validate(callback(this, &Task::step2_done));
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
	_startup_method->startup(callback(this, &Task::step2_2_done));
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
	_config_validation->validate(callback(this, &Task::step2_3_done));
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

    if (_xrls.empty()) {
	step4_wait();
    } else {
	if (_stop_module) {
	    //
	    // We don't call any XRLs on a module if we are going to
	    // shut it down immediately afterwards, but we do need to
	    // unschedule the XRLs.
	    //
	    while (!_xrls.empty()) {
		_xrls.front().unschedule();
		_xrls.pop_front();
	    }
	    // Skip step4 and go directly to stopping the process
	    step5_stop();
	} else {
	    string errmsg;
	    debug_msg("step3: execute\n");

	    if (_xrls.front().execute(errmsg) == false) {
		XLOG_WARNING("Failed to execute XRL: %s", errmsg.c_str());
		task_fail(errmsg, false);
		return;
	    }
	}
    }
}

void
Task::xrl_done(bool success, bool fatal, string errmsg)
{
    debug_msg("xrl_done (%s)\n", _module_name.c_str());

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
    debug_msg("step4 (%s)\n", _module_name.c_str());

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
	    _shutdown_method->shutdown(callback(this, &Task::step5_done));
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
	XLOG_WARNING(("Can't subtly stop process " + _module_name).c_str());
	_taskmgr.module_manager().kill_module(_module_name,
					callback(this, &Task::step6_wait));
    }
}

void
Task::step6_wait()
{
    debug_msg("step6 (%s)\n", _module_name.c_str());

    if (_stop_module && (_shutdown_validation != NULL)) {
	_shutdown_validation->validate(callback(this, &Task::step6_done));
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

    if (fatal) {
	XLOG_ERROR("Shutting down fatally wounded process %s",
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

TaskManager::TaskManager(ConfigTree& config_tree, ModuleManager& mmgr,
			 XorpClient& xclient, bool global_do_exec,
			 bool verbose)
    : _config_tree(config_tree),
      _module_manager(mmgr),
      _xorp_client(xclient),
      _global_do_exec(global_do_exec),
      _verbose(verbose)
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
    _shutdown_order.clear();
    _tasklist.clear();
}

int
TaskManager::add_module(const ModuleCommand& module_command)
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
	if (!_module_manager.new_module(module_name, module_exec_path)) {
	    fail_tasklist_initialization("Can't create module");
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
    debug_msg("TaskManager::run, tasks (old order): ");

    if (_verbose) {
	string debug_output;
	list<Task*>::const_iterator iter;
	for (iter = _tasklist.begin(); iter != _tasklist.end(); ++iter) {
	    debug_output += c_format("%s ", (*iter)->name().c_str());
	}
	debug_output += "\n";
	debug_msg("%s", debug_output.c_str());
    }

    reorder_tasks();

    debug_msg("TaskManager::run, tasks: ");
    if (_verbose) {
	string debug_output;
	list<Task*>::const_iterator iter;
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
    while (!_tasklist.empty()) {
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
    while (!configs.empty()) {
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

    if (!success) {
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

int 
TaskManager::shell_execute(uid_t userid, const vector<string>& argv, 
			   TaskManager::CallBack cb)
{
    return _module_manager.shell_execute(userid, argv, cb, do_exec());
}
