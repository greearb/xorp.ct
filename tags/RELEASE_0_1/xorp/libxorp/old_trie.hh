// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/libxorp/old_trie.hh,v 1.54 2002/12/09 18:29:13 hodson Exp $

#ifndef __LIBXORP_OLD_TRIE_HH__
#define __LIBXORP_OLD_TRIE_HH__

#include "ipnet.hh"

//#define VALIDATE_TRIE
//#define DEBUG_LOGGING
#include "debug.h"

//#define DEBUG_CODEPATH
#ifdef DEBUG_CODEPATH
#define cp_h(x) debug_msg("TrieCodePath: %3d\n", x);
#else
#define cp_h(x)
#endif

template <class A> class BaseTrieNode {
public:
    BaseTrieNode(const IPNet<A>& net,
		 BaseTrieNode<A> *parent);
    virtual ~BaseTrieNode() {}
    void add_child(BaseTrieNode<A> *child, int d);
    void replace_child(BaseTrieNode<A> *oldchild, BaseTrieNode<A> *newchild);
    virtual void delete_children();
    void set_parent(BaseTrieNode<A> *newparent);
    BaseTrieNode<A> *parent() const {return _parent;}
    BaseTrieNode<A> *leftchild() const {return _leftchild;}
    BaseTrieNode<A> *rightchild() const {return _rightchild;}
    void set_net(const IPNet<A> &n) {_net = n;}
    const IPNet<A> &net() const {return _net;}
    virtual bool has_route() const {return false;}
    virtual void print(int indent) const;
    void dirty_node();
    const A find_top_addr() const;
    const A find_bottom_addr() const;
    const A find_upper_bound(const A& addr) const;
    const A find_lower_bound(const A& addr) const;
    void replace_node(BaseTrieNode<A> *old_node);
    int common_parent(const IPNet<A> &net) const;
    bool valid_child(const IPNet<A> &net) const;
    bool check_if_needed();
    void validate() const;
protected:

    IPNet<A> _net;
    BaseTrieNode *_parent;
    BaseTrieNode *_leftchild, *_rightchild;
};

template <class A, class Payload> 
class TrieNode : public BaseTrieNode<A> {
public:
    TrieNode(const IPNet<A>& net, const Payload& route,
		 TrieNode<A,Payload> *parent);
    void delete_children();
    void set_route(Payload *r) {_route = r;}
    Payload& route() {return _route;}
    const Payload& const_route() const {return _route;}
    bool has_route() const {return true;}
    void print(int indent) const;
private:
    Payload _route;
};

template <class A, class Payload> class TrieIterator {
public:
    TrieIterator();
    TrieIterator(BaseTrieNode<A>* node);
    void set_root(IPNet<A> n) {_root = n;}
    const IPNet<A>& root() const {return _root;}
    void set_trienode(BaseTrieNode<A>* node) {
	_current = node;
	if (_current != 0 && _current->has_route()==false)
	    force_next();
    }
    const BaseTrieNode<A>* const_current() const {return _current;}
    BaseTrieNode<A>* current() {return _current;}
    TrieIterator operator++(int)	{ // postincrement
	TrieIterator x = *this;
	this->next();
	return x;
    }
    void force_next();
    const IPNet<A>& net() {return _current->net();}
    Payload& payload() {
	if (_current == NULL) {
	    fprintf(stderr, "Attempt to dereference Trie iterator end()\n");
	    abort();
	}
	assert(_current->has_route());
	return ((TrieNode<A,Payload>*)_current)->route();
    }
    bool operator==(const TrieIterator<A,Payload>& them) const {
	return ((_current == them.const_current()) && 
		((_root == them.root()) || (_current == NULL)));
    }

private:
    void next();
    bool _doneleft;
    bool _doneright;
    IPNet<A> _root;
    BaseTrieNode<A>* _current;
};

template <class A, class Payload> class Trie {
public:
    Trie();
    typedef TrieIterator<A,Payload> iterator;
    iterator insert(const IPNet<A>& net, const Payload& route);
    void erase(const IPNet<A>& net);
    void erase(iterator iter);
    void delete_all_nodes();
    iterator lookup_node(const IPNet<A>& net) const;

    iterator find(const A& addr) const;

    //find_less_specific asks the question: if I were to add this
    //net to the trie, what would be its parent node?  net may or
    //may not already be in the trie
    iterator find_less_specific(const IPNet<A>& net) const;

    void find_bounds(const A& a, A &lo, A &hi) const	{
	    lo = find_lower_bound(a);
	    hi = find_higher_bound(a);
    }
    iterator begin() const;
    iterator search_subtree(const IPNet<A> net) const;

    void print() const;
    const iterator end() const {return iterator(0);}
    int route_count() const {return _route_count;}
private:
    const A find_higher_bound(const A& net) const; 
    const A find_lower_bound(const A& net) const;
    BaseTrieNode<A> *find_node(const IPNet<A> &net) const;
    BaseTrieNode<A> *find_node_by_addr(const A& addr) const;
    BaseTrieNode<A> *find_less_specific_node(const IPNet<A>& net) const;
    void validate();

    BaseTrieNode<A> *_root;
    int _route_count;
};

