// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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



#include "olsr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/exceptions.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/status_codes.h"
#include "libxorp/service.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/tokenize.hh"
#include "libxorp/test_main.hh"

#include "olsr.hh"
#include "message.hh"

#include <algorithm>

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

inline
void
populate_mid(MidMessage* mid)
{
    mid->set_hop_count(0);
    mid->set_ttl(OlsrTypes::MAX_TTL);
    mid->set_origin(IPv4("192.168.1.1"));
    mid->set_seqno(31337);

    TimeVal expiry(256,0);
    mid->set_expiry_time(expiry);
    TimeVal rx(1, 0);
    mid->set_receive_time(rx);

    mid->add_interface(IPv4("192.168.0.1"));

    mid->set_valid(true);
}

/**
 * Create a packet containing a MID message, and print it.
 */
bool
mid_packet_print(TestInfo& info)
{
    MessageDecoder md;

    Packet* pkt = new Packet(md);
    MidMessage* mid = new MidMessage();

    populate_mid(mid);
    // The next line makes 'mid' invalid in this scope,
    // as it will be turned into a ref_ptr.
    pkt->add_message(mid);

    DOUT(info) << pkt->str() << endl;

    delete mid;	// XXX refcounting was removed
    delete pkt;

    return true;
}

inline
void
populate_hna(HnaMessage* hna)
{
    hna->set_hop_count(0);
    hna->set_ttl(OlsrTypes::MAX_TTL);
    hna->set_origin(IPv4("192.168.0.123"));
    hna->set_seqno(31337);

    TimeVal expiry(256,0);
    hna->set_expiry_time(expiry);
    TimeVal rx(1, 0);
    hna->set_receive_time(rx);

    hna->add_network(IPv4Net("192.168.2.0/24"));

    hna->set_valid(true);
}

/**
 * Create a packet containing an HNA message, and print it.
 */
bool
hna_packet_print(TestInfo& info)
{
    MessageDecoder md;

    Packet* pkt = new Packet(md);
    HnaMessage* hna = new HnaMessage();

    populate_hna(hna);
    pkt->add_message(hna);

    DOUT(info) << pkt->str() << endl;

    delete hna;	// XXX refcounting was removed
    delete pkt;

    return true;
}

inline
void
populate_tc(TcMessage* tc)
{
    tc->set_hop_count(0);
    tc->set_ttl(OlsrTypes::MAX_TTL);
    tc->set_origin(IPv4("192.168.0.123"));
    tc->set_seqno(31337);

    TimeVal expiry(256,0);
    tc->set_expiry_time(expiry);
    TimeVal rx(1, 0);
    tc->set_receive_time(rx);

    tc->add_neighbor(IPv4("192.168.1.124"));
    tc->set_ansn(111);

    tc->set_valid(true);
}

/**
 * Create a packet containing a TC message, and print it.
 */
bool
tc_packet_print(TestInfo& info)
{
    MessageDecoder md;

    Packet* pkt = new Packet(md);
    TcMessage* tc = new TcMessage();

    populate_tc(tc);
    pkt->add_message(tc);

    DOUT(info) << pkt->str() << endl;

    delete tc;	// XXX refcounting was removed
    delete pkt;

    return true;
}

inline
void
populate_hello(HelloMessage* hello)
{
    hello->set_hop_count(0);
    hello->set_ttl(OlsrTypes::MAX_TTL);
    hello->set_origin(IPv4("192.168.0.1"));
    hello->set_seqno(31338);

    TimeVal expiry(256,0);
    hello->set_expiry_time(expiry);
    TimeVal rx(1, 0);
    hello->set_receive_time(rx);

    TimeVal htime(5,0);
    hello->set_htime(htime);
    hello->set_willingness(OlsrTypes::WILL_ALWAYS);

    LinkCode lc(OlsrTypes::MPR_NEIGH, OlsrTypes::SYM_LINK);
    hello->add_link(lc, IPv4("192.168.0.2"));

    hello->set_valid(true);
}

/**
 * Create a packet containing a HELLO message, and print it.
 */
