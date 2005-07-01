// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/policy/term.cc,v 1.6 2005/04/15 03:46:03 atanu Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "policy_module.h"
#include "config.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "term.hh"
#include "policy/common/policy_utils.hh"

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
	clear_map(*_block_nodes[i]);
	delete _block_nodes[i];
    }
}

void
Term::set_block(const uint32_t& block, const uint32_t& order,
		const string& variable, const string& op, const string& arg)
{
    if (block >= LAST_BLOCK) {
	throw term_syntax_error("Unknown block: " + to_str(block));
    }

    // check if we want to delete
    if (variable.empty() && op.empty() && arg.empty()) {
	del_block(block, order);
	return;
    }

    // check that position is empty
    Nodes& conf_block = *_block_nodes[block];
    if (conf_block.find(order) != conf_block.end()) {
	throw term_syntax_error("A statement is already present in position: " 
				+ to_str(order));
    }

    // XXX: rebuild original statement
    // FIXME: no need to rebuild original statement + reparse... be smart...
    // rtrmgr is "helping" us... well we have less freedom on syntax though...
    // XXX: when we know what the front end syntax should be, and who does the
    // parsing, we should fix all this...
    string statement = variable + " " + op + " " + arg + ";";
    debug_msg("[POLICY] Statement= (%s)\n", statement.c_str());

    // parse and check syntax error
    Parser::Nodes* nodes = _parser.parse(statement);
    if (!nodes) {
	string err = _parser.last_error();
	// XXX convert block from int to string... [human readable]
	throw term_syntax_error("Syntax error in term " + _name + 
				" block " + to_str(block) + " " + err);
    }
    XLOG_ASSERT(nodes->size() == 1); // XXX a single statement!

    conf_block[order] = nodes->front();
}

void
Term::del_block(const uint32_t& block, const uint32_t& order)
{
    XLOG_ASSERT (block < LAST_BLOCK);

    Nodes& conf_block = *_block_nodes[block];

    Nodes::iterator i = conf_block.find(order);
    if (i == conf_block.end()) {
	throw term_syntax_error("Want to delete an empty position: " 
				+ to_str(order));
    }
    conf_block.erase(i);
}
