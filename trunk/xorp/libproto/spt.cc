// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/libproto/spt.cc,v 1.1 2004/11/02 23:15:22 atanu Exp $"

// #define INCREMENTAL_SPT

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "libxorp/xorp.h"
#include "libproto_module.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "spt.hh"

template <typename A>
Spt<A>::~Spt()
{
    for (typename Nodes::iterator i = _nodes.begin(); i != _nodes.end(); i++)
	i->second->clear();
}

template <typename A>
bool
Spt<A>::set_origin(A node)
{
    // Lookup this node. It must exist.
    typename Node<A>::NodeRef srcnode = find_node(node);
    if (srcnode.is_empty()) {
	XLOG_WARNING("Node does not exist %s", Node<A>(node).str().c_str());
	return false;
    }

    _origin = srcnode;
    return true;
}

template <typename A>
bool
Spt<A>::add_node(A node)
{
    // If a valid node already exists return false
    typename Node<A>::NodeRef srcnode = find_node(node);
    if (!srcnode.is_empty()) {
	if (srcnode->valid()) {
	    XLOG_WARNING("Node already exists %s",
			 Node<A>(node).str().c_str());
	    return false;
	} else {
	    // We are going to revive this node so dump its adjacency
	    // info.
	    srcnode->drop_adjacencies();
	    srcnode->set_valid(true);
	    return true;
	}
    }

    Node<A> *n = new Node<A>(node);
    _nodes[node] = typename Node<A>::NodeRef(n);

    return true;
}

template <typename A>
bool
Spt<A>::remove_node(A node)
{
    // If a valid node doesn't exist return false
    typename Node<A>::NodeRef srcnode = find_node(node);
    if (srcnode.is_empty()) {
	XLOG_WARNING("Request to delete non-existant node %s",
		     Node<A>(node).str().c_str());
	return false;
    }
    if (!srcnode->valid()) {
	XLOG_WARNING("Node already removed %s", Node<A>(node).str().c_str());
	return false;
    }
    srcnode->set_valid(false);

    return true;
}

template <typename A>
bool
Spt<A>::exists_node(A node)
{
    return _nodes.count(node);
}

template <typename A>
bool
Spt<A>::add_edge(A src, int weight, A dst)
{
    // Find the src node it must exist.
    typename Node<A>::NodeRef srcnode = find_node(src);
    if (srcnode.is_empty()) {
	XLOG_WARNING("Node: %s not found", Node<A>(src).str().c_str());
	return false;
    }

    // The dst node doesn't have to exist. If it doesn't exist create it.
    typename Node<A>::NodeRef dstnode = find_node(dst);
    if (dstnode.is_empty()) {
	if (!add_node(dst)) {
	    XLOG_WARNING("Add node %s failed", Node<A>(dst).str().c_str());
	    return false;
	}
    }
    
    dstnode = find_node(dst);
    if (dstnode.is_empty()) {
	XLOG_WARNING("Node: %s not found",  Node<A>(dst).str().c_str());
	return false;
    }

    srcnode->add_edge(dstnode, weight);

    return true;
}

template <typename A>
bool
Spt<A>::update_edge_weight(A src, int weight, A dst)
{
    typename Node<A>::NodeRef srcnode = find_node(src);
    if (srcnode.is_empty()) {
	debug_msg("Node: %s not found\n", Node<A>(src).str().c_str());
	return false;
    }

    typename Node<A>::NodeRef dstnode = find_node(dst);
    if (dstnode.is_empty()) {
	debug_msg("Node: %s not found\n", Node<A>(dst).str().c_str());
	return false;
    }

    return srcnode->update_edge_weight(dstnode, weight);
}

