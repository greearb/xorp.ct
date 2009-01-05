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

// $XORP: xorp/rtrmgr/slave_conf_tree.hh,v 1.31 2008/10/02 21:58:24 bms Exp $

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
#include "xorpsh_base.hh"
#include "xorpsh_main.hh"


class CommandTree;
class ConfTemplate;
class RouterCLI;

/**
 * @short A class for storing information regarding the current phase of
 * commit operations.
 */
class CommitStatus {
public:
    enum Phase {
	COMMIT_PHASE_NONE = 0,
	COMMIT_PHASE_1,
	COMMIT_PHASE_2,
	COMMIT_PHASE_3,
	COMMIT_PHASE_4,
	COMMIT_PHASE_5,
	COMMIT_PHASE_DONE
    };

    /**
     * Default constructor.
     */
    CommitStatus() {
	reset();
    }

    /**
     * Reset the commit status.
     */
    void reset() {
	_success = true;
	_error_msg = "";
	_commit_phase = COMMIT_PHASE_NONE;
    }

    /**
     * Test if the current phase has been successful.
     * 
     * @return true if the current phase has been successful, otherwise false.
     */
    bool success() const { return _success; }

    /**
     * Get a string with the current error message.
     * 
     * @return a string with the current error message.
     */
    const string& error_msg() const { return _error_msg; }

    /**
     * Set the commit status as being in error.
     * 
     * @param error_msg the message that describes the error.
     */
    void set_error(const string& error_msg) {
        _success = false;
        _error_msg = error_msg;
    }

    /**
     * Get the current commit phase.
     * 
     * @return the current commit phase.
     */
    CommitStatus::Phase commit_phase() const { return _commit_phase; }

    /**
     * Set the current commit phase.
     * 
     * @param commit_phase the new value of the current commit phase.
     */
    void set_commit_phase(CommitStatus::Phase commit_phase) {
	_commit_phase = commit_phase;
    }

private:
    bool	_success;	// True if current commit phase is successful
    string	_error_msg;	// The error message (if error)
    Phase	_commit_phase;	// The current commit phase
};


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
					const ConfigNodeId& node_id,
					uid_t user_id, bool verbose);
    virtual ConfigTree* create_tree(TemplateTree *tt, bool verbose);


    bool parse(const string& configuration, const string& config_file,
	       string& errmsg);
    bool commit_changes(string& response, XorpShellBase& xorpsh, CallBack cb);
    void commit_phase2(const XrlError& e, const bool* locked,
		       const uint32_t* lock_holder, CallBack cb,
		       XorpShellBase* xorpsh);
    void commit_phase3(const XrlError& e, CallBack cb, XorpShellBase* xorpsh);
    void commit_phase4(bool success, const string& errmsg, CallBack cb,
		       XorpShellBase* xorpsh);
    void commit_phase5(const XrlError& e, bool success, CallBack cb,
		       XorpShellBase* xorpsh);
    void save_phase4(bool success, const string& errmsg, CallBack cb,
		     XorpShellBase* xorpsh);
    void save_phase5(const XrlError& e, bool success, CallBack cb,
		     XorpShellBase* xorpsh);
    string discard_changes();
    string mark_subtree_for_deletion(const list<string>& path_segments, 
				     uid_t user_id);

    bool get_deltas(const SlaveConfigTree& main_tree);
    bool get_deletions(const SlaveConfigTree& main_tree);

    virtual ConfigTreeNode& root_node() { return _root_node; }
    virtual const ConfigTreeNode& const_root_node() const {
	return _root_node;
    }

    SlaveConfigTreeNode& slave_root_node() { return _root_node; }
    const SlaveConfigTreeNode& const_slave_root_node() const {
	return _root_node;
    }
    SlaveConfigTreeNode* find_node(const list<string>& path) {
	return reinterpret_cast<SlaveConfigTreeNode*>(ConfigTree::find_node(path));
    }

    const CommitStatus& commit_status() const { return _commit_status; }

    void reset_commit_status() { _commit_status.reset(); }

private:
    SlaveConfigTreeNode	_root_node;
    XorpClient&		_xclient;

    XorpShellBase::LOCK_CALLBACK _stage2_cb;

    string		_commit_errmsg;
    string		_save_errmsg;
    uint32_t		_clientid;
    bool		_verbose;	// Set to true if output is verbose

    CommitStatus	_commit_status;
};

#endif // __RTRMGR_SLAVE_CONF_FILE_HH__
