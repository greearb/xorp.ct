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

#ident "$XORP: xorp/cli/cli_command_pipe.cc,v 1.1.1.1 2002/12/11 23:55:51 hodson Exp $"


//
// CLI command "pipe" ("|") implementation.
//


#include "cli_module.h"
#include "cli_private.hh"
#include "libxorp/token.hh"
#include "libxorp/utils.hh"
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
static int cli_pipe_dummy_func(const char *server_name,
			       const char *cli_term_name,
			       uint32_t cli_session_id,
			       const char *command_global_name,
			       const vector<string>& argv);


CliPipe::CliPipe(const char *init_pipe_name)
    : CliCommand(NULL, init_pipe_name, CliPipe::name2help(init_pipe_name))
{
    _pipe_type = name2pipe_type(init_pipe_name);
    _counter = 0;
    _flag_bool = false;
    _cli_client = NULL;
    
    CLI_PROCESS_CALLBACK _cb = callback(cli_pipe_dummy_func);
    set_cli_process_callback(_cb);
    set_can_pipe(true);
    
    switch (pipe_type()) {
    case CLI_PIPE_COMPARE:
	_start_func_ptr = &CliPipe::pipe_compare_start;
	_process_func_ptr = &CliPipe::pipe_compare_process;
	_eof_func_ptr = &CliPipe::pipe_compare_eof;
	break;
    case CLI_PIPE_COMPARE_ROLLBACK:
	_start_func_ptr = &CliPipe::pipe_compare_rollback_start;
	_process_func_ptr = &CliPipe::pipe_compare_rollback_process;
	_eof_func_ptr = &CliPipe::pipe_compare_rollback_eof;
	break;
    case CLI_PIPE_COUNT:
	_start_func_ptr = &CliPipe::pipe_count_start;
	_process_func_ptr = &CliPipe::pipe_count_process;
	_eof_func_ptr = &CliPipe::pipe_count_eof;
	break;
    case CLI_PIPE_DISPLAY:
	_start_func_ptr = &CliPipe::pipe_display_start;
	_process_func_ptr = &CliPipe::pipe_display_process;
	_eof_func_ptr = &CliPipe::pipe_display_eof;
	break;
    case CLI_PIPE_DISPLAY_DETAIL:
	_start_func_ptr = &CliPipe::pipe_display_detail_start;
	_process_func_ptr = &CliPipe::pipe_display_detail_process;
	_eof_func_ptr = &CliPipe::pipe_display_detail_eof;
	break;
    case CLI_PIPE_DISPLAY_INHERITANCE:
	_start_func_ptr = &CliPipe::pipe_display_inheritance_start;
	_process_func_ptr = &CliPipe::pipe_display_inheritance_process;
	_eof_func_ptr = &CliPipe::pipe_display_inheritance_eof;
	break;
    case CLI_PIPE_DISPLAY_XML:
	_start_func_ptr = &CliPipe::pipe_display_xml_start;
	_process_func_ptr = &CliPipe::pipe_display_xml_process;
	_eof_func_ptr = &CliPipe::pipe_display_xml_eof;
	break;
    case CLI_PIPE_EXCEPT:
	_start_func_ptr = &CliPipe::pipe_except_start;
	_process_func_ptr = &CliPipe::pipe_except_process;
	_eof_func_ptr = &CliPipe::pipe_except_eof;
	break;
    case CLI_PIPE_FIND:
	_start_func_ptr = &CliPipe::pipe_find_start;
	_process_func_ptr = &CliPipe::pipe_find_process;
	_eof_func_ptr = &CliPipe::pipe_find_eof;
	break;
    case CLI_PIPE_HOLD:
	_start_func_ptr = &CliPipe::pipe_hold_start;
	_process_func_ptr = &CliPipe::pipe_hold_process;
	_eof_func_ptr = &CliPipe::pipe_hold_eof;
	break;
    case CLI_PIPE_MATCH:
	_start_func_ptr = &CliPipe::pipe_match_start;
	_process_func_ptr = &CliPipe::pipe_match_process;
	_eof_func_ptr = &CliPipe::pipe_match_eof;
	break;
    case CLI_PIPE_NOMORE:
	_start_func_ptr = &CliPipe::pipe_nomore_start;
	_process_func_ptr = &CliPipe::pipe_nomore_process;
	_eof_func_ptr = &CliPipe::pipe_nomore_eof;
	break;
    case CLI_PIPE_RESOLVE:
	_start_func_ptr = &CliPipe::pipe_resolve_start;
	_process_func_ptr = &CliPipe::pipe_resolve_process;
	_eof_func_ptr = &CliPipe::pipe_resolve_eof;
	break;
    case CLI_PIPE_SAVE:
	_start_func_ptr = &CliPipe::pipe_save_start;
	_process_func_ptr = &CliPipe::pipe_save_process;
	_eof_func_ptr = &CliPipe::pipe_save_eof;
	break;
    case CLI_PIPE_TRIM:
	_start_func_ptr = &CliPipe::pipe_trim_start;
	_process_func_ptr = &CliPipe::pipe_trim_process;
	_eof_func_ptr = &CliPipe::pipe_trim_eof;
	break;
    case CLI_PIPE_MAX:
    default:
	_start_func_ptr = &CliPipe::pipe_unknown_start;
	_process_func_ptr = &CliPipe::pipe_unknown_process;
	_eof_func_ptr = &CliPipe::pipe_unknown_eof;
	break;
    }
}

