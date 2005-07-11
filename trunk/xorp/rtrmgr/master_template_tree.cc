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

#ident "$XORP: xorp/rtrmgr/master_template_tree.cc,v 1.3 2005/07/11 21:49:29 pavlin Exp $"


#include <glob.h>

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "master_template_tree.hh"
#include "master_template_tree_node.hh"

MasterTemplateTree::MasterTemplateTree(const string& xorp_root_dir,
				       XRLdb& xrldb,
				       bool verbose) throw (InitError)
    : TemplateTree(xorp_root_dir, verbose),
      _xrldb(xrldb)
{
    string errmsg;

}

bool 
MasterTemplateTree::load_template_tree(const string& config_template_dir,
				       string& errmsg)
{
    if (TemplateTree::load_template_tree(config_template_dir, errmsg) != true)
	return (false);

    if (expand_template_tree(errmsg) != true)
	return (false);

    if (check_template_tree(errmsg) != true)
	return (false);

    return (true);
}

bool
MasterTemplateTree::expand_template_tree(string& errmsg)
{
    // Expand the template tree
    return root_node()->expand_template_tree(errmsg);
}

bool
MasterTemplateTree::check_template_tree(string& errmsg)
{
    // Verify the template tree
    return root_node()->check_template_tree(errmsg);
}

void
MasterTemplateTree::add_cmd(char* cmd)
{
    MasterTemplateTreeNode *n = (MasterTemplateTreeNode*)_current_node;
    n->add_cmd(string(cmd), *this);
}

void
MasterTemplateTree::add_cmd_action(const string& cmd, 
				   const list<string>& action)
{
    MasterTemplateTreeNode *n = (MasterTemplateTreeNode*)_current_node;
    n->add_action(cmd, action, _xrldb);
}
