// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP$

#ifndef __POLICY_POLICY_PARSER_HH__
#define __POLICY_POLICY_PARSER_HH__


#include "node.hh"

#include <string>
#include <vector>


/**
 * @short Minimises global namespace pollution of yacc/lex variables.
 *
 * The nature of lex and yacc causes global variables / functions to be present.
 * Here such methods and functions are grouped under one namespace.
 */
namespace policy_parser {

/**
 * Parser a policy.
 *
 * Caller is responsible for deleting nodes created from a partial parse due to
 * errors.
 *
 * @return 0 on success.
 * @param outnodes where parse tree will be stored.
 * @param conf configuration to parse
 * @param outerr on error, this buffer will be filled with an error message.
 */
int policy_parse(vector<Node*>& outnodes, const string& conf, string& outerr);


// THESE SHOULD NOT BE TOUCHED!
extern vector<Node*>* _parser_nodes;
extern unsigned _parser_lineno;

} // namespace

#endif // __POLICY_POLICY_PARSER_HH__
