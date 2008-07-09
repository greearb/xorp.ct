// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/rtrmgr/cli.hh,v 1.52 2007/10/13 01:30:21 pavlin Exp $

#ifndef __RTRMGR_CLI_HH__
#define __RTRMGR_CLI_HH__


#include <list>
#include <map>

#include "libxipc/xrl_error.hh"

#include "cli/cli_node.hh"
#include "cli/cli_client.hh"

#include "config_operators.hh"
#include "rtrmgr_error.hh"


class CommandTree;
class CommandTreeNode;
class ConfigTree;
class ConfigTreeNode;
class OpCommandList;
class OpInstance;
class SlaveConfigTree;
class SlaveConfigTreeNode;
class TemplateTree;
class TemplateTreeNode;

class XorpShellBase;

class XrlAtomList;

enum CliModeType {
    CLI_MODE_NONE		= 0,
    CLI_MODE_OPERATIONAL	= 1,
    CLI_MODE_CONFIGURE		= 2,
    CLI_MODE_TEXTENTRY		= 3
};

class RouterCLI {
public:
    RouterCLI(XorpShellBase& xorpsh, CliNode& cli_node,
	      XorpFd cli_client_input_fd, XorpFd cli_client_output_fd,
	      bool verbose) throw (InitError);
    ~RouterCLI();

    bool done() const;
    bool is_config_mode() const;
    void clear_command_set();
    int configure_func(const string& ,
		       const string& ,
		       uint32_t ,
		       const vector<string>& command_global_name,
		       const vector<string>& argv);
    void enter_config_done(const XrlError& e);
    void got_config_users(const XrlError& e, const XrlAtomList* users);
    void new_config_user(uid_t user_id);
    void leave_config_done(const XrlError& e);
    void notify_user(const string& alert, bool urgent);
    void config_changed_by_other_user();
    int op_help_func(const string& ,
		   const string& ,
		   uint32_t ,
		   const vector<string>& command_global_name,
		   const vector<string>& argv);
    int conf_help_func(const string& ,
		   const string& ,
		   uint32_t ,
		   const vector<string>& command_global_name,
		   const vector<string>& argv);
    int logout_func(const string& ,
		   const string& ,
		   uint32_t ,
		   const vector<string>& command_global_name,
		   const vector<string>& argv);
    int exit_func(const string& ,
		  const string& ,
		  uint32_t ,
		  const vector<string>& command_global_name,
		  const vector<string>& argv);
    int edit_func(const string& ,
		  const string& ,
		  uint32_t ,
		  const vector<string>& command_global_name,
		  const vector<string>& argv);
    int extract_leaf_node_operator_and_value(const TemplateTreeNode& ttn,
					     const vector<string>& argv,
					     ConfigOperator& node_operator,
					     string& value,
					     string& error_msg);
    map<string, CliCommandMatch> text_entry_children_func(
	const vector<string>& vector_path) const;
    int text_entry_func(const string& ,
			const string& ,
			uint32_t ,
			const vector<string>& command_global_name,
			const vector<string>& argv);
    int delete_func(const string& ,
		    const string& ,
		    uint32_t ,
		    const vector<string>& command_global_name,
		    const vector<string>& argv);
    int commit_func(const string& ,
		    const string& ,
		    uint32_t ,
		    const vector<string>& command_global_name,
		    const vector<string>& argv);
    void commit_done(bool success, string error_msg);

    int show_func(const string& ,
		  const string& ,
		  uint32_t ,
		  const vector<string>& command_global_name,
		  const vector<string>& argv);
    int op_mode_func(const string& ,
		     const string& ,
		     uint32_t ,
		     const vector<string>& command_global_name,
		     const vector<string>& argv);
    int save_func(const string& ,
		  const string& ,
		  uint32_t ,
		  const vector<string>& command_global_name,
		  const vector<string>& argv);
    void save_communicated(const XrlError& e);
    void save_done(bool success, string error_msg);

    int load_func(const string& ,
		  const string& ,
		  uint32_t ,
		  const vector<string>& command_global_name,
		  const vector<string>& argv);
    void load_communicated(const XrlError& e);
    void load_done(bool success, string error_msg);

    map<string, CliCommandMatch> op_mode_help(const vector<string>& command_global_name) const;
    map<string, CliCommandMatch> configure_mode_help(const vector<string>& command_global_name) const;

    typedef XorpCallback1<void, const string&>::RefPtr OpModePrintCallback;
    typedef XorpCallback2<void, bool,const string&>::RefPtr OpModeDoneCallback;

