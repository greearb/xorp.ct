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

inline
void
populate_hello(HelloPacket *hello, OspfTypes::Version version)
{
    switch(version) {
    case OspfTypes::V2:
	populate_helloV2(hello);
	break;
    case OspfTypes::V3:
	populate_helloV3(hello);
	break;
    }
}

inline
void
populate_standard_header(Packet *packet, OspfTypes::Version version)
{
    packet->set_router_id(IPv4("128.16.64.16"));
    packet->set_area_id(IPv4("4.3.2.1"));

    switch(version) {
    case OspfTypes::V2:
	packet->set_auth_type(5);
	break;
    case OspfTypes::V3:
	packet->set_instance_id(5);
	break;
    }
}

inline
void
populate_data_description(DataDescriptionPacket *ddp,
			  OspfTypes::Version version)
{
    populate_standard_header(ddp, version);

    ddp->set_interface_mtu(1500);
    ddp->set_options(0xfe);
    ddp->set_i_bit(true);
    ddp->set_m_bit(true);
    ddp->set_ms_bit(true);
    ddp->set_dd_seqno(0x01020304);

    // Create a LSA Header to add
    Lsa_header header(version);

    header.set_ls_age(500);
    switch(version) {
    case OspfTypes::V2:
	header.set_options(0xff);
	break;
    case OspfTypes::V3:
	break;
    }
    header.set_link_state_id(0x01020304);
    header.set_advertising_router(0x04030201);
    header.set_ls_sequence_number(0x0A0B0C0D);
    header.set_ls_checksum(0x1234);
    header.set_length(200);

    ddp->get_lsa_headers().push_back(header);
}

inline
void
populate_link_state_request(LinkStateRequestPacket *lsrp,
			    OspfTypes::Version version)
{
    populate_standard_header(lsrp, version);

    lsrp->get_ls_request().
	push_back(Ls_request(version, 0xff, 0x0a0b0c0d, 0x12345678));
}

bool
hello_packet_print(TestInfo& info)
{
    HelloPacket *hello= new HelloPacket(OspfTypes::V2);
    populate_hello(hello, OspfTypes::V2);

    DOUT(info) << hello->str() << endl;

    delete hello;

    hello= new HelloPacket(OspfTypes::V3);
    populate_hello(hello, OspfTypes::V3);

    DOUT(info) << hello->str() << endl;

    delete hello;

    return true;
}

bool
hello_packet_compare(TestInfo& info, OspfTypes::Version version)
{
    HelloPacket *hello1= new HelloPacket(version);
    populate_hello(hello1, version);

    DOUT(info) << hello1->str() << endl;

    // Encode the hello packet.
    vector<uint8_t> pkt1;
    hello1->encode(pkt1);

    // Now decode the packet.
    // Create a new packet to provide the decoder.
    HelloPacket *hello2= new HelloPacket(version);

    HelloPacket *hello3 =
	dynamic_cast<HelloPacket *>(hello2->decode(&pkt1[0], pkt1.size()));

    DOUT(info) << hello3->str() << endl;

    // Encode the second packet and compare.
    vector<uint8_t> pkt2;
    hello3->encode(pkt2);
    
    if (pkt1.size() != pkt2.size()) {
	DOUT(info) << "Packet lengths don't match " <<
	    pkt1.size() << " " << pkt2.size() << endl;
	return false;
    }
    
    if (0 != memcmp(&pkt1[0], &pkt2[0], pkt1.size())) {
	for(size_t i = 0; i < pkt1.size(); i++) {
	    if (pkt1[i] != pkt2[i]) {
		DOUT(info) << "mismatch at byte position " << i << endl;
		DOUT(info) << "bytes " <<
		    (int)pkt1[i] << " " << (int)pkt2[i] << endl;
		break;
	    }
	}
	return false;
    }

    delete hello1;
    delete hello2;
    delete hello3;

    return true;
}

bool
data_description_packet_print(TestInfo& info)
{
    DataDescriptionPacket *ddp= new DataDescriptionPacket(OspfTypes::V2);
    populate_data_description(ddp, OspfTypes::V2);

    DOUT(info) << ddp->str() << endl;

    delete ddp;

    ddp = new DataDescriptionPacket(OspfTypes::V3);
    populate_data_description(ddp, OspfTypes::V3);

    DOUT(info) << ddp->str() << endl;

    delete ddp;

    return true;
}

