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

// $XORP: xorp/ospf/debug_io.hh,v 1.30 2008/11/14 12:44:19 bms Exp $

#ifndef __OSPF_DEBUG_IO_HH__
#define __OSPF_DEBUG_IO_HH__

/**
 * Debugging implementation of IO for use by test programs.
 */
template <typename A>
class DebugIO : public IO<A> {
 public:
    DebugIO(TestInfo& info, OspfTypes::Version version, EventLoop& eventloop)
	: _info(info), _eventloop(eventloop), _packets(0),
	  _lsa_decoder(version), _next_interface_id(1)
    {
	initialise_lsa_decoder(version, _lsa_decoder);
	initialise_packet_decoder(version, _dec, _lsa_decoder);
    }

    /**
     * Pretty print frames. Specific to DebugIO.
     */
    void pp(const string& which, int level, const string& interface, 
	    const string& vif, A dst, A src,
	    uint8_t* data, uint32_t len) {

	TimeVal now;
	_eventloop.current_time(now);
	DOUT_LEVEL(_info, level) << now.pretty_print() << endl;
	DOUT_LEVEL(_info, level) << which << "(" << interface << "," << vif
		    << "," << dst.str() << "," << src.str()
		    <<  "...)" << endl;

	try {
	    // Decode the packet in order to pretty print it.
	    Packet *packet = _dec.decode(data, len);
	    DOUT_LEVEL(_info, level) << packet->str() << endl;
	    delete packet;
	} catch(InvalidPacket& e) {
	    DOUT_LEVEL(_info, level) << "Probably no decoder provided: " <<
		e.str() <<
		endl;
	}

    }

    int startup() {
	ServiceBase::set_status(SERVICE_READY);
	return (XORP_OK);
    }
    
    int shutdown() {
	ServiceBase::set_status(SERVICE_SHUTDOWN);
	return (XORP_OK);
    }

    /**
     * Send Raw frames.
     */
    bool send(const string& interface, const string& vif, 
	      A dst, A src,
	      int ttl, uint8_t* data, uint32_t len)
    {
	pp("SEND", 0, interface, vif, dst, src, data, len);

	_packets++;
	DOUT(_info) << "packets sent " << _packets << endl;

	if (!_forward_cb.is_empty())
	    _forward_cb->dispatch(interface, vif, dst, src, data, len);
	return true;
	UNUSED(ttl);
    }

    /**
     * Register where frames should be forwarded. Specific to DebugIO.
     */
    bool register_forward(typename IO<A>::ReceiveCallback cb)
    {
	_forward_cb = cb;

	return true;
    }

    /**
     * Receive frames. Specific to DebugIO.
     */
    void receive(const string& interface, const string& vif, 
		 A dst, A src,
		 uint8_t* data, uint32_t len)
    {
	pp("RECEIVE", 1, interface, vif, dst, src, data, len);

	if (! IO<A>::_receive_cb.is_empty())
	    IO<A>::_receive_cb->dispatch(interface, vif, dst, src, data, len);
    }

    /**
     * Enable the interface/vif to receive frames.
     */
    bool enable_interface_vif(const string& interface, const string& vif)
    {
	DOUT(_info) << "enable_interface_vif(" << interface << "," << vif <<
	    "...)" << endl;

	return true;
    }

    /**
     * Disable this interface/vif from receiving frames.
     */
    bool disable_interface_vif(const string& interface, const string& vif)
    {
	DOUT(_info) << "disable_interface_vif(" << interface << "," << vif <<
	    "...)" << endl;

	return true;
    }

    /**
     * Test whether this interface is enabled.
     *
     * @return true if it exists and is enabled, otherwise false.
     */
    bool is_interface_enabled(const string& interface) const
    {
	DOUT(_info) << "enabled(" << interface << ")\n";

	return true;
    }

    /**
     * Test whether this interface/vif is enabled.
     *
     * @return true if it exists and is enabled, otherwise false.
     */
    bool is_vif_enabled(const string& interface, const string& vif) const
    {
	DOUT(_info) << "enabled(" << interface << "," << vif << ")\n";

	return true;
    }

    /**
     * Test whether this interface/vif/address is enabled.
     *
     * @return true if it exists and is enabled, otherwise false.
     */
    bool is_address_enabled(const string& interface, const string& vif,
				    const A& address) const
    {
	DOUT(_info) << "enabled(" << interface << "," << vif << ","
		    << cstring(address) << ")\n";

	return true;
    }

    bool get_addresses(const string& interface, const string& vif,
		       list<A>& /*addresses*/) const
    {
	DOUT(_info) << "get_addresses(" << interface << "," << vif << ","
		    << ")\n";

	return true;
    }

    // It should be safe to return the same address for each interface
    // as it is link local.

    bool get_link_local_address(const string& interface, const string& vif,
				IPv4& address) {

	DOUT(_info) << "link_local(" << interface << "," << vif << ")\n";

	// RFC 3330
	address = IPv4("169.254.0.1");
	
	return true;
    }

    bool get_link_local_address(const string& interface, const string& vif,
				IPv6& address) {

	DOUT(_info) << "link_local(" << interface << "," << vif << ")\n";
	
	// XXX
	// Nasty hack to generate a different link local address for
	// each vif. Assumes that the last character of the vif name
	// is a number, solves a problem with test_peering. Should
	// really hash the whole interface and vid and use the name
	// from the info structure.
	string addr = "fe80::";
	addr.push_back(vif[vif.size() -1]);
	DOUT(_info) << "address = " << addr << ")\n";

	address = IPv6(addr.c_str());

	return true;
    }

