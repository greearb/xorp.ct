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



#include "ospf_module.h"

#include "libxorp/xorp.h"
#include "libxorp/test_main.hh"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/tokenize.hh"





#include "ospf.hh"
#include "packet.hh"
#include "test_common.hh"


// Make sure that all tests free up any memory that they use. This will
// allow us to use the leak checker program.

/*
 * Compare two packets
 *
 * @return true when they are equal
 */
inline
bool
compare_packets(TestInfo& info, vector<uint8_t>& pkt1, vector<uint8_t>& pkt2)
{
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

    return true;
}

/*
 * Compare the string renditions of packets
 *
 * @return true when they are equal
 */
inline
bool
compare_packet_strings(TestInfo& info, string str1, string str2)
{
    if (str1 != str2) {
	if (info.verbose()) {
	    vector<string> token1;
	    vector<string> token2;
	    tokenize(str1, token1, "\n");
	    tokenize(str2, token2, "\n");
	    vector<string>::iterator i1 = token1.begin();
	    vector<string>::iterator i2 = token2.begin();
	    for(;;) {
		if (token1.end() == i1 || token2.end() == i2) {
		    DOUT(info) << "Ran out of tokens\n";
		    break;
		}
		if (*i1 != *i2) {
		    DOUT(info) << *i1
			       << " *** DOES NOT MATCH ***" 
			       << *i2 << endl;
		    break;
		}
		i1++;
		i2++;
	    }
	}
	return false;
    }

    return true;
}

/**
 * Modify the frame lengths to see if we can freak the decoder.
 */
inline
bool
packet_bad_length(TestInfo& info, OspfTypes::Version version,
		  vector<uint8_t>& pkt)
{
    PacketDecoder packet_decoder;
    LsaDecoder lsa_decoder(version);

    initialise_lsa_decoder(version, lsa_decoder);
    initialise_packet_decoder(version, packet_decoder, lsa_decoder);

    uint8_t *ptr = &pkt[0];
    Packet *packet = packet_decoder.decode(ptr, pkt.size());

    // Try shortening the frame.
    try {
	packet = packet_decoder.decode(ptr, pkt.size() - 1);
	DOUT(info) << "Accepted short frame (bad)\n";
	return false;
    } catch(InvalidPacket& e) {
	DOUT(info) << "Rejected short frame (good): " << e.str() << endl;
    }

    // If we are going to feed in an oversize frame make sure that the
    // space is actually allocated. OpenBSD actually notices if the
    // space is not allocated.
    const int oversize = 64;
    vector<uint8_t> large_pkt;
    large_pkt.resize(pkt.size() + oversize);
    ptr = &large_pkt[0];
    memcpy(ptr, &pkt[0], pkt.size());

    // Make the frame longer.
    for (int i = 0; i < oversize; i++) {
	try {
	    packet = packet_decoder.decode(ptr, pkt.size() + i);
	    DOUT(info) << "Accepted large frame (good)\n";
	} catch(InvalidPacket& e) {
	    DOUT(info) << "Didn't accept large frame (bad): " << e.str() << 
		endl;
	    return false;
	}
	vector<uint8_t> pktn;
	packet->encode(pktn);
	if (!compare_packets(info, pkt, pktn)) {
	    DOUT(info) << " The frame is " << i << 
		" bytes larger decoded differently " << packet->str() << endl;
	    return false;
	}
    }

    DOUT(info) << packet->str() << endl;
    
    return true;
}

/**
 * Fill all the fields except for the 8 byte auth in a V2 hello packet.
 */
inline
void
populate_helloV2(HelloPacket *hello)
{
    hello->set_router_id(set_id("128.16.64.16"));
    hello->set_area_id(set_id("4.3.2.1"));
    hello->set_auth_type(5);

    hello->set_network_mask(0xffff0000);
    hello->set_hello_interval(9876);
    hello->set_options(compute_options(OspfTypes::V2, OspfTypes::NORMAL));
    hello->set_router_priority(42);
    hello->set_router_dead_interval(66000);
    hello->set_designated_router(set_id("1.2.3.4"));
    hello->set_backup_designated_router(set_id("2.4.6.8"));

    // Add some neighbours.
    hello->get_neighbours().push_back(set_id("10.11.12.13"));
    hello->get_neighbours().push_back(set_id("11.12.13.14"));
}

/**
 * Fill all the fields in a V3 hello packet.
 */