bool
hello_packet_print(TestInfo& info)
{
    MessageDecoder md;

    Packet* pkt = new Packet(md);
    HelloMessage* hello = new HelloMessage();

    populate_hello(hello);
    pkt->add_message(hello);

    DOUT(info) << pkt->str() << endl;

    delete hello;	// XXX refcounting was removed
    delete pkt;

    return true;
}

uint8_t multi_v4_packet_data[] = {
#include "regression/multi_msg_ok_ipv4.data"
};

/**
 * Decode a packet containing many messages, and print it.
 */
bool
multi_packet_decode(TestInfo& info)
{
    MessageDecoder md;
    initialize_message_decoder(md);

    Packet* pkt = new Packet(md);
    try {
	pkt->decode(&multi_v4_packet_data[0], sizeof(multi_v4_packet_data));
    } catch (InvalidPacket& e) {
	DOUT(info) << cstring(e) << endl;
	return false;
    }

    DOUT(info) << pkt->str() << endl;

#if 1
    {
    // Check that the 'first' and 'last' packet flags are set as
    // S-OLSR needs to see them.
    const vector<Message*>& msgs = pkt->messages();

    size_t index;
    vector<Message*>::const_iterator ii;
    for (index = 0, ii = msgs.begin(); ii != msgs.end(); ii++, index++) {
	if (0 == index)
	    XLOG_ASSERT((*ii)->is_first() == true);
	if ((msgs.size() - 1) == index)
	    XLOG_ASSERT((*ii)->is_last() == true);
    }
    }
#endif

#if 1	// XXX refcounting was removed for now
    {
    const vector<Message*>& msgs = pkt->messages();

    vector<Message*>::const_iterator ii;
    for (ii = msgs.begin(); ii != msgs.end(); ii++) {
	delete (*ii);
    }
    }
#endif

    delete pkt;
    return true;
}

uint8_t hna_v4_packet_data[] = {
#include "regression/hna_ok_ipv4.data"
};

/**
 * Decode a packet containing an HNA message, and print it.
 */
bool
hna_packet_decode(TestInfo& info)
{
    MessageDecoder md;
    initialize_message_decoder(md);

    Packet* pkt = new Packet(md);
    try {
	pkt->decode(&hna_v4_packet_data[0], sizeof(hna_v4_packet_data));
    } catch (InvalidPacket& e) {
	DOUT(info) << cstring(e) << endl;
	return false;
    }

    DOUT(info) << pkt->str() << endl;

#if 1	// XXX refcounting was removed for now
    {
    const vector<Message*>& msgs = pkt->messages();

    vector<Message*>::const_iterator ii;
    for (ii = msgs.begin(); ii != msgs.end(); ii++) {
	delete (*ii);
    }
    }
#endif

    delete pkt;
    return true;
}

uint8_t mid_v4_packet_data[] = {
#include "regression/mid_ok_ipv4.data"
};

/**
 * Decode a packet containing a MID message, and print it.
 */
bool
mid_packet_decode(TestInfo& info)
{
    MessageDecoder md;
    initialize_message_decoder(md);

    Packet* pkt = new Packet(md);
    try {
	pkt->decode(&mid_v4_packet_data[0], sizeof(mid_v4_packet_data));
    } catch (InvalidPacket& e) {
	DOUT(info) << cstring(e) << endl;
	return false;
    }

    DOUT(info) << pkt->str() << endl;

#if 1	// XXX refcounting was removed for now
    {
    const vector<Message*>& msgs = pkt->messages();

    vector<Message*>::const_iterator ii;
    for (ii = msgs.begin(); ii != msgs.end(); ii++) {
	delete (*ii);
    }
    }
#endif

    delete pkt;
    return true;
}

uint8_t tc_v4_packet_data[] = {
#include "regression/tc_ok_ipv4.data"
};

/**
 * Decode a packet containing a TC message, and print it.
 */
