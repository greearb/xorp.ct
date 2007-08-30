// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/rip/test_auth.cc,v 1.26 2007/02/16 22:47:16 pavlin Exp $"

#include "rip_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "libxorp/c_format.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/utils.hh"

#include "auth.hh"
#include "test_utils.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Constants
//

static const char *program_name         = "test_auth";
static const char *program_description  = "Test RIP authentication operations";
static const char *program_version_id   = "0.1";
static const char *program_date         = "April, 2003";
static const char *program_copyright    = "See file LICENSE";
static const char *program_return_value = "0 on success, 1 if test error, 2 if internal error";


// ----------------------------------------------------------------------------
// Utility functions

/**
 * Build an authenticated RIP packet.
 *
 * @param pkt vector to hold raw packet.
 * @param ah authentication handler to be used.
 * @param n number of route entries to place in packet.
 *
 * @return 0 on success, 1 on failure.
 */
static int
build_auth_packet(vector<uint8_t>& pkt, AuthHandlerBase& ah, uint32_t n)
{
    vector<uint8_t> trailer;

    pkt.resize(RipPacketHeader::size() +
	       (n + ah.head_entries()) * PacketRouteEntry<IPv4>::size());

    RipPacketHeaderWriter rph(&pkt[0]);
    rph.initialize(RipPacketHeader::REQUEST, 2);

    for (uint32_t i = 0; i < n; i++) {
	uint32_t offset = RipPacketHeader::size() +
	    (i + ah.head_entries()) * PacketRouteEntry<IPv4>::size();
	PacketRouteEntryWriter<IPv4> p(&pkt[0] + offset);
	p.initialize(0, IPv4Net("10.0.0.0/8"), IPv4("172.11.100.1"), 3);
    }

    RipPacket<IPv4> rip_packet(IPv4::ZERO(), RIP_PORT, n + ah.head_entries());
    rip_packet.data() = pkt;

    size_t n_routes = 0;
    list<RipPacket<IPv4>*> auth_packets;
    if ((ah.authenticate_outbound(rip_packet, auth_packets, n_routes) != true)
	|| (n_routes != n)) {
	verbose_log("Unexpected outbound authentication failure: %s\n",
		    ah.error().c_str());
	return 1;
    }

    //
    // XXX: there should be only one copy of the authenticated packet,
    // and we don't care about it.
    //
    XLOG_ASSERT(auth_packets.size() == 1);
    delete_pointers_list(auth_packets);

    // Copy the modified data back to the original packet
    pkt = rip_packet.data();

    return 0;
}

/**
 * Check an authenticated RIP packet.
 *
 * @param pkt raw packet to be checked.
 * @param ah authentication handler to be used to check packet.
 * @param n number of entries expected in packet.
 * @param expect_fail expect failure flag.
 *
 * @return 0 on success, 1 on failure.
 */
static int
check_auth_packet(const vector<uint8_t>& pkt,
		  AuthHandlerBase&	 ah,
		  uint32_t		 n,
		  bool			 expect_fail = false)
{
    const uint8_t* entries_ptr = NULL;
    uint32_t n_entries = 0;

    if (ah.authenticate_inbound(&pkt[0], pkt.size(), entries_ptr, n_entries,
				IPv4::ZERO(), false)
	== false) {
	if (expect_fail == false) {
	    verbose_log("Unexpected failure (actual entries %u, "
			"expected %u) - %s\n",
			XORP_UINT_CAST(n_entries), XORP_UINT_CAST(n),
			ah.error().c_str());
	    return 1;
	}
	return 0;
    }

    if (n == 0) {
	if (entries_ptr != NULL) {
	    verbose_log("Got an address for start of entries when no entries "
			"present in a packet.\n");
	    return 1;
	}
	return 0;
    }

    const uint8_t* exp0 = (&pkt[0] + RipPacketHeader::size());

    exp0 += PacketRouteEntry<IPv4>::size() * ah.head_entries();
    if (entries_ptr != exp0) {
	verbose_log("First entry in packet does not correspond with expected "
		    "position\n");
	return 1;
    }

    return 0;
}

/**
 * Dump a packet.
 */
