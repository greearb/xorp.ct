// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/rtrmgr/master_conf_tree_node.hh,v 1.24 2008/10/02 21:58:23 bms Exp $

#ifndef __RTRMGR_MASTER_CONF_TREE_NODE_HH__
#define __RTRMGR_MASTER_CONF_TREE_NODE_HH__

#include "libxorp/xorp.h"






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
