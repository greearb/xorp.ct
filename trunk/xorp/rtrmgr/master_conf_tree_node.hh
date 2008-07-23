// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/rtrmgr/master_conf_tree_node.hh,v 1.22 2008/01/04 03:17:40 pavlin Exp $

#ifndef __RTRMGR_MASTER_CONF_TREE_NODE_HH__
#define __RTRMGR_MASTER_CONF_TREE_NODE_HH__

#include "libxorp/xorp.h"

#include <map>
#include <list>
#include <set>
#include <vector>

#include "conf_tree_node.hh"
#include "task.hh"


class MasterConfigTreeNode : public ConfigTreeNode {
public:
    MasterConfigTreeNode(bool verbose);
    MasterConfigTreeNode(const MasterConfigTreeNode& ctn);
    MasterConfigTreeNode(const string& node_name, const string& path, 
			 const TemplateTreeNode* ttn, 
			 MasterConfigTreeNode* parent,
			 const ConfigNodeId& node_id,
			 uid_t user_id, bool verbose);

    virtual ConfigTreeNode* create_node(const string& segment, 
					const string& path,
					const TemplateTreeNode* ttn, 
					ConfigTreeNode* parent_node, 
					const ConfigNodeId& node_id,
					uid_t user_id,
					uint32_t clientid,
					bool verbose);
    virtual ConfigTreeNode* create_node(const ConfigTreeNode& ctn);
    void command_status_callback(const Command* cmd, bool success);

    void find_changed_modules(set<string>& changed_modules) const;
    void find_active_modules(set<string>& active_modules) const;
    void find_all_modules(set<string>& all_modules) const;

    void initialize_commit();
    bool children_changed();
    bool commit_changes(TaskManager& task_manager, bool do_commit,
			int depth, int last_depth, string& error_msg,
			bool& needs_activate, bool& needs_update);
    bool check_commit_status(string& error_msg) const;
    void finalize_commit();


protected:

    int _actions_pending;	// Needed to track how many response callbacks
				// callbacks we expect during a commit
    bool _actions_succeeded;	// Did any action fail during the commit?
    const Command* _cmd_that_failed;

private:
};

#endif // __RTRMGR_MASTER_CONF_TREE_NODE_HH__
