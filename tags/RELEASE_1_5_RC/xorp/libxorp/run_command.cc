// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/libxorp/run_command.cc,v 1.32 2007/09/04 16:40:11 bms Exp $

#include "libxorp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"
#include "libxorp/xorpfd.hh"
#include "libxorp/asyncio.hh"
#include "libxorp/popen.hh"
#ifdef HOST_OS_WINDOWS
#include "libxorp/win_io.h"
#endif

#include <map>

#ifdef HAVE_PATHS_H
#include <paths.h>
#endif

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif

#include "run_command.hh"


#ifdef HOST_OS_WINDOWS

#ifndef _PATH_BSHELL
#define _PATH_BSHELL "C:\\MSYS\\bin\\sh.exe"
#endif


#ifdef fileno
#undef fileno
#endif

#define	fileno(stream) (_get_osfhandle(_fileno(stream)))

#else // ! HOST_OS_WINDOWS

#ifndef _PATH_BSHELL
#define _PATH_BSHELL "/bin/sh"
#endif

#endif // ! HOST_OS_WINDOWS

static map<pid_t, RunCommandBase *> pid2command;

#ifndef HOST_OS_WINDOWS
static void
child_handler(int signo)
{
    XLOG_ASSERT(signo == SIGCHLD);

    //
    // XXX: Wait for any child process.
    // If we are executing any child process outside of the RunProcess
    // mechanism, then the waitpid() here may create a wait() blocking
    // problem for those processes. If this ever becomes an issue, then
    // we should try non-blocking waitpid() for each pid in the
    // pid2command map.
    //
    do {
	pid_t pid = 0;
	int wait_status = 0;
	map<pid_t, RunCommandBase *>::iterator iter;

	pid = waitpid(-1, &wait_status, WUNTRACED | WNOHANG);
	debug_msg("pid=%d, wait status=%d\n", XORP_INT_CAST(pid), wait_status);
	if (pid <= 0)
	    return;	// XXX: no more child processes

	XLOG_ASSERT(pid > 0);
	popen2_mark_as_closed(pid, wait_status);
	iter = pid2command.find(pid);
	if (iter == pid2command.end()) {
	    // XXX: we don't know anything about the exited process
	    continue;
	}

	RunCommandBase* run_command = iter->second;
	run_command->wait_status_changed(wait_status);
    } while (true);
}
#endif // ! HOST_OS_WINDOWS

static string
get_path_bshell()
{
#ifndef HOST_OS_WINDOWS
    return (string(_PATH_BSHELL));

#else // HOST_OS_WINDOWS
    //
    // Use the DOS style path to the user's shell specified in the
    // environment if available; otherwise, fall back to hard-coded default.
    //
    static string path_bshell;
    if (path_bshell.empty()) {
	char* g = getenv("SHELL");
	if (g != NULL)
	    path_bshell = string(g);
	else
	    path_bshell = string(_PATH_BSHELL);
    }
    return (path_bshell);
#endif // HOST_OS_WINDOWS
}

RunCommand::RunCommand(EventLoop&			eventloop,
		       const string&			command,
		       const list<string>&		argument_list,
		       RunCommand::OutputCallback	stdout_cb,
		       RunCommand::OutputCallback	stderr_cb,
		       RunCommand::DoneCallback		done_cb,
		       bool				redirect_stderr_to_stdout)
    : RunCommandBase(eventloop, command, command),
      _stdout_cb(stdout_cb),
      _stderr_cb(stderr_cb),
      _done_cb(done_cb),
      _redirect_stderr_to_stdout(redirect_stderr_to_stdout)
{
    set_argument_list(argument_list);
}

RunShellCommand::RunShellCommand(EventLoop&		eventloop,
				 const string&		command,
				 const string&		argument_string,
				 RunShellCommand::OutputCallback stdout_cb,
				 RunShellCommand::OutputCallback stderr_cb,
				 RunShellCommand::DoneCallback	 done_cb)
    : RunCommandBase(eventloop, get_path_bshell(), command),
      _stdout_cb(stdout_cb),
      _stderr_cb(stderr_cb),
      _done_cb(done_cb)
{
    list<string> l;

    string final_command_argument_string = command + " " + argument_string;

    l.push_back("-c");
    l.push_back(final_command_argument_string);

    set_argument_list(l);
}

RunCommandBase::RunCommandBase(EventLoop&		eventloop,
			       const string&		command,
			       const string&		real_command_name)
    : _eventloop(eventloop),
      _command(command),
      _real_command_name(real_command_name),
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
      _command_is_signal_terminated(false),
      _command_is_coredumped(false),
      _command_is_stopped(false),
      _command_exit_status(0),
      _command_term_signal(0),
      _command_stop_signal(0),
      _stdout_eof_received(false),
      _stderr_eof_received(false)
{
    memset(_stdout_buffer, 0, BUF_SIZE);
    memset(_stderr_buffer, 0, BUF_SIZE);

    _done_timer = _eventloop.new_timer(callback(this, &RunCommandBase::done));
}

