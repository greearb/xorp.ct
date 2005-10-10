// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

// $XORP: xorp/libxorp/run_command.hh,v 1.6 2005/08/18 15:28:40 bms Exp $

#ifndef __LIBXORP_RUN_COMMAND_HH__
#define __LIBXORP_RUN_COMMAND_HH__


#include <list>

#include "libxorp/asyncio.hh"
#include "libxorp/callback.hh"

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
     */
    RunCommandBase(EventLoop&				eventloop,
		   const string&			command);

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
     * Test if the stderr should be redirected to stdout.
     * 
     * @return true if the stderr should be redirected to stdout, otherwise
     * false.
     */
    virtual bool redirect_stderr_to_stdout() const = 0;

    /**
     * Close the output for the command.
     */
    void close_output();

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
     * The command has completed.
     *
     * @param Event the last event from the command
     * (@see AsyncFileOperator::Event).
     */
    void done(AsyncFileOperator::Event);

#ifdef HOST_OS_WINDOWS
    void win_proc_done_cb(XorpFd fd, IoEventType type);
#endif

    static const size_t	BUF_SIZE = 8192;
    EventLoop&		_eventloop;
    string		_command;
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
#endif
    bool		_is_error;
    string		_error_msg;
    bool		_is_running;
    ExecId		_exec_id;

    bool		_command_is_exited;
    bool		_command_is_signaled;
    bool		_command_is_stopped;
    int			_command_exit_status;
    int			_command_term_sig;
    bool		_command_is_coredump;
    int			_command_stop_signal;
};

/**
 * @short A class for running an external command.
 */
class RunCommand : public RunCommandBase {
public:
    typedef XorpCallback2<void, RunCommand*, const string&>::RefPtr OutputCallback;
    typedef XorpCallback3<void, RunCommand*, bool, const string&>::RefPtr DoneCallback;

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
     */
    RunCommand(EventLoop&			eventloop,
	       const string&			command,
	       const list<string>&		argument_list,
	       RunCommand::OutputCallback	stdout_cb,
	       RunCommand::OutputCallback	stderr_cb,
	       RunCommand::DoneCallback		done_cb,
	       bool				redirect_stderr_to_stdout);

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

    bool		_redirect_stderr_to_stdout;
};

/**
 * @short A class for running an external command by invoking a shell.
 */
class RunShellCommand : public RunCommandBase {
public:
    typedef XorpCallback2<void, RunShellCommand*, const string&>::RefPtr OutputCallback;
    typedef XorpCallback3<void, RunShellCommand*, bool, const string&>::RefPtr DoneCallback;

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
     */
    RunShellCommand(EventLoop&				eventloop,
		    const string&			command,
		    const string&			argument_string,
		    RunShellCommand::OutputCallback	stdout_cb,
		    RunShellCommand::OutputCallback	stderr_cb,
		    RunShellCommand::DoneCallback	done_cb);

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
};

#endif // __LIBXORP_RUN_COMMAND_HH__
