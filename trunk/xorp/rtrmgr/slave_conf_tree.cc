// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/rtrmgr/slave_conf_tree.cc,v 1.37 2007/02/16 22:47:24 pavlin Exp $"


#include "rtrmgr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "cli.hh"
#include "slave_conf_tree.hh"
#include "slave_conf_tree_node.hh"
#include "template_tree_node.hh"
#include "template_commands.hh"
#include "template_tree.hh"
#include "util.hh"


extern int booterror(const char *s) throw (ParseError);

/*************************************************************************
 * Slave Config Tree class
 *************************************************************************/



SlaveConfigTree::SlaveConfigTree(XorpClient& xclient, bool verbose)
    : ConfigTree(NULL, verbose),
      _root_node(verbose),
      _xclient(xclient),
      _verbose(verbose)
{
   _current_node = &_root_node;

}

SlaveConfigTree::SlaveConfigTree(const string& configuration,
				 TemplateTree* tt,
				 XorpClient& xclient,
				 uint32_t clientid,
				 bool verbose) throw (InitError)
    : ConfigTree(tt, verbose),
      _root_node(verbose),
      _xclient(xclient),
      _clientid(clientid),
      _verbose(verbose)
{
    _current_node = &_root_node;

    string errmsg;

    if (parse(configuration, "", errmsg) != true) {
	xorp_throw(InitError, errmsg);
    }

    _root_node.mark_subtree_as_committed();
}

ConfigTree* SlaveConfigTree::create_tree(TemplateTree *tt, bool verbose)
{
    SlaveConfigTree *mct;
    mct = new SlaveConfigTree("", tt, _xclient, _clientid, verbose);
    return mct;
}

ConfigTreeNode*
SlaveConfigTree::create_node(const string& segment, const string& path,
			     const TemplateTreeNode* ttn,
			     ConfigTreeNode* parent_node,
			     const ConfigNodeId& node_id,
			     uid_t user_id, 
			     bool verbose)
{
    SlaveConfigTreeNode *ctn, *parent;
    parent = dynamic_cast<SlaveConfigTreeNode *>(parent_node);
    if (parent_node != NULL)
	XLOG_ASSERT(parent != NULL);
    ctn = new SlaveConfigTreeNode(segment, path, ttn, parent, 
				  node_id, user_id, _clientid, verbose);
    return reinterpret_cast<ConfigTreeNode*>(ctn);
}

bool
SlaveConfigTree::parse(const string& configuration, const string& config_file,
		       string& errmsg)
{
    if (ConfigTree::parse(configuration, config_file, errmsg) != true)
	return false;

    return true;
}

bool
SlaveConfigTree::commit_changes(string& result, XorpShellBase& xorpsh,
				CallBack cb)
{
    bool success = true;
    _commit_status.reset();

    XLOG_TRACE(_verbose,
	       "##########################################################\n");
    XLOG_TRACE(_verbose, "SlaveConfigTree::commit_changes\n");

#if 0	// TODO: XXX: FIX IT!!
    // XXX: we really should do this check, but we need to deal with
    // unexpanded variables more gracefully.

    //
    // Two passes: the first checks for errors.  If no errors are
    // found, attempt the actual commit.
    //
    size_t tid = 0;
    tid = _xclient.begin_transaction();
    _root_node.initialize_commit();
    bool needs_activate = false;
    bool needs_update = false;
    if (_root_node.commit_changes(NULL, "", _xclient, tid,
				  /* do_exec = */ false,
				  /* do_commit = */ false,
				  0, 0,
				  result,
				  needs_activate,
				  needs_update) == false) {
	// Something went wrong - return the error message.
	return false;
    }
    CallBack empty_cb;
    _xclient.end_transaction(tid, empty_cb);
#else
    // Check the tree: whether all mandatory children nodes are present, etc.
    if (_root_node.check_config_tree(result) == false) {
	_commit_status.set_error(result);
	return false;
    }
    UNUSED(result);
#endif

    _commit_status.set_commit_phase(CommitStatus::COMMIT_PHASE_1);

    _stage2_cb = callback(this, &SlaveConfigTree::commit_phase2, cb, &xorpsh);
    if (xorpsh.lock_config(_stage2_cb) != true) {
	result = c_format("ERROR: failed to lock the configuration. "
			  "No Finder?\n");
	_commit_status.set_error(result);
	return false;
    }

    return success;
}

