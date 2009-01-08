// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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

// $XORP: xorp/contrib/olsr/vertex.hh,v 1.3 2008/10/02 21:56:36 bms Exp $

#ifndef __OLSR_VERTEX_HH__
#define __OLSR_VERTEX_HH__

/**
 * @short A vertex in the shortest path tree.
 */
class Vertex {
 public:
    Vertex();

    explicit Vertex(const Neighbor& n);

    explicit Vertex(const TwoHopNeighbor& n2);

    explicit Vertex(const IPv4& main_addr);

    explicit Vertex(const TopologyEntry& tc);

    /**
     * Compare two Vertex instances for less-than ordering.
     *
     * Collation order for sort:
     * 1. Main address; always unique.
     *
     * XXX The following two can't be relied upon... because differentiating
     * them at all means we can't look up nodes purely by their address!
     * 2. Is origin vertex: true, false.
     * 3. Vertex type: origin, onehop, twohop, tc.
     *
     * @param other node to compare with.
     * @return true if this node comes before the other node.
     */
    inline bool operator<(const Vertex& other) const {
#if 0
	if (_main_addr == other.main_addr()) {
	    if (_is_origin == other.is_origin())
		return _t < other.type();
	    return _is_origin > other.is_origin();
	}
#endif
	return _main_addr < other.main_addr();
    }

    /**
     * Compare two Vertex instances for equality.
     *
     * Comparison is performed solely on the main address of the node,
     * which is always unique.
     *
     * @param other node to compare with.
     * @return true if this node and the other node are equal.
     */
    inline bool operator==(const Vertex& other) const {
	return _main_addr == other.main_addr();
    }

    inline void set_main_addr(const IPv4& main_addr) {
	_main_addr = main_addr;
    }

    inline IPv4 main_addr() const {
	return _main_addr;
    }

    void set_nodeid(uint32_t nid) {
	_nodeid = nid;
    }

    uint32_t nodeid() const {
	return _nodeid;
    }

    void set_is_origin(bool v) {
	_is_origin = v;
    }

    /**
     * @return true if this node represents this router.
     */
    bool is_origin() const {
	return _is_origin;
    }

    void set_producer(const IPv4& producer) {
	_producer = producer;
    }

    IPv4 producer() const {
	return _producer;
    }

    void set_faceid(const OlsrTypes::FaceID fid) {
	_faceid = fid;
    }

    OlsrTypes::FaceID faceid() const {
	return _faceid;
    }

    /**
     * @return the LogicalLink associated with a one-hop neighbor.
     */
    const LogicalLink* link() const {
	return _link;
    }

    void set_link(const LogicalLink* l) {
	XLOG_ASSERT(_t == OlsrTypes::VT_NEIGHBOR);
	_link = l;
    }

    /**
     * @return the TwoHopLink associated with a two-hop neighbor.
     */
    const TwoHopLink* twohop_link() const {
	return _twohop_link;
    }

    void set_twohop_link(const TwoHopLink* l2) {
	XLOG_ASSERT(_t == OlsrTypes::VT_TWOHOP);
	_twohop_link = l2;
    }

    void set_type(const OlsrTypes::VertexType t) {
	_t = t;
    }

    OlsrTypes::VertexType type() const {
	return _t;
    }

    string str() const {
	string output = "OLSR";
	output += c_format(" Node %s", cstring(_main_addr));
	output += c_format(" Type %u", XORP_UINT_CAST(type()));
	output += c_format(" ID %u", XORP_UINT_CAST(nodeid()));
	return output;
    }

 private:
    /**
     * @short true if this Vertex represents this OLSR router.
     */
    bool	_is_origin;

    /**
     * The type of node in the OLSR link state databases from which
     * this Vertex is derived.
     */
    OlsrTypes::VertexType _t;

    /**
     * ID of the Neighbor, TwoHopNeighbor or TopologyEntry responsible
     * for the creation of this vertex.
     */
    uint32_t	_nodeid;

    /**
     * The main protocol address of the OLSR node is used as the unique
     * node identifier in the shortest-path tree.
     */
    IPv4	_main_addr;

    /**
     * The main protocol address of the OLSR node from where this vertex
     * was learned. In the routing CLI this is referred to as the origin;
     * it is NOT the origin of the SPT graph.
     */
    IPv4	_producer;

    // XXX: union candidate.

    /**
     * If the OLSR node which this vertex represents is directly
     * connected to the origin, this is the FaceID which should be
     * used to reach the next hop.
     */
    OlsrTypes::FaceID	_faceid;

    /**
     * If this is a one-hop neighbor vertex, this is the link which
     * was chosen to reach the neighbor.
     */
    const LogicalLink*	_link;

    /**
     * If this is a two-hop neighbor vertex, this is the link which
     * was chosen to reach it.
     */
    const TwoHopLink*	_twohop_link;
};

/**
 * Default constructor for Vertex.
 */
inline
Vertex::Vertex()
 : _is_origin(false),
   _t(OlsrTypes::VT_UNKNOWN),
   _nodeid(OlsrTypes::UNUSED_NEIGHBOR_ID),
   _faceid(OlsrTypes::UNUSED_FACE_ID)
{
}

// There are several explicit constructors for Vertex. These are used
// to populate Vertex with the information which we will need when
// plumbing routes to the policy engine for redistribution, after the
// SPT computation.

/**
 * Explicit constructor to:
 *  Create a Vertex from a Neighbor.
 *
 * A neighbor vertex is learned from the neighbor itself, so mark it
 * as the producer.
 *
 * @param n the Neighbor represented by this Vertex.
 */
inline
Vertex::Vertex(const Neighbor& n)
 : _is_origin(false),
   _t(OlsrTypes::VT_NEIGHBOR),
   _nodeid(n.id()),
   _main_addr(n.main_addr()),
   _producer(n.main_addr()),
   _faceid(OlsrTypes::UNUSED_FACE_ID)
{
}

/**
 * Explicit constructor to:
 *  Create a Vertex from a TwoHopNeighbor.
 *
 * The producer of a two-hop vertex is the one-hop neighbor which
 * advertised the link we choose to reach it, which we set after
 * creation using set_producer().
 *
 * The face ID is not known until the SPT calculation has run and
 * paths have been chosen; it will be that of the path selected at
 * radius = 1.
 *
 * @param n2 the TwoHopNeighbor represented by this Vertex.
 */
inline
Vertex::Vertex(const TwoHopNeighbor& n2)
 : _is_origin(false),
   _t(OlsrTypes::VT_TWOHOP),
   _nodeid(n2.id()),
   _main_addr(n2.main_addr()),
   _faceid(OlsrTypes::UNUSED_FACE_ID)
{
}

/**
 * Explicit constructor to:
 *  Create a Vertex from a protocol address.
 *
 * Typically used for sanity checking when populating the SPT from
 * OLSR's TC database.
 */
inline
Vertex::Vertex(const IPv4& main_addr)
 : _is_origin(false),
   _t(OlsrTypes::VT_TOPOLOGY),
   _main_addr(main_addr)
{
}

/**
 * Explicit constructor to:
 *  Create a Vertex from a topology control tuple.
 *
 * The producer of a TC vertex is the OLSR node which advertised
 * the TC entry from which the vertex has been created.
 */
inline
Vertex::Vertex(const TopologyEntry& tc)
 : _is_origin(false),
   _t(OlsrTypes::VT_TOPOLOGY),
   _nodeid(tc.id()),
   _main_addr(tc.destination()),
   _producer(tc.lasthop())
{
}

#endif // __OLSR_VERTEX_HH__
