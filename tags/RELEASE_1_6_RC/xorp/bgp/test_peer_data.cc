// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/bgp/test_peer_data.cc,v 1.20 2008/07/23 05:09:39 pavlin Exp $"

#define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"

#include "libxorp/xorp.h"
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
    EventLoop eventloop;
    LocalData localdata(eventloop);
    Iptuple iptuple;
    BGPPeerData pd(localdata, iptuple, AsNum(12), IPv4("10.10.10.10"), 0);

    if(pd.multiprotocol<IPv4>(SAFI_UNICAST)) {
	DOUT(info) << "We should not offer unicast IPv4 by default\n";
	return false;
    }

    if(pd.multiprotocol<IPv6>(SAFI_UNICAST)) {
	DOUT(info) << "We should not offer unicast IPv6 by default\n";
	return false;
    }

    /*
    ** We support Unicast IPv4.
    */
    pd.add_sent_parameter(new BGPMultiProtocolCapability(AFI_IPV4,
							 SAFI_UNICAST));
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
	ParameterList pl = pd.parameter_recv_list();
	for(ParameterList::iterator i = pl.begin(); i != pl.end(); i++)
	    DOUT(info) << "Parameter = " << (*i)->str() << endl;
	return false;
    }

    return true;
}

int
main(int argc, char** argv) 
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

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

    xlog_stop();
    xlog_exit();

    return t.exit();
}
