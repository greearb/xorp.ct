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




//
// CLI network-related functions
//


#include "cli_module.h"
#include "libxorp/xorp.h"

#include <errno.h>

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif
#ifdef HAVE_TERMIOS_H
#include <termios.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/c_format.hh"
#include "libxorp/time_slice.hh"
#include "libxorp/token.hh"

#include "libcomm/comm_api.h"

#include "cli_client.hh"
#include "cli_private.hh"
#include "libxorp/build_info.hh"

#ifdef HAVE_ARPA_TELNET_H
#include <arpa/telnet.h>
#endif

#ifdef HOST_OS_WINDOWS
#include "libxorp/win_io.h"
#define DEFAULT_TERM_TYPE	"ansi-nt"
#define FILENO(x)	((HANDLE)_get_osfhandle(_fileno(x)))
#define FDOPEN(x,y)	_fdopen(_open_osfhandle((x),_O_RDWR|_O_TEXT),(y))
#else // ! HOST_OS_WINDOWS
#define DEFAULT_TERM_TYPE "vt100"
#define FILENO(x) fileno(x)
#define FDOPEN(x,y) fdopen((x), (y))
#endif // HOST_OS_WINDOWS

static set<CliClient *> local_cli_clients_;

#ifndef HOST_OS_WINDOWS
static void
sigwinch_handler(int signo)
{
    XLOG_ASSERT(signo == SIGWINCH);

    set<CliClient *>::iterator iter;
    for (iter = local_cli_clients_.begin();
	 iter != local_cli_clients_.end();
	 ++iter) {
	CliClient *cli_client = *iter;
	cli_client->terminal_resized();
    }
}
#endif // ! HOST_OS_WINDOWS

/**
 * CliNode::sock_serv_open:
 * 
 * Open a socket for the CLI to listen on for connections.
 * 
 * Return value: The new socket to listen on success, othewise a XockFd
 * that contains an invalid socket.
 **/
XorpFd
CliNode::sock_serv_open()
{
    // Open the socket
    switch (family()) {
    case AF_INET:
	_cli_socket = comm_bind_tcp4(NULL, _cli_port, COMM_SOCK_NONBLOCKING, NULL);
	break;
#ifdef HAVE_IPV6
    case AF_INET6:
	_cli_socket = comm_bind_tcp6(NULL, 0, _cli_port,
				     COMM_SOCK_NONBLOCKING, NULL);
	break;
#endif // HAVE_IPV6
    default:
	XLOG_UNREACHABLE();
	_cli_socket.clear();
	break;
    }
    if (comm_listen(_cli_socket, COMM_LISTEN_DEFAULT_BACKLOG) != XORP_OK) {
	_cli_socket.clear();
    }
    return (_cli_socket);
}

