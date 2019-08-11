// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2012 XORP, Inc and Others
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

#include <xorp_config.h>
#ifdef XORP_USE_CLICK


#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/run_command.hh"
#include "libxorp/utils.hh"



#ifdef HAVE_SYS_LINKER_H
#include <sys/linker.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

#include "libcomm/comm_api.h"

#include "click_socket.hh"


const string ClickSocket::PROC_LINUX_MODULES_FILE = "/proc/modules";
const string ClickSocket::LINUX_COMMAND_LOAD_MODULE = "/sbin/insmod";
const string ClickSocket::LINUX_COMMAND_UNLOAD_MODULE = "/sbin/rmmod";
const string ClickSocket::CLICK_FILE_SYSTEM_TYPE = "click";

uint16_t ClickSocket::_instance_cnt = 0;
pid_t ClickSocket::_pid = getpid();

const TimeVal ClickSocket::USER_CLICK_STARTUP_MAX_WAIT_TIME = TimeVal(1, 0);

//
// Click Sockets communication with Click
//

ClickSocket::ClickSocket(EventLoop& eventloop)
    : _eventloop(eventloop),
      _seqno(0),
      _instance_no(_instance_cnt++),
      _is_enabled(false),
      _duplicate_routes_to_kernel(false),
      _is_kernel_click(false),
      _is_user_click(false),
      _kernel_click_install_on_startup(false),
      _user_click_command_execute_on_startup(false),
      _user_click_control_address(DEFAULT_USER_CLICK_CONTROL_ADDRESS()),
      _user_click_control_socket_port(DEFAULT_USER_CLICK_CONTROL_SOCKET_PORT),
      _user_click_run_command(NULL)
{

}

ClickSocket::~ClickSocket()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the Click socket: %s", error_msg.c_str());
    }

    XLOG_ASSERT(_ol.empty());
}