inline
void
populate_helloV3(HelloPacket *hello)
{
    hello->set_router_id(set_id("128.16.64.16"));
    hello->set_area_id(set_id("4.3.2.1"));
    hello->set_instance_id(5);

    hello->set_interface_id(12345678);
    hello->set_hello_interval(9876);
    hello->set_options(compute_options(OspfTypes::V3, OspfTypes::NORMAL));
    hello->set_router_priority(42);
    hello->set_router_dead_interval(6600);
    hello->set_designated_router(set_id("1.2.3.4"));
    hello->set_backup_designated_router(set_id("2.4.6.8"));

    // Add some neighbours.
    hello->get_neighbours().push_back(set_id("10.11.12.13"));
    hello->get_neighbours().push_back(set_id("11.12.13.14"));
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
    packet->set_router_id(set_id("128.16.64.16"));
    packet->set_area_id(set_id("4.3.2.1"));

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
populate_lsa_header(Lsa_header& header, OspfTypes::Version version)
{
    header.set_ls_age(500);
    switch(version) {
    case OspfTypes::V2:
	header.set_options(compute_options(version, OspfTypes::NORMAL));
	break;
    case OspfTypes::V3:
	break;
    }
    header.set_link_state_id(0x01020304);
    header.set_advertising_router(0x04030201);
    header.set_ls_sequence_number(0x0A0B0C0D);
    header.set_ls_checksum(0x1234);
    header.set_length(200);
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
    populate_lsa_header(header, version);
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

inline
void
populate_router_lsa(RouterLsa *rlsa, OspfTypes::Version version)
{
    populate_lsa_header(rlsa->get_header(), version);
    switch(version) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	rlsa->set_w_bit(true);
	rlsa->set_options(compute_options(version, OspfTypes::NORMAL));
	break;
    }
    
    RouterLink rl1(version);
    rl1.set_type(RouterLink::p2p);
    rl1.set_metric(2);
    switch(version) {
    case OspfTypes::V2:
	rl1.set_link_id(3);
	rl1.set_link_data(4);
	break;
    case OspfTypes::V3:
	rl1.set_interface_id(3);
	rl1.set_neighbour_interface_id(4);
	rl1.set_neighbour_router_id(5);
	break;
    }
    rlsa->get_router_links().push_back(rl1);

    RouterLink rl2(version);
    rl2.set_type(RouterLink::transit);
    rl2.set_metric(2);
    switch(version) {
    case OspfTypes::V2:
	rl2.set_link_id(3);
	rl2.set_link_data(4);
	break;
    case OspfTypes::V3:
	rl2.set_interface_id(3);
	rl2.set_neighbour_interface_id(4);
	rl2.set_neighbour_router_id(5);
	break;
    }
    rlsa->get_router_links().push_back(rl2);

    RouterLink rl3(version);
    switch(version) {
    case OspfTypes::V2:
	rl3.set_metric(2);
	rl3.set_type(RouterLink::stub);
	rl3.set_link_id(3);
	rl3.set_link_data(4);
	rlsa->get_router_links().push_back(rl3);
	break;
    case OspfTypes::V3:
	break;
    }

    RouterLink rl4(version);
    rl4.set_type(RouterLink::vlink);
    rl4.set_metric(2);
    switch(version) {
    case OspfTypes::V2:
	rl4.set_link_id(3);
	rl4.set_link_data(4);
	break;
    case OspfTypes::V3:
	rl4.set_interface_id(3);
	rl4.set_neighbour_interface_id(4);
	rl4.set_neighbour_router_id(5);
	break;
    }
    rlsa->get_router_links().push_back(rl4);

    // This will set the checksum and the length.
    rlsa->encode();
}

inline
void
populate_network_lsa(NetworkLsa *nlsa, OspfTypes::Version version)
{
    populate_lsa_header(nlsa->get_header(), version);
    switch(version) {
    case OspfTypes::V2:
	break;
    case OspfTypes::V3:
	nlsa->set_options(compute_options(version, OspfTypes::NORMAL));
	break;
    }
    
    switch(version) {
    case OspfTypes::V2:
	nlsa->set_network_mask(0xffff0000);
	break;
    case OspfTypes::V3:
	break;
    }
    nlsa->get_attached_routers().push_back(set_id("128.16.64.16"));
    nlsa->get_attached_routers().push_back(set_id("128.16.64.32"));

    // This will set the checksum and the length.
    nlsa->encode();
}

inline
void
populate_summary_network_lsa(SummaryNetworkLsa *snlsa,
			     OspfTypes::Version version)
{
    populate_lsa_header(snlsa->get_header(), version);

    snlsa->set_metric(5);
    
    switch(version) {
    case OspfTypes::V2:
	snlsa->set_network_mask(0xffff0000);
	break;
    case OspfTypes::V3:
	IPNet<IPv6> net("2001:468:e21:c800:220:edff:fe61:f033", 64);
	IPv6Prefix prefix(version);
	prefix.set_network(net);
	prefix.set_nu_bit(false);
	prefix.set_la_bit(false);
	prefix.set_mc_bit(false);
	prefix.set_p_bit(false);
	prefix.set_dn_bit(false);
	snlsa->set_ipv6prefix(prefix);
	break;
    }

    // This will set the checksum and the length.
    snlsa->encode();
}

inline
void
populate_summary_router_lsa(SummaryRouterLsa *srlsa,
			    OspfTypes::Version version)
{
    populate_lsa_header(srlsa->get_header(), version);

    srlsa->set_metric(5);
    
    switch(version) {
    case OspfTypes::V2:
	srlsa->set_network_mask(0xffff0000);
	break;
    case OspfTypes::V3:
	srlsa->set_options(compute_options(version, OspfTypes::NORMAL));
	srlsa->set_destination_id(set_id("128.16.64.32"));
	break;
    }

    // This will set the checksum and the length.
    srlsa->encode();
}

inline
void
populate_as_external_lsa(ASExternalLsa *aelsa,
			 OspfTypes::Version version)
{
    populate_lsa_header(aelsa->get_header(), version);

    aelsa->set_metric(OspfTypes::LSInfinity);
    
    switch(version) {
    case OspfTypes::V2:
	aelsa->set_network_mask(0xffff0000);
	aelsa->set_e_bit(true);
	aelsa->set_forwarding_address_ipv4(IPv4("10.10.10.10"));
	break;
    case OspfTypes::V3:
	aelsa->set_e_bit(true);
	aelsa->set_f_bit(true);
	aelsa->set_t_bit(true);
	aelsa->set_referenced_ls_type(2);
	IPNet<IPv6> net("2001:468:e21:c800:220:edff:fe61:f033", 64);
	IPv6Prefix prefix(version);
	prefix.set_network(net);
	prefix.set_nu_bit(true);
	prefix.set_la_bit(true);
	prefix.set_mc_bit(true);
	prefix.set_p_bit(true);
	prefix.set_dn_bit(true);
	aelsa->set_ipv6prefix(prefix);
	aelsa->set_forwarding_address_ipv6(
			      IPv6("2001:468:e21:c800:220:edff:fe61:f033"));
	aelsa->set_referenced_link_state_id(0x10);
	break;
    }

    aelsa->set_external_route_tag(0x42);

    // This will set the checksum and the length.
    aelsa->encode();
}

inline
void
populate_link_lsa(LinkLsa *llsa, OspfTypes::Version version)
{
    populate_lsa_header(llsa->get_header(), version);

    llsa->set_rtr_priority(42);
    llsa->set_options(compute_options(version, OspfTypes::NORMAL));
    llsa->set_link_local_address(IPv6("fe80::202:b3ff:fe19:be47"));
    IPNet<IPv6> net1("2001:468:e21:c800:220:edff:fe61:f033", 64);
    IPv6Prefix prefix1(version);
    prefix1.set_network(net1);
    prefix1.set_nu_bit(true);
    prefix1.set_la_bit(true);
    prefix1.set_mc_bit(true);
    prefix1.set_p_bit(true);
    prefix1.set_dn_bit(true);
    llsa->get_prefixes().push_back(prefix1);
    IPNet<IPv6> net2("2001:700:0:fff1::2", 64);
    IPv6Prefix prefix2(version);
    prefix2.set_network(net2);
    prefix2.set_nu_bit(false);
    prefix2.set_la_bit(false);
    prefix2.set_mc_bit(false);
    prefix2.set_p_bit(false);
    prefix2.set_dn_bit(false);
    llsa->get_prefixes().push_back(prefix2);
}

inline
void
populate_intra_area_prefix_lsa(IntraAreaPrefixLsa *iaplsa,
			       OspfTypes::Version version)
{
    populate_lsa_header(iaplsa->get_header(), version);

    iaplsa->set_referenced_ls_type(0x2001);
    iaplsa->set_referenced_link_state_id(1);
    iaplsa->set_referenced_advertising_router(set_id("192.1.1.2"));
    IPNet<IPv6> net1("2001:468:e21:c800:220:edff:fe61:f033", 64);
    IPv6Prefix prefix1(version, true);
    prefix1.set_network(net1);
    prefix1.set_metric(256);
    prefix1.set_nu_bit(true);
    prefix1.set_la_bit(true);
    prefix1.set_mc_bit(true);
    prefix1.set_p_bit(true);
    prefix1.set_dn_bit(true);
    iaplsa->get_prefixes().push_back(prefix1);
    IPNet<IPv6> net2("2001:700:0:fff1::2", 64);
    IPv6Prefix prefix2(version, true);
    prefix2.set_network(net2);
    prefix2.set_metric(3841);
    prefix2.set_nu_bit(false);
    prefix2.set_la_bit(false);
    prefix2.set_mc_bit(false);
    prefix2.set_p_bit(false);
    prefix2.set_dn_bit(false);
    iaplsa->get_prefixes().push_back(prefix2);
}

inline
void
populate_link_state_update(LinkStateUpdatePacket *lsup,
			   OspfTypes::Version version)
{
    populate_standard_header(lsup, version);

    RouterLsa *rlsa = new RouterLsa(version);
    populate_router_lsa(rlsa, version);
    lsup->get_lsas().push_back(Lsa::LsaRef(rlsa));

    rlsa = new RouterLsa(version);
    populate_router_lsa(rlsa, version);
    lsup->get_lsas().push_back(Lsa::LsaRef(rlsa));

    NetworkLsa *nlsa = new NetworkLsa(version);
    populate_network_lsa(nlsa, version);
    lsup->get_lsas().push_back(Lsa::LsaRef(nlsa));

    nlsa = new NetworkLsa(version);
    populate_network_lsa(nlsa, version);
    lsup->get_lsas().push_back(Lsa::LsaRef(nlsa));

    SummaryNetworkLsa *snlsa = new SummaryNetworkLsa(version);
    populate_summary_network_lsa(snlsa, version);
    lsup->get_lsas().push_back(Lsa::LsaRef(snlsa));

    snlsa = new SummaryNetworkLsa(version);
    populate_summary_network_lsa(snlsa, version);
    lsup->get_lsas().push_back(Lsa::LsaRef(snlsa));

    SummaryRouterLsa *srlsa = new SummaryRouterLsa(version);
    populate_summary_router_lsa(srlsa, version);
    lsup->get_lsas().push_back(Lsa::LsaRef(srlsa));

    srlsa = new SummaryRouterLsa(version);
    populate_summary_router_lsa(srlsa, version);
    lsup->get_lsas().push_back(Lsa::LsaRef(srlsa));

    ASExternalLsa *aelsa = new ASExternalLsa(version);
    populate_as_external_lsa(aelsa, version);
    lsup->get_lsas().push_back(Lsa::LsaRef(aelsa));

    aelsa = new ASExternalLsa(version);
    populate_as_external_lsa(aelsa, version);
    lsup->get_lsas().push_back(Lsa::LsaRef(aelsa));
}

inline
void
populate_link_state_acknowledgement(LinkStateAcknowledgementPacket *lsack,
				    OspfTypes::Version version)
{
    populate_standard_header(lsack, version);

    // Create a LSA Header to add
    Lsa_header header(version);
    populate_lsa_header(header, version);
    lsack->get_lsa_headers().push_back(header);
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
    
    if (!compare_packets(info, pkt1, pkt2))
	return false;

    if (!compare_packet_strings(info, hello1->str(), hello3->str()))
	return false;

    delete hello1;
    delete hello2;
    delete hello3;

    return true;
}

bool
hello_packet_bad_length(TestInfo& info, OspfTypes::Version version)
{
    HelloPacket *packet = new HelloPacket(version);
    populate_hello(packet, version);

    DOUT(info) << packet->str() << endl;

    // Encode the hello packet.
    vector<uint8_t> pkt;
    packet->encode(pkt);

    delete packet;

    return packet_bad_length(info, version, pkt);
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
    
    if (!compare_packets(info, pkt1, pkt2))
	return false;

    if (!compare_packet_strings(info, ddp1->str(), ddp3->str()))
	return false;

    delete ddp1;
    delete ddp2;
    delete ddp3;

    return true;
}

bool
data_description_packet_bad_length(TestInfo& info, OspfTypes::Version version)
{
    DataDescriptionPacket *packet = new DataDescriptionPacket(version);
    populate_data_description(packet, version);

    DOUT(info) << packet->str() << endl;

    // Encode the hello packet.
    vector<uint8_t> pkt;
    packet->encode(pkt);

    delete packet;

    return packet_bad_length(info, version, pkt);
}

bool
link_state_update_packet_print(TestInfo& info)
{
    LsaDecoder lsa_decoder(OspfTypes::V2);
    LinkStateUpdatePacket *lsup;

    lsup = new LinkStateUpdatePacket(OspfTypes::V2, lsa_decoder);
    populate_link_state_update(lsup, OspfTypes::V2);

    DOUT(info) << lsup->str() << endl;

    delete lsup;

    lsup = new LinkStateUpdatePacket(OspfTypes::V3, lsa_decoder);
    populate_link_state_update(lsup, OspfTypes::V3);

    DOUT(info) << lsup->str() << endl;

    delete lsup;

    return true;
}

bool
link_state_update_packet_compare(TestInfo& info, OspfTypes::Version version)
{
    LsaDecoder lsa_decoder(version);
    initialise_lsa_decoder(version, lsa_decoder);

    LinkStateUpdatePacket *lsup1;
    lsup1 = new LinkStateUpdatePacket(version, lsa_decoder);
    populate_link_state_update(lsup1, version);

    DOUT(info) << lsup1->str() << endl;

    // Encode the Link State Update Packet.
    vector<uint8_t> pkt1;
    lsup1->encode(pkt1);

    // Now decode the packet.
    // Create a new packet to provide the decoder.
    LinkStateUpdatePacket *lsup2;
    lsup2 = new LinkStateUpdatePacket(version, lsa_decoder);

    LinkStateUpdatePacket *lsup3 =
	dynamic_cast<LinkStateUpdatePacket *>(lsup2->
					       decode(&pkt1[0], pkt1.size()));

    DOUT(info) << lsup3->str() << endl;

    // Encode the second packet and compare.
    vector<uint8_t> pkt2;
    lsup3->encode(pkt2);
    
    if (!compare_packets(info, pkt1, pkt2))
	return false;

    if (!compare_packet_strings(info, lsup1->str(), lsup3->str()))
	return false;

    delete lsup1;
    delete lsup2;
    delete lsup3;

    return true;
}

bool
link_state_update_packet_bad_length(TestInfo& info, OspfTypes::Version version)
{
    LsaDecoder lsa_decoder(OspfTypes::V2);
    
    LinkStateUpdatePacket *packet = new LinkStateUpdatePacket(version,
							      lsa_decoder);
    populate_link_state_update(packet, version);

    DOUT(info) << packet->str() << endl;

    // Encode the hello packet.
    vector<uint8_t> pkt;
    packet->encode(pkt);

    delete packet;

    return packet_bad_length(info, version, pkt);
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
    
    if (!compare_packets(info, pkt1, pkt2))
	return false;

    if (!compare_packet_strings(info, lsrp1->str(), lsrp3->str()))
	return false;

    delete lsrp1;
    delete lsrp2;
    delete lsrp3;

    return true;
}

bool
link_state_request_packet_bad_length(TestInfo& info,
				     OspfTypes::Version version)
{
    LinkStateRequestPacket *packet = new LinkStateRequestPacket(version);
    populate_link_state_request(packet, version);

    DOUT(info) << packet->str() << endl;

    // Encode the hello packet.
    vector<uint8_t> pkt;
    packet->encode(pkt);

    delete packet;

    return packet_bad_length(info, version, pkt);
}

bool
link_state_acknowledgement_packet_print(TestInfo& info)
{
    LinkStateAcknowledgementPacket *lsrp = 
	new LinkStateAcknowledgementPacket(OspfTypes::V2);
    populate_link_state_acknowledgement(lsrp, OspfTypes::V2);

    DOUT(info) << lsrp->str() << endl;

    delete lsrp;

    lsrp = new LinkStateAcknowledgementPacket(OspfTypes::V3);
    populate_link_state_acknowledgement(lsrp, OspfTypes::V3);

    DOUT(info) << lsrp->str() << endl;

    delete lsrp;

    return true;
}

bool
link_state_acknowledgement_packet_compare(TestInfo& info,
					  OspfTypes::Version version)
{
    LinkStateAcknowledgementPacket *lsrp1 = 
	new LinkStateAcknowledgementPacket(version);
    populate_link_state_acknowledgement(lsrp1, version);

    DOUT(info) << lsrp1->str() << endl;

    // Encode the Link State Acknowledgement Packet.
    vector<uint8_t> pkt1;
    lsrp1->encode(pkt1);

    // Now decode the packet.
    // Create a new packet to provide the decoder.
    LinkStateAcknowledgementPacket *lsrp2 = 
	new LinkStateAcknowledgementPacket(version);

    LinkStateAcknowledgementPacket *lsrp3 =
	dynamic_cast<LinkStateAcknowledgementPacket *>(lsrp2->
					       decode(&pkt1[0], pkt1.size()));

    DOUT(info) << lsrp3->str() << endl;

    // Encode the second packet and compare.
    vector<uint8_t> pkt2;
    lsrp3->encode(pkt2);
    
    if (!compare_packets(info, pkt1, pkt2))
	return false;

    if (!compare_packet_strings(info, lsrp1->str(), lsrp3->str()))
	return false;

    delete lsrp1;
    delete lsrp2;
    delete lsrp3;

    return true;
}

bool
link_state_acknowledgement_packet_bad_length(TestInfo& info,
					     OspfTypes::Version version)
{
    LinkStateAcknowledgementPacket *packet = 
	new LinkStateAcknowledgementPacket(version);
    populate_link_state_acknowledgement(packet, version);

    DOUT(info) << packet->str() << endl;

    // Encode the hello packet.
    vector<uint8_t> pkt;
    packet->encode(pkt);

    delete packet;

    return packet_bad_length(info, version, pkt);
}

bool
packet_decoder1(TestInfo& info, OspfTypes::Version version)
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
    } catch(InvalidPacket& e) {
	DOUT(info) << "Caught exception: " << e.str() << endl;
	return true;
    }

    return false;
}