RunCommandBase::~RunCommandBase()
{
    cleanup();
}

void
RunCommandBase::cleanup()
{
    terminate_with_prejudice();
    close_output();
    // Remove the entry from the pid map and perform other cleanup
    if (_pid != 0) {
	pid2command.erase(_pid);
	_pid = 0;
    }
    _done_timer.unschedule();
    _is_running = false;
    unblock_child_signals();
}

int
RunCommandBase::block_child_signals()
{
#ifndef HOST_OS_WINDOWS
    //
    // Block SIGCHLD signal in current signal mask
    //
    int r;
    sigset_t sigchld_sigset;

    r = sigemptyset(&sigchld_sigset);
    XLOG_ASSERT(r >= 0);
    r = sigaddset(&sigchld_sigset, SIGCHLD);
    XLOG_ASSERT(r >= 0);

    if (sigprocmask(SIG_BLOCK, &sigchld_sigset, NULL) < 0) {
	XLOG_ERROR("Failed to block SIGCHLD in current signal mask: %s",
		   strerror(errno));
	return (XORP_ERROR);
    }
#endif // ! HOST_OS_WINDOWS

    return (XORP_OK);
}

int
RunCommandBase::unblock_child_signals()
{
#ifndef HOST_OS_WINDOWS
    //
    // Unblock SIGCHLD signal in current signal mask
    //
    int r;
    sigset_t sigchld_sigset;

    r = sigemptyset(&sigchld_sigset);
    XLOG_ASSERT(r >= 0);
    r = sigaddset(&sigchld_sigset, SIGCHLD);
    XLOG_ASSERT(r >= 0);

    if (sigprocmask(SIG_UNBLOCK, &sigchld_sigset, NULL) < 0) {
	XLOG_ERROR("Failed to unblock SIGCHLD in current signal mask: %s",
		   strerror(errno));
	return (XORP_ERROR);
    }
#endif // ! HOST_OS_WINDOWS

    return (XORP_OK);
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

#ifndef HOST_OS_WINDOWS
    signal(SIGCHLD, child_handler);
#endif

    //
    // Temporary block the child signals
    //
    block_child_signals();

    //
    // Run the command
    //
    _pid = popen2(_command, _argument_list, _stdout_stream, _stderr_stream,
		  redirect_stderr_to_stdout());
    if (_stdout_stream == NULL) {
	XLOG_ERROR("Failed to execute command \"%s\"", final_command.c_str());
	cleanup();
	_exec_id.restore_saved_exec_id(error_msg);
	return (XORP_ERROR);
    }
    // Insert the new process to the map of processes
    XLOG_ASSERT(pid2command.find(_pid) == pid2command.end());
    pid2command[_pid] = this;

#ifdef HOST_OS_WINDOWS
    // We need to obtain the handle directly from the popen code because
    // the handle returned by CreateProcess() has the privileges we need.
    _ph = pgethandle(_pid);

    // If the handle is invalid, the process failed to start.
    if (_ph == INVALID_HANDLE_VALUE) {
	XLOG_ERROR("Failed to execute command \"%s\"", final_command.c_str());
	cleanup();
	_exec_id.restore_saved_exec_id(error_msg);
	return (XORP_ERROR);
    }

    // We can't rely on end-of-file indicators alone on Win32 to determine
    // when a child process died; we must wait for it in the event loop.
    if (!_eventloop.add_ioevent_cb(
	    _ph,
	    IOT_EXCEPTION,
	    callback(this, &RunCommandBase::win_proc_done_cb)))
	XLOG_FATAL("Cannot add child process handle to event loop.\n");
#endif // HOST_OS_WINDOWS

    // Create the stdout and stderr readers
    _stdout_file_reader = new AsyncFileReader(_eventloop,
					      XorpFd(fileno(_stdout_stream)));
    _stdout_file_reader->add_buffer(
	_stdout_buffer,
	BUF_SIZE,
	callback(this, &RunCommandBase::append_data));
    if (! _stdout_file_reader->start()) {
	XLOG_ERROR("Failed to start a stdout reader for command \"%s\"",
		   final_command.c_str());
	cleanup();
	_exec_id.restore_saved_exec_id(error_msg);
	return (XORP_ERROR);
    }

    _stderr_file_reader = new AsyncFileReader(_eventloop,
					      XorpFd(fileno(_stderr_stream)));
    _stderr_file_reader->add_buffer(
	_stderr_buffer,
	BUF_SIZE,
	callback(this, &RunCommandBase::append_data));
    if (! _stderr_file_reader->start()) {
	XLOG_ERROR("Failed to start a stderr reader for command \"%s\"",
		   final_command.c_str());
	cleanup();
	_exec_id.restore_saved_exec_id(error_msg);
	return (XORP_ERROR);
    }

    _is_running = true;

    //
    // Restore the saved execution ID
    //
    _exec_id.restore_saved_exec_id(error_msg);

    //
    // Unblock the child signals that were blocked earlier
    //
    unblock_child_signals();

    return (XORP_OK);
}

