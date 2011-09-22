// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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


#ifndef __RTRMGR_TASK_HH__
#define __RTRMGR_TASK_HH__




#include "libxorp/bug_catcher.hh"
#include "libxorp/run_command.hh"
#include "libxorp/status_codes.h"

#include "libxipc/xrl_router.hh"

#include "unexpanded_program.hh"
#include "unexpanded_xrl.hh"


class MasterConfigTree;
class ModuleCommand;
class ModuleManager;
class Task;
class TaskManager;
class XorpClient;

class Validation {
public:
    typedef XorpCallback1<void, bool>::RefPtr CallBack;

    Validation(const string& module_name, bool verbose)
	: _module_name(module_name), _verbose(verbose) {};
    virtual ~Validation() {};

    virtual void validate(RunShellCommand::ExecId exec_id, CallBack cb) = 0;

protected:
    const string	_module_name;
    RunShellCommand::ExecId _exec_id;
    bool		_verbose;	 // Set to true if output is verbose
};

class DelayValidation : public Validation {
public:
    DelayValidation(const string& module_name, EventLoop& eventloop,
		    uint32_t ms, bool verbose);

    void validate(RunShellCommand::ExecId exec_id, CallBack cb);

private:
    void timer_expired();

    EventLoop&	_eventloop;
    CallBack	_cb;
    uint32_t	_delay_in_ms;
    XorpTimer	_timer;
};

class XrlStatusValidation : public Validation {
public:
    XrlStatusValidation(const string& module_name, const XrlAction& xrl_action,
			TaskManager& taskmgr);
    virtual ~XrlStatusValidation() {
	eventloop().remove_timer(_retry_timer);
	// TODO:  Should remove callbacks pointing to this as well,
	// but that is a whole other problem...
    }

    void validate(RunShellCommand::ExecId exec_id, CallBack cb);

protected:
    void dummy_response();
    virtual void xrl_done(const XrlError& e, XrlArgs* xrl_args);
    virtual void handle_status_response(ProcessStatus status,
					const string& reason) = 0;
    EventLoop& eventloop();

    const XrlAction&	_xrl_action;
    TaskManager&	_task_manager;
    CallBack		_cb;
    XorpTimer		_retry_timer;
    uint32_t		_retries;
};

class ProgramStatusValidation : public Validation {
public:
    ProgramStatusValidation(const string& module_name,
			    const ProgramAction& program_action,
			    TaskManager& taskmgr);
    virtual ~ProgramStatusValidation();

    void validate(RunShellCommand::ExecId exec_id, CallBack cb);

protected:
    virtual void handle_status_response(bool success,
					const string& stdout_output,
					const string& stderr_output) = 0;
    EventLoop& eventloop();

    const ProgramAction& _program_action;
    TaskManager&	_task_manager;
    CallBack		_cb;

private:
    void stdout_cb(RunShellCommand* run_command, const string& output);
    void stderr_cb(RunShellCommand* run_command, const string& output);
    void done_cb(RunShellCommand* run_command, bool success,
		 const string& error_msg);
    void execute_done(bool success);

    RunShellCommand*	_run_command;
    string		_command_stdout;
    string		_command_stderr;
    XorpTimer		_delay_timer;
};

class XrlStatusStartupValidation : public XrlStatusValidation {
public:
    XrlStatusStartupValidation(const string& module_name,
			       const XrlAction& xrl_action,
			       TaskManager& taskmgr);

private:
    void handle_status_response(ProcessStatus status, const string& reason);
};

class ProgramStatusStartupValidation : public ProgramStatusValidation {
public:
    ProgramStatusStartupValidation(const string& module_name,
				   const ProgramAction& program_action,
				   TaskManager& taskmgr);

private:
    void handle_status_response(bool success,
				const string& stdout_output,
				const string& stderr_output);
};

class XrlStatusReadyValidation : public XrlStatusValidation {
public:
    XrlStatusReadyValidation(const string& module_name,
			     const XrlAction& xrl_action,
			     TaskManager& taskmgr);

private:
    void handle_status_response(ProcessStatus status, const string& reason);
};

class ProgramStatusReadyValidation : public ProgramStatusValidation {
public:
    ProgramStatusReadyValidation(const string& module_name,
				 const ProgramAction& program_action,
				 TaskManager& taskmgr);

private:
    void handle_status_response(bool success,
				const string& stdout_output,
				const string& stderr_output);
};