/**
 * CliNode::sock_serv_close:
 * @: 
 * 
 * Close the socket that is used by the CLI to listen on for connections.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
CliNode::sock_serv_close()
{
    int ret_value = XORP_OK;

    if (!_cli_socket.is_valid())
	return (XORP_OK);	// Nothing to do

    if (comm_close(_cli_socket) != XORP_OK)
	ret_value = XORP_ERROR;

    _cli_socket.clear();

    return (ret_value);
}

void
CliNode::accept_connection(XorpFd fd, IoEventType type)
{
    string error_msg;

    debug_msg("Received connection on socket = %s, family = %d\n",
	      fd.str().c_str(), family());
    
    XLOG_ASSERT(type == IOT_ACCEPT);
    
    XorpFd client_socket = comm_sock_accept(fd);
    
    if (client_socket.is_valid()) {
	if (add_connection(client_socket, client_socket, true,
			   _startup_cli_prompt, error_msg)
	    == NULL) {
	    XLOG_ERROR("Cannot accept CLI connection: %s", error_msg.c_str());
	}
    }
}

CliClient *
CliNode::add_connection(XorpFd input_fd, XorpFd output_fd, bool is_network,
			const string& startup_cli_prompt, string& error_msg)
{
    string dummy_error_msg;
    CliClient *cli_client = NULL;
    
    debug_msg("Added connection with input_fd = %s, output_fd = %s, "
	      "family = %d\n",
	      input_fd.str().c_str(), output_fd.str().c_str(), family());
    
    cli_client = new CliClient(*this, input_fd, output_fd, startup_cli_prompt);
    cli_client->set_network_client(is_network);
    _client_list.push_back(cli_client);

#ifdef HOST_OS_WINDOWS
    if (cli_client->is_interactive()) {
	BOOL retval;
#if 0
	// XXX: This always fails, so it's commented out.
	retval = SetConsoleMode(input_fd,
ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT);
	if (retval == 0) {
	    XLOG_WARNING("SetConsoleMode(input) failed: %s",
	    		 win_strerror(GetLastError()));
	}
#endif
	retval = SetConsoleMode(output_fd,
ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT);
	if (retval == 0) {
	    XLOG_WARNING("SetConsoleMode(output) failed: %s",
			 win_strerror(GetLastError()));
	}
    }
#endif
    
    //
    // Set peer address (for network connection only)
    //
    if (cli_client->is_network()) {
	struct sockaddr_storage ss;
	socklen_t len = sizeof(ss);
	// XXX
	if (getpeername(cli_client->input_fd(), (struct sockaddr *)&ss, &len)
	    < 0) {
	    error_msg = c_format("Cannot get peer name");
	    // Error getting peer address
	    delete_connection(cli_client, dummy_error_msg);
	    return (NULL);
	}
	IPvX peer_addr = IPvX::ZERO(family());

	// XXX: The fandango below can go away once the IPvX
	// constructors are fixed to do the right thing.
	switch (ss.ss_family) {
	case AF_INET:
	{
	    struct sockaddr_in *s_in = (struct sockaddr_in *)&ss;
	    peer_addr.copy_in(*s_in);
	}
	break;
#ifdef HAVE_IPV6
	case AF_INET6:
	{
	    struct sockaddr_in6 *s_in6 = (struct sockaddr_in6 *)&ss;
	    peer_addr.copy_in(*s_in6);
	}
	break;
#endif // HAVE_IPV6
	default:
	    // Invalid address family
	    error_msg = c_format("Cannot set peer address: "
				 "invalid address family (%d)",
				 ss.ss_family);
	    delete_connection(cli_client, dummy_error_msg);
	    return (NULL);
	}
	cli_client->set_cli_session_from_address(peer_addr);
    }
    
    //
    // Check access control for this peer address
    //
    if (! is_allow_cli_access(cli_client->cli_session_from_address())) {
	error_msg = c_format("CLI access from address %s is not allowed",
			     cli_client->cli_session_from_address().str().c_str());
	delete_connection(cli_client, dummy_error_msg);
	return (NULL);
    }
    
    if (cli_client->start_connection(error_msg) != XORP_OK) {
	// Error connecting to the client
	delete_connection(cli_client, dummy_error_msg);
	return (NULL);
    }
    
    //
    // Connection OK
    //
    
    //
    // Set user name
    //
    cli_client->set_cli_session_user_name("guest");	// TODO: get user name
    
    //
    // Set terminal name
    //
    {
	string term_name = "cli_unknown";
	uint32_t i = 0;
	
	for (i = 0; i < CLI_MAX_CONNECTIONS; i++) {
	    term_name = c_format("cli%u", XORP_UINT_CAST(i));
	    if (find_cli_by_term_name(term_name) == NULL)
		break;
	}
	if (i >= CLI_MAX_CONNECTIONS) {
	    // Too many connections
	    error_msg = c_format("Too many CLI connections (max is %u)",
				 XORP_UINT_CAST(CLI_MAX_CONNECTIONS));
	    delete_connection(cli_client, dummy_error_msg);
	    return (NULL);
	}
	cli_client->set_cli_session_term_name(term_name);
    }
    
    //
    // Set session id
    //
    {
	uint32_t session_id = ~0U;	// XXX: ~0U has no particular meaning
	uint32_t i = 0;
	
	for (i = 0; i < CLI_MAX_CONNECTIONS; i++) {
	    session_id = _next_session_id++;
	    if (find_cli_by_session_id(session_id) == NULL)
		break;
	}
	if (i >= CLI_MAX_CONNECTIONS) {
	    // This should not happen: there are available session slots,
	    // but no available session IDs.
	    XLOG_FATAL("Cannot assign CLI session ID");
	    return (NULL);
	}
	cli_client->set_cli_session_session_id(session_id);
    }
    
    //
    // Set session start time
    //
    {
	TimeVal now;
	
	eventloop().current_time(now);
	cli_client->set_cli_session_start_time(now);
    }
    cli_client->set_is_cli_session_active(true);
    
    return (cli_client);
}

int
CliNode::delete_connection(CliClient *cli_client, string& error_msg)
{
    list<CliClient *>::iterator iter;

    iter = find(_client_list.begin(), _client_list.end(), cli_client);
    if (iter == _client_list.end()) {
	error_msg = c_format("Cannot delete CLI connection: invalid client");
	return (XORP_ERROR);
    }

    debug_msg("Delete connection on input fd = %s, output fd = %s, "
	      "family = %d\n",
	      cli_client->input_fd().str().c_str(),
	      cli_client->output_fd().str().c_str(),
	      family());
    cli_client->cli_flush();
    
    // The callback when deleting this client
    if (! _cli_client_delete_callback.is_empty())
	_cli_client_delete_callback->dispatch(cli_client);

    if (cli_client->is_network()) {
	// XXX: delete the client only if this was a network connection
	_client_list.erase(iter);
	delete cli_client;
    } else {
	eventloop().remove_ioevent_cb(cli_client->input_fd(), IOT_READ);
    }

    return (XORP_OK);
}

int
CliClient::start_connection(string& error_msg)
{
    if (cli_node().eventloop().add_ioevent_cb(
	    input_fd(),
	    IOT_READ,
	    callback(this, &CliClient::client_read),
	    XorpTask::PRIORITY_HIGHEST) == false) {
	return (XORP_ERROR);
    }
  
#ifdef HAVE_ARPA_TELNET_H
    //
    // Setup the telnet options
    //
    if (is_telnet()) {
	uint8_t will_echo_cmd[] = { IAC, WILL, TELOPT_ECHO, '\0' };
	uint8_t will_sga_cmd[]  = { IAC, WILL, TELOPT_SGA, '\0'  };
	uint8_t dont_linemode_cmd[] = { IAC, DONT, TELOPT_LINEMODE, '\0' };
	uint8_t do_window_size_cmd[] = { IAC, DO, TELOPT_NAWS, '\0' };
	uint8_t do_transmit_binary_cmd[] = { IAC, DO, TELOPT_BINARY, '\0' };
	uint8_t will_transmit_binary_cmd[] = { IAC, WILL, TELOPT_BINARY, '\0' };

	send(input_fd(), will_echo_cmd, sizeof(will_echo_cmd), 0);
	send(input_fd(), will_sga_cmd, sizeof(will_sga_cmd), 0);
	send(input_fd(), dont_linemode_cmd, sizeof(dont_linemode_cmd), 0);
	send(input_fd(), do_window_size_cmd, sizeof(do_window_size_cmd), 0);
	send(input_fd(), do_transmit_binary_cmd,
	     sizeof(do_transmit_binary_cmd), 0);
	send(input_fd(), will_transmit_binary_cmd,
	     sizeof(will_transmit_binary_cmd), 0);
    }
#endif

#ifndef HOST_OS_WINDOWS
    if (! is_network()) {
	signal(SIGWINCH, sigwinch_handler);
    }
#endif
    
#ifdef HAVE_TERMIOS_H
    //
    // Put the terminal in non-canonical and non-echo mode.
    // In addition, disable signals INTR, QUIT, [D]SUSP
    // (i.e., force their value to be received when read from the terminal).
    //
    if (is_output_tty()) {
	struct termios termios;

	//
	// Get the parameters associated with the terminal
	//
	while (tcgetattr(output_fd(), &termios) != 0) {
	    if (errno != EINTR) {
		error_msg = c_format("start_connection(): "
				     "tcgetattr() error: %s", 
				     strerror(errno));
		return (XORP_ERROR);
	    }
	}

	//
	// Save state regarding any terminal-related modifications we may do
	//
	if (termios.c_lflag & ICANON)
	    _is_modified_stdio_termios_icanon = true;
	if (termios.c_lflag & ECHO)
	    _is_modified_stdio_termios_echo = true;
	if (termios.c_lflag & ISIG)
	    _is_modified_stdio_termios_isig = true;
	_saved_stdio_termios_vmin = termios.c_cc[VMIN];
	_saved_stdio_termios_vtime = termios.c_cc[VTIME];

	//
	// Change the termios:
	// - Reset the flags.
	// - Set VMIN and VTIME to values that allow us to read one
	//   character at a time.
	//
	termios.c_lflag &= ~(ICANON | ECHO | ISIG);
	termios.c_cc[VMIN] = 1;
	termios.c_cc[VTIME] = 0;

	//
	// Modify the terminal
	//
	while (tcsetattr(output_fd(), TCSADRAIN, &termios) != 0) {
	    if (errno != EINTR) {
		error_msg = c_format("start_connection(): "
				     "tcsetattr() error: %s", 
				     strerror(errno));
		return (XORP_ERROR);
	    }
	}
    }
#endif // HAVE_TERMIOS_H
    
    //
    // Setup the read/write file descriptors
    //
    if (input_fd() == XorpFd(FILENO(stdin))) {
	_input_fd_file = stdin;
    } else {
	_input_fd_file = FDOPEN(input_fd(), "r");
	if (_input_fd_file == NULL) {
	    error_msg = c_format("Cannot associate a stream with the "
				 "input file descriptor: %s",
				 strerror(errno));
	    return (XORP_ERROR);
	}
    }
    if (output_fd() == XorpFd(FILENO(stdout))) {
	_output_fd_file = stdout;
    } else {
	_output_fd_file = FDOPEN(output_fd(), "w");
	if (_output_fd_file == NULL) {
	    error_msg = c_format("Cannot associate a stream with the "
				 "output file descriptor: %s",
				 strerror(errno));
	    return (XORP_ERROR);
	}
    }
    
    _gl = new_GetLine(1024, 2048);		// TODO: hardcoded numbers
    if (_gl == NULL) {
	error_msg = c_format("Cannot create a new GetLine instance");
	return (XORP_ERROR);
    }
    
    // XXX: always set to network type
    gl_set_is_net(_gl, 1);
    
    // Set the terminal
    string term_name = DEFAULT_TERM_TYPE;
    if (is_output_tty()) {
#ifdef HOST_OS_WINDOWS
	//
	// Do not ask the environment what kind of terminal we use
	// under Windows, as MSYS is known to lie to us and say 'cygwin'
	// when in fact we're using an 'ansi-nt'. We've hard-coded
	// appropriate control sequences in our fork of libtecla to
	// reflect this fact.
	// XXX: We need a better way of figuring out when we're in
	// this situation.

	; // do nothing
#else
	char *term = getenv("TERM");
	if ((term != NULL) && (! string(term).empty()))
	    term_name = string(term);
#endif
    }

    //
    // Change the input and output streams for libtecla
    //
    // Note that it must happen before gl_terminal_size(),
    // because gl_change_terminal() resets internally the terminal
    // size to its default value.
    //
    if (gl_change_terminal(_gl, _input_fd_file, _output_fd_file,
			   term_name.c_str())
	!= 0) {
	error_msg = c_format("Cannot change the I/O streams");
	_gl = del_GetLine(_gl);
	return (XORP_ERROR);
    }

    // Update the terminal size
    update_terminal_size();

    // Add the command completion hook
    if (gl_customize_completion(_gl, this, command_completion_func) != 0) {
	error_msg = c_format("Cannot customize command-line completion");
	_gl = del_GetLine(_gl);
	return (XORP_ERROR);
    }

    //
    // Key bindings
    //

    // Bind Ctrl-C to no-op
    gl_configure_getline(_gl, "bind ^C user-event4", NULL, NULL);

    // Bind Ctrl-W to delete the word before the cursor, because
    // the default libtecla behavior is to delete the whole line.
    gl_configure_getline(_gl, "bind ^W backward-delete-word", NULL, NULL);

    // Add ourselves to the local set of clients
    local_cli_clients_.insert(this);

    // Print the welcome message
    char hostname[MAXHOSTNAMELEN];
    if (gethostname(hostname, sizeof(hostname)) < 0) {
	XLOG_ERROR("gethostname() failed: %s", strerror(errno));
	// XXX: if gethostname() fails, then default to "xorp"
	strncpy(hostname, "xorp", sizeof(hostname) - 1);
    }
    hostname[sizeof(hostname) - 1] = '\0';
    cli_print(c_format("Welcome to XORP v%s on %s\n",
		       BuildInfo::getXorpVersion(), hostname));
#ifdef XORP_BUILDINFO
    const char* bits = "32-bit";
    if (sizeof(void*) == 8)
	bits = "64-bit";
    cli_print(c_format("Version tag: %s  Build Date: %s %s\n",
		       BuildInfo::getGitVersion(),
		       BuildInfo::getShortBuildDate(),
		       bits));
#endif


    // Show the prompt
    cli_print(current_cli_prompt());

    return (XORP_OK);
}

int
CliClient::stop_connection(string& error_msg)
{
    // Delete ourselves from the local set of clients
    local_cli_clients_.erase(this);

#ifdef HAVE_TERMIOS_H
    //
    // Restore the terminal settings
    //
    if (is_output_tty()) {
	struct termios termios;

	//
	// Get the parameters associated with the terminal
	//
	while (tcgetattr(output_fd(), &termios) != 0) {
	    if (errno != EINTR) {
		XLOG_ERROR("stop_connection(): tcgetattr() error: %s", 
			   strerror(errno));
		return (XORP_ERROR);
	    }
	}

	//
	// Restore the termios changes
	//
	if (_is_modified_stdio_termios_icanon)
	    termios.c_lflag |= ICANON;
	if (_is_modified_stdio_termios_echo)
	    termios.c_lflag |= ECHO;
	if (_is_modified_stdio_termios_isig)
	    termios.c_lflag |= ISIG;
	_saved_stdio_termios_vmin = termios.c_cc[VMIN];
	_saved_stdio_termios_vtime = termios.c_cc[VTIME];

	//
	// Modify the terminal
	//
	while (tcsetattr(output_fd(), TCSADRAIN, &termios) != 0) {
	    if (errno != EINTR) {
		error_msg = c_format("stop_connection(): "
				     "tcsetattr() error: %s",
				     strerror(errno));
		return (XORP_ERROR);
	    }
	}
    }
#endif // HAVE_TERMIOS_H

    error_msg = "";
    return (XORP_OK);
}

void
CliClient::terminal_resized()
{
    update_terminal_size();
}

void
CliClient::update_terminal_size()
{
#ifdef HAVE_TERMIOS_H
    // Get the terminal size
    if (is_output_tty()) {
	struct winsize window_size;

	if (ioctl(output_fd(), TIOCGWINSZ, &window_size) < 0) {
	    XLOG_ERROR("Cannot get window size (ioctl(TIOCGWINSZ) failed): %s",
		       strerror(errno));
	} else {
	    // Set the window width and height
	    uint16_t new_window_width, new_window_height;

	    new_window_width = window_size.ws_col;
	    new_window_height = window_size.ws_row;

	    if (new_window_width > 0) {
		set_window_width(new_window_width);
	    } else {
		cli_print(c_format("Invalid window width (%u); "
				   "window width unchanged (%u)\n",
				   new_window_width,
				   XORP_UINT_CAST(window_width())));
	    }
	    if (new_window_height > 0) {
		set_window_height(new_window_height);
	    } else {
		cli_print(c_format("Invalid window height (%u); "
				   "window height unchanged (%u)\n",
				   new_window_height,
				   XORP_UINT_CAST(window_height())));
	    }

	    gl_terminal_size(gl(), window_width(), window_height());
	    debug_msg("Client window size changed to width = %u "
		      "height = %u\n",
		      XORP_UINT_CAST(window_width()),
		      XORP_UINT_CAST(window_height()));
	}
    }
#endif // HAVE_TERMIOS_H
}

//
// If @v is true, block the client terminal, otherwise unblock it.
//
void
CliClient::set_is_waiting_for_data(bool v)
{
    _is_waiting_for_data = v;
    // block_connection(v);
}

//
// If @is_blocked is true, block the connection (by removing its I/O
// event hook), otherwise add its socket back to the event loop.
//
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
int
CliClient::block_connection(bool is_blocked)
{
    if (!input_fd().is_valid())
	return (XORP_ERROR);
    
    if (is_blocked) {
	cli_node().eventloop().remove_ioevent_cb(input_fd(), IOT_READ);
	return (XORP_OK);
    }

    if (cli_node().eventloop().add_ioevent_cb(input_fd(), IOT_READ,
					    callback(this, &CliClient::client_read),
					      XorpTask::PRIORITY_HIGHEST)
	== false)
	return (XORP_ERROR);

    return (XORP_OK);
}

void
CliClient::client_read(XorpFd fd, IoEventType type)
{
    string dummy_error_msg;

    char buf[1024];		// TODO: 1024 size must be #define
    int n;
    
    XLOG_ASSERT(type == IOT_READ);

#ifdef HOST_OS_WINDOWS
    if (!is_interactive()) {
	n = recv(fd, buf, sizeof(buf), 0);
    } else {
	//
	// A 0-byte interactive read is not an error; it may simply
	// mean the read routine filtered out an event which we
	// weren't interested in.
	//
	n = win_con_read(fd, buf, sizeof(buf));
	if (n == 0) {
	    return;
	}
    }
#else /* !HOST_OS_WINDOWS */
    n = read(fd, buf, sizeof(buf) - 1);
