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

// $XORP: xorp/rtrmgr/master_template_tree_node.hh,v 1.9 2008/01/04 03:17:40 pavlin Exp $

#ifndef __RTRMGR_MASTER_TEMPLATE_TREE_NODE_HH__
#define __RTRMGR_MASTER_TEMPLATE_TREE_NODE_HH__


#include <map>
#include <list>
#include <set>
#include <vector>

#include "template_tree_node.hh"

class MasterTemplateTreeNode : public TemplateTreeNode {
public:
    /* normally we'll just call the TemplateTreeNode constructor directly*/
    MasterTemplateTreeNode(TemplateTree& template_tree, 
			   TemplateTreeNode* parent, 
			   const string& path, const string& varname)
	: TemplateTreeNode(template_tree, parent, path, varname)
    {}

    void add_cmd(const string& cmd, TemplateTree& tt) throw (ParseError);
    void add_action(const string& cmd, const list<string>& action_list,
		    const XRLdb& xrldb) throw (ParseError);
    bool expand_master_template_tree(string& error_msg);
    bool check_master_template_tree(string& error_msg) const;

protected:
private:
};


#endif // __RTRMGR_MASTER_TEMPLATE_TREE_NODE_HH__
