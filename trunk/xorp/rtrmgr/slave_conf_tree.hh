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

// $XORP: xorp/rtrmgr/slave_conf_tree.hh,v 1.10 2004/01/15 08:51:58 pavlin Exp $

#ifndef __RTRMGR_SLAVE_CONF_FILE_HH__
#define __RTRMGR_SLAVE_CONF_FILE_HH__

#include <map>
#include <list>
#include <set>
#include "libxorp/xorp.h"
#include "conf_tree_node.hh"
#include "module_manager.hh"
#include "xorp_client.hh"
#include "rtrmgr_error.hh"
#include "conf_tree.hh"
#include "xorpsh_main.hh"


class CommandTree;
class ConfTemplate;
class RouterCLI;
class SlaveConfigTreeNode;

class SlaveConfigTree : public ConfigTree {
    typedef XorpCallback2<void, bool, string>::RefPtr CallBack;
public:
    SlaveConfigTree(XorpClient& xclient, bool verbose);
    SlaveConfigTree(const string& configuration, TemplateTree *tt,
		    XorpClient& xclient, bool verbose) throw (InitError);

    bool parse(const string& configuration, const string& config_file,
	       string& errmsg);
    bool commit_changes(string& response, XorpShell& xorpsh, CallBack cb);
    void commit_phase2(const XrlError& e, const bool* locked,
		       const uint32_t* lock_holder, CallBack cb,
		       XorpShell *xorpsh);
    void commit_phase3(const XrlError& e, CallBack cb, XorpShell* xorpsh);
    void commit_phase4(bool success, const string& errmsg, CallBack cb,
		       XorpShell* xorpsh);
    void commit_phase5(const XrlError& e, bool success, CallBack cb,
		       XorpShell* xorpsh);
    string discard_changes();
    string mark_subtree_for_deletion(const list<string>& path_segments, 
				     uid_t user_id);

    bool get_deltas(const SlaveConfigTree& main_tree);
    bool get_deletions(const SlaveConfigTree& main_tree);

    // Adaptors so we don't need to cast elsewhere
    inline SlaveConfigTreeNode& root_node() {
	return reinterpret_cast<SlaveConfigTreeNode&>(ConfigTree::root_node());
    }
    inline const SlaveConfigTreeNode& const_root_node() const {
	return reinterpret_cast<const SlaveConfigTreeNode&>(ConfigTree::const_root_node());
    }
    inline SlaveConfigTreeNode* find_node(const list<string>& path) {
	return reinterpret_cast<SlaveConfigTreeNode*>(ConfigTree::find_node(path));
    }

private:
    XorpClient&	_xclient;

    XorpShell::LOCK_CALLBACK _stage2_cb;

    string	_commit_errmsg;
    bool	_verbose;	// Set to true if output is verbose
};

#endif // __RTRMGR_SLAVE_CONF_FILE_HH__