#endif /* HOST_OS_WINDOWS */
    
    debug_msg("client_read %d octet(s)\n", n);
    if (n <= 0) {
	cli_node().delete_connection(this, dummy_error_msg);
	return;
    }

    // Add the new data to the buffer with the pending data
    size_t old_size = _pending_input_data.size();
    _pending_input_data.resize(old_size + n);
    memcpy(&_pending_input_data[old_size], buf, n);

    process_input_data();
}

void
CliClient::process_input_data()
{
    int ret_value;
    string dummy_error_msg;
    vector<uint8_t> input_data = _pending_input_data;
    bool stop_processing = false;

    //
    // XXX: Remove the stored input data. Later we will add-back
    // only the data which we couldn't process.
    //
    _pending_input_data.clear();

    TimeSlice time_slice(1000000, 1); // 1s, test every iteration

    // Process the input data
    vector<uint8_t>::iterator iter;
    for (iter = input_data.begin(); iter != input_data.end(); ++iter) {
	uint8_t val = *iter;
	bool ignore_current_character = false;
	
	if (is_telnet()) {
	    // Filter-out the Telnet commands
	    bool is_telnet_option = false;
	    int ret = process_telnet_option(val, is_telnet_option);
	    if (ret != XORP_OK) {
		// Kick-out the client
		// TODO: print more informative message about the client:
		// E.g. where it came from, etc.
		XLOG_WARNING("Removing client (socket = %s family = %d): "
			     "error processing telnet option",
			     input_fd().str().c_str(),
			     cli_node().family());
		cli_node().delete_connection(this, dummy_error_msg);
		return;
	    }
	    if (is_telnet_option) {
		// Telnet option processed
		continue;
	    }
	}

	if (val == CHAR_TO_CTRL('c')) {
	    //
	    // Interrupt current command
	    //
	    interrupt_command();
	    _pending_input_data.clear();
	    return;
	}

	if (stop_processing)
	    continue;

	preprocess_char(val, stop_processing);

	if (is_waiting_for_data() && (! is_page_mode())) {
	    stop_processing = true;
	    ignore_current_character = true;
	}

	if (! stop_processing) {
	    //
	    // Get a character and process it
	    //
	    do {
		char *line;
		line = gl_get_line_net(gl(),
				       current_cli_prompt().c_str(),
				       (char *)command_buffer().data(),
				       buff_curpos(),
				       val);
		ret_value = XORP_ERROR;
		if (line == NULL) {
		    ret_value = XORP_ERROR;
		    break;
		}
		if (is_page_mode()) {
		    ret_value = process_char_page_mode(val);
		    break;
		}
		ret_value = process_char(string(line), val, stop_processing);
		break;
	    } while (false);

	    if (ret_value != XORP_OK) {
		// Either error or end of input
		cli_print("\nEnd of connection.\n");
		cli_node().delete_connection(this, dummy_error_msg);
		return;
	    }
	}

	if (time_slice.is_expired()) {
	    stop_processing = true;
	}

	if (stop_processing) {
	    //
	    // Stop processing and save the remaining input data for later
	    // processing.
	    // However we continue scanning the rest of the data
	    // primarily to look for Ctrl-C input.
	    //
	    vector<uint8_t>::iterator iter2 = iter;

	    if (! ignore_current_character)
		++iter2;
	    if (iter2 != input_data.end())
		_pending_input_data.assign(iter2, input_data.end());
	}
    }
    if (! _pending_input_data.empty())
	schedule_process_input_data();
	
    cli_flush();		// Flush-out the output
}

