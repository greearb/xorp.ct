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

// $XORP: xorp/rtrmgr/master_template_tree.hh,v 1.9 2008/01/04 03:17:40 pavlin Exp $

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
