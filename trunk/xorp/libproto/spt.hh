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

// $XORP$

#ifndef __IGP_SPT_HH__
#define __IGP_SPT_HH__

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

    ~Spt();

    /**
     * Set the origin node.
     *
     * @return false if the node doesn't exist, otherwise true.
     */
    bool set_origin(A node);

    /**
     * Add node
     *
     * @return false if the node already exists, otherwise true.
     */
    bool add_node(A node);

    /**
     * Remove node
     *
     * @return false if the node doesn't exist or has already been
     * removed, otherwise true.
     */
    bool remove_node(A node);

    /**
     * Does this node exist?
     *
     * @return true if the node exists.
     */
    bool exists_node(A node);

    /**
     * Add a new edge.
     *
     * @param src source node must exist.
     * @param weight edge weight.
     * @param dst destination node, created if necessary.
     * @return true on success.
     */
    bool add_edge(A src, int weight, A dst);

    /**
     * Update existing edge weight.
     *
     * @param src source node must exist.
     * @param weight new edge weight.
     * @param dst destination node must exist
     * @return true on success.
     */
    bool update_edge_weight(A src, int weight, A dst);

    /**
     * Get edge weight.
     *
     * @param src source node must exist.
     * @param weight of this edge returned.
     * @param dst destination node must exist
     * @return true on success.
     */
    bool get_edge_weight(A src, int& weight, A dst);

    /**
     * Remove an edge
     *
     * @param src source node must exist.
     * @param dst destination node must exist
     * @return true on success.
     */
    bool remove_edge(A src, A dst);
    
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
    typename Node<A>::NodeRef find_node(A node);

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

    Node(A a);

    /**
     * @return nodename
     */
    A nodename();

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
     */
    void set_local_weight(int weight);

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
     */
    void add(typename Node<A>::NodeRef n, int weight);

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

    RouteCmd(Cmd cmd, A node, A nexthop) :
	_cmd(cmd), _node(node), _nexthop(nexthop)
    {}

    Cmd cmd() const { return _cmd; }
    A node() const { return _node; }
    A nexthop() const { return _nexthop; }

    bool operator==(const RouteCmd& lhs) {
	return _cmd == lhs._cmd &&
	    _node == lhs._node &&
	    _nexthop == lhs._nexthop;
    }

    string str() const {
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
	return cmd + " node: " + _node + " nexthop: " + _nexthop;
    }
 private:
    Cmd _cmd;
    A _node;
    A _nexthop;
};

#endif // __IGP_SPT_HH__