/*****************************************************************
 * BaseTrieNode
 *****************************************************************/

template <class A> 
BaseTrieNode<A>::BaseTrieNode<A>(const IPNet<A>& subnet, 
				 BaseTrieNode<A> *parent_node) :
    _net(subnet), _parent(parent_node)
{
    _leftchild = NULL;
    _rightchild = NULL;
}

template <class A> 
void
BaseTrieNode<A>::validate() const {
    if (_leftchild != NULL) {
	assert(_leftchild->parent()==this);
	_leftchild->validate();
    }
    if (_rightchild != NULL) {
	assert(_rightchild->parent()==this);
	_rightchild->validate();
    }
}

/* valid_child returns true if the subnet "net" could be a child of
   this node*/
template <class A> 
bool
BaseTrieNode<A>::valid_child(const IPNet<A> &n) const {
    /* prerequisite is that net has a longer prefix*/
    if (n.prefix_len() <= _net.prefix_len())
	return false;
    
    if (_net.is_overlap(n)) {
	cp_h(1);
	return true;
    }
    cp_h(2);

    return false;
}

/* common_parent returns the longest prefix that could
   aggregate both this node's net and "net"*/
template <class A> 
int
BaseTrieNode<A>::common_parent(const IPNet<A> &n) const {
    int prefix_len;
    if (n.prefix_len() < _net.prefix_len()) {
	cp_h(3);
	prefix_len = n.prefix_len() - 1;
    } else {
	cp_h(4);
	prefix_len = _net.prefix_len() - 1;
    }

    while(prefix_len > 0) {
	cp_h(5);
	if (_net.masked_addr().mask_by_prefix(prefix_len) 
	    == n.masked_addr().mask_by_prefix(prefix_len)) {
	    cp_h(6);
	    return prefix_len;
	}
	cp_h(7);
	prefix_len--;
    }
    cp_h(8);
    return 0;
}

