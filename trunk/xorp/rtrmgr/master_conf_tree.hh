// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/rtrmgr/master_conf_tree.hh,v 1.1.1.1 2002/12/11 23:56:15 hodson Exp $

#ifndef __RTRMGR_MASTER_CONF_TREE_HH__
#define __RTRMGR_MASTER_CONF_TREE_HH__

#include <map>
#include <list>
#include <set>
#include "config.h"
#include "libxorp/xorp.h"
#include "master_conf_tree_node.hh"
#include "module_manager.hh"
#include "xorp_client.hh"
#include "parse_error.hh"
#include "conf_tree.hh"

class RouterCLI;
class CommandTree;
class ConfTemplate;

class MasterConfigTree :public ConfigTree {
public:
    MasterConfigTree(const string& conffile, TemplateTree *ct, 
		     ModuleManager *mm,
		     XorpClient *xclient, bool no_execute);
    bool read_file(string& configuration, const string& conffile,
		   string& errmsg);
    bool parse(const string& configuration, const string& conffile);
    void execute();
    void config_done(int status, const string& errmsg);
    list <string> find_changed_modules() const;
    bool commit_changes(string &response,
			XorpBatch::CommitCallback cb);
    bool check_commit_status(string &response);
    string discard_changes();
    string mark_subtree_for_deletion(const list <string>& pathsegs, 
				     uid_t user_id);
    bool lock_node(const string& node, uid_t user_id, uint32_t timeout, 
		   uint32_t& holder);
    bool unlock_node(const string& node, uid_t user_id);

    bool save_to_file(const string& filename, uid_t user_id, string& errmsg);
    bool load_from_file(const string& filename, uid_t user_id,
			string& errmsg, string& deltas, string& deletions);

    /*adaptors so we don't need casts elsewhere*/
    MasterConfigTreeNode *root() { 
	return (MasterConfigTreeNode*)(&_root_node);
    }
    const MasterConfigTreeNode *const_root() const {
	return (const MasterConfigTreeNode*)(&_root_node);
    }
private:
    void diff_configs(const ConfigTree& new_tree, ConfigTree& delta_tree,
		      ConfigTree& deletion_tree);

    /**
     * Normally the command to start a new module gets added to the
     * xorp_client queue as part of the queuing of the configuration
     * of that module.  However, if the module doesn't need
     * configuration, but needs starting anyway (to satisfy a
     * dependency), then it will need to be explicitly started up.
     * explicit_module_startup does this. 
     */
    bool explicit_module_startup(const string& module_name,
				 uint tid, string& result);

    ModuleManager *_module_manager;
    XorpClient *_xclient;

    //if _no_execute is true, run in debug mode, saying what we'd do,
    //but not actually doing anything
    bool _no_execute; 
};

#endif // __RTRMGR_MASTER_CONF_TREE_HH__
