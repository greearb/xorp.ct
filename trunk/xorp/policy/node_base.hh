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

private:
    unsigned _line;
};

// macro ugliness for visitor implemntation.
#define DEFINE_VISITABLE() \
const Element* accept(Visitor& visitor) { \
    return visitor.visit(*this); \
}

#endif // __POLICY_NODE_BASE_HH__
