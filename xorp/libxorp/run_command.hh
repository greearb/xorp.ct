// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net


#ifndef __LIBXORP_RUN_COMMAND_HH__
#define __LIBXORP_RUN_COMMAND_HH__




#include "libxorp/asyncio.hh"
#include "libxorp/callback.hh"
#include "libxorp/timer.hh"

class EventLoop;


/**
 * @short Base virtual class for running an external command.
 */
class RunCommandBase {
public:
    class ExecId;

    /**
     * Constructor for a given command.
     *
     * @param eventloop the event loop.
     * @param command the command to execute.
     * @param real_command_name the real command name.
     * @param task_priority the priority to read stdout and stderr.
     */
    RunCommandBase(EventLoop&		eventloop,
		   const string&	command,
		   const string&	real_command_name,
		   int 	   		task_priority =
		   XorpTask::PRIORITY_DEFAULT);

    /**
     * Destructor.
     */
    virtual ~RunCommandBase();

    /**
     * Set the list with the arguments for the command to execute.
     * 
     * @param v the list with the arguments for the command to execute.
     */
    void set_argument_list(const list<string>& v) { _argument_list = v; }

    /**
     * Execute the command.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int execute();

    /**
     * Terminate the command.
     */
    void terminate();

    /**
     * Terminate the command with prejudice.
     */
    void terminate_with_prejudice();

    /**
     * Get the name of the command to execute.
     *
     * @return a string with the name of the command to execute.
     */
    const string& command() const { return _command; }

    /**
     * Get the list with the arguments of the command to execute.
     *
     * @return a string with the arguments of the command to execute.
     */
    const list<string>& argument_list() const { return _argument_list; }

    /**
     * Receive a notification that the status of the process has changed.
     *
     * @param wait_status the wait status.
     */
    void wait_status_changed(int wait_status);

    /**
     * Test if the command has exited.
     *
     * @return true if the command has exited, otherwise false.
     */
    bool is_exited() const { return _command_is_exited; }

    /**
     * Test if the command has been terminated by a signal.
     *
     * @return true if the command has been terminated by a signal,
     * otherwise false.
     */
    bool is_signal_terminated() const { return _command_is_signal_terminated; }

    /**
     * Test if the command has coredumped.
     *
     * @return true if the command has coredumped, otherwise false.
     */
    bool is_coredumped() const { return _command_is_coredumped; }

    /**
     * Test if the command has been stopped.
     *
     * @return true if the command has been stopped, otherwise false.
     */
    bool is_stopped() const { return _command_is_stopped; }

    /**
     * Get the command exit status.
     *
     * @return the command exit status.
     */
    int exit_status() const { return _command_exit_status; }

    /**
     * Get the signal that terminated the command.
     *
     * @return the signal that terminated the command.
     */
    int term_signal() const { return _command_term_signal; }

    /**
     * Get the signal that stopped the command.
     *
     * @return the signal that stopped the command.
     */
    int stop_signal() const { return _command_stop_signal; }

    /**
     * Get the priority to use when reading output from the command.
     *
     * @return the stdout and stderr priority.
     */
    int task_priority() const { return _task_priority; }

    /**
     * Get a reference to the ExecId object.
     * 
     * @return a reference to the ExecId object that is used
     * for setting the execution ID when running the command.
     */
    ExecId& exec_id() { return (_exec_id); }

    /**
     * Set the execution ID for executing the command.
     * 
     * @param v the execution ID.
     */
    void set_exec_id(const ExecId& v);

    /**
     * @short Class for setting the execution ID when running the command.
     */
    class ExecId {
    public:
	/**
	 * Default constructor.
	 */
	ExecId();

	/**
	 * Constructor for a given user ID.
	 * 
	 * @param uid the user ID.
	 */
	ExecId(uid_t uid);

	/**
	 * Constructor for a given user ID and group ID.
	 * 
	 * @param uid the user ID.
	 * @param gid the group ID.
	 */
	ExecId(uid_t uid, gid_t gid);

	/**
	 * Save the current execution ID.
	 */
	void save_current_exec_id();