template <class A> 
void 
BaseTrieNode<A>::add_child(BaseTrieNode<A> *newchild, int d) {
    d++;
    debug_msg("XX [%d] add_child to %s\n", d, _net.str().c_str());
    debug_msg("XX [%d] newchild is %s\n", d, newchild->net().str().c_str());
    int children = 0;
    BaseTrieNode *existing = NULL;
    if (_leftchild != NULL) {
	debug_msg("XX [%d] leftchild is %s\n", d, _leftchild->net().str().c_str());
	children++;
	existing = _leftchild;
	cp_h(9);
    } else {
	debug_msg("XX [%d] leftchild is NULL\n", d);
    }
    if (_rightchild != NULL) {
	debug_msg("XX [%d] rightchild is %s\n", d, _rightchild->net().str().c_str());
	children++;
	existing = _rightchild;
	cp_h(10);
    } else {
	debug_msg("XX [%d] rightchild is NULL\n", d);
    }
    if (children == 0) {
	cp_h(11);
	debug_msg("node had no children\n");
	if (newchild->has_route() == false) {
	    cp_h(12);
	    debug_msg("removing unneeded interior node\n");
	    _leftchild = newchild->leftchild();
	    if (_leftchild != NULL) {
		cp_h(13);
		_leftchild->set_parent(this);
	    }
	    _rightchild = newchild->rightchild();
	    if (_rightchild != NULL) {
		cp_h(14);
		_rightchild->set_parent(this);
	    }
	    newchild->dirty_node();
	    delete newchild;
	} else {
	    cp_h(15);
	    _leftchild = newchild;
	    newchild->set_parent(this);
	}
	return;
    }

    //A new node might be the same as an existing child if the
    //existing child is an interior node
    if ((_leftchild != NULL) && (_leftchild->net() == newchild->net())) {
	cp_h(1900);
	assert(_leftchild->has_route() == false);
	assert(newchild->leftchild()==NULL);
	assert(newchild->rightchild()==NULL);
	assert(_leftchild->parent()==this);
	newchild->replace_node(_leftchild);
	return;
    } 
    if ((_rightchild != NULL) && (_rightchild->net() == newchild->net())) {
	cp_h(1901);
	assert(_rightchild->has_route() == false);
	assert(newchild->leftchild()==NULL);
	assert(newchild->rightchild()==NULL);
	assert(_rightchild->parent()==this);
	newchild->replace_node(_rightchild);
	return;
    } 



    if ((children == 1) && (newchild->valid_child(existing->net())==false)) {
	if (existing->net() < newchild->net()) {
	    cp_h(16);
	    debug_msg("new node goes on right\n");
	    _leftchild = existing;
	    _rightchild = newchild;
	} else {
	    cp_h(17);
	    debug_msg("new node goes on left\n");
	    _leftchild = newchild;
	    _rightchild = existing;
	}
	newchild->set_parent(this);
	cp_h(18);
	return;
    }
    debug_msg("XX [%d] node is full\n", d);

    /*we're full - to do this we need to replace one or other children*/
    if (children == 2 &&
	(newchild->valid_child(_leftchild->net())) && 
	(newchild->valid_child(_rightchild->net()))) {
	cp_h(19);
	/* both children can be a child of newchild*/
	_leftchild->set_parent(newchild);
	newchild->add_child(_leftchild, d);
	_rightchild->set_parent(newchild);
	newchild->add_child(_rightchild, d);

	_leftchild = newchild;
	_rightchild = NULL;
	newchild->set_parent(this);
	return;
    }
    if (_leftchild != NULL && newchild->valid_child(_leftchild->net())) {
	cp_h(20);
	/* the left child can be a child of newchild*/
	debug_msg("reparent left child\n");
	_leftchild->set_parent(newchild);
	newchild->add_child(_leftchild, d);
	_leftchild = newchild;
	newchild->set_parent(this);
	return;
    }
    if (_rightchild != NULL && newchild->valid_child(_rightchild->net())) {
	cp_h(21);
	/* the right child can be a child of newchild*/
	debug_msg("reparent right child\n");
	_rightchild->set_parent(newchild);
	newchild->add_child(_rightchild, d);
	_rightchild = newchild;
	newchild->set_parent(this);
	return;
    }
    debug_msg("XX 2: children: %d\n", children);
    assert(children == 2);

    /*Neither existing child can be a child of newchild.  We're going
      to have to create an intermediate child so that the trie stays
      valid*/
    int option1 = _leftchild->common_parent(_rightchild->net());
    int option2 = _leftchild->common_parent(newchild->net());
    int option3 = _rightchild->common_parent(newchild->net());

    if ((option1 > option2) &&
	(option1 > option3)) {
	cp_h(22);
	/*option 1 creates the interior node with the longest prefix*/
	IPNet<A> option1net(_leftchild->net().masked_addr(), option1);
	BaseTrieNode *interiornode = new BaseTrieNode(option1net, this);
	interiornode->add_child(_leftchild, d);
	interiornode->add_child(_rightchild, d);
	if (option1net < newchild->net()) {
	    cp_h(23);
	    _leftchild = interiornode;
	    _rightchild = newchild;
	} else {
	    cp_h(24);
	    _leftchild = newchild;
	    _rightchild = interiornode;
	}
	newchild->set_parent(this);
	return;
    }

    if ((option2 > option1) &&
	(option2 > option3)) {
	cp_h(25);
	/*option 2 creates the interior node with the longest prefix*/
	IPNet<A> option2net(_leftchild->net().masked_addr(), option2);
	BaseTrieNode *interiornode = new BaseTrieNode(option2net, this);
	debug_msg("XX [%d] interiornode: %s\n", d, option2net.str().c_str());
	interiornode->add_child(_leftchild, d);
	interiornode->add_child(newchild, d);

	if (option2net < _rightchild->net()) {
	    cp_h(26);
	    _leftchild = interiornode;
	} else {
	    //I don't think you can get here.  Mail mjh@icir.org if you can.
	    abort();
	    _leftchild = _rightchild;
	    _rightchild = interiornode;
	}
	return;
    }

    /*option 3 it is then*/
    cp_h(28);
    IPNet<A> option3net(_rightchild->net().masked_addr(), option3);
    BaseTrieNode *interiornode = new BaseTrieNode(option3net, this);
    interiornode->add_child(newchild, d);
    interiornode->add_child(_rightchild, d);
    if (option3net < _leftchild->net()) {
	//I don't think you can get here.  Mail mjh@icir.org if you can.
	abort();
	_rightchild = _leftchild;
	_leftchild = interiornode;
    } else {
	cp_h(30);
	_rightchild = interiornode;
    }
    return;
}

template <class A> 
void 
BaseTrieNode<A>::replace_node(BaseTrieNode<A> *old_node) {
    //    assert(old_node->parent() != NULL);
    assert(_leftchild == NULL);
    assert(_rightchild == NULL);
    _parent = old_node->parent();
    _leftchild = old_node->leftchild();
    _rightchild = old_node->rightchild();
    cp_h(2001);
    if (_leftchild != NULL) {
	cp_h(2002);
	_leftchild->set_parent(this);
    }
    if (_rightchild != NULL) {
	cp_h(2003);	
	_rightchild->set_parent(this);
    }
    if (old_node->parent() != NULL) {
	cp_h(2004);
	old_node->parent()->replace_child(old_node, this);
    }
    assert(_parent == old_node->parent());
    if (_leftchild != NULL) 
	assert(_leftchild->parent() == this);
    if (_rightchild != NULL) 
	assert(_rightchild->parent() == this);
    old_node->dirty_node();
    delete old_node;

}

template <class A> 
void 
BaseTrieNode<A>::replace_child(BaseTrieNode<A> *oldchild, BaseTrieNode<A> *newchild) {
    if (_leftchild == oldchild) {
	cp_h(31);
	_leftchild = newchild;
    } else if (_rightchild == oldchild) {
	cp_h(32);
	_rightchild = newchild;
    } else {
	abort();
    }
    if (newchild!=NULL) {
	cp_h(33);
	newchild->set_parent(this);
    }
    cp_h(34);
}