bool
packet_decoder2(TestInfo& info, OspfTypes::Version version)
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

uint8_t bad_packet1[] = {
#include "packet1.data"
};

uint8_t bad_packet2[] = {
#include "packet2.data"
};

uint8_t bad_packet3[] = {
#include "packet3.data"
};

bool
packet_decode_bad_packet(TestInfo& info, OspfTypes::Version version,
			 uint8_t *ptr, size_t len)
{
    PacketDecoder packet_decoder;
    LsaDecoder lsa_decoder(version);

    initialise_lsa_decoder(version, lsa_decoder);
    initialise_packet_decoder(version, packet_decoder, lsa_decoder);

    Packet *packet;
    try {
	packet = packet_decoder.decode(ptr, len);
	DOUT(info) << "Accepted bad packet (bad)\n";
	return false;
    } catch(InvalidPacket& e) {
	DOUT(info) << "Rejected bad packet (good): " << e.str() << endl;
	return true;
    }
    
    XLOG_UNREACHABLE();

    return false;
}

/* Packet stuff above - LSA stuff below */

inline
void
fill_vector(vector<uint8_t>& pkt, uint8_t *ptr, size_t len)
{
    pkt.resize(len);
    for(size_t i = 0; i < len; i++)
	pkt[i] = ptr[i];
}