    /**
     * Callback: partial output generated by the operational command
     *
     * @param result partial output
     */
    void op_mode_cmd_print(const string& result);

    /**
     * Callback: called when the operational command completes
     *
     * @param success true if the command suceeded
     * @param error_msg if the command failed the error message
     */
    void op_mode_cmd_done(bool success, const string& error_msg);

    /**
     * Callback: called when a user send an interrupt terminate the
     * operational mode command if there is one running.
     *
     * @param server_name the name of the server that returned the result.
     * @param cli_term_name the name of the terminal that originated
     * the command.
     * @param cli_session_id the session ID of the terminal that originated
     * the command.
     * @param command_global_name the name of the command that is interrupted.
     * @param command_args the arguments to the command that is interrupted.
     */
    void op_mode_cmd_interrupt(const string& server_name,
			       const string& cli_term_name,
			       uint32_t cli_session_id,
			       const vector<string>& command_global_name,
			       const vector<string>& command_args);

    /**
     * Tidy up operational mode command.
     *
     * Can safely be called multiple times.
     */
    void op_mode_cmd_tidy();

private:
    static const string DEFAULT_XORP_PROMPT_OPERATIONAL;
    static const string DEFAULT_XORP_PROMPT_CONFIGURATION;

    CliClient& cli_client() const { return (*_cli_client_ptr); }
    void reset_path();
    void set_path(string path);
    void apply_path_change();
    int add_static_configure_mode_commands(string& error_msg);
    void set_prompt(const string& line1, 
		    const string& line2);
    void config_mode_prompt();
    void idle_ui();
    void reenable_ui();
    void silent_reenable_ui();

    string pathstr() const;
    string pathstr2() const;
    void operational_mode();
    int add_op_mode_commands(CliCommand *root, string& error_msg);
    void configure_mode();
    void text_entry_mode();
    void add_command_subtree(CliCommand& current_cli_node,
			     const CommandTreeNode& current_ctn,
			     const CLI_PROCESS_CALLBACK& cli_process_callback,
			     vector<string> vector_path, size_t depth,
			     bool can_pipe, bool include_allowed_values);
    void add_text_entry_commands(CliCommand* com0);
    void add_edit_subtree();
    void add_create_subtree();
    void add_delete_subtree();
    void add_set_subtree();
    void add_show_subtree();

    void display_config_mode_users() const;
    void display_alerts();

    bool check_for_rtrmgr_restart();
    void verify_rtrmgr_restart(const XrlError& e, const uint32_t* pid);

    string get_help_o(const string& s) const;
    string get_help_c(const string& s) const;

    string makepath(const list<string>& parts) const;
    string makepath(const vector<string>& parts) const;

    const TemplateTree*	template_tree() const;
    SlaveConfigTree*	config_tree();
    const SlaveConfigTree* config_tree() const;
    OpCommandList*	op_cmd_list() const;
    uint32_t clientid() const;

    void add_allowed_values_and_ranges(
	const TemplateTreeNode* ttn,
	bool is_executable, bool can_pipe,
	map<string, CliCommandMatch>& children) const;

    XorpShellBase&     _xorpsh;

    //    SlaveConfigTreeNode* _current_config_node;

    CliNode&		_cli_node;
    CliClient*		_cli_client_ptr;
    CliClient*		_removed_cli_client_ptr;
    bool		_verbose;	// Set to true if output is verbose
    string		_operational_mode_prompt;
    string		_configuration_mode_prompt;
    CliModeType		_mode;
    CliCommand*		_set_node;
    CliCommand*		_show_node;
    CliCommand*		_edit_node;
    CliCommand*		_create_node;
    CliCommand*		_delete_node;
    CliCommand*		_run_node;
    list<string>	_path;
    list<uint32_t>      _braces; // keep trace of the indent depth of
				 // braces in text_entry mode
    list<uint32_t>	_config_mode_users;
    list<string>	_alerts;
    // size_t		_nesting_depth;	// for text_entry mode: number of
					// brackets deep
    // list<size_t>	_nesting_lengths; // for text_entry mode: number of
					// nodes for each backet nested
    bool		_changes_made;	// true if there are uncommitted
					// changes

    map<string,string> _help_o;  // short help strings for operational
				 // mode commands
    map<string,string> _help_long_o; // detailed help information for
                                     // operational mode commands
    map<string,string> _help_c;  // short help strings for configuration
				 // mode commands
    map<string,string> _help_long_c; // detailed help information for
                                     // configuration mode commands

    OpInstance *_op_mode_cmd;// Currently executing operational mode commands
};

#endif // __RTRMGR_CLI_HH__
