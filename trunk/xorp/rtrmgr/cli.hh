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

// $XORP: xorp/rtrmgr/cli.hh,v 1.16 2004/05/28 22:27:55 pavlin Exp $

#ifndef __RTRMGR_CLI_HH__
#define __RTRMGR_CLI_HH__


#include <list>
#include <map>

#include "libxipc/xrl_error.hh"

#include "cli/cli_node.hh"
#include "cli/cli_client.hh"


class CommandTree;
class CommandTreeNode;
class ConfigTree;
class ConfigTreeNode;
class OpCommandList;
class SlaveConfigTree;
class SlaveConfigTreeNode;
class TemplateTree;
class XorpShell;
class XrlAtomList;

enum CliModeType {
    CLI_MODE_NONE		= 0,
    CLI_MODE_OPERATIONAL	= 1,
    CLI_MODE_CONFIGURE		= 2,
    CLI_MODE_TEXTENTRY		= 3
};

class RouterCLI {
public:
    RouterCLI(XorpShell& xorpsh, CliNode& cli_node, bool verbose);
    ~RouterCLI();

    bool is_config_mode() const;
    void commit_done_by_user(int uid);
    void clear_command_set();
    int configure_func(const string& ,
		       const string& ,
		       uint32_t ,
		       const string& command_global_name,
		       const vector<string>& argv);
    void enter_config_done(const XrlError& e);
    void got_config_users(const XrlError& e, const XrlAtomList* users);
    void new_config_user(uid_t user_id);
    void leave_config_done(const XrlError& e);
    void notify_user(const string& alert, bool urgent);
    int op_help_func(const string& ,
		   const string& ,
		   uint32_t ,
		   const string& command_global_name,
		   const vector<string>& argv);
    int conf_help_func(const string& ,
		   const string& ,
		   uint32_t ,
		   const string& command_global_name,
		   const vector<string>& argv);
    int logout_func(const string& ,
		   const string& ,
		   uint32_t ,
		   const string& command_global_name,
		   const vector<string>& argv);
    int exit_func(const string& ,
		  const string& ,
		  uint32_t ,
		  const string& command_global_name,
		  const vector<string>& argv);
    int edit_func(const string& ,
		  const string& ,
		  uint32_t ,
		  const string& command_global_name,
		  const vector<string>& argv);
    map <string, string> text_entry_children_func(const string& path,
						  bool& is_executable) const;
    int text_entry_func(const string& ,
			const string& ,
			uint32_t ,
			const string& command_global_name,
			const vector<string>& argv);
    int delete_func(const string& ,
		    const string& ,
		    uint32_t ,
		    const string& command_global_name,
		    const vector<string>& argv);
    int set_func(const string& ,
		 const string& ,
		 uint32_t ,
		 const string& command_global_name,
		 const vector<string>& argv);
    int immediate_set_func(const string& ,
			   const string& ,
			   uint32_t ,
			   const string& command_global_name,
			   const vector<string>& argv);
    int commit_func(const string& ,
		    const string& ,
		    uint32_t ,
		    const string& command_global_name,
		    const vector<string>& argv);
    void commit_done(bool success, string errmsg);

    int show_func(const string& ,
		  const string& ,
		  uint32_t ,
		  const string& command_global_name,
		  const vector<string>& argv);
    int op_mode_func(const string& ,
		     const string& ,
		     uint32_t ,
		     const string& command_global_name,
		     const vector<string>& argv);
    int save_func(const string& ,
		  const string& ,
		  uint32_t ,
		  const string& command_global_name,
		  const vector<string>& argv);
    void save_done(const XrlError& e);
    int load_func(const string& ,
		  const string& ,
		  uint32_t ,
		  const string& command_global_name,
		  const vector<string>& argv);
    void load_communicated(const XrlError& e);
    void load_done(bool success, string errmsg);

    map <string, string> op_mode_help(const string& path,
				      bool& is_executable) const;
    map <string, string> configure_mode_help(const string& path,
					     bool& is_executable) const;
    typedef XorpCallback2<void, bool, const string&>::RefPtr OpModeCallback;
    void op_mode_cmd_done(bool success, const string& result);

private:
    void reset_path();
    void set_path(string path);
    void apply_path_change();
    void add_static_configure_mode_commands();
    void set_prompt(const string& line1, 
		    const string& line2);
    void config_mode_prompt();
    void idle_ui();
    void reenable_ui();
    void silent_reenable_ui();

    string pathstr() const;
    string pathstr2() const;
    void operational_mode();
    void add_op_mode_commands(CliCommand *root);
    void configure_mode();
    void text_entry_mode();
    void add_command_subtree(CliCommand& current_cli_node,
			     const CommandTreeNode& current_ctn,
			     const CLI_PROCESS_CALLBACK& cli_process_callback,
			     string path, size_t depth);

#if 0
    /**
     * @short add commands for direct configuration of new nodes
     *
     * To add a new node in the command tree, you just start typing
     * the name of the node, and continue adding new config until the
     * new config subtree is complete.  add_immediate_commands adds
     * the names of possible new configution nodes to the command menu
     * to permit this. 
     *
     * @param current_cli_node the current position in the config tree.
     *
     * @param the command tree whose nodes need to be added to the CLI.
     *
     * @param cmd_names the list of template tree command names that
     * we will use to find the template_tree nodes that form the
     * command tree rooted at this config tree node.
     *
     * @param include_intermediates true if we want to add commands
     * for nodes indirectly rooted at the current node, false if we
     * only want to include commands that are direct children of the
     * current node.
     *
     * @param cb the callback to call if any of these commands is
     * typed by the user.
     *
     * @param path the configuration path to current position in the config 
     * tree
     */
    void add_immediate_commands(CliCommand& current_cli_node,
				const CommandTree& command_tree,
				const list<string>& cmd_names,
				bool include_intermediates,
				const CLI_PROCESS_CALLBACK& cb,
				const string& path);
#endif
    void add_text_entry_commands();

    void add_edit_subtree();
    void add_delete_subtree();
    void add_set_subtree();
    void add_show_subtree();

    void display_config_mode_users() const;
    void display_alerts();

    string run_set_command(const string& path, const vector<string>& argv);

    void check_for_rtrmgr_restart();
    void verify_rtrmgr_restart(const XrlError& e, const uint32_t* pid);

    string get_help_o(const string& s) const;
    string get_help_c(const string& s) const;

    string makepath(const list<string>& parts) const;
    list <string> splitpath(const string& path) const;

    const TemplateTree*	template_tree() const;
    SlaveConfigTree*	config_tree();
    OpCommandList*	op_cmd_list() const;

    XorpShell&		_xorpsh;

    //    SlaveConfigTreeNode* _current_config_node;

    CliNode&		_cli_node;
    CliClient&		_cli_client;
    bool		_verbose;	// Set to true if output is verbose
    CliModeType		_mode;
    CliCommand*		_set_node;
    CliCommand*		_show_node;
    CliCommand*		_edit_node;
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
};

#endif // __RTRMGR_CLI_HH__