void
RunCommandBase::terminate()
{
    terminate_process(false);
}

void
RunCommandBase::terminate_with_prejudice()
{
    terminate_process(true);
}

void
RunCommandBase::terminate_process(bool with_prejudice)
{
    // Kill the process group
    if (0 != _pid) {
#ifdef HOST_OS_WINDOWS
	UNUSED(with_prejudice);
	//
	// _ph will be invalid if the process didn't start.
	// _ph will be valid if the process handle is open but the
	// process is no longer running. Calling TerminateProcess()
	// on a valid handle to a process which has terminated results
	// in an ACCESS_DENIED error.
	// Don't close the handle. pclose2() does this.
	//
	if (_ph != INVALID_HANDLE_VALUE) {
	    DWORD result; 
	    GetExitCodeProcess(_ph, &result);
	    if (result == STILL_ACTIVE) {
		DWORD result = 0;
		result = TerminateProcess(_ph, 32768);
		if (result == 0) {
		    XLOG_WARNING("TerminateProcess(): %s",
				 win_strerror(GetLastError()));
		}
	    }
	}
#else // ! HOST_OS_WINDOWS
	if (with_prejudice)
	    killpg(_pid, SIGKILL);
	else
	    killpg(_pid, SIGTERM);
#endif // ! HOST_OS_WINDOWS
    }
}

void
RunCommandBase::wait_status_changed(int wait_status)
{
    set_command_status(wait_status);

    //
    // XXX: Schedule a timer to complete the command so we can return
    // control to the caller.
    //
    // TODO: Temporary print any errors and catch any exceptions
    // (for debugging purpose).
    try {
	errno = 0;
	_done_timer.schedule_now();
    } catch(...) {
	XLOG_ERROR("Error scheduling RunCommand::_done_timer: %d", errno);
	xorp_catch_standard_exceptions();
    }
}

void
RunCommandBase::close_output()
{
    //
    // XXX: we should close stderr before stdout, because
    // close_stdout_output() invokes pclose2() that performs the
    // final cleanup.
    //
    close_stderr_output();
    close_stdout_output();
}

void
RunCommandBase::close_stdout_output()
{
    if (_stdout_file_reader != NULL) {
	delete _stdout_file_reader;
	_stdout_file_reader = NULL;
    }

    if (_stdout_stream != NULL) {
#ifdef HOST_OS_WINDOWS
	// pclose2() will close the process handle from under us.
	_eventloop.remove_ioevent_cb(_ph, IOT_EXCEPTION);
	_ph = INVALID_HANDLE_VALUE;

	//
	// XXX: in case of Windows we don't use the SIGCHLD mechanism
	// hence the process is done when the I/O is completed.
	//
	int status = pclose2(_stdout_stream, false);
	_stdout_stream = NULL;
	set_command_status(status);
	_command_is_exited = true;	// XXX: A Windows hack

#else // ! HOST_OS_WINDOWS
	pclose2(_stdout_stream, true);
	_stdout_stream = NULL;
#endif // ! HOST_OS_WINDOWS
    }
}

void
RunCommandBase::close_stderr_output()
{
    if (_stderr_file_reader != NULL) {
	delete _stderr_file_reader;
	_stderr_file_reader = NULL;
    }

    //
    // XXX: don't call pclose2(_stderr_stream), because pclose2(_stdout_stream)
    // automatically takes care of the _stderr_stream as well.
    //
    _stderr_stream = NULL;
}

