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

// $XORP: xorp/devnotes/template.hh,v 1.2 2003/01/16 19:08:48 mjh Exp $

#ifndef __RTRMGR_TASK_HH__
#define __RTRMGR_TASK_HH__

#include "rtrmgr_module.h"
#include "libxorp/xorp.h"
#include "libxipc/xrl_router.hh"
#include "unexpanded_xrl.hh"
#include "xorp_client.hh"

class Task;

class Validation {
public:
    typedef XorpCallback1<void, bool>::RefPtr CallBack;

    Validation();
    virtual ~Validation() {};
    virtual void validate(CallBack cb) = 0;
private:
    CallBack _cb;
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
    typedef XorpCallback1<void, bool>::RefPtr CallBack;

    Task(const string& name, XorpClient& xorp_client, bool do_exec);
    ~Task();
    void add_start_module(ModuleManager& mmgr, const string& mod_name,
			  Validation* validation);
    void add_stop_module(ModuleManager& mmgr, const string& mod_name,
			 Validation* validation);
    void add_xrl(const UnexpandedXrl& xrl, XrlRouter::XrlCallback& cb);
    void run(CallBack cb);
    void xrl_done(bool success, string errmsg); 
    bool do_exec() const {return _do_exec;}
    XorpClient& xorp_client() const {return _xclient;}
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
    XorpClient& _xclient;
    bool _do_exec;
    string _modname; //the name of the module to start and stop
    bool _start_module;
    bool _stop_module;
    Validation* _validation; // the validation mechanism for the module 
                             // start or module stop
    list <TaskXrlItem> _xrls;
    ModuleManager* _mmgr;
    CallBack _task_complete_cb; //the task completion callback
};

#endif // __RTRMGR_TASK_HH__
