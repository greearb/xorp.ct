// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/cli/cli_client.cc,v 1.62 2008/07/23 05:09:45 pavlin Exp $"


//
// CLI (Command-Line Interface) implementation for XORP.
//


#include "cli_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/token.hh"
#include "libxorp/utils.hh"

#include "libcomm/comm_api.h"

#include "cli_client.hh"
#include "cli_command_pipe.hh"
#include "cli_private.hh"

#ifdef HOST_OS_WINDOWS
#define isatty(x) (x).is_console()
#endif

//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//


//
// Local variables
//

//
// Local functions prototypes
//


// TODO: use a parameter to define the buffer size
CliClient::CliClient(CliNode& init_cli_node, XorpFd input_fd, XorpFd output_fd,
		     const string& startup_cli_prompt)
    : _cli_node(init_cli_node),
      _input_fd(input_fd),
      _output_fd(output_fd),
      _input_fd_file(NULL),
      _output_fd_file(NULL),
      _client_type(CLIENT_TERMINAL),	// XXX: default is terminal
      _gl(NULL),
      _telnet_iac(false),
      _telnet_sb(false),
      _telnet_dont(false),
      _telnet_do(false),
      _telnet_wont(false),
      _telnet_will(false),
      _telnet_binary(false),
      _window_width(80),		// TODO: use a parameter instead
      _window_height(25),		// TODO: use a parameter instead
      _command_buffer(1024),
      _telnet_sb_buffer(1024),
      _is_modified_stdio_termios_icanon(false),
      _is_modified_stdio_termios_echo(false),
      _is_modified_stdio_termios_isig(false),
      _saved_stdio_termios_vmin(0),
      _saved_stdio_termios_vtime(0),
      _executed_cli_command(NULL),
      _current_cli_command(NULL),
      _buff_curpos(0),
      _is_pipe_mode(false),
      _is_nomore_mode(false),
      _is_hold_mode(false),
      _is_page_mode(false),
      _is_page_buffer_mode(NULL),
      _page_buffer(NULL),
      _page_buffer_last_line_n(NULL),
      _is_output_buffer_mode(false),
      _output_buffer_last_line_n(0),
      _is_help_buffer_mode(false),
      _help_buffer_last_line_n(0),
      _is_help_mode(false),
      _is_prompt_flushed(false),
      _cli_session_from_address(_cli_node.family()),
      _is_cli_session_active(false),
      _cli_session_session_id(0),
      _is_network(false),
      _is_log_output(false),
      _is_waiting_for_data(false)
{
    set_current_cli_command(_cli_node.cli_command_root());
    set_current_cli_prompt(startup_cli_prompt);

    _is_page_buffer_mode = &_is_output_buffer_mode;
    _page_buffer = &_output_buffer;
    _page_buffer_last_line_n = &_output_buffer_last_line_n;

    //
    // Session info state
    //
    set_cli_session_user_name("unknown_user");
    set_cli_session_term_name("unknown_terminal");
    set_cli_session_session_id(~0U);	// XXX: ~0U has no particular meaning
    set_cli_session_start_time(TimeVal(0, 0));
    set_cli_session_stop_time(TimeVal(0, 0));
    set_is_cli_session_active(false);

    //
    // Set in "no-more" mode if a non-interactive client
    //
    if (! is_interactive())
	set_nomore_mode(true);
}

CliClient::~CliClient()
{
    string dummy_error_msg;

    stop_connection(dummy_error_msg);
    
    set_log_output(false);

    // Remove the input file descriptor from the eventloop
    if (_input_fd.is_valid()) {
	cli_node().eventloop().remove_ioevent_cb(_input_fd, IOT_READ);
    }

    // Close files and file descriptors
    if (_input_fd_file != NULL) {
	fclose(_input_fd_file);
	_input_fd_file = NULL;
	_input_fd.clear();
    }
    if (_output_fd_file != NULL) {
	fclose(_output_fd_file);
	_output_fd_file = NULL;
	_output_fd.clear();
    }
    if (_input_fd.is_valid()) {
	comm_close(_input_fd);
	_input_fd.clear();
    }
    if (_output_fd.is_valid()) {
	comm_close(_output_fd);
	_output_fd.clear();
    }

    if (_gl != NULL)
	_gl = del_GetLine(_gl);
    
    delete_pipe_all();
}

bool
CliClient::done() const
{
    if (_is_waiting_for_data)
	return (false);

    if (! _pending_input_data.empty())
	return (false);

    return (true);
}

int
CliClient::set_log_output(bool v)
{
    if (v) {
	if (is_log_output())
	    return (XORP_ERROR);		// Already added
	if (xlog_add_output_func(&CliNode::xlog_output, this) != 0)
	    return (XORP_ERROR);
	_is_log_output = true;
	return (XORP_OK);
    } else {
	if (! is_log_output())
	    return (XORP_ERROR);		// Was not added
	if (xlog_remove_output_func(&CliNode::xlog_output, this) != 0)
	    return (XORP_ERROR);
	_is_log_output = false;
	return (XORP_OK);
    }
    
    // NOTERACHED
    return (XORP_ERROR);
}

bool
CliClient::is_input_tty() const
{
    return (isatty(_input_fd) != 0);
}

bool
CliClient::is_output_tty() const
{
    return (isatty(_output_fd) != 0);
}

bool
CliClient::is_network() const
{
    return (_is_network);
}

void
CliClient::set_network_client(bool v)
{
    _is_network = v;
}

bool
CliClient::is_telnet() const
{
    //
    // TODO: XXX: for the time being we assume that all network connections
    // are telnet.
    //
    return (is_network());
}

bool
CliClient::is_interactive() const
{
    return (is_input_tty() || is_telnet());
}

CliPipe *
CliClient::add_pipe(const string& pipe_name)
{
    CliPipe *cli_pipe;
    
    cli_pipe = new CliPipe(pipe_name);
    if (cli_pipe->is_invalid()) {
	delete cli_pipe;
	return (NULL);
    }
    _pipe_list.push_back(cli_pipe);
    cli_pipe->set_cli_client(this);
    set_pipe_mode(true);
    
    return (cli_pipe);
}

