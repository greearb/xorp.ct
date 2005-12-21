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

// $XORP: xorp/libxorp/run_command.cc,v 1.16 2005/10/23 18:47:25 pavlin Exp $

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

#include "libxorp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/eventloop.hh"
#include "libxorp/xlog.h"
#include "libxorp/xorpfd.hh"
#include "libxorp/asyncio.hh"
#include "libxorp/popen.hh"
#include "run_command.hh"

#ifdef HOST_OS_WINDOWS

#include "libxorp/win_io.h"

#ifndef _PATH_BSHELL
#define _PATH_BSHELL "C:\\MSYS\\bin\\sh.exe"
#endif

static char *_path_bshell = NULL;

#define	fileno(stream) (_get_osfhandle(_fileno(stream)))

#else // !HOST_OS_WINDOWS

#ifndef _PATH_BSHELL
#define _PATH_BSHELL "/bin/sh"
#endif

#endif // HOST_OS_WINDOWS

RunCommand::RunCommand(EventLoop&			eventloop,
		       const string&			command,
		       const list<string>&		argument_list,
		       RunCommand::OutputCallback	stdout_cb,
		       RunCommand::OutputCallback	stderr_cb,
		       RunCommand::DoneCallback		done_cb,
		       bool				redirect_stderr_to_stdout)
    : RunCommandBase(eventloop, command),
      _stdout_cb(stdout_cb),
      _stderr_cb(stderr_cb),
      _done_cb(done_cb),
      _redirect_stderr_to_stdout(redirect_stderr_to_stdout)
{
    set_argument_list(argument_list);
}

RunShellCommand::RunShellCommand(EventLoop&			eventloop,
				 const string&			command,
				 const string&			argument_string,
				 RunShellCommand::OutputCallback stdout_cb,
				 RunShellCommand::OutputCallback stderr_cb,
				 RunShellCommand::DoneCallback	done_cb)
    : RunCommandBase(eventloop, _PATH_BSHELL),
      _stdout_cb(stdout_cb),
      _stderr_cb(stderr_cb),
      _done_cb(done_cb)
{
    list<string> l;

#ifdef HOST_OS_WINDOWS
    // XXX: THIS IS A DIRTY HACK.
    // Use the DOS style path to the user's shell specified in the
    // environment if available; otherwise, fall back to hard-coded default.
    if (_path_bshell == NULL) {
	_path_bshell = getenv("SHELL");
	if (_path_bshell == NULL)
	    _path_bshell = const_cast<char *>(_PATH_BSHELL);
	_command = string(_path_bshell);
    }
#endif

    string final_command_argument_string = command + " " + argument_string;

    l.push_back("-c");
    l.push_back(final_command_argument_string);

    set_argument_list(l);
}

RunCommandBase::RunCommandBase(EventLoop&			eventloop,
			       const string&			command)
    : _eventloop(eventloop),
      _command(command),
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
      _command_stop_signal(0),
      _stdout_eof_received(false),
      _stderr_eof_received(false)
{
    memset(_stdout_buffer, 0, BUF_SIZE);
    memset(_stderr_buffer, 0, BUF_SIZE);
}

RunCommandBase::~RunCommandBase()
{
    if (_is_running)
	terminate();
#ifdef HOST_OS_WINDOWS
    _reaper_timer.unschedule();
    _reaper_timer.clear();
#endif
}

