// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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



#include "rip_module.h"

#include "libxorp/xlog.h"

#include "libxorp/c_format.hh"
#include "libxorp/eventloop.hh"

#include "packets.hh"
#include "test_utils.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

///////////////////////////////////////////////////////////////////////////////
//
// Constants
//

static const char *program_name         = "test_packets";
static const char *program_description  = "Test RIP packet operations";
static const char *program_version_id   = "0.1";
static const char *program_date         = "April, 2003";
static const char *program_copyright    = "See file LICENSE";
static const char *program_return_value = "0 on success, 1 if test error, 2 if internal error";


//----------------------------------------------------------------------------
// The test

static int
test_main()
{
    // Static sizing tests
    x_static_assert(RipPacketHeader::SIZE == 4);
    x_static_assert(PacketRouteEntry<IPv4>::SIZE == 20);
    x_static_assert(RipPacketHeader::SIZE == RIPv2_MIN_PACKET_BYTES);
    x_static_assert(RipPacketHeader::SIZE + PacketRouteEntry<IPv4>::SIZE
		  == RIPv2_MIN_AUTH_PACKET_BYTES);
    x_static_assert(PacketRouteEntry<IPv4>::SIZE
		  == PlaintextPacketRouteEntry4::SIZE);
    x_static_assert(PacketRouteEntry<IPv4>::SIZE == MD5PacketRouteEntry4::SIZE);
    x_static_assert(MD5PacketTrailer::SIZE == 20);
    x_static_assert(PacketRouteEntry<IPv4>::SIZE
		  == PacketRouteEntry<IPv6>::SIZE);

    //
    // Test packet header
    //
    {
	uint8_t h4[4] = { 1, 2, 0, 0 };
	const RipPacketHeader rph(h4);
	if (rph.valid_command() == false) {
	    verbose_log("Bad valid command check\n");
	    return 1;
	}
	h4[0] = 3;
	if (rph.valid_command() == true) {
	    verbose_log("Bad valid command check\n");
	    return 1;
	}
	if (rph.valid_version(2) == false) {
	    verbose_log("Bad version check\n");
	    return 1;
	}
	if (rph.valid_version(3) == true) {
	    verbose_log("Bad version check\n");
	    return 1;
	}
	if (rph.valid_padding() == false) {
	    verbose_log("Bad padding check\n");
	    return 1;
	}
	h4[3] = 1;
	if (rph.valid_padding() == true) {
	    verbose_log("Bad padding check\n");
	    return 1;
	}
	h4[2] = 1;
	h4[3] = 0;
	if (rph.valid_padding() == true) {
	    verbose_log("Bad padding check\n");
	    return 1;
	}
    }

    //
    // Test RIPv2 Route Entry
    //
    {
	uint16_t tag(1096);
	IPv4Net net(IPv4("10.0.10.0"), 24);
	IPv4 nh("10.0.10.1");
	uint32_t metric(12);

	uint8_t r[20];
	PacketRouteEntryWriter<IPv4> pre(r);

	pre.initialize(tag, net, nh, metric);

	uint8_t e[20] = {
	    0x00, 0x02, 0x04, 0x48,
	    0x0a, 0x00, 0x0a, 0x00,
	    0xff, 0xff, 0xff, 0x00,
	    0x0a, 0x00, 0x0a, 0x01,
	    0x00, 0x00, 0x00, 0x0c
	};
	for (size_t i = 0; i < 20; i++) {
	    if (e[i] != r[i]) {
		verbose_log("Expected packet data wrong at position %u\n",
			    XORP_UINT_CAST(i));
		return 1;
	    }
	}

	if (pre.addr_family() != PacketRouteEntry<IPv4>::ADDR_FAMILY) {
	    verbose_log("Bad address family accessor\n");
	    return 1;
	} else if (pre.tag() != tag) {
	    verbose_log("Bad tag accessor\n");
	    return 1;
	} else if (pre.net() != net) {
	    verbose_log("Bad net accessor\n");
	    return 1;
	} else if (pre.nexthop() != nh) {
	    verbose_log("Bad nexthop accessor\n");
	    return 1;
	} else if (pre.metric() != metric) {
	    verbose_log("Bad cost accessor\n");
	    return 1;
	}
    }

    //
    // Test Plaintext Password
    //
    {
	uint8_t r[20];
	PlaintextPacketRouteEntry4Writer pre(r);
	pre.initialize("16 character password");

	uint8_t e[20] = {
	    0xff, 0xff, 0x00, 0x02,
	    '1', '6', ' ', 'c',
	    'h', 'a', 'r', 'a',
	    'c', 't', 'e', 'r',
	    ' ', 'p', 'a', 's'
	};
	for (size_t i = 0; i < 20; i++) {
	    if (e[i] != r[i]) {
		verbose_log("Expected packet data wrong at position %u\n",
			    XORP_UINT_CAST(i));
		return 1;
	    }
	}
	if (pre.password() != "16 character pas") {
	    verbose_log("Password accessor wrong\n");
	}

	pre.initialize("8 character");
	uint8_t f[20] = {
	    0xff, 0xff, 0x00, 0x02,
	    '8', ' ', 'c', 'h',
	    'a', 'r', 'a', 'c',
	    't', 'e', 'r', 0x0,
	    0x0, 0x0, 0x0, 0x0
	};
	for (size_t i = 0; i < 20; i++) {
	    if (f[i] != r[i]) {
		verbose_log("Expected packet data wrong at position %u\n",
			    XORP_UINT_CAST(i));
		return 1;
	    }
	}

    }

    //
    // Test MD5 Password
    //
    {
	uint8_t r[20];
	MD5PacketRouteEntry4Writer pre(r);
	pre.initialize(0x7fee, 0xcc, 0x08, 0x12345678);

	uint8_t e[20] = {
	    0xff, 0xff, 0x00, 0x03,
	    0x7f, 0xee, 0xcc, 0x08,
	    0x12, 0x34, 0x56, 0x78,
	    0x00, 0x00, 0x00, 0x00,
	    0x00, 0x00, 0x00, 0x00
	};
	for (size_t i = 0; i < 20; i++) {
	    if (e[i] != r[i]) {
		verbose_log("Expected packet data wrong at position %u\n",
			    XORP_UINT_CAST(i));
		return 1;
	    }
	}

	if (pre.addr_family() != MD5PacketRouteEntry4::ADDR_FAMILY) {
	    verbose_log("bad address family accessor\n");
	    return 1;
	} else if (pre.auth_type() != MD5PacketRouteEntry4::AUTH_TYPE) {
	    verbose_log("bad auth type accessor\n");
	    return 1;
	} else if (pre.auth_off() != 0x7fee) {
	    verbose_log("bad packet bytes accessor\n");
	    return 1;
	} else if (pre.key_id() != 0xcc) {
	    verbose_log("bad key id accessor\n");
	    return 1;
	} else if (pre.auth_bytes() != 0x08) {

	    verbose_log("bad auth bytes accessor\n");
	    return 1;
	} else if (pre.seqno() != 0x12345678) {
	    verbose_log("bad seqno accessor\n");
	    return 1;
	}
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

    return rval;
}
