// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/libxorp/run_command.hh,v 1.1 2004/11/25 03:23:29 pavlin Exp $

#ifndef __LIBXORP_RUN_COMMAND_HH__
#define __LIBXORP_RUN_COMMAND_HH__


#include "libxorp/asyncio.hh"
#include "libxorp/callback.hh"

class EventLoop;


/**
 * @short Class for running an external command.
 */
class RunCommand {
public:
    typedef XorpCallback2<void, RunCommand*, const string&>::RefPtr OutputCallback;
    typedef XorpCallback3<void, RunCommand*, bool, const string&>::RefPtr DoneCallback;

    /**
     * Constructor for a given command and its arguments.
     *
     * @param eventloop the event loop.
     * @param command the command to execute.
     * @param arguments the arguments for the command to execute.
     * @param stdout_cb the callback to call when there is data on the
     * standard output.
     * @param stderr_cb the callback to call when there is data on the
     * standard error.
     * @param done_cb the callback to call when the command is completed.
     */
    RunCommand(EventLoop&			eventloop,
	       const string&			command,
	       const string&			arguments,
	       RunCommand::OutputCallback	stdout_cb,
	       RunCommand::OutputCallback	stderr_cb,
	       RunCommand::DoneCallback		done_cb);

    /**
     * Destructor.
     */
    ~RunCommand();

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
     * Get the arguments of the command to execute.
     *
     * @return a string with the arguments of the command to execute.
     */
    const string& arguments() const { return _arguments; }

private:
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

    static const size_t	BUF_SIZE = 8192;
    EventLoop&		_eventloop;
    string		_command;
    string		_arguments;
    OutputCallback	_stdout_cb;
    OutputCallback	_stderr_cb;
    DoneCallback	_done_cb;

    AsyncFileReader*	_stdout_file_reader;
    AsyncFileReader*	_stderr_file_reader;
    FILE*		_stdout_stream;
    FILE*		_stderr_stream;
    uint8_t		_stdout_buffer[BUF_SIZE];
    uint8_t		_stderr_buffer[BUF_SIZE];
    pid_t		_pid;
    bool		_is_error;
    string		_error_msg;
    size_t		_last_offset;
    bool		_is_running;

    bool		_command_is_exited;
    bool		_command_is_signaled;
    bool		_command_is_stopped;
    int			_command_exit_status;
    int			_command_term_sig;
    bool		_command_is_coredump;
    int			_command_stop_signal;
};

#endif // __LIBXORP_RUN_COMMAND_HH__
