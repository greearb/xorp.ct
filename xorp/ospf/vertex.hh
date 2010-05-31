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

// $XORP: xorp/ospf/vertex.hh,v 1.18 2008/10/02 21:57:50 bms Exp $

#ifndef __OSPF_VERTEX_HH__
#define __OSPF_VERTEX_HH__

/**
 * Vertex required for computing the shortest path tree.
 */

class Vertex {
 public:
    Vertex() : _origin(false), _nexthop_id(OspfTypes::UNUSED_INTERFACE_ID)
    {}

    bool operator<(const Vertex& other) const {
	XLOG_ASSERT(get_version() == other.get_version());
	switch(_version) {
	case OspfTypes::V2:
	    if (_nodeid == other.get_nodeid())
		return _t < other.get_type();
	    break;
	case OspfTypes::V3:
	    if (_nodeid == other.get_nodeid() && _t != other.get_type())
		return _t < other.get_type();
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

    bool operator==(const Vertex& other) const {
	XLOG_ASSERT(get_version() == other.get_version());
	return _nodeid == other.get_nodeid() && _t ==  other.get_type();
    }

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
     * OSPFv2 only.
     * Set the LSA that is responsible for this vertex.
     */
    void set_lsa(Lsa::LsaRef lsar) {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	XLOG_ASSERT(0 == _lsars.size());
	_lsars.push_back(lsar);
    }

    /**
     * OSPFv2 only.
     * Get the LSA that is responsible for this vertex.
     */
    Lsa::LsaRef get_lsa() const {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	XLOG_ASSERT(1 == _lsars.size());
	return *(_lsars.begin());
    }

    /**
     * OSPFv3 only.
     * Return the list of LSAs that may be responsible for this vertex.
     */
    list<Lsa::LsaRef>& get_lsas() {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return _lsars;
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

    void set_address(IPv4 address) {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	_address_ipv4 = address;
    }

    IPv4 get_address_ipv4() const {
	XLOG_ASSERT(OspfTypes::V2 == get_version());
	return _address_ipv4;
    }

    void set_address(IPv6 address) {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	_address_ipv6 = address;
    }

    IPv6 get_address_ipv6() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return _address_ipv6;
    }

    void set_nexthop_id(uint32_t nexthop_id) {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	_nexthop_id = nexthop_id;
    }

    uint32_t get_nexthop_id() const {
	XLOG_ASSERT(OspfTypes::V3 == get_version());
	return _nexthop_id;
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
			       _nodeid, cstring(_address_ipv4),
			       _address_ipv4.addr());
	    break;
	case OspfTypes::V3:
	    output = "OSPFv3";
	    if (_origin)
		output += "(Origin)";
	    switch(_t) {
	    case OspfTypes::Router:
		output += c_format(" Router %s(%#x)", pr_id(_nodeid).c_str(),
				   _nodeid);
		break;
	    case OspfTypes::Network:
		output += c_format(" Transit %s(%#x) %u", 
				   pr_id(_nodeid).c_str(), _nodeid,
				   _interface_id);
		break;
	    }
	    output += c_format(" %s", cstring(_address_ipv6));
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

    // The address of the Vertex that should be used as the nexthop by
    // the origin.
    IPv4 _address_ipv4;		
    IPv6 _address_ipv6;

    // If this vertex is directly connected to the origin this is the
    // interface ID that should be used with the nexthop.
    uint32_t _nexthop_id;

    list<Lsa::LsaRef> _lsars;

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
