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

#ident "$XORP: xorp/bgp/test_main.cc,v 1.1 2003/02/10 23:40:09 mjh Exp $"

#include <stdio.h>
#include "bgp_module.h"
#include "config.h"

bool test_ribin(int, char**);
bool test_deletion(int, char**);
bool test_filter(int, char**);
bool test_cache(int, char**);
bool test_nhlookup(int, char**);
bool test_decision(int, char**);
bool test_fanout(int, char**);
bool test_dump(int, char**);
bool test_ribout(int, char**);

int main(int argc, char** argv) 
{
    bool test_ribin_succeeded = test_ribin(argc, argv);
    bool test_deletion_succeeded = test_deletion(argc, argv);
    bool test_filter_succeeded = test_filter(argc, argv);
    bool test_cache_succeeded = test_cache(argc, argv);
    bool test_nhlookup_succeeded = test_nhlookup(argc, argv);
    bool test_decision_succeeded = test_decision(argc, argv);
    bool test_fanout_succeeded = test_fanout(argc, argv);
    bool test_dump_succeeded = test_dump(argc, argv);
    bool test_ribout_succeeded = test_ribout(argc, argv);


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
    return status;
}



