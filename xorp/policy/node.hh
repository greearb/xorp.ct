// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2010 XORP, Inc and Others
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

// $XORP: xorp/policy/node.hh,v 1.17 2008/10/02 21:57:58 bms Exp $

#ifndef __POLICY_NODE_HH__
#define __POLICY_NODE_HH__





#include "policy/common/operator_base.hh"
#include "policy/common/varrw.hh"

#include "node_base.hh"
#include "visitor.hh"

/**
 * @short A generic node wrapper.
 */
template<class T>
class NodeAny :
    public Node
{
public:
    /**
     * @param val the value of the node.
     * @param line the line of the configuration where the node was created.
     */
    NodeAny(const T& val, unsigned line) : Node(line),  _val(val) {}

    // semicolon for kdoc
    DEFINE_VISITABLE();

    /**
     * @return the value of the node.
     */
    const T& val() const { return _val; }

private:
    T _val;
};

/**
 * A node that holds a variable [such as aspath].
 */
typedef NodeAny<string>	NodeVar;

/**
 * @short A node which holds an element.
 *
 * Such as an IP address.
 */
class NodeElem :
    public NONCOPYABLE,
    public Node
{
public:
    /**
     * The node owns the element. Caller must not modify / delete.
     *
     * @param elem the element to hold.
     * @param line line of configuration where node was created.
     */
    NodeElem(Element* elem, unsigned line) : Node(line), _elem(elem) {}
    ~NodeElem() { delete _elem; } 

    DEFINE_VISITABLE();

    /**
     * @return the element in this node.
     */
    const Element& val() const { return *_elem; }

private:
    Element* _elem;
};

/**
 * @short A node which holds a set.
 *
 * The set name is only stored, as the SetMap is used for dereferencing.
 */
class NodeSet : public Node {
public:
    /**
     * @param c_str the name of the set.
     * @param line line of configuration where node was created.
     */
    NodeSet(const char* c_str, unsigned line) : Node(line), _setid(c_str) {}

    DEFINE_VISITABLE();

    /**
     * @return the name of the set.
     */
    const string& setid() const { return _setid; }

private:
    string _setid;
};

/**
 * @short A node for a binary operation.
 *
 * The node will thus have two children. It owns both of them.
 *
 */
class NodeBin :
    public NONCOPYABLE,
    public Node
{
public:
    /**
     * Caller must not delete / modify pointers.
     *
     * @param op binary operation of node.
     * @param left first argument of operation.
     * @param right second argument of operation.
     * @param line line where node was created.
     */
    NodeBin(BinOper* op, Node* left, Node* right, unsigned line) : Node(line),
	    _op(op), _left(left), _right(right) {}
    ~NodeBin() {
	delete _op;
	delete _left; 
	delete _right; 
    }

    DEFINE_VISITABLE();

    /**
     * @return operation associated with node.
     */
    const BinOper& op() const { return *_op; }

    /**
     * @return first argument of operation.
     */
    Node& left() const { return *_left; }

    /**
     * @return second argument of operation.
     */
    Node& right() const { return *_right; }

private:
    BinOper* _op;
    Node *_left;
    Node *_right;
};

/**
 * @short Unary operation.
 *
 * The node will have one child. It owns it.
 */
class NodeUn :
    public NONCOPYABLE,
    public Node
{
public:
    /**
     * Caller must not delete / modify pointers.
     *
     * @param op unary operation associated with node.
     * @param node child of node -- argument of operation.
     * @param line line of configuration where node was created.
     */
    NodeUn(UnOper* op, Node* node, unsigned line) : Node(line), _op(op),  _node(node) {}
    ~NodeUn() {
	delete _op;
	delete _node;
    }

    DEFINE_VISITABLE();

    /**
     * @return unary operation associated with node.
     */
    const UnOper& op() const { return *_op; }

    /**
     * @return argument of unary operation.
     */
    Node& node() const { return *_node; }

private:
    UnOper* _op;
    Node* _node;
};


/**
 * @short An assignment operation.
 */
class NodeAssign :
    public NONCOPYABLE,
    public Node
{
public:
    /**
     * Caller must not delete / modify pointer.
     *
     * @param varid the name of the variable being assigned to.
     * @param mod the modifier (e.g., += has a modifier OpAdd).
     * @param rvalue the expression being assigned to the variable.
     * @param line line of configuration where node was created.
     */
    NodeAssign(const string& varid, BinOper* mod, Node* rvalue, unsigned line)
	: Node(line), _varid(varid), _mod(mod), _rvalue(rvalue) {}
    ~NodeAssign() { delete _rvalue; delete _mod; }

    DEFINE_VISITABLE();

    /**
     * @return name of variable being assigned to.
     */
    const string& varid() const { return _varid; }

    /**
     * @return argument of assignment.
     */
    Node& rvalue() const { return *_rvalue; }

    BinOper* mod() const { return _mod; }

private:
    string	_varid;
    BinOper*	_mod;
    Node*	_rvalue;
};

/**
 * @short Node representing an accept statement.
 */
class NodeAccept : public Node {
public:
    /**
     * @param line line of configuration where node was created.
     */
    NodeAccept(unsigned line) : Node(line) {}

    DEFINE_VISITABLE();

    /**
     * Test whether this is "accept" or "reject" statement.
     *
     * @return true if this is "accept" or "reject" statement.
     */
    virtual bool is_accept_or_reject() const { return (true); }
};

/**
 * @short Node representing a reject statement.
 */
class NodeReject : public Node {
public:
    /**
     * @param line line of configuration where node was created.
     */
    NodeReject(unsigned line) : Node(line) {}

    DEFINE_VISITABLE();

    /**
     * Test whether this is "accept" or "reject" statement.
     *
     * @return true if this is "accept" or "reject" statement.
     */
    virtual bool is_accept_or_reject() const { return (true); }
};

/**
 * @short Node representing a protocol statement.
 */
class NodeProto : public Node {
public:
    /**
     * @param protocol the protocol of the statement.
     * @param line line of configuration where node was created.
     */
    NodeProto(const string& proto, unsigned line) : Node(line), _proto(proto) {}

    DEFINE_VISITABLE();

    /**
     * Test whether this is a "protocol" statement.
     *
     * @return true if this is a "protocol" statement.
     */
    virtual bool is_protocol_statement() const { return (true); }

    /**
     * @return the protocol being referenced.
     */
    const string& proto() const { return _proto; }

private:
    string _proto;
};

class NodeNext : public Node {
public:
    enum Flow {
	POLICY = 0,
	TERM
    };

    NodeNext(unsigned line, Flow f) : Node(line), _flow(f) {}

    DEFINE_VISITABLE();

    Flow flow() const { return _flow; }

private:
    Flow    _flow;
};

class NodeSubr : public Node {
public:
    NodeSubr(unsigned line, string policy) : Node(line), _policy(policy) {}

    DEFINE_VISITABLE();

    string policy() const { return _policy; }

private:
    string  _policy;
};

#endif // __POLICY_NODE_HH__