bool
router_lsa_print(TestInfo& info)
{
    RouterLsa *rlsa = new RouterLsa(OspfTypes::V2);
    populate_router_lsa(rlsa, OspfTypes::V2);

    DOUT(info) << rlsa->str() << endl;

    delete rlsa;

    rlsa = new RouterLsa(OspfTypes::V3);
    populate_router_lsa(rlsa, OspfTypes::V3);

    DOUT(info) << rlsa->str() << endl;
    XLOG_ASSERT(rlsa->known());
    XLOG_ASSERT(!rlsa->link_local_scope());
    XLOG_ASSERT(rlsa->area_scope());
    XLOG_ASSERT(!rlsa->as_scope());

    delete rlsa;

    return true;
}

bool
network_lsa_print(TestInfo& info)
{
    NetworkLsa *nlsa = new NetworkLsa(OspfTypes::V2);
    populate_network_lsa(nlsa, OspfTypes::V2);

    DOUT(info) << nlsa->str() << endl;

    delete nlsa;

    nlsa = new NetworkLsa(OspfTypes::V3);
    populate_network_lsa(nlsa, OspfTypes::V3);

    DOUT(info) << nlsa->str() << endl;
    XLOG_ASSERT(nlsa->known());
    XLOG_ASSERT(!nlsa->link_local_scope());
    XLOG_ASSERT(nlsa->area_scope());
    XLOG_ASSERT(!nlsa->as_scope());

    delete nlsa;

    return true;
}