//
// Schedule a timer to process (pending) input data
//
void
CliClient::schedule_process_input_data()
{
    EventLoop& eventloop = cli_node().eventloop();
    OneoffTimerCallback cb = callback(this, &CliClient::process_input_data);

    //
    // XXX: Schedule the processing after 10ms to avoid increasing
    // the CPU usage.
    //
    _process_pending_input_data_timer = eventloop.new_oneoff_after(
	TimeVal(0, 10),
	cb,
	XorpTask::PRIORITY_LOWEST);
}

//
// Preprocess a character before 'libtecla' get its hand on it
//
int
CliClient::preprocess_char(uint8_t val, bool& stop_processing)
{
    stop_processing = false;

    if (is_page_mode())
	return (XORP_OK);

    if ((val == '\n') || (val == '\r')) {
	// New command
	if (is_waiting_for_data())
	    stop_processing = true;

	return (XORP_OK);
    }

    //
    // XXX: Bind/unbind the 'SPACE' to complete-word so it can
    // complete a half-written command.
    // TODO: if the beginning of the line, shall we explicitly unbind as well?
    //
    if (val == ' ') {
	int tmp_buff_curpos = buff_curpos();
	char *tmp_line = (char *)command_buffer().data();
	string command_line = string(tmp_line, tmp_buff_curpos);
	if (! is_multi_command_prefix(command_line)) {
	    // Un-bind the "SPACE" to complete-word
	    // Don't ask why we need six '\' to specify the ASCII value
	    // of 'SPACE'...
	    gl_configure_getline(gl(),
				 "bind \\\\\\040 ",
				 NULL, NULL);
	} else {
	    // Bind the "SPACE" to complete-word
	    gl_configure_getline(gl(),
				 "bind \\\\\\040   complete-word",
				 NULL, NULL);
	}

	return (XORP_OK);
    }
    
    return (XORP_OK);
}

