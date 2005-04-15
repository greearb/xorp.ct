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

#ident "$XORP$"

#define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"
#include <map>
#include <list>

#include "ospf_module.h"
#include "libxorp/xorp.h"

#include "libxorp/test_main.hh"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

#include "libxorp/eventloop.hh"

#include "ospf.hh"

class DebugIO : public IO {
 public:
    DebugIO(TestInfo& info, OspfTypes::Version version, EventLoop& eventloop)
	: _info(info), _eventloop(eventloop), _packets(0)
    {
	_dec.register_decoder(new HelloPacket(version));
    }

    /**
     * Send Raw frames.
     */
    bool send(const string& interface, const string& vif, 
	      uint8_t* data, uint32_t len)
    {
	TimeVal now;
	_eventloop.current_time(now);
	DOUT(_info) << now.pretty_print() << endl;
	DOUT(_info) << "send(" << interface << "," << vif << "...)" << endl;
	Packet *packet = _dec.decode(data, len);

	DOUT(_info) << packet->str() << endl;

	delete packet;

	_packets++;

	return true;
    }

    /**
     * Register for receiving raw frames.
     */
    bool register_receive(ReceiveCallback /*cb*/)
    {
	return true;
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
     * Add route to RIB.
     */
    bool add_route()
    {
	return true;
    }

    /**
     * Delete route from RIB
     */
    bool delete_route()
    {
	return true;
    }

    /**
     * Return the number of packets we have seen so far.
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
};

template <typename A> 
bool
single_peer(TestInfo& info, OspfTypes::Version version)
{
    DOUT(info) << "hello" << endl;

    EventLoop eventloop;
    string ribname = "rib";
    DebugIO io(info, version, eventloop);
    
    Ospf<A> ospf(version, eventloop, &io);
    ospf.set_router_id("192.150.187.78");

    OspfTypes::AreaID area("128.16.64.16");

    // Create an area
    ospf.get_peer_manager().create_area_router(area, OspfTypes::BORDER);

    // Create a peer associated with this area.
    const string interface = "eth0";
    const string vif = "vif0";
    PeerID peerid = ospf.get_peer_manager().create_peer(interface, vif,
							OspfTypes::BROADCAST,
							area);

    // Bring the peering up
    ospf.get_peer_manager().set_state_peer(peerid, true);

    while (ospf.running()) {
	eventloop.run();
	if (2 == io.packets())
	    break;
    }

    return true;
}

int
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    TestMain t(argc, argv);

    string test =
	t.get_optional_args("-t", "--test", "run only the specified test");
    t.complete_args_parsing();

    struct test {
	string test_name;
	XorpCallback1<bool, TestInfo&>::RefPtr cb;
    } tests[] = {
	{"single_peerV2", callback(single_peer<IPv4>, OspfTypes::V2)},
	{"single_peerV3", callback(single_peer<IPv6>, OspfTypes::V3)},
    };

    try {
	if (test.empty()) {
	    for (size_t i = 0; i < sizeof(tests) / sizeof(struct test); i++)
		t.run(tests[i].test_name, tests[i].cb);
	} else {
	    for (size_t i = 0; i < sizeof(tests) / sizeof(struct test); i++)
		if (test == tests[i].test_name) {
		    t.run(tests[i].test_name, tests[i].cb);
		    return t.exit();
		}
	    t.failed("No test with name " + test + " found\n");
	}
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    return t.exit();
}