CliPipe *
CliClient::add_pipe(const string& pipe_name, const list<string>& args_list)
{
    CliPipe *cli_pipe;
    
    cli_pipe = add_pipe(pipe_name);
    if (cli_pipe == NULL)
	return (NULL);
    
    // Add the list of arguments
    list<string>::const_iterator iter;
    for (iter = args_list.begin(); iter != args_list.end(); ++iter) {
	string arg = *iter;
	cli_pipe->add_pipe_arg(arg);
    }
    
    return (cli_pipe);
}

void
CliClient::delete_pipe_all()
{
    delete_pointers_list(_pipe_list);
    set_pipe_mode(false);
}

void
CliClient::append_page_buffer_line(const string& buffer_line)
{
    page_buffer().push_back(buffer_line);
}

void
CliClient::concat_page_buffer_line(const string& buffer_line, size_t pos)
{
    XLOG_ASSERT(pos < page_buffer().size());
    string& line = page_buffer()[pos];
    line += buffer_line;
}

// Process the line throught the pipes
void
CliClient::process_line_through_pipes(string& pipe_line)
{
    list<CliPipe*>::iterator iter;
    
    if (! is_pipe_mode())
	return;
    
    for (iter = _pipe_list.begin(); iter != _pipe_list.end(); ++iter) {
	CliPipe *cli_pipe = *iter;
	cli_pipe->process_func(pipe_line);
	if (pipe_line.empty())
	    break;
    }
}