	/**
	 * Restore the previously saved execution ID.
	 * 
	 * @param error_msg the error message (if error).
	 * @return XORP_OK on success, otherwise XORP_ERROR.
	 */
	int restore_saved_exec_id(string& error_msg) const;

	/**
	 * Set the effective execution ID.
	 * 
	 * @param error_msg the error message (if error).
	 * @return XORP_OK on success, otherwise XORP_ERROR.
	 */
	int set_effective_exec_id(string& error_msg);

	/**
	 * Test if the execution ID is set.
	 * 
	 * @return true if the execution ID is set, otherwise false.
	 */
	bool is_set() const;

	/**
	 * Get the user ID.
	 * 
	 * @return the user ID.
	 */
	uid_t	uid() const { return (_uid); }

	/**
	 * Get the group ID.
	 * 
	 * @return the group ID.
	 */
	gid_t	gid() const { return (_gid); }

	/**
	 * Set the user ID.
	 * 
	 * @param v the user ID.
	 */
	void	set_uid(uid_t v) { _uid = v; _is_uid_set = true; }

	/**
	 * Set the group ID.
	 * 
	 * @param v the group ID.
	 */
	void	set_gid(gid_t v) { _gid = v; _is_gid_set = true; }

	/**
	 * Test if the user ID was assigned.
	 * 
	 * @return true if the user ID was assigned, otherwise false.
	 */
	bool	is_uid_set() const { return (_is_uid_set); }

	/**
	 * Test if the group ID was assigned.
	 * 
	 * @return true if the group ID was assigned, otherwise false.
	 */
	bool	is_gid_set() const { return (_is_gid_set); }

	/**
	 * Reset the assigned user ID and group ID.
	 */
	void	reset();

    private:
	uid_t	saved_uid() const { return (_saved_uid); }
	uid_t	saved_gid() const { return (_saved_gid); }

	uid_t	_uid;
	gid_t	_gid;
	bool	_is_uid_set;
	bool	_is_gid_set;

	uid_t	_saved_uid;
	gid_t	_saved_gid;
	bool	_is_exec_id_saved;
    };

private:
    /**
     * A pure virtual method called when there is output on the program's
     * stdout.
     * 
     * @param output the string with the output.
     */
    virtual void stdout_cb_dispatch(const string& output) = 0;

    /**
     * A pure virtual method called when there is output on the program's
     * stderr.
     * 
     * @param output the string with the output.
     */
    virtual void stderr_cb_dispatch(const string& output) = 0;

    /**
     * A pure virtual method called when the program execution is completed.
     * 
     * @param success if true the program execution has succeeded, otherwise
     * it has failed.
     * @param error_msg if error, the string with the error message.
     */
    virtual void done_cb_dispatch(bool success, const string& error_msg) = 0;

    /**
     * A pure virtual method called when the program has been stopped.
     * 
     * @param stop_signal the signal used to stop the program.
     */
    virtual void stopped_cb_dispatch(int stop_signal) = 0;

    /**
     * Test if the stderr should be redirected to stdout.
     * 
     * @return true if the stderr should be redirected to stdout, otherwise
     * false.
     */
    virtual bool redirect_stderr_to_stdout() const = 0;

    /**
     * Cleanup state.
     */
    void cleanup();

    /**
     * Block child process signal(s) in current signal mask.
     */
    int block_child_signals();

    /**
     * Unblock child process signal(s) in current signal mask.
     */
    int unblock_child_signals();

    /**
     * Terminate the process.
     *
     * @param with_prejudice if true then terminate the process with
     * prejudice, otherwise attempt to kill it gracefully.
     */
    void terminate_process(bool with_prejudice);

    /**
     * Close the output for the command.
     */
    void close_output();

    /**
     * Close the stdout output for the command.
     */
    void close_stdout_output();

    /**
     * Close the stderr output for the command.
     */
    void close_stderr_output();

    /**
     * Set the command status.
     *
     * @param status the command status.
     */
    void set_command_status(int status);

    /**
     * Append data from the command.
     *
     * @param event the event from the command (@see AsyncFileOperator::Event).
     * @param buffer the buffer with the data.
     * @param buffer_bytes the maximum number of bytes in the buffer.
     * @param offset offset of the last byte read.
     */
    void append_data(AsyncFileOperator::Event event, const uint8_t* buffer,
		     size_t buffer_bytes, size_t offset);

