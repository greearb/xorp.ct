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

#ident "$XORP: xorp/bgp/test_main.cc,v 1.9 2004/03/04 03:21:57 atanu Exp $"

#include <stdio.h>
#include "bgp_module.h"
#include "config.h"
#include "libxorp/xlog.h"
#include "test_next_hop_resolver.hh"

bool test_ribin(TestInfo& info);
bool test_ribin_dump(TestInfo& info);
bool test_deletion(TestInfo& info);
bool test_filter(TestInfo& info);
bool test_cache(TestInfo& info);
bool test_nhlookup(TestInfo& info);
bool test_decision(TestInfo& info);
bool test_fanout(TestInfo& info);
bool test_dump_create(TestInfo& info);
bool test_dump(TestInfo& info);
bool test_ribout(TestInfo& info);
template <class A> bool test_subnet_route1(TestInfo& info, IPNet<A> net);
template <class A> bool test_subnet_route2(TestInfo& info, IPNet<A> net);

bool
validate_reference_file(string reference_file, string output_file,
			string testname)
{
    FILE *file = fopen(output_file.c_str(), "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read %s\n", output_file.c_str());
	fprintf(stderr, "TEST %s FAILED\n", testname.c_str());
	fclose(file);
	return false;
    }
#define BUFSIZE 8192
    char testout[BUFSIZE];
    memset(testout, 0, BUFSIZE);
    int bytes1 = fread(testout, 1, BUFSIZE, file);
    if (bytes1 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST %s FAILED\n", testname.c_str());
	fclose(file);
	return false;
    }
    fclose(file);

    string ref_filename;
    const char* srcdir = getenv("srcdir");
    if (srcdir) {
	ref_filename = string(srcdir); 
    } else {
	ref_filename = ".";
    }
    ref_filename += reference_file;
    file = fopen(ref_filename.c_str(), "r");
    if (file == NULL) {
	fprintf(stderr, "Failed to read %s\n", ref_filename.c_str());
	fprintf(stderr, "TEST %s FAILED\n", testname.c_str());
	return false;
    }
    char refout[BUFSIZE];
    memset(refout, 0, BUFSIZE);
    int bytes2 = fread(refout, 1, BUFSIZE, file);
    if (bytes2 == BUFSIZE) {
	fprintf(stderr, "Output too long for buffer\n");
	fprintf(stderr, "TEST %s FAILED\n", testname.c_str());
	fclose(file);
	return false;
    }
    fclose(file);
    
    if ((bytes1 != bytes2) || (memcmp(testout, refout, bytes1)!= 0)) {
	fprintf(stderr, "Output in %s doesn't match reference output\n",
		output_file.c_str());
	fprintf(stderr, "TEST %s FAILED\n", testname.c_str());
	return false;
	
    }
    unlink(output_file.c_str());

    return true;
}

int
main(int argc, char** argv) 
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);

    TestMain t(argc, argv);

    string test_name =
	t.get_optional_args("-t", "--test", "run only the specified test");
    t.complete_args_parsing();

    try {
	/*
	** For next hop resolver tests.
	*/
	IPv4 nh4("128.16.64.1");
	IPv4 rnh4("1.1.1.1");
	IPv4Net nlri4("22.0.0.0/8");
	IPv6 nh6("::128.16.64.1");
	IPv6 rnh6("::1.1.1.1");
	IPv6Net nlri6("::22.0.0.0/8");
	
	IPv4Net route4("128.16.0.0/24");
	IPv6Net route6("1::0/24");
	const int iter = 1000;

	struct test {
	    string test_name;
	    XorpCallback1<bool, TestInfo&>::RefPtr cb;
	} tests[] = {
	    {"RibIn", callback(test_ribin)},
	    {"RibInDump", callback(test_ribin_dump)},
	    {"Deletion", callback(test_deletion)},
	    {"Filter", callback(test_filter)},
	    {"Cache", callback(test_cache)},
	    {"NhLookup", callback(test_nhlookup)},
	    {"Decision", callback(test_decision)},
	    {"Fanout", callback(test_fanout)},
	    {"DumpCreate", callback(test_dump_create)},
	    {"Dump", callback(test_dump)},
	    {"Ribout", callback(test_ribout)},
	    {"SubnetRoute1", callback(test_subnet_route1<IPv4>, route4)},
	    {"SubnetRoute1.ipv6", callback(test_subnet_route1<IPv6>, route6)},
	    {"SubnetRoute2", callback(test_subnet_route2<IPv4>, route4)},
	    {"SubnetRoute2.ipv6", callback(test_subnet_route2<IPv6>, route6)},

	    {"nhr.test1", callback(nhr_test1<IPv4>, nh4, rnh4, nlri4)},
	    {"nhr.test1.ipv6", callback(nhr_test1<IPv6>, nh6, rnh6, nlri6)},

	    {"nhr.test2", callback(nhr_test2<IPv4>, nh4, rnh4, nlri4, iter)},
	    {"nhr.test2.ipv6", callback(nhr_test2<IPv6>, nh6, rnh6, nlri6,
					iter)},

	    {"nhr.test3", callback(nhr_test3<IPv4>, nh4, rnh4, nlri4, iter)},
	    {"nhr.test3.ipv6", callback(nhr_test3<IPv6>, nh6, rnh6, nlri6,
					iter)},

	    {"nhr.test4", callback(nhr_test4<IPv4>, nh4, rnh4, nlri4)},
	    {"nhr.test4.ipv6", callback(nhr_test4<IPv6>, nh6, rnh6, nlri6)},

	    {"nhr.test5", callback(nhr_test5<IPv4>, nh4, rnh4, nlri4)},
	    {"nhr.test5.ipv6", callback(nhr_test5<IPv6>, nh6, rnh6, nlri6)},

	    {"nhr.test6", callback(nhr_test6<IPv4>, nh4, rnh4, nlri4)},
	    {"nhr.test6.ipv6", callback(nhr_test6<IPv6>, nh6, rnh6, nlri6)},

	    {"nhr.test7", callback(nhr_test7<IPv4>, nh4, rnh4, nlri4)},
	    {"nhr.test7.ipv6", callback(nhr_test7<IPv6>, nh6, rnh6, nlri6)},

	    {"nhr.test8", callback(nhr_test8<IPv4>, nh4, rnh4, nlri4)},
	    {"nhr.test8.ipv6", callback(nhr_test8<IPv6>, nh6, rnh6, nlri6)},
	};

	if("" == test_name) {
	    for(unsigned int i = 0; i < sizeof(tests) / sizeof(struct test); 
		i++)
		t.run(tests[i].test_name, tests[i].cb);
	} else {
	    for(unsigned int i = 0; i < sizeof(tests) / sizeof(struct test); 
		i++)
		if(test_name == tests[i].test_name) {
		    t.run(tests[i].test_name, tests[i].cb);
		    return t.exit();
		}
	    t.failed("No test with name " + test_name + " found\n");
	}
    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    return t.exit();
}
