// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2005 International Computer Science Institute
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

// $XORP: xorp/rtrmgr/slave_conf_tree_node.hh,v 1.16 2005/09/27 18:37:31 pavlin Exp $

#ifndef __RTRMGR_SLAVE_CONF_TREE_NODE_HH__
#define __RTRMGR_SLAVE_CONF_TREE_NODE_HH__

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <map>
#include <list>
#include <set>
#include <vector>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "conf_tree_node.hh"
#include "module_manager.hh"
#include "xorp_client.hh"


class Command;
class CommandTree;
class RouterCLI;
class TemplateTreeNode;

class SlaveConfigTreeNode : public ConfigTreeNode {
public:
    SlaveConfigTreeNode(bool verbose);
    SlaveConfigTreeNode(const string& node_name, const string& path,
			const TemplateTreeNode* ttn,
			SlaveConfigTreeNode* parent,
			const ConfigNodeId& node_id,
			uid_t user_id,
			uint32_t clientid,
			bool verbose);

    virtual ConfigTreeNode* create_node(const string& segment, 
					const string& path,
					const TemplateTreeNode* ttn,
					ConfigTreeNode* parent_node,
					const ConfigNodeId& node_id,
					uid_t user_id, 
					uint32_t clientid,
					bool verbose);
    virtual ConfigTreeNode* create_node(const ConfigTreeNode& ctn);

    void create_command_tree(CommandTree& cmd_tree,
			     const list<string>& commands,
			     bool include_intermediate_nodes,
			     bool include_children_templates,
			     bool include_leaf_value_nodes,
			     bool include_read_only_nodes) const;

    bool check_allowed_value(string& errmsg) const;
    int get_deltas(const SlaveConfigTreeNode& master_node);
    int get_deletions(const SlaveConfigTreeNode& master_node);

    // adaptors so we don't need to cast elsewhere
    inline SlaveConfigTreeNode* parent() {
	return (SlaveConfigTreeNode*)_parent;
    }

    void finalize_commit();

protected:
    bool build_command_tree(CommandTree& cmdtree, 
			    const list<string>& commands, 
			    int depth,
			    bool include_intermediate_nodes,
			    bool include_children_templates,
			    bool include_leaf_value_nodes,
			    bool include_read_only_nodes) const;

private:
    //
    // XXX: don't add any storage here, SlaveConfigTreeNode needs to be the
    // same size as ConfigTreeNode.
    //
};

#endif // __RTRMGR_SLAVE_CONF_TREE_NODE_HH__