bool
tc_packet_decode(TestInfo& info)
{
    MessageDecoder md;
    initialize_message_decoder(md);

    Packet* pkt = new Packet(md);
    try {
	pkt->decode(&tc_v4_packet_data[0], sizeof(tc_v4_packet_data));
    } catch (InvalidPacket& e) {
	DOUT(info) << cstring(e) << endl;
	return false;
    }

    DOUT(info) << pkt->str() << endl;

#if 1	// XXX refcounting was removed for now
    {
    const vector<Message*>& msgs = pkt->messages();

    vector<Message*>::const_iterator ii;
    for (ii = msgs.begin(); ii != msgs.end(); ii++) {
	delete (*ii);
    }
    }
#endif

    delete pkt;
    return true;
}

uint8_t hello_multi_v4_packet_data[] = {
#include "regression/hello_multi_ok_ipv4.data"
};

/**
 * Decode a packet containing a HELLO message, and print it.
 */
bool
hello_multi_link_packet_decode(TestInfo& info)
{
    MessageDecoder md;
    initialize_message_decoder(md);

    Packet* pkt = new Packet(md);
    try {
	pkt->decode(&hello_multi_v4_packet_data[0],
	    sizeof(hello_multi_v4_packet_data));
    } catch (InvalidPacket& e) {
	DOUT(info) << cstring(e) << endl;
	return false;
    }

    DOUT(info) << pkt->str() << endl;

#if 1	// XXX refcounting was removed for now
    {
    const vector<Message*>& msgs = pkt->messages();

    vector<Message*>::const_iterator ii;
    for (ii = msgs.begin(); ii != msgs.end(); ii++) {
	delete (*ii);
    }
    }
#endif

    delete pkt;
    return true;
}

inline
void
populate_hna_for_encode(HnaMessage *hna)
{
    TimeVal expiry(256,0);
    hna->set_expiry_time(expiry);

    hna->set_origin(IPv4("192.168.124.1"));
    hna->set_ttl(OlsrTypes::MAX_TTL);
    hna->set_hop_count(0);
    hna->set_seqno(38018);

    hna->add_network(IPv4Net("192.168.123.0/24"));

    hna->set_valid(true);
}

/**
 * Create a packet containing an HNA message, intended to be
 * identical to a hard-coded test packet.
 */
bool
hna_packet_encode(TestInfo& info)
{
    MessageDecoder md;
    Packet* pkt = new Packet(md);
    HnaMessage* hna = new HnaMessage();

    populate_hna_for_encode(hna);
    pkt->add_message(hna);
    pkt->set_seqno(58449);

    vector<uint8_t> buf1;
    vector<uint8_t> buf2;

    pkt->encode(buf1);

    std::copy(&hna_v4_packet_data[0],
	&hna_v4_packet_data[0] + sizeof(hna_v4_packet_data),
	std::back_inserter(buf2));

    bool result = compare_packets(info, buf1, buf2);

    delete hna;	// XXX refcounting was removed
    delete pkt;

    return result;
}

inline
void
populate_mid_for_encode(MidMessage *mid)
{
    TimeVal expiry(256,0);
    mid->set_expiry_time(expiry);

    mid->set_origin(IPv4("192.168.124.1"));
    mid->set_ttl(OlsrTypes::MAX_TTL);
    mid->set_hop_count(0);
    mid->set_seqno(38019);

    mid->add_interface(IPv4("192.168.122.1"));
    mid->add_interface(IPv4("192.168.123.1"));
    mid->add_interface(IPv4("192.168.125.1"));

    mid->set_valid(true);
}

/**
 * Create a packet containing a MID message, intended to be
 * identical to a hard-coded test packet.
 */
bool
mid_packet_encode(TestInfo& info)
{
    MessageDecoder md;
    Packet* pkt = new Packet(md);
    MidMessage* mid = new MidMessage();

    populate_mid_for_encode(mid);
    pkt->add_message(mid);
    pkt->set_seqno(58450);

    vector<uint8_t> buf1;
    vector<uint8_t> buf2;

    pkt->encode(buf1);

    std::copy(&mid_v4_packet_data[0],
	&mid_v4_packet_data[0] + sizeof(mid_v4_packet_data),
	std::back_inserter(buf2));

    bool result = compare_packets(info, buf1, buf2);

    delete mid;	// XXX refcounting was removed
    delete pkt;

    return result;
}

