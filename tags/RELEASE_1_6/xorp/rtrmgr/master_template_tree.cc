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

#ident "$XORP: xorp/rtrmgr/master_template_tree.cc,v 1.14 2008/10/02 21:58:23 bms Exp $"

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
