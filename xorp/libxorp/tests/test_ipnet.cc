// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "libxorp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "ipv4net.hh"


// ----------------------------------------------------------------------------
// Verbose output

static bool s_verbose = false;

bool verbose()			{ return s_verbose; }
void set_verbose(bool v)	{ s_verbose = v; }

#define verbose_log(x...) 						      \
do {									      \
    if (verbose()) {							      \
	printf("From %s:%d: ", __FILE__, __LINE__);			      \
	printf(x);							      \
    }									      \
} while(0)

// ----------------------------------------------------------------------------
// The tests

static bool
v4_serialization_test()
{
    struct s_data {
	const char* 	net;
	IPv4		v4;
	uint32_t	prefix_len;
    } srep[] = {
	{ "128.16.92.160/27", IPv4(htonl_literal(0x80105ca0U)), 27 },
	{ "128.16.0.0/16", IPv4(htonl_literal(0x80100000U)), 16 },
	{ "255.255.255.255/32", IPv4(htonl_literal(0xffffffffU)), 32},
	{ "0.0.0.0/16", IPv4(htonl_literal(0x00000000U)), 16 },
    };

    for (size_t i = 0; i < sizeof(srep) / sizeof(srep[0]); i++) {
	IPv4Net n(srep[i].net);
	if (n.prefix_len() != srep[i].prefix_len) {
	    verbose_log("item %u bad prefix_len %u\n", XORP_UINT_CAST(i),
			XORP_UINT_CAST(n.prefix_len()));
	    return false;
	} else if (n.masked_addr() != srep[i].v4) {
	    verbose_log("item %u bad addr %s != %s\n", 
			XORP_UINT_CAST(i), n.masked_addr().str().c_str(),
			srep[i].v4.str().c_str());
	    return false;
	} else if (n.netmask() != IPv4::make_prefix(n.prefix_len())) {
	    verbose_log("item %u bad netmask %s != %s\n",
			XORP_UINT_CAST(i), n.netmask().str().c_str(),
			IPv4::make_prefix(n.prefix_len()).str().c_str());
	    return false;
	}

	IPv4Net u (n.str().c_str());
	if (u != n) {
	    verbose_log("item %u to string and back failed.",
			XORP_UINT_CAST(i));
	    return false;
	}
    }
    verbose_log("Passed serialization test.\n");
    return true;
}

static bool
v4_less_than_test()
{
    const char* d[] = { "128.16.0.0/24", "128.16.64.0/24", 
			"128.16.0.0/16", "128.17.0.0/24" };
    for (size_t i = 1; i < sizeof(d) / sizeof(d[0]); i++) {
	IPv4Net lower(d[i - 1]);
	IPv4Net upper(d[i]);
	if (!(lower < upper)) {
	    verbose_log("%s is not less than %s\n",
			lower.str().c_str(),
			upper.str().c_str());
	    return false;
	}
    }
    verbose_log("Passed operator< test.\n");
    return true;
}

static bool
v4_is_unicast_test()
{
    IPv4Net uni("128.16.0.0/24");	// regular unicast
    IPv4Net multi("224.0.0.0/24");	// multicast, not valid
    IPv4Net odd1("128.0.0.0/1");	// not valid, includes multicast
    IPv4Net odd2("192.0.0.0/2");	// not valid, includes multicast
    IPv4Net odd3("0.0.0.0/1");		// only unicast, should be valid
    IPv4Net odd4("128.0.0.0/2");	// only unicast, should be valid
    IPv4Net deflt("0.0.0.0/0");		// default route, is valid
    if (!uni.is_unicast()) {
	verbose_log("%s failed is_unicast test.\n", uni.str().c_str());
	return false;
    }
    if (multi.is_unicast()) {
	verbose_log("%s failed is_unicast test.\n", multi.str().c_str());
	return false;
    }
    if (odd1.is_unicast()) {
	verbose_log("%s failed is_unicast test.\n", odd1.str().c_str());
	return false;
    }
    if (odd2.is_unicast()) {
	verbose_log("%s failed is_unicast test.\n", odd2.str().c_str());
	return false;
    }
    if (!odd3.is_unicast()) {
	verbose_log("%s failed is_unicast test.\n", odd3.str().c_str());
	return false;
    }
    if (!odd4.is_unicast()) {
	verbose_log("%s failed is_unicast test.\n", odd4.str().c_str());
	return false;
    }
    if (!deflt.is_unicast()) {
	verbose_log("%s failed is_unicast test.\n", deflt.str().c_str());
	return false;
    }
    verbose_log("Passed is_unicast test.\n");
    return true;
}

static bool
v4_overlap_test()
{
    // Address that overlap with netmask
    struct x {
	const char* a;
	const char* b;
	size_t	    overlap;
    } data[] = {
	{ "180.10.10.0", "180.10.8.0",   22 },
	{ "180.10.10.0", "180.10.200.0", 16 },
	{ "180.10.20.0", "180.255.200.0", 8 },
    };
    
    for (size_t i = 0; i < sizeof(data) / sizeof(data[0]); i++) {
	for (size_t p = 0; p < 32; p++) {
	    IPv4 a(data[i].a);
	    IPv4 b(data[i].b);
	    IPv4Net u(a, p);
	    IPv4Net v(b, p);
	    if (u.is_overlap(v) != (p <= data[i].overlap)) {
		verbose_log("bad overlap %u\n", XORP_UINT_CAST(p));
		return -1;
	    }
	}
    }
    verbose_log("Passed overlap test.\n");
    return true;
}

static bool
test_ipv4net_okay()
{
    return (v4_serialization_test() == true) &
	(v4_less_than_test() == true) &
	(v4_overlap_test() == true) &
	(v4_is_unicast_test() == true);
}

// ----------------------------------------------------------------------------
// Usage

static void
usage(const char* argv0)
{
    fprintf(stderr, "usage: %s [-v]\n", argv0);
    fprintf(stderr, "A test program for XORP ipnet classes\n");
}

// ----------------------------------------------------------------------------
// main

int main(int argc, char* const* argv)
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
	    return -1;
	}
    }
    argc -= optind;
    argv += optind;

    int r = 0;
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    try {
	if (test_ipv4net_okay() == false) {
	    r = -2;
	}
    } catch (...) {
	xorp_catch_standard_exceptions();
	r = -3;
    }
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return r;
}