void dump_binary_data(const vector<uint8_t>& data)
{
    static const uint32_t BPL = 8;	// Bytes Per Line

    vector<uint8_t>::size_type i = 0;
    while (i != data.size()) {
	fprintf(stdout, "%08x ", XORP_UINT_CAST(i));
	string hex;
	string asc;
	int r = (data.size() - i) > BPL ? BPL : data.size() - i;
	while (r != 0) {
	    if ((i & 7) == 4)
		hex += " ";
	    hex += c_format("%02x", data[i]);
	    asc += (xorp_isprint(data[i])) ? char(data[i]) : '.';
	    i++;
	    r--;
	}
	hex += string(BPL, ' ');
	hex = string(hex, 0, 2 * BPL + 1);
	fprintf(stdout, "%s %s\n", hex.c_str(), asc.c_str());
    }
}


/**
 * A MD5 rip packet captured on the wire.
 */
uint8_t rip_packet[] = {
 0x2,   0x2,   0x0,   0x0,  0xff,  0xff,   0x0,   0x3, 
 0x0,  0xf4,   0x1,  0x14,   0x0,   0x0,   0x1,  0x13, 
 0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0, 
 0x0,   0x2,   0x0,   0x0,  0xc0,  0x96,  0xba,   0x0, 
0xff,  0xff,  0xff,   0x0,   0x0,   0x0,   0x0,   0x0, 
 0x0,   0x0,   0x0,   0xc,   0x0,   0x2,   0x0,   0x0, 
0xc0,  0x96,  0xbb,  0xe0,  0xff,  0xff,  0xff,  0xfc, 
 0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0xc, 
 0x0,   0x2,   0x0,   0x0,  0xc0,  0x96,  0xbb,  0xf0, 
0xff,  0xff,  0xff,  0xf8,   0x0,   0x0,   0x0,   0x0, 
 0x0,   0x0,   0x0,   0xc,   0x0,   0x2,   0x0,   0x0, 
0xc0,  0x96,  0xbb,  0xf8,  0xff,  0xff,  0xff,  0xf8, 
 0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0xb, 
 0x0,   0x2,   0x0,   0x0,  0xc0,  0xa8,   0x3,   0x0, 
0xff,  0xff,  0xff,   0x0,   0x0,   0x0,   0x0,   0x0, 
 0x0,   0x0,   0x0,   0xc,   0x0,   0x2,   0x0,   0x0, 
0xc0,  0xa8,   0x4,   0x0,  0xff,  0xff,  0xff,   0x0, 
 0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0xc, 
 0x0,   0x2,   0x0,   0x0,  0xc0,  0xa8,  0xfe,   0x1, 
0xff,  0xff,  0xff,  0xff,   0x0,   0x0,   0x0,   0x0, 
 0x0,   0x0,   0x0,   0xc,   0x0,   0x2,   0x0,   0x0, 
0xc0,  0xa8,  0xfe,   0x2,  0xff,  0xff,  0xff,  0xff, 
 0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0xc, 
 0x0,   0x2,   0x0,   0x0,  0xc0,  0xa8,  0xfe,   0x3, 
0xff,  0xff,  0xff,  0xff,   0x0,   0x0,   0x0,   0x0, 
 0x0,   0x0,   0x0,   0xb,   0x0,   0x2,   0x0,   0x0, 
0xc0,  0xa8,  0xfe,   0x4,  0xff,  0xff,  0xff,  0xff, 
 0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0x0,   0xc, 
 0x0,   0x2,   0x0,   0x0,  0xc0,  0xa8,  0xfe,   0x5, 
0xff,  0xff,  0xff,  0xff,   0x0,   0x0,   0x0,   0x0, 
 0x0,   0x0,   0x0,   0xd,  0xff,  0xff,   0x0,   0x1, 
0x2d,  0xaa,  0xa4,  0xba,  0x2e,  0xfd,  0x5c,   0xb, 
0x25,  0x44,  0xb5,  0x98,  0xcd,  0x5f,  0x24,  0xab, 
};

/**
 * Check MD5 authentication against a saved packet.
 */
