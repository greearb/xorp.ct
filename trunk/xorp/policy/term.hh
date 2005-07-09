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

// $XORP: xorp/policy/term.hh,v 1.6 2005/07/01 22:54:34 abittau Exp $

#ifndef __POLICY_TERM_HH__
#define __POLICY_TERM_HH__

#include "policy/common/policy_exception.hh"

#include "parser.hh"

#include <map>
#include <string>

/**
 * @short A term is an atomic policy unit. 
 *
 * It is a complete specification of how a route needs to be matched, and what
 * actions must be taken.
 */
class Term {
public:
    // the integer is the "line number", the node is the parsed structure [AST]
    // of the statement(s) in that line.
    typedef map<uint64_t, Node*> Nodes;

    /**
     * @short Exception thrown on a syntax error while parsing configuration.
     */
    class term_syntax_error :  public PolicyException {
    public:
	term_syntax_error(const string& r) : PolicyException(r) {}
    };

    /**
     * @param name term name.
     */
    Term(const string& name);
    ~Term();
   
    /**
     * @return name of the term.
     */
    const string& name() const { return _name; }
    
    /**
     * Updates the source/dest/action block of a term.
     *
     * @param block the block to update (0:source, 1:dest, 2:action).
     * @param order numerical position (local) of statement.
     * @param variable the attribute (such as metric) to operate on.
     * @param op specific operation to perform on variable.
     * @param arg the argument to the operator.
     */
    void set_block(const uint32_t& block, const uint64_t& order, 
		   const string& variable, const string& op, const string& arg);

    /**
     * Deletes statements in the location specified by order and block.
     *
     * @param block the block to update (0:source, 1:dest, 2:action).
     * @param order numerical position (local) of statement.
     */
    void del_block(const uint32_t& block, const uint64_t& order); 

    /**
     * Visitor implementation.
     *
     * @param v visitor used to visit this term.
     */
    const Element* accept(Visitor& v) {
	return v.visit(*this);
    }

    /**
     * @return parse tree of source block.
     */
    Nodes& source_nodes() { return *_source_nodes; }

    /**
     * @return parse tree of dest block.
     */
    Nodes& dest_nodes() { return *_dest_nodes; }

    /**
     * @return parse tree of action block.
     */
    Nodes& action_nodes() { return *_action_nodes; }

private:
    enum BLOCKS {
	SOURCE = 0,
	DEST,
	ACTION,

	// keep this last
	LAST_BLOCK
    };

    string _name;
   
    Nodes* _block_nodes[3];

    Nodes*& _source_nodes; 
    Nodes*& _dest_nodes;
    Nodes*& _action_nodes;

    Parser _parser;

    // not impl
    Term(const Term&);
    Term& operator=(const Term&);
};

#endif // __POLICY_TERM_HH__
