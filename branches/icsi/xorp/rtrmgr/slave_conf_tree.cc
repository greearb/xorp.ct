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

#ident "$XORP: xorp/rtrmgr/slave_conf_tree.cc,v 1.18 2002/12/09 18:29:38 hodson Exp $"

#include "rtrmgr_module.h"
#include "template_tree_node.hh"
#include "template_commands.hh"
#include "template_tree.hh"
#include "slave_conf_tree.hh"
#include "slave_conf_tree_node.hh"
#include "cli.hh"
#include "split.hh"

extern int booterror(const char *s);

/*************************************************************************
 * Slave Config Tree class 
 *************************************************************************/

SlaveConfigTree::SlaveConfigTree()
    : ConfigTree(NULL) {
    _xclient = NULL;
}

SlaveConfigTree::SlaveConfigTree(const string& configuration, 
				 TemplateTree *tt, 
				 XorpClient *xclient) 
    : ConfigTree(tt)
{
    parse(configuration, "");
    _root_node.mark_subtree_as_committed();
    _xclient = xclient;
}

int SlaveConfigTree::parse(const string& configuration, 
			   const string& conffile) {
    try {
	((ConfigTree*)this)->parse(configuration, conffile);
    } catch (ParseError &pe) {
	booterror(pe.err.c_str());
	exit(1);
    }
    return 0;
}

bool
SlaveConfigTree::commit_changes(string &result,
				XorpShell& xorpsh,
				XorpBatch::CommitCallback cb) {
#ifdef DEBUG_COMMIT
    printf("##############################################################\n");
    printf("SlaveConfigTree::commit_changes\n");
#endif

    bool success = true;

#ifdef NOTDEF
    //XXX we really should do this check, but we need to deal with
    //unexpanded variables more gracefully.

    /* Two passes: the first checks for errors.  If no errors are
       found, attempt the actual commit */
    uint tid = 0;
    tid = _xclient->begin_transaction();
    _root_node.initialize_commit();
    if (_root_node.commit_changes(NULL, "", _xclient, tid, 
				  /*no_execute*/true, 
				  /*no_commit*/true, 
				  0, 0, result) == false) {
	//something went wrong - return the error message.
	return false;
    }
    XorpBatch::CommitCallback empty_cb;
    try {
	_xclient->end_transaction(tid, empty_cb);
    } catch (UnexpandedVariable& uvar) {
	printf("%s\n", uvar.str().c_str());
    }
#else
    UNUSED(result);
#endif    
    _stage2_cb = callback(this, 
			  &SlaveConfigTree::commit_phase2,
			  cb, 
			  &xorpsh);
    xorpsh.lock_config(_stage2_cb);
    return success;
}

void SlaveConfigTree::commit_phase2(const XrlError& e, 
				    const bool* locked, 
				    const uint32_t* /*lock_holder*/,
				    XorpBatch::CommitCallback cb,
				    XorpShell* xorpsh) {
    if (!locked || (e != XrlError::OKAY())) {
	cb->dispatch(XORP_ERROR, "Failed to get lock");
	return;
    }
    //we managed to get the master lock
    SlaveConfigTree delta_tree;
    delta_tree.get_deltas(*this);
    string deltas = delta_tree.show_unannotated_tree();

    SlaveConfigTree withdraw_tree;
    withdraw_tree.get_deletions(*this);
    string deletions = withdraw_tree.show_unannotated_tree();
#ifdef DEBUG_COMMIT
    printf("deletions = >>>\n%s<<<\n", deletions.c_str());
#endif
    _root_node.initialize_commit();
    xorpsh->commit_changes(deltas, deletions, 
			   callback(this, &SlaveConfigTree::commit_phase3, cb,
				    xorpsh),
			   cb);
}

void SlaveConfigTree::commit_phase3(const XrlError& e, 
				    XorpBatch::CommitCallback cb,
				    XorpShell* xorpsh) {
#ifdef DEBUG_COMMIT
    printf("commit_phase3\n");
#endif
    //We get here when the rtrmgr has received our request, but before
    //it has actually tried to do the commit.  If we got an error, we
    //call unlock_config immediately.  Otherwise we don't unlock it
    //until we get called back with the final results of the commit.
    if (e != XrlError::OKAY()) {
	_commit_errmsg = e.note();
	xorpsh->unlock_config(callback(this, &SlaveConfigTree::commit_phase5, 
				       false, cb, xorpsh));
    }
    xorpsh->set_mode(XorpShell::MODE_COMMITTING);
}

void SlaveConfigTree::commit_phase4(bool success, const string& errmsg,
				    XorpBatch::CommitCallback cb,
				    XorpShell *xorpsh) {
    //We get here when we're called back by the rtrmgr with the
    //results of our commit.
#ifdef DEBUG_COMMIT
    printf("commit_phase4\n");
#endif
    _commit_errmsg = errmsg;
    xorpsh->unlock_config(callback(this, &SlaveConfigTree::commit_phase5, 
				   success, cb, xorpsh));
}

void SlaveConfigTree::commit_phase5(const XrlError& /*e*/, 
				    bool success,
				    XorpBatch::CommitCallback cb,
				    XorpShell* /*xorpsh*/) {
#ifdef DEBUG_COMMIT
    printf("commit_phase5\n");
#endif
    if (success) {
	_root_node.finalize_commit();
	cb->dispatch(XORP_OK, "");
    } else
	cb->dispatch(XORP_ERROR, _commit_errmsg);
}

bool SlaveConfigTree::get_deltas(const SlaveConfigTree& main_tree) {
#ifdef DEBUG_COMMIT
    printf("SlaveConfigTree::get_deltas\n");
#endif
    if (root()->get_deltas(*(main_tree.const_root())) > 0) {
#ifdef DEBUG_COMMIT
	printf("FOUND DELTAS:\n");
	print();
#endif
	return true;
    } else
	return false;
}

bool SlaveConfigTree::get_deletions(const SlaveConfigTree& main_tree) {
#ifdef DEBUG_COMMIT
    printf("SlaveConfigTree::get_deltas\n");
#endif
    if (root()->get_deletions(*(main_tree.const_root())) > 0) {
#ifdef DEBUG_COMMIT
	printf("FOUND DELETIONS:>>>>\n");
	print();
	printf("<<<<\n");
#endif
	return true;
    } else
	return false;
}

string
SlaveConfigTree::discard_changes() {
#ifdef DEBUG_COMMIT
    printf("##############################################################\n");
    printf("SlaveConfigTree::discard_changes\n");
#endif
    string result =
	_root_node.discard_changes(NULL, _xclient, 
				   /*no_execute*/true, 0, 0);
#ifdef DEBUG_COMMIT
    printf("##############################################################\n");
#endif
    return result;
}

string
SlaveConfigTree::mark_subtree_for_deletion(const list <string>& pathsegs,
				    uid_t user_id) {
    SlaveConfigTreeNode *found = find_node(pathsegs);
    if (found == NULL)
	return "ERROR";

    if (found->parent() != NULL
	&& found->parent()->is_tag()
	&& found->parent()->children().size()==1) {
	found = found->parent();
    }
    
    found->mark_subtree_for_deletion(user_id);
    return string("OK");
}
