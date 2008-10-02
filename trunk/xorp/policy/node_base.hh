// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/policy/node_base.hh,v 1.9 2008/10/01 23:44:24 pavlin Exp $

#ifndef __POLICY_NODE_BASE_HH__
#define __POLICY_NODE_BASE_HH__

#include "visitor.hh"

/**
 * @short The base class of a node for building a hierarchy.
 *
 * Each node has a line number to associate it with a line in the configuration
 * file. This is useful for reporting error location in semantic checks.
 *
 * Nodes implement the visitor pattern [Inspired by Alexandrescu].
 */
class Node {
public:
    /**
     * @param line the configuration line number where this node was created.
     */
    Node(unsigned line) : _line(line) {}
    virtual ~Node() {}

    /**
     * @return line number of configuration where node was created.
     */
    unsigned line() const { return _line; }

    /**
     * Implementation of visitor.
     *
     * @param v visit node with this pattern.
     * @return element at the end of node evaluation.
     */
    virtual const Element* accept(Visitor& v) =0;

    /**
     * Test whether this is a "protocol" statement.
     *
     * @return true if this is a "protocol" statement.
     */
    virtual bool is_protocol_statement() const { return (false); }

    /**
     * Test whether this is "accept" or "reject" statement.
     *
     * @return true if this is "accept" or "reject" statement.
     */
    virtual bool is_accept_or_reject() const { return (false); }

private:
    unsigned _line;
};

// macro ugliness for visitor implemntation.
#define DEFINE_VISITABLE() \
const Element* accept(Visitor& visitor) { \
    return visitor.visit(*this); \
}

#endif // __POLICY_NODE_BASE_HH__