template <class A> 
void 
BaseTrieNode<A>::delete_children() {
    if (_leftchild != NULL)
	_leftchild->delete_children();
    delete _leftchild;
    if (_rightchild != NULL)
	_rightchild->delete_children();
    delete _rightchild;
}


template <class A> 
void BaseTrieNode<A>::set_parent(BaseTrieNode<A> *newparent) {
    cp_h(35);
    _parent = newparent;
}

template <class A> 
const A 
BaseTrieNode<A>::find_top_addr() const {
    cp_h(101);
    if (has_route()) {
	cp_h(102);
	return _net.top_addr();
    }
    if (_rightchild != NULL) {
	cp_h(103);
	return _rightchild->find_top_addr();
    }
    //can't get here
    //to get here the node has to be an interior node with 
    //a NULL right child, and interior nodes should always have two children
    abort();
#ifdef NOTDEF
    if (_leftchild != NULL) {
	return _leftchild->find_top_addr();
    }
    return A::ZERO();
#endif
}

template <class A> 
const A 
BaseTrieNode<A>::find_bottom_addr() const {
    cp_h(104);
    if (has_route()) {
	cp_h(105);
	return _net.masked_addr();
    }
    if (_leftchild != NULL) {
	cp_h(106);
	return _leftchild->find_bottom_addr();
    }
    //can't get here
    //to get here the node has to be an interior node with 
    //a NULL right child, and interior nodes should always have two children
    abort();
#ifdef NOTDEF
    if (_rightchild != NULL) {
	return _rightchild->find_bottom_addr();
    }
    return A::ZERO();
#endif
}


template <class A> 
const A 
BaseTrieNode<A>::find_lower_bound(const A& addr) const {
    A a;
    debug_msg("  flb: node: %s\n", _net.str().c_str());

    cp_h(107);
    //all routes are higher than addr.  Return no answer
    if (_net.masked_addr() > addr) {
	cp_h(108);
	return A::ALL_ONES();	
    }

    //all routes are lower than addr
    debug_msg("net.top_address = %s\n", _net.top_addr().str().c_str());
    if (_net.top_addr() < addr) {
	cp_h(109);
	a = find_top_addr();
	++a;
	return a;  
    }

    //Am I a leaf?
    if ((_leftchild == NULL) && (_rightchild == NULL)) {
	cp_h(110);
	return _net.masked_addr();  //y
    }
	
    //either one or two children
    if (_rightchild != NULL) {
	cp_h(111);
	a = _rightchild->find_lower_bound(addr);
	if (a != A::ALL_ONES()) {
	    cp_h(112);
	    return a;
	}
    }
    if (_leftchild != NULL) {
	cp_h(113);
	a = _leftchild->find_lower_bound(addr);
	if (a != A::ALL_ONES()) {
	    cp_h(114);
	    return a;
	}
    }
    
    cp_h(115);
    //neither child gave an answer
    if (has_route() == false) {
	cp_h(116);
	return A::ALL_ONES();
    }
    
    cp_h(117);
    return _net.masked_addr();    
    
}

template <class A> 
const A 
BaseTrieNode<A>::find_upper_bound(const A& addr) const {
    A a;
    debug_msg("  fub: node: %s\n", _net.str().c_str());

    //all routes are lower than addr.  Return no answer
    if (_net.top_addr() < addr) {
	cp_h(118);
	return A::ZERO();
    }

    //all routes are higher than addr
    if (_net.masked_addr() > addr) {
	cp_h(119);
	a = find_bottom_addr(); 
	--a;
	return a;
    }

    //Am I a leaf?
    if ((_leftchild == NULL) && (_rightchild == NULL)) {
	cp_h(120);
	return _net.top_addr();  //y
    }
	
    //either one or two children
    if (_leftchild != NULL) {
	cp_h(121);
	a = _leftchild->find_upper_bound(addr);
	if (a != A::ZERO()) {
	    cp_h(122);
	    return a;
	}
    }
    if (_rightchild != NULL) {
	cp_h(123);
	a = _rightchild->find_upper_bound(addr);
	if (a != A::ZERO()) {
	    cp_h(124);
	    return a;
	}
    }
    
    cp_h(125);
    //neither child gave an answer
    if (has_route() == false) {
	cp_h(126);
	return A::ZERO();
    }
    
    cp_h(127);
    return _net.top_addr();    
    
}

template <class A> 
void 
BaseTrieNode<A>::print(int indent) const {
    printf("node: %s parent: %p\n", _net.str().c_str(), _parent);
    printf(" INTERIOR\n");
    if (_leftchild == NULL) {
	printf("L: NULL\n");
    } else {
	printf("L: \n");
	_leftchild->print(indent + 2);
    }
    if (_rightchild == NULL) {
	printf("R: NULL\n");
    } else {
	printf("R: \n");
	_rightchild->print(indent + 2);
    }
    UNUSED(indent);
}


//prevent accidental reuse of a node
template <class A> 
void 
BaseTrieNode<A>::dirty_node() {
    cp_h(36);
    _parent = (BaseTrieNode<A> *)3;
    _leftchild = (BaseTrieNode<A> *)3;
    _rightchild = (BaseTrieNode<A> *)3;
}

