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

// $XORP: xorp/cli/cli_command_pipe.hh,v 1.3 2003/03/10 23:20:11 hodson Exp $


#ifndef __CLI_CLI_COMMAND_PIPE_HH__
#define __CLI_CLI_COMMAND_PIPE_HH__


//
// CLI command "pipe" ("|") definition.
//


#include <sys/types.h>
#include <regex.h>
#include <string>
#include <list>

#include "cli_command.hh"


//
// Constants definitions
//

//
// Structures/classes, typedefs and macros
//

/**
 * @short The class for the "pipe" ("|") command.
 */
class CliPipe : public CliCommand {
public:
    /**
     * Constructor for a given pipe name.
     * 
     * Currently, the list of recognized pipe names are:
     *  count
     *  except
     *  find
     *  hold
     *  match
     *  no-more
     *  resolve
     *  save
     *  trim
     * 
     * @param init_pipe_name the pipe name (see above about the list of
     * recogined pipe names).
     */
    CliPipe(const string& init_pipe_name);
    
    /**
     * Destructor
     */
    virtual ~CliPipe();
    
private:
    friend class CliClient;
    
    bool is_invalid() { return (_pipe_type == CLI_PIPE_MAX); }
    void add_pipe_arg(const string& v) { _pipe_args_list.push_back(v); }
    void set_cli_client(CliClient *v) { _cli_client = v; }
    
    int start_func(string& input_line) { return (this->*_start_func_ptr)(input_line); }
    int process_func(string& input_line) { return (this->*_process_func_ptr)(input_line); }
    int eof_func(string& input_line) { return (this->*_eof_func_ptr)(input_line); }
    
    // The "pipe" types
    enum cli_pipe_t {
	CLI_PIPE_COMPARE		= 0,
	CLI_PIPE_COMPARE_ROLLBACK	= 1,
	CLI_PIPE_COUNT			= 2,
	CLI_PIPE_DISPLAY		= 3,
	CLI_PIPE_DISPLAY_DETAIL		= 4,
	CLI_PIPE_DISPLAY_INHERITANCE	= 5,
	CLI_PIPE_DISPLAY_XML		= 6,
	CLI_PIPE_EXCEPT			= 7,
	CLI_PIPE_FIND			= 8,
	CLI_PIPE_HOLD			= 9,
	CLI_PIPE_MATCH			= 10,
	CLI_PIPE_NOMORE			= 11,
	CLI_PIPE_RESOLVE		= 12,
	CLI_PIPE_SAVE			= 13,
	CLI_PIPE_TRIM			= 14,
	CLI_PIPE_MAX
    };
    string name2help(const string& pipe_name);
    cli_pipe_t name2pipe_type(const string& pipe_name);
    cli_pipe_t pipe_type() { return (_pipe_type); }
    
    // The line processing functions
    typedef int (CliPipe::*LineProcess)(string& input_line);
    LineProcess _start_func_ptr;
    LineProcess _process_func_ptr;
    LineProcess _eof_func_ptr;
    
    int pipe_compare_start(string& input_line);
    int pipe_compare_process(string& input_line);
    int pipe_compare_eof(string& input_line);
    int pipe_compare_rollback_start(string& input_line);
    int pipe_compare_rollback_process(string& input_line);
    int pipe_compare_rollback_eof(string& input_line);
    int pipe_count_start(string& input_line);
    int pipe_count_process(string& input_line);
    int pipe_count_eof(string& input_line);
    int pipe_display_start(string& input_line);
    int pipe_display_process(string& input_line);
    int pipe_display_eof(string& input_line);
    int pipe_display_detail_start(string& input_line);
    int pipe_display_detail_process(string& input_line);
    int pipe_display_detail_eof(string& input_line);
    int pipe_display_inheritance_start(string& input_line);
    int pipe_display_inheritance_process(string& input_line);
    int pipe_display_inheritance_eof(string& input_line);
    int pipe_display_xml_start(string& input_line);
    int pipe_display_xml_process(string& input_line);
    int pipe_display_xml_eof(string& input_line);
    int pipe_except_start(string& input_line);
    int pipe_except_process(string& input_line);
    int pipe_except_eof(string& input_line);
    int pipe_find_start(string& input_line);
    int pipe_find_process(string& input_line);
    int pipe_find_eof(string& input_line);
    int pipe_hold_start(string& input_line);
    int pipe_hold_process(string& input_line);
    int pipe_hold_eof(string& input_line);
    int pipe_match_start(string& input_line);
    int pipe_match_process(string& input_line);
    int pipe_match_eof(string& input_line);
    int pipe_nomore_start(string& input_line);
    int pipe_nomore_process(string& input_line);
    int pipe_nomore_eof(string& input_line);
    int pipe_resolve_start(string& input_line);
    int pipe_resolve_process(string& input_line);
    int pipe_resolve_eof(string& input_line);
    int pipe_save_start(string& input_line);
    int pipe_save_process(string& input_line);
    int pipe_save_eof(string& input_line);
    int pipe_trim_start(string& input_line);
    int pipe_trim_process(string& input_line);
    int pipe_trim_eof(string& input_line);
    int pipe_unknown_start(string& input_line);
    int pipe_unknown_process(string& input_line);
    int pipe_unknown_eof(string& input_line);
    
    cli_pipe_t		_pipe_type;
    vector<string>	_pipe_args_list; // The arguments for the pipe command
    int			_counter;	// Internal counter to keep state
    regex_t		_preg;		// Regular expression (internal form)
    bool		_flag_bool;	// Internal bool flag to keep state
    
    CliClient		*_cli_client;	// The CliClient I belong to, or NULL
};


//
// Global variables
//


//
// Global functions prototypes
//

#endif // __CLI_CLI_COMMAND_PIPE_HH__
