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

#ident "$XORP: xorp/rtrmgr/slave_conf_tree.cc,v 1.19 2004/05/28 22:27:57 pavlin Exp $"


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
      _xclient(xclient),
      _verbose(verbose)
{

}

SlaveConfigTree::SlaveConfigTree(const string& configuration,
				 TemplateTree* tt,
				 XorpClient& xclient,
				 bool verbose) throw (InitError)
    : ConfigTree(tt, verbose),
      _xclient(xclient),
      _verbose(verbose)
{
    string errmsg;

    if (parse(configuration, "", errmsg) != true) {
	xorp_throw(InitError, errmsg);
    }

    _root_node.mark_subtree_as_committed();
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
SlaveConfigTree::commit_changes(string& result, XorpShell& xorpsh, CallBack cb)
{
    bool success = true;

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
    if (_root_node.commit_changes(NULL, "", _xclient, tid,
				  /* do_exec = */ false,
				  /* do_commit = */ false,
				  0, 0, result) == false) {
	// Something went wrong - return the error message.
	return false;
    }
    CallBack empty_cb;
    _xclient.end_transaction(tid, empty_cb);
#else
    // check that all mandatory node children are present
    if (_root_node.check_config_tree(result) == false) {
	return false;
    }
    UNUSED(result);
#endif

    _stage2_cb = callback(this, &SlaveConfigTree::commit_phase2, cb, &xorpsh);
    xorpsh.lock_config(_stage2_cb);
    return success;
}

void
SlaveConfigTree::commit_phase2(const XrlError& e, const bool* locked,
			       const uint32_t* /* lock_holder */,
			       CallBack cb, XorpShell* xorpsh)
{
    if (!locked || (e != XrlError::OKAY())) {
	cb->dispatch(false, "Failed to get lock");
	return;
    }

    // We managed to get the master lock
    SlaveConfigTree delta_tree(_xclient, _verbose);
    delta_tree.get_deltas(*this);
    string deltas = delta_tree.show_unannotated_tree();

    SlaveConfigTree withdraw_tree(_xclient, _verbose);
    withdraw_tree.get_deletions(*this);
    string deletions = withdraw_tree.show_unannotated_tree();

    XLOG_TRACE(_verbose, "deletions = >>>\n%s<<<\n", deletions.c_str());

    _root_node.initialize_commit();
    xorpsh->commit_changes(deltas, deletions,
			   callback(this, &SlaveConfigTree::commit_phase3, cb,
				    xorpsh),
			   cb);
}

void
SlaveConfigTree::commit_phase3(const XrlError& e, CallBack cb,
			       XorpShell* xorpsh)
{
    XLOG_TRACE(_verbose, "commit_phase3\n");

    //
    // We get here when the rtrmgr has received our request, but before
    // it has actually tried to do the commit.  If we got an error, we
    // call unlock_config immediately.  Otherwise we don't unlock it
    // until we get called back with the final results of the commit.
    //
    if (e != XrlError::OKAY()) {
	_commit_errmsg = e.note();
	xorpsh->unlock_config(callback(this, &SlaveConfigTree::commit_phase5,
				       false, cb, xorpsh));
    }
    xorpsh->set_mode(XorpShell::MODE_COMMITTING);
}

void
SlaveConfigTree::commit_phase4(bool success, const string& errmsg, CallBack cb,
			       XorpShell *xorpsh)
{
    XLOG_TRACE(_verbose, "commit_phase4\n");

    //
    // We get here when we're called back by the rtrmgr with the
    // results of our commit.
    //
    _commit_errmsg = errmsg;
    xorpsh->unlock_config(callback(this, &SlaveConfigTree::commit_phase5,
				   success, cb, xorpsh));
}

void
SlaveConfigTree::commit_phase5(const XrlError& /* e */,
			       bool success,
			       CallBack cb,
			       XorpShell* /* xorpsh */)
{
    XLOG_TRACE(_verbose, "commit_phase5\n");

    if (success) {
	_root_node.finalize_commit();
	cb->dispatch(true, "");
    } else {
	cb->dispatch(false, _commit_errmsg);
    }
}

bool
SlaveConfigTree::get_deltas(const SlaveConfigTree& main_tree)
{
    XLOG_TRACE(_verbose, "SlaveConfigTree::get_deltas\n");

    if (root_node().get_deltas(main_tree.const_root_node()) > 0) {
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

    if (root_node().get_deletions(main_tree.const_root_node()) > 0) {
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