void
SlaveConfigTree::commit_phase2(const XrlError& e, const bool* locked,
			       const uint32_t* /* lock_holder */,
			       CallBack cb, XorpShellBase* xorpsh)
{
    string error_msg;

    _commit_status.set_commit_phase(CommitStatus::COMMIT_PHASE_2);

    if (!locked || (e != XrlError::OKAY())) {
	error_msg = c_format("Failed to get lock");
	cb->dispatch(false, error_msg);
        _commit_status.set_error(error_msg);
	return;
    }

    slave_root_node().update_node_id_position();

    // We managed to get the master lock
    SlaveConfigTree delta_tree(_xclient, _verbose);
    delta_tree.get_deltas(*this);
    string deltas = delta_tree.show_unannotated_tree(/*numbered*/ true);

    SlaveConfigTree withdraw_tree(_xclient, _verbose);
    withdraw_tree.get_deletions(*this);
    string deletions = withdraw_tree.show_unannotated_tree(/*numbered*/ true);

    XLOG_TRACE(_verbose, "deletions = >>>\n%s<<<\n", deletions.c_str());

    if (xorpsh->commit_changes(deltas,
			       deletions,
			       callback(this, &SlaveConfigTree::commit_phase3,
					cb, xorpsh),
			       cb)
	!= true) {
	error_msg = c_format("Cannot commit the configuration changes. "
			     "No Finder?");
	cb->dispatch(false, error_msg);
        _commit_status.set_error(error_msg);
	return;
    }
}

void
SlaveConfigTree::commit_phase3(const XrlError& e, CallBack cb,
			       XorpShellBase* xorpsh)
{
    bool success = true;
    bool should_dispatch_phase5 = false;
    XLOG_TRACE(_verbose, "commit_phase3\n");
    _commit_status.set_commit_phase(CommitStatus::COMMIT_PHASE_3);

    //
    // We get here when the rtrmgr has received our request, but before
    // it has actually tried to do the commit.  If we got an error, we
    // call unlock_config immediately.  Otherwise we don't unlock it
    // until we get called back with the final results of the commit.
    //
    if (e != XrlError::OKAY()) {
	success = false;
	_commit_errmsg = e.note();
	if (xorpsh->unlock_config(callback(this, &SlaveConfigTree::commit_phase5,
					   false, cb, xorpsh))
	    != true) {
	    XLOG_WARNING("Cannot unlock the config tree. No Finder?");
	    // XXX: need to dispatch the callback to move to phase5
	    should_dispatch_phase5 = true;
	}
	_commit_status.set_error(_commit_errmsg);
    }
    xorpsh->set_mode(XorpShellBase::MODE_COMMITTING);

    if (should_dispatch_phase5)
	commit_phase5(e, success, cb, xorpsh);
}

void
SlaveConfigTree::commit_phase4(bool success, const string& errmsg, CallBack cb,
			       XorpShellBase* xorpsh)
{
    bool should_dispatch_phase5 = false;
    string error_msg;

    XLOG_TRACE(_verbose, "commit_phase4\n");
    _commit_status.set_commit_phase(CommitStatus::COMMIT_PHASE_4);

    //
    // We get here when we're called back by the rtrmgr with the
    // results of our commit.
    //
    _commit_errmsg = errmsg;
    if (xorpsh->unlock_config(callback(this, &SlaveConfigTree::commit_phase5,
				       success, cb, xorpsh))
	!= true) {
	XLOG_WARNING("Cannot unlock the config tree. No Finder?");
	// XXX: need to dispatch the callback to move to phase5
	should_dispatch_phase5 = true;
    }

    if (should_dispatch_phase5) {
	//
	// XXX: here we ignore the fact that the unlock may have failed,
	// because the commit itself has succeeded.
	// 
	commit_phase5(XrlError::OKAY(), success, cb, xorpsh);
    }
}

