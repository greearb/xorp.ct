// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

// $XORP: xorp/rtrmgr/master_conf_tree.hh,v 1.38 2007/02/16 22:47:22 pavlin Exp $

#ifndef __RTRMGR_MASTER_CONF_TREE_HH__
#define __RTRMGR_MASTER_CONF_TREE_HH__


#include <map>
#include <list>
#include <set>

#include "conf_tree.hh"
#include "master_conf_tree_node.hh"
#include "rtrmgr_error.hh"
#include "task.hh"


class CommandTree;
class ConfTemplate;
class RouterCLI;
class MasterTemplateTree;

class MasterConfigTree : public ConfigTree {
    typedef XorpCallback2<void, bool, string>::RefPtr CallBack;
    typedef XorpCallback4<void, bool, string, string, string>::RefPtr ConfigChangeCallBack;
    typedef XorpCallback2<void, bool, string>::RefPtr ConfigSaveCallBack;
    typedef XorpCallback4<void, bool, string, string, string>::RefPtr ConfigLoadCallBack;

public:
    MasterConfigTree(const string& config_file, MasterTemplateTree* tt,
		     ModuleManager& mmgr, XorpClient& xclient,
		     bool global_do_exec, bool verbose) throw (InitError);
    MasterConfigTree(TemplateTree* tt, bool verbose);
    MasterConfigTree& operator=(const MasterConfigTree& orig_tree);
    ~MasterConfigTree();
    

    bool read_file(string& configuration, const string& config_file,
		   string& errmsg);
    bool parse(const string& configuration, const string& config_file,
	       string& errmsg);
    void execute();
    void config_done(bool success, string errmsg);

    virtual ConfigTreeNode* create_node(const string& segment, 
					const string& path,
					const TemplateTreeNode* ttn, 
					ConfigTreeNode* parent_node, 
					const ConfigNodeId& node_id,
					uid_t user_id, bool verbose);
    virtual ConfigTree* create_tree(TemplateTree *tt, bool verbose);

    void commit_changes_pass1(CallBack cb);
    void commit_pass1_done(bool success, string errmsg);
    void commit_changes_pass2();
    void commit_pass2_done(bool success, string errmsg);

    bool commit_in_progress() const { return _commit_in_progress; }
    bool config_failed() const { return _config_failed; }
    const string& config_failed_msg() const { return _config_failed_msg; }
    bool check_commit_status(string& response);
    string discard_changes();
    string mark_subtree_for_deletion(const list<string>& path_segments, 
				     uid_t user_id);
    void delete_entire_config();
    bool lock_node(const string& node, uid_t user_id, uint32_t timeout, 
		   uint32_t& holder);
    bool unlock_node(const string& node, uid_t user_id);

    bool change_config(uid_t user_id, CallBack cb, string& errmsg);
    bool apply_config_change(uid_t user_id, string& errmsg,
			     const string& deltas,
			     const string& deletions,
			     ConfigChangeCallBack cb);
    bool save_config(const string& filename, uid_t user_id,
		     string& errmsg, ConfigSaveCallBack cb);
    bool load_config(const string& filename, uid_t user_id, string& errmsg,
		     ConfigLoadCallBack cb);

    void set_config_directory(const string& config_directory);

    ModuleManager& module_manager() const {
	return _task_manager->module_manager();
    }

    virtual ConfigTreeNode& root_node() {
	return _root_node;
    }
    virtual const ConfigTreeNode& const_root_node() const {
	return _root_node;
    }

    MasterConfigTreeNode& master_root_node() {
	return (MasterConfigTreeNode&)_root_node;
    }
    const MasterConfigTreeNode& const_master_root_node() const {
	return (const MasterConfigTreeNode&)_root_node;
    }

    MasterConfigTreeNode* find_node(const list<string>& path) {
	return (MasterConfigTreeNode*)(ConfigTree::find_node(path));
    }
    MasterConfigTreeNode* find_config_module(const string& module_name) {
	return (MasterConfigTreeNode*)(ConfigTree::find_config_module(module_name));
    }
    

private:
    string config_full_filename(const string& filename) const;

    void remove_tmp_config_file();
    bool set_config_file_permissions(FILE* fp, uid_t user_id, string& errmsg);

    void apply_config_commit_changes_cb(bool success, string errmsg,
					ConfigChangeCallBack cb);
    void save_config_file_sent_cb(bool success, string errmsg,
				  string filename, uid_t user_id,
				  ConfigSaveCallBack cb);
    void save_config_file_cleanup_cb(bool success, string errmsg,
				     bool orig_success, string orig_errmsg,
				     string filename, uid_t user_id,
				     ConfigSaveCallBack cb);
    void save_config_done_cb(bool success, string errmsg,
			     ConfigSaveCallBack cb);
    bool save_to_file(const string& filename, uid_t user_id, string& errmsg);

    void load_config_file_received_cb(bool success, string errmsg,
				      string filename, uid_t user_id,
				      ConfigLoadCallBack cb);
    void load_config_file_cleanup_cb(bool success, string errmsg,
				     bool orig_success, string orig_errmsg,
				     string rtrmgr_config_value,
				     string filename, uid_t user_id,
				     ConfigLoadCallBack cb);
    void load_config_commit_changes_cb(bool success, string errmsg,
				       string deltas, string deletions,
				       ConfigLoadCallBack cb);
    bool load_from_file(const string& filename, uid_t user_id, string& errmsg,
			string& deltas, string& deletions);

    void diff_configs(const MasterConfigTree& new_tree, 
		      MasterConfigTree& delta_tree,
		      MasterConfigTree& deletion_tree);
    list<string> find_changed_modules() const;
    list<string> find_active_modules() const;
    list<string> find_inactive_modules() const;
    void order_module_list(const set<string>& module_set,
			   list<string>& ordered_modules) const;
    bool module_config_start(const string& module_name, string& error_msg);
    bool module_shutdown(const string& module_name, string& error_msg);

    bool do_exec() const { return _task_manager->do_exec(); }
    bool verbose() const { return _task_manager->verbose(); }

    XorpClient& xorp_client() const { return _task_manager->xorp_client(); }

    MasterConfigTreeNode _root_node;
    TaskManager*        _task_manager;
    CallBack		_commit_cb;
    bool		_commit_in_progress;
    bool		_config_failed;
    string		_config_failed_msg;
    bool		_rtrmgr_config_node_found;
    XorpTimer		_save_config_completed_timer;
    string		_tmp_config_filename;
    string		_config_directory;

    gid_t		_xorp_gid;
    bool		_is_xorp_gid_set;
    RunCommand::ExecId	_exec_id;
    bool		_enable_program_exec_id;

    MasterConfigTree*	_config_tree_copy;
};

#endif // __RTRMGR_MASTER_CONF_TREE_HH__
