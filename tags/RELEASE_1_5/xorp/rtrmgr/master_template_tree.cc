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

#ident "$XORP: xorp/rtrmgr/master_template_tree.cc,v 1.12 2008/01/04 03:17:40 pavlin Exp $"

#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HAVE_GLOB_H
#include <glob.h>
#elif defined(HOST_OS_WINDOWS)
#include "glob_win32.h"
#endif

#include "master_template_tree.hh"
#include "master_template_tree_node.hh"


MasterTemplateTree::MasterTemplateTree(const string& xorp_root_dir,
				       XRLdb& xrldb,
				       bool verbose) throw (InitError)
    : TemplateTree(xorp_root_dir, verbose),
      _xrldb(xrldb)
{

}

bool 
MasterTemplateTree::load_template_tree(const string& config_template_dir,
					string& error_msg)
{
    if (TemplateTree::load_template_tree(config_template_dir, error_msg)
	!= true) {
	return (false);
    }

    if (expand_master_template_tree(error_msg) != true)
	return (false);

    if (check_master_template_tree(error_msg) != true)
	return (false);

    return (true);
}

bool
MasterTemplateTree::expand_master_template_tree(string& error_msg)
{
    // Expand the template tree
    return root_node()->expand_master_template_tree(error_msg);
}

bool
MasterTemplateTree::check_master_template_tree(string& error_msg)
{
    // Verify the template tree
    return root_node()->check_master_template_tree(error_msg);
}

void
MasterTemplateTree::add_cmd(char* cmd) throw (ParseError)
{
    MasterTemplateTreeNode *n = (MasterTemplateTreeNode*)_current_node;
    n->add_cmd(string(cmd), *this);
}

void
MasterTemplateTree::add_cmd_action(const string& cmd, 
				   const list<string>& action)
    throw (ParseError)
{
    MasterTemplateTreeNode *n = (MasterTemplateTreeNode*)_current_node;
    n->add_action(cmd, action, _xrldb);
}
