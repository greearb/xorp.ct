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

#include "ospf.hh"
#include "packet.hh"

// Make sure that all tests free up any memory that they use. This will
// allow us to use the leak checker program.

/**
 * Fill all the fields except for the 8 byte auth in a V2 hello packet.
 */
inline
void
populate_helloV2(HelloPacket *hello)
{
    hello->set_router_id(IPv4("128.16.64.16"));
    hello->set_area_id(IPv4("4.3.2.1"));
    hello->set_auth_type(5);
    hello->set_network_mask(0x12345678);
    hello->set_hello_interval(9876);
    hello->set_options(0xfe);
    hello->set_router_priority(42);
    hello->set_router_dead_interval(66000);
    hello->set_designated_router(IPv4("1.2.3.4"));
    hello->set_backup_designated_router(IPv4("2.4.6.8"));

    // Add some neighbours.
    hello->get_neighbours().push_back(IPv4("10.11.12.13"));
    hello->get_neighbours().push_back(IPv4("11.12.13.14"));
}

/**
 * Fill all the fields in a V3 hello packet.
 */
inline
void
populate_helloV3(HelloPacket *hello)
{
    hello->set_router_id(IPv4("128.16.64.16"));
    hello->set_area_id(IPv4("4.3.2.1"));
    hello->set_instance_id(5);
    hello->set_interface_id(0x12345678);
    hello->set_hello_interval(98760);
    hello->set_options(0xfefefe);
    hello->set_router_priority(42);
    hello->set_router_dead_interval(6600);
    hello->set_designated_router(IPv4("1.2.3.4"));
    hello->set_backup_designated_router(IPv4("2.4.6.8"));

    // Add some neighbours.
    hello->get_neighbours().push_back(IPv4("10.11.12.13"));
    hello->get_neighbours().push_back(IPv4("11.12.13.14"));
}

bool
hello_packet_print(TestInfo& info)
{
    HelloPacket *hello= new HelloPacket(OspfTypes::V2);

    DOUT(info) << hello->str() << endl;

    delete hello;

    hello= new HelloPacket(OspfTypes::V3);

    DOUT(info) << hello->str() << endl;

    delete hello;

    return true;
}

bool
hello_packet_compareV2(TestInfo& info)
{
    HelloPacket *hello1= new HelloPacket(OspfTypes::V2);
    populate_helloV2(hello1);

    DOUT(info) << hello1->str() << endl;

    // Encode the hello packet.
    size_t len;
    uint8_t *ptr = hello1->encode(len);

    // Now decode the packet.
    HelloPacket *hello2= new HelloPacket(OspfTypes::V2);

    HelloPacket *hello3 =
	dynamic_cast<HelloPacket *>(hello2->decode(ptr, len));

    DOUT(info) << hello3->str() << endl;

    delete hello1;
    delete hello2;
    delete hello3;
    delete ptr;

    return true;
}

bool
hello_packet_compareV3(TestInfo& info)
{
    HelloPacket *hello1= new HelloPacket(OspfTypes::V3);
    populate_helloV3(hello1);

    DOUT(info) << hello1->str() << endl;

    // Encode the hello packet.
    size_t len;
    uint8_t *ptr = hello1->encode(len);

    // Now decode the packet.
    HelloPacket *hello2= new HelloPacket(OspfTypes::V3);

    HelloPacket *hello3 =
	dynamic_cast<HelloPacket *>(hello2->decode(ptr, len));

    DOUT(info) << hello3->str() << endl;

    delete hello1;
    delete hello2;
    delete hello3;
    delete ptr;

    return true;
}

bool
decoder1(TestInfo& info)
{
    PacketDecoder dec;
    HelloPacket hello(OspfTypes::V2);
    populate_helloV2(&hello);

    size_t len;
    uint8_t *ptr = hello.encode(len);

    // An attempt to decode a packet with no decoders installed should
    // fail.
    try {
	dec.decode(ptr, len);
    } catch(BadPacket& e) {
	DOUT(info) << "Caught exception: " << e.str() << endl;
	return true;
    }

    return false;
}

bool
decoder2(TestInfo& info)
{
    PacketDecoder dec;
    // Install a decoder for hello packets.
    Packet *hello_decoder = new HelloPacket(OspfTypes::V2);
    dec.register_decoder(hello_decoder);

    HelloPacket hello(OspfTypes::V2);
    populate_helloV2(&hello);

    size_t len;
    uint8_t *ptr = hello.encode(len);

    // An attempt to decode a packet with a decoder installed should succeed.
    Packet *packet = dec.decode(ptr, len);

    DOUT(info) << packet->str() << endl;

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
	{"hello_print", callback(hello_packet_print)},
	{"hello_compareV2", callback(hello_packet_compareV2)},
	{"hello_compareV3", callback(hello_packet_compareV3)},
	{"decoder1", callback(decoder1)},
	{"decoder2", callback(decoder2)},
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
