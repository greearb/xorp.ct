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

// $XORP: xorp/fea/click_socket.hh,v 1.4 2004/11/23 00:53:19 pavlin Exp $

#ifndef __FEA_CLICK_SOCKET_HH__
#define __FEA_CLICK_SOCKET_HH__

#include <list>

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
class ClickSocket {
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
     * Enable/disable kernel-level Click.
     * 
     * Note that this operation does not start the kernel-level Click
     * operations.
     * 
     * @param v if true, enable kernel-level Click, otherwise disable it.
     */
    void enable_kernel_click(bool v) { _is_kernel_click = v; }

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
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int start();

    /**
     * Stop the Click socket operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int stop();

    /**
     * Specify the external program to generate the Click configuration.
     *
     * @param v the name of the external program to generate the Click
     * configuration.
     */
    void set_click_config_generator_file(const string& v) {
	_click_config_generator_file = v;
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
     * Test if the Click socket is open.
     * 
     * This method is needed because ClickSocket may fail to open
     * Click socket during startup.
     * 
     * @return true if the Click socket is open, otherwise false.
     */
    inline bool is_open() const { return _fd >= 0; }

    /**
     * Write Click configuration.
     *
     * @param handler the Click handler to write the configuration to.
     * @param data the configuration data to write.
     * @param errmsg the error message (if an error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int write_config(const string& handler, const string& data,
		     string& errmsg);

    /**
     * Write data to Click socket.
     * 
     * This method also updates the sequence number associated with
     * this Click socket.
     * 
     * @return the number of bytes which were written, or -1 if error.
     */
    ssize_t write(const void* data, size_t nbytes);

    /**
     * Check the status of a previous command.
     *
     * @param is_warning if true, the previous command generated a warning.
     * @param command_warning if @ref is_warning is true, then it contains
     * the generated warning message.
     * @param is_error if true, the previous command generated an error.
     * @param command_error if @ref is_error is true, then it contains
     * the generated error message.
     * @param errmsg if the command status cannot be checked, then it contains
     * the error message with the reason.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int check_command_status(bool& is_warning, string& command_warning,
			     bool& is_error, string& command_error,
			     string& errmsg);

    /**
     * Get the sequence number for next message written into Click.
     * 
     * The sequence number is derived from the instance number of this Click
     * socket and a 16-bit counter.
     * 
     * @return the sequence number for the next message written into
     * Click.
     */
    inline uint32_t seqno() const { return (_instance_no << 16 | _seqno); }

    /**
     * Get the cached process identifier value.
     * 
     * @return the cached process identifier value.
     */
    inline pid_t pid() const { return _pid; }

    /**
     * Force socket to read data.
     * 
     * This usually is performed after writing a request that
     * Click will answer (e.g., after writing a configuration change).
     * Use sparingly, with caution, and at your own risk.
     *
     * @param errmsg the error message (if an error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int force_read(string& errmsg);

private:
    typedef list<ClickSocketObserver*> ObserverList;

    /**
     * Read data available for ClickSocket and invoke
     * ClickSocketObserver::clsock_data() on all observers of Click
     * socket.
     */
    void select_hook(int fd, SelectorMask sm);

    /**
     * Force socket to read data and return the result.
     * 
     * Note that unlike method @ref force_read(), this method does not
     * propagate the data to the socket observers.
     *
     * @param message the message with the result.
     * @param errmsg the error message (if an error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int  force_read_message(vector<uint8_t>& message, string& errmsg);

    /**
     * Execute the user-level Click command.
     *
     * @param command the command to execute.
     * @param arguments the command arguments.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int execute_user_click_command(const string& command,
				   const string& arguments);

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

    void shutdown();

    ClickSocket& operator=(const ClickSocket&);		// Not implemented
    ClickSocket(const ClickSocket&);			// Not implemented

private:
    static const size_t CLSOCK_BYTES = 8*1024; // Initial guess at msg size

    static const IPv4 DEFAULT_USER_CLICK_CONTROL_ADDRESS;
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

private:
    EventLoop&	 _eventloop;
    int		 _fd;
    ObserverList _ol;

    uint16_t 	 _seqno;	// Seqno of next write()
    uint16_t	 _instance_no;  // Instance number of this Click socket

    static uint16_t _instance_cnt;
    static pid_t    _pid;

    bool	_is_enabled;		// True if Click is enabled
    bool	_is_kernel_click;	// True if kernel Click is enabled
    bool	_is_user_click;		// True if user Click is enabled

    string	_click_config_generator_file;
    string	_user_click_command_file;
    string	_user_click_command_extra_arguments;
    bool	_user_click_command_execute_on_startup;
    string	_user_click_startup_config_file;
    IPv4	_user_click_control_address;
    uint16_t	_user_click_control_socket_port;

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
     * Force the reader to receive data from the specified Click socket.
     *
     * @param cs the Click socket to receive the data from.
     * @param seqno the sequence number of the data to receive.
     * @param errmsg the error message (if an error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int receive_data(ClickSocket& cs, uint32_t seqno, string& errmsg);

    /**
     * Return the buffer as a string with the data that was received.
     * 
     * @return a C-style string with the data that was received.
     */
    const string& buffer_str() const { return (_cache_data); }

    /**
     * Get the size of the buffer with the data that was received.
     * 
     * @return the size of the buffer with the data that was received.
     */
    const size_t   buffer_size() const { return (_cache_data.size()); }

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

#endif // __FEA_CLICK_SOCKET_HH__
