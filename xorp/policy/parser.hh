// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/policy/parser.hh,v 1.8 2008/10/02 21:57:59 bms Exp $

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