template <typename A>
bool
Spt<A>::get_edge_weight(A src, int& weight, A dst)
{
    typename Node<A>::NodeRef srcnode = find_node(src);
    if (srcnode.is_empty()) {
	debug_msg("Node: %s not found\n", Node<A>(src).str().c_str());
	return false;
    }

    typename Node<A>::NodeRef dstnode = find_node(dst);
    if (dstnode.is_empty()) {
	debug_msg("Node: %s not found\n", Node<A>(dst).str().c_str());
	return false;
    }

    return srcnode->get_edge_weight(dstnode, weight);
}

template <typename A>
bool
Spt<A>::remove_edge(A src, A dst)
{
    typename Node<A>::NodeRef srcnode = find_node(src);
    if (srcnode.is_empty()) {
	debug_msg("Node: %s not found\n", Node<A>(src).str().c_str());
	return false;
    }

    typename Node<A>::NodeRef dstnode = find_node(dst);
    if (dstnode.is_empty()) {
	debug_msg("Node: %s not found\n", Node<A>(dst).str().c_str());
	return false;
    }

    return srcnode->remove_edge(dstnode);
}

template <typename A>
bool
Spt<A>::compute(list<RouteCmd<A> >& routes)
{
#ifdef	INCREMENTAL_SPT
    if (!incremental_spt())
	return false;
#else
    if (!dijkstra())
	return false;
#endif

    for(typename Nodes::const_iterator ni = _nodes.begin();
	ni != _nodes.end(); ni++) {
	// We don't need to know how to reach ourselves.
	if (ni->second == _origin)
	    continue;
	RouteCmd<A> rcmd;
	if (ni->second->delta(rcmd))
	    routes.push_back(rcmd);
    }

    // Remove all the deleted nodes.
    garbage_collect();

    return true;
}

template <typename A>
string
Spt<A>::str() const
{
    string pres;

    if (_origin.is_empty()) {
	pres = "No origin\n";
    } else
	pres = "Origin: " + _origin->str() + "\n";

    for(typename Nodes::const_iterator ni = _nodes.begin();
	ni != _nodes.end(); ni++) {
	pres += ni->second->pp() + "\n";
    }

    return pres;
}

template <typename A>
inline
void
init_dijkstra(const pair<A, typename Node<A>::NodeRef >& p)
{
    p.second->set_tentative(true);
    p.second->invalidate_weights();
}

template <typename A>
bool
Spt<A>::dijkstra()
{
    if (_origin.is_empty()) {
	XLOG_WARNING("No origin");
	return false;
    }

    for_each(_nodes.begin(), _nodes.end(), ptr_fun(init_dijkstra<A>));

    typename Node<A>::NodeRef current = _origin;
    _origin->set_tentative(false);

    int weight = 0;
    // Map of tentative nodes.
    PriorityQueue<A> tentative;

    for(;;) {
	// Set the weight on all the nodes that are adjacent to this one.
	current->set_adjacent_weights(current, weight, tentative);

	if (tentative.empty())
	    break;

	current = tentative.pop();
	XLOG_ASSERT(!current.is_empty());

	// Get the weight of this node.
	weight = current->get_local_weight();

	// Make the node permanent.
	current->set_tentative(false);

	// Compute the next hop to get to this node.
	typename Node<A>::NodeRef prev = current->get_last_hop();
	if (prev == _origin)
	    current->set_first_hop(current);
	else
	    current->set_first_hop(prev->get_first_hop());

	debug_msg("Permanent: %s distance %d next hop %s\n",
		  current->str().c_str(),
		  weight,
		  current->get_first_hop()->str().c_str());
    }
    
    return true;
}

template <typename A>
bool
Spt<A>::incremental_spt()
{
    XLOG_UNFINISHED();
}

template <typename A>
typename Node<A>::NodeRef
Spt<A>::find_node(A node)
{
    typename map<A, typename Node<A>::NodeRef>::iterator i = _nodes.find(node);

    if (i != _nodes.end()) {
	// debug_msg("Node %s found\n", Node<A>(node).str().c_str());
	return (*i).second;
    }

    //  debug_msg("Node %s not found\n", Node<A>(node).str().c_str());

//    Node<A> *n = 0;
    return typename Node<A>::NodeRef(/*n*/);
}
template <typename A>
Node<A>::Node(A nodename)
    :  _valid(true), _nodename(nodename)
{
}