class XrlStatusConfigMeValidation : public XrlStatusValidation {
public:
    XrlStatusConfigMeValidation(const string& module_name,
				const XrlAction& xrl_action,
				TaskManager& taskmgr);

private:
    void handle_status_response(ProcessStatus status, const string& reason);
};

class ProgramStatusConfigMeValidation : public ProgramStatusValidation {
public:
    ProgramStatusConfigMeValidation(const string& module_name,
				    const ProgramAction& program_action,
				    TaskManager& taskmgr);

private:
    void handle_status_response(bool success,
				const string& stdout_output,
				const string& stderr_output);
};

class XrlStatusShutdownValidation : public XrlStatusValidation {
public:
    XrlStatusShutdownValidation(const string& module_name,
				const XrlAction& xrl_action,
				TaskManager& taskmgr);

private:
    void xrl_done(const XrlError& e, XrlArgs* xrl_args);
    void handle_status_response(ProcessStatus status, const string& reason);
};

class ProgramStatusShutdownValidation : public ProgramStatusValidation {
public:
    ProgramStatusShutdownValidation(const string& module_name,
				    const ProgramAction& program_action,
				    TaskManager& taskmgr);

private:
    void handle_status_response(bool success,
				const string& stdout_output,
				const string& stderr_output);
};

class Startup {
public:
    typedef XorpCallback1<void, bool>::RefPtr CallBack;
    Startup(const string& module_name, bool verbose);
    virtual ~Startup() {}

    virtual void startup(const RunShellCommand::ExecId& exec_id,
			 CallBack cb) = 0;

protected:
    const string _module_name;
    bool	_verbose;	 // Set to true if output is verbose
};

class XrlStartup : public Startup {
public:
    XrlStartup(const string& module_name, const XrlAction& xrl_action,
	       TaskManager& taskmgr);
    virtual ~XrlStartup() {}

    void startup(const RunShellCommand::ExecId& exec_id, CallBack cb);
    EventLoop& eventloop() const;

private:
    void dummy_response();
    void startup_done(const XrlError& err, XrlArgs* xrl_args);

    const XrlAction&	_xrl_action;
    TaskManager&	_task_manager;
    CallBack		_cb;
    XorpTimer		_dummy_timer;
};

class ProgramStartup : public Startup {
public:
    ProgramStartup(const string& module_name,
		   const ProgramAction& program_action,
		   TaskManager& taskmgr);
    virtual ~ProgramStartup();

    void startup(const RunShellCommand::ExecId& exec_id, CallBack cb);
    EventLoop& eventloop() const;

private:
    void stdout_cb(RunShellCommand* run_command, const string& output);
    void stderr_cb(RunShellCommand* run_command, const string& output);
    void done_cb(RunShellCommand* run_command, bool success,
		 const string& error_msg);
    void execute_done(bool success);

    const ProgramAction& _program_action;
    TaskManager&	_task_manager;
    CallBack		_cb;

    RunShellCommand*	_run_command;
    string		_command_stdout;
    string		_command_stderr;
    XorpTimer		_delay_timer;
};

class Shutdown {
public:
    typedef XorpCallback1<void, bool>::RefPtr CallBack;
    Shutdown(const string& module_name, bool verbose);
    virtual ~Shutdown() {}

    virtual void shutdown(const RunShellCommand::ExecId& exec_id,
			  CallBack cb) = 0;

protected:
    const string _module_name;
    bool	_verbose;	 // Set to true if output is verbose
};

class XrlShutdown : public Shutdown {
public:
    XrlShutdown(const string& module_name, const XrlAction& xrl_action,
		TaskManager& taskmgr);
    virtual ~XrlShutdown() {}

    void shutdown(const RunShellCommand::ExecId& exec_id, CallBack cb);
    EventLoop& eventloop() const;

private:
    void dummy_response();
    void shutdown_done(const XrlError& err, XrlArgs* xrl_args);

    const XrlAction&	_xrl_action;
    TaskManager&	_task_manager;
    CallBack		_cb;
    XorpTimer		_dummy_timer;
};

class ProgramShutdown : public Shutdown {
public:
    ProgramShutdown(const string& module_name,
		    const ProgramAction& program_action,
		    TaskManager& taskmgr);
    virtual ~ProgramShutdown();

    void shutdown(const RunShellCommand::ExecId& exec_id, CallBack cb);
    EventLoop& eventloop() const;

private:
    void stdout_cb(RunShellCommand* run_command, const string& output);
    void stderr_cb(RunShellCommand* run_command, const string& output);
    void done_cb(RunShellCommand* run_command, bool success,
		 const string& error_msg);
    void execute_done(bool success);