template <class A> 
bool
BaseTrieNode<A>::check_if_needed() {
    if (has_route()) {
	cp_h(63);
	return true;
    }

    /*this is an interior node, so it may be possible to remove it*/
    if (_leftchild == NULL) {
	cp_h(64);
	if (_parent!=NULL) {
	    _parent->replace_child(this, _rightchild);
	    dirty_node();
	    delete this;
	    cp_h(65);
	}
	return false;
    }

    if (_rightchild == NULL) {
	cp_h(66);
	if (_parent!=NULL) {
	    _parent->replace_child(this, _leftchild);
	    dirty_node();
	    delete this;
	    cp_h(67);
	}
	return false;
    }
    cp_h(68);
    return true;
}

/*****************************************************************
 * TrieNode
 *****************************************************************/

template <class A, class Payload> 
TrieNode<A,Payload>::TrieNode<A,Payload>(const IPNet<A>& subnet, 
					 const Payload& payload,
					 TrieNode<A,Payload> *parent_node) :
    BaseTrieNode<A>(subnet, parent_node), _route(payload)
{
}


template <class A, class Payload> 
void 
TrieNode<A,Payload>::delete_children() {
    if (_leftchild != NULL)
	_leftchild->delete_children();
    delete _leftchild;
    if (_rightchild != NULL)
	_rightchild->delete_children();
    delete _rightchild;
}

template <class A, class Payload> 
void 
TrieNode<A,Payload>::print(int indent) const {
    printf("node: %s parent: %p\n", _net.str().c_str(), _parent);
    printf(" DATA\n");
    if (_leftchild == NULL) {
	printf("L: NULL\n");
    } else {
	printf("L: \n");
	_leftchild->print(indent+2);
    }
    if (_rightchild == NULL) {
	printf("R: NULL\n");
    } else {
	printf("R: \n");
	_rightchild->print(indent+2);
    }
    UNUSED(indent);
}

/*************************************************************************/

template <class A, class Payload> 
TrieIterator<A,Payload>::TrieIterator<A,Payload>() 
{
    _doneleft = false;
    _doneright = false;
}

template <class A, class Payload> 
TrieIterator<A,Payload>::TrieIterator<A,Payload>(BaseTrieNode<A>* node) 
    : _root()
{
    _current = node;
    _doneleft = false;
    _doneright = false;
}

template <class A, class Payload> 
void
TrieIterator<A,Payload>::next() {
    if (_current == NULL) {
	fprintf(stderr, "next() called on NULL TrieIterator\n");
	abort();
    }
    force_next();
}

template <class A, class Payload> 
void
TrieIterator<A,Payload>::force_next() {
    if (_current == NULL) {
	return;
    }
    debug_msg("next: current = %s\n", _current->net().str().c_str());

    if (_doneleft == false) {
	cp_h(1001);
	if ((_current->leftchild() != NULL) &&
	    (_current->leftchild()->net().is_overlap(_root))){
	    cp_h(1002);
	    _current = _current->leftchild();
	    _doneleft = false;
	    _doneright = false;
	    debug_msg("1. current now %s\n", _current->net().str().c_str());
	    if (_current->has_route()==false) {
		cp_h(1003);
		next();
		return;
	    }
	    if (_current->leftchild()!=NULL) {
		cp_h(1004);
		next();
		return;
	    }
	    cp_h(1005);
	    return;
	}
    }
    if (_doneright == false) {
	cp_h(1006);
	if ((_current->rightchild() != NULL) && 
	    (_current->rightchild()->net().is_overlap(_root))) {
	    cp_h(1007);
	    _current = _current->rightchild();
	    _doneleft = false;
	    _doneright = false;
	    debug_msg("2. current now %s\n", _current->net().str().c_str());
	    if (_current->has_route()==false) {
		//probably can't get here, but handle it anyway
		cp_h(1008);
		next();
		return;
	    }
	    if (_current->leftchild()!=NULL) {
		cp_h(1009);
		next();
		return;
	    }
	    cp_h(1010);
	    return;
	}
    }
    cp_h(1011);
    if (_current->parent()==NULL) {
	cp_h(1012);
	debug_msg("current now NULL\n");
	_current = NULL;
	return;
    }
    if (_current->parent()->valid_child(_root)) {
	cp_h(1013);
	//can't go to parent - it's above our root
	debug_msg("current now NULL\n");
	_current = NULL;
	return;
    }
    if (_current == _current->parent()->leftchild()) {
	cp_h(1014);
	_current = _current->parent();
	debug_msg("current now %s\n", _current->net().str().c_str());
	_doneleft = true;
	_doneright = false;
	if (_current->has_route()==false) {
	    cp_h(1015);
	    next();
	    return;
	}
	cp_h(1017);
	return;
    }
    if (_current == _current->parent()->rightchild()) {
	_doneleft = true;
	_doneright = true;
	_current = _current->parent();
	cp_h(1018);
	next();
	return;
    }
    abort();
}



/*************************************************************************/