bool
summary_network_lsa_print(TestInfo& info)
{
    SummaryNetworkLsa *snlsa = new SummaryNetworkLsa(OspfTypes::V2);
    populate_summary_network_lsa(snlsa, OspfTypes::V2);

    DOUT(info) << snlsa->str() << endl;

    delete snlsa;

    snlsa = new SummaryNetworkLsa(OspfTypes::V3);
    populate_summary_network_lsa(snlsa, OspfTypes::V3);

    DOUT(info) << snlsa->str() << endl;
    XLOG_ASSERT(snlsa->known());
    XLOG_ASSERT(!snlsa->link_local_scope());
    XLOG_ASSERT(snlsa->area_scope());
    XLOG_ASSERT(!snlsa->as_scope());

    delete snlsa;

    return true;
}

bool
summary_router_lsa_print(TestInfo& info)
{
    SummaryRouterLsa *srlsa = new SummaryRouterLsa(OspfTypes::V2);
    populate_summary_router_lsa(srlsa, OspfTypes::V2);

    DOUT(info) << srlsa->str() << endl;

    delete srlsa;

    srlsa = new SummaryRouterLsa(OspfTypes::V3);
    populate_summary_router_lsa(srlsa, OspfTypes::V3);

    DOUT(info) << srlsa->str() << endl;
    XLOG_ASSERT(srlsa->known());
    XLOG_ASSERT(!srlsa->link_local_scope());
    XLOG_ASSERT(srlsa->area_scope());
    XLOG_ASSERT(!srlsa->as_scope());

    delete srlsa;

    return true;
}

bool
as_external_lsa_print(TestInfo& info)
{
    ASExternalLsa *aelsa = new ASExternalLsa(OspfTypes::V2);
    populate_as_external_lsa(aelsa, OspfTypes::V2);

    DOUT(info) << aelsa->str() << endl;

    delete aelsa;

    aelsa = new ASExternalLsa(OspfTypes::V3);
    populate_as_external_lsa(aelsa, OspfTypes::V3);

    DOUT(info) << aelsa->str() << endl;
    XLOG_ASSERT(aelsa->known());
    XLOG_ASSERT(!aelsa->link_local_scope());
    XLOG_ASSERT(!aelsa->area_scope());
    XLOG_ASSERT(aelsa->as_scope());

    delete aelsa;

    return true;
}

bool
type7_lsa_print(TestInfo& info)
{
    Type7Lsa *type7 = new Type7Lsa(OspfTypes::V2);
    populate_as_external_lsa(type7, OspfTypes::V2);

    DOUT(info) << type7->str() << endl;

    delete type7;

    type7 = new Type7Lsa(OspfTypes::V3);
    populate_as_external_lsa(type7, OspfTypes::V3);

    DOUT(info) << type7->str() << endl;
    XLOG_ASSERT(type7->known());
    XLOG_ASSERT(!type7->link_local_scope());
    XLOG_ASSERT(type7->area_scope());
    XLOG_ASSERT(!type7->as_scope());

    delete type7;

    return true;
}

bool
link_lsa_print(TestInfo& info)
{
    LinkLsa *llsa = new LinkLsa(OspfTypes::V3);
    populate_link_lsa(llsa, OspfTypes::V3);

    DOUT(info) << llsa->str() << endl;
    XLOG_ASSERT(llsa->known());
    XLOG_ASSERT(llsa->link_local_scope());
    XLOG_ASSERT(!llsa->area_scope());
    XLOG_ASSERT(!llsa->as_scope());

    delete llsa;

    return true;
}

bool
intra_area_prefix_lsa_print(TestInfo& info)
{
    IntraAreaPrefixLsa *iaplsa = new IntraAreaPrefixLsa(OspfTypes::V3);
    populate_intra_area_prefix_lsa(iaplsa, OspfTypes::V3);

    DOUT(info) << iaplsa->str() << endl;

    delete iaplsa;

    return true;
}

