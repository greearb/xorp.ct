// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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

// $XORP: xorp/libproto/spt.hh,v 1.19 2008/01/04 03:16:20 pavlin Exp $

#ifndef __LIBPROTO_SPT_HH__
#define __LIBPROTO_SPT_HH__

// #define INCREMENTAL_SPT

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "libxorp/ref_ptr.hh"
#include "libxorp/c_format.hh"
#include <list>
#include <map>
#include <set>

template <typename A> class Spt;
template <typename A> class Edge;
template <typename A> class Node;
template <typename A> class PriorityQueue;
template <typename A> class RouteCmd;

/**
 * Shortest Path Tree
 *
 * Compute shortest path tree's
 *
 */
template <typename A>
class Spt {
 public:
    //    typedef Node<A>::NodeRef NodeRef;
    typedef map<A, typename Node<A>::NodeRef> Nodes;

    Spt(bool trace = true) : _trace(trace)
    {}

    ~Spt();

    /**
     * Clear all state from this Spt instance.
     */
    void clear();

    /**
     * Set the origin node.
     *
     * @return false if the node doesn't exist, otherwise true.
     */
    bool set_origin(const A& node);

    /**
     * Add node
     *
     * @return false if the node already exists, otherwise true.
     */
    bool add_node(const A& node);

     /**
     * Update node
     *
     * @return false if the node doesn't exist, otherwise true.
     */
    bool update_node(const A& node);

   /**
     * Remove node
     *
     * @return false if the node doesn't exist or has already been
     * removed, otherwise true.
     */
    bool remove_node(const A& node);

    /**
     * Does this node exist?
     *
     * @return true if the node exists.
     */
    bool exists_node(const A& node);

    /**
     * Add a new edge.
     *
     * @param src source node must exist.
     * @param weight edge weight.
     * @param dst destination node, created if necessary.
     * @return true on success.
     */
    bool add_edge(const A& src, int weight, const A& dst);

    /**
     * Update existing edge weight.
     *
     * @param src source node must exist.
     * @param weight new edge weight.
     * @param dst destination node must exist
     * @return true on success.
     */
    bool update_edge_weight(const A& src, int weight, const A& dst);

    /**
     * Get edge weight.
     *
     * @param src source node must exist.
     * @param weight of this edge returned.
     * @param dst destination node must exist
     * @return true on success.
     */
    bool get_edge_weight(const A& src, int& weight, const A& dst);

    /**
     * Remove an edge
     *
     * @param src source node must exist.
     * @param dst destination node must exist
     * @return true on success.
     */
    bool remove_edge(const A& src, const A& dst);
    
    /**
     * Compute the tree.
     *
     * @param routes a list of route adds, deletes and replaces that must be
     * performed.
     * @return true on success
     */
    bool compute(list<RouteCmd<A> >& routes);

    /**
     * Convert this graph to presentation format.
     *
     * @return C++ string with the human-readable ASCII representation
     * of the graph.
     */
    string str() const;
 private:
    bool _trace;		// True of tracing is enabled.
    /**
     * Dijkstra
     *
     * @return true on success.
     */
    bool dijkstra();
    
    /**
     * Incremental SPT.
     *
     * @return true on success.
     */
    bool incremental_spt();

    /**
     * Find this node.
     */
    typename Node<A>::NodeRef find_node(const A& node);

    /**
     * Remove all the nodes that have been marked for deletion.
     */
    void garbage_collect();

    typename Node<A>::NodeRef _origin;	// Origin node

    Nodes _nodes;		// Nodes
};

template <typename A>
class Node {
 public:
    typedef map <A, Edge<A> > adjacency; // Only one edge allowed
					 // between nodes.

    typedef ref_ptr<Node<A> > NodeRef;

    Node(A a, bool trace = false);

    ~Node();

    /**
     * @return nodename
     */
    A nodename();

    /**
     * Set nodename
     * DONT' USE THIS METHOD.
     * Changing the nodename could alter the result of the comparison function,
     * causing confusion in the spt map.
     *
     * @return true on success.
     */
    bool set_nodename(A nodename);

    /**
     * Add a new edge.
     *
     * @return true on success. false if edge already exists.
     */
    bool add_edge(NodeRef dst, int weight);

    /**
     * Update edge weight.
     *
     * @return true on success, false if the edge doesn't exist.
     */
   bool update_edge_weight(NodeRef dst, int weight);