inline
void
populate_tc_for_encode(TcMessage *tc)
{
    TimeVal expiry(256,0);
    tc->set_expiry_time(expiry);

    tc->set_origin(IPv4("192.168.124.2"));
    tc->set_ttl(OlsrTypes::MAX_TTL);
    tc->set_hop_count(0);
    tc->set_seqno(38017);

    tc->set_ansn(1);
    tc->add_neighbor(IPv4("192.168.124.17"));

    tc->set_valid(true);
}

/**
 * Create a packet containing a TC message, intended to be
 * identical to a hard-coded test packet.
 */
bool
tc_packet_encode(TestInfo& info)
{
    MessageDecoder md;
    Packet* pkt = new Packet(md);
    TcMessage* tc = new TcMessage();

    populate_tc_for_encode(tc);
    pkt->add_message(tc);
    pkt->set_seqno(58448);

    vector<uint8_t> buf1;
    vector<uint8_t> buf2;

    pkt->encode(buf1);

    std::copy(&tc_v4_packet_data[0],
	&tc_v4_packet_data[0] + sizeof(tc_v4_packet_data),
	std::back_inserter(buf2));

    bool result = compare_packets(info, buf1, buf2);

    delete tc;	// XXX refcounting was removed
    delete pkt;

    return result;
}

inline
void
populate_hello_for_encode(HelloMessage *hello)
{
    TimeVal expiry(256,0);
    hello->set_expiry_time(expiry);

    hello->set_origin(IPv4("192.168.124.1"));
    hello->set_ttl(OlsrTypes::MAX_TTL);
    hello->set_hop_count(0);
    hello->set_seqno(38038);

    hello->set_htime(TimeVal(6,0));
    hello->set_willingness(OlsrTypes::WILL_LOW);

    LinkCode lc1(OlsrTypes::NOT_NEIGH, OlsrTypes::ASYM_LINK);
    hello->add_link(lc1, IPv4("192.168.122.22"));
    hello->add_link(lc1, IPv4("192.168.122.23"));

    LinkCode lc2(OlsrTypes::SYM_NEIGH, OlsrTypes::SYM_LINK);
    hello->add_link(lc2, IPv4("192.168.122.24"));
    hello->add_link(lc2, IPv4("192.168.122.25"));

    hello->set_valid(true);
}

/**
 * Create a packet containing a HELLO message, intended to be
 * identical to a hard-coded test packet.
 */
bool
hello_packet_encode(TestInfo& info)
{
    MessageDecoder md;
    Packet* pkt = new Packet(md);
    HelloMessage* hello = new HelloMessage();

    populate_hello_for_encode(hello);
    pkt->add_message(hello);
    pkt->set_seqno(58445);

    vector<uint8_t> buf1;
    vector<uint8_t> buf2;

    pkt->encode(buf1);

#if 0
    for (size_t i = 0; i < buf1.size(); i++) {
	fprintf(stderr, "0x%02x ", XORP_UINT_CAST(buf1[i]));
    }
#endif

    std::copy(&hello_multi_v4_packet_data[0],
	&hello_multi_v4_packet_data[0] + sizeof(hello_multi_v4_packet_data),
	std::back_inserter(buf2));

    bool result = compare_packets(info, buf1, buf2);

    delete hello;	// XXX refcounting was removed
    delete pkt;

    return result;
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
	{"tc_print", callback(tc_packet_print)},
	{"mid_print", callback(mid_packet_print)},
	{"hna_print", callback(hna_packet_print)},

	{"hello_multi_decode", callback(hello_multi_link_packet_decode)},
	{"tc_decode", callback(tc_packet_decode)},
	{"mid_decode", callback(mid_packet_decode)},
	{"hna_decode", callback(hna_packet_decode)},
	{"multi_decode", callback(multi_packet_decode)},

	{"hello_encode", callback(hello_packet_encode)},
	{"tc_encode", callback(tc_packet_encode)},
	{"mid_encode", callback(mid_packet_encode)},
	{"hna_encode", callback(hna_packet_encode)},
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