inline
int
check_saved_md5()
{
    EventLoop e;
    MD5AuthHandler mah(e);
    string dummy_error_msg;
    mah.add_key(1, "bgp@icsi", TimeVal::ZERO(), TimeVal::MAXIMUM(),
		dummy_error_msg);

    vector<uint8_t> pkt;
    for (uint32_t i = 0; i < sizeof(rip_packet); i++)
	pkt.push_back(rip_packet[i]);

    uint32_t n = 11;
    if (check_auth_packet(pkt, mah, n, false)) {
	verbose_log("Failed MD5 authentication with %u entries\n",
		    XORP_UINT_CAST(n));
	dump_binary_data(pkt);
	return 1;
    }

    return 0;
}

//----------------------------------------------------------------------------
// The test

static int
test_main()
{
    string dummy_error_msg;

    // Static sizing tests
    static_assert(RipPacketHeader::SIZE == 4);
    static_assert(PacketRouteEntry<IPv4>::SIZE == 20);
    static_assert(RipPacketHeader::SIZE == RIPv2_MIN_PACKET_BYTES);
    static_assert(RipPacketHeader::SIZE + PacketRouteEntry<IPv4>::SIZE
		  == RIPv2_MIN_AUTH_PACKET_BYTES);
    static_assert(PacketRouteEntry<IPv4>::SIZE
		  == PlaintextPacketRouteEntry4::SIZE);
    static_assert(PacketRouteEntry<IPv4>::SIZE == MD5PacketRouteEntry4::SIZE);
    static_assert(MD5PacketTrailer::SIZE == 20);
    static_assert(PacketRouteEntry<IPv4>::SIZE
		  == PacketRouteEntry<IPv6>::SIZE);

    vector<uint8_t> pkt;

    //
    // Null Authentication Handler test
    //

    NullAuthHandler nah;
    for (uint32_t n = 0; n < nah.max_routing_entries(); n++) {
	if (build_auth_packet(pkt, nah, n)) {
	    verbose_log("Failed to build null authentication scheme packet "
			"with %u entries.\n", XORP_UINT_CAST(n));
	    return 1;
	}

	if (check_auth_packet(pkt, nah, n, false)) {
	    verbose_log("Failed null authentication with %u entries\n",
			XORP_UINT_CAST(n));
	    return 1;
	}

	// Add some extra data to break packet size.
	pkt.push_back(uint8_t(0));
	if (check_auth_packet(pkt, nah, n, true)) {
	    verbose_log("Null authentication passed broken packet "
			"with %u entries\n", XORP_UINT_CAST(n));
	    return 1;
	}
    }

    //
    // Plaintext Authentication Handler test
    //
    // Plaintext test three run throughs one without password, one
    // with password less than 16 characters, one with password > 16
    // characters
    //
    PlaintextAuthHandler pah;
    for (uint32_t i = 0; i < 3; i++) {
	for (uint32_t n = 0; n < pah.max_routing_entries(); n++) {
	    if (build_auth_packet(pkt, pah, n)) {
		verbose_log("Failed to build plaintext authentication scheme "
			    "packet with %u entries.\n", XORP_UINT_CAST(n));
		return 1;
	    }

	    if (check_auth_packet(pkt, pah, n, false)) {
		verbose_log("Failed plaintext authentication with "
			    "%u entries\n", XORP_UINT_CAST(n));
		return 1;
	    }

	    // Add some extra data to break packet size.
	    pkt.push_back(uint8_t(0));
	    if (check_auth_packet(pkt, pah, n, true)) {
		verbose_log("Plaintext authentication passed broken packet "
			    "with %u entries\n", XORP_UINT_CAST(n));
		return 1;
	    }
	    if (n == 3 && verbose()) {
		verbose_log("Example Plaintext password packet.\n");
		dump_binary_data(pkt);
	    }
	}
	pah.set_key(pah.key() + string("A password"));
    }

    //    xlog_enable(XLOG_LEVEL_INFO);

    //
    // MD5 Authentication Handler tests
    //
    if (0 != check_saved_md5())
 	return 1;

    EventLoop e;
    MD5AuthHandler mah(e);
    mah.add_key(1, "Hello World!", TimeVal::ZERO(), TimeVal::MAXIMUM(),
		dummy_error_msg);
    for (uint32_t n = 0; n < mah.max_routing_entries(); n++) {
	if (build_auth_packet(pkt, mah, n)) {
	    verbose_log("Failed to build MD5 authentication scheme packet "
			"with %u entries.\n", XORP_UINT_CAST(n));
	    return 1;
	}

	if (check_auth_packet(pkt, mah, n, false)) {
	    verbose_log("Failed MD5 authentication with %u entries\n",
			XORP_UINT_CAST(n));
	    dump_binary_data(pkt);
	    return 1;
	}

	// Add some extra data to break packet size.
	pkt.push_back(uint8_t(0));
	if (check_auth_packet(pkt, mah, n, true)) {
	    verbose_log("MD5 authentication passed broken packet "
			"with %u entries\n", XORP_UINT_CAST(n));
	    dump_binary_data(pkt);
	    return 1;
	}

	// Build other packets of same size and corrupt bytes in order
	// nb we have to build another packet otherwise we always fail
	// the sequence number test.
	for (vector<uint8_t>::size_type c = 0; c != pkt.size(); c++) {
	    if (build_auth_packet(pkt, mah, n)) {
		verbose_log("Failed to build MD5 authentication scheme packet "
			    "with %u entries.\n", XORP_UINT_CAST(n));
		dump_binary_data(pkt);
		return 1;
	    }
	    vector<uint8_t> bad_pkt(pkt);
	    bad_pkt[c] = bad_pkt[c] ^ 0x01;
	    if (check_auth_packet(bad_pkt, mah, n, true)) {
		verbose_log("MD5 authentication passed corruption in byte "
			    "%u with in packet %u entries\n",
			    XORP_UINT_CAST(c),
			    XORP_UINT_CAST(n));
		dump_binary_data(bad_pkt);
		return 1;
	    }
	}
    }

    //
    // Check removing the 1 MD5 key we have on ring works
    //
    mah.remove_key(1, dummy_error_msg);
    if (! mah.empty()) {
	verbose_log("Key removal failed\n");
	return 1;
    }

    //
    // Add a selection of keys and check they timeout correctly
    //
    TimeVal now;
    e.current_time(now);
    uint32_t i;
    for (i = 0; i < 5; i++) {
	mah.add_key(i, "testing123", now, now + TimeVal(i, 0),
		    dummy_error_msg);
    }
    mah.add_key(i, "testing123", now, TimeVal::MAXIMUM(), dummy_error_msg);

    bool stop_eventloop = false;
    XorpTimer to = e.set_flag_after(TimeVal(i, 0), &stop_eventloop);
    while (stop_eventloop == false) {
	e.run();
    }

    if (mah.valid_key_chain().size() != 1) {
	verbose_log("Bogus key count: expected 1 key left, have %u\n",
		    XORP_UINT_CAST(mah.valid_key_chain().size()));
	return 1;
    }

    return 0;
}


