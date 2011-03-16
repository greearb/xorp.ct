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

// $XORP: xorp/fea/data_plane/control_socket/click_socket.hh,v 1.6 2008/10/02 21:56:53 bms Exp $

#ifndef __FEA_DATA_PLANE_CONTROL_SOCKET_CLICK_SOCKET_HH__
#define __FEA_DATA_PLANE_CONTROL_SOCKET_CLICK_SOCKET_HH__

#include <xorp_config.h>
#ifdef XORP_USE_CLICK



#include "libxorp/eventloop.hh"
#include "libxorp/exceptions.hh"
#include "libxorp/ipvx.hh"

class ClickSocketObserver;
struct ClickSocketPlumber;
class RunCommand;

/**
 * ClickSocket class opens a Click socket and forwards data arriving
 * on the socket to ClickSocketObservers.  The ClickSocket hooks itself
 * into the EventLoop and activity usually happens asynchronously.
 */
class ClickSocket :
    public NONCOPYABLE
{
public:
    ClickSocket(EventLoop& eventloop);
    ~ClickSocket();

    /**
     * Enable/disable Click support.
     *
     * Note that this operation does not start the Click operations.
     *
     * @param v if true, then enable Click support, otherwise disable it.
     */
    void enable_click(bool v) { _is_enabled = v; }

    /**
     * Test if the Click support is enabled.
     *
     * @return true if the Click support is enabled, otherwise false.
     */
    bool is_enabled() const { return (_is_enabled); }

    /**
     * Test if duplicating the Click routes to the system kernel is enabled.
     *
     * @return true if duplicating the Click routes to the system kernel is
     * enabled, otherwise false.
     */
    bool is_duplicate_routes_to_kernel_enabled() const { return _duplicate_routes_to_kernel; }

    /**
     * Enable/disable duplicating the Click routes to the system kernel.
     *
     * @param enable if true, then enable duplicating the Click routes to the
     * system kernel, otherwise disable it.
     */
    void enable_duplicate_routes_to_kernel(bool v) { _duplicate_routes_to_kernel = v; }

    /**
     * Enable/disable kernel-level Click.
     * 
     * Note that this operation does not start the kernel-level Click
     * operations.
     * 
     * @param v if true, enable kernel-level Click, otherwise disable it.
     */
    void enable_kernel_click(bool v) { _is_kernel_click = v; }

    /**
     * Enable/disable installing kernel-level Click on startup.
     *
     * @param v if true, then install kernel-level Click on startup.
     */
    void enable_kernel_click_install_on_startup(bool v) {
	_kernel_click_install_on_startup = v;
    }

    /**
     * Specify the list of kernel Click modules to load on startup if
     * installing kernel-level Click on startup is enabled.
     *
     * @param v the list of kernel Click modules to load.
     */
    void set_kernel_click_modules(const list<string>& v) {
	_kernel_click_modules = v;
    }

    /**
     * Specify the kernel-level Click mount directory.
     *
     * @param v the kernel-level Click mount directory.
     */
    void set_kernel_click_mount_directory(const string& v) {
	_kernel_click_mount_directory = v;
    }

    /**
     * Enable/disable user-level Click.
     * 
     * Note that this operation does not start the user-level Click
     * operations.
     * 
     * @param v if true, enable user-level Click, otherwise disable it.
     */
    void enable_user_click(bool v) { _is_user_click = v; }

    /**
     * Test if we are running kernel-level Click operations.
     *
     * @return true if kernel-level Click is enabled, otherwise false.
     */
    bool is_kernel_click() const { return _is_kernel_click; }

    /**
     * Test if we are running user-level Click operations.
     *
     * @return true if user-level Click is enabled, otherwise false.
     */
    bool is_user_click() const { return _is_user_click; }

    /**
     * Start the Click socket operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int start(string& error_msg);

    /**
     * Stop the Click socket operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int stop(string& error_msg);

    /**
     * Get the name of the external program to generate the kernel-level Click
     * configuration.
     *
     * @return the name of the external program to generate the kernel-level
     * Click configuration.
     */
    const string& kernel_click_config_generator_file() const {
	return (_kernel_click_config_generator_file);
    }

    /**
     * Specify the external program to generate the kernel-level Click
     * configuration.
     *
     * @param v the name of the external program to generate the kernel-level
     * Click configuration.
     */
    void set_kernel_click_config_generator_file(const string& v) {
	_kernel_click_config_generator_file = v;
    }

    /**
     * Specify the user-level Click command file.
     *
     * @param v the name of the user-level Click command file.
     */
    void set_user_click_command_file(const string& v) {
	_user_click_command_file = v;
    }

    /**
     * Specify the extra arguments to the user-level Click command.
     *
     * @param v the extra arguments to the user-level Click command.
     */
    void set_user_click_command_extra_arguments(const string& v) {
	_user_click_command_extra_arguments = v;
    }

    /**
     * Specify whether to execute on startup the user-level Click command.
     *
     * @param v if true, then execute the user-level Click command on startup.
     */
    void set_user_click_command_execute_on_startup(bool v) {
	_user_click_command_execute_on_startup = v;
    }

    /**
     * Specify the configuration file to be used by user-level Click on
     * startup.
     *
     * @param v the name of the configuration file to be used by user-level
     * Click on startup.
     */
    void set_user_click_startup_config_file(const string& v) {
	_user_click_startup_config_file = v;
    }

    /**
     * Specify the address to use for control access to the user-level
     * Click.
     *
     * @param v the address to use for control access to the user-level
     * Click.
     */
    void set_user_click_control_address(const IPv4& v) {
	_user_click_control_address = v;
    }

    /**
     * Specify the socket port to use for control access to the user-level
     * Click.
     *
     * @param v the socket port to use for control access to the user-level
     * Click.
     */
    void set_user_click_control_socket_port(uint16_t v) {
	_user_click_control_socket_port = v;
    }

    /**
     * Get the name of the external program to generate the user-level Click
     * configuration.
     *
     * @return the name of the external program to generate the user-level
     * Click configuration.
     */
    const string& user_click_config_generator_file() const {
	return (_user_click_config_generator_file);
    }

    /**
     * Specify the external program to generate the user-level Click
     * configuration.
     *
     * @param v the name of the external program to generate the user-level
     * Click configuration.
     */
    void set_user_click_config_generator_file(const string& v) {
	_user_click_config_generator_file = v;
    }

    /**
     * Write Click configuration.
     *
     * @param element the Click element to write the configuration to. If it
     * is an empty string, then we use only the @ref handler to write the
     * configuration.
     * @param handler the Click handler to write the configuration to.
     * @param has_kernel_config true if we wish to write the kernel-level Click
     * configuration (if kernel-level Click is enabled).
     * @param kernel_config the kernel-level Click configuration to write.
     * @param has_user_config true if we wish to write the user-level Click
     * configuration (if user-level Click is enabled).
     * @param user_config the user-level Click configuration to write.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int write_config(const string& element, const string& handler,
		     bool has_kernel_config, const string& kernel_config,
		     bool has_user_config, const string& user_config,
		     string& error_msg);

    /**
     * Write data to Click socket.
     * 
     * This method also updates the sequence number associated with
     * this Click socket.
     * 
     * @return the number of bytes which were written, or -1 if error.
     */
    ssize_t write(XorpFd fd, const void* data, size_t nbytes);

    /**
     * Check the status of a previous command.
     *
     * @param is_warning if true, the previous command generated a warning.
     * @param command_warning if @ref is_warning is true, then it contains
     * the generated warning message.
     * @param is_error if true, the previous command generated an error.
     * @param command_error if @ref is_error is true, then it contains
     * the generated error message.
     * @param error_msg if the command status cannot be checked, then it
     * contains the error message with the reason.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int check_user_command_status(bool& is_warning, string& command_warning,
				  bool& is_error, string& command_error,
				  string& error_msg);

    /**
     * Get the sequence number for next message written into Click.
     * 
     * The sequence number is derived from the instance number of this Click
     * socket and a 16-bit counter.
     * 
     * @return the sequence number for the next message written into
     * Click.
     */
    uint32_t seqno() const { return (_instance_no << 16 | _seqno); }

    /**
     * Get the cached process identifier value.
     * 
     * @return the cached process identifier value.
     */
    pid_t pid() const { return _pid; }

    /**
     * Force socket to read data from kernel-level Click.
     * 
     * This usually is performed after writing a request that
     * Click will answer (e.g., after writing a configuration change).
     * Use sparingly, with caution, and at your own risk.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int force_kernel_click_read(string& error_msg) {
	return (force_read(_kernel_fd, error_msg));
    }

    /**
     * Force socket to read data from user-level Click.
     * 
     * This usually is performed after writing a request that
     * Click will answer (e.g., after writing a configuration change).
     * Use sparingly, with caution, and at your own risk.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int force_user_click_read(string& error_msg) {
	return (force_read(_user_fd, error_msg));
    }

private:
    typedef list<ClickSocketObserver*> ObserverList;

    /**
     * Force socket to read data.
     * 
     * This usually is performed after writing a request that
     * Click will answer (e.g., after writing a configuration change).
     * Use sparingly, with caution, and at your own risk.
     *
     * @param fd the file descriptor to read from.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int force_read(XorpFd fd, string& error_msg);

    /**
     * Read data available for ClickSocket and invoke
     * ClickSocketObserver::clsock_data() on all observers of Click
     * socket.
     */
    void io_event(XorpFd fd, IoEventType type);

    /**
     * Force socket to read data and return the result.
     * 
     * Note that unlike method @ref force_read(), this method does not
     * propagate the data to the socket observers.
     *
     * @param fd the file descriptor to read from.
     * @param message the message with the result.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int  force_read_message(XorpFd fd, vector<uint8_t>& message,
			    string& error_msg);

    /**
     * Load the kernel Click modules.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int load_kernel_click_modules(string& error_msg);

    /**
     * Unload the kernel Click modules.
     * 
     * Note: the modules are unloaded in the reverse order they are loaded.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unload_kernel_click_modules(string& error_msg);

    /**
     * Load a kernel module.
     * 
     * @param module_filename the kernel module filename to load.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int load_kernel_module(const string& module_filename, string& error_msg);

    /**
     * Unload a kernel module.
     * 
     * Note: the module will not be unloaded if it was not loaded
     * previously by us.
     * 
     * @param module_filename the kernel module filename to unload.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unload_kernel_module(const string& module_filename, string& error_msg);

    /**
     * Get the kernel module name from the kernel module filename.
     * 
     * The module name is composed of the name of the file (without
     * the directory path) but excluding the suffix if that is a well-known
     * suffix (e.g., ".o" or ".ko").
     * 
     * @param module_filename the module filename.
     * @return the kernel module name.
     */
    string kernel_module_filename2modulename(const string& module_filename);

    /**
     * Mount the Click file system.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int mount_click_file_system(string& error_msg);

    /**
     * Unmount the Click file system.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unmount_click_file_system(string& error_msg);

    /**
     * Execute the user-level Click command.
     *
     * @param command the command to execute.
     * @param argument_list the list with the command arguments.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int execute_user_click_command(const string& command,
				   const list<string>& argument_list);

    /**
     * Terminate the user-level Click command.
     */
    void terminate_user_click_command();

    /**
     * The callback to call when there is data on the stdandard output of the
     * user-level Click command.
     */
    void user_click_command_stdout_cb(RunCommand* run_command,
				      const string& output);

    /**
     * The callback to call when there is data on the stdandard error of the
     * user-level Click command.
     */
    void user_click_command_stderr_cb(RunCommand* run_command,
				      const string& output);

    /**
     * The callback to call when there the user-level Click command
     * is completed.
     */
    void user_click_command_done_cb(RunCommand* run_command, bool success,
				    const string& error_msg);

private:
    static const size_t CLSOCK_BYTES = 8*1024; // Initial guess at msg size

    inline const IPv4 DEFAULT_USER_CLICK_CONTROL_ADDRESS() {
	return (IPv4::LOOPBACK());
    }
    static const uint16_t DEFAULT_USER_CLICK_CONTROL_SOCKET_PORT = 13000;
    static const int CLICK_MAJOR_VERSION = 1;
    static const int CLICK_MINOR_VERSION = 1;
    static const size_t CLICK_COMMAND_RESPONSE_MIN_SIZE = 4;
    static const size_t CLICK_COMMAND_RESPONSE_CODE_SEPARATOR_INDEX = 3;
    static const int CLICK_COMMAND_CODE_OK = 200;
    static const int CLICK_COMMAND_CODE_WARNING_MIN = 201;
    static const int CLICK_COMMAND_CODE_WARNING_MAX = 299;
    static const int CLICK_COMMAND_CODE_ERROR_MIN = 500;
    static const int CLICK_COMMAND_CODE_ERROR_MAX = 599;
    static const TimeVal USER_CLICK_STARTUP_MAX_WAIT_TIME;

    static const string PROC_LINUX_MODULES_FILE;
    static const string LINUX_COMMAND_LOAD_MODULE;
    static const string LINUX_COMMAND_UNLOAD_MODULE;
    static const string CLICK_FILE_SYSTEM_TYPE;

private:
    EventLoop&	 _eventloop;
    XorpFd	 _kernel_fd;
    XorpFd	 _user_fd;
    ObserverList _ol;

    uint16_t 	 _seqno;	// Seqno of next write()
    uint16_t	 _instance_no;  // Instance number of this Click socket

    static uint16_t _instance_cnt;
    static pid_t    _pid;

    bool	_is_enabled;		// True if Click is enabled
    bool	_duplicate_routes_to_kernel; // True if duplicating the Click
					// routes to the kernel is enabled
    bool	_is_kernel_click;	// True if kernel Click is enabled
    bool	_is_user_click;		// True if user Click is enabled

    bool	_kernel_click_install_on_startup;
    list<string> _kernel_click_modules;
    list<string> _loaded_kernel_click_modules;
    string	_kernel_click_mount_directory;
    string	_mounted_kernel_click_mount_directory;
    string	_kernel_click_config_generator_file;
    string	_user_click_command_file;
    string	_user_click_command_extra_arguments;
    bool	_user_click_command_execute_on_startup;
    string	_user_click_startup_config_file;
    IPv4	_user_click_control_address;
    uint16_t	_user_click_control_socket_port;
    string	_user_click_config_generator_file;

    RunCommand*	_user_click_run_command;
    
    friend class ClickSocketPlumber; // class that hooks observers in and out
};

