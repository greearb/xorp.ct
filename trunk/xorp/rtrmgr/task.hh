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

// $XORP: xorp/rtrmgr/task.hh,v 1.12 2003/05/30 04:42:09 mjh Exp $

#ifndef __RTRMGR_TASK_HH__
#define __RTRMGR_TASK_HH__

#include <map>
#include "rtrmgr_module.h"
#include "libxorp/xorp.h"
#include "libxorp/status_codes.h"
#include "libxipc/xrl_router.hh"
#include "unexpanded_xrl.hh"

class Task;
class TaskManager;
class XorpClient;
class ModuleManager;
class TaskManager;
class ModuleCommand;

class Validation {
public:
    typedef XorpCallback1<void, bool>::RefPtr CallBack;

    Validation() {};
    virtual ~Validation() {};
    virtual void validate(CallBack cb) = 0;
protected:
};

class DelayValidation : public Validation {
public:
    DelayValidation(EventLoop& eventloop, uint32_t ms);
    void validate(CallBack cb);
private:
    void timer_expired();
    EventLoop& _eventloop;
    CallBack _cb;
    uint32_t _delay_in_ms;
    XorpTimer _timer;
};

class XrlStatusValidation : public Validation {
public:
    XrlStatusValidation(const string& target, 
			TaskManager& taskmgr);
    virtual ~XrlStatusValidation() {}
    void validate(CallBack cb);
protected:
    void dummy_response();
    virtual void xrl_done(const XrlError& e, XrlArgs* xrlargs);
    virtual void handle_status_response(ProcessStatus status,
					const string& reason) = 0;
    EventLoop& eventloop();

    string _target;
    TaskManager& _task_manager;
    CallBack _cb;
    XorpTimer _retry_timer;
    uint32_t _retries;
};

class StatusReadyValidation : public XrlStatusValidation {
public:
    StatusReadyValidation(const string& target, 
			  TaskManager& taskmgr);
private:
    void handle_status_response(ProcessStatus status,
				const string& reason);
};

class StatusConfigMeValidation : public XrlStatusValidation {
public:
    StatusConfigMeValidation(const string& target, 
			     TaskManager& taskmgr);
private:
    void handle_status_response(ProcessStatus status,
				const string& reason);
};

class TaskXrlItem {
public:
    TaskXrlItem(const UnexpandedXrl& uxrl,
		const XrlRouter::XrlCallback& cb, Task& task);
    TaskXrlItem::TaskXrlItem(const TaskXrlItem& them);
    bool execute(string& errmsg);
    void execute_done(const XrlError& err, XrlArgs* xrlargs);
    void resend();
private:
    UnexpandedXrl _unexpanded_xrl;
    XrlRouter::XrlCallback _xrl_callback;
    Task& _task;
    XorpTimer _resend_timer;
    uint32_t _resend_counter;
};

class Task {
public:
    typedef XorpCallback2<void, bool, string>::RefPtr CallBack;

    Task(const string& name, TaskManager& taskmgr);
    ~Task();
    void add_start_module(const string& mod_name,
			  Validation* validation);
    void add_stop_module(const string& mod_name,
			 Validation* validation);
    void add_xrl(const UnexpandedXrl& xrl, XrlRouter::XrlCallback& cb);
    void set_ready_validation(Validation* validation);
    Validation* ready_validation() const {
	return _ready_validation;
    }
    void run(CallBack cb);
    void xrl_done(bool success, bool fatal, string errmsg); 
    bool do_exec() const;
    XorpClient& xorp_client() const;

    const string& name() const {return _name;}
    EventLoop& eventloop() const;
protected:
    void step1_start();
    void step1_done(bool success);

    void step2_wait();
    void step2_done(bool success);

    void step3_config();
    void step3_done(bool success);

    void step4_wait();
    void step4_done(bool success);

    void step5_stop();
    void step5_done();

    void step6_wait();
    void step6_done(bool success);

    void step7_report();
    void task_fail(string errmsg, bool fatal);
private:
    string _name; //the name of the task
    TaskManager& _taskmgr;
    string _modname; //the name of the module to start and stop
    bool _start_module;
    bool _stop_module;
    Validation* _start_validation; // the validation mechanism for the module 
                                   // start
    Validation* _ready_validation; // the validation mechanism for the module 
                                   // reconfiguration
    Validation* _stop_validation;  // the validation mechanism for the module 
                                   // stop
    list <TaskXrlItem> _xrls;
    bool _config_done; //true if we changed the module's config
    CallBack _task_complete_cb; //the task completion callback
};

class TaskManager {
    typedef XorpCallback2<void, bool, string>::RefPtr CallBack;
public:
    TaskManager::TaskManager(ModuleManager &mmgr, XorpClient &xclient, 
			     bool global_do_exec);
    void set_do_exec(bool do_exec);
    void reset();
    int add_module(const ModuleCommand& mod_cmd);
    void add_xrl(const string& modname, const UnexpandedXrl& xrl, 
		 XrlRouter::XrlCallback& cb);
    void run(CallBack cb);
    XorpClient& xorp_client() const {return _xorp_client;}
    ModuleManager& module_manager() const {return _module_manager;}
    bool do_exec() const {return _current_do_exec;}
    EventLoop& eventloop() const;

    /**
     * @short kill_process is used to kill a fatally wounded process
     *
     * kill_process is used to kill a fatally wounded process. This
     * does not politely ask the process to die, because if we get
     * here we can't communicate with the process using XRLs, so we
     * just kill it outright.
     * 
     * @param modname the module name of the process to be killed.  
     */
    void kill_process(const string& modname);
private:
    void run_task();
    void task_done(bool success, string errmsg);
    void fail_tasklist_initialization(const string& errmsg);
    Task& find_task(const string& modname);

    ModuleManager& _module_manager;
    XorpClient& _xorp_client;
    bool _global_do_exec; //false if we're never going to execute
                          //anything because we're in a debug mode
    bool _current_do_exec;

    //_tasks provides fast access to a Task by name.
    map <string, Task*> _tasks;

    //_tasklist maintains the execution order
    list <Task*> _tasklist;

    map <string, const ModuleCommand*> _module_commands;

    CallBack _completion_cb;
};

#endif // __RTRMGR_TASK_HH__