/**
 * Print program info to output stream.
 *
 * @param stream the output stream the print the program info to.
 */
static void
print_program_info(FILE *stream)
{
    fprintf(stream, "Name:          %s\n", program_name);
    fprintf(stream, "Description:   %s\n", program_description);
    fprintf(stream, "Version:       %s\n", program_version_id);
    fprintf(stream, "Date:          %s\n", program_date);
    fprintf(stream, "Copyright:     %s\n", program_copyright);
    fprintf(stream, "Return:        %s\n", program_return_value);
}

/**
 * Print program usage information to the stderr.
 *
 * @param progname the name of the program.
 */
static void
usage(const char* progname)
{
    print_program_info(stderr);
    fprintf(stderr, "usage: %s [-v] [-h]\n", progname);
    fprintf(stderr, "       -h          : usage (this message)\n");
    fprintf(stderr, "       -v          : verbose output\n");
}

int
main(int argc, char* const argv[])
{
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);         // Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    int ch;
    while ((ch = getopt(argc, argv, "hv")) != -1) {
        switch (ch) {
        case 'v':
            set_verbose(true);
            break;
        case 'h':
        case '?':
        default:
            usage(argv[0]);
            xlog_stop();
            xlog_exit();
            if (ch == 'h')
                return (0);
            else
                return (1);
        }
    }
    argc -= optind;
    argv += optind;

    int rval = 0;
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    try {
	rval = test_main();
    } catch (...) {
        // Internal error
        xorp_print_standard_exceptions();
        rval = 2;
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    verbose_log("Exit status %d\n", rval);
    return rval;
}
