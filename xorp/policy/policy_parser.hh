// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// $XORP: xorp/policy/policy_parser.hh,v 1.8 2008/10/02 21:57:59 bms Exp $

#ifndef __POLICY_POLICY_PARSER_HH__
#define __POLICY_POLICY_PARSER_HH__

#include "node.hh"
#include "term.hh"



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
 * @param block the policy block [source, action, dest] which is being parsed.
 * @param conf configuration to parse.
 * @param outerr on error, this buffer will be filled with an error message.
 */
int policy_parse(vector<Node*>& outnodes, const Term::BLOCKS& block,
	         const string& conf, string& outerr);

// THESE SHOULD NOT BE TOUCHED!
extern vector<Node*>* _parser_nodes;
extern unsigned _parser_lineno;

} // namespace

#endif // __POLICY_POLICY_PARSER_HH__
