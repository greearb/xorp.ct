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

// $XORP: xorp/policy/term.hh,v 1.18 2008/10/02 21:58:01 bms Exp $

#ifndef __POLICY_TERM_HH__
#define __POLICY_TERM_HH__

#include "libproto/config_node_id.hh"
#include "policy/common/policy_exception.hh"

#include <map>
#include <string>
#include "node_base.hh"


/**
 * @short A term is an atomic policy unit. 
 *
 * It is a complete specification of how a route needs to be matched, and what
 * actions must be taken.
 */
class Term {
public:
    enum BLOCKS {
	SOURCE = 0,
	DEST,
	ACTION,

	// keep this last
	LAST_BLOCK
    };

    // the integer is the "line number", the node is the parsed structure [AST]
    // of the statement(s) in that line.
    typedef ConfigNodeIdMap<Node*> Nodes;

    /**
     * @short Exception thrown on a syntax error while parsing configuration.
     */
    class term_syntax_error :  public PolicyException {
    public:
        term_syntax_error(const char* file, size_t line, 
			  const string& init_why = "")   
            : PolicyException("term_syntax_error", file, line, init_why) {}  
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
     * Perform operations at the end of the term.
     */
    void set_term_end();

    /**
     * Updates the source/dest/action block of a term.
     *
     * @param block the block to update (0:source, 1:dest, 2:action).
     * @param order node ID with position of term.
     * @param statement the statement to insert.
     */
    void set_block(const uint32_t& block, const ConfigNodeId& order,
		   const string& statement);

    /**
     * Deletes statements in the location specified by order and block.
     *
     * @param block the block to update (0:source, 1:dest, 2:action).
     * @param order node ID with position of term.
     */
    void del_block(const uint32_t& block, const ConfigNodeId& order);

    /**
     * Perform operations at the end of the block.
     *
     * @param block the block to perform operations on
     * (0:source, 1:dest, 2:action).
     */
    void set_block_end(uint32_t block);

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

    /**
     * Convert block number to human readable form.
     *
     * @param num the block number.
     * @return human readable representation of block name.
     */
    static string block2str(uint32_t num); 

    /**
     * Get the protocol name (in the "from" block).
     *
     * @return the protocol name (in the "from" block) if set, otherwise
     * an empty string.
     */
    const string& from_protocol() const { return (_from_protocol); }

    /**
     * Set the protocol name (in the "from" block).
     *
     * @param v the protocol name (in the "from" block).
     */
    void set_from_protocol(const string& v) { _from_protocol = v; }

private:
    list<pair<ConfigNodeId, Node*> >::iterator find_out_of_order_node(
	const uint32_t& block, const ConfigNodeId& order);

    string _name;

    Nodes* _block_nodes[3];
    list<pair<ConfigNodeId, Node*> > _out_of_order_nodes[3];

    Nodes*& _source_nodes; 
    Nodes*& _dest_nodes;
    Nodes*& _action_nodes;

    string  _from_protocol;	// The protocol (in the "from" block)

    // not impl
    Term(const Term&);
    Term& operator=(const Term&);
};

#endif // __POLICY_TERM_HH__
