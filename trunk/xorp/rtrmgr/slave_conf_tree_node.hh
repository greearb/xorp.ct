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

// $XORP: xorp/rtrmgr/slave_conf_tree_node.hh,v 1.1.1.1 2002/12/11 23:56:16 hodson Exp $

#ifndef __RTRMGR_SLAVE_CONFIG_TREE_NODE_HH__
#define __RTRMGR_SLAVE_CONFIG_TREE_NODE_HH__

#include <map>
#include <list>
#include <set>
#include <vector>
#include <sys/time.h>
#include "config.h"
#include "libxorp/xorp.h"
#include "module_manager.hh"
#include "xorp_client.hh"
#include "conf_tree_node.hh"

class RouterCLI;
class CommandTree;
class TemplateTreeNode;
class Command;

class SlaveConfigTreeNode : public ConfigTreeNode {
public:
    SlaveConfigTreeNode();
    SlaveConfigTreeNode(const string &node_name, const string &path, 
		   const TemplateTreeNode *ttn, 
		   SlaveConfigTreeNode *parent, uid_t user_id);
    CommandTree *get_command_tree(const list<string>& commands,
				  bool include_intermediates,
				  bool include_templates) const;

    bool check_allowed_value(string& errmsg) const;

    //adaptors so we don't need to cast elsewhere
    inline SlaveConfigTreeNode* parent() {
	return (SlaveConfigTreeNode*)_parent;
    }
    int get_deltas(const SlaveConfigTreeNode& master_node);
    int get_deletions(const SlaveConfigTreeNode& master_node);

protected:
    bool build_command_tree(const list<string>& commands, 
			    CommandTree &cmdtree, int depth,
			    bool include_intermediates,
			    bool include_templates) const;
private:
    //don't add any storage here, SlaveConfigTreeNode needs to be the
    //same size as ConfigTreeNode
};

#endif // __RTRMGR_SLAVE_CONFIG_TREE_NODE_HH__
