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

// $XORP: xorp/libxorp/run_command.cc,v 1.5 2005/06/27 08:38:06 pavlin Exp $

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
      _last_stdout_offset(0),
      _last_stderr_offset(0),
      _pid(0),
      _is_error(false),
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
    string error_msg;

    if (_is_running)
	return (XORP_OK);	// XXX: already running

    string final_command = _command;
    if (! _arguments.empty())
	final_command += " " + _arguments;

    //
    // Save the current execution ID, and set the new execution ID
    //
    _exec_id.save_current_exec_id();
    if (_exec_id.set_effective_exec_id(error_msg) != XORP_OK) {
	XLOG_ERROR("Failed to set effective execution ID: %s",
		   error_msg.c_str());
	_exec_id.restore_saved_exec_id(error_msg);
	return (XORP_ERROR);
    }

    // Run the command
    _pid = popen2(final_command, _stdout_stream, _stderr_stream);
    if (_stdout_stream == NULL) {
	XLOG_ERROR("Failed to execute command %s", final_command.c_str());
	terminate();
	_exec_id.restore_saved_exec_id(error_msg);
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
	_exec_id.restore_saved_exec_id(error_msg);
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
	_exec_id.restore_saved_exec_id(error_msg);
	return (XORP_ERROR);
    }

    _is_running = true;

    //
    // Restore the saved execution ID
    //
    _exec_id.restore_saved_exec_id(error_msg);

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
    size_t* last_offset_ptr = NULL;
    if (buffer == _stderr_buffer) {
	if (_last_stderr_offset == 0) {
	    // We hadn't previously seen any error output
	    if (event == AsyncFileOperator::END_OF_FILE) {
		// We just got EOF on stderr - ignore this and wait for
		// EOF on stdout
		return;
	    }
	}
	last_offset_ptr = &_last_stderr_offset;
    } else {
	XLOG_ASSERT(buffer == _stdout_buffer);
	last_offset_ptr = &_last_stdout_offset;
    }

    if ((event == AsyncFileOperator::DATA)
	|| (event == AsyncFileOperator::END_OF_FILE)) {
	XLOG_ASSERT(offset >= *last_offset_ptr);
	if (offset == *last_offset_ptr) {
	    XLOG_ASSERT(event == AsyncFileOperator::END_OF_FILE);
	    done(event);
	    return;
	}

	if (offset != *last_offset_ptr) {
	    const char* p   = (const char*)buffer + *last_offset_ptr;
	    size_t      len = offset - *last_offset_ptr;
	    debug_msg("len = %u\n", XORP_UINT_CAST(len));
	    if (!_is_error) {
		if (buffer == _stdout_buffer)
		    _stdout_cb->dispatch(this, string(p, len));
		else
		    _stderr_cb->dispatch(this, string(p, len));
	    } else {
		_error_msg.append(p, p + len);
	    }
	    *last_offset_ptr = offset;
	}

	if (event == AsyncFileOperator::END_OF_FILE) {
	    done(event);
	    return;
	}

	if (offset == BUF_SIZE) {
	    // The buffer is exhausted
	    *last_offset_ptr = 0;
	    if (buffer == _stdout_buffer) {
		memset(_stdout_buffer, 0, BUF_SIZE);
		_stdout_file_reader->add_buffer(_stdout_buffer, BUF_SIZE,
						callback(this, &RunCommand::append_data));
		_stdout_file_reader->start();
	    } else {
		memset(_stderr_buffer, 0, BUF_SIZE);
		_stderr_file_reader->add_buffer(_stderr_buffer, BUF_SIZE,
						callback(this, &RunCommand::append_data));
		_stderr_file_reader->start();
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

void
RunCommand::set_exec_id(const ExecId& v)
{
    _exec_id = v;
}

RunCommand::ExecId::ExecId()
    : _uid(0),
      _gid(0),
      _is_uid_set(false),
      _is_gid_set(false),
      _saved_uid(0),
      _saved_gid(0),
      _is_exec_id_saved(false)
{

}

RunCommand::ExecId::ExecId(uid_t uid)
    : _uid(uid),
      _gid(0),
      _is_uid_set(true),
      _is_gid_set(false),
      _saved_uid(0),
      _saved_gid(0),
      _is_exec_id_saved(false)
{

}

RunCommand::ExecId::ExecId(uid_t uid, gid_t gid)
    : _uid(uid),
      _gid(gid),
      _is_uid_set(true),
      _is_gid_set(true),
      _saved_uid(0),
      _saved_gid(0),
      _is_exec_id_saved(false)
{

}

void
RunCommand::ExecId::save_current_exec_id()
{
    _saved_uid = getuid();
    _saved_gid = getgid();
    _is_exec_id_saved = true;
}

int
RunCommand::ExecId::restore_saved_exec_id(string& error_msg) const
{
    if (! _is_exec_id_saved)
	return (XORP_OK);	// Nothing to do, because nothing was saved

    if (seteuid(saved_uid()) != 0) {
	error_msg = c_format("Cannot restore saved user ID to %u: %s",
			     XORP_UINT_CAST(saved_uid()), strerror(errno));
	return (XORP_ERROR);
    }

    if (setegid(saved_gid()) != 0) {
	error_msg = c_format("Cannot restore saved group ID to %u: %s",
			     XORP_UINT_CAST(saved_gid()), strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
RunCommand::ExecId::set_effective_exec_id(string& error_msg)
{
    if (! is_set())
	return (XORP_OK);

    //
    // Set the effective group ID
    //
    if (is_gid_set() && (gid() != saved_gid())) {
	if (setegid(gid()) != 0) {
	    error_msg = c_format("Cannot set the effective group ID to %u: %s",
				 XORP_UINT_CAST(gid()), strerror(errno));
	    return (XORP_ERROR);
	}
    }

    //
    // Set the effective user ID
    //
    if (is_uid_set() && (uid() != saved_uid())) {
	if (seteuid(uid()) != 0) {
	    error_msg = c_format("Cannot set effective user ID to %u: %s",
				 XORP_UINT_CAST(uid()), strerror(errno));
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);
}

bool
RunCommand::ExecId::is_set() const
{
    return (is_uid_set() || is_gid_set());
}

void
RunCommand::ExecId::reset()
{
    _uid = 0;
    _gid = 0;
    _is_uid_set = false;
    _is_gid_set = false;
    _saved_uid = 0;
    _saved_gid = 0;
    _is_exec_id_saved = false;
}