    const ProgramAction& _program_action;
    TaskManager&	_task_manager;
    CallBack		_cb;

    RunShellCommand*	_run_command;
    string		_command_stdout;
    string		_command_stderr;
    XorpTimer		_delay_timer;
};

class TaskBaseItem {
public:
    TaskBaseItem(Task& task) : _task(task) {}
    TaskBaseItem(const TaskBaseItem& them) : _task(them._task) {}
    virtual ~TaskBaseItem() {}

    virtual bool execute(string& errmsg) = 0;
    virtual void unschedule() = 0;

    Task& task() { return (_task); }

private:
    Task&	_task;
};

class TaskXrlItem : public TaskBaseItem {
public:
    TaskXrlItem(const UnexpandedXrl& uxrl, const XrlRouter::XrlCallback& cb,
		Task& task,
		uint32_t xrl_resend_count = TaskXrlItem::DEFAULT_RESEND_COUNT,
		int xrl_resend_delay_ms = TaskXrlItem::DEFAULT_RESEND_DELAY_MS);
    TaskXrlItem(const TaskXrlItem& them);

    bool execute(string& errmsg);
    void execute_done(const XrlError& err, XrlArgs* xrl_args);
    void resend();
    void unschedule();

private:
    static const uint32_t	DEFAULT_RESEND_COUNT;
    static const int		DEFAULT_RESEND_DELAY_MS;

    UnexpandedXrl		_unexpanded_xrl;
    XrlRouter::XrlCallback	_xrl_callback;
    uint32_t			_xrl_resend_count_limit;
    uint32_t			_xrl_resend_count;
    int				_xrl_resend_delay_ms;
    XorpTimer			_xrl_resend_timer;
    bool			_verbose;   // Set to true if output is verbose
};

class TaskProgramItem : public TaskBaseItem {
public:
    typedef XorpCallback4<void, bool, const string&, const string&, bool>::RefPtr ProgramCallback;

    TaskProgramItem(const UnexpandedProgram&		program,
		    TaskProgramItem::ProgramCallback	program_cb,
		    Task&				task);
    TaskProgramItem(const TaskProgramItem& them);
    ~TaskProgramItem();

    bool execute(string& errmsg);
    void execute_done(bool success);
    void unschedule();

private:
    void stdout_cb(RunShellCommand* run_command, const string& output);
    void stderr_cb(RunShellCommand* run_command, const string& output);
    void done_cb(RunShellCommand* run_command, bool success,
		 const string& error_msg);

    UnexpandedProgram		_unexpanded_program;
    RunShellCommand*		_run_command;
    string			_command_stdout;
    string			_command_stderr;
    TaskProgramItem::ProgramCallback _program_cb;
    XorpTimer			_delay_timer;
    bool			_verbose;   // Set to true if output is verbose
};

class Task {
public:
    typedef XorpCallback2<void, bool, const string&>::RefPtr CallBack;

    Task(const string& name, TaskManager& taskmgr);
    ~Task();

    void start_module(const string& mod_name, Validation* startup_validation,
		      Validation* config_validation, Startup* startup);
    void shutdown_module(const string& mod_name, Validation* validation,
			 Shutdown* shutdown);
    void add_xrl(const UnexpandedXrl& xrl, XrlRouter::XrlCallback& cb);
    void add_program(const UnexpandedProgram&		program,
		     TaskProgramItem::ProgramCallback	program_cb);
    void set_ready_validation(Validation* validation);
    Validation* ready_validation() const { return _ready_validation; }
    bool will_shutdown_module() const { return _stop_module; }
    void run(CallBack cb);
    void item_done(bool success, bool fatal, const string& errmsg); 
    bool do_exec() const;
    bool is_verification() const;
    XorpClient& xorp_client() const;

    /**
     * Get a reference to the ExecId object.
     * 
     * @return a reference to the ExecId object that is used
     * for setting the execution ID when running the task.
     */
    const RunShellCommand::ExecId& exec_id() const { return _exec_id; }

    /**
     * Set the execution ID for executing the task.
     * 
     * @param v the execution ID.
     */
    void set_exec_id(const RunShellCommand::ExecId& v) { _exec_id = v; }

    const string& name() const { return _name; }
    EventLoop& eventloop() const;

    bool verbose() const { return _verbose; }

protected:
    void step1_start();
    void step1_done(bool success);