template <class A, class Payload> 
Trie<A,Payload>::Trie<A,Payload>() {
    _root = NULL;
    _route_count = 0;
}

template <class A, class Payload> 
void
Trie<A,Payload>::validate() {
#ifdef VALIDATE_TRIE
    if (_root != NULL) {
	assert(_root->parent() == NULL);
	_root->validate();
    }
#endif
}

template <class A, class Payload> 
Trie<A,Payload>::iterator
Trie<A,Payload>::insert(const IPNet<A>& net, const Payload& payload_route) {
    debug_msg("XX insert: %s\n", net.str().c_str());
    _route_count++;
    cp_h(36);
    if (_root == NULL) {
	cp_h(37);
	_root = new TrieNode<A,Payload>(net, payload_route, NULL);
	validate();
	return iterator(_root);
    }
    if (find_node(net)!=NULL) {
	fprintf(stderr, 
		"Attempt to add node to Trie that was already there\n");
	abort();
    }
    
    BaseTrieNode<A> *here = _root, *parent = NULL;
    TrieNode<A,Payload> *newnode;
    newnode = new TrieNode<A,Payload>(net, payload_route, NULL);

    while(here != NULL) {
	cp_h(38);
	debug_msg("here: net: %s\n", here->net().str().c_str());
	if (here->net() == net) {
	    cp_h(39);
	    /* there's an interior node that we can replace*/
	    if (here->has_route())
		abort();
	    if (here == _root)
		_root = newnode;
	    newnode->replace_node(here);
	    validate();
	    return iterator(newnode);
	}
	if (newnode->valid_child(here->net())) {
	    cp_h(40);
	    /*this node should be a child of the new node*/
	    /*should only be able to get here if this node is the root node*/
	    assert(here == _root);
	    if (here->has_route() == false) {
		cp_h(41);
		/*actually we can just reuse this node because its only an 
		  interior node*/
		if (here == _root)
		    _root = newnode;
		newnode->replace_node(here);
		validate();
		return iterator(newnode);
	    }
	    cp_h(42);
	    /*create a new node and replumb*/
	    newnode->add_child(here, 0);
	    _root = newnode;
	    _root->set_parent(NULL);
	    here->set_parent(newnode);
	    validate();
	    return iterator(newnode);
	}

	cp_h(43);
	cp_h(44);
	cp_h(45);
	/*if it can't be a child of the new node, can we be a child of
          its children?*/
	if ((here->leftchild()!=NULL) && 
	    (here->leftchild()->valid_child(net))) {
	    cp_h(46);
	    parent = here;
	    here = here->leftchild();
	    continue;
	}
	if ((here->rightchild()!=NULL) && 
	    (here->rightchild()->valid_child(net))) {
	    cp_h(47);
	    parent = here;
	    here = here->rightchild();
	    continue;
	}

	/*we can't go further down the tree, so we must plumb
          ourselves in here.  add_child does the hard work*/
	if (here->valid_child(net)) {
	    cp_h(48);
	    here->add_child(newnode, 0);
	    validate();
	    return iterator(newnode);
	} else {
	    cp_h(49);
	    //here and newnode need to be peers.
	    BaseTrieNode<A> *common_parent_node;
	    int prefix_len = here->common_parent(net);
	    IPNet<A> parent_net(net.masked_addr(),prefix_len);
	    common_parent_node = 
		new BaseTrieNode<A>(parent_net, here->parent());
	    debug_msg("XX here: net: %s, parent: %x\n", 
		   here->net().str().c_str(), (u_int)(here->parent()));
	    if (here->parent() == NULL) {
		cp_h(50);
		_root = common_parent_node;
		_root->set_parent(NULL);
	    }
	    common_parent_node->add_child(here, 0);
	    common_parent_node->add_child(newnode, 0);
	    validate();
	    return iterator(newnode);
	}
    }
    abort();
}

template <class A, class Payload> 
void 
Trie<A,Payload>::delete_all_nodes() {
    if (_root != NULL)
	_root->delete_children();
    _root = NULL;
    _route_count = 0;
}

template <class A, class Payload> 
void 
Trie<A,Payload>::erase(const IPNet<A>& net) {
    if (_root == NULL) abort();
    BaseTrieNode<A> *here;
    here = find_node(net);
    if (here == NULL) {
	fprintf(stderr, "Attempt to delete %s which is not in trie\n", 
		net.str().c_str());
	abort();
    }
    erase(iterator(here));
}