// Set the page mode
void
CliClient::set_page_mode(bool v)
{
    const char *s;
    
    if (v) {
	// TRUE
	if (_is_page_mode)
	    return;
	_is_page_mode = v;
	
	//
	// Save the key bind commands
	//
	_action_name_up_arrow
	    = (s = gl_get_key_binding_action_name(gl(), "up")) ? (s) : "";
	_action_name_down_arrow
	    = (s = gl_get_key_binding_action_name(gl(), "down")) ? (s) : "";
	_action_name_tab
	    = (s = gl_get_key_binding_action_name(gl(), "\t")) ? (s) : "";
	_action_name_newline_n
	    = (s = gl_get_key_binding_action_name(gl(), "\n")) ? (s) : "";
	_action_name_newline_r
	    = (s = gl_get_key_binding_action_name(gl(), "\r")) ? (s) : "";
	// XXX: no need to save the spacebar binding
	//_action_name_spacebar
	//= (s = gl_get_key_binding_action_name(gl(), "\\\\\\040")) ? (s) : "";
	_action_name_ctrl_a
	    = (s = gl_get_key_binding_action_name(gl(), "^A")) ? (s) : "";
	_action_name_ctrl_b
	    = (s = gl_get_key_binding_action_name(gl(), "^B")) ? (s) : "";
	_action_name_ctrl_c
	    = (s = gl_get_key_binding_action_name(gl(), "^C")) ? (s) : "";
	_action_name_ctrl_d
	    = (s = gl_get_key_binding_action_name(gl(), "^D")) ? (s) : "";
	_action_name_ctrl_e
	    = (s = gl_get_key_binding_action_name(gl(), "^E")) ? (s) : "";
	_action_name_ctrl_f
	    = (s = gl_get_key_binding_action_name(gl(), "^F")) ? (s) : "";
	_action_name_ctrl_h
	    = (s = gl_get_key_binding_action_name(gl(), "^H")) ? (s) : "";
	_action_name_ctrl_k
	    = (s = gl_get_key_binding_action_name(gl(), "^K")) ? (s) : "";
	_action_name_ctrl_l
	    = (s = gl_get_key_binding_action_name(gl(), "^L")) ? (s) : "";
	_action_name_ctrl_m
	    = (s = gl_get_key_binding_action_name(gl(), "^M")) ? (s) : "";
	_action_name_ctrl_n
	    = (s = gl_get_key_binding_action_name(gl(), "^N")) ? (s) : "";
	_action_name_ctrl_p
	    = (s = gl_get_key_binding_action_name(gl(), "^P")) ? (s) : "";
	_action_name_ctrl_u
	    = (s = gl_get_key_binding_action_name(gl(), "^U")) ? (s) : "";
	_action_name_ctrl_x
	    = (s = gl_get_key_binding_action_name(gl(), "^X")) ? (s) : "";
	
	//
	// Set new binding
	//
	string bind_command;
	if (_action_name_up_arrow.size()) {
	    bind_command = "bind up user-event1";
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_down_arrow.size()) {
	    bind_command = "bind down user-event2";
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	// XXX: all bindings below are not used, but are needed to
	// avoid 'beeps'
	if (_action_name_tab.size()) {
	    bind_command = "bind \\t user-event3";
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_newline_n.size()) {
	    bind_command = "bind \\n user-event4";
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_newline_r.size()) {
	    bind_command = "bind \\r user-event4";
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_spacebar.size() || true) {	// XXX: always (re)bind
	    bind_command = "bind \\\\\\040 user-event4";
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_a.size()) {
	    bind_command = "bind ^A user-event4";
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_b.size()) {
	    bind_command = "bind ^B user-event4";
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_c.size()) {
	    bind_command = "bind ^C user-event4";
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_d.size()) {
	    bind_command = "bind ^D user-event4";
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_e.size()) {
	    bind_command = "bind ^E user-event4";
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_f.size()) {
	    bind_command = "bind ^F user-event4";
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_h.size()) {
	    bind_command = "bind ^H user-event4";
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_k.size()) {
	    bind_command = "bind ^K user-event4";
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_l.size()) {
	    bind_command = "bind ^L user-event4";
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_m.size()) {
	    bind_command = "bind ^M user-event4";
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_n.size()) {
	    bind_command = "bind ^N user-event4";
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_p.size()) {
	    bind_command = "bind ^P user-event4";
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_u.size()) {
	    bind_command = "bind ^U user-event4";
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_x.size()) {
	    bind_command = "bind ^X user-event4";
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	
	return;
    } else {
	// FALSE
	if (! _is_page_mode)
	    return;
	_is_page_mode = v;
	
	//
	// Restore the key bind commands
	//
	string bind_command;
	if (_action_name_up_arrow.size()) {
	    bind_command = "bind up " + _action_name_up_arrow;
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_down_arrow.size()) {
	    bind_command = "bind down " + _action_name_down_arrow;
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_tab.size()) {
	    bind_command = "bind \\t " + _action_name_tab;
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_newline_n.size()) {
	    bind_command = "bind \\n " + _action_name_newline_n;
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_newline_r.size()) {
	    bind_command = "bind \\r " + _action_name_newline_r;
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_spacebar.size() || true) {	// XXX: always (re)bind
	    bind_command = "bind \\\\\\040 " + _action_name_spacebar;
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_a.size()) {
	    bind_command = "bind ^A " + _action_name_ctrl_a;
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_b.size()) {
	    bind_command = "bind ^B " + _action_name_ctrl_b;
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_c.size()) {
	    bind_command = "bind ^C " + _action_name_ctrl_c;
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_d.size()) {
	    bind_command = "bind ^D " + _action_name_ctrl_d;
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_e.size()) {
	    bind_command = "bind ^E " + _action_name_ctrl_e;
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_f.size()) {
	    bind_command = "bind ^F " + _action_name_ctrl_f;
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_h.size()) {
	    bind_command = "bind ^H " + _action_name_ctrl_h;
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_k.size()) {
	    bind_command = "bind ^K " + _action_name_ctrl_k;
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_l.size()) {
	    bind_command = "bind ^L " + _action_name_ctrl_l;
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_m.size()) {
	    bind_command = "bind ^M " + _action_name_ctrl_m;
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_n.size()) {
	    bind_command = "bind ^N " + _action_name_ctrl_n;
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_p.size()) {
	    bind_command = "bind ^P " + _action_name_ctrl_p;
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_u.size()) {
	    bind_command = "bind ^U " + _action_name_ctrl_u;
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	if (_action_name_ctrl_x.size()) {
	    bind_command = "bind ^X " + _action_name_ctrl_x;
	    gl_configure_getline(gl(), bind_command.c_str(), NULL, NULL);
	}
	
	return;
    }
}

const string&
CliClient::page_buffer_line(size_t line_n) const
{
    XLOG_ASSERT(line_n < _page_buffer->size());
    return ((*_page_buffer)[line_n]);
}

size_t
CliClient::page_buffer_window_lines_n()
{
    if (page_buffer_lines_n() == 0)
	return (0);
    return (page_buffer2window_line_n(page_buffer_lines_n() - 1));
}

size_t
CliClient::page_buffer_last_window_line_n()
{
    if (page_buffer_last_line_n() == 0)
	return (0);
    return (page_buffer2window_line_n(page_buffer_last_line_n() - 1));
}

size_t
CliClient::page_buffer2window_line_n(size_t buffer_line_n)
{
    size_t i;
    size_t window_line_n = 0;

    for (i = 0; i <= buffer_line_n; i++) {
	window_line_n += window_lines_n(i);
    }

    return (window_line_n);
}

size_t
CliClient::window_lines_n(size_t buffer_line_n)
{
    bool has_newline = false;
    size_t window_line_n = 0;

    XLOG_ASSERT(buffer_line_n < _page_buffer->size());

    const string& line = page_buffer_line(buffer_line_n);

    // Get the line size, but don't count the trailing '\r' and '\n'
    size_t line_size = line.size();
    while (line_size > 0) {
	if ((line[line_size - 1] == '\r') || (line[line_size - 1] == '\n')) {
	    line_size--;
	    has_newline = true;
	    continue;
	}
	break;
    }

    window_line_n = (line_size / window_width())
	+ ((line_size % window_width())? (1) : (0));
    if ((line_size == 0) && has_newline)
	window_line_n++;

    return (window_line_n);
}

size_t
CliClient::calculate_first_page_buffer_line_by_window_size(
    size_t last_buffer_line_n,
    size_t max_window_size)
{
    size_t first_buffer_line_n;
    size_t window_size = 0;

    if (last_buffer_line_n == 0)
	return (0);

    first_buffer_line_n = last_buffer_line_n - 1;
    window_size += window_lines_n(first_buffer_line_n);
    while (window_size < max_window_size) {
	if (first_buffer_line_n == 0)
	    break;
	window_size += window_lines_n(first_buffer_line_n - 1);
	if (window_size > max_window_size)
	    break;
	first_buffer_line_n--;
    }

    return (first_buffer_line_n);
}

//
// Print a message. If a terminal connection, add '\r' before each '\n'
// (unless the previous character to print is indeed '\r').
//
// XXX: if we cli_print(""), this is EOF, and will "clear-out"
// any remaining data in _buffer_line through the pipes
int
CliClient::cli_print(const string& msg)
{
    int ret_value;
    string pipe_line, pipe_result;
    bool is_eof_input = false;
    bool is_incomplete_last_line = false;

    if (msg.size() == 0 || (msg[0] == '\0')) {
	is_eof_input = true;
    }

    // Test if the last line added to the page buffer was incomplete
    do {
	if (page_buffer().empty())
	    break;
	const string& last_line = page_buffer_line(page_buffer().size() - 1);
	if (last_line.empty())
	    break;
	if (last_line[last_line.size() - 1] == '\n')
	    break;
	is_incomplete_last_line = true;
	break;
    } while (false);
    
    // Process the data throught the pipe
    pipe_line += _buffer_line;
    _buffer_line = "";
    size_t i = 0;
    while (msg[i] != '\0') {
	pipe_line += msg[i];
	if (msg[i] == '\n') {
	    // Process the line throught the pipe
	    process_line_through_pipes(pipe_line);
	    pipe_result += pipe_line;
	    pipe_line = "";
	}
	i++;
    }

    if (pipe_line.size()) {
	if (! _pipe_list.empty()) {
	    if (is_eof_input) {
		process_line_through_pipes(pipe_line);
	    } else {
		_buffer_line += pipe_line;
		pipe_line = "";
	    }
	}
	pipe_result += pipe_line;
	pipe_line = "";
    }
    
    // If a terminal connection, add '\r' before each '\n'
    // (unless the previous character to print is indeed '\r').
    pipe_line = "";
    string output_string = "";
    for (i = 0; i < pipe_result.size(); i++) {
	if ((_client_type == CLIENT_TERMINAL) && (pipe_result[i] == '\n')) {
	    if ((! telnet_binary())
		&& (! ((i > 0) && (pipe_result[i-1] == '\r')))) {
		pipe_line += '\r';		// XXX: Carrige-return
	    }
	}
	pipe_line += pipe_result[i];
	if (is_page_buffer_mode()
	    && (_client_type == CLIENT_TERMINAL)
	    && (pipe_result[i] == '\n')) {
	    // Add the line to the buffer output
	    if (is_incomplete_last_line) {
		concat_page_buffer_line(pipe_line, page_buffer().size() - 1);
	    } else {
		append_page_buffer_line(pipe_line);
	    }
	    if ((page_buffer_window_lines_n() >= window_height())
		&& (! is_nomore_mode())) {
		set_page_mode(true);
	    } else {
		if (! is_incomplete_last_line)
		    incr_page_buffer_last_line_n();
		output_string += pipe_line;
	    }
	    pipe_line = "";
	    is_incomplete_last_line = false;
	}
    }

    if (pipe_line.size()) {
	// Insert the remaining partial line into the buffer
	if (is_page_buffer_mode() && (_client_type == CLIENT_TERMINAL)) {
	    // Add the line to the buffer output
	    if (is_incomplete_last_line) {
		concat_page_buffer_line(pipe_line, page_buffer().size() - 1);
	    } else {
		append_page_buffer_line(pipe_line);
	    }
	    if ((page_buffer_window_lines_n() >= window_height())
		&& (! is_nomore_mode())) {
		set_page_mode(true);
	    } else {
		if (! is_incomplete_last_line)
		    incr_page_buffer_last_line_n();
	    }
	}
    }

    if (! (is_page_buffer_mode() && is_page_mode())) {
	if (pipe_line.size())
	    output_string += pipe_line;	// XXX: the remaining partial line
    }
    
    ret_value = output_string.size();
    // if (! (is_page_buffer_mode() && is_page_mode()))
    if (output_string.size())
	ret_value = fprintf(_output_fd_file, "%s", output_string.c_str());
    
    return (ret_value);
}

// Return: %XORP_OK on success, otherwise %XORP_ERROR.
int
CliClient::cli_flush()
{
    if ((_output_fd_file != NULL) && (fflush(_output_fd_file) == 0))
	return (XORP_OK);
    return (XORP_ERROR);
}

void
CliClient::set_current_cli_command(CliCommand *cli_command)
{
    _current_cli_command = cli_command;
    if (cli_command->cd_prompt().size() > 0)
	set_current_cli_prompt(cli_command->cd_prompt());
}

void
CliClient::set_current_cli_prompt(const string& cli_prompt)
{
    _current_cli_prompt = cli_prompt;
    gl_replace_prompt(gl(), _current_cli_prompt.c_str());
}

//
// Process one input character while in "page mode"
//
int
CliClient::process_char_page_mode(uint8_t val)
{
    string restore_cli_prompt = current_cli_prompt();	// The current prompt
    bool old_page_buffer_mode = is_page_buffer_mode();
    
    //
    // Reset the line and clear the current prompt
    //
    gl_redisplay_line(gl());		// XXX: must be first
    gl_reset_line(gl());
    set_current_cli_prompt("");
    gl_reset_line(gl());
    cli_flush();
    
    
    //
    // Page commands
    //

    //
    // Print help
    //
    if ((val == 'h')) {
	if (! is_help_mode()) {
	    set_help_mode(true);
	    _is_page_buffer_mode = &_is_help_buffer_mode;
	    _page_buffer = &_help_buffer;
	    _page_buffer_last_line_n = &_help_buffer_last_line_n;
	    set_page_buffer_mode(true);
#define CLI_HELP_STRING							\
"                   SUMMARY OF MORE COMMANDS\n"				\
"\n"									\
"    -- Get Help --\n"							\
"  h                 *  Display this help.\n"				\
"\n"									\
"    -- Scroll Down --\n"						\
"  Enter   Return  j *  Scroll down one line.\n"			\
"  ^M  ^N  DownArrow\n"							\
"  Tab d   ^D  ^X    *  Scroll down one-half screen.\n"			\
"  Space   ^F        *  Scroll down one whole screen.\n"		\
"  ^E  G             *  Scroll down to the bottom of the output.\n"	\
"  N                 *  Display the output all at once instead of one\n"\
"                       screen at a time. (Same as specifying the\n"	\
"                       | no-more command.)\n"				\
"\n"									\
"    -- Scroll Up --\n"							\
"  k   ^H  ^P        *  Display the previous line of output.\n"		\
"  UpArrow\n"								\
"  u   ^U            *  Scroll up one-half screen.\n"			\
"  b   ^B            *  Scroll up one whole screen.\n"			\
"  ^A  g             *  Scroll up to the top of the output.\n"		\
"\n"									\
"    -- Misc Commands --\n"						\
"  ^L                *  Redraw the output on the screen.\n"		\
"  q   Q   ^C  ^K    *  Interrupt the display of output.\n"		\
"\n"

	    cli_print(CLI_HELP_STRING);
	    set_page_buffer_mode(false);
	}
	goto redisplay_screen_label;
    }
    
    //
    // Interrupt the display of output
    //
    if ((val == 'q')
	|| (val == 'Q')
	|| (val == CHAR_TO_CTRL('c'))
	|| (val == CHAR_TO_CTRL('k'))) {

	if (is_waiting_for_data()) {
	    interrupt_command();
	}
	goto exit_page_mode_label;
    }
    
    //
    // Scroll down one line
    //
    if ((val == '\n')
	|| (val == '\r')
	|| (val == 'j')
	|| (val == CHAR_TO_CTRL('m'))
	|| (val == CHAR_TO_CTRL('n'))
	|| (gl_get_user_event(gl()) == 2)) {
	if (page_buffer_last_line_n() < page_buffer_lines_n()) {
	    set_page_buffer_mode(false);
	    cli_print(page_buffer_line(page_buffer_last_line_n()));
	    set_page_buffer_mode(old_page_buffer_mode);
	    incr_page_buffer_last_line_n();
	}
	goto redisplay_line_label;
    }
    
    //
    // Scroll down one-half screen
    //
    if ((val == '\t')
	|| (val == 'd')
	|| (val == CHAR_TO_CTRL('d'))
	|| (val == CHAR_TO_CTRL('x'))) {
	for (size_t i = 0; i <= window_height() / 2; ) {
	    if (page_buffer_last_line_n() >= page_buffer_lines_n())
		break;
	    i += window_lines_n(page_buffer_last_line_n());
	    if (i > window_height() / 2)
		break;
	    set_page_buffer_mode(false);
	    cli_print(page_buffer_line(page_buffer_last_line_n()));
	    set_page_buffer_mode(old_page_buffer_mode);
	    incr_page_buffer_last_line_n();
	}
	goto redisplay_line_label;
    }
    
    //
    // Scroll down one whole screen
    //
    if ((val == ' ')
	|| (val == CHAR_TO_CTRL('f'))) {
	for (size_t i = 0; i <= window_height() - 1; ) {
	    if (page_buffer_last_line_n() >= page_buffer_lines_n())
		break;
	    i += window_lines_n(page_buffer_last_line_n());
	    if (i > window_height() - 1)
		break;
	    set_page_buffer_mode(false);
	    cli_print(page_buffer_line(page_buffer_last_line_n()));
	    set_page_buffer_mode(old_page_buffer_mode);
	    incr_page_buffer_last_line_n();
	}
	goto redisplay_line_label;
    }
    
    //
    // Scroll down to the bottom of the output
    //
    if ((val == 'G')
	|| (val == CHAR_TO_CTRL('e'))) {
	set_page_buffer_last_line_n(page_buffer_lines_n());
	goto redisplay_screen_label;
    }
    
    //
    // Display the output all at once instead of oen screen at a time.
    // (Same as specifying the "| no-more" command.)
    //
    if ((val == 'N')) {
	while (page_buffer_last_line_n() < page_buffer_lines_n()) {
	    set_page_buffer_mode(false);
	    cli_print(page_buffer_line(page_buffer_last_line_n()));
	    set_page_buffer_mode(old_page_buffer_mode);
	    incr_page_buffer_last_line_n();
	}
	// TODO: do we want to exit the page mode at the end?
	// If "yes", then the line below should be changed to
	// goto exit_page_mode_label;
	goto redisplay_line_label;
    }
    
    //
    // Display the previous line of output
    //
    if ((val == 'k')
	|| (val == CHAR_TO_CTRL('h'))
	|| (val == CHAR_TO_CTRL('p'))
	|| (gl_get_user_event(gl()) == 1)) {
	if (page_buffer_last_line_n() > 0)
	    decr_page_buffer_last_line_n();
	goto redisplay_screen_label;
    }
    
    //
    // Scroll up one-half screen
    //
    if ((val == 'u')
	|| (val == CHAR_TO_CTRL('u'))) {
	if (page_buffer_last_line_n() > 0) {
	    size_t start_window_line = calculate_first_page_buffer_line_by_window_size(
		page_buffer_last_line_n(), window_height() / 2);
	    set_page_buffer_last_line_n(start_window_line);
	}
	goto redisplay_screen_label;
    }
    
    //
    // Scroll up one whole screen
    //
    if ((val == 'b')
	|| (val == CHAR_TO_CTRL('b'))) {
	if (page_buffer_last_line_n() > 0) {
	    size_t start_window_line = calculate_first_page_buffer_line_by_window_size(
		page_buffer_last_line_n(), window_height() - 1);
	    set_page_buffer_last_line_n(start_window_line);
	}
	goto redisplay_screen_label;
    }
    
    //
    // Scroll up to the top of the output
    //
    if ((val == 'g')
	|| (val == CHAR_TO_CTRL('a'))) {
	set_page_buffer_last_line_n(0);
	goto redisplay_screen_label;
    }
    
    //
    // Redraw the output of the screen
    //
    if ((val == CHAR_TO_CTRL('l'))) {
    redisplay_screen_label:
	size_t i, start_window_line = 0;
	set_page_buffer_mode(false);
	// XXX: clean-up the previous window
	for (i = 0; i < window_height() - 1; i++)
	    cli_print("\n");
	if (page_buffer_last_line_n() > 0) {
	    start_window_line = calculate_first_page_buffer_line_by_window_size(
		page_buffer_last_line_n(), window_height() - 1);
	}
	set_page_buffer_last_line_n(start_window_line);
	for (i = 0; i <= window_height() - 1; ) {
	    if (page_buffer_last_line_n() >= page_buffer_lines_n())
		break;
	    i += window_lines_n(page_buffer_last_line_n());
	    if (i > window_height() - 1)
		break;
	    cli_print(page_buffer_line(page_buffer_last_line_n()));
	    incr_page_buffer_last_line_n();
	}
	// XXX: fill-up the rest of the window
	for ( ; i < window_height() - 1; i++)
	    cli_print("\n");
	set_page_buffer_mode(old_page_buffer_mode);
	goto redisplay_line_label;
    }
    
    goto redisplay_line_label;
    
 exit_page_mode_label:
    reset_page_buffer();
    if (is_interactive())
	set_nomore_mode(false);
    if (! is_help_mode()) {
	// Exit the page mode
	set_page_mode(false);
	set_buff_curpos(0);
	gl_reset_line(gl());
	command_buffer().reset();
	restore_cli_prompt = current_cli_command()->cd_prompt();
    } else {
	// Exit the help page mode
	set_help_mode(false);
	_is_page_buffer_mode = &_is_output_buffer_mode;
	_page_buffer = &_output_buffer;
	_page_buffer_last_line_n = &_output_buffer_last_line_n;
	goto redisplay_screen_label;
    }
    // FALLTHROUGH
    
 redisplay_line_label:
    cli_flush();
    if (is_page_mode()) {
	if (page_buffer_last_line_n() < page_buffer_lines_n())
	    restore_cli_prompt = " --More-- ";
	else
	    restore_cli_prompt = " --More-- (END) ";
    }
    set_current_cli_prompt(restore_cli_prompt);
    gl_redisplay_line(gl());
    cli_flush();
    return (XORP_OK);
}

void
CliClient::post_process_command()
{
    //
    // Test if we are waiting for the result from a processor
    //
    if (is_waiting_for_data()) {
	// We are waiting for the result; silently return.
	return;
    }
    
    //
    // Reset the state for the currently executed command
    //
    _executed_cli_command = NULL;
    _executed_cli_command_name.clear();
    _executed_cli_command_args.clear();

    //
    // Pipe-process the result
    //
    string final_string = "";
    
    cli_print("");		// XXX: EOF: clear-out the pipe
    list<CliPipe*>::iterator iter;
    for (iter = _pipe_list.begin(); iter != _pipe_list.end(); ++iter) {
	CliPipe *cli_pipe = *iter;
	cli_pipe->process_func(final_string);
	cli_pipe->eof_func(final_string);
    }
    if (final_string.size()) {
	bool old_pipe_mode = is_pipe_mode();
	set_pipe_mode(false);
	cli_print(final_string);
	set_pipe_mode(old_pipe_mode);
    }
    if (is_hold_mode()) {
	set_page_mode(true);
	set_hold_mode(false);
    }
    delete_pipe_all();
    
    if (! is_page_mode())
	reset_page_buffer();
    
    //
    // Page-related state
    //
    set_page_buffer_mode(false);
    if (is_page_mode()) {
	if (page_buffer_last_line_n() < page_buffer_lines_n())
	    set_current_cli_prompt(" --More-- ");
	else
	    set_current_cli_prompt(" --More-- (END) ");
    } else {
	reset_page_buffer();
	if (is_interactive())
	    set_nomore_mode(false);
    }
    
    //
    // Reset buffer, cursor, prompt
    //
    command_buffer().reset();
    set_buff_curpos(0);
    if (! is_prompt_flushed())
	cli_print(current_cli_prompt());
    set_prompt_flushed(false);
    cli_flush();

    //
    // Process the pending input data (if any)
    //
    if (! _pending_input_data.empty()) {
	schedule_process_input_data();
    }
}

void
CliClient::flush_process_command_output()
{
    //
    // Test if we are waiting for the result from a processor
    //
    if (! is_waiting_for_data()) {
	// We are not waiting for the result; silently return.
	return;
    }

    if (is_help_mode()) {
	// We don't want the output while in help mode
	return;
    }

    //
    // Page-related state
    //
    if (is_page_mode() && ! is_prompt_flushed()) {
	string restore_cli_prompt;
	bool old_page_buffer_mode = is_page_buffer_mode();

	set_page_buffer_mode(false);
	if (page_buffer_last_line_n() < page_buffer_lines_n())
	    restore_cli_prompt = " --More-- ";
	else
	    restore_cli_prompt = " --More-- (END) ";
	set_current_cli_prompt(restore_cli_prompt);
	cli_print(current_cli_prompt());
	cli_flush();
	set_page_buffer_mode(old_page_buffer_mode);
	set_prompt_flushed(true);
    }
}

//
// Process one input character
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
// XXX: returning %XORP_ERROR is also an indication that the connection
// has been closed.
int
CliClient::process_char(const string& line, uint8_t val, bool& stop_processing)
{
    int gl_buff_curpos = gl_get_buff_curpos(gl());
    int ret_value = XORP_OK;
    
    stop_processing = false;

    if ((val == '\n') || (val == '\r')) {
	// New command
	XLOG_ASSERT(is_waiting_for_data() == false);
	set_page_buffer_mode(true);
	process_command(line);
	post_process_command();

	//
	// Set the flag to stop processing pending input data while
	// processing previous commands.
	//
	if (is_waiting_for_data()) {
	    stop_processing = true;
	}

	return (XORP_OK);
    }
    
    if (val == '?') {
	// Command-line help
	//set_page_buffer_mode(true);
	// TODO: add "page" support
	command_line_help(line, gl_buff_curpos, true);
	//set_page_buffer_mode(false);
	//if (is_page_mode()) {
	//if (page_buffer_last_line_n() < page_buffer_lines_n())
	//set_current_cli_prompt(" --More-- ");
	//else
	//set_current_cli_prompt(" --More-- (END) ");
	//} else {
	//reset_page_buffer();
	//}
	//cli_print(current_cli_prompt());
	//command_buffer().reset();
	//set_buff_curpos(0);
	return (XORP_OK);
    }
    
    //
    // XXX: The (val == ' ') and 'Ctrl-C' cases are handled by the
    // parent function.
    //

    // All other characters which we need to print
    // Store the line in the command buffer
    command_buffer().reset();
    ret_value = XORP_OK;
    for (int i = 0; line[i] != '\0'; i++) {
	ret_value = command_buffer().add_data(line[i]);
	if (ret_value != XORP_OK)
	    break;
    }
    if (ret_value == XORP_OK)
	ret_value = command_buffer().add_data('\0');
    if (ret_value != XORP_OK) {
	// This client is sending too much data. Kick it out!
	// TODO: print more informative message about the
	// client: E.g. where it came from, etc.
	XLOG_WARNING("Removing client (input fd = %s output fd = %s "
		     "family = %d): "
		     "data buffer full",
		     input_fd().str().c_str(),
		     output_fd().str().c_str(),
		     cli_node().family());
	return (XORP_ERROR);
    }
    set_buff_curpos(gl_buff_curpos);
    
    return (XORP_OK);
}


/**
 * CliClient::command_line_help:
 * @line: The current command line.
 * @word_end: The cursor position.
 * @remove_last_input_char: If true, then remove the last input character.
 * 
 * Print the help for the same-line command.
 **/
void
CliClient::command_line_help(const string& line, int word_end,
			     bool remove_last_input_char)
{
    CliCommand *curr_cli_command = _current_cli_command;
    set<string> command_help_strings;
    bool is_found = false;
    
    if (remove_last_input_char)
	word_end--;			// XXX: exclude the '?' character

    list<CliCommand *>::iterator iter;
    for (iter = curr_cli_command->child_command_list().begin();
	 iter != curr_cli_command->child_command_list().end();
	 ++iter) {
	CliCommand *tmp_cli_command = *iter;
	if (tmp_cli_command->find_command_help(line.c_str(), word_end,
					       command_help_strings))
	    is_found = true;
    }
    if (is_found) {
	cli_print("\nPossible completions:\n");
	set<string>::const_iterator iter;
	for (iter = command_help_strings.begin();
	     iter != command_help_strings.end();
	     ++iter) {
	    cli_print(*iter);
	}
    } else {
	string token_line = string(line, 0, word_end);
	token_line = strip_empty_spaces(token_line);
	cli_print(c_format("\nsyntax error, command \"%s\" is not recognized.\n",
			   token_line.c_str()));
    }
    
    gl_redisplay_line(gl());
    if (remove_last_input_char) {
	// XXX: Move the cursor over the '?'
	gl_place_cursor(gl(), gl_get_buff_curpos(gl()) - 1);
	cli_print(" \b");	// XXX: A hack to delete the '?'
    }
}

bool
CliClient::is_multi_command_prefix(const string& command_line)
{
    return (_current_cli_command->is_multi_command_prefix(command_line));
}

int
CliClient::process_command(const string& command_line)
{
    string token, token_line;
    CliCommand *parent_cli_command = current_cli_command();
    CliCommand *child_cli_command = NULL;
    int syntax_error_offset_next = current_cli_prompt().size();
    int syntax_error_offset_prev = syntax_error_offset_next;
    int i, old_len, new_len;
    vector<string> command_global_name;
    bool found_type_match_cb = false;
    
    token_line = command_line;
    new_len = token_line.size();
    old_len = new_len;

    if (parent_cli_command != NULL)
	command_global_name = parent_cli_command->global_name();

    for (token = pop_token(token_line);
	 ! token.empty();
	 token = pop_token(token_line)) {

	if (token != "|") {
	    child_cli_command = parent_cli_command->command_find(token);

	    new_len = token_line.size();
	    syntax_error_offset_prev = syntax_error_offset_next;
	    syntax_error_offset_next += old_len - new_len;
	    old_len = new_len;

	    if (child_cli_command != NULL
		&& (! child_cli_command->is_command_argument())) {
		parent_cli_command = child_cli_command;
		// Add the token to the command
		found_type_match_cb |= child_cli_command->has_type_match_cb();

		if (! found_type_match_cb)
		    command_global_name = child_cli_command->global_name();
		else
		    command_global_name.push_back(copy_token(token));
		continue;
	    }
	
	    if (parent_cli_command->has_cli_process_callback()) {
		// The parent command has processing function, so the rest
		// of the tokens could be arguments for that function
		child_cli_command = parent_cli_command;
	    }
	}

	// Put-back the token and process the arguments after the loop
	token_line = copy_token(token) + token_line;
	break;
    }
    
    if (parent_cli_command->has_cli_process_callback()) {
	// Process the rest of the tokens as arguments for this function
	vector<string> args_vector;
	bool is_process_func_arguments = true;
	bool is_pipe_command_arguments = false;
	bool pipe_command_empty = false;	// true if empty pipe command
	string pipe_command_name = "";
	list<string> pipe_command_args_list;
	for (token = pop_token(token_line);
	     ! token.empty();
	     token = pop_token(token_line)) {
	    if (token == "|") {
		if ((! parent_cli_command->can_pipe())
		    || (parent_cli_command->cli_command_pipe() == NULL)) {
		    // We cannot use pipe with this command
		    goto print_syntax_error_label;
		}
		
		// Start of a pipe command
		is_process_func_arguments = false;
		is_pipe_command_arguments = false;
		pipe_command_empty = true;
		if (pipe_command_name.size()) {
		    // Add the previous pipe command
		    add_pipe(pipe_command_name, pipe_command_args_list);
		    pipe_command_name = "";
		    pipe_command_args_list.clear();
		}
		continue;
	    }
	    if (is_process_func_arguments) {
		// Arguments for the processing command
		args_vector.push_back(token);
		continue;
	    }
	    if (! is_pipe_command_arguments) {
		// The pipe command name
		is_pipe_command_arguments = true;
		pipe_command_empty = false;
		pipe_command_name = token;
		continue;
	    }
	    // The pipe command arguments
	    pipe_command_args_list.push_back(token);
	}
	if (pipe_command_name.size()) {
	    // Add the last pipe command
	    add_pipe(pipe_command_name, pipe_command_args_list);
	    pipe_command_name = "";
	    pipe_command_args_list.clear();
	}
	
	if (pipe_command_empty) {
	    // Empty pipe command
	    parent_cli_command = parent_cli_command->cli_command_pipe();
	    goto print_syntax_error_label;
	}
	
	// Run the command function
	{
	    int ret_value;
	    string final_string = "";
	    bool is_error = false;
	    string error_msg;
	    
	    if (parent_cli_command->default_nomore_mode())
		set_nomore_mode(true);
	    list<CliPipe*>::iterator iter;
	    for (iter = _pipe_list.begin(); iter != _pipe_list.end(); ++iter) {
		CliPipe *cli_pipe = *iter;
		if (cli_pipe->start_func(final_string, error_msg) != XORP_OK) {
		    is_error = true;
		    break;
		}
	    }
	    if (is_error) {
		// Stop the started pipes
		string error_msg2;
		while (iter != _pipe_list.begin()) {
		    --iter;
		    CliPipe *cli_pipe = *iter;
		    cli_pipe->stop_func(error_msg2);
		}
		if (is_interactive())
		    set_nomore_mode(false);
		cli_print(c_format("ERROR: %s\n", error_msg.c_str()));
		return (XORP_ERROR);
	    }

	    if (final_string.size()) {
		bool old_pipe_mode = is_pipe_mode();
		set_pipe_mode(false);
		cli_print(final_string);
		set_pipe_mode(old_pipe_mode);
	    }
	    final_string = "";
	    
	    _executed_cli_command = parent_cli_command;
	    _executed_cli_command_name = command_global_name;
	    _executed_cli_command_args = args_vector;
	    ret_value = parent_cli_command->_cli_process_callback->dispatch(
		parent_cli_command->server_name(),
		cli_session_term_name(),
		cli_session_session_id(),
		_executed_cli_command_name,
		_executed_cli_command_args);
	    return (ret_value);
	}
    }
    
    // The rest of the tokens (if any) cannot be processed as arguments
    
    // Test if we can "cd" to this function.
    token = pop_token(token_line);
    if (token.empty()) {
	if (parent_cli_command->allow_cd()) {
	    // Set the current command level
	    set_current_cli_command(parent_cli_command);
	    return (XORP_OK);
	}
	
	// Error. Will print the list of child commands (done below).
    }
    
 print_syntax_error_label:
    //
    // If there are more tokens, the first one has to be a sub-command.
    // However, there wasn't a match in our search, hence it has
    // to be an error.
    // Further, there was no processing function, hence those cannot
    // be command arguments.
    //
    syntax_error_offset_next -= token.size();

    // Unknown command
    if (parent_cli_command == current_cli_command()) {
	cli_print(c_format("%*s%s\n", syntax_error_offset_next, " ", "^"));
	cli_print("unknown command.\n");
	return (XORP_ERROR);
    }

    if (token.empty())
	syntax_error_offset_next++;

    // Command that cannot be executed
    if (parent_cli_command->child_command_list().empty()) {
	string cmd_name = token_vector2line(parent_cli_command->global_name());
	if (token.empty()) {
	    cli_print(c_format("syntax error, command \"%s\" is not executable.\n",
			       cmd_name.c_str()));
	} else {
	    cli_print(c_format("syntax error, command \"%s\" cannot be executed with argument \"%s\".\n",
			       cmd_name.c_str(),
			       token.c_str()));
	}
	return (XORP_ERROR);
    }

    // Command with invalid sub-parts
    cli_print(c_format("%*s%s\n", syntax_error_offset_next, " ", "^"));
    cli_print("syntax error, expecting");
    if (parent_cli_command->child_command_list().size() > 4) {
	// TODO: replace the hard-coded "4" with a #define or a parameter.
	cli_print(" <command>.\n");
	return (XORP_ERROR);
    }
    list<CliCommand *>::iterator iter;
    i = 0;
    for (iter = parent_cli_command->child_command_list().begin();
	 iter != parent_cli_command->child_command_list().end();
	 ++iter) {
	child_cli_command = *iter;
	if (i > 0) {
	    cli_print(",");
	    if ((size_t)(i + 1)
		== parent_cli_command->child_command_list().size())
		cli_print(" or");
	}
	i++;
	cli_print(c_format(" `%s'", child_cli_command->name().c_str()));
    }
    cli_print(".\n");
    
    return (XORP_ERROR);
}

void
CliClient::interrupt_command()
{
    if (! is_waiting_for_data())
	goto cleanup_label;

    if ((_executed_cli_command == NULL)
	|| (! _executed_cli_command->has_cli_interrupt_callback())) {
	goto cleanup_label;
    }

    _executed_cli_command->_cli_interrupt_callback->dispatch(
	_executed_cli_command->server_name(),
	cli_session_term_name(),
	cli_session_session_id(),
	_executed_cli_command_name,
	_executed_cli_command_args);

 cleanup_label:
    // Reset everything about the command
    _executed_cli_command = NULL;
    _executed_cli_command_name.clear();
    _executed_cli_command_args.clear();
    delete_pipe_all();
    set_pipe_mode(false);
    set_hold_mode(false);
    set_page_mode(false);
    reset_page_buffer();
    set_page_buffer_mode(false);
    if (is_interactive())
	set_nomore_mode(false);

    if (is_waiting_for_data()) {
	cli_print("\n");            // XXX: new line
	cli_print("Command interrupted!\n");
    }

    //
    // Ignore current line, reset buffer, line, cursor, prompt
    //
    if (current_cli_command() != NULL) {
	set_current_cli_prompt(current_cli_command()->cd_prompt());
    }
    cli_print("\n");			// XXX: new line
    gl_redisplay_line(gl());
    gl_reset_line(gl());
    set_buff_curpos(0);
    command_buffer().reset();
    cli_flush();

    set_prompt_flushed(false);
    set_is_waiting_for_data(false);
}

int
CliClient::command_completion_func(WordCompletion *cpl, void *data,
				   const char *line, int word_end)
{
    int ret_value = 1;
    CliClient *cli_client = reinterpret_cast<CliClient*>(data);
    CliCommand *curr_cli_command = cli_client->_current_cli_command;
    list<CliCommand *> cli_command_match_list;
    set<string> type_names, no_type_names;
    
    if (cpl == NULL)
	return (1);
    
    list<CliCommand *>::iterator iter;
    for (iter = curr_cli_command->child_command_list().begin();
	 iter != curr_cli_command->child_command_list().end();
	 ++iter) {
	CliCommand *tmp_cli_command = *iter;
	if (! tmp_cli_command->has_cli_completion_func())
	    continue;
	if (tmp_cli_command->_cli_completion_func(tmp_cli_command,
						  cpl, NULL, line, word_end,
						  cli_command_match_list)) {
	    ret_value = 0;		// XXX: there was a completion
	}
    }
    if (curr_cli_command->can_pipe()
	&& (curr_cli_command->cli_command_pipe() != NULL)) {
	// Add the pipe completions
	if (curr_cli_command->_cli_completion_func(
                curr_cli_command->cli_command_pipe(),
		cpl, NULL, line, word_end,
		cli_command_match_list)) {
	    ret_value = 0;
	}
    }

    //
    // Separate the type-match commands from the rest
    //
    for (iter = cli_command_match_list.begin();
	 iter != cli_command_match_list.end();
	 ++iter) {
	CliCommand *tmp_cli_command = *iter;
	if (tmp_cli_command->has_type_match_cb())
	    type_names.insert(tmp_cli_command->name());
	else
	    no_type_names.insert(tmp_cli_command->name());
    }

    if (no_type_names.size() > 1) {
	// Prepare and print the initial message(s)
	string token_line = string(line, word_end);
	string token;
	
	// Get the lastest token
	do {
	    string next_token = pop_token(token_line);
	    if (next_token.empty())
		break;
	    token = next_token;
	} while (true);
	
	cli_client->cli_print(c_format("\n`%s' is ambiguous.", token.c_str()));
	cli_client->cli_print("\nPossible completions:");
    } else {
	if (type_names.size() > 0) {
	    cli_client->command_line_help(line, word_end, false);
	}
    }
    
    if (ret_value != 0) {
	cpl_record_error(cpl, "Not a XORP command!");
    }
    
    return (ret_value);
}
