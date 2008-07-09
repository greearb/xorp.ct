// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/policy/term.cc,v 1.22 2007/02/16 22:46:56 pavlin Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "policy_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "policy/common/policy_utils.hh"

#include "term.hh"
#include "parser.hh"


using namespace policy_utils;

Term::Term(const string& name) : _name(name), 
				 _source_nodes(_block_nodes[SOURCE]),
				 _dest_nodes(_block_nodes[DEST]),
				 _action_nodes(_block_nodes[ACTION])
{
    for(unsigned int i = 0; i < LAST_BLOCK; i++)
	_block_nodes[i] = new Nodes;
}

Term::~Term()
{
    for(unsigned int i = 0; i < LAST_BLOCK; i++) {
	clear_map_container(*_block_nodes[i]);
	delete _block_nodes[i];

	list<pair<ConfigNodeId, Node*> >::iterator iter;
	for (iter = _out_of_order_nodes[i].begin();
	     iter != _out_of_order_nodes[i].end();
	     ++iter) {
	    delete iter->second;
	}
    }
}

void
Term::set_term_end()
{
    uint32_t i;

    for (i = 0; i < LAST_BLOCK; i++) {
	set_block_end(i);
    }
}

void
Term::set_block(const uint32_t& block, const ConfigNodeId& order,
		const string& statement)
{
    if (block >= LAST_BLOCK) {
	xorp_throw(term_syntax_error, "Unknown block: " + to_str(block));
    }

    // check if we want to delete
    if (statement.empty()) {
	del_block(block, order);
	return;
    }

    // check that position is empty... 
    // if it's not delete and then add...  This usually occurs when "replacing"
    // an existing statement. like:
    // localpref: 101
    // setting localpref: 102 in same node will cause this...
    Nodes& conf_block = *_block_nodes[block];
    if ((conf_block.find(order) != conf_block.end())
	|| (find_out_of_order_node(block, order)
	    != _out_of_order_nodes[block].end())) {
	debug_msg("[POLICY] Deleting previous statement...\n");
	del_block(block, order);
#if 0
	//
	// XXX: don't throw an error if a previous statement is in this
	// position. Given that currently, say, same "nexthop4"
	// rtrmgr policy template node exists in more than one template
	// (bgp.tp, ospfv2.tp, rip.tp), we could receive three times same
	// XRL to set same block. The reason for this is because each
	// *.tp "nexthop4" entry incremantally adds same "%set xrl" action.
	// The alternative solution would be to avoid node/action duplication
	// in the rtrmgr templates.
	//
	xorp_throw(term_syntax_error,
		   "A statement is already present in position: " + to_str(order));
#endif // 0
    }

    debug_msg("[POLICY] Statement=(%s)\n", statement.c_str());

    // parse and check syntax error
    Parser parser;
    // cast should be safe... because of check earlier in method.
    Parser::Nodes* nodes = parser.parse(static_cast<BLOCKS>(block), statement);
    if (!nodes) {
	string err = parser.last_error();
	// XXX convert block from int to string... [human readable]
	xorp_throw(term_syntax_error, "Syntax error in term " + _name + 
				" block " + block2str(block) + " statement=("
				+ statement + "): " + err);
    }
    XLOG_ASSERT(nodes->size() == 1); // XXX a single statement!

    pair<Nodes::iterator, bool> res;
    res = conf_block.insert(order, nodes->front());
    if (res.second != true) {
	//
	// Failed to add the entry, probably because it was received out of
	// order. Add it to the list of entries that need to be added later.
	//
	_out_of_order_nodes[block].push_back(make_pair(order, nodes->front()));
	return;
    }

    //
    // Try to add any entries that are out of order.
    // Note that we need to keep trying traversing the list until
    // no entry is added.
    //
    while (true) {
	bool entry_added = false;
	list<pair<ConfigNodeId, Node*> >::iterator iter;
	for (iter = _out_of_order_nodes[block].begin();
	     iter != _out_of_order_nodes[block].end();
	     ++iter) {
	    res = conf_block.insert(iter->first, iter->second);
	    if (res.second == true) {
		// Entry added successfully
		entry_added = true;
		_out_of_order_nodes[block].erase(iter);
		break;
	    }
	}
	
	if (! entry_added)
	    break;
    }
}

