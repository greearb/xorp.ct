// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/cli/cli_client.cc,v 1.12 2003/08/27 23:15:30 pavlin Exp $"


//
// CLI (Command-Line Interface) implementation for XORP.
//


#include "cli_module.h"
#include "cli_private.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/token.hh"
#include "libxorp/utils.hh"
#include "libcomm/comm_api.h"
#include "cli_client.hh"
#include "cli_command_pipe.hh"




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
CliClient::CliClient(CliNode& init_cli_node, int fd)
    : _cli_node(init_cli_node),
      _cli_fd(fd),
      _command_buffer(1024),
      _telnet_sb_buffer(1024),
      _cli_session_from_address(_cli_node.family())
{
    _cli_fd_file_read = NULL;
    _cli_fd_file_write = NULL;
    _client_type = CLIENT_TERMINAL;	// XXX: default is terminal
    
    _gl = NULL;
    
    _telnet_iac = false;
    _telnet_sb = false;
    _telnet_dont = false;
    _telnet_do = false;
    _telnet_wont = false;
    _telnet_will = false;
    _telnet_binary = false;

    // TODO: use parameters instead
    _window_width = 80;
    _window_height = 25;
    
    _is_modified_stdio_termios_icanon = false;
    _is_modified_stdio_termios_echo = false;
    _is_modified_stdio_termios_isig = false;
    
    set_current_cli_command(_cli_node.cli_command_root());
    set_current_cli_prompt(current_cli_command()->cd_prompt());
    _buff_curpos = 0;
    
    _is_pipe_mode = false;
    _is_nomore_mode = false;
    _is_hold_mode = false;
    
    _is_page_mode = false;
    
    _is_output_buffer_mode = false;
    _output_buffer_last_line = 0;
    
    _is_help_buffer_mode = false;
    _help_buffer_last_line = 0;
    _is_help_mode = false;
    
    _is_page_buffer_mode = &_is_output_buffer_mode;
    _page_buffer = &_output_buffer;
    _page_buffer_last_line = &_output_buffer_last_line;
    
    
    //
    // Session info state
    //
    set_cli_session_user_name("unknown_user");
    set_cli_session_term_name("unknown_terminal");
    set_cli_session_session_id(~0);	// XXX: ~0 has no particular meaning
    set_cli_session_start_time(TimeVal(0, 0));
    set_cli_session_stop_time(TimeVal(0, 0));
    set_is_cli_session_active(false);
    
    //
    // Log-related state
    //
    _is_log_output = false;

    //
    // Server communication state
    //
    _is_waiting_for_data = false;
}

