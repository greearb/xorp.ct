// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/rtrmgr/slave_conf_tree.hh,v 1.12 2002/12/09 18:29:38 hodson Exp $

#ifndef __RTRMGR_SLAVE_CONF_FILE_HH__
#define __RTRMGR_SLAVE_CONF_FILE_HH__

#include <map>
#include <list>
#include <set>
#include "config.h"
#include "libxorp/xorp.h"
#include "conf_tree_node.hh"
#include "module_manager.hh"
#include "xorp_client.hh"
#include "parse_error.hh"
#include "conf_tree.hh"
#include "xorpsh_main.hh"

class RouterCLI;
class CommandTree;
class ConfTemplate;
class SlaveConfigTreeNode;

class SlaveConfigTree : public ConfigTree {
public:
    SlaveConfigTree();
    SlaveConfigTree(const string& configuration, TemplateTree *ct,
		    XorpClient *xclient);
    int parse(const string& configuration, 
	      const string& conffile);
    bool commit_changes(string &response,
			XorpShell& xorpsh,
			XorpBatch::CommitCallback cb);
    void commit_phase2(const XrlError& e, 
		       const bool* locked, 
		       const uint32_t* lock_holder,
		       XorpBatch::CommitCallback cb,
		       XorpShell *xorpsh);
    void commit_phase3(const XrlError& e, 
		       XorpBatch::CommitCallback cb,
		       XorpShell* xorpsh);
    void commit_phase4(bool success, const string& errmsg,
		       XorpBatch::CommitCallback cb,
		       XorpShell* xorpsh);
    void commit_phase5(const XrlError& e, 
		       bool success,
		       XorpBatch::CommitCallback cb,
		       XorpShell* xorpsh);
    string discard_changes();
    string mark_subtree_for_deletion(const list <string>& pathsegs, 
				     uid_t user_id);

    bool get_deltas(const SlaveConfigTree &main_tree);
    bool get_deletions(const SlaveConfigTree &main_tree);

    //adaptors so we don't need to cast elsewhere
    inline SlaveConfigTreeNode* root() {
	return (SlaveConfigTreeNode*)(((ConfigTree*)this)->root());
    }
    inline const SlaveConfigTreeNode* const_root() const {
	return (const SlaveConfigTreeNode*)
	    (((const ConfigTree*)this)->const_root());
    }
    inline SlaveConfigTreeNode *find_node(const list <string>& path) {
	return (SlaveConfigTreeNode*)(((ConfigTree*)this)->find_node(path));
    }
    
private:
    XorpClient *_xclient;

    XorpShell::LOCK_CALLBACK _stage2_cb;

    string _commit_errmsg;
};

#endif // __RTRMGR_SLAVE_CONF_FILE_HH__