bool
router_lsa_compare(TestInfo& info, OspfTypes::Version version)
{
    RouterLsa *rlsa1= new RouterLsa(version);
    populate_router_lsa(rlsa1, version);

    DOUT(info) << rlsa1->str() << endl;

    // Encode the Router-LSA.
    rlsa1->encode();
    size_t len1;
    uint8_t *ptr1 = rlsa1->lsa(len1);

    // Now decode the packet.
    // Create a new packet to provide the decoder.
    RouterLsa *rlsa2= new RouterLsa(version);

    Lsa::LsaRef rlsa3 = rlsa2->decode(ptr1, len1);

    DOUT(info) << rlsa3->str() << endl;

    // Encode the second packet and compare.
    rlsa3->encode();

    DOUT(info) << rlsa3->str() << endl;

    size_t len2;
    uint8_t *ptr2 = rlsa3->lsa(len2);
    
    vector<uint8_t> pkt1;
    fill_vector(pkt1, ptr1, len1);
    vector<uint8_t> pkt2;
    fill_vector(pkt2, ptr2, len2);

    if (!compare_packets(info, pkt1, pkt2))
	return false;

    if (!compare_packet_strings(info, rlsa1->str(), rlsa3->str()))
	return false;

    delete rlsa1;
    delete rlsa2;

    return true;
}

bool
network_lsa_compare(TestInfo& info, OspfTypes::Version version)
{
    NetworkLsa *nlsa1= new NetworkLsa(version);
    populate_network_lsa(nlsa1, version);

    DOUT(info) << nlsa1->str() << endl;

    // Encode the Network-LSA.
    nlsa1->encode();
    size_t len1;
    uint8_t *ptr1 = nlsa1->lsa(len1);

    // Now decode the packet.
    // Create a new packet to provide the decoder.
    NetworkLsa *nlsa2= new NetworkLsa(version);

    Lsa::LsaRef nlsa3 = nlsa2->decode(ptr1, len1);

    DOUT(info) << nlsa3->str() << endl;

    // Encode the second packet and compare.
    nlsa3->encode();

    DOUT(info) << nlsa3->str() << endl;

    size_t len2;
    uint8_t *ptr2 = nlsa3->lsa(len2);
    
    vector<uint8_t> pkt1;
    fill_vector(pkt1, ptr1, len1);
    vector<uint8_t> pkt2;
    fill_vector(pkt2, ptr2, len2);

    if (!compare_packets(info, pkt1, pkt2))
	return false;

    if (!compare_packet_strings(info, nlsa1->str(), nlsa3->str()))
	return false;

    delete nlsa1;
    delete nlsa2;

    return true;
}

bool
summary_network_lsa_compare(TestInfo& info, OspfTypes::Version version)
{
    SummaryNetworkLsa *snlsa1= new SummaryNetworkLsa(version);
    populate_summary_network_lsa(snlsa1, version);

    DOUT(info) << snlsa1->str() << endl;

    // Encode the SummaryNetwork-LSA.
    snlsa1->encode();
    size_t len1;
    uint8_t *ptr1 = snlsa1->lsa(len1);

    // Now decode the packet.
    // Create a new packet to provide the decoder.
    SummaryNetworkLsa *snlsa2= new SummaryNetworkLsa(version);

    Lsa::LsaRef snlsa3 = snlsa2->decode(ptr1, len1);

    DOUT(info) << snlsa3->str() << endl;

    // Encode the second packet and compare.
    snlsa3->encode();

    DOUT(info) << snlsa3->str() << endl;

    size_t len2;
    uint8_t *ptr2 = snlsa3->lsa(len2);
    
    vector<uint8_t> pkt1;
    fill_vector(pkt1, ptr1, len1);
    vector<uint8_t> pkt2;
    fill_vector(pkt2, ptr2, len2);

    if (!compare_packets(info, pkt1, pkt2))
	return false;

    if (!compare_packet_strings(info, snlsa1->str(), snlsa3->str()))
	return false;

    delete snlsa1;
    delete snlsa2;

    return true;
}

bool
summary_router_lsa_compare(TestInfo& info, OspfTypes::Version version)
{
    SummaryRouterLsa *srlsa1= new SummaryRouterLsa(version);
    populate_summary_router_lsa(srlsa1, version);

    DOUT(info) << srlsa1->str() << endl;

    // Encode the SummaryRouter-LSA.
    srlsa1->encode();
    size_t len1;
    uint8_t *ptr1 = srlsa1->lsa(len1);

    // Now decode the packet.
    // Create a new packet to provide the decoder.
    SummaryRouterLsa *srlsa2= new SummaryRouterLsa(version);

    Lsa::LsaRef srlsa3 = srlsa2->decode(ptr1, len1);

    DOUT(info) << srlsa3->str() << endl;

    // Encode the second packet and compare.
    srlsa3->encode();

    DOUT(info) << srlsa3->str() << endl;

    size_t len2;
    uint8_t *ptr2 = srlsa3->lsa(len2);
    
    vector<uint8_t> pkt1;
    fill_vector(pkt1, ptr1, len1);
    vector<uint8_t> pkt2;
    fill_vector(pkt2, ptr2, len2);

    if (!compare_packets(info, pkt1, pkt2))
	return false;

    if (!compare_packet_strings(info, srlsa1->str(), srlsa3->str()))
	return false;

    delete srlsa1;
    delete srlsa2;

    return true;
}

bool
as_external_lsa_compare(TestInfo& info, OspfTypes::Version version)
{
    ASExternalLsa *aelsa1= new ASExternalLsa(version);
    populate_as_external_lsa(aelsa1, version);

    DOUT(info) << aelsa1->str() << endl;

    // Encode the AS-External-LSA.
    aelsa1->encode();
    size_t len1;
    uint8_t *ptr1 = aelsa1->lsa(len1);

    // Now decode the packet.
    // Create a new packet to provide the decoder.
    ASExternalLsa *aelsa2= new ASExternalLsa(version);

    Lsa::LsaRef aelsa3 = aelsa2->decode(ptr1, len1);

    DOUT(info) << aelsa3->str() << endl;

    // Encode the second packet and compare.
    aelsa3->encode();

    DOUT(info) << aelsa3->str() << endl;

    size_t len2;
    uint8_t *ptr2 = aelsa3->lsa(len2);
    
    vector<uint8_t> pkt1;
    fill_vector(pkt1, ptr1, len1);
    vector<uint8_t> pkt2;
    fill_vector(pkt2, ptr2, len2);

    if (!compare_packets(info, pkt1, pkt2))
	return false;

    if (!compare_packet_strings(info, aelsa1->str(), aelsa3->str()))
	return false;

    delete aelsa1;
    delete aelsa2;

    return true;
}

