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

// $XORP: xorp/libxorp/run_command.cc,v 1.1 2004/11/25 03:23:29 pavlin Exp $

#include "libxorp_module.h"
#include "xorp.h"

#include <sys/wait.h>

#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"
#include "libxorp/asyncio.hh"
#include "libxorp/popen.hh"
#include "run_command.hh"

RunCommand::RunCommand(EventLoop&			eventloop,
		       const string&			command,
		       const string&			arguments,
		       RunCommand::OutputCallback	stdout_cb,
		       RunCommand::OutputCallback	stderr_cb,
		       RunCommand::DoneCallback		done_cb)
    : _eventloop(eventloop),
      _command(command),
      _arguments(arguments),
      _stdout_cb(stdout_cb),
      _stderr_cb(stderr_cb),
      _done_cb(done_cb),
      _stdout_file_reader(NULL),
      _stderr_file_reader(NULL),
      _stdout_stream(NULL),
      _stderr_stream(NULL),
      _pid(0),
      _is_error(false),
      _last_offset(0),
      _is_running(false),
      _command_is_exited(false),
      _command_is_signaled(false),
      _command_is_stopped(false),
      _command_exit_status(0),
      _command_term_sig(0),
      _command_is_coredump(false),
      _command_stop_signal(0)
{
    memset(_stdout_buffer, 0, BUF_SIZE);
    memset(_stderr_buffer, 0, BUF_SIZE);
}

RunCommand::~RunCommand()
{
    if (_is_running)
	terminate();
}

int
RunCommand::execute()
{
    if (_is_running)
	return (XORP_OK);	// XXX: already running

    string final_command = _command;
    if (! _arguments.empty())
	final_command += " " + _arguments;

    // Run the command
    _pid = popen2(final_command, _stdout_stream, _stderr_stream);
    if (_stdout_stream == NULL) {
	XLOG_ERROR("Failed to execute command %s", final_command.c_str());
	terminate();
	return (XORP_ERROR);
    }

    // Create the stdout and stderr readers
    _stdout_file_reader = new AsyncFileReader(_eventloop,
					      fileno(_stdout_stream));
    _stdout_file_reader->add_buffer(_stdout_buffer, BUF_SIZE,
				    callback(this, &RunCommand::append_data));
    if (! _stdout_file_reader->start()) {
	XLOG_ERROR("Failed to start a stdout reader for command %s",
		   final_command.c_str());
	terminate();
	return (XORP_ERROR);
    }

    _stderr_file_reader = new AsyncFileReader(_eventloop,
					      fileno(_stderr_stream));
    _stderr_file_reader->add_buffer(_stderr_buffer, BUF_SIZE,
				    callback(this, &RunCommand::append_data));
    if (! _stderr_file_reader->start()) {
	XLOG_ERROR("Failed to start a stderr reader for command %s",
		   final_command.c_str());
	terminate();
	return (XORP_ERROR);
    }

    _is_running = true;

    return (XORP_OK);
}

void
RunCommand::terminate()
{
    // Kill the process group
    if (0 != _pid) {
	killpg(_pid, SIGTERM);
	_pid = 0;
    }

    close_output();
}

void
RunCommand::close_output()
{
    if (_stdout_file_reader != NULL) {
	delete _stdout_file_reader;
	_stdout_file_reader = NULL;
    }

    if (_stderr_file_reader != NULL) {
	delete _stderr_file_reader;
	_stderr_file_reader = NULL;
    }

    if (_stdout_stream != NULL) {
	int status = pclose2(_stdout_stream);
	_stdout_stream = NULL;

	// Get the command status
	if (status >= 0) {
	    _command_is_exited = WIFEXITED(status);
	    _command_is_signaled = WIFSIGNALED(status);
	    _command_is_stopped = WIFSTOPPED(status);
	    if (_command_is_exited)
		_command_exit_status = WEXITSTATUS(status);
	    if (_command_is_signaled) {
		_command_term_sig = WTERMSIG(status);
		_command_is_coredump = WCOREDUMP(status);
	    }
	    if (_command_is_stopped) {
		_command_stop_signal = WSTOPSIG(status);
	    }
	}
    }

    //
    // XXX: don't call pclose2(_stderr_stream), because pclose2(_stdout_stream)
    // automatically takes care of the _stderr_stream as well.
    //
    _stderr_stream = NULL;
}

void
RunCommand::append_data(AsyncFileOperator::Event	event,
			const uint8_t*			buffer,
			size_t				/* buffer_bytes */,
			size_t				offset)
{
    if (buffer == _stderr_buffer) {
	if (_is_error == false) {
	    // We hadn't previously seen any error output
	    if (event == AsyncFileOperator::END_OF_FILE) {
		// We just got EOF on stderr - ignore this and wait for
		// EOF on stdout
		return;
	    }
	    _is_error = true;
	    // Reset for reading error response
	    _error_msg = "";
	    _last_offset = 0;
	}
    } else {
	XLOG_ASSERT(buffer == _stdout_buffer);
	if (_is_error == true) {
	    // Discard further output on stdout
	    return;
	}
    }

    if ((event == AsyncFileOperator::DATA)
	|| (event == AsyncFileOperator::END_OF_FILE)) {
	XLOG_ASSERT(offset >= _last_offset);
	if (offset == _last_offset) {
	    XLOG_ASSERT(event == AsyncFileOperator::END_OF_FILE);
	    done(event);
	    return;
	}

	if (offset != _last_offset) {
	    const char* p   = (const char*)buffer + _last_offset;
	    size_t      len = offset - _last_offset;
	    debug_msg("len = %u\n", static_cast<uint32_t>(len));
	    if (!_is_error) {
		if (buffer == _stdout_buffer)
		    _stdout_cb->dispatch(this, string(p, len));
		else
		    _stderr_cb->dispatch(this, string(p, len));
	    } else {
		_error_msg.append(p, p + len);
	    }
	    _last_offset = offset;
	}

	if (event == AsyncFileOperator::END_OF_FILE) {
	    done(event);
	    return;
	}

	if (offset == BUF_SIZE) {
	    // The buffer is exhausted
	    _last_offset = 0;
	    if (_is_error) {
		memset(_stderr_buffer, 0, BUF_SIZE);
		_stderr_file_reader->add_buffer(_stderr_buffer, BUF_SIZE,
						callback(this, &RunCommand::append_data));
		_stderr_file_reader->start();
	    } else {
		memset(_stdout_buffer, 0, BUF_SIZE);
		_stdout_file_reader->add_buffer(_stdout_buffer, BUF_SIZE,
						callback(this, &RunCommand::append_data));
		_stdout_file_reader->start();
	    }
	}
    } else {
	// Something bad happened
	// XXX ideally we'd figure out what.
	_error_msg += "\nsomething bad happened\n";
	done(event);
    }
}

void
RunCommand::done(AsyncFileOperator::Event event)
{
    if (event != AsyncFileOperator::END_OF_FILE) {
	_is_error = true;
	terminate();
    } else {
	close_output();
    }

    if (_command_is_exited && (_command_exit_status != 0))
	_is_error = true;
    if (_command_is_coredump)
	_is_error = true;
    
    _done_cb->dispatch(this, !_is_error, _error_msg);

    // XXX: the callback will delete us. Don't do anything more in this method.
    // delete this;
}