void
Term::del_block(const uint32_t& block, const ConfigNodeId& order)
{
    XLOG_ASSERT (block < LAST_BLOCK);

    Nodes& conf_block = *_block_nodes[block];

    Nodes::iterator i = conf_block.find(order);
    if (i != conf_block.end()) {
	conf_block.erase(i);
	return;
    }

    // Try to delete from the list of out-of-order nodes
    list<pair<ConfigNodeId, Node*> >::iterator iter;
    iter = find_out_of_order_node(block, order);
    if (iter != _out_of_order_nodes[block].end()) {
	_out_of_order_nodes[block].erase(iter);
	return;
    }

#if 0
    //
    // XXX: don't throw an error if no previous statement is in this
    // position. Given that currently, say, same "nexthop4"
    // rtrmgr policy template node exists in more than one template
    // (bgp.tp, ospfv2.tp, rip.tp), we could receive three times same
    // XRL to delete same block. The reason for this is because each
    // *.tp "nexthop4" entry incremantally adds same "%delete xrl" action.
    // The alternative solution would be to avoid node/action duplication
    // in the rtrmgr templates.
    //
    xorp_throw(term_syntax_error,
	       "Want to delete an empty position: " + order.str());
#endif // 0
}

void
Term::set_block_end(uint32_t block)
{
    if (block >= LAST_BLOCK) {
	xorp_throw(term_syntax_error, "Unknown block: " + to_str(block));
    }

    Nodes& conf_block = *_block_nodes[block];

    //
    // Add all remaining entries that are out of order
    //
    while (! _out_of_order_nodes[block].empty()) {
	list<pair<ConfigNodeId, Node*> >::iterator iter;
	pair<Nodes::iterator, bool> res;

	//
	// Try to add as much as possible out-of-order entries
	//
	while (true) {
	    bool entry_added = false;
	    for (iter = _out_of_order_nodes[block].begin();
		 iter != _out_of_order_nodes[block].end();
		 ++iter) {
		res = conf_block.insert(iter->first, iter->second);
		if (res.second == true) {
		    // Entry added successfully
		    entry_added = true;
		    _out_of_order_nodes[block].erase(iter);
		    break;
		}
	    }
	    if (! entry_added)
		break;
	}

	//
	// If there are any remaining entries, then add the first at the
	// end of the block. After that try to add the remaining entries
	// in the proper order.
	//
	// XXX: Note that we need this mechanism in case there were some
	// holes in the order of received entries. E.g., if the rtrmgr
	// template file contained leaf nodes without XRLs, those nodes
	// will have a node ID that may place them in the middle of the
	// ordered set of leaf nodes. Such nodes won't be received, hence
	// we need this mechanism to add the nodes after the hole.
	// However, this mechanism may still add some of the nodes in
	// the wrong order. For example, lets say the original configuration
	// contained the following nodes, and that (H) was a leaf node
	// without an XRL:
	//    A B C (H) E F G
	// We will correctly order the nodes as "A B C E F G".
	// However, lets say a new node D is added to the configuration:
	//   A B C (H) D E F G
	// When node D is received, it will look like an out-of-order node,
	// and this algorithm will add it to the end. I.e., the result
	// will be "A B C E F G D".
	// Fortunately, this appears to be an issue only for leaf nodes
	// (to be more specific, only for nodes that are not multi-value
	// nodes), which, arguably, should not need node IDs.
	// At least, in case of the policy manager, the order of the
	// statements inside a policy block shouldn't matter.
	//
	if (! _out_of_order_nodes[block].empty()) {
	    iter = _out_of_order_nodes[block].begin();
	    // XXX: we ignore the return result, because we should fail
	    // to insert only if the node was added already.
	    conf_block.insert_out_of_order(iter->first, iter->second);
	    _out_of_order_nodes[block].erase(iter);
	    break;
	}
    }
}

list<pair<ConfigNodeId, Node*> >::iterator
Term::find_out_of_order_node(const uint32_t& block, const ConfigNodeId& order)
{
    list<pair<ConfigNodeId, Node*> >::iterator iter;

    XLOG_ASSERT (block < LAST_BLOCK);

    for (iter = _out_of_order_nodes[block].begin();
	 iter != _out_of_order_nodes[block].end();
	 ++iter) {
	const ConfigNodeId& list_order = iter->first;
	if (list_order.unique_node_id() == order.unique_node_id())
	    return (iter);
    }

    return (_out_of_order_nodes[block].end());
}

string
Term::block2str(uint32_t block)
{
    switch (block) {
	case SOURCE:
	    return "source";

	case DEST:
	    return "dest";
	    
	case ACTION:
	    return "action";
    
	default:
	    return "UNKNOWN";
    }
}