template <class A, class Payload> 
void 
Trie<A,Payload>::erase(iterator iter) {
    assert(_root != NULL);
    assert(iter != end());
    _route_count--;
    BaseTrieNode<A> *here, *parent = NULL;
    here = iter.current();
    parent = here->parent();
    if (here->has_route() == false) {
	cp_h(51);
	validate();
	return;
    }

    if (parent == NULL) {
	cp_h(52);
	/*we're trying to delete the root node*/
	if (here->leftchild() == NULL) {
	    cp_h(53);
	    _root = here->rightchild();
	    if (_root != NULL)
		_root->set_parent(NULL);
	    here->dirty_node();
	    delete here;
	} else if (here->rightchild() == NULL) {
	    cp_h(54);
	    _root = here->leftchild();
	    if (_root != NULL)
		_root->set_parent(NULL);
	    here->dirty_node();
	    delete here;
	} else {
	    cp_h(55);
	    /* we just have to turn this into an interior node*/
	    BaseTrieNode<A> *interior_node = 
		new BaseTrieNode<A>(here->net(), NULL);
	    _root = interior_node;
	    interior_node->replace_node(here);
	}
	//	debug_msg("Delete node %s results in:\n", net.str().c_str());
	//	print();
	validate();
	return;
    }

    cp_h(56);
    /*unplumb and delete the node*/
    parent->replace_child(here, here->leftchild());
    if (here->rightchild() != NULL) {
	cp_h(57);
	parent->add_child(here->rightchild(), 0);
    }
    here->dirty_node();
    delete here;

    if (parent->check_if_needed() == false) {
	cp_h(58);
	if (parent == _root) {
	    cp_h(59);
	    if (_root->leftchild()!=NULL) {
		cp_h(60);
		_root = _root->leftchild();
		_root->set_parent(NULL);
	    } else {
		cp_h(61);
		_root = _root->rightchild();
		_root->set_parent(NULL);
	    }
	    parent->dirty_node();
	    delete parent;
	}
    }
    cp_h(62);
    //    debug_msg("Delete node %s results in:\n", net.str().c_str());
    //print();
    validate();
    return;
}

template <class A, class Payload> 
BaseTrieNode<A> *
Trie<A,Payload>::find_node(const IPNet<A>& net) const {
    cp_h(69);
    if (_root == NULL) return NULL;
    BaseTrieNode<A> *here = _root, *parent = NULL;
    while(here != NULL) {
	cp_h(70);
	debug_msg("here: net: %s\n", here->net().str().c_str());
	if (here->net() == net) {
	    cp_h(71);
	    if (here->has_route() == false) {
		cp_h(72);
		return NULL;
	    }
	    cp_h(73);
	    return here;
	}
	
	cp_h(74);
	/* walk the tree */
	if ((here->leftchild()!=NULL) && 
	    ( here->leftchild()->valid_child(net) ||
	      here->leftchild()->net() == net )) {
	    cp_h(75);
	    parent = here;
	    here = here->leftchild();
	} else if ((here->rightchild()!=NULL) && 
	    ( here->rightchild()->valid_child(net) ||
	      here->rightchild()->net() == net )) {
	    cp_h(76);
	    parent = here;
	    here = here->rightchild();
	} else {
	    cp_h(77);
	    return NULL;
	}
    }
    //can't get here
    abort();
}

template <class A, class Payload> 
TrieIterator<A,Payload>
Trie<A,Payload>::lookup_node(const IPNet<A>& net) const {
    BaseTrieNode<A> *found = find_node(net);
    if (found == NULL)
	return end();
    if (found->has_route()) {
	TrieIterator<A,Payload> iter(found);
	return iter;
    }
    return end();
}

template <class A, class Payload> 
BaseTrieNode<A>*
Trie<A,Payload>::find_node_by_addr(const A& addr) const {
    debug_msg("Trie::find_node_by_addr %s\n", addr.str().c_str());
    IPNet<A> subnet(addr, addr.addr_bitlen());
    if (_root == NULL) return NULL;
    BaseTrieNode<A> *here = _root, *last_valid = NULL;
    cp_h(78);
    while(here != NULL) {
	debug_msg("  here: addr: %s\n", here->net().str().c_str());

	/* keep track of the last valid parent, as we may need to backtrack*/
	if ((here->has_route()) && (here->valid_child(subnet))) {
	    cp_h(79);
	    last_valid = here;
	}

	/* walk the tree */
	if ((here->leftchild()!=NULL) && 
	    ( here->leftchild()->valid_child(subnet) ||
	      here->leftchild()->net() == subnet )) {
	    cp_h(80);
	    here = here->leftchild();
	} else if ((here->rightchild()!=NULL) && 
	    ( here->rightchild()->valid_child(subnet) ||
	      here->rightchild()->net() == subnet )) {
	    cp_h(81);
	    here = here->rightchild();
	} else {
	    cp_h(82);
	    if (here->valid_child(subnet)
		|| here->net() == subnet) {
		cp_h(83);
		if (here->has_route() == false) {
		    cp_h(84);
		    if (last_valid == NULL) {
			cp_h(85);
			debug_msg("NO ROUTE FOUND\n");
		    } else {
			cp_h(86);
			debug_msg("  found: LV addr: %s\n", 
				  last_valid->net().str().c_str());
		    }
		    return last_valid;
		} else {
		    cp_h(87);
		    debug_msg("  found: addr: %s\n", 
			      here->net().str().c_str());
		    return here;
		}
	    }
	    cp_h(88);
	    debug_msg("  find failed\n");
	    print();
	    return NULL;
	}
    }
    cp_h(89);
    debug_msg("fell out the bottom\n");
    return NULL;
}


