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

#ident "$XORP: xorp/bgp/test_peer_data.cc,v 1.5 2004/04/15 16:13:30 hodson Exp $"

#define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "config.h"
#include "libxorp/test_main.hh"
#include "libxorp/debug.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/asnum.hh"
#include "iptuple.hh"
#include "parameter.hh"
#include "peer_data.hh"

bool
test1(TestInfo& info)
{
    DOUT(info) << "test1: " <<  endl;

    assert(BGPPeerData::SENT < BGPPeerData::ARRAY_SIZE);
    assert(BGPPeerData::RECEIVED < BGPPeerData::ARRAY_SIZE);
    assert(BGPPeerData::NEGOTIATED < BGPPeerData::ARRAY_SIZE);

    return true;
}

/*
** Fill in a parameter list structure with the parameters a peer might
** send.
*/
void
open_packet_parameters(ParameterList& parameters)
{
    parameters.push_back(new BGPRefreshCapability());
    parameters.push_back(new BGPMultiProtocolCapability(AFI_IPV4,
							SAFI_UNICAST));
    parameters.push_back(new BGPMultiProtocolCapability(AFI_IPV6,
							SAFI_UNICAST));
    parameters.push_back(new BGPMultiProtocolCapability(AFI_IPV4,
							SAFI_MULTICAST));
    parameters.push_back(new BGPMultiProtocolCapability(AFI_IPV6,
							SAFI_MULTICAST));
}

bool
test2(TestInfo& info)
{
    DOUT(info) << "test2: " <<  endl;

    /*
    ** Create a PeerData structure.
    */
    Iptuple iptuple;
    BGPPeerData pd(iptuple, AsNum(12), IPv4("10.10.10.10"), 0);

    if(!pd.multiprotocol<IPv4>(SAFI_UNICAST)) {
	DOUT(info) << "We should offer unicast IPv4 by default\n";
	return false;
    }

    if(pd.multiprotocol<IPv6>(SAFI_UNICAST)) {
	DOUT(info) << "We should not offer unicast IPv6 by default\n";
	return false;
    }

    /*
    ** We support Unicast IPv6.
    */
    pd.add_sent_parameter(new BGPMultiProtocolCapability(AFI_IPV6,
							 SAFI_UNICAST));

    /*
    ** Create a parameter list analogous to the list we get from an
    ** open packet (our peer).
    */
    ParameterList parameters;
    open_packet_parameters(parameters);

    /*
    ** Pass the paremeters into the PeerData as we would do from an
    ** open packet.
    */
    pd.save_parameters(parameters);

    /*
    ** Do the negotiation.
    */
    pd.open_negotiation();

    if(!pd.multiprotocol<IPv4>(SAFI_UNICAST)) {
	DOUT(info) << "We should still be offering unicast IPv4\n";
	return false;
    }

    /*
    ** We should have decided that unicast IPv6 is a go.
    */
    if(!pd.multiprotocol<IPv6>(SAFI_UNICAST)) {
	DOUT(info) << "We did not manage to negotiate IPv6\n";
	return false;
    }

    /*
    ** Create a parameter list analogous to the list we get from an
    ** open packet (our peer).
    */
    ParameterList np;
    open_packet_parameters(np);

    /*
    ** Remove the parameters.
    */
    ParameterList::iterator i;
    for(i = np.begin(); i != np.end(); i++)
	pd.remove_recv_parameter(*i);

    /*
    ** Verify that the list is empty.
    */
    if(!pd.parameter_recv_list().empty()) {
	DOUT(info) << "The receive list should be empty\n";
	return false;
    }

    return true;
}

int
main(int argc, char** argv) 
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    TestMain t(argc, argv);

    string test_name =
	t.get_optional_args("-t", "--test", "run only the specified test");
    t.complete_args_parsing();

    try {
	struct test {
	    string test_name;
	    XorpCallback1<bool, TestInfo&>::RefPtr cb;
	} tests[] = {
	    {"test1", callback(test1)},
	    {"test2", callback(test2)},
	};

	if("" == test_name) {
	    for(unsigned int i = 0; i < sizeof(tests) / sizeof(struct test); 
		i++)
		t.run(tests[i].test_name, tests[i].cb);
	} else {
	    for(unsigned int i = 0; i < sizeof(tests) / sizeof(struct test); 
		i++)
		if(test_name == tests[i].test_name) {
		    t.run(tests[i].test_name, tests[i].cb);
		    return t.exit();
		}
	    t.failed("No test with name " + test_name + " found\n");
	}
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    return t.exit();
}