    /**
     * The command's I/O operation has completed.
     *
     * @param event the last event from the command.
     * (@see AsyncFileOperator::Event).
     * @param error_code the error code if the @ref event indicates an error
     * (e.g., if it is not equal to AsyncFileOperator::END_OF_FILE), otherwise
     * its value is ignored.
     */
    void io_done(AsyncFileOperator::Event event, int error_code);

    /**
     * The command has completed.
     *
     * @param done_timer the timer associated with the event.
     */
    void done(XorpTimer& done_timer);

#ifdef HOST_OS_WINDOWS
    static const int WIN32_PROC_TIMEOUT_MS = 500;

    void win_proc_done_cb(XorpFd fd, IoEventType type);
    void win_proc_reaper_cb();
#endif

    static const size_t	BUF_SIZE = 8192;
    EventLoop&		_eventloop;

    string		_command;
    string		_real_command_name;
    list<string>	_argument_list;

    AsyncFileReader*	_stdout_file_reader;
    AsyncFileReader*	_stderr_file_reader;
    FILE*		_stdout_stream;
    FILE*		_stderr_stream;
    uint8_t		_stdout_buffer[BUF_SIZE];
    uint8_t		_stderr_buffer[BUF_SIZE];
    size_t		_last_stdout_offset;
    size_t		_last_stderr_offset;
    pid_t		_pid;
#ifdef HOST_OS_WINDOWS
    HANDLE		_ph;
    XorpTimer		_reaper_timer;
#endif
    bool		_is_error;
    string		_error_msg;
    bool		_is_running;
    ExecId		_exec_id;

    bool		_command_is_exited;
    bool		_command_is_signal_terminated;
    bool		_command_is_coredumped;
    bool		_command_is_stopped;
    int			_command_exit_status;
    int			_command_term_signal;
    int			_command_stop_signal;
    XorpTimer		_done_timer;

    bool		_stdout_eof_received;
    bool		_stderr_eof_received;

    int			_task_priority;
};

/**
 * @short A class for running an external command.
 */
class RunCommand : public RunCommandBase {
public:
    typedef XorpCallback2<void, RunCommand*, const string&>::RefPtr OutputCallback;
    typedef XorpCallback3<void, RunCommand*, bool, const string&>::RefPtr DoneCallback;
    typedef XorpCallback2<void, RunCommand*, int>::RefPtr StoppedCallback;

    /**
     * Constructor for a given command and its list with arguments.
     *
     * @param eventloop the event loop.
     * @param command the command to execute.
     * @param argument_list the list with the arguments for the command
     * to execute.
     * @param stdout_cb the callback to call when there is data on the
     * standard output.
     * @param stderr_cb the callback to call when there is data on the
     * standard error.
     * @param done_cb the callback to call when the command is completed.
     * @param redirect_stderr_to_stdout if true redirect the stderr to stdout.
     * @param task_priority the priority to read stdout and stderr.
     */
    RunCommand(EventLoop&			eventloop,
	       const string&			command,
	       const list<string>&		argument_list,
	       RunCommand::OutputCallback	stdout_cb,
	       RunCommand::OutputCallback	stderr_cb,
	       RunCommand::DoneCallback		done_cb,
	       bool				redirect_stderr_to_stdout,
	       int task_priority = XorpTask::PRIORITY_DEFAULT);

    /**
     * Set the callback to dispatch when the program is stopped.
     *
     * @param cb the callback's value.
     */
    void set_stopped_cb(StoppedCallback cb) { _stopped_cb = cb; }

private:
    /**
     * A method called when there is output on the program's stdout.
     * 
     * @param output the string with the output.
     */
    void stdout_cb_dispatch(const string& output) {
	_stdout_cb->dispatch(this, output);
    }

    /**
     * A method called when there is output on the program's stderr.
     * 
     * @param output the string with the output.
     */
    void stderr_cb_dispatch(const string& output) {
	_stderr_cb->dispatch(this, output);
    }