int
ClickSocket::start(string& error_msg)
{
    if (is_kernel_click() && !_kernel_fd.is_valid()) {
	//
	// Install kernel Click (if necessary)
	//
	if (_kernel_click_install_on_startup) {
	    string error_msg2;

	    // Load the kernel Click modules
	    if (load_kernel_click_modules(error_msg) != XORP_OK) {
		unload_kernel_click_modules(error_msg2);
		return (XORP_ERROR);
	    }

	    // Mount the Click file system
	    if (mount_click_file_system(error_msg) != XORP_OK) {
		unload_kernel_click_modules(error_msg2);
		return (XORP_ERROR);
	    }
	}

#ifdef O_NONBLOCK
	//
	// Open the Click error file (for reading error messages)
	//
	string click_error_filename;
	click_error_filename = _kernel_click_mount_directory + "/errors";
	_kernel_fd = open(click_error_filename.c_str(), O_RDONLY | O_NONBLOCK);
	if (!_kernel_fd.is_valid()) {
	    error_msg = c_format("Cannot open kernel Click error file %s: %s",
				 click_error_filename.c_str(),
				 strerror(errno));
	    return (XORP_ERROR);
	}
#endif
    }

    if (is_user_click() && !_user_fd.is_valid()) {
	//
	// Execute the Click command (if necessary)
	//
	if (_user_click_command_execute_on_startup) {
	    // Compose the command and the arguments
	    string command = _user_click_command_file;
	    list<string> argument_list;
	    argument_list.push_back("-f");
	    argument_list.push_back(_user_click_startup_config_file);
	    argument_list.push_back("-p");
	    argument_list.push_back(c_format("%u",
					 _user_click_control_socket_port));
	    if (! _user_click_command_extra_arguments.empty()) {
		list<string> l = split(_user_click_command_extra_arguments,
				       ' ');
		argument_list.insert(argument_list.end(), l.begin(), l.end());
	    }

	    if (execute_user_click_command(command, argument_list)
		!= XORP_OK) {
		error_msg = c_format("Could not execute the user-level Click");
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
	TimeVal max_wait_time = USER_CLICK_STARTUP_MAX_WAIT_TIME;
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
	    int in_progress = 0;
	    _user_fd = comm_connect_tcp4(&in_addr,
					 htons(_user_click_control_socket_port),
					 COMM_SOCK_BLOCKING, &in_progress, NULL);
	    if (_user_fd.is_valid())
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
	    error_msg = c_format("Could not open user-level Click socket: %s",
				 strerror(errno));
	    terminate_user_click_command();
	    return (XORP_ERROR);
	} while (true);

	//
	// Read the expected banner
	//
	vector<uint8_t> message;
	string error_msg2;
	if (force_read_message(_user_fd, message, error_msg2) != XORP_OK) {
	    error_msg = c_format("Could not read on startup from user-level "
				 "Click socket: %s", error_msg2.c_str());
	    terminate_user_click_command();
	    comm_close(_user_fd);
	    _user_fd.clear();
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
		error_msg = c_format("Invalid user-level Click banner: %s",
				     banner.c_str());
		goto error_label;
	    }
	    slash2 = banner.find('/', slash1 + 1);
	    if (slash2 != string::npos)
		version = banner.substr(slash1 + 1, slash2 - slash1 - 1);
	    else
		version = banner.substr(slash1 + 1);

	    dot1 = version.find('.');
	    if (dot1 == string::npos) {
		error_msg = c_format("Invalid user-level Click version: %s",
				     version.c_str());
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
		error_msg = c_format("Invalid user-level Click version: "
				     "expected at least %d.%d "
				     "(found %s)",
			   CLICK_MAJOR_VERSION, CLICK_MINOR_VERSION,
			   version.c_str());
		goto error_label;
	    }
	    break;

	error_label:
	    terminate_user_click_command();
	    comm_close(_user_fd);
	    _user_fd.clear();
	    return (XORP_ERROR);
	} while (false);

	//
	// Add the socket to the event loop
	//
	if (_eventloop.add_ioevent_cb(_user_fd, IOT_READ,
				    callback(this, &ClickSocket::io_event))
	    == false) {
	    error_msg = c_format("Failed to add user-level Click socket "
				 "to EventLoop");
	    terminate_user_click_command();
	    comm_close(_user_fd);
	    _user_fd.clear();
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);
}

int
ClickSocket::stop(string& error_msg)
{
#ifdef HOST_OS_WINDOWS
    UNUSED(error_msg)
    return (XORP_ERROR);
#else /* !HOST_OS_WINDOWS */
    //
    // XXX: First we should stop user-level Click, and then kernel-level Click.
    // Otherwise, the user-level Click process may block the unmounting
    // of the kernel-level Click file system. The reason for this blocking
    // is unknown...
    //
    if (is_user_click()) {
	terminate_user_click_command();
	if (_user_fd >= 0) {
	    //
	    // Remove the socket from the event loop and close it
	    //
	    _eventloop.remove_ioevent_cb(_user_fd);
	    comm_close(_user_fd);
	    _user_fd.clear();
	}
    }


    if (is_kernel_click()) {
	//
	// Close the Click error file (for reading error messages)
	//
	if (_kernel_fd.is_valid()) {
	    close(_kernel_fd);
	    _kernel_fd.clear();
	}
	if (unmount_click_file_system(error_msg) != XORP_OK) {
	    string error_msg2;
	    unload_kernel_click_modules(error_msg2);
	    return (XORP_ERROR);
	}
	if (unload_kernel_click_modules(error_msg) != XORP_OK) {
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);
#endif /* HOST_OS_WINDOWS */
}

int
ClickSocket::load_kernel_click_modules(string& error_msg)
{
    list<string>::iterator iter;

    for (iter = _kernel_click_modules.begin();
	 iter != _kernel_click_modules.end();
	 ++iter) {
	const string& module_filename = *iter;
	if (load_kernel_module(module_filename, error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
ClickSocket::unload_kernel_click_modules(string& error_msg)
{
    list<string>::reverse_iterator riter;

    for (riter = _kernel_click_modules.rbegin();
	 riter != _kernel_click_modules.rend();
	 ++riter) {
	const string& module_filename = *riter;
	if (unload_kernel_module(module_filename, error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
ClickSocket::load_kernel_module(const string& module_filename,
				string& error_msg)
{
    if (module_filename.empty()) {
	error_msg = c_format("Kernel module filename to load is empty");
	return (XORP_ERROR);
    }

    if (find(_loaded_kernel_click_modules.begin(),
	     _loaded_kernel_click_modules.end(),
	     module_filename)
	!= _loaded_kernel_click_modules.end()) {
	return (XORP_OK);	// Module already loaded
    }

    string module_name = kernel_module_filename2modulename(module_filename);
    if (module_name.empty()) {
	error_msg = c_format("Invalid kernel module filename: %s",
			     module_filename.c_str());
	return (XORP_ERROR);
    }

    //
    // Load the kernel module using system-specific mechanism
    //

#ifdef HOST_OS_FREEBSD
    //
    // Test if the kernel module was installed already
    //
    if (kldfind(module_name.c_str()) >= 0) {
	return (XORP_OK);	// Module with the same name already loaded
    }

    //
    // Load the kernel module
    //
    if (kldload(module_filename.c_str()) < 0) {
	error_msg = c_format("Cannot load kernel module %s: %s",
			     module_filename.c_str(), strerror(errno));
	return (XORP_ERROR);
    }

    _loaded_kernel_click_modules.push_back(module_filename);

    return (XORP_OK);

#endif // HOST_OS_FREEBSD

#ifdef HOST_OS_LINUX
    //
    // Test if the kernel module was installed already
    //
    char buf[1024];
    char name[1024];

    FILE* fh = fopen(PROC_LINUX_MODULES_FILE.c_str(), "r");
    if (fh == NULL) {
	error_msg = c_format("Cannot open file %s for reading: %s",
			     PROC_LINUX_MODULES_FILE.c_str(), strerror(errno));
	return (XORP_ERROR);
    }
    while (fgets(buf, sizeof(buf), fh) != NULL) {
	char* n = name;
	char* p = buf;
	char* s = NULL;

	// Get the module name: the first word in the line
	do {
	    while (xorp_isspace(*p))
		p++;
	    if (*p == '\0') {
		s = NULL;
		break;
	    }

	    while (*p) {
		if (xorp_isspace(*p))
		    break;
		*n++ = *p++;
	    }
	    *n++ = '\0';
	    s = p;
	    break;
	} while (true);

	if (s == NULL) {
	    XLOG_ERROR("%s: cannot get module name for line %s",
		       PROC_LINUX_MODULES_FILE.c_str(), buf);
	    continue;
	}
	if (module_name == string(name)) {
	    fclose(fh);
	    return (XORP_OK);	// Module with the same name already loaded
	}
    }
    fclose(fh);

    //
    // Load the kernel module
    //
    // XXX: unfortunately, Linux doesn't have a consistent system API
    // for loading kernel modules, so we have to relay on user-land command
    // to do this. Sigh...
    //
    string command_line = LINUX_COMMAND_LOAD_MODULE + " " + module_filename;
    int ret_value = system(command_line.c_str());
    if (ret_value != 0) {
	if (ret_value < 0) {
	    error_msg = c_format("Cannot execute system command '%s': %s",
				 command_line.c_str(), strerror(errno));
	} else {
	    error_msg = c_format("Executing system command '%s' "
				 "returned value '%d'",
				 command_line.c_str(), ret_value);
	}
	return (XORP_ERROR);
    }

    _loaded_kernel_click_modules.push_back(module_filename);

    return (XORP_OK);

#endif // HOST_OS_LINUX

#ifdef HOST_OS_MACOSX
    // TODO: implement it
    error_msg = c_format("No mechanism to load a kernel module");
    return (XORP_ERROR);
#endif // HOST_OS_MACOSX

#ifdef HOST_OS_NETBSD
    // TODO: implement it
    error_msg = c_format("No mechanism to load a kernel module");
    return (XORP_ERROR);
#endif // HOST_OS_NETBSD

#ifdef HOST_OS_OPENBSD
    // TODO: implement it
    error_msg = c_format("No mechanism to load a kernel module");
    return (XORP_ERROR);
#endif // HOST_OS_OPENBSD

#ifdef HOST_OS_SOLARIS
    // TODO: implement it
    error_msg = c_format("No mechanism to load a kernel module");
    return (XORP_ERROR);
#endif // HOST_OS_SOLARIS

    error_msg = c_format("No mechanism to load a kernel module");
    return (XORP_ERROR);
}

int
ClickSocket::unload_kernel_module(const string& module_filename,
				  string& error_msg)
{
    if (module_filename.empty()) {
	error_msg = c_format("Kernel module filename to unload is empty");
	return (XORP_ERROR);
    }

    if (find(_loaded_kernel_click_modules.begin(),
	     _loaded_kernel_click_modules.end(),
	     module_filename)
	== _loaded_kernel_click_modules.end()) {
	return (XORP_OK);	// Module not loaded
    }

    string module_name = kernel_module_filename2modulename(module_filename);
    if (module_name.empty()) {
	error_msg = c_format("Invalid kernel module filename: %s",
			     module_filename.c_str());
	return (XORP_ERROR);
    }

    //
    // Unload the kernel module using system-specific mechanism
    //

#ifdef HOST_OS_FREEBSD
    //
    // Find the kernel module ID.
    //
    int module_id = kldfind(module_name.c_str());
    if (module_id < 0) {
	error_msg = c_format("Cannot unload kernel module %s: "
			     "module ID not found: %s",
			     module_filename.c_str(), strerror(errno));
	return (XORP_ERROR);
    }

    //
    // Unload the kernel module
    //
    if (kldunload(module_id) < 0) {
	error_msg = c_format("Cannot unload kernel module %s: %s",
			     module_filename.c_str(), strerror(errno));
	return (XORP_ERROR);
    }

    // Remove the module filename from the list of loaded modules
    list<string>::iterator iter;
    iter = find(_loaded_kernel_click_modules.begin(),
		_loaded_kernel_click_modules.end(),
		module_filename);
    XLOG_ASSERT(iter != _loaded_kernel_click_modules.end());
    _loaded_kernel_click_modules.erase(iter);

    return (XORP_OK);

#endif // HOST_OS_FREEBSD

#ifdef HOST_OS_LINUX
    //
    // Unload the kernel module
    //
    // XXX: unfortunately, Linux doesn't have a consistent system API
    // for loading kernel modules, so we have to relay on user-land command
    // to do this. Sigh...
    //
    string command_line = LINUX_COMMAND_UNLOAD_MODULE + " " + module_name;
    int ret_value = system(command_line.c_str());
    if (ret_value != 0) {
	if (ret_value < 0) {
	    error_msg = c_format("Cannot execute system command '%s': %s",
				 command_line.c_str(), strerror(errno));
	} else {
	    error_msg = c_format("Executing system command '%s' "
				 "returned value '%d'",
				 command_line.c_str(), ret_value);
	}
	return (XORP_ERROR);
    }

    // Remove the module filename from the list of loaded modules
    list<string>::iterator iter;
    iter = find(_loaded_kernel_click_modules.begin(),
		_loaded_kernel_click_modules.end(),
		module_filename);
    XLOG_ASSERT(iter != _loaded_kernel_click_modules.end());
    _loaded_kernel_click_modules.erase(iter);

    return (XORP_OK);

#endif // HOST_OS_LINUX

#ifdef HOST_OS_MACOSX
    // TODO: implement it
    error_msg = c_format("No mechanism to unload a kernel module");
    return (XORP_ERROR);
#endif // HOST_OS_MACOSX

#ifdef HOST_OS_NETBSD
    // TODO: implement it
    error_msg = c_format("No mechanism to unload a kernel module");
    return (XORP_ERROR);
#endif // HOST_OS_NETBSD

#ifdef HOST_OS_OPENBSD
    // TODO: implement it
    error_msg = c_format("No mechanism to unload a kernel module");
    return (XORP_ERROR);
#endif // HOST_OS_OPENBSD

#ifdef HOST_OS_SOLARIS
    // TODO: implement it
    error_msg = c_format("No mechanism to unload a kernel module");
    return (XORP_ERROR);
#endif // HOST_OS_SOLARIS

    error_msg = c_format("No mechanism to unload a kernel module");
    return (XORP_ERROR);
}

string
ClickSocket::kernel_module_filename2modulename(const string& module_filename)
{
    string filename, module_name;
    string::size_type slash, dot;
    list<string> suffix_list;

    // Find the file name after the last '/'
    slash = module_filename.rfind('/');
    if (slash == string::npos)
	filename = module_filename;
    else
	filename = module_filename.substr(slash + 1);

    //
    // Find the module name by excluding the suffix after the last '.'
    // if that is a well-known suffix (e.g., ".o" or ".ko").
    //
    suffix_list.push_back(".o");
    suffix_list.push_back(".ko");
    module_name = filename;
    list<string>::iterator iter;
    for (iter = suffix_list.begin(); iter != suffix_list.end(); ++iter) {
	string suffix = *iter;
	dot = filename.rfind(suffix);
	if (dot != string::npos) {
	    if (filename.substr(dot) == suffix) {
		module_name = filename.substr(0, dot);
		break;
	    }
	}
    }

    return (module_name);
}

int
ClickSocket::mount_click_file_system(string& error_msg)
{
#if defined(HOST_OS_WINDOWS)
    // Whilst Cygwin has a mount(), it is very different.
    // Windows itself has no mount().
    UNUSED(error_msg);
    return (XORP_ERROR);
#else
    if (_kernel_click_mount_directory.empty()) {
	error_msg = c_format("Kernel Click mount directory is empty");
	return (XORP_ERROR);
    }

    if (! _mounted_kernel_click_mount_directory.empty()) {
	if (_kernel_click_mount_directory
	    == _mounted_kernel_click_mount_directory) {
	    return (XORP_OK);	// Directory already mounted
	}

	error_msg = c_format("Cannot mount Click file system on directory %s: "
			     "Click file system already mounted on "
			     "directory %s",
			     _kernel_click_mount_directory.c_str(),
			     _mounted_kernel_click_mount_directory.c_str());
	return (XORP_ERROR);
    }

    //
    // Test if the Click file system has been installed already.
    //
    // We do this by tesing whether we can access a number of Click files
    // within the Click file system.
    //
    list<string> click_files;
    list<string>::iterator iter;
    size_t files_found = 0;

    click_files.push_back("/config");
    click_files.push_back("/flatconfig");
    click_files.push_back("/packages");
    click_files.push_back("/version");

    for (iter = click_files.begin(); iter != click_files.end(); ++iter) {
	string click_filename = _kernel_click_mount_directory + *iter;
	if (access(click_filename.c_str(), R_OK) == 0) {
	    files_found++;
	}
    }
    if (files_found > 0) {
	if (files_found == click_files.size()) {
	    return (XORP_OK);	// Directory already mounted
	}
	error_msg = c_format("Click file system mount directory contains "
			     "some Click files");
	return (XORP_ERROR);
    }

    //
    // XXX: Linux has different mount(2) API, hence we need to take
    // care of this.
    //
    int ret_value = -1;
#ifdef HOST_OS_LINUX
    ret_value = mount("none", _kernel_click_mount_directory.c_str(),
		      CLICK_FILE_SYSTEM_TYPE.c_str(), 0, 0);
#elif defined(__NetBSD__) && __NetBSD_Version__ >= 499002400
    ret_value = mount(CLICK_FILE_SYSTEM_TYPE.c_str(),
		      _kernel_click_mount_directory.c_str(), 0, 0, 0);
#else
    ret_value = mount(CLICK_FILE_SYSTEM_TYPE.c_str(),
		      _kernel_click_mount_directory.c_str(), 0, 0);
#endif // ! HOST_OS_LINUX

    if (ret_value != 0) {
	error_msg = c_format("Cannot mount Click file system "
			     "on directory %s: %s",
			     _kernel_click_mount_directory.c_str(),
			     strerror(errno));
	return (XORP_ERROR);
    }

    _mounted_kernel_click_mount_directory = _kernel_click_mount_directory;

    return (XORP_OK);
#endif // HOST_OS_WINDOWS
}

int
ClickSocket::unmount_click_file_system(string& error_msg)
{
#ifdef HOST_OS_WINDOWS
    UNUSED(error_msg);
    return (XORP_OK);
#else
    if (_mounted_kernel_click_mount_directory.empty())
	return (XORP_OK);	// Directory not mounted

    //
    // XXX: Linux and Solaris don't have unmount(2). Instead, they have
    // umount(2), hence we need to take care of this.
    //
    int ret_value = -1;
#if defined(HOST_OS_LINUX) || defined(HOST_OS_SOLARIS)
    ret_value = umount(_mounted_kernel_click_mount_directory.c_str());
#else
    ret_value = unmount(_mounted_kernel_click_mount_directory.c_str(), 0);
#endif

    if (ret_value != 0) {
	error_msg = c_format("Cannot unmount Click file system "
			     "from directory %s: %s",
			     _mounted_kernel_click_mount_directory.c_str(),
			     strerror(errno));
	return (XORP_ERROR);
    }

    _mounted_kernel_click_mount_directory.erase();

    return (XORP_OK);
#endif // HOST_OS_WINDOWS
}

int
ClickSocket::execute_user_click_command(const string& command,
					const list<string>& argument_list)
{
    if (_user_click_run_command != NULL)
	return (XORP_ERROR);	// XXX: command is already running

    _user_click_run_command = new RunCommand(_eventloop,
					     command,
					     argument_list,
					     callback(this, &ClickSocket::user_click_command_stdout_cb),
					     callback(this, &ClickSocket::user_click_command_stderr_cb),
					     callback(this, &ClickSocket::user_click_command_done_cb),
					     false /* redirect_stderr_to_stdout */);
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
	//
	// XXX: if error_msg is empty, the real error message has been
	// printed already when it was received on stderr.
	//
	string msg = c_format("User-level Click command (%s) failed",
			      run_command->command().c_str());
	if (error_msg.size())
	    msg += c_format(": %s", error_msg.c_str());
	XLOG_ERROR("%s", msg.c_str());
    }
    delete _user_click_run_command;
    _user_click_run_command = NULL;
}

int
ClickSocket::write_config(const string& element, const string& handler,
			  bool has_kernel_config, const string& kernel_config,
			  bool has_user_config, const string& user_config,
			  string& error_msg)
{
#ifdef HOST_OS_WINDOWS
    UNUSED(element);
    UNUSED(handler);
    UNUSED(has_kernel_config);
    UNUSED(kernel_config);
    UNUSED(has_user_config);
    UNUSED(user_config);
    UNUSED(error_msg);
    return (0);
#else /* !HOST_OS_WINDOWS */
    if (is_kernel_click() && has_kernel_config) {
	//
	// Prepare the output handler name
	//
	string output_handler = element;
	if (! output_handler.empty())
	    output_handler += "/" + handler;
	else
	    output_handler = handler;
	output_handler = _kernel_click_mount_directory + "/" + output_handler;


	//
	// Prepare the socket to write the configuration
	//
	int openflags = O_WRONLY | O_TRUNC;
#ifdef O_FSYNC
	openflags |= O_FSYNC;
#endif
	XorpFd fd = ::open(output_handler.c_str(), openflags);
	if (!fd.is_valid()) {
	    error_msg = c_format("Cannot open kernel Click handler '%s' "
				 "for writing: %s",
				 output_handler.c_str(), strerror(errno));
	    return (XORP_ERROR);
	}

	//
	// Write the configuration
	//
	if (::write(fd, kernel_config.c_str(), kernel_config.size())
	    != static_cast<ssize_t>(kernel_config.size())) {
	    error_msg = c_format("Error writing to kernel Click "
				 "handler '%s': %s",
				 output_handler.c_str(), strerror(errno));
	    return (XORP_ERROR);
	}
	// XXX: we must close the socket before checking the result
	int close_ret_value = close(fd);

	//
	// Check the command status
	//
	char error_buf[8 * 1024];
	int error_bytes;
	error_bytes = ::read(_kernel_fd, error_buf, sizeof(error_buf));
	if (error_bytes < 0) {
	    error_msg = c_format("Error verifying the command status after "
				 "writing to kernel Click handler: %s",
				 strerror(errno));
	    return (XORP_ERROR);
	}
	if (error_bytes > 0) {
	    error_msg = c_format("Kernel Click command error: %s", error_buf);
	    return (XORP_ERROR);
	}
	if (close_ret_value < 0) {
	    //
	    // XXX: If the close() system call returned an error, this is
	    // an indication that the write to the Click handler failed.
	    //
	    // Note that typically we should have caught any errors by
	    // reading the "<click>/errors" handler, so the check for
	    // the return value of close() is the last line of defense.
	    //
	    // The errno should be EINVAL, but we are liberal about this
	    // additional check.
	    //
	    error_msg = c_format("Kernel Click command error: unknown");
	    return (XORP_ERROR);
	}
    }

    if (is_user_click() && has_user_config) {
	//
	// Prepare the output handler name
	//
	string output_handler = element;
	if (! output_handler.empty())
	    output_handler += "." + handler;
	else
	    output_handler = handler;

	//
	// Prepare the configuration to write
	//
	string config = c_format("WRITEDATA %s %u\n",
				 output_handler.c_str(),
				 XORP_UINT_CAST(user_config.size()));
	config += user_config;

	//
	// Write the configuration
	//
	if (ClickSocket::write(_user_fd, config.c_str(), config.size())
	    != static_cast<ssize_t>(config.size())) {
	    error_msg = c_format("Error writing to user-level "
				 "Click socket: %s",
				 strerror(errno));
	    return (XORP_ERROR);
	}

	//
	// Check the command status
	//
	bool is_warning, is_error;
	string command_warning, command_error;
	if (check_user_command_status(is_warning, command_warning,
				      is_error, command_error,
				      error_msg)
	    != XORP_OK) {
	    error_msg = c_format("Error verifying the command status after "
				 "writing to user-level Click socket: %s",
				 error_msg.c_str());
	    return (XORP_ERROR);
	}

	if (is_warning) {
	    XLOG_WARNING("User-level Click command warning: %s",
			 command_warning.c_str());
	}
	if (is_error) {
	    error_msg = c_format("User-level Click command error: %s",
				 command_error.c_str());
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);
#endif /* HOST_OS_WINDOWS */
}

ssize_t
ClickSocket::write(XorpFd fd, const void* data, size_t nbytes)
{
#ifdef HOST_OS_WINDOWS
    UNUSED(fd);
    UNUSED(data);
    UNUSED(nbytes);
    return (0);
#else
    _seqno++;
    return ::write(fd, data, nbytes);
#endif
}

int
ClickSocket::check_user_command_status(bool& is_warning,
				       string& command_warning,
				       bool& is_error,
				       string& command_error,
				       string& error_msg)
{
    vector<uint8_t> buffer;

    is_warning = false;
    is_error = false;

    if (force_read_message(_user_fd, buffer, error_msg) != XORP_OK)
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
	    error_msg = c_format(
		"User-level Click command line response is too short "
		"(expected min size %u received %u): %s",
		XORP_UINT_CAST(CLICK_COMMAND_RESPONSE_MIN_SIZE),
		XORP_UINT_CAST(line.size()),
		line.c_str());
	    return (XORP_ERROR);
	}

	//
	// Get the error code
	//
	char separator = line[CLICK_COMMAND_RESPONSE_CODE_SEPARATOR_INDEX];
	if ((separator != ' ') && (separator != '-')) {
	    error_msg = c_format("Invalid user-level Click command line "
				 "response (missing code separator): %s",
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
	error_msg = c_format("Unknown user-level Click error code: %s",
			     line.c_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
ClickSocket::force_read(XorpFd fd, string& error_msg)
{
    vector<uint8_t> message;

    if (force_read_message(fd, message, error_msg) != XORP_OK)
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
ClickSocket::force_read_message(XorpFd fd, vector<uint8_t>& message,
				string& error_msg)
{
#ifdef HOST_OS_WINDOWS
    UNUSED(fd);
    UNUSED(message);
    UNUSED(error_msg);
    return (XORP_ERROR);
#else /* !HOST_OS_WINDOWS */
    vector<uint8_t> buffer(CLSOCK_BYTES);

    for ( ; ; ) {
	ssize_t got;
	// Find how much data is queued in the first message
	do {
	    got = recv(fd, XORP_BUF_CAST(&buffer[0]),
		       buffer.size(),
#ifdef MSG_DONTWAIT
		       MSG_DONTWAIT |
#endif
		       MSG_PEEK);
	    if ((got < 0) && (errno == EINTR))
		continue;	// XXX: the receive was interrupted by a signal
	    if ((got < 0) || (got < (ssize_t)buffer.size()))
		break;		// The buffer is big enough
	    buffer.resize(buffer.size() + CLSOCK_BYTES);
	} while (true);

	got = read(fd, &buffer[0], buffer.size());
	if (got < 0) {
	    if (errno == EINTR)
		continue;
	    error_msg = c_format("Click socket read error: %s",
				 strerror(errno));
	    return (XORP_ERROR);
	}
	message.resize(got);
	memcpy(&message[0], &buffer[0], got);
	break;
    }

    return (XORP_OK);
#endif /* HOST_OS_WINDOWS */
}

void
ClickSocket::io_event(XorpFd fd, IoEventType type)
{
    string error_msg;

    XLOG_ASSERT((fd == _kernel_fd) || (fd == _user_fd));
    XLOG_ASSERT(type == IOT_READ);
    if (force_read(fd, error_msg) != XORP_OK) {
	XLOG_ERROR("Error force_read() from Click socket: %s",
		   error_msg.c_str());
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
	debug_msg("Unplumbing ClickSocketObserver %p from "
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
 * Force the reader to receive kernel-level Click data from the specified
 * Click socket.
 *
 * @param cs the Click socket to receive the data from.
 * @param seqno the sequence number of the data to receive.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
ClickSocketReader::receive_kernel_click_data(ClickSocket& cs, uint32_t seqno,
					     string& error_msg)
{
    _cache_seqno = seqno;
    _cache_valid = false;
    while (_cache_valid == false) {
	if (cs.force_kernel_click_read(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * Force the reader to receive user-level Click data from the specified
 * Click socket.
 *
 * @param cs the Click socket to receive the data from.
 * @param seqno the sequence number of the data to receive.
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
ClickSocketReader::receive_user_click_data(ClickSocket& cs, uint32_t seqno,
					   string& error_msg)
{
    _cache_seqno = seqno;
    _cache_valid = false;
    while (_cache_valid == false) {
	if (cs.force_user_click_read(error_msg) != XORP_OK)
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


#endif //click
