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

#ident "$XORP: xorp/bgp/test_main.cc,v 1.4 2003/02/12 20:24:26 mjh Exp $"

#include <stdio.h>
#include "bgp_module.h"
#include "config.h"
#include "libxorp/xlog.h"

bool test_ribin();
bool test_deletion();
bool test_filter();
bool test_cache();
bool test_nhlookup();
bool test_decision();
bool test_fanout();
bool test_dump();
bool test_ribout();
bool test_next_hop_resolver(int, char**);

int main(int argc, char** argv) 
{
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    xlog_add_default_output();
    xlog_start();

    bool test_ribin_succeeded = test_ribin();
    bool test_deletion_succeeded = test_deletion();
    bool test_filter_succeeded = test_filter();
    bool test_cache_succeeded = test_cache();
    bool test_nhlookup_succeeded = test_nhlookup();
    bool test_decision_succeeded = test_decision();
    bool test_fanout_succeeded = test_fanout();
    bool test_dump_succeeded = test_dump();
    bool test_ribout_succeeded = test_ribout();
    bool test_next_hop_resolver_succeeded 
	= test_next_hop_resolver(argc, argv);


    bool status = 0;
    if (test_ribin_succeeded) {
	printf("Test RibIn: PASS\n");
    } else {
	status = -1;
	printf("Test RibIn: FAIL\n");
    }
    if (test_deletion_succeeded) {
	printf("Test Deletion: PASS\n");
    } else {
	status = -1;
	printf("Test Deletion: FAIL\n");
    }
    if (test_filter_succeeded) {
	printf("Test Filter: PASS\n");
    } else {
	status = -1;
	printf("Test Filter: FAIL\n");
    }
    if (test_cache_succeeded) {
	printf("Test Cache: PASS\n");
    } else {
	status = -1;
	printf("Test Cache: FAIL\n");
    }
    if (test_nhlookup_succeeded) {
	printf("Test NhLookup: PASS\n");
    } else {
	status = -1;
	printf("Test NhLookup: FAIL\n");
    }
    if (test_cache_succeeded) {
	printf("Test Cache: PASS\n");
    } else {
	status = -1;
	printf("Test Cache: FAIL\n");
    }
    if (test_decision_succeeded) {
	printf("Test Decision: PASS\n");
    } else {
	status = -1;
	printf("Test Decision: FAIL\n");
    }
    if (test_fanout_succeeded) {
	printf("Test Fanout: PASS\n");
    } else {
	status = -1;
	printf("Test Fanout: FAIL\n");
    }
    if (test_dump_succeeded) {
	printf("Test Dump: PASS\n");
    } else {
	status = -1;
	printf("Test Dump: FAIL\n");
    }
    if (test_ribout_succeeded) {
	printf("Test Ribout: PASS\n");
    } else {
	status = -1;
	printf("Test Ribout: FAIL\n");
    }
    if (test_next_hop_resolver_succeeded) {
	printf("Test NextHopResolver: PASS\n");
    } else {
	status = -1;
	printf("Test NextHopResolver: FAIL\n");
    }
    return status;
}



