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

// $XORP: xorp/rtrmgr/slave_conf_tree.hh,v 1.18 2005/07/03 21:06:00 mjh Exp $

#ifndef __RTRMGR_SLAVE_CONF_FILE_HH__
#define __RTRMGR_SLAVE_CONF_FILE_HH__


#include <map>
#include <list>
#include <set>

#include "conf_tree.hh"
#include "slave_conf_tree_node.hh"
#include "slave_module_manager.hh"
#include "rtrmgr_error.hh"
#include "xorp_client.hh"
#include "xorpsh_main.hh"


class CommandTree;
class ConfTemplate;
class RouterCLI;

class SlaveConfigTree : public ConfigTree {
    typedef XorpCallback2<void, bool, string>::RefPtr CallBack;
public:
    SlaveConfigTree(XorpClient& xclient, bool verbose);
    SlaveConfigTree(const string& configuration, TemplateTree *tt,
		    XorpClient& xclient, uint32_t clientid, 
		    bool verbose) throw (InitError);
    virtual ConfigTreeNode* create_node(const string& segment, 
					const string& path,
					const TemplateTreeNode* ttn, 
					ConfigTreeNode* parent_node, 
					uint64_t nodenum,
					uid_t user_id, bool verbose);
    virtual ConfigTree* create_tree(TemplateTree *tt, bool verbose);


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
    void save_phase4(bool success, const string& errmsg, CallBack cb,
		     XorpShell* xorpsh);
    void save_phase5(const XrlError& e, bool success, CallBack cb,
		     XorpShell* xorpsh);
    string discard_changes();
    string mark_subtree_for_deletion(const list<string>& path_segments, 
				     uid_t user_id);

    bool get_deltas(const SlaveConfigTree& main_tree);
    bool get_deletions(const SlaveConfigTree& main_tree);

    virtual ConfigTreeNode& root_node() {
	return _root_node;
    }
    virtual const ConfigTreeNode& const_root_node() const {
	return _root_node;
    }

    inline SlaveConfigTreeNode& slave_root_node() {
	return _root_node;
    }
    inline const SlaveConfigTreeNode& const_slave_root_node() const {
	return _root_node;
    }
    inline SlaveConfigTreeNode* find_node(const list<string>& path) {
	return reinterpret_cast<SlaveConfigTreeNode*>(ConfigTree::find_node(path));
    }

private:
    SlaveConfigTreeNode _root_node;
    XorpClient&	_xclient;

    XorpShell::LOCK_CALLBACK _stage2_cb;

    string	_commit_errmsg;
    string	_save_errmsg;
    uint32_t    _clientid;
    bool	_verbose;	// Set to true if output is verbose
};

#endif // __RTRMGR_SLAVE_CONF_FILE_HH__
