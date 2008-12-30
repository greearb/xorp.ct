// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/rtrmgr/master_template_tree_node.hh,v 1.10 2008/07/23 05:11:42 pavlin Exp $

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