bool
type7_lsa_compare(TestInfo& info, OspfTypes::Version version)
{
    Type7Lsa *type71= new Type7Lsa(version);
    populate_as_external_lsa(type71, version);

    DOUT(info) << type71->str() << endl;

    // Encode the Type7-LSA.
    type71->encode();
    size_t len1;
    uint8_t *ptr1 = type71->lsa(len1);

    // Now decode the packet.
    // Create a new packet to provide the decoder.
    Type7Lsa *type72= new Type7Lsa(version);

    Lsa::LsaRef type73 = type72->decode(ptr1, len1);

    DOUT(info) << type73->str() << endl;

    // Encode the second packet and compare.
    type73->encode();

    DOUT(info) << type73->str() << endl;

    size_t len2;
    uint8_t *ptr2 = type73->lsa(len2);
    
    vector<uint8_t> pkt1;
    fill_vector(pkt1, ptr1, len1);
    vector<uint8_t> pkt2;
    fill_vector(pkt2, ptr2, len2);

    if (!compare_packets(info, pkt1, pkt2))
	return false;

    if (!compare_packet_strings(info, type71->str(), type73->str()))
	return false;

    delete type71;
    delete type72;

    return true;
}

bool
link_lsa_compare(TestInfo& info, OspfTypes::Version version)
{
    LinkLsa *llsa1= new LinkLsa(version);
    populate_link_lsa(llsa1, version);

    DOUT(info) << llsa1->str() << endl;

    // Encode the Link-LSA.
    llsa1->encode();
    size_t len1;
    uint8_t *ptr1 = llsa1->lsa(len1);

    // Now decode the packet.
    // Create a new packet to provide the decoder.
    LinkLsa *llsa2= new LinkLsa(version);

    Lsa::LsaRef llsa3 = llsa2->decode(ptr1, len1);

    DOUT(info) << llsa3->str() << endl;

    // Encode the second packet and compare.
    llsa3->encode();

    DOUT(info) << llsa3->str() << endl;

    size_t len2;
    uint8_t *ptr2 = llsa3->lsa(len2);
    
    vector<uint8_t> pkt1;
    fill_vector(pkt1, ptr1, len1);
    vector<uint8_t> pkt2;
    fill_vector(pkt2, ptr2, len2);

    if (!compare_packets(info, pkt1, pkt2))
	return false;

    if (!compare_packet_strings(info, llsa1->str(), llsa3->str()))
	return false;

    delete llsa1;
    delete llsa2;

    return true;
}

bool
intra_area_prefix_lsa_compare(TestInfo& info, OspfTypes::Version version)
{
    IntraAreaPrefixLsa *iaplsa1= new IntraAreaPrefixLsa(version);
    populate_intra_area_prefix_lsa(iaplsa1, version);

    DOUT(info) << iaplsa1->str() << endl;

    // Encode the Link-LSA.
    iaplsa1->encode();
    size_t len1;
    uint8_t *ptr1 = iaplsa1->lsa(len1);

    // Now decode the packet.
    // Create a new packet to provide the decoder.
    IntraAreaPrefixLsa *iaplsa2= new IntraAreaPrefixLsa(version);

    Lsa::LsaRef iaplsa3 = iaplsa2->decode(ptr1, len1);

    DOUT(info) << iaplsa3->str() << endl;

    // Encode the second packet and compare.
    iaplsa3->encode();

    DOUT(info) << iaplsa3->str() << endl;

    size_t len2;
    uint8_t *ptr2 = iaplsa3->lsa(len2);
    
    vector<uint8_t> pkt1;
    fill_vector(pkt1, ptr1, len1);
    vector<uint8_t> pkt2;
    fill_vector(pkt2, ptr2, len2);

    if (!compare_packets(info, pkt1, pkt2))
	return false;

    if (!compare_packet_strings(info, iaplsa1->str(), iaplsa3->str()))
	return false;

    delete iaplsa1;
    delete iaplsa2;

    return true;
}

bool
lsa_decoder1(TestInfo& info, OspfTypes::Version version)
{
    LsaDecoder dec(version);
    RouterLsa router_lsa(version);
    populate_router_lsa(&router_lsa, version);

    router_lsa.encode();
    size_t len;
    uint8_t *ptr = router_lsa.lsa(len);

    // An attempt to decode a packet with no decoders installed should
    // fail.
    try {
	dec.decode(ptr, len);
    } catch(InvalidPacket& e) {
	DOUT(info) << "Caught exception: " << e.str() << endl;
	return true;
    }

    return false;
}

bool
lsa_decoder2(TestInfo& info, OspfTypes::Version version)
{
    LsaDecoder dec(version);
    // Install a decoder for router LSAs.
    RouterLsa *router_lsa_decoder = new RouterLsa(version);
    dec.register_decoder(router_lsa_decoder);

    RouterLsa router_lsa(version);
    populate_router_lsa(&router_lsa, version);
    router_lsa.encode();
    size_t len;
    uint8_t *ptr = router_lsa.lsa(len);

    // An attempt to decode a LSA with a decoder installed should succeed.
    Lsa::LsaRef lsaref = dec.decode(ptr, len);

    DOUT(info) << lsaref->str() << endl;

    return true;
}

bool
lsa_decoder3(TestInfo& info, OspfTypes::Version version)
{
    LsaDecoder dec(version);
    // Install the unknown LSA decoder.
    UnknownLsa *unknown_lsa_decoder = new UnknownLsa(version);
    dec.register_unknown_decoder(unknown_lsa_decoder);

    RouterLsa router_lsa(version);
    populate_router_lsa(&router_lsa, version);
    router_lsa.encode();
    size_t len;
    uint8_t *ptr = router_lsa.lsa(len);

    // The only decoder installed is the unknown decoder which should
    // be able to handle the Router-LSA.
    Lsa::LsaRef lsaref = dec.decode(ptr, len);

    DOUT(info) << lsaref->str() << endl;

    return true;
}