template <typename A>
A
Node<A>::nodename()
{
    return _nodename;
}

template <typename A>
bool
Node<A>::add_edge(NodeRef dst, int weight)
{
    // See if this edge already exists.
    typename adjacency::iterator i = _adjacencies.find(dst->nodename());

    // If this edge already exists consider this an error.
    if (i != _adjacencies.end()) {
	debug_msg("Edge from %s to %s exists\n", str().c_str(),
		  dst->str().c_str());
	return false;
    }

    _adjacencies[dst->nodename()] = Edge<A>(dst, weight);

    return true;
}

template <typename A>
bool
Node<A>::update_edge_weight(NodeRef dst, int weight)
{
    typename adjacency::iterator i = _adjacencies.find(dst->nodename());

    // This edge should exist.
    if (i == _adjacencies.end()) {
	debug_msg("Edge from %s to %s doesn't exists\n", str().c_str(),
		  dst->str().c_str());
	return false;
    }

    Edge<A> edge = i->second;
    edge._weight = weight;
    i->second = edge;

    return true;
}

template <typename A>
bool
Node<A>::get_edge_weight(NodeRef dst, int& weight)
{
    typename adjacency::iterator i = _adjacencies.find(dst->nodename());

    // This edge should exist.
    if (i == _adjacencies.end()) {
	debug_msg("Edge from %s to %s doesn't exists\n", str().c_str(),
		  dst->str().c_str());
	return false;
    }

    Edge<A> edge = i->second;
    weight = edge._weight;

    return true;
}

template <typename A>
bool
Node<A>::remove_edge(NodeRef dst)
{
    typename adjacency::iterator i = _adjacencies.find(dst->nodename());

    if (i == _adjacencies.end()) {
	XLOG_WARNING("Edge from %s to %s doesn't exists", str().c_str(),
		     dst->str().c_str());
	return false;
    }

    _adjacencies.erase(i);

    return true;
}

template <typename A>
void
Node<A>::drop_adjacencies()
{
    _adjacencies.clear();
}

template <typename A>
void
Node<A>::garbage_collect()
{
    typename adjacency::iterator ni;
    for(ni = _adjacencies.begin(); ni != _adjacencies.end();) {
	NodeRef node = ni->second._dst;
	if (!node->valid()) {
	    // Clear any references that this node may have to itself.
	    node->clear();
	    _adjacencies.erase(ni++);
	} else {
	    ni++;
	}
    }    
}

template <typename A>
void
Node<A>::set_adjacent_weights(NodeRef me, int delta_weight,
			      PriorityQueue<A>& tentative)
{
    typename adjacency::iterator i;
    for(i = _adjacencies.begin(); i != _adjacencies.end(); i++) {
	NodeRef n = i->second._dst;
	debug_msg("Node: %s\n", n->str().c_str());
	if (n->valid() && n->tentative()) {
	    n->set_last_hop(me);
	    // It is critial that the weight of a node is not changed
	    // while it is in the PriorityQueue.
	    tentative.add(n, delta_weight + i->second._weight);
	}
    }
}

template <typename A>
void
Node<A>::set_local_weight(int weight)
{
    // If this node is no longer tentative we shouldn't be changing
    // its value.
    XLOG_ASSERT(_tentative);

    // If no valid state exists just set the weight otherwise make
    // sure it's less than the value already present.
    if (!_current._valid) {
	_current._path_length = weight;
	_current._valid = true;
    } else {
	if (_current._path_length > weight)
	    _current._path_length = weight;
    }
}

template <typename A>
int
Node<A>::get_local_weight()
{
//     debug_msg("Node: %s\n", str().c_str());

    // This node must be valid, tentative and its value must be valid.

    XLOG_ASSERT(_valid);
    XLOG_ASSERT(_tentative);
    XLOG_ASSERT(_current._valid);

    return _current._path_length;
}

