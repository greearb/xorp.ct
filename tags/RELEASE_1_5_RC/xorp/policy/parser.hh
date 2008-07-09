// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
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

// $XORP: xorp/policy/parser.hh,v 1.5 2007/02/16 22:46:54 pavlin Exp $

#ifndef __POLICY_PARSER_HH__
#define __POLICY_PARSER_HH__

#include <string>
#include <vector>
#include "node_base.hh"
#include "term.hh"

/**
 * @short A lex/yacc wrapper which parses a configuration and returns nodes.
 *
 * This class parses a raw user configuration and returns a vector of nodes.
 *
 * Each node will normally relate to a single statement. The vector of nodes
 * reflects all the statements present.
 */
class Parser {
public:
    typedef vector<Node*> Nodes;

    /**
     * @param block the term block which is being parsed [action/src/dest].
     * @param text Configuration to parse.
     * @return the parse-tree of the configuration. Null on error.
     */
    Nodes* parse(const Term::BLOCKS& block, const string& text);

    /**
     * This should be called if parse returns null.
     *
     * If parse is successful, the value of last_error is undefined.
     * 
     * @return the last error of the parse.
     */
    string last_error();

private:
    string _last_error;
};

#endif // __POLICY_PARSER_HH__
