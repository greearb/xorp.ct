// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/test_ipnet.cc,v 1.2 2003/01/26 04:06:21 pavlin Exp $"

#include <unistd.h>
#include <stdio.h>

#include "config.h"
#include "xorp.h"

#include "libxorp_module.h"
#include "exceptions.hh"
#include "ipv4net.hh"
#include "xlog.h"

// ----------------------------------------------------------------------------
// Verbose output

static bool s_verbose = false;

inline bool verbose() 		{ return s_verbose; }
inline void set_verbose(bool v)	{ s_verbose = v; }

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
	uint32_t	prefix;
    } srep[] = {
	{ "128.16.92.160/27", IPv4(htonl_literal(0x80105ca0U)), 27 },
	{ "128.16.0.0/16", IPv4(htonl_literal(0x80100000U)), 16 },
	{ "255.255.255.255/32", IPv4(htonl_literal(0xffffffffU)), 32},
	{ "0.0.0.0/16", IPv4(htonl_literal(0x00000000U)), 16 },
    };

    for (size_t i = 0; i < sizeof(srep) / sizeof(srep[0]); i++) {
	IPv4Net n(srep[i].net);
	if (n.prefix_len() != srep[i].prefix) {
	    verbose_log("item %u bad prefix %u\n", (uint32_t)i,
			(uint32_t)n.prefix_len());
	    return false;
	} else if (n.masked_addr() != srep[i].v4) {
	    verbose_log("item %u bad addr %s != %s\n", 
			(uint32_t)i, n.masked_addr().str().c_str(),
			srep[i].v4.str().c_str());
	    return false;
	} else if (n.netmask() != IPv4::make_prefix(n.prefix_len())) {
	    verbose_log("item %u bad netmask %s != %s\n",
			(uint32_t)i, n.netmask().str().c_str(),
			IPv4::make_prefix(n.prefix_len()).str().c_str());
	    return false;
	}

	IPv4Net u (n.str().c_str());
	if (u != n) {
	    verbose_log("item %u to string and back failed.", (uint32_t)i);
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
		verbose_log("bad overlap %u\n", (uint32_t)p);
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
	(v4_overlap_test() == true);
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
