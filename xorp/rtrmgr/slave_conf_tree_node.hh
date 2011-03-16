// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// $XORP: xorp/rtrmgr/slave_conf_tree_node.hh,v 1.28 2008/10/02 21:58:24 bms Exp $

#ifndef __RTRMGR_SLAVE_CONF_TREE_NODE_HH__
#define __RTRMGR_SLAVE_CONF_TREE_NODE_HH__

#include "libxorp/xorp.h"






#include "conf_tree_node.hh"
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
			     bool include_read_only_nodes,
			     bool include_permanent_nodes,
			     bool include_user_hidden_nodes) const;

    int get_deltas(const SlaveConfigTreeNode& master_node);
    int get_deletions(const SlaveConfigTreeNode& master_node);

    // adaptors so we don't need to cast elsewhere
    SlaveConfigTreeNode* parent() { return (SlaveConfigTreeNode*)_parent; }

    void finalize_commit();

protected:
    bool build_command_tree(CommandTree& cmdtree, 
			    const list<string>& commands, 
			    int depth,
			    bool include_intermediate_nodes,
			    bool include_children_templates,
			    bool include_leaf_value_nodes,
			    bool include_read_only_nodes,
			    bool include_permanent_nodes,
			    bool include_user_hidden_nodes) const;

private:
    //
    // XXX: don't add any storage here, SlaveConfigTreeNode needs to be the
    // same size as ConfigTreeNode.
    //
};

#endif // __RTRMGR_SLAVE_CONF_TREE_NODE_HH__