int
CliClient::process_telnet_option(int val, bool& is_telnet_option)
{
#ifdef HOST_OS_WINDOWS
    UNUSED(val);
    is_telnet_option = false;
    return (XORP_OK);
#else
    is_telnet_option = true;
    if (val == IAC) {
	// Probably a telnet command
	if (! telnet_iac()) {
	    set_telnet_iac(true);
	    return (XORP_OK);
	}
	set_telnet_iac(false);
    }
    if (telnet_iac()) {
	switch (val) {
	case SB:
	    // Begin subnegotiation of the indicated option.
	    telnet_sb_buffer().reset();
	    set_telnet_sb(true);
	    break;
	case SE:
	    // End subnegotiation of the indicated option.
	    if (! telnet_sb())
		break;
	    switch(telnet_sb_buffer().data(0)) {
	    case TELOPT_NAWS:
		// Telnet Window Size Option
		if (telnet_sb_buffer().data_size() < 5)
		    break;
		{
		    uint16_t new_window_width, new_window_height;
		    
		    new_window_width   = 256*telnet_sb_buffer().data(1);
		    new_window_width  += telnet_sb_buffer().data(2);
		    new_window_height  = 256*telnet_sb_buffer().data(3);
		    new_window_height += telnet_sb_buffer().data(4);

		    if (new_window_width > 0) {
			set_window_width(new_window_width);
		    } else {
			cli_print(c_format("Invalid window width (%u); "
					   "window width unchanged (%u)\n",
					   new_window_width,
					   XORP_UINT_CAST(window_width())));
		    }
		    if (new_window_height > 0) {
			set_window_height(new_window_height);
		    } else {
			cli_print(c_format("Invalid window height (%u); "
					   "window height unchanged (%u)\n",
					   new_window_height,
					   XORP_UINT_CAST(window_height())));
		    }

		    gl_terminal_size(gl(), window_width(), window_height());
		    debug_msg("Client window size changed to width = %u "
			      "height = %u\n",
			      XORP_UINT_CAST(window_width()),
			      XORP_UINT_CAST(window_height()));
		}
		break;
	    default:
		break;
	    }
	    telnet_sb_buffer().reset();
	    set_telnet_sb(false);
	    break;
	case DONT:
	    // you are not to use option
	    set_telnet_dont(true);
	    break;
	case DO:
	    // please, you use option
	    set_telnet_do(true);
	    break;
	case WONT:
	    // I won't use option
	    set_telnet_wont(true);
	    break;
	case WILL:
	    // I will use option
	    set_telnet_will(true);
	    break;
	case GA:
	    // you may reverse the line
	    break;
	case EL:
	    // erase the current line
	    break;
	case EC:
	    // erase the current character
	    break;
	case AYT:
	    // are you there
	    break;
	case AO:
	    // abort output--but let prog finish
	    break;
	case IP:
	    // interrupt process--permanently
	    break;
	case BREAK:
	    // break
	    break;
	case DM:
	    // data mark--for connect. cleaning
	    break;
	case NOP:
	    // nop
	    break;
	case EOR:
	    // end of record (transparent mode)
	    break;
	case ABORT:
	    // Abort process
	    break;
	case SUSP:
	    // Suspend process
	    break;
	case xEOF:
	    // End of file: EOF is already used...
	    break;
	case TELOPT_BINARY:
	    if (telnet_do())
		set_telnet_binary(true);
	    else
		set_telnet_binary(false);
	    break;
	default:
	    break;
	}
	set_telnet_iac(false);
	return (XORP_OK);
    }
    //
    // Cleanup the telnet options state
    //
    if (telnet_sb()) {
	// A negotiated option value
	if (telnet_sb_buffer().add_data(val) != XORP_OK) {
	    // This client is sending too much options. Kick it out!
	    return (XORP_ERROR);
	}
	return (XORP_OK);
    }
    if (telnet_dont()) {
	// Telnet DONT option code
	set_telnet_dont(false);
	return (XORP_OK);
    }
    if (telnet_do()) {
	// Telnet DO option code
	set_telnet_do(false);
	return (XORP_OK);
    }
    if (telnet_wont()) {
	// Telnet WONT option code
	set_telnet_wont(false);
	return (XORP_OK);
    }
    if (telnet_will()) {
	// Telnet WILL option code
	set_telnet_will(false);
	return (XORP_OK);
    }

    // XXX: Not a telnet option
    is_telnet_option = false;

    return (XORP_OK);
#endif // HOST_OS_WINDOWS
}