CliPipe::~CliPipe()
{
    
}

const char *
CliPipe::name2help(const char *pipe_name)
{
    switch (name2pipe_type(pipe_name)) {
    case CLI_PIPE_COMPARE:
	return ("Compare configuration changes with a prior version");
    case CLI_PIPE_COMPARE_ROLLBACK:
	return ("Compare configuration changes with a prior rollback version");
    case CLI_PIPE_COUNT:
	return ("Count occurences");    
    case CLI_PIPE_DISPLAY:
	return ("Display additional configuration information");
    case CLI_PIPE_DISPLAY_DETAIL:
	return ("Display configuration data detail");
    case CLI_PIPE_DISPLAY_INHERITANCE:
	return ("Display inherited configuration data and source group");
    case CLI_PIPE_DISPLAY_XML:
	return ("Display XML content of the command");
    case CLI_PIPE_EXCEPT:
	return ("Show only text that does not match a pattern");
    case CLI_PIPE_FIND:
	return ("Search for the first occurence of a pattern");
    case CLI_PIPE_HOLD:
	return ("Hold text without exiting the --More-- prompt");
    case CLI_PIPE_MATCH:
	return ("Show only text that matches a pattern");
    case CLI_PIPE_NOMORE:
	return ("Don't paginate output");
    case CLI_PIPE_RESOLVE:
	return ("Resolve IP addresses (NOT IMPLEMENTED YET)");
    case CLI_PIPE_SAVE:
	return ("Save output text to a file (NOT IMPLEMENTED YET)");
    case CLI_PIPE_TRIM:
	return ("Trip specified number of columns from the start line (NOT IMPLEMENTED YET)");
    case CLI_PIPE_MAX:
    default:
	return ("Pipe type unknown");
    }
}

CliPipe::cli_pipe_t
CliPipe::name2pipe_type(const char *pipe_name)
{
    string token_line = pipe_name;
    string token;
    
    token = pop_token(token_line);
    
    if (token.empty())
	return (CLI_PIPE_MAX);
    
    if (token == "compare")
	return (CLI_PIPE_COMPARE);
    if (token == "count")
	return (CLI_PIPE_COUNT);
    if (token == "display") {
	token = pop_token(token_line);
	if (token.empty())
	    return (CLI_PIPE_DISPLAY);
	if (token == "detail")
	    return (CLI_PIPE_DISPLAY_DETAIL);
	if (token == "inheritance")
	    return (CLI_PIPE_DISPLAY_INHERITANCE);
	if (token == "xml")
	    return (CLI_PIPE_DISPLAY_XML);
	return (CLI_PIPE_MAX);
    }
    if (token == "except")
	return (CLI_PIPE_EXCEPT);
    if (token == "find")
	return (CLI_PIPE_FIND);
    if (token == "hold")
	return (CLI_PIPE_HOLD);
    if (token == "match")
	return (CLI_PIPE_MATCH);
    if (token == "no-more")
	return (CLI_PIPE_NOMORE);
    if (token == "resolve")
	return (CLI_PIPE_RESOLVE);
    if (token == "save")
	return (CLI_PIPE_SAVE);
    if (token == "trim")
	return (CLI_PIPE_TRIM);
    
    return (CLI_PIPE_MAX);
}

