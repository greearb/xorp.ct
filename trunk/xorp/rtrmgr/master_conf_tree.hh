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

// $XORP: xorp/rtrmgr/master_conf_tree.hh,v 1.14 2003/12/15 22:31:24 pavlin Exp $

#ifndef __RTRMGR_MASTER_CONF_TREE_HH__
#define __RTRMGR_MASTER_CONF_TREE_HH__

#include <map>
#include <list>
#include <set>

#include "libxorp/xorp.h"

#include "main_rtrmgr.hh"
#include "task.hh"
#include "parse_error.hh"
#include "conf_tree.hh"


class CommandTree;
class ConfTemplate;
class RouterCLI;

class MasterConfigTree : public ConfigTree {
    typedef XorpCallback2<void, bool, string>::RefPtr CallBack;

public:
    MasterConfigTree(const string& config_file, TemplateTree* tt,
		     ModuleManager& mmgr, XorpClient& xclient,
		     bool global_do_exec) throw (InitError);

    bool read_file(string& configuration, const string& config_file,
		   string& errmsg);
    bool parse(const string& configuration, const string& config_file);
    void execute();
    void config_done(bool success, string errmsg);

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

    bool save_to_file(const string& filename, uid_t user_id, string& errmsg);
    bool load_from_file(const string& filename, uid_t user_id, string& errmsg,
			string& deltas, string& deletions);

private:
    void diff_configs(const ConfigTree& new_tree, ConfigTree& delta_tree,
		      ConfigTree& deletion_tree);
    list<string> find_changed_modules() const;
    list<string> find_active_modules() const;
    list<string> find_inactive_modules() const;
    void order_module_list(const set<string>& module_set,
			   list<string>& ordered_modules) const;
    bool module_config_start(const string& module_name, string& errmsg);
    bool module_shutdown(const string& module_name, string& errmsg);

    bool do_exec() const { return _task_manager.do_exec(); }

    ModuleManager& module_manager() const {
	return _task_manager.module_manager();
    }

    XorpClient& xorp_client() const { return _task_manager.xorp_client(); }

    TaskManager		_task_manager;
    CallBack		_commit_cb;
    bool		_commit_in_progress;
    bool		_config_failed;
    string		_config_failed_msg;
};

#endif // __RTRMGR_MASTER_CONF_TREE_HH__