    /**
     * Get edge weight.
     *
     * @return true on success, false if the edge doesn't exist.
     */
   bool get_edge_weight(NodeRef dst, int& weight);

    /**
     * Remove an edge
     */
    bool remove_edge(NodeRef dst);

    /**
     * Drop all adjacencies.
     * Used to revive invalid nodes.
     */
    void drop_adjacencies();

    /**
     * Remove all edges that point at invalid nodes.
     */
    void garbage_collect();

    /**
     * Set the valid state.
     */
    void set_valid(bool p) { _valid = p; }

    /**
     * @return true if this node is not marked for deletion.
     */
    bool valid() { return _valid;}

    /**
     * Set the tentative state.
     */
    void set_tentative(bool p) { _tentative = p; }

    /**
     * Get the tentative state.
     */
    bool tentative() { return _tentative; }

    /**
     * Invalidate the weights.
     */
    void invalidate_weights() { _current._valid = false; }

    /**
     * Is the current entry valid.
     */
    bool valid_weight() { return _current._valid; }

    /**
     * Set weights.
     * Visit all neighbours that are tentative and add this weight.
     * 
     * @param delta_weight to add to this node.
     * @param tentative add all updated adjacent nodes to the
     * tentative set.
     */
    void set_adjacent_weights(NodeRef me, int delta_weight,
			      PriorityQueue<A>& tentative);

    /**
     * Set local weight.
     * Set the weight on this node if its tentative and less than the
     * previous value.
     *
     * @return true if its accepted.
     */
    bool set_local_weight(int weight);

    /**
     * get local weight.
     * 
     */
    int get_local_weight();

    /**
     * The first hop to this node.
     */
    void set_first_hop(NodeRef n) { _current._first_hop = n; }

    /**
     * The first hop to this node.
     */
    NodeRef get_first_hop() {
	XLOG_ASSERT(_current._valid);
	return _current._first_hop;
    }

    /**
     * The node before this.
     */
    void set_last_hop(NodeRef n) { _current._last_hop = n; }

    /**
     * The node before this.
     */
    NodeRef get_last_hop() {
	XLOG_ASSERT(_current._valid);
	return _current._last_hop;
    }

    /**
     * Return the difference between this computation and the last.
     *
     * @param rcmd the new route to this node if it has changed.
     * @return true if the node has changed.
     */
    bool delta(RouteCmd<A>& rcmd);

    /**
     * Clear all the references to other nodes as well as possible
     * references to ourselves.
     */
    void clear() {
	_current.clear();
	_previous.clear();
	_adjacencies.clear();
    }

    /**
     * Pretty print this node with its adjacencies
     */
    string pp() const;

    /**
     * @return C++ string with the human-readable ASCII representation
     * of the node.
     */
    string str() const;
 private:
    bool _valid;		// True if node is not marked for deletion.
    A _nodename;		// Node name, external name of this node.
    adjacency _adjacencies;	// Adjacency list
    bool _trace;		// True of tracing is enabled.

    // private:
    //    friend class Spt<A>;

    bool _tentative;		// Intermediate state for Dijkstra.

    struct path {
	path()
	    : _valid(false) {}

	bool _valid;		// Are these entries valid.
	NodeRef _first_hop;	// On the path to this node, the
				// neighbour of the origin.
	NodeRef _last_hop;	// On the path to this node the
				// previous node.
	int _path_length;	// The sum of all the edge weights.

	void clear() {
	    _first_hop = _last_hop = typename Node<A>::NodeRef();
	}
    };

    path _current;		// Current computation.
    path _previous;		// Previous computation.
};

template <typename A>
class Edge {
 public:
    Edge() {}
    Edge(typename Node<A>::NodeRef dst, int weight) :
	_dst(dst), _weight(weight)
    {}

    string str() const {
	return c_format("%s(%d) ", _dst->str().c_str(), _weight);
    }

    typename Node<A>::NodeRef _dst;
    int _weight;
};


/**
 * Tentative nodes in a priority queue.
 */
template <typename A> 
class PriorityQueue {
 public:
    /**
     * Add or Update the weight of a node.
     * @return true if the weight was used.
     */
    bool add(typename Node<A>::NodeRef n, int weight);

    /**
     * Pop the node with lowest weight.
     */
    typename Node<A>::NodeRef pop();