    /**
     * A method called when the program execution is completed.
     * 
     * @param success if true the program execution has succeeded, otherwise
     * it has failed.
     * @param error_msg if error, the string with the error message.
     */
    void done_cb_dispatch(bool success, const string& error_msg) {
	_done_cb->dispatch(this, success, error_msg);
    }

    /**
     * A method called when the program has been stopped.
     * 
     * @param stop_signal the signal used to stop the program.
     */
    void stopped_cb_dispatch(int stop_signal) {
	if (! _stopped_cb.is_empty())
	    _stopped_cb->dispatch(this, stop_signal);
    }

    /**
     * Test if the stderr should be redirected to stdout.
     * 
     * @return true if the stderr should be redirected to stdout, otherwise
     * false.
     */
    bool redirect_stderr_to_stdout() const {
	return (_redirect_stderr_to_stdout);
    }

    OutputCallback	_stdout_cb;
    OutputCallback	_stderr_cb;
    DoneCallback	_done_cb;
    StoppedCallback	_stopped_cb;

    bool		_redirect_stderr_to_stdout;
};

/**
 * @short A class for running an external command by invoking a shell.
 */
class RunShellCommand : public RunCommandBase {
public:
    typedef XorpCallback2<void, RunShellCommand*, const string&>::RefPtr OutputCallback;
    typedef XorpCallback3<void, RunShellCommand*, bool, const string&>::RefPtr DoneCallback;
    typedef XorpCallback2<void, RunShellCommand*, int>::RefPtr StoppedCallback;

    /**
     * Constructor for a given command and its list with arguments.
     *
     * @param eventloop the event loop.
     * @param command the command to execute.
     * @param argument_string the string with the arguments for the command
     * to execute.
     * @param stdout_cb the callback to call when there is data on the
     * standard output.
     * @param stderr_cb the callback to call when there is data on the
     * standard error.
     * @param done_cb the callback to call when the command is completed.
     * @param task_priority the priority to read stdout and stderr.
     */
    RunShellCommand(EventLoop&				eventloop,
		    const string&			command,
		    const string&			argument_string,
		    RunShellCommand::OutputCallback	stdout_cb,
		    RunShellCommand::OutputCallback	stderr_cb,
		    RunShellCommand::DoneCallback	done_cb,
		    int task_priority = XorpTask::PRIORITY_DEFAULT);

    /**
     * Set the callback to dispatch when the program is stopped.
     *
     * @param cb the callback's value.
     */
    void set_stopped_cb(StoppedCallback cb) { _stopped_cb = cb; }

private:
    /**
     * A method called when there is output on the program's stdout.
     * 
     * @param output the string with the output.
     */
    void stdout_cb_dispatch(const string& output) {
	_stdout_cb->dispatch(this, output);
    }

    /**
     * A method called when there is output on the program's stderr.
     * 
     * @param output the string with the output.
     */
    void stderr_cb_dispatch(const string& output) {
	_stderr_cb->dispatch(this, output);
    }

    /**
     * A method called when the program execution is completed.
     * 
     * @param success if true the program execution has succeeded, otherwise
     * it has failed.
     * @param error_msg if error, the string with the error message.
     */
    void done_cb_dispatch(bool success, const string& error_msg) {
	_done_cb->dispatch(this, success, error_msg);
    }

    /**
     * A method called when the program has been stopped.
     * 
     * @param stop_signal the signal used to stop the program.
     */
    void stopped_cb_dispatch(int stop_signal) {
	if (! _stopped_cb.is_empty())
	    _stopped_cb->dispatch(this, stop_signal);
    }

    /**
     * Test if the stderr should be redirected to stdout.
     * 
     * @return true if the stderr should be redirected to stdout, otherwise
     * false.
     */
    bool redirect_stderr_to_stdout() const {
	//
	// XXX: Redirecting stderr to stdout is always disabled by defailt.
	// If we want stderr to be redirected, this should be specified
	// by the executed command itself. E.g.:
	//     "my_command my_args 2>&1"
	//
	return (false);
    }

    OutputCallback	_stdout_cb;
    OutputCallback	_stderr_cb;
    DoneCallback	_done_cb;
    StoppedCallback	_stopped_cb;
};

#endif // __LIBXORP_RUN_COMMAND_HH__
