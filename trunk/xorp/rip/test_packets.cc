// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/rip/test_packets.cc,v 1.5 2004/02/20 21:19:11 hodson Exp $"

#include "rip_module.h"

#include "libxorp/xlog.h"

#include "libxorp/c_format.hh"
#include "libxorp/eventloop.hh"

#include "packets.hh"
#include "test_utils.hh"

///////////////////////////////////////////////////////////////////////////////
//
// Constants
//

static const char *program_name         = "test_packets";
static const char *program_description  = "Test RIP packet operations";
static const char *program_version_id   = "0.1";
static const char *program_date         = "April, 2003";
static const char *program_copyright    = "See file LICENSE.XORP";
static const char *program_return_value = "0 on success, 1 if test error, 2 if internal error";


//----------------------------------------------------------------------------
// The test

static int
test_main()
{
    // Static sizing tests
    static_assert(sizeof(RipPacketHeader) == 4);
    static_assert(sizeof(PacketRouteEntry<IPv4>) == 20);
    static_assert(sizeof(RipPacketHeader) == RIPv2_MIN_PACKET_BYTES);
    static_assert(sizeof(RipPacketHeader) + sizeof(PacketRouteEntry<IPv4>)
		  == RIPv2_MIN_AUTH_PACKET_BYTES);
    static_assert(sizeof(PacketRouteEntry<IPv4>)
		  == sizeof(PlaintextPacketRouteEntry4));
    static_assert(sizeof(PacketRouteEntry<IPv4>)
		  == sizeof(MD5PacketRouteEntry4));
    static_assert(sizeof(MD5PacketTrailer) == 20);
    static_assert(sizeof(PacketRouteEntry<IPv4>)
		  == sizeof(PacketRouteEntry<IPv6>));

    //
    // Test packet header
    //
    {
	uint8_t h4[4] = { 1, 2, 0, 0 };
	const RipPacketHeader* rph =
	    reinterpret_cast<const RipPacketHeader*>(&h4);
	if (rph->valid_command() == false) {
	    verbose_log("Bad valid command check\n");
	    return 1;
	}
	h4[0] = 3;
	if (rph->valid_command() == true) {
	    verbose_log("Bad valid command check\n");
	    return 1;
	}
	if (rph->valid_version(2) == false) {
	    verbose_log("Bad version check\n");
	    return 1;
	}
	if (rph->valid_version(3) == true) {
	    verbose_log("Bad version check\n");
	    return 1;
	}
	if (rph->valid_padding() == false) {
	    verbose_log("Bad padding check\n");
	    return 1;
	}
	h4[3] = 1;
	if (rph->valid_padding() == true) {
	    verbose_log("Bad padding check\n");
	    return 1;
	}
	h4[2] = 1;
	h4[3] = 0;
	if (rph->valid_padding() == true) {
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
	PacketRouteEntry<IPv4>* pre =
	    reinterpret_cast<PacketRouteEntry<IPv4>*>(r);

	pre->initialize(tag, net, nh, metric);

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
			    static_cast<uint32_t>(i));
		return 1;
	    }
	}

	if (pre->addr_family() != PacketRouteEntry<IPv4>::ADDR_FAMILY) {
	    verbose_log("Bad address family accessor\n");
	    return 1;
	} else if (pre->tag() != tag) {
	    verbose_log("Bad tag accessor\n");
	    return 1;
	} else if (pre->net() != net) {
	    verbose_log("Bad net accessor\n");
	    return 1;
	} else if (pre->nexthop() != nh) {
	    verbose_log("Bad nexthop accessor\n");
	    return 1;
	} else if (pre->metric() != metric) {
	    verbose_log("Bad cost accessor\n");
	    return 1;
	}
    }

    //
    // Test Plaintext Password
    //
    {
	uint8_t r[20];
	PlaintextPacketRouteEntry4* pre =
	    reinterpret_cast<PlaintextPacketRouteEntry4*>(r);
	pre->initialize("16 character password");

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
			    static_cast<uint32_t>(i));
		return 1;
	    }
	}
	if (pre->password() != "16 character pas") {
	    verbose_log("Password accessor wrong\n");
	}

	pre->initialize("8 character");
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
			    static_cast<uint32_t>(i));
		return 1;
	    }
	}

    }

    //
    // Test MD5 Password
    //
    {
	uint8_t r[20];
	MD5PacketRouteEntry4* pre =
	    reinterpret_cast<MD5PacketRouteEntry4*>(r);
	pre->initialize(0x7fee, 0xcc, 0x08, 0x12345678);

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
			    static_cast<uint32_t>(i));
		return 1;
	    }
	}

	if (pre->addr_family() != MD5PacketRouteEntry4::ADDR_FAMILY) {
	    verbose_log("bad address family accessor\n");
	    return 1;
	} else if (pre->auth_type() != MD5PacketRouteEntry4::AUTH_TYPE) {
	    verbose_log("bad auth type accessor\n");
	    return 1;
	} else if (pre->auth_offset() != 0x7fee) {
	    verbose_log("bad packet bytes accessor\n");
	    return 1;
	} else if (pre->key_id() != 0xcc) {
	    verbose_log("bad key id accessor\n");
	    return 1;
	} else if (pre->auth_bytes() != 0x08) {

	    verbose_log("bad auth bytes accessor\n");
	    return 1;
	} else if (pre->seqno() != 0x12345678) {
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