CliClient::~CliClient()
{
    stop_connection();
    
    set_log_output(false);
    
    if (_cli_fd >= 0) {
	cli_node().eventloop().remove_selector(_cli_fd);
	comm_close(_cli_fd);
    }
    if (_gl != NULL)
	_gl = del_GetLine(_gl);
    
    delete_pipe_all();
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

CliPipe *
CliClient::add_pipe(const char *pipe_name)
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
CliClient::add_pipe(const char *pipe_name, const list<string>args_list)
{
    CliPipe *cli_pipe;
    
    cli_pipe = add_pipe(pipe_name);
    if (cli_pipe == NULL)
	return (NULL);
    
    // Add the list of arguments
    list<string>::const_iterator iter;
    for (iter = args_list.begin(); iter != args_list.end(); ++iter) {
	string arg = *iter;
	cli_pipe->add_pipe_arg(arg.c_str());
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
CliClient::add_page_buffer_line(const char *line)
{
    string string_line = line;
    page_buffer().push_back(string_line);
}

// Process the line throught the pipes
void
CliClient::process_line_through_pipes(string &pipe_line)
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
    const char* buf_ptr = msg.c_str();
    string pipe_line, pipe_result;
    bool eof_input_bool = false;

    if (msg.size() == 0 || (buf_ptr[0] == '\0')) {
	eof_input_bool = true;
    }
    
    // Process the data throught the pipe
    pipe_line += _buffer_line;
    _buffer_line = "";
    size_t i = 0;
    while (buf_ptr[i] != '\0') {
	pipe_line += buf_ptr[i];
	if (buf_ptr[i] == '\n') {
	    // Process the line throught the pipe
	    process_line_through_pipes(pipe_line);
	    pipe_result += pipe_line;
	    pipe_line = "";
	}
	i++;
    }

    if (pipe_line.size()) {
	if (! _pipe_list.empty()) {
	    if (eof_input_bool) {
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
#if 0	    // TODO: need to modify to handle longer lines
	    size_t window_line_n = pipe_line.size() / window_width()
		+ (pipe_line.size() % window_width)? (1) : (0);
#endif
	    add_page_buffer_line(pipe_line.c_str());
	    if ((page_buffer_lines_n() >= window_height())
		&& (! is_nomore_mode())) {
		set_page_mode(true);
	    } else {
		incr_page_buffer_last_line();
		output_string += pipe_line;
	    }
	    pipe_line = "";
	}
    }
    if (! (is_page_buffer_mode() && is_page_mode())) {
	if (pipe_line.size())
	    output_string += pipe_line;	// XXX: the remaining partial line
    }
    
    ret_value = output_string.size();
    // if (! (is_page_buffer_mode() && is_page_mode()))
    if (output_string.size())
	ret_value = fprintf(_cli_fd_file_write, "%s", output_string.c_str());
    
    return (ret_value);
}

// Return: %XORP_OK on success, otherwise %XORP_ERROR.
int
CliClient::cli_flush()
{
    if ((_cli_fd_file_write != NULL) && (fflush(_cli_fd_file_write) == 0))
	return (XORP_OK);
    return (XORP_ERROR);
}

void
CliClient::set_current_cli_command(CliCommand *cli_command)
{
    _current_cli_command = cli_command;
    if (strlen(cli_command->cd_prompt()) > 0)
	set_current_cli_prompt(cli_command->cd_prompt());
}

void
CliClient::set_current_cli_prompt(const char *cli_prompt)
{
    if (cli_prompt == NULL) {
	_current_cli_prompt[0] = '\0';
    } else {
	strncpy(_current_cli_prompt, cli_prompt,
		sizeof(_current_cli_prompt) -1);
	_current_cli_prompt[sizeof(_current_cli_prompt) - 1] = '\0';
    }
    gl_replace_prompt(gl(), _current_cli_prompt);
}

//
// Process one input character while in "page mode"
//
int
CliClient::process_char_page_mode(uint8_t val)
{
    string restore_cli_prompt = current_cli_prompt();	// The current prompt
    
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
	    _page_buffer_last_line = &_help_buffer_last_line;
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

	    cli_print(c_format("%s", CLI_HELP_STRING));
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
	if (page_buffer_last_line() < page_buffer_lines_n()) {
	    cli_print(c_format("%s", page_buffer()[page_buffer_last_line()].c_str()));
	    incr_page_buffer_last_line();
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
	for (size_t i = 0; i < window_height()/2; i++) {
	    if (page_buffer_last_line() >= page_buffer_lines_n()) {
		break;
	    }
	    cli_print(c_format("%s", page_buffer()[page_buffer_last_line()].c_str()));
	    incr_page_buffer_last_line();
	}
	goto redisplay_line_label;
    }
    
    //
    // Scroll down one whole screen
    //
    if ((val == ' ')
	|| (val == CHAR_TO_CTRL('f'))) {
	for (size_t i = 0; i < window_height() - 1; i++) {
	    if (page_buffer_last_line() >= page_buffer_lines_n()) {
		break;
	    }
	    const vector<string>& pb = page_buffer();
	    cli_print(c_format("%s", pb[page_buffer_last_line()].c_str()));
	    incr_page_buffer_last_line();
	}
	goto redisplay_line_label;
    }
    
    //
    // Scroll down to the bottom of the output
    //
    if ((val == 'G')
	|| (val == CHAR_TO_CTRL('e'))) {
	if (page_buffer_last_line() + window_height() - 1
	    < page_buffer_lines_n())
	    set_page_buffer_last_line(page_buffer_lines_n() - window_height() + 1);
	while (page_buffer_last_line() < page_buffer_lines_n()) {
	    cli_print(c_format("%s", page_buffer()[page_buffer_last_line()].c_str()));
	    incr_page_buffer_last_line();
	}
	goto redisplay_line_label;
    }
    
    //
    // Display the output all at once instead of oen screen at a time.
    // (Same as specifying the "| no-more" command.)
    //
    if ((val == 'N')) {
	while (page_buffer_last_line() < page_buffer_lines_n()) {
	    cli_print(c_format("%s", page_buffer()[page_buffer_last_line()].c_str()));
	    incr_page_buffer_last_line();
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
	// TODO: add the "up arrow" as well
	if (page_buffer_last_line() > 0)
	    decr_page_buffer_last_line();
	goto redisplay_screen_label;
    }
    
    //
    // Scroll up one-half screen
    //
    if ((val == 'u')
	|| (val == CHAR_TO_CTRL('u'))) {
	if (page_buffer_last_line() > window_height() - 1 + window_height()/2)
	    set_page_buffer_last_line(page_buffer_last_line() - window_height()/2);
	else
	    set_page_buffer_last_line(min(window_height() - 1,
					  page_buffer_lines_n()));
	goto redisplay_screen_label;
    }
    
    //
    // Scroll up one whole screen
    //
    if ((val == 'b')
	|| (val == CHAR_TO_CTRL('b'))) {
	if (page_buffer_last_line() > window_height() - 1 + window_height() - 1)
	    set_page_buffer_last_line(page_buffer_last_line() - window_height() + 1);
	else
	    set_page_buffer_last_line(min(window_height() - 1,
					  page_buffer_lines_n()));
	goto redisplay_screen_label;
    }
    
    //
    // Scroll up to the top of the output
    //
    if ((val == 'g')
	|| (val == CHAR_TO_CTRL('a'))) {
	size_t start_window_line = 0;
	set_page_buffer_last_line(min(start_window_line + window_height() - 1,
				      page_buffer_lines_n()));
	goto redisplay_screen_label;
    }
    
    //
    // Redraw the output of the screen
    //
    if ((val == CHAR_TO_CTRL('l'))) {
    redisplay_screen_label:
	size_t i, start_window_line = 0;
	if (page_buffer_last_line() >= window_height() - 1)
	    start_window_line = page_buffer_last_line() - window_height() + 1;
	else
	    start_window_line = 0;
	set_page_buffer_last_line(min(start_window_line + window_height() - 1,
				      page_buffer_lines_n()));
	for (i = start_window_line; i < page_buffer_last_line(); i++)
	    cli_print(c_format("%s", page_buffer()[i].c_str()));
	// XXX: fill-up the rest of the window
	for ( ; i < window_height() - 1; i++)
	    cli_print("\n");
	goto redisplay_line_label;
    }
    
    goto redisplay_line_label;
    
 exit_page_mode_label:
    reset_page_buffer();
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
	_page_buffer_last_line = &_output_buffer_last_line;
	goto redisplay_screen_label;
    }
    // FALLTHROUGH
    
 redisplay_line_label:
    cli_flush();
    if (is_page_mode()) {
	if (page_buffer_last_line() < page_buffer_lines_n())
	    restore_cli_prompt = " --More-- ";
	else
	    restore_cli_prompt = " --More-- (END) ";
    }
    set_current_cli_prompt(restore_cli_prompt.c_str());
    gl_redisplay_line(gl());
    cli_flush();
    return (XORP_OK);
}

void
CliClient::post_process_command()
{
    //
    // Test if we are waiting for the result from a server
    //
    if (is_waiting_for_data()) {
	// We are waiting for the result; silently return.
	return;
    }
    
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
	cli_print(c_format("%s", final_string.c_str()));
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
	if (page_buffer_last_line() < page_buffer_lines_n())
	    set_current_cli_prompt(" --More-- ");
	else
	    set_current_cli_prompt(" --More-- (END) ");
    } else {
	reset_page_buffer();
    }
    
    //
    // Reset buffer, cursor, prompt
    //
    command_buffer().reset();
    set_buff_curpos(0);
    cli_print(c_format("%s", current_cli_prompt()));
    cli_flush();
}

//
// Process one input character
// Return: %XORP_OK on success, otherwise %XORP_ERROR.
// XXX: returning %XORP_ERROR is also an indication that the connection
// has been closed.
int
CliClient::process_char(const char *line, uint8_t val)
{
    int gl_buff_curpos = gl_get_buff_curpos(gl());
    int ret_value = XORP_OK;
    
    if (line == NULL)
	return (XORP_ERROR);
    
    if ((val == '\n') || (val == '\r')) {
	// New command
	// cli_print(c_format("Your command is: %s\n", line));
	set_page_buffer_mode(true);
	process_command(line);
	post_process_command();
	return (XORP_OK);
    }
    
    if (val == '?') {
	// Command-line help
	//set_page_buffer_mode(true);
	// TODO: add "page" support
	command_line_help(line, gl_buff_curpos);
	//set_page_buffer_mode(false);
	//if (is_page_mode()) {
	//if (page_buffer_last_line() < page_buffer_lines_n())
	//set_current_cli_prompt(" --More-- ");
	//else
	//set_current_cli_prompt(" --More-- (END) ");
	//} else {
	//reset_page_buffer();
	//}
	//cli_print(c_format("%s", current_cli_prompt()));
	//command_buffer().reset();
	//set_buff_curpos(0);
	return (XORP_OK);
    }
    
    //
    // XXX: the (val == ' ') case is handled by the parent function
    //

    if (val == CHAR_TO_CTRL('c')) {
	//
	// Interrupt current command if it still is pending
	//
	if (is_waiting_for_data()) {
	    // Reset everything about the command
	    set_is_waiting_for_data(false);
	    delete_pipe_all();
	    set_pipe_mode(false);
	    set_hold_mode(false);
	    set_page_mode(false);
	    reset_page_buffer();
	    set_page_buffer_mode(false);
	    
	    cli_print(c_format("\n"));	// XXX: new line
	    cli_print(c_format("Command interrupted!\n"));
	}
	
	//
	// Ignore current line, reset buffer, line, cursor, prompt
	//
	cli_print(c_format("\n"));	// XXX: new line
	gl_redisplay_line(gl());
	gl_reset_line(gl());
	set_buff_curpos(0);
	command_buffer().reset();
	cli_flush();
	
	return (XORP_OK);
    }
    
    // All other characters which we need to print
    // Store the line in the command buffer
    command_buffer().reset();
    ret_value = XORP_OK;
    for (int i = 0; line[i] != '\0'; i++) {
	ret_value = command_buffer().add_data(line[i]);
	if (ret_value < 0)
	    break;
    }
    if (ret_value == XORP_OK)
	ret_value = command_buffer().add_data('\0');
    if (ret_value != XORP_OK) {
	// This client is sending too much data. Kick it out!
	// TODO: print more informative message about the
	// client: E.g. where it came from, etc.
	XLOG_WARNING("Removing client (socket = %d family = %d): "
		     "data buffer full",
		     cli_fd(),
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
 * 
 * Print the help for the same-line command.
 **/
void
CliClient::command_line_help(const char *line, int word_end)
{
    CliCommand *curr_cli_command = _current_cli_command;
    string command_help_string = "";
    bool found_bool = false;
    
    word_end--;			// XXX: exclude the '?' character
    cli_print("\nPossible completions:\n");
    
    list<CliCommand *>::iterator iter;
    for (iter = curr_cli_command->child_command_list().begin();
	 iter != curr_cli_command->child_command_list().end();
	 ++iter) {
	CliCommand *tmp_cli_command = *iter;
	if (tmp_cli_command->find_command_help(line, word_end,
					       command_help_string))
	    found_bool = true;
    }
    if (found_bool)
	cli_print(c_format("%s", command_help_string.c_str()));
    
    gl_redisplay_line(gl());
    // XXX: Move the cursor over the '?'
    gl_place_cursor(gl(), gl_get_buff_curpos(gl()) - 1);
    cli_print(" \b");	// XXX: A hack to delete the '?'
}

CliCommand *
CliClient::multi_command_find(const char *line, int word_end)
{
    string command_line = string(line, word_end);
    
    return (_current_cli_command->multi_command_find(command_line));
}


int
CliClient::process_command(const char *line)
{
    string token, token_line;
    CliCommand *parent_cli_command = current_cli_command();
    CliCommand *child_cli_command = NULL;
    int syntax_error_offset_next = strlen(current_cli_prompt());
    int syntax_error_offset_prev = syntax_error_offset_next;
    int i, old_len, new_len;
    
    token_line = line;
    new_len = token_line.size();
    old_len = new_len;
    
    for (token = pop_token(token_line);
	 ! token.empty();
	 token = pop_token(token_line)) {
	child_cli_command = parent_cli_command->command_find(token);
	
	new_len = token_line.size();
	syntax_error_offset_prev = syntax_error_offset_next;
	syntax_error_offset_next += old_len - new_len;
	old_len = new_len;
	
	if (child_cli_command != NULL) {
	    parent_cli_command = child_cli_command;
	    continue;
	}
	
	if (parent_cli_command->has_cli_process_callback()) {
	    // The parent command has processing function, so the rest
	    // of the tokens could be arguments for that function
	    child_cli_command = parent_cli_command;
	}
	
	// Put-back the token and process the arguments after the loop
	token_line = copy_token(token) + token_line;
	break;
    }
    
    if (parent_cli_command->has_cli_process_callback()) {
	// Process the rest of the tokens as arguments for this function
	vector<string> args_vector;
	bool process_func_arguments_bool = true;
	bool pipe_command_arguments_bool = false;
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
		process_func_arguments_bool = false;
		pipe_command_arguments_bool = false;
		pipe_command_empty = true;
		if (pipe_command_name.size()) {
		    // Add the previous pipe command
		    add_pipe(pipe_command_name.c_str(),
			     pipe_command_args_list);
		    pipe_command_name = "";
		    pipe_command_args_list.clear();
		}
		continue;
	    }
	    if (process_func_arguments_bool) {
		// Arguments for the processing command
		args_vector.push_back(token);
		continue;
	    }
	    if (! pipe_command_arguments_bool) {
		// The pipe command name
		pipe_command_arguments_bool = true;
		pipe_command_empty = false;
		pipe_command_name = token;
		continue;
	    }
	    // The pipe command arguments
	    pipe_command_args_list.push_back(token);
	}
	if (pipe_command_name.size()) {
	    // Add the last pipe command
	    add_pipe(pipe_command_name.c_str(), pipe_command_args_list);
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
	    
	    list<CliPipe*>::iterator iter;
	    for (iter = _pipe_list.begin(); iter != _pipe_list.end(); ++iter) {
		CliPipe *cli_pipe = *iter;
		cli_pipe->start_func(final_string);
	    }
	    if (final_string.size()) {
		bool old_pipe_mode = is_pipe_mode();
		set_pipe_mode(false);
		cli_print(c_format("%s", final_string.c_str()));
		set_pipe_mode(old_pipe_mode);
	    }
	    final_string = "";
	    
	    ret_value = parent_cli_command->_cli_process_callback->dispatch(parent_cli_command->server_name(), cli_session_term_name(), cli_session_session_id(), parent_cli_command->global_name(), args_vector);
	    
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
    // If there are more tokens, the first one has to be a sub-command.
    // However, there wasn't a match in our search, hence it has
    // to be an error.
    // Further, there was no processing function, hence those cannot
    // be command arguments.
    syntax_error_offset_next -= token.size();
    if (parent_cli_command == current_cli_command()) {
	cli_print(c_format("%*s%s\n", syntax_error_offset_next, " ", "^"));
	cli_print("unknown command.\n");
    } else {
	if (token.empty())
	    syntax_error_offset_next++;
	cli_print(c_format("%*s%s\n", syntax_error_offset_next, " ", "^"));
	cli_print("syntax error, expecting");
	if (parent_cli_command->child_command_list().size() > 4) {
	    // TODO: replace the hard-coded "4" with a #define or a parameter.
	    cli_print(" <command>.\n");
	} else {
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
		cli_print(c_format(" `%s'", child_cli_command->name()));
	    }
	    cli_print(".\n");
	}
    }
    
    return (XORP_ERROR);
}

int
CliClient::command_completion_func(WordCompletion *cpl, void *data,
				   const char *line, int word_end)
{
    int ret_value = 1;
    CliClient *cli_client = reinterpret_cast<CliClient*>(data);
    CliCommand *curr_cli_command = cli_client->_current_cli_command;
    list<CliCommand *> cli_command_match_list;
    
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
    
    if (cli_command_match_list.size() > 1) {
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
    }
    
    if (ret_value != 0) {
	cpl_record_error(cpl, "Not a XORP command!");
    }
    
    return (ret_value);
}