template <class A, class Payload> 
TrieIterator<A,Payload>
Trie<A,Payload>::find(const A& addr) const {
    debug_msg("Trie::find %s\n", addr.str().c_str());
    BaseTrieNode<A> *found = find_node_by_addr(addr);
    if (found == NULL)
	return end();
    if (found->has_route()) {
	TrieIterator<A,Payload> iter(found);
	return iter;
    }
    return end();
}

template <class A, class Payload> 
BaseTrieNode<A> *
Trie<A,Payload>::find_less_specific_node(const IPNet<A>& net) const {
    debug_msg("Trie::find_less_specific_node %s\n", net.str().c_str());
    if (_root == NULL) return NULL;
    BaseTrieNode<A> *here = _root, *parent = NULL;
    cp_h(90);
    while(here != NULL) {
	cp_h(91);
	debug_msg("  here: net: %s\n", here->net().str().c_str());
	/* walk the tree */
	if ((here->leftchild()!=NULL) && 
	    ( here->leftchild()->valid_child(net) ||
	      here->leftchild()->net() == net )) {
	    cp_h(92);
	    if (here->has_route()) {
		cp_h(93);
		parent = here;
	    }
	    here = here->leftchild();
	} else if ((here->rightchild()!=NULL) && 
	    ( here->rightchild()->valid_child(net) ||
	      here->rightchild()->net() == net )) {
	    cp_h(94);
	    if (here->has_route()) {
		cp_h(95);
		parent = here;
	    }
	    here = here->rightchild();
	} else {
	    cp_h(96);
	    if (here->valid_child(net)) {
		cp_h(97);
		//net is a valid child of "here", but not of its children.
		return here;
	    }
	    if (parent != NULL) {
		cp_h(98);
		//net must be a valid child of "parent" to have got here.
		return parent;
	    }
	    cp_h(99);
	    return NULL;
	}
    }
    cp_h(100);
    debug_msg("fell out the bottom\n");
    return NULL;
}

template <class A, class Payload> 
TrieIterator<A,Payload>
Trie<A,Payload>::find_less_specific(const IPNet<A>& net) const {
    debug_msg("Trie::find_less_specific %s\n", net.str().c_str());
    BaseTrieNode<A> *found;
    found = find_less_specific_node(net);
    if (found == NULL)
	return end();
    if (found->has_route()) {
	TrieIterator<A,Payload> iter(found);
	return iter;
    }
    return end();
}


template <class A, class Payload> 
const A 
Trie<A,Payload>::find_lower_bound(const A& addr) const {
    debug_msg("Trie::find %s\n", addr.str().c_str());
    if (_root==NULL) {
	//there are no routes
	cp_h(128);
	return A::ZERO();
    }
    A a = _root->find_lower_bound(addr);
    if (a==A::ALL_ONES()) {
	cp_h(129);
	return A::ZERO();
    } else {
	cp_h(130);
	return a; 
    }
}	

template <class A, class Payload> 
const A 
Trie<A,Payload>::find_higher_bound(const A& addr) const {
    debug_msg("Trie::find %s\n", addr.str().c_str());
    if (_root==NULL) {
	//there are no routes
	cp_h(131);
	return A::ALL_ONES();
    }
    A a = _root->find_upper_bound(addr);
    if (a==A::ZERO()) {
	cp_h(132);
	return A::ALL_ONES();
    } else {
	cp_h(133);
	return a;
    }
}	

template <class A, class Payload> 
TrieIterator<A,Payload>
Trie<A,Payload>::begin() const {
    TrieIterator<A,Payload> ti;
    ti.set_trienode(_root);
    return ti;
}

template <class A, class Payload> 
TrieIterator<A,Payload>
Trie<A,Payload>::search_subtree(const IPNet<A> net) const {
    TrieIterator<A,Payload> ti;
    ti.set_root(net);
    BaseTrieNode<A> *tn = find_node(net);
    if (tn != NULL)
	debug_msg("exact match\n");
    if (tn == NULL) {
	tn = find_less_specific_node(net);
	if (tn != NULL)
	    debug_msg("less specific match\n");
    }
    if (tn == NULL) {
	if (_root != NULL && net.is_overlap(_root->net())) {
	    debug_msg("whole tree match\n");
	    tn = _root;
	}
    }
    ti.set_trienode(tn);
    if (tn != NULL) {
	if ((tn->leftchild()!=NULL)||(tn->rightchild()!=NULL))
	    ti++;
    }
    if (ti != end())
	debug_msg("first match: %s\n", tn->net().str().c_str());
    else
	debug_msg("no match\n");
    return ti;
}

template <class A, class Payload> 
void 
Trie<A,Payload>::print() const {
    if (_root != NULL) {
	assert(_root->parent() == NULL);
	_root->print(0);
    }
    debug_msg("---------------\n");
#if 0
    TrieIterator<A,Payload> ti;
    ti = begin();
    while (ti!=end()) {
	debug_msg("*** node: %s\n", ti.net().str().c_str());
	ti.next();
    }
    debug_msg("---------------\n");
#endif
}


#endif // __LIBXORP_OLD_TRIE_HH__

