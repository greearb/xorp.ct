// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/fea/click_socket.cc,v 1.6 2004/11/23 00:53:19 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/run_command.hh"

#include "libcomm/comm_api.h"

#include <algorithm>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <errno.h>

#include "click_socket.hh"


uint16_t ClickSocket::_instance_cnt = 0;
pid_t ClickSocket::_pid = getpid();

const IPv4 ClickSocket::DEFAULT_USER_CLICK_CONTROL_ADDRESS = IPv4::LOOPBACK();

//
// Click Sockets communication with Click
//

ClickSocket::ClickSocket(EventLoop& eventloop)
    : _eventloop(eventloop),
      _fd(-1),
      _seqno(0),
      _instance_no(_instance_cnt++),
      _is_enabled(false),
      _is_kernel_click(false),
      _is_user_click(false),
      _user_click_command_execute_on_startup(false),
      _user_click_control_address(DEFAULT_USER_CLICK_CONTROL_ADDRESS),
      _user_click_control_socket_port(DEFAULT_USER_CLICK_CONTROL_SOCKET_PORT),
      _user_click_run_command(NULL)
{

}

ClickSocket::~ClickSocket()
{
    stop();
    XLOG_ASSERT(_ol.empty());
}

int
ClickSocket::start()
{
    if (is_kernel_click()) {
	//
	// TODO: XXX: PAVPAVPAV: Mount the Click FS, etc.
	//
    }

    if (is_user_click()) {
	if (_fd >= 0)
	    return (XORP_OK);

	//
	// Execute the Click command (if necessary)
	//
	if (_user_click_command_execute_on_startup) {
	    // Compose the command and the arguments
	    string command = _user_click_command_file;
	    string arguments = c_format("-f %s -p %u",
					_user_click_startup_config_file.c_str(),
					_user_click_control_socket_port);
	    if (! _user_click_command_extra_arguments.empty())
		arguments += " " + _user_click_command_extra_arguments;

	    if (execute_user_click_command(command, arguments) != XORP_OK) {
		XLOG_ERROR("Could not execute the user-level Click");
		return (XORP_ERROR);
	    }
	}

	//
	// Open the socket
	//
	struct in_addr in_addr;
	_user_click_control_address.copy_out(in_addr);
	//
	// TODO: XXX: get rid of this hackish mechanism of waiting
	// pre-defined amount of time until the user-level Click program
	// starts responding.
	//
	TimeVal max_wait_time(1, 0);		// XXX: max wait time: 1 second
	TimeVal curr_wait_time(0, 100000);	// XXX: 100ms
	TimeVal total_wait_time;
	do {
	    //
	    // XXX: try-and-wait a number of times up to "max_wait_time",
	    // because the user-level Click program may not response
	    // immediately.
	    //
	    TimerList::system_sleep(curr_wait_time);
	    total_wait_time += curr_wait_time;
	    _fd = comm_connect_tcp4(&in_addr,
				    htons(_user_click_control_socket_port),
				    COMM_SOCK_BLOCKING);
	    if (_fd >= 0)
		break;
	    if (total_wait_time < max_wait_time) {
		// XXX: exponentially increase the wait time
		curr_wait_time += curr_wait_time;
		if (total_wait_time + curr_wait_time > max_wait_time)
		    curr_wait_time = max_wait_time - total_wait_time;
		XLOG_WARNING("Could not open user-level Click socket: %s. "
			     "Trying again...",
			     strerror(errno));
		continue;
	    }
	    XLOG_ERROR("Could not open user-level Click socket: %s",
		       strerror(errno));
	    terminate_user_click_command();
	    return (XORP_ERROR);
	} while (true);

	//
	// Read the expected banner
	//
	vector<uint8_t> message;
	string errmsg;
	if (force_read_message(message, errmsg) != XORP_OK) {
	    XLOG_ERROR("Could not read on startup from user-level Click "
		       "socket: %s", errmsg.c_str());
	    terminate_user_click_command();
	    comm_close(_fd);
	    _fd = -1;
	    return (XORP_ERROR);
	}

	//
	// Check the expected banner.
	// The banner should look like: "Click::ControlSocket/1.1"
	//
	do {
	    string::size_type slash1, slash2, dot1, dot2;
	    string banner = string(reinterpret_cast<const char*>(&message[0]),
				   message.size());
	    string version;
	    int major, minor;

	    // Find the version number and check it.
	    slash1 = banner.find('/');
	    if (slash1 == string::npos) {
		XLOG_ERROR("Invalid Click banner: %s", banner.c_str());
		goto error_label;
	    }
	    slash2 = banner.find('/', slash1 + 1);
	    if (slash2 != string::npos)
		version = banner.substr(slash1 + 1, slash2 - slash1 - 1);
	    else
		version = banner.substr(slash1 + 1);

	    dot1 = version.find('.');
	    if (dot1 == string::npos) {
		XLOG_ERROR("Invalid Click version: %s", version.c_str());
		goto error_label;
	    }
	    dot2 = version.find('.', dot1 + 1);
	    major = atoi(version.substr(0, dot1).c_str());
	    if (dot2 != string::npos)
		minor = atoi(version.substr(dot1 + 1, dot2 - dot1 - 1).c_str());
	    else
		minor = atoi(version.substr(dot1 + 1).c_str());
	    if ((major < CLICK_MAJOR_VERSION)
		|| ((major == CLICK_MAJOR_VERSION)
		    && (minor < CLICK_MINOR_VERSION))) {
		XLOG_ERROR("Invalid Click version: expected at least %d.%d "
			   "(found %s)",
			   CLICK_MAJOR_VERSION, CLICK_MINOR_VERSION,
			   version.c_str());
		goto error_label;
	    }
	    break;

	error_label:
	    terminate_user_click_command();
	    comm_close(_fd);
	    _fd = -1;
	    return (XORP_ERROR);
	} while (false);

	//
	// Add the socket to the event loop
	//
	if (_eventloop.add_selector(_fd, SEL_RD,
				    callback(this, &ClickSocket::select_hook))
	    == false) {
	    XLOG_ERROR("Failed to add user-level Click socket to EventLoop");
	    terminate_user_click_command();
	    comm_close(_fd);
	    _fd = -1;
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);
}

int
ClickSocket::stop()
{
    shutdown();
    
    return (XORP_OK);
}

int
ClickSocket::execute_user_click_command(const string& command,
					const string& arguments)
{
    if (_user_click_run_command != NULL)
	return (XORP_ERROR);	// XXX: command is already running

    _user_click_run_command = new RunCommand(_eventloop, command, arguments,
					     callback(this, &ClickSocket::user_click_command_stdout_cb),
					     callback(this, &ClickSocket::user_click_command_stderr_cb),
					     callback(this, &ClickSocket::user_click_command_done_cb));
    if (_user_click_run_command->execute() != XORP_OK) {
	delete _user_click_run_command;
	_user_click_run_command = NULL;
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

void
ClickSocket::terminate_user_click_command()
{
    if (_user_click_run_command != NULL) {
	delete _user_click_run_command;
	_user_click_run_command = NULL;
    }
}

void
ClickSocket::user_click_command_stdout_cb(RunCommand* run_command,
					  const string& output)
{
    XLOG_ASSERT(_user_click_run_command == run_command);
    XLOG_INFO("User-level Click stdout output: %s", output.c_str());
}

void
ClickSocket::user_click_command_stderr_cb(RunCommand* run_command,
					  const string& output)
{
    XLOG_ASSERT(_user_click_run_command == run_command);
    XLOG_ERROR("User-level Click stderr output: %s", output.c_str());
}

void
ClickSocket::user_click_command_done_cb(RunCommand* run_command, bool success,
					const string& error_msg)
{
    XLOG_ASSERT(_user_click_run_command == run_command);
    if (! success) {
	XLOG_ERROR("User-level Click command (%s) failed: %s",
		   run_command->command().c_str(), error_msg.c_str());
    }
    delete _user_click_run_command;
    _user_click_run_command = NULL;
}

void
ClickSocket::shutdown()
{
    if (is_kernel_click()) {
	//
	// TODO: XXX: PAVPAVPAV: Unmount the Click FS, etc.
	//
    }

    if (is_user_click()) {
	terminate_user_click_command();
	if (_fd >= 0) {
	    //
	    // Remove the socket from the event loop and close it
	    //
	    _eventloop.remove_selector(_fd, SEL_ALL);
	    comm_close(_fd);
	    _fd = -1;
	}
    }
}

int
ClickSocket::write_config(const string& handler, const string& data,
			  string& errmsg)
{
    if (is_kernel_click()) {
	//
	// TODO: XXX: PAVPAVPAV: implement it.
	//
    }

    if (is_user_click()) {
	string config = c_format("WRITEDATA %s %u\n",
				 handler.c_str(),
				 static_cast<uint32_t>(data.size()));
	config += data;

	if (ClickSocket::write(config.c_str(), config.size())
	    != static_cast<ssize_t>(config.size())) {
	    errmsg = c_format("Error writing to Click socket: %s",
			      strerror(errno));
	    return (XORP_ERROR);
	}

	//
	// Check the command status
	//
	bool is_warning, is_error;
	string command_warning, command_error;
	if (check_command_status(is_warning, command_warning,
				 is_error, command_error,
				 errmsg) != XORP_OK) {
	    errmsg = c_format("Error verifying the command status after "
			      "writing to Click socket: %s",
			      errmsg.c_str());
	    return (XORP_ERROR);
	}

	if (is_warning) {
	    XLOG_WARNING("Click command warning: %s", command_warning.c_str());
	}
	if (is_error) {
	    errmsg = c_format("Click command error: %s",
			      command_error.c_str());
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);
}

ssize_t
ClickSocket::write(const void* data, size_t nbytes)
{
    _seqno++;
    return ::write(_fd, data, nbytes);
}

int
ClickSocket::check_command_status(bool& is_warning, string& command_warning,
				  bool& is_error, string& command_error,
				  string& errmsg)
{
    vector<uint8_t> buffer;

    is_warning = false;
    is_error = false;

    if (force_read_message(buffer, errmsg) != XORP_OK)
	return (XORP_ERROR);

    //
    // Split the message into lines
    //
    string buffer_str = string(reinterpret_cast<char *>(&buffer[0]));
    list<string> lines;
    do {
	string::size_type idx = buffer_str.find("\n");
	if (idx == string::npos) {
	    if (! buffer_str.empty())
		lines.push_back(buffer_str);
	    break;
	}

	string line = buffer_str.substr(0, idx + 1);
	lines.push_back(line);
	buffer_str = buffer_str.substr(idx + 1);
    } while (true);

    //
    // Parse the message line-by-line
    //
    list<string>::const_iterator iter;
    bool is_last_command_response = false;
    for (iter = lines.begin(); iter != lines.end(); ++iter) {
	const string& line = *iter;
	if (is_last_command_response)
	    break;
	if (line.size() < CLICK_COMMAND_RESPONSE_MIN_SIZE) {
	    errmsg = c_format("Command line response is too short "
			      "(expected min size %u received %u): %s",
			      static_cast<uint32_t>(CLICK_COMMAND_RESPONSE_MIN_SIZE),
			      static_cast<uint32_t>(line.size()),
			      line.c_str());
	    return (XORP_ERROR);
	}

	//
	// Get the error code
	//
	char separator = line[CLICK_COMMAND_RESPONSE_CODE_SEPARATOR_INDEX];
	if ((separator != ' ') && (separator != '-')) {
	    errmsg = c_format("Invalid command line response "
			      "(missing code separator): %s",
			      line.c_str());
	    return (XORP_ERROR);
	}
	int error_code = atoi(line.substr(0, CLICK_COMMAND_RESPONSE_CODE_SEPARATOR_INDEX).c_str());

	if (separator == ' ')
	    is_last_command_response = true;

	//
	// Test the error code
	//
	if (error_code == CLICK_COMMAND_CODE_OK)
	    continue;

	if ((error_code >= CLICK_COMMAND_CODE_WARNING_MIN)
	    && (error_code <= CLICK_COMMAND_CODE_WARNING_MAX)) {
	    is_warning = true;
	    command_warning += line;
	    continue;
	}

	if ((error_code >= CLICK_COMMAND_CODE_ERROR_MIN)
	    && (error_code <= CLICK_COMMAND_CODE_ERROR_MAX)) {
	    is_error = true;
	    command_error += line;
	    continue;
	}

	// Unknown error code
	errmsg = c_format("Unknown error code: %s", line.c_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
ClickSocket::force_read(string& errmsg)
{
    vector<uint8_t> message;

    if (force_read_message(message, errmsg) != XORP_OK)
	return (XORP_ERROR);

    //
    // Notify observers
    //
    for (ObserverList::iterator i = _ol.begin(); i != _ol.end(); i++) {
	(*i)->clsock_data(&message[0], message.size());
    }

    return (XORP_OK);
}

int
ClickSocket::force_read_message(vector<uint8_t>& message, string& errmsg)
{
    vector<uint8_t> buffer(CLSOCK_BYTES);

    for ( ; ; ) {
	ssize_t got;
	// Find how much data is queued in the first message
	do {
	    got = recv(_fd, &buffer[0], buffer.size(),
		       MSG_DONTWAIT | MSG_PEEK);
	    if ((got < 0) && (errno == EINTR))
		continue;	// XXX: the receive was interrupted by a signal
	    if ((got < 0) || (got < (ssize_t)buffer.size()))
		break;		// The buffer is big enough
	    buffer.resize(buffer.size() + CLSOCK_BYTES);
	} while (true);

	got = read(_fd, &buffer[0], buffer.size());
	if (got < 0) {
	    if (errno == EINTR)
		continue;
	    errmsg = c_format("Click socket read error: %s", strerror(errno));
	    return (XORP_ERROR);
	}
	message.resize(got);
	memcpy(&message[0], &buffer[0], got);
	break;
    }

    return (XORP_OK);
}

void
ClickSocket::select_hook(int fd, SelectorMask m)
{
    string errmsg;

    XLOG_ASSERT(fd == _fd);
    XLOG_ASSERT(m == SEL_RD);
    if (force_read(errmsg) != XORP_OK) {
	XLOG_ERROR("Error force_read() from Click socket: %s", errmsg.c_str());
    }
}

//
// Observe Click sockets activity
//

struct ClickSocketPlumber {
    typedef ClickSocket::ObserverList ObserverList;

    static void
    plumb(ClickSocket& r, ClickSocketObserver* o)
    {
	ObserverList& ol = r._ol;
	ObserverList::iterator i = find(ol.begin(), ol.end(), o);
	debug_msg("Plumbing ClickSocketObserver %p to ClickSocket%p\n",
		  o, &r);
	XLOG_ASSERT(i == ol.end());
	ol.push_back(o);
    }
    static void
    unplumb(ClickSocket& r, ClickSocketObserver* o)
    {
	ObserverList& ol = r._ol;
	debug_msg("Unplumbing ClickSocketObserver%p from "
		  "ClickSocket %p\n", o, &r);
	ObserverList::iterator i = find(ol.begin(), ol.end(), o);
	XLOG_ASSERT(i != ol.end());
	ol.erase(i);
    }
};

ClickSocketObserver::ClickSocketObserver(ClickSocket& cs)
    : _cs(cs)
{
    ClickSocketPlumber::plumb(cs, this);
}

ClickSocketObserver::~ClickSocketObserver()
{
    ClickSocketPlumber::unplumb(_cs, this);
}

ClickSocket&
ClickSocketObserver::click_socket()
{
    return _cs;
}

ClickSocketReader::ClickSocketReader(ClickSocket& cs)
    : ClickSocketObserver(cs),
      _cs(cs),
      _cache_valid(false),
      _cache_seqno(0)
{

}

ClickSocketReader::~ClickSocketReader()
{

}

/**
 * Force the reader to receive data from the specified Click socket.
 *
 * @param cs the Click socket to receive the data from.
 * @param seqno the sequence number of the data to receive.
 * @param errmsg the error message (if an error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
ClickSocketReader::receive_data(ClickSocket& cs, uint32_t seqno,
				string& errmsg)
{
    _cache_seqno = seqno;
    _cache_valid = false;
    while (_cache_valid == false) {
	if (cs.force_read(errmsg) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * Receive data from the Click socket.
 *
 * Note that this method is called asynchronously when the Click socket
 * has data to receive, therefore it should never be called directly by
 * anything else except the Click socket facility itself.
 *
 * @param data the buffer with the received data.
 * @param nbytes the number of bytes in the @param data buffer.
 */
void
ClickSocketReader::clsock_data(const uint8_t* data, size_t nbytes)
{
    //
    // Copy data that has been requested to be cached
    //
    _cache_data = string(reinterpret_cast<const char*>(data), nbytes);
    // memcpy(&_cache_data[0], data, nbytes);
    _cache_valid = true;
}