    bool empty() { return _tentative.empty(); }
 private:
    template <typename B>
    struct lweight {
	bool
	operator ()(const typename Node<B>::NodeRef& a,
		    const typename Node<B>::NodeRef& b) const {
	    int aw = a->get_local_weight();
	    int bw = b->get_local_weight();

	    // If the weights match then sort on node names which must
	    // be unique.
	    if (aw == bw)
		return a.get() < b.get();

	    return aw < bw;
	}
    };

    typedef set<typename Node<A>::NodeRef, lweight<A> > Tent;
    Tent _tentative;
};

/**
 * The idealised command to execute.
 */
template <typename A>
class RouteCmd {
 public:
    enum Cmd {ADD, DELETE, REPLACE};

    RouteCmd() {}

    RouteCmd(Cmd cmd, A node, A nexthop, A prevhop, int weight = 0,
	     bool next_hop_changed = false, bool weight_changed = false) :
	_cmd(cmd), _node(node), _nexthop(nexthop), _prevhop(prevhop), 
	_weight(weight),_next_hop_changed(next_hop_changed),
	_weight_changed(weight_changed)
	
    {}

    Cmd cmd() const { return _cmd; }
    const A& node() const { return _node; }
    const A& nexthop() const { return _nexthop; }
    const A& prevhop() const { return _prevhop; }
    int weight() const { return _weight; }
    bool next_hop_changed() const { return _next_hop_changed; }
    bool weight_changed() const { return _weight_changed; }

    bool operator==(const RouteCmd& lhs) {
	return _cmd == lhs._cmd &&
	    _node == lhs._node &&
	    _nexthop == lhs._nexthop &&
	    _prevhop == lhs._prevhop &&
	    _weight == lhs._weight &&
	    _next_hop_changed == lhs._next_hop_changed &&
	    _weight_changed == lhs._weight_changed;
    }

    string c() const {
	string cmd;
	switch(_cmd) {
	case ADD:
	    cmd = "ADD";
	    break;
	case DELETE:
	    cmd = "DELETE";
	    break;
	case REPLACE:
	    cmd = "REPLACE";
	    break;
	}
	return cmd;
    }


    string str() const {
	return c() + " node: " + _node.str() +
	    " nexthop: " + _nexthop.str() +
	    " prevhop: " + _prevhop.str() +
	    " weight: " + c_format("%d", _weight) +
	    " next hop changed: " + bool_c_str(_next_hop_changed) +
	    " weight changed: " + bool_c_str(_weight_changed);
    }

 private:
    Cmd _cmd;
    A _node;
    A _nexthop;
    A _prevhop;
    int _weight;
    bool _next_hop_changed;
    bool _weight_changed;
};

template <typename A>
Spt<A>::~Spt()
{
    clear();
}

template <typename A>
void
Spt<A>::clear()
{
    //XLOG_TRACE(_trace, "Clearing spt %p.", this);

    // Release the origin node by assigning an empty value to its ref_ptr.
    _origin = typename Node<A>::NodeRef();

    // Free all node state in the Spt.
    // A depth first traversal might be more efficient, but we just want
    // to free memory here. Container Nodes knows nothing about the
    // degree of each Node.
    // Because the last node reference is held in the container we must be
    // careful not to introduce another one in this scope by using
    // a reference to a Node ref_ptr.
    while (! _nodes.empty()) {
	typename Nodes::iterator ii;
	for (ii = _nodes.begin(); ii != _nodes.end(); ) {
	    typename Node<A>::NodeRef & rnr = (*ii).second;
	    //XLOG_ASSERT(! rnr.is_empty());
	    rnr->clear();
	    if (rnr.is_only()) {
		_nodes.erase(ii++);
	    } else {
		ii++;
	    }
	}
    }
}

template <typename A>
bool
Spt<A>::set_origin(const A& node)
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
Spt<A>::add_node(const A& node)
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

    Node<A> *n = new Node<A>(node, _trace);
    _nodes[node] = typename Node<A>::NodeRef(n);

    //debug_msg("added node %p\n", n);

    return true;
}

template <typename A>
bool
Spt<A>::update_node(const A& node)
{
    // If a valid node doesn't exist return false
    typename Node<A>::NodeRef srcnode = find_node(node);
    if (srcnode.is_empty()) {
	XLOG_WARNING("Request to update non-existant node %s",
		     Node<A>(node).str().c_str());
	return false;
    }
    if (!srcnode->valid()) {
	XLOG_WARNING("Node is not valid %s", Node<A>(node).str().c_str());
	return false;
    }
    srcnode->set_nodename(node);

    return true;
}

