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

#ifndef __POLICY_NODE_HH__
#define __POLICY_NODE_HH__

#include "policy/common/operator_base.hh"
#include "node_base.hh"
#include "visitor.hh"
#include <string>

using std::string;

/**
 * @short A generic node wrapper.
 */
template<class T>
class NodeAny : public Node {
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
typedef NodeAny<string>		NodeVar;

/**
 * @short A node which holds an element.
 *
 * Such as an IP address.
 */
class NodeElem : public Node {
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

    // not impl
    NodeElem(const NodeElem&);
    NodeElem& operator=(const NodeElem&);
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
class NodeBin : public Node {
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
   
    // not impl
    NodeBin(const NodeBin&);
    NodeBin& operator=(const NodeBin&);
};

/**
 * @short Unary operation.
 *
 * The node will have one child. It owns it.
 */
class NodeUn : public Node {
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

    // not impl
    NodeUn(const NodeUn&);
    NodeUn& operator=(const NodeUn&);
};


/**
 * @short An assignment operation.
 */
class NodeAssign : public Node {
public:
    /**
     * Caller must not delete / modify pointer.
     *
     * @param varid the name of the variable being assigned to.
     * @param rvalue the expression being assigned to the variable.
     * @param line line of configuration where node was created.
     */
    NodeAssign(const string& varid, Node* rvalue, unsigned line) : 
		Node(line), _varid(varid), _rvalue(rvalue) {}
    ~NodeAssign() { delete _rvalue; }

    DEFINE_VISITABLE();

    /**
     * @return name of variable being assigned to.
     */
    const string& varid() const { return _varid; }

    /**
     * @return argument of assignment.
     */
    Node& rvalue() const { return *_rvalue; }

private:
    string _varid;
    Node* _rvalue;

    // not impl
    NodeAssign(const NodeAssign&);
    NodeAssign& operator=(const NodeAssign&);
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
     * @return the protocol being referenced.
     */
    const string& proto() const { return _proto; }

private:
    string _proto;
};


/**
 * @short Node representing a regular expression.
 */
class NodeRegex : public Node {
public:
    /**
     * Caller must not delete / modify pointer.
     *
     * @param arg the argument of the regular expression.
     * @param reg the regular expression used for matching.
     * @param line line of configuration where node was created.
     */
    NodeRegex(Node* arg, const string& reg, unsigned line) : Node(line),
							     _arg(arg), 
							     _reg(reg) {}
    ~NodeRegex() { delete _arg; }

    DEFINE_VISITABLE();

    /**
     * @return the argument of the regular expression.
     */
    Node& arg() const { return *_arg; }

    /**
     * @return the regular expression.
     */
    const string& reg() const { return _reg; }

private:
    Node* _arg;
    string _reg;

    // not impl
    NodeRegex(const NodeRegex&);
    NodeRegex operator=(const NodeRegex&);
};

#endif // __POLICY_NODE_HH__
