// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
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

// $XORP: xorp/ospf/debug_io.hh,v 1.8 2005/09/16 23:33:28 atanu Exp $

#ifndef __OSPF_DEBUG_IO_HH__
#define __OSPF_DEBUG_IO_HH__

/**
 * Debugging implementation of IO for use by test programs.
 */
template <typename A>
class DebugIO : public IO<A> {
 public:
    DebugIO(TestInfo& info, OspfTypes::Version version, EventLoop& eventloop)
	: _info(info), _eventloop(eventloop), _packets(0), _running(false),
	  _lsa_decoder(version)
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
	} catch(BadPacket& e) {
	    DOUT_LEVEL(_info, level) << "Probably no decoder provided: " <<
		e.str() <<
		endl;
	}

    }

    bool startup() {
	_running = true;
	return true;
    }
    
    bool running() {
	return _running;
    }

    bool shutdown() {
	_running = false;
	return true;
    }

    /**
     * Send Raw frames.
     */
    bool send(const string& interface, const string& vif, 
	      A dst, A src,
	      uint8_t* data, uint32_t len)
    {
	pp("SEND", 0, interface, vif, dst, src, data, len);

	_packets++;
	DOUT(_info) << "packets sent " << _packets << endl;

	if (!_forward_cb.is_empty())
	    _forward_cb->dispatch(interface, vif, dst, src, data, len);
	return true;
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

    bool enabled(const string& interface, const string& vif, A address)
    {
	DOUT(_info) << "enabled(" << interface << "," << vif << ","
		    << cstring(address) << ")\n";

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
    bool add_route(IPNet<A> net, A nexthop, uint32_t metric, bool equal,
		   bool discard)
    {
	DOUT(_info) << "Net: " << net.str() <<
	    " nexthop: " << nexthop.str() <<
	    " metric: " << metric <<
	    " equal: " << pb(equal) <<
	    " discard: " << pb(discard) << endl;
	return true;
    }

    /**
     * Replace route in RIB.
     */
    bool replace_route(IPNet<A> net, A nexthop, uint32_t metric, bool equal,
		   bool discard)
    {
	DOUT(_info) << "Net: " << net.str() <<
	    " nexthop: " << nexthop.str() <<
	    " metric: " << metric <<
	    " equal: " << pb(equal) <<
	    " discard: " << pb(discard) << endl;
	return true;
    }

    /**
     * Delete route from RIB
     */
    bool delete_route(IPNet<A> net)
    {
	DOUT(_info) << "Net: " << net.str() << endl;
	
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
    bool _running;
    LsaDecoder _lsa_decoder;

    typename IO<A>::ReceiveCallback _forward_cb;
};
#endif // __OSPF_DEBUG_IO_HH__