    void step2_wait();
    void step2_done(bool success);

    void step2_2_wait();
    void step2_2_done(bool success);

    void step2_3_wait();
    void step2_3_done(bool success);

    void step3_config();
    void step3_done(bool success);

    void step4_wait();
    void step4_done(bool success);

    void step5_stop();
    void step5_done(bool success);

    void step6_wait();
    void step6_done(bool success);

    void step7_wait();
    void step7_kill();

    void step8_report();
    void task_fail(const string& errmsg, bool fatal);

private:
    string	_name;		// The name of the task
    TaskManager& _taskmgr;
    string	_module_name;	// The name of the module to start and stop
    bool	_start_module;
    bool	_stop_module;
    Validation*	_startup_validation; // The validation mechanism for the
                                     // module startup
    Validation* _config_validation;  // The validation mechanism for the
				     // module configuration
    Validation*	_ready_validation; // The validation mechanism for the module 
                                   // reconfiguration
    Validation*	_shutdown_validation;  // The validation mechanism for the 
                                       // module shutdown
    Startup*	_startup_method;
    Shutdown*	_shutdown_method;
    list<TaskBaseItem *> _task_items;
    bool	_config_done;	// True if we changed the module's config
    CallBack	_task_complete_cb; // The task completion callback
    XorpTimer	_wait_timer;
    RunShellCommand::ExecId _exec_id;
    bool	_verbose;	 // Set to true if output is verbose
};

class TaskManager : public BugCatcher {
    typedef XorpCallback2<void, bool, string>::RefPtr CallBack;

public:
    TaskManager(MasterConfigTree& config_tree, 
		ModuleManager& mmgr,
		XorpClient& xclient, bool global_do_exec,
		bool verbose);
    ~TaskManager();

    void set_do_exec(bool do_exec, bool is_verification);
    void reset();
    int add_module(const ModuleCommand& mod_cmd, string& error_msg);
    void add_xrl(const string& module_name, const UnexpandedXrl& xrl, 
		 XrlRouter::XrlCallback& cb);
    void add_program(const string&			module_name,
		     const UnexpandedProgram&		program,
		     TaskProgramItem::ProgramCallback	program_cb);
    void shutdown_module(const string& module_name);
    void run(CallBack cb);
    XorpClient& xorp_client() const { return _xorp_client; }
    ModuleManager& module_manager() const { return _module_manager; }
    MasterConfigTree& config_tree() const { return _config_tree; }
    bool do_exec() const { return _current_do_exec; }
    bool is_verification() const { return _is_verification; }
    bool verbose() const { return _verbose; }
    EventLoop& eventloop() const;

    /**
     * @short kill_process is used to kill a fatally wounded process
     *
     * kill_process is used to kill a fatally wounded process. This
     * does not politely ask the process to die, because if we get
     * here we can't communicate with the process using XRLs or any other
     * mechanism, so we just kill it outright.
     * 
     * @param module_name the module name of the process to be killed.  
     */
    void kill_process(const string& module_name);

    /**
     * Get a reference to the ExecId object.
     * 
     * @return a reference to the ExecId object that is used
     * for setting the execution ID when running the tasks.
     */
    const RunShellCommand::ExecId& exec_id() const { return _exec_id; }

    /**
     * Set the execution ID for executing the tasks.
     * 
     * @param v the execution ID.
     */
    void set_exec_id(const RunShellCommand::ExecId& v) { _exec_id = v; }


private:
    void reorder_tasks();
    void run_task();
    void task_done(bool success, const string& errmsg);
    void fail_tasklist_initialization(const string& errmsg);
    Task& find_task(const string& module_name);
    void null_callback();

    MasterConfigTree&   _config_tree;
    ModuleManager&	_module_manager;
    XorpClient&		_xorp_client;
    bool		_global_do_exec; // Set to false if we're never going
					// to execute anything because we're
					// in a debug mode
    bool		_current_do_exec;
    bool		_is_verification; // Set to true if current execution
					  // is for verification purpose
    bool		_verbose;	// Set to true if output is verbose

    // _tasks provides fast access to a Task by name
    map<string, Task*> _tasks;

    // _tasklist maintains the execution order
    list<Task*> _tasklist;

    // _shutdown_order maintains the shutdown ordering
    list<Task*> _shutdown_order;

    map<string, const ModuleCommand*> _module_commands;

    RunShellCommand::ExecId _exec_id;

    CallBack _completion_cb;
};

#endif // __RTRMGR_TASK_HH__