int
RunCommandBase::execute()
{
    string error_msg;

    if (_is_running)
	return (XORP_OK);	// XXX: already running

    // Create a single string with the command name and the arguments
    string final_command = _command;
    list<string>::const_iterator iter;
    for (iter = _argument_list.begin(); iter != _argument_list.end(); ++iter) {
	final_command += " ";
	final_command += *iter;
    }

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
    _pid = popen2(_command, _argument_list, _stdout_stream, _stderr_stream,
		  redirect_stderr_to_stdout());
    if (_stdout_stream == NULL) {
	XLOG_ERROR("Failed to execute command %s", final_command.c_str());
	terminate();
	_exec_id.restore_saved_exec_id(error_msg);
	return (XORP_ERROR);
    }

#ifdef HOST_OS_WINDOWS
    // We need to obtain the handle directly from the popen code because
    // the handle returned by CreateProcess() has the privileges we need.
    _ph = pgethandle(_pid);

    // If the handle is invalid, the process failed to start.
    if (_ph == INVALID_HANDLE_VALUE) {
	XLOG_ERROR("Failed to execute command %s", final_command.c_str());
	terminate();
	_exec_id.restore_saved_exec_id(error_msg);
	return (XORP_ERROR);
    }

    // We can't rely on end-of-file indicators alone on Win32 to determine
    // when a child process died; we must wait for it in the event loop.
    if (!_eventloop.add_ioevent_cb(_ph, IOT_EXCEPTION,
			   callback(this, &RunCommandBase::win_proc_done_cb)))
	XLOG_FATAL("Cannot add child process handle to event loop.\n");
#endif

    // Create the stdout and stderr readers
    _stdout_file_reader = new AsyncFileReader(_eventloop,
      XorpFd(fileno(_stdout_stream))
     );
    _stdout_file_reader->add_buffer(_stdout_buffer, BUF_SIZE,
				    callback(this, &RunCommandBase::append_data));
    if (! _stdout_file_reader->start()) {
	XLOG_ERROR("Failed to start a stdout reader for command %s",
		   final_command.c_str());
	terminate();
	_exec_id.restore_saved_exec_id(error_msg);
	return (XORP_ERROR);
    }

    _stderr_file_reader = new AsyncFileReader(_eventloop,
      XorpFd(fileno(_stderr_stream))
     );
    _stderr_file_reader->add_buffer(_stderr_buffer, BUF_SIZE,
				    callback(this, &RunCommandBase::append_data));
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
RunCommandBase::terminate()
{
    // Kill the process group
    if (0 != _pid) {
#ifdef HOST_OS_WINDOWS
	// _ph will be invalid if the process didn't start.
	// _ph will be valid if the process handle is open but the
	// process is no longer running. Calling TerminateProcess()
	// on a valid handle to a process which has terminated results
	// in an ACCESS_DENIED error.
	// Don't close the handle. pclose2() does this.
	if (_ph != INVALID_HANDLE_VALUE) {
	    DWORD result; 
	    GetExitCodeProcess(_ph, &result);
	    if (result == STILL_ACTIVE) {
		DWORD result = TerminateProcess(_ph, 32768);
		if (result == 0) {
		    XLOG_WARNING("TerminateProcess(): %s",
				 win_strerror(GetLastError()));
		}
	    }
	}
#else
	killpg(_pid, SIGTERM);
#endif
	_pid = 0;
    }

    // XXX: May need to delay close on Windows, otherwise we may
    // lose output if we're terminated in this way.

    close_output();
}

void
RunCommandBase::close_output()
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
#ifdef HOST_OS_WINDOWS
	// pclose2() will close the process handle from under us.
	_eventloop.remove_ioevent_cb(_ph, IOT_EXCEPTION);
	_ph = INVALID_HANDLE_VALUE;
#endif
	int status = pclose2(_stdout_stream);
	_stdout_stream = NULL;

#ifdef HOST_OS_WINDOWS
	_command_is_exited = true;
	_command_exit_status = status;
#else /* !HOST_OS_WINDOWS */
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
#endif /* HOST_OS_WINDOWS */
    }

    //
    // XXX: don't call pclose2(_stderr_stream), because pclose2(_stdout_stream)
    // automatically takes care of the _stderr_stream as well.
    //
    _stderr_stream = NULL;
}

void
RunCommandBase::append_data(AsyncFileOperator::Event	event,
			    const uint8_t*		buffer,
			    size_t			/* buffer_bytes */,
			    size_t			offset)
{
    size_t* last_offset_ptr = NULL;
    bool is_stdout = false;

    if (buffer == _stdout_buffer) {
	is_stdout = true;
	last_offset_ptr = &_last_stdout_offset;
    } else {
	XLOG_ASSERT(buffer == _stderr_buffer);
	is_stdout = false;
	last_offset_ptr = &_last_stderr_offset;
    }

    if ((event != AsyncFileOperator::END_OF_FILE)
	&& (event != AsyncFileOperator::DATA)) {
	//
	// Something bad happened.
	// XXX: ideally we'd figure out what.
	//
	int error_code = 0;
	if (is_stdout)
	    error_code = _stdout_file_reader->error();
	else
	    error_code = _stderr_file_reader->error();
	done(event, error_code);
	return;
    }

    //
    // Either DATA or END_OF_FILE
    //
    XLOG_ASSERT(offset >= *last_offset_ptr);

    if (offset != *last_offset_ptr) {
	const char* p   = (const char*)buffer + *last_offset_ptr;
	size_t      len = offset - *last_offset_ptr;
	debug_msg("len = %u\n", XORP_UINT_CAST(len));
	if (!_is_error) {
	    if (is_stdout)
		stdout_cb_dispatch(string(p, len));
	    else
		stderr_cb_dispatch(string(p, len));
	} else {
	    _error_msg.append(p, p + len);
	}
	*last_offset_ptr = offset;
    }

    if (offset == BUF_SIZE) {
	// The buffer is exhausted
	*last_offset_ptr = 0;
	if (is_stdout) {
	    memset(_stdout_buffer, 0, BUF_SIZE);
	    _stdout_file_reader->add_buffer(_stdout_buffer, BUF_SIZE,
					    callback(this, &RunCommandBase::append_data));
	    _stdout_file_reader->start();
	} else {
	    memset(_stderr_buffer, 0, BUF_SIZE);
	    _stderr_file_reader->add_buffer(_stderr_buffer, BUF_SIZE,
					    callback(this, &RunCommandBase::append_data));
	    _stderr_file_reader->start();
	}
    }

    if (event == AsyncFileOperator::END_OF_FILE) {
	if (is_stdout) {
	    _stdout_eof_received = true;
	} else {
	    _stderr_eof_received = true;
	}
	if (_stdout_eof_received
	    && (_stderr_eof_received || redirect_stderr_to_stdout())) {
	    done(event, 0);
	}
	return;
    }
}

