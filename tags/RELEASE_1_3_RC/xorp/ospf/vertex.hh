// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2006 International Computer Science Institute
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

// $XORP: xorp/ospf/vertex.hh,v 1.5 2005/09/13 18:27:27 atanu Exp $

#ifndef __OSPF_VERTEX_HH__
#define __OSPF_VERTEX_HH__

/**
 * Vertex required for computing the shortest path tree.
 */

class Vertex {
 public:
    Vertex() : _origin(false), _address(0)
    {}

    bool operator<(const Vertex& other) const {
	XLOG_ASSERT(get_version() == other.get_version());
	switch(_version) {
	case OspfTypes::V2:
	    if (_nodeid == other.get_nodeid())
		return _t < other.get_type();
	    break;
	case OspfTypes::V3:
	    switch(_t) {
	    case OspfTypes::Router:
		break;
	    case OspfTypes::Network:
		if (_nodeid == other.get_nodeid())
		    return _interface_id < other.get_interface_id();
		break;
	    }
	    break;
	}
	return _nodeid < other.get_nodeid();
    }

#if	1
    bool operator==(const Vertex& other) const {
	XLOG_ASSERT(get_version() == other.get_version());
	return _nodeid == other.get_nodeid() && _t ==  other.get_type();
    }
#endif

    void set_version(OspfTypes::Version v) {
	_version = v;
    }

    OspfTypes::Version get_version() const {
	return _version;
    }

    void set_type(OspfTypes::VertexType t) {
	_t = t;
    }

    OspfTypes::VertexType get_type() const {
	return _t;
    }

    void set_nodeid(uint32_t nodeid) {
	_nodeid = nodeid;
    }

    uint32_t get_nodeid() const {
	return _nodeid;
    }

    /**
     * Set the LSA that is responsible for this vertex.
     */
    void set_lsa(Lsa::LsaRef lsar) {
	_lsar = lsar;
    }

    /**
     * Get the LSA that is responsible for this vertex.
     */
    Lsa::LsaRef get_lsa() const {
	return _lsar;
    }

    void set_interface_id(uint32_t interface_id) {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	_interface_id = interface_id;
    }

    uint32_t get_interface_id() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return _interface_id;
    }

    void set_origin(bool origin) {
	_origin = origin;
    }

    bool get_origin() const {
	return _origin;
    }

    void set_address(uint32_t address) {
	_address = address;
    }

    uint32_t get_address() const {
	return _address;
    }

    string str() const {
	string output;
	switch(_version) {
	case OspfTypes::V2:
	    output = "OSPFv2";
	    if (_origin)
		output += "(Origin)";
	    switch(_t) {
	    case OspfTypes::Router:
		output += " Router";
		break;
	    case OspfTypes::Network:
		output += " Network";
		break;
	    }
	    output += c_format(" %s(%#x) %s(%#x)", pr_id(_nodeid).c_str(), 
			       _nodeid, pr_id(_address).c_str(), _address);
	    break;
	case OspfTypes::V3:
	    output = "OSPFv3";
	    switch(_t) {
	    case OspfTypes::Router:
		output += c_format(" Router %#x", _nodeid);
		break;
	    case OspfTypes::Network:
		output += c_format(" Transit %#x %#x", _nodeid, _interface_id);
		break;
	    }
	    break;
	}
	return output;
    }

 private:
    /*const */OspfTypes::Version _version;
    
    OspfTypes::VertexType _t;	// Router or Network (Transit in OSPFv3) 
    uint32_t _nodeid;
    uint32_t _interface_id;	// OSPFv3 Only

    bool _origin;		// Is this the vertex of the router.
    uint32_t _address;		// The address of the Vertex that
				// should be used as the nexthop by the origin.
    Lsa::LsaRef _lsar;

    // RFC 2328 Section 16.1.  Calculating the shortest-path tree for an area:
    // Vertex (node) ID
    //     A 32-bit number which together with the vertex type (router
    // 	   or network) uniquely identifies the vertex.  For router
    // 	   vertices the Vertex ID is the router's OSPF Router ID.  For
    // 	   network vertices, it is the IP address of the network's
    // 	   Designated Router.

    // RFC 2470 Section 3.8.1.Calculating the shortest path tree for an area:
    // o  The Vertex ID for a router is the OSPF Router ID. The Vertex ID
    //    for a transit network is a combination of the Interface ID and
    //    OSPF Router ID of the network's Designated Router.
};

#endif // __OSPF_VERTEX_HH__