int
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

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
	{"link_state_update_print", callback(link_state_update_packet_print)},
	{"link_state_request_print",callback(link_state_request_packet_print)},
	{"link_state_acknowledgement_print",
	 callback(link_state_acknowledgement_packet_print)},
	{"router_lsa_print", callback(router_lsa_print)},
	{"network_lsa_print", callback(network_lsa_print)},
	{"summary_network_lsa_print", callback(summary_network_lsa_print)},
	{"summary_router_lsa_print", callback(summary_router_lsa_print)},
	{"as_external_lsa_print", callback(as_external_lsa_print)},
	{"type7_lsa_print", callback(type7_lsa_print)},
	{"link_lsa_print", callback(link_lsa_print)},
	{"intra_area_prefix_lsa_print", callback(intra_area_prefix_lsa_print)},

	{"hello_compareV2", callback(hello_packet_compare, OspfTypes::V2)},
	{"hello_compareV3", callback(hello_packet_compare, OspfTypes::V3)},
	{"hello_badlenV2", callback(hello_packet_bad_length, OspfTypes::V2)},
	{"hello_badlenV3", callback(hello_packet_bad_length, OspfTypes::V2)},

	{"ddp_compareV2", callback(data_description_packet_compare,
				   OspfTypes::V2)},
	{"ddp_compareV3", callback(data_description_packet_compare,
				   OspfTypes::V3)},
	{"ddp_badlenV2", callback(data_description_packet_bad_length,
				   OspfTypes::V2)},
	{"ddp_badlenV3", callback(data_description_packet_bad_length,
				   OspfTypes::V3)},

	{"lsup_compareV2", callback(link_state_update_packet_compare,
				   OspfTypes::V2)},
	{"lsup_compareV3", callback(link_state_update_packet_compare,
				   OspfTypes::V3)},
	{"lsup_badlenV2", callback(link_state_update_packet_bad_length,
				   OspfTypes::V2)},
	{"lsup_badlenV3", callback(link_state_update_packet_bad_length,
				   OspfTypes::V3)},

	{"lsrp_compareV2", callback(link_state_request_packet_compare,
				   OspfTypes::V2)},
	{"lsrp_compareV3", callback(link_state_request_packet_compare,
				   OspfTypes::V3)},
	{"lsrp_badlenV2", callback(link_state_request_packet_bad_length,
				   OspfTypes::V2)},
	{"lsrp_badlenV3", callback(link_state_request_packet_bad_length,
				   OspfTypes::V3)},

	{"lsap_compareV2", callback(link_state_acknowledgement_packet_compare,
				   OspfTypes::V2)},
	{"lsap_compareV3", callback(link_state_acknowledgement_packet_compare,
				   OspfTypes::V3)},
	{"lsap_badlenV2",callback(link_state_acknowledgement_packet_bad_length,
				   OspfTypes::V2)},
	{"lsap_badlenV3",callback(link_state_acknowledgement_packet_bad_length,
				   OspfTypes::V3)},

	{"packet_decoder1V2", callback(packet_decoder1, OspfTypes::V2)},
	{"packet_decoder1V3", callback(packet_decoder1, OspfTypes::V3)},
	{"packet_decoder2V2", callback(packet_decoder2, OspfTypes::V2)},
	{"packet_decoder2V3", callback(packet_decoder2, OspfTypes::V3)},

	{"packet_decode_bad1V2", callback(packet_decode_bad_packet,
					  OspfTypes::V2,
					  bad_packet1, sizeof(bad_packet1))},
	{"packet_decode_bad2V2", callback(packet_decode_bad_packet,
					  OspfTypes::V2,
					  bad_packet2, sizeof(bad_packet2))},
	{"packet_decode_bad3V2", callback(packet_decode_bad_packet,
					  OspfTypes::V2,
					  bad_packet3, sizeof(bad_packet3))},

	{"router_lsa_compareV2", callback(router_lsa_compare, OspfTypes::V2)},
	{"router_lsa_compareV3", callback(router_lsa_compare, OspfTypes::V3)},
	{"network_lsa_compareV2",callback(network_lsa_compare, OspfTypes::V2)},
	{"network_lsa_compareV3",callback(network_lsa_compare, OspfTypes::V3)},
	{"summary_network_lsa_compareV2",
	 callback(summary_network_lsa_compare, OspfTypes::V2)},
	{"summary_network_lsa_compareV3",
	 callback(summary_network_lsa_compare, OspfTypes::V3)},
	{"summary_router_lsa_compareV2",
	 callback(summary_router_lsa_compare, OspfTypes::V2)},
	{"summary_router_lsa_compareV3",
	 callback(summary_router_lsa_compare, OspfTypes::V3)},
	{"as_external_lsa_compareV2",
	 callback(as_external_lsa_compare, OspfTypes::V2)},
	{"as_external_lsa_compareV3",
	 callback(as_external_lsa_compare, OspfTypes::V3)},
	{"type7_lsa_compareV2",	 callback(type7_lsa_compare, OspfTypes::V2)},
	{"type7_lsa_compareV3",	 callback(type7_lsa_compare, OspfTypes::V3)},
	{"link_lsa_compareV3",	 callback(link_lsa_compare, OspfTypes::V3)},
	{"intra_area_prefix_lsa_compareV3", 
	 callback(intra_area_prefix_lsa_compare, OspfTypes::V3)},

	{"lsa_decoder1V2", callback(lsa_decoder1, OspfTypes::V2)},
	{"lsa_decoder1V3", callback(lsa_decoder1, OspfTypes::V3)},
	{"lsa_decoder2V2", callback(lsa_decoder2, OspfTypes::V2)},
	{"lsa_decoder2V3", callback(lsa_decoder2, OspfTypes::V3)},
	{"lsa_decoder3V3", callback(lsa_decoder3, OspfTypes::V3)},
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

    xlog_stop();
    xlog_exit();

    return t.exit();
}