    bool get_interface_id(const string& interface, uint32_t& interface_id)
    {
	DOUT(_info) << "get_interface_id(" << interface << ")\n";

	if (0 == _interface_ids.count(interface)) {
	    interface_id = _next_interface_id++;
	    _interface_ids[interface] = interface_id;
	} else {
	    interface_id = _interface_ids[interface];
	}

	return true;
    }

    uint32_t get_prefix_length(const string& interface, const string& vif,
			       A address)
    {
	DOUT(_info) << "get_prefix_length(" << interface << "," << vif << ","
		    << cstring(address) << ")\n";

	return 16;
    }

    uint32_t get_mtu(const string& interface)
    {
	DOUT(_info) << "get_mtu(" << interface << ")\n";

	return 1500;
    }

    /**
     * On the interface/vif join this multicast group.
     */
    bool join_multicast_group(const string& interface, const string& vif,
			      A mcast)
    {
	DOUT(_info) << "join_multicast_group(" << interface << "," << vif <<
	    "," << mcast.str() << ")" << endl;

	return true;
    }

    /**
     * On the interface/vif leave this multicast group.
     */
    bool leave_multicast_group(const string& interface, const string& vif,
			      A mcast)
    {
	DOUT(_info) << "leave_multicast_group(" << interface << "," << vif <<
	    "," << mcast.str() << ")" << endl;

	return true;
    }

    /**
     * Add route to RIB.
     */
    bool add_route(IPNet<A> net, A nexthop, uint32_t nexthop_id, 
		   uint32_t metric, bool equal, bool discard,
		   const PolicyTags& policytags)
    {
	DOUT(_info) << "Net: " << net.str() <<
	    " nexthop: " << nexthop.str() <<
	    " nexthop_id: " << nexthop_id <<
	    " metric: " << metric <<
	    " equal: " << bool_c_str(equal) <<
	    " discard: " << bool_c_str(discard) << 
	    " policy: " << policytags.str() << endl;

	XLOG_ASSERT(0 == _routing_table.count(net));
	DebugRouteEntry dre;
	dre._nexthop = nexthop;
	dre._metric = metric;
	dre._equal = equal;
	dre._discard = discard;
	dre._policytags = policytags;

	_routing_table[net] = dre;

	return true;
    }

    /**
     * Replace route in RIB.
     */
    bool replace_route(IPNet<A> net, A nexthop, uint32_t nexthop_id,
		       uint32_t metric, bool equal, bool discard,
		       const PolicyTags& policytags)
    {
	DOUT(_info) << "Net: " << net.str() <<
	    " nexthop: " << nexthop.str() <<
	    " nexthop_id: " << nexthop_id <<
	    " metric: " << metric <<
	    " equal: " << bool_c_str(equal) <<
	    " discard: " << bool_c_str(discard) <<
	    " policy: " << policytags.str() << endl;

	if (!delete_route(net))
	    return false;

	return add_route(net, nexthop, nexthop_id, metric, equal, discard,
			 policytags);
    }

    /**
     * Delete route from RIB
     */
    bool delete_route(IPNet<A> net)
    {
	DOUT(_info) << "Net: " << net.str() << endl;

	XLOG_ASSERT(1 == _routing_table.count(net));
	_routing_table.erase(_routing_table.find(net));
	
	return true;
    }

    /**
     * A debugging entry point.
     * Empty the routing table.
     */
    void routing_table_empty() {
	_routing_table.clear();
    }

    uint32_t routing_table_size() {
	return _routing_table.size();
    }

    /**
     * Verify that this route is in the routing table.
     */
    bool routing_table_verify(IPNet<A> net, A nexthop, uint32_t metric,
			      bool equal, bool discard) {
	DOUT(_info) << "Net: " << net.str() <<
	    " nexthop: " << nexthop.str() <<
	    " metric: " << metric <<
	    " equal: " << bool_c_str(equal) <<
	    " discard: " << bool_c_str(discard) << endl;

	if (0 == _routing_table.count(net)) {
	    DOUT(_info) << "Net: " << net.str() << " not in table\n";
	    return false;
	}

	DebugRouteEntry dre = _routing_table[net];
	if (dre._nexthop != nexthop) {
	    DOUT(_info) << "Nexthop mismatch: " << nexthop.str() << " " <<
		dre._nexthop.str() << endl;
	    return false;
	}
	if (dre._metric != metric) {
	    DOUT(_info) << "Metric mismatch: " << metric << " " <<
		dre._metric << endl;
	    return false;
	}
	if (dre._equal != equal) {
	    DOUT(_info) << "Equal mismatch: " << bool_c_str(equal) << " " <<
		bool_c_str(dre._equal) << endl;
	    return false;
	}
	if (dre._discard != discard) {
	    DOUT(_info) << "Discard mismatch: " << bool_c_str(discard) << " " <<
		bool_c_str(dre._discard) << endl;
	    return false;
	}

	return true;
    }

    /**
     * Return the number of packets that have seen so far.
     */
    int packets()
    {
	return _packets;
    }
 private:
    TestInfo& _info;
    EventLoop& _eventloop;
    PacketDecoder _dec;
    int _packets;
    LsaDecoder _lsa_decoder;

    typename IO<A>::ReceiveCallback _forward_cb;

    uint32_t _next_interface_id;
    map<string, uint32_t> _interface_ids;

    struct DebugRouteEntry {
	A _nexthop;
	uint32_t _metric;
	bool _equal;
	bool _discard;
	PolicyTags _policytags;
    };

    map<IPNet<A>, DebugRouteEntry> _routing_table;
};
#endif // __OSPF_DEBUG_IO_HH__