void
RunCommandBase::done(AsyncFileOperator::Event event, int error_code)
{
    string prefix, suffix;

    if (_error_msg.size()) {
	prefix = "[";
	suffix = "]";
    }
    _error_msg += prefix;

    if (event != AsyncFileOperator::END_OF_FILE) {
	_is_error = true;
	_error_msg += c_format("Command \"%s\" terminated because of "
			       "unexpected event (event = 0x%x error = %d).",
			       _command.c_str(), event, error_code);
	terminate();
    } else {
	close_output();
    }

    if (_command_is_exited && (_command_exit_status != 0)) {
	_is_error = true;
	_error_msg += c_format("Command \"%s\" exited with exit status %d.",
			       _command.c_str(), _command_exit_status);
    }
    if (_command_is_coredump) {
	_is_error = true;
	_error_msg += c_format("Command \"%s\" aborted with a core dump.",
			       _command.c_str());
    }

    _error_msg += suffix;

    done_cb_dispatch(!_is_error, _error_msg);

    // XXX: the callback will delete us. Don't do anything more in this method.
    // delete this;
}

void
RunCommandBase::set_exec_id(const ExecId& v)
{
    _exec_id = v;
}

RunCommandBase::ExecId::ExecId()
    : _uid(0),
      _gid(0),
      _is_uid_set(false),
      _is_gid_set(false),
      _saved_uid(0),
      _saved_gid(0),
      _is_exec_id_saved(false)
{

}

RunCommandBase::ExecId::ExecId(uid_t uid)
    : _uid(uid),
      _gid(0),
      _is_uid_set(true),
      _is_gid_set(false),
      _saved_uid(0),
      _saved_gid(0),
      _is_exec_id_saved(false)
{

}

RunCommandBase::ExecId::ExecId(uid_t uid, gid_t gid)
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
RunCommandBase::ExecId::save_current_exec_id()
{
#ifndef HOST_OS_WINDOWS
    _saved_uid = getuid();
    _saved_gid = getgid();
#endif
    _is_exec_id_saved = true;
}

int
RunCommandBase::ExecId::restore_saved_exec_id(string& error_msg) const
{
#ifdef HOST_OS_WINDOWS
    UNUSED(error_msg);
#else
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
#endif

    return (XORP_OK);
}

int
RunCommandBase::ExecId::set_effective_exec_id(string& error_msg)
{
#ifdef HOST_OS_WINDOWS
    UNUSED(error_msg);
#else
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
#endif

    return (XORP_OK);
}

bool
RunCommandBase::ExecId::is_set() const
{
    return (is_uid_set() || is_gid_set());
}

void
RunCommandBase::ExecId::reset()
{
    _uid = 0;
    _gid = 0;
    _is_uid_set = false;
    _is_gid_set = false;
    _saved_uid = 0;
    _saved_gid = 0;
    _is_exec_id_saved = false;
}

#ifdef HOST_OS_WINDOWS
// Hack to get asynchronous notification of Win32 process death.
// The process handle will be SIGNALLED when the process terminates.
// Because Win32 pipes must be polled, we must make sure that RunCommand
// does not tear down the pipes until everything is read, otherwise the
// process death event will be dispatched before the pipe I/O events.
// We schedule a timer to make sure output is read from the pipes
// before they close, as Win32 pipes lose data when closed; the parent
// must hold handles for both ends of the pipe, unlike UNIX.
void
RunCommandBase::win_proc_done_cb(XorpFd fd, IoEventType type)
{
    XLOG_ASSERT(type == IOT_EXCEPTION);
    XLOG_ASSERT(static_cast<HANDLE>(fd) == _ph);
    _eventloop.remove_ioevent_cb(_ph, IOT_EXCEPTION);
    _reaper_timer = _eventloop.new_oneoff_after_ms(WIN32_PROC_TIMEOUT_MS,
	callback(this, &RunCommandBase::win_proc_reaper_cb));
    XLOG_ASSERT(_reaper_timer.scheduled());
}

void
RunCommandBase::win_proc_reaper_cb()
{
    done(AsyncFileOperator::END_OF_FILE, 0);
}
#endif