void
RunCommandBase::set_command_status(int status)
{
    // Reset state
    _command_is_exited = false;
    _command_is_signal_terminated = false;
    _command_is_coredumped = false;
    _command_is_stopped = false;
    _command_exit_status = 0;
    _command_term_signal = 0;
    _command_stop_signal = 0;

    // Set the command status
#ifdef HOST_OS_WINDOWS
    _command_exit_status = status;
#else // ! HOST_OS_WINDOWS
    if (status >= 0) {
	_command_is_exited = WIFEXITED(status);
	_command_is_signal_terminated = WIFSIGNALED(status);
	_command_is_stopped = WIFSTOPPED(status);
	if (_command_is_exited)
	    _command_exit_status = WEXITSTATUS(status);
	if (_command_is_signal_terminated) {
	    _command_term_signal = WTERMSIG(status);
	    _command_is_coredumped = WCOREDUMP(status);
	}
	if (_command_is_stopped) {
	    _command_stop_signal = WSTOPSIG(status);
	}
    }
#endif // ! HOST_OS_WINDOWS

    if (_command_is_stopped)
	stopped_cb_dispatch(_command_stop_signal);
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
	io_done(event, error_code);
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
	    _stdout_file_reader->add_buffer(
		_stdout_buffer,
		BUF_SIZE,
		callback(this, &RunCommandBase::append_data));
	    _stdout_file_reader->start();
	} else {
	    memset(_stderr_buffer, 0, BUF_SIZE);
	    _stderr_file_reader->add_buffer(
		_stderr_buffer,
		BUF_SIZE,
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
	do {
	    if (_stdout_eof_received
		&& (_stderr_eof_received || redirect_stderr_to_stdout())) {
		io_done(event, 0);
		break;
	    }
	    if ((! is_stdout) && _stderr_eof_received) {
		close_stderr_output();
		break;
	    }
	    break;
	} while (false);
	return;
    }
}

void
RunCommandBase::io_done(AsyncFileOperator::Event event, int error_code)
{
    if (event != AsyncFileOperator::END_OF_FILE) {
	string prefix, suffix;

	_is_error = true;
	if (_error_msg.size()) {
	    prefix = "[";
	    suffix = "]";
	}
	_error_msg += prefix;
	_error_msg += c_format("Command \"%s\" terminated because of "
			       "unexpected event (event = 0x%x error = %d).",
			       _real_command_name.c_str(), event, error_code);
	_error_msg += suffix;
	terminate_with_prejudice();
    }

    close_output();

    done(_done_timer);
}

void
RunCommandBase::done(XorpTimer& done_timer)
{
    string prefix, suffix, reason;

    done_timer.unschedule();

    if (_stdout_stream != NULL)
	return;		// XXX: I/O is not done yet

    if (! (_command_is_exited || _command_is_signal_terminated))
	return;		// XXX: the command is not done yet

    // Remove the entry from the pid map
    pid2command.erase(_pid);
    _pid = 0;
    _done_timer.unschedule();
    _is_running = false;

    if (_error_msg.size()) {
	prefix = "[";
	suffix = "]";
    }
    _error_msg += prefix;

    if (_command_is_exited && (_command_exit_status != 0)) {
	_is_error = true;
	if (! reason.empty())
	    reason += "; ";
	reason += c_format("exited with exit status %d",
			   _command_exit_status);
    }
    if (_command_is_signal_terminated) {
	_is_error = true;
	if (! reason.empty())
	    reason += "; ";
	reason += c_format("terminated with signal %d",
			   _command_term_signal);
    }
    if (_command_is_coredumped) {
	_is_error = true;
	if (! reason.empty())
	    reason += "; ";
	reason += c_format("aborted with a core dump");
    }
    if (! reason.empty()) {
	_error_msg += c_format("Command \"%s\": %s.",
			       _real_command_name.c_str(),
			       reason.c_str());
    }

    _error_msg += suffix;

    done_cb_dispatch(!_is_error, _error_msg);

    //
    // XXX: the callback will delete us. Don't do anything more in this method.
    //
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
#else // ! HOST_OS_WINDOWS
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
#endif // ! HOST_OS_WINDOWS

    return (XORP_OK);
}

int
RunCommandBase::ExecId::set_effective_exec_id(string& error_msg)
{
#ifdef HOST_OS_WINDOWS
    UNUSED(error_msg);
#else // ! HOST_OS_WINDOWS
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
#endif // ! HOST_OS_WINDOWS

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
//
// Hack to get asynchronous notification of Win32 process death.
// The process handle will be SIGNALLED when the process terminates.
// Because Win32 pipes must be polled, we must make sure that RunCommand
// does not tear down the pipes until everything is read, otherwise the
// process death event will be dispatched before the pipe I/O events.
// We schedule a timer to make sure output is read from the pipes
// before they close, as Win32 pipes lose data when closed; the parent
// must hold handles for both ends of the pipe, unlike UNIX.
//
void
RunCommandBase::win_proc_done_cb(XorpFd fd, IoEventType type)
{
    XLOG_ASSERT(type == IOT_EXCEPTION);
    XLOG_ASSERT(static_cast<HANDLE>(fd) == _ph);
    _eventloop.remove_ioevent_cb(_ph, IOT_EXCEPTION);
    _reaper_timer = _eventloop.new_oneoff_after_ms(
	WIN32_PROC_TIMEOUT_MS,
	callback(this, &RunCommandBase::win_proc_reaper_cb));
    XLOG_ASSERT(_reaper_timer.scheduled());
}

void
RunCommandBase::win_proc_reaper_cb()
{
    io_done(AsyncFileOperator::END_OF_FILE, 0);
}
#endif // HOST_OS_WINDOWS
