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

// $XORP: xorp/rtrmgr/task.hh,v 1.7 2003/05/04 06:25:21 mjh Exp $

#ifndef __RTRMGR_TASK_HH__
#define __RTRMGR_TASK_HH__

#include <map>
#include "rtrmgr_module.h"
#include "libxorp/xorp.h"
#include "libxipc/xrl_router.hh"
#include "unexpanded_xrl.hh"

class Task;
class TaskManager;
class XorpClient;
class ModuleManager;

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


class TaskXrlItem {
public:
    TaskXrlItem(const UnexpandedXrl& uxrl,
		const XrlRouter::XrlCallback& cb, Task& task);
    TaskXrlItem::TaskXrlItem(const TaskXrlItem& them);
    bool execute(string& errmsg);
    void execute_done(const XrlError& err, XrlArgs* xrlargs);

private:
    UnexpandedXrl _unexpanded_xrl;
    XrlRouter::XrlCallback _xrl_callback;
    Task& _task;
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
    void run(CallBack cb);
    void xrl_done(bool success, string errmsg); 
    bool do_exec() const;
    XorpClient& xorp_client() const;

    const string& name() const {return _name;}
protected:
    void step1();
    void step1_done(bool success);

    void step2();
    void step2_done(bool success);

    void step3();
    void step3_done(bool success);

    void step4();
    void step4_done();

    void step5();
    void step5_done(bool success);

    void step6();
    void task_fail(string errmsg);
private:
    string _name; //the name of the task
    TaskManager& _taskmgr;
    string _modname; //the name of the module to start and stop
    bool _start_module;
    bool _stop_module;
    Validation* _validation; // the validation mechanism for the module 
                             // start or module stop
    list <TaskXrlItem> _xrls;
    CallBack _task_complete_cb; //the task completion callback
};

class TaskManager {
    typedef XorpCallback2<void, bool, string>::RefPtr CallBack;
public:
    TaskManager::TaskManager(ModuleManager &mmgr, XorpClient &xclient, 
			     bool global_do_exec);
    void set_do_exec(bool do_exec);
    void reset();
    int add_module(const string& modname, const string& modpath,
		   Validation *validation);
    void add_xrl(const string& modname, const UnexpandedXrl& xrl, 
		 XrlRouter::XrlCallback& cb);
    void run(CallBack cb);
    XorpClient& xorp_client() const {return _xorp_client;}
    ModuleManager& module_manager() const {return _module_manager;}
    bool do_exec() const {return _current_do_exec;}
    EventLoop& eventloop() const;
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

    CallBack _completion_cb;
};

#endif // __RTRMGR_TASK_HH__