void
SlaveConfigTree::commit_phase5(const XrlError& /* e */,
			       bool success,
			       CallBack cb,
			       XorpShellBase* /* xorpsh */)
{
    XLOG_TRACE(_verbose, "commit_phase5\n");
    _commit_status.set_commit_phase(CommitStatus::COMMIT_PHASE_5);

    if (success) {
	slave_root_node().finalize_commit();
	cb->dispatch(true, "");
	_commit_status.set_commit_phase(CommitStatus::COMMIT_PHASE_DONE);
    } else {
	cb->dispatch(false, _commit_errmsg);
	_commit_status.set_error(_commit_errmsg);
    }
}

void
SlaveConfigTree::save_phase4(bool success, const string& errmsg, CallBack cb,
			     XorpShellBase *xorpsh)
{
    bool should_dispatch_phase5 = false;
    XLOG_TRACE(_verbose, "save_phase4\n");

    //
    // We get here when we're called back by the rtrmgr with the
    // results of our save.
    //
    _save_errmsg = errmsg;
    if (xorpsh->unlock_config(callback(this, &SlaveConfigTree::save_phase5,
				   success, cb, xorpsh))
	!= true) {
	// XXX: need to dispatch the callback to move to phase5
	should_dispatch_phase5 = true;
    }

    if (should_dispatch_phase5) {
	//
	// XXX: here we ignore the fact that the unlock may have failed,
	// because the save itself has succeeded.
	// 
	save_phase5(XrlError::OKAY(), success, cb, xorpsh);
    }
}

void
SlaveConfigTree::save_phase5(const XrlError& /* e */,
			     bool success,
			     CallBack cb,
			     XorpShellBase* /* xorpsh */)
{
    XLOG_TRACE(_verbose, "save_phase5\n");

    if (success) {
	cb->dispatch(true, "");
    } else {
	cb->dispatch(false, _save_errmsg);
    }
}

bool
SlaveConfigTree::get_deltas(const SlaveConfigTree& main_tree)
{
    XLOG_TRACE(_verbose, "SlaveConfigTree::get_deltas\n");

    if (slave_root_node().get_deltas(main_tree.const_slave_root_node()) > 0) {
	debug_msg("FOUND DELTAS:\n");
	debug_msg("%s", tree_str().c_str());
	return true;
    }

    return false;
}

bool
SlaveConfigTree::get_deletions(const SlaveConfigTree& main_tree)
{
    XLOG_TRACE(_verbose, "SlaveConfigTree::get_deltas\n");

    if (slave_root_node().get_deletions(main_tree.const_slave_root_node()) > 0) {
	debug_msg("FOUND DELETIONS:>>>>\n");
	debug_msg("%s", tree_str().c_str());
	debug_msg("<<<<\n");
	return true;
    }

    return false;
}

string
SlaveConfigTree::discard_changes()
{
    XLOG_TRACE(_verbose,
	       "##########################################################\n");
    XLOG_TRACE(_verbose, "SlaveConfigTree::discard_changes\n");

    string result = _root_node.discard_changes(0, 0);

    XLOG_TRACE(_verbose,
	       "##########################################################\n");

    return result;
}

string
SlaveConfigTree::mark_subtree_for_deletion(const list<string>& path_segments,
				    uid_t user_id)
{
    SlaveConfigTreeNode *found = find_node(path_segments);
    if (found == NULL)
	return string("ERROR");

    if ((found->parent() != NULL) /* this is not the root node */
        && (found->parent()->parent() != NULL)
	&& found->parent()->is_tag()
	&& (found->parent()->children().size() == 1)) {
	found = found->parent();
    }

    found->mark_subtree_for_deletion(user_id);
    return string("OK");
}