template <typename A>
bool
Node<A>::delta(RouteCmd<A>& rcmd)
{
    // Has this node been deleted?
    if (!valid()) {
	rcmd = RouteCmd<A>(RouteCmd<A>::DELETE, nodename(), nodename());
	return true;
    }

    path p,c;
    c = _current;
    p = _previous;
    _previous = _current;

    // It is possible that this node is not reachable.
    if (!c._valid) {
	XLOG_WARNING("Node: %s not reachable", str().c_str());
	if (p._valid) {
	    rcmd = RouteCmd<A>(RouteCmd<A>::DELETE, nodename(), nodename());
	    return true;	    
	}
	return false;
    }

    // If the previous result is invalid this is a new route.
    if (!p._valid) {
	XLOG_ASSERT(_current._valid);
	rcmd = RouteCmd<A>(RouteCmd<A>::ADD, nodename(),
			   _current._first_hop->nodename());
	return true;
    }

    XLOG_ASSERT(c._valid);
    XLOG_ASSERT(p._valid);

    // If nothing has changed, nothing to report.
    if (c._first_hop == p._first_hop)
	return false;

    rcmd = RouteCmd<A>(RouteCmd<A>::REPLACE, nodename(),
			   _current._first_hop->nodename());

    return true;
}

template <typename A>
class Pa: public unary_function<pair<A, Edge<A> >, void> {
 public:
    void operator()(const pair<A, Edge<A> >& p) {
	Edge<A> e = p.second;
	_result += e.str();
    }

    string result() const {
	return _result;
    }
 private:
    string _result;
};

template <typename A>
string
Node<A>::pp() const
{
    string result = str() + ":: ";

    result += for_each(_adjacencies.begin(),_adjacencies.end(),
		       Pa<A>()).result();

    return result;
}

template <>
string
Node<string>::str() const
{
    return _nodename;
}

template <typename A>
string
Node<A>::str() const
{
    return _nodename.str();
}

template <typename A>
inline
void
gc(const pair<A, typename Node<A>::NodeRef >& p)
{
    p.second->garbage_collect();
}

template <typename A>
void
Spt<A>::garbage_collect()
{
    // Remove all the invalid nodes.
    for(typename Nodes::iterator ni = _nodes.begin(); ni != _nodes.end();) {
	typename Node<A>::NodeRef node = ni->second;
	if (!node->valid()) {
	    _nodes.erase(ni++);
	} else {
	    ni++;
	}
    }

    // Garbage collect all the edges that point at deleted nodes.
    for_each(_nodes.begin(), _nodes.end(), ptr_fun(gc<A>));
}

template <typename A> 
void
PriorityQueue<A>::add(typename Node<A>::NodeRef n, int weight)
{
    // Find this node if its already in set and remove it.
    if (n->valid_weight()) {
	typename Tent::iterator i = _tentative.find(n);
	for(; i != _tentative.end(); i++) {
	    if ((*i) == n) {
// 		debug_msg("Erase %s\n", (*i)->str().c_str());
		_tentative.erase(i);
		break;
	    }
	}
    }
    n->set_local_weight(weight);
    _tentative.insert(n);

//     debug_msg("Insert %s\n", n->str().c_str());
}

template <typename A> 
typename Node<A>::NodeRef
PriorityQueue<A>::pop()
{
    // Find this node if its already in set and remove it.
    typename Tent::iterator i = _tentative.begin();
    if (i == _tentative.end())
	return typename Node<A>::NodeRef();

    typename Node<A>::NodeRef n = *i;
    _tentative.erase(i);

//     debug_msg("Pop %s\n", n->str().c_str());

    return n;
}

template class Node<string>;
template class Spt<string>;

template class Node<IPv4>;
template class Spt<IPv4>;

template class Node<IPv6>;
template class Spt<IPv6>;