class ClickSocketObserver {
public:
    ClickSocketObserver(ClickSocket& cs);

    virtual ~ClickSocketObserver();

    /**
     * Receive data from the Click socket.
     *
     * Note that this method is called asynchronously when the Click socket
     * has data to receive, therefore it should never be called directly by
     * anything else except the Click socket facility itself.
     *
     * @param data the buffer with the received data.
     * @param nbytes the number of bytes in the data buffer.
     */
    virtual void clsock_data(const uint8_t* data, size_t nbytes) = 0;

    /**
     * Get ClickSocket associated with Observer.
     */
    ClickSocket& click_socket();

private:
    ClickSocket& _cs;
};

class ClickSocketReader : public ClickSocketObserver {
public:
    ClickSocketReader(ClickSocket& cs);
    virtual ~ClickSocketReader();

    /**
     * Force the reader to receive kernel-level Click data from the specified
     * Click socket.
     *
     * @param cs the Click socket to receive the data from.
     * @param seqno the sequence number of the data to receive.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int receive_kernel_click_data(ClickSocket& cs, uint32_t seqno,
				  string& error_msg);

    /**
     * Force the reader to receive user-level Click data from the specified
     * Click socket.
     *
     * @param cs the Click socket to receive the data from.
     * @param seqno the sequence number of the data to receive.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int receive_user_click_data(ClickSocket& cs, uint32_t seqno,
				string& error_msg);

    /**
     * Return the buffer as a string with the data that was received.
     * 
     * @return a C-style string with the data that was received.
     */
    const string& buffer_str() const { return (_cache_data); }

    /**
     * Receive data from the Click socket.
     *
     * Note that this method is called asynchronously when the Click socket
     * has data to receive, therefore it should never be called directly by
     * anything else except the Click socket facility itself.
     *
     * @param data the buffer with the received data.
     * @param nbytes the number of bytes in the data buffer.
     */
    virtual void clsock_data(const uint8_t* data, size_t nbytes);

private:
    ClickSocket&    _cs;

    bool	    _cache_valid;	// Cache data arrived.
    uint32_t	    _cache_seqno;	// Seqno of Click socket data to
					// cache so reading via Click
					// socket can appear synchronous.
    string	    _cache_data;	// Cached Click socket data.
};

#endif // click
#endif // __FEA_DATA_PLANE_CONTROL_SOCKET_CLICK_SOCKET_HH__