int
CliPipe::pipe_compare_start(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_compare_process(string& input_line)
{
    if (input_line.size())
	return (XORP_OK);
    else
	return (XORP_ERROR);
}

int
CliPipe::pipe_compare_eof(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_compare_rollback_start(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_compare_rollback_process(string& input_line)
{
    if (input_line.size())
	return (XORP_OK);
    else
	return (XORP_ERROR);
}

int
CliPipe::pipe_compare_rollback_eof(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_count_start(string& input_line)
{
    _counter = 0;
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_count_process(string& input_line)
{
    if (input_line.size()) {
	input_line = "";
	_counter++;
	return (XORP_OK);
    } else {
	return (XORP_ERROR);
    }
}

int
CliPipe::pipe_count_eof(string& input_line)
{
    pipe_count_process(input_line);
    input_line += c_format("Count: %d lines\n", _counter);
    
    return (XORP_OK);
}

int
CliPipe::pipe_display_start(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_display_process(string& input_line)
{
    if (input_line.size())
	return (XORP_OK);
    else
	return (XORP_ERROR);
}

int
CliPipe::pipe_display_eof(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_display_detail_start(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_display_detail_process(string& input_line)
{
    if (input_line.size())
	return (XORP_OK);
    else
	return (XORP_ERROR);
}

int
CliPipe::pipe_display_detail_eof(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_display_inheritance_start(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_display_inheritance_process(string& input_line)
{
    if (input_line.size())
	return (XORP_OK);
    else
	return (XORP_ERROR);
}

int
CliPipe::pipe_display_inheritance_eof(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_display_xml_start(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_display_xml_process(string& input_line)
{
    if (input_line.size())
	return (XORP_OK);
    else
	return (XORP_ERROR);
}

int
CliPipe::pipe_display_xml_eof(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_except_start(string& input_line)
{
    string arg1;
    
    if (_pipe_args_list.empty())
	return (XORP_ERROR);
    arg1 = _pipe_args_list[0];
    // TODO: check the flags
    if (regcomp(&_preg, arg1.c_str(),
		REG_EXTENDED | REG_ICASE | REG_NOSUB | REG_NEWLINE) != 0) {
	return (XORP_ERROR);
    }
    
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_except_process(string& input_line)
{
    int ret_value;
    
    if (! input_line.size())
	return (XORP_ERROR);
    
    ret_value = regexec(&_preg, input_line.c_str(), 0, NULL, 0);
    if (ret_value == 0) {
	// Match
	input_line = "";
    } else {
	// No-match
    }
    
    return (XORP_OK);
}

int
CliPipe::pipe_except_eof(string& input_line)
{
    regfree(&_preg);
    
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_find_start(string& input_line)
{
    string arg1;
    
    if (_pipe_args_list.empty())
	return (XORP_ERROR);
    arg1 = _pipe_args_list[0];
    // TODO: check the flags
    if (regcomp(&_preg, arg1.c_str(),
		REG_EXTENDED | REG_ICASE | REG_NOSUB | REG_NEWLINE) != 0) {
	return (XORP_ERROR);
    }
    
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_find_process(string& input_line)
{
    int ret_value;
    
    if (! input_line.size())
	return (XORP_ERROR);
    
    if (! _flag_bool) {
	// Evaluate the line
	ret_value = regexec(&_preg, input_line.c_str(), 0, NULL, 0);
	if (ret_value == 0) {
	    // Match
	    _flag_bool = true;
	} else {
	    // No-match
	}
    }
    if (! _flag_bool)
	input_line = "";	// Don't print yet
    
    return (XORP_OK);
}

int
CliPipe::pipe_find_eof(string& input_line)
{
    regfree(&_preg);
    
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_hold_start(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_hold_process(string& input_line)
{
    if (input_line.size())
	return (XORP_OK);
    else
	return (XORP_ERROR);
}

int
CliPipe::pipe_hold_eof(string& input_line)
{
    if (_cli_client != NULL) {
	_cli_client->set_hold_mode(true);
    }
    
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_match_start(string& input_line)
{
    string arg1;
    
    if (_pipe_args_list.empty())
	return (XORP_ERROR);
    arg1 = _pipe_args_list[0];
    // TODO: check the flags
    if (regcomp(&_preg, arg1.c_str(),
		REG_EXTENDED | REG_ICASE | REG_NOSUB | REG_NEWLINE) != 0) {
	return (XORP_ERROR);
    }
    
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_match_process(string& input_line)
{
    int ret_value;
    
    if (! input_line.size())
	return (XORP_ERROR);
    
    ret_value = regexec(&_preg, input_line.c_str(), 0, NULL, 0);
    if (ret_value == 0) {
	// Match
    } else {
	// No-match
	input_line = "";
    }
    
    return (XORP_OK);
}

int
CliPipe::pipe_match_eof(string& input_line)
{
    regfree(&_preg);
    
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_nomore_start(string& input_line)
{
    if (_cli_client != NULL) {
	_cli_client->set_nomore_mode(true);
    }
    
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_nomore_process(string& input_line)
{
    if (input_line.size())
	return (XORP_OK);
    else
	return (XORP_ERROR);
}

int
CliPipe::pipe_nomore_eof(string& input_line)
{
    if (_cli_client != NULL) {
	_cli_client->set_nomore_mode(false);
    }
    
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_resolve_start(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_resolve_process(string& input_line)
{
    // TODO: implement it
    if (input_line.size())
	return (XORP_OK);
    else
	return (XORP_ERROR);
}

int
CliPipe::pipe_resolve_eof(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_save_start(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_save_process(string& input_line)
{
    // TODO: implement it
    if (input_line.size())
	return (XORP_OK);
    else
	return (XORP_ERROR);
}

int
CliPipe::pipe_save_eof(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_trim_start(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_trim_process(string& input_line)
{
    // TODO: implement it
    if (input_line.size())
	return (XORP_OK);
    else
	return (XORP_ERROR);
}

int
CliPipe::pipe_trim_eof(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_unknown_start(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

int
CliPipe::pipe_unknown_process(string& input_line)
{
    if (input_line.size())
	return (XORP_OK);
    else
	return (XORP_ERROR);
}

int
CliPipe::pipe_unknown_eof(string& input_line)
{
    UNUSED(input_line);
    return (XORP_OK);
}

//
// A dummy function
//
static int
cli_pipe_dummy_func(const char *		, // server_name,
		    const char *		, // cli_term_name
		    uint32_t			, // cli_session_id
		    const char *		, // command_global_name
		    const vector<string>&	  // argv
    )
{
    return (XORP_OK);
}