bool
data_description_packet_compare(TestInfo& info, OspfTypes::Version version)
{
    DataDescriptionPacket *ddp1= new DataDescriptionPacket(version);
    populate_data_description(ddp1, version);

    DOUT(info) << ddp1->str() << endl;

    // Encode the Data Description Packet.
    vector<uint8_t> pkt1;
    ddp1->encode(pkt1);

    // Now decode the packet.
    // Create a new packet to provide the decoder.
    DataDescriptionPacket *ddp2= new DataDescriptionPacket(version);

    DataDescriptionPacket *ddp3 =
	dynamic_cast<DataDescriptionPacket *>(ddp2->
					      decode(&pkt1[0], pkt1.size()));

    DOUT(info) << ddp3->str() << endl;

    // Encode the second packet and compare.
    vector<uint8_t> pkt2;
    ddp3->encode(pkt2);
    
    if (pkt1.size() != pkt2.size()) {
	DOUT(info) << "Packet lengths don't match " <<
	    pkt1.size() << " " << pkt2.size() << endl;
	return false;
    }
    
    if (0 != memcmp(&pkt1[0], &pkt2[0], pkt1.size())) {
	for(size_t i = 0; i < pkt1.size(); i++) {
	    if (pkt1[i] != pkt2[i]) {
		DOUT(info) << "mismatch at byte position " << i << endl;
		DOUT(info) << "bytes " <<
		    (int)pkt1[i] << " " << (int)pkt2[i] << endl;
		break;
	    }
	}
	return false;
    }

    delete ddp1;
    delete ddp2;
    delete ddp3;

    return true;
}

bool
link_state_request_packet_print(TestInfo& info)
{
    LinkStateRequestPacket *lsrp= new LinkStateRequestPacket(OspfTypes::V2);
    populate_link_state_request(lsrp, OspfTypes::V2);

    DOUT(info) << lsrp->str() << endl;

    delete lsrp;

    lsrp = new LinkStateRequestPacket(OspfTypes::V3);
    populate_link_state_request(lsrp, OspfTypes::V3);

    DOUT(info) << lsrp->str() << endl;

    delete lsrp;

    return true;
}

bool
link_state_request_packet_compare(TestInfo& info, OspfTypes::Version version)
{
    LinkStateRequestPacket *lsrp1= new LinkStateRequestPacket(version);
    populate_link_state_request(lsrp1, version);

    DOUT(info) << lsrp1->str() << endl;

    // Encode the Data Description Packet.
    vector<uint8_t> pkt1;
    lsrp1->encode(pkt1);

    // Now decode the packet.
    // Create a new packet to provide the decoder.
    LinkStateRequestPacket *lsrp2= new LinkStateRequestPacket(version);

    LinkStateRequestPacket *lsrp3 =
	dynamic_cast<LinkStateRequestPacket *>(lsrp2->
					       decode(&pkt1[0], pkt1.size()));

    DOUT(info) << lsrp3->str() << endl;

    // Encode the second packet and compare.
    vector<uint8_t> pkt2;
    lsrp3->encode(pkt2);
    
    if (pkt1.size() != pkt2.size()) {
	DOUT(info) << "Packet lengths don't match " <<
	    pkt1.size() << " " << pkt2.size() << endl;
	return false;
    }
    
    if (0 != memcmp(&pkt1[0], &pkt2[0], pkt1.size())) {
	for(size_t i = 0; i < pkt1.size(); i++) {
	    if (pkt1[i] != pkt2[i]) {
		DOUT(info) << "mismatch at byte position " << i << endl;
		DOUT(info) << "bytes " <<
		    (int)pkt1[i] << " " << (int)pkt2[i] << endl;
		break;
	    }
	}
	return false;
    }

    delete lsrp1;
    delete lsrp2;
    delete lsrp3;

    return true;
}

bool
decoder1(TestInfo& info, OspfTypes::Version version)
{
    PacketDecoder dec;
    HelloPacket hello(version);
    populate_hello(&hello, version);

    vector<uint8_t> pkt;
    hello.encode(pkt);

    // An attempt to decode a packet with no decoders installed should
    // fail.
    try {
	dec.decode(&pkt[0], pkt.size());
    } catch(BadPacket& e) {
	DOUT(info) << "Caught exception: " << e.str() << endl;
	return true;
    }

    return false;
}

bool
decoder2(TestInfo& info, OspfTypes::Version version)
{
    PacketDecoder dec;
    // Install a decoder for hello packets.
    Packet *hello_decoder = new HelloPacket(version);
    dec.register_decoder(hello_decoder);

    HelloPacket hello(version);
    populate_hello(&hello, version);

    
    vector<uint8_t> pkt;
    hello.encode(pkt);

    // An attempt to decode a packet with a decoder installed should succeed.
    Packet *packet = dec.decode(&pkt[0], pkt.size());

    DOUT(info) << packet->str() << endl;

    delete packet;

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
	{"data_description_print", callback(data_description_packet_print)},
	{"link_state_request_print",
	 callback(link_state_request_packet_print)},
	{"hello_compareV2", callback(hello_packet_compare, OspfTypes::V2)},
	{"hello_compareV3", callback(hello_packet_compare, OspfTypes::V3)},
	{"ddp_compareV2", callback(data_description_packet_compare,
				   OspfTypes::V2)},
	{"ddp_compareV3", callback(data_description_packet_compare,
				   OspfTypes::V3)},
	{"lsrp_compareV2", callback(link_state_request_packet_compare,
				   OspfTypes::V2)},
	{"lsrp_compareV3", callback(link_state_request_packet_compare,
				   OspfTypes::V3)},
	{"decoder1V2", callback(decoder1, OspfTypes::V2)},
	{"decoder1V3", callback(decoder1, OspfTypes::V3)},
	{"decoder2V2", callback(decoder2, OspfTypes::V2)},
	{"decoder2V3", callback(decoder2, OspfTypes::V3)},
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
