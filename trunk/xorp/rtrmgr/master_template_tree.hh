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

// $XORP: xorp/rtrmgr/master_template_tree.hh,v 1.10 2008/07/23 05:11:42 pavlin Exp $

#ifndef __RTRMGR_MASTER_TEMPLATE_TREE_HH__
#define __RTRMGR_MASTER_TEMPLATE_TREE_HH__


#include "xrldb.hh"
#include "template_tree.hh"
#include "master_template_tree_node.hh"

class MasterTemplateTree : public TemplateTree {
public:
    MasterTemplateTree(const string& xorp_root_dir,
		       XRLdb& xrldb,
		       bool verbose)  throw (InitError);

    bool load_template_tree(const string& config_template_dir,
			    string& error_msg);

    void add_cmd(char* cmd) throw (ParseError);
    void add_cmd_action(const string& cmd, const list<string>& action)
	throw (ParseError);
    const XRLdb& xrldb() const { return _xrldb; }

    const MasterTemplateTreeNode* find_node(const list<string>& path_segments) 
	const 
    {
	return (const MasterTemplateTreeNode*)
	    (TemplateTree::find_node(path_segments));
    }

    MasterTemplateTreeNode* root_node() const { 
	return (MasterTemplateTreeNode*)_root_node; 
    }
private:
    bool expand_master_template_tree(string& error_msg);
    bool check_master_template_tree(string& error_msg);

    XRLdb		_xrldb;
};

#endif // __RTRMGR_MASTER_TEMPLATE_TREE_HH__