template <typename A>
bool
Spt<A>::remove_node(const A& node)
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
Spt<A>::exists_node(const A& node)
{
    return _nodes.count(node);
}

template <typename A>
bool
Spt<A>::add_edge(const A& src, int weight, const A& dst)
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

    return srcnode->add_edge(dstnode, weight);
}

template <typename A>
bool
Spt<A>::update_edge_weight(const A& src, int weight, const A& dst)
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
Spt<A>::get_edge_weight(const A& src, int& weight, const A& dst)
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
Spt<A>::remove_edge(const A& src, const A& dst)
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

    for_each(_nodes.begin(), _nodes.end(), init_dijkstra<A>);

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

	debug_msg("Previous: %s\n", prev->str().c_str());
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

    return true;
}

template <typename A>
typename Node<A>::NodeRef
Spt<A>::find_node(const A& node)
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
Node<A>::Node(A nodename, bool trace)
    :  _valid(true), _nodename(nodename), _trace(trace)
{
}

template <typename A>
Node<A>::~Node()
{
    clear();
}

template <typename A>
A
Node<A>::nodename()
{
    return _nodename;
}

template <typename A>
bool
Node<A>::set_nodename(A nodename)
{
    _nodename = nodename;
    
    return true;
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

    _adjacencies.insert(make_pair(dst->nodename(), Edge<A>(dst, weight)));

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
	    // It is critial that the weight of a node is not changed
	    // while it is in the PriorityQueue.
	    if (tentative.add(n, delta_weight + i->second._weight))
		n->set_last_hop(me);
	}
    }
}

template <typename A>
bool
Node<A>::set_local_weight(int weight)
{
    // If this node is no longer tentative we shouldn't be changing
    // its value.
    XLOG_ASSERT(_tentative);

    bool accepted = false;

    // If no valid state exists just set the weight otherwise make
    // sure it's less than the value already present.
    if (!_current._valid) {
	_current._path_length = weight;
	_current._valid = true;
	accepted = true;
    } else {
	if (_current._path_length > weight) {
	    _current._path_length = weight;
	    accepted = true;
	}
    }

    return accepted;
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
	rcmd = RouteCmd<A>(RouteCmd<A>::DELETE,
			   nodename(), nodename(), nodename());
	return true;
    }

    path p,c;
    c = _current;
    p = _previous;
    _previous = _current;

    // It is possible that this node is not reachable.
    if (!c._valid) {
	XLOG_TRACE(_trace, "Node: %s not reachable", str().c_str());
	if (p._valid) {
	    rcmd = RouteCmd<A>(RouteCmd<A>::DELETE,
			       nodename(), nodename(), nodename());
	    return true;	    
	}
	return false;
    }

    // If the previous result is invalid this is a new route.
    if (!p._valid) {
	XLOG_ASSERT(_current._valid);
	rcmd = RouteCmd<A>(RouteCmd<A>::ADD, nodename(),
			   _current._first_hop->nodename(),
			   _current._last_hop->nodename(),
			   _current._path_length);
	return true;
    }

    XLOG_ASSERT(c._valid);
    XLOG_ASSERT(p._valid);

    // If nothing has changed, nothing to report.
    if (c._first_hop == p._first_hop && c._path_length == p._path_length)
	return false;

    rcmd = RouteCmd<A>(RouteCmd<A>::REPLACE, nodename(),
		       _current._first_hop->nodename(),
		       _current._last_hop->nodename(),
		       _current._path_length,
		       c._first_hop != p._first_hop,
		       c._path_length != p._path_length);

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

template <typename A>
string
Node<A>::str() const
{
    return _nodename.str();
}

#if	0
template <>
string
Node<string>::str() const
{
    return _nodename;
}
#endif

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
    // Use a reference, no need to bump the ref_ptr in this scope.
    for(typename Nodes::iterator ni = _nodes.begin(); ni != _nodes.end();) {
	typename Node<A>::NodeRef & node = ni->second;
	if (!node->valid()) {
	    _nodes.erase(ni++);
	} else {
	    ni++;
	}
    }

    // Garbage collect all the edges that point at deleted nodes.
    for_each(_nodes.begin(), _nodes.end(), gc<A>);
}

template <typename A> 
bool
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
    bool accepted = n->set_local_weight(weight);
    _tentative.insert(n);

    return accepted;
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

#endif // __LIBPROTO_SPT_HH__
