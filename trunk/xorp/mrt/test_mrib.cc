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

#ident "$XORP: xorp/mrt/test_mrib.cc,v 1.3 2003/03/10 23:20:45 hodson Exp $"


//
// Multicast Routing Information Base information test program.
//


#include "mrt_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "mrt/mrib_table.hh"


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//

//
// Local variables
//

//
// Local functions prototypes
//
static int	run_test1();
static int	run_test2();
static int	run_test3();


int
main(int /* argc */, char *argv[])
{
    
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();
    
    try {
	//
	// Run the tests
	//
	run_test1();
	run_test2();
	run_test3();
    } catch(...) {
	xorp_catch_standard_exceptions();
    }
    
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();
    
    exit(0);
}

static int
run_test1()
{
    Mrib *t;
    MribTable mrib_table(AF_INET);
    IPvX s1(IPv4("123.45.0.1"));
    IPvX s2(IPv4("123.45.0.2"));
    IPvX s3(IPv4("123.45.0.255"));
    IPvX s4(IPv4("123.45.1.255"));
    Mrib *mrib1 = new Mrib(IPvXNet(s1, 24));
    Mrib *mrib2 = new Mrib(IPvXNet(s2, 24));
    Mrib *mrib3 = new Mrib(IPvXNet(s3, 24));

    printf("\n\n");
    printf("RUNNING TEST1\n");
    
    // Entries insertion
    printf("\n");
    printf("Installing entry for %s\n", cstring(mrib1->dest_prefix()));
    mrib_table.insert(*mrib1);
    printf("PASS\n");
    printf("Installing entry for %s\n", cstring(mrib2->dest_prefix()));
    mrib_table.insert(*mrib2);
    printf("PASS\n");
    printf("Installing entry for %s\n", cstring(mrib3->dest_prefix()));
    mrib_table.insert(*mrib3);
    printf("PASS\n");
    
    // Entries lookup
    printf("\n");
    printf("Searching for   %s\n", cstring(s1));
    t = mrib_table.find(s1);
    if (t != NULL) {
	printf("Found entry: %s\n", cstring(t->dest_prefix()));
	printf("PASS\n");
    } else {
	printf("Entry not found!\n");
	printf("FAIL\n");
    }
    
    // Lookup for non-existing entry
    printf("\n");
    printf("Searching for   %s\n", cstring(s4));
    t = mrib_table.find(s4);
    if (t == NULL) {
	printf("PASS\n");
    } else {
	printf("Found entry: %s\n", cstring(t->dest_prefix()));
	printf("FAIL\n");
    }
    
    // MribTable size
    printf("\n");
    printf("MribTable size = %u\n", (uint32_t)mrib_table.size());
    printf("PASS\n");
    
    // All entries printout
    printf("\n");
    printf("List of all entries:\n");
    for (MribTable::iterator iter = mrib_table.begin();
	 iter != mrib_table.end(); ++iter) {
	t = *iter;
	if (t != NULL)
	    printf("%s\n", cstring(t->dest_prefix()));
    }
    printf("PASS\n");
    
    return (0);
}

static int
run_test2()
{
    Mrib *t;
    MribTable mrib_table(AF_INET);
    IPvX s1(IPv4("1.1.0.0"));
    IPvX s2(IPv4("1.2.0.0"));
    IPvX s3(IPv4("1.2.0.0"));
    Mrib  *mrib1 = new Mrib(IPvXNet(s1, 16));
    Mrib  *mrib2 = new Mrib(IPvXNet(s2, 24));
    Mrib  *mrib3 = new Mrib(IPvXNet(s3, 16));
    IPvX s4(IPv4("1.2.0.1"));
    IPvX s5(IPv4("1.2.1.1"));
    
    printf("\n\n");
    printf("RUNNING TEST2\n");
    
    // Entries insertion
    printf("\n");
    printf("Installing entry for %s\n", cstring(mrib1->dest_prefix()));
    mrib_table.insert(*mrib1);
    printf("PASS\n");
    printf("Installing entry for %s\n", cstring(mrib2->dest_prefix()));
    mrib_table.insert(*mrib2);
    printf("PASS\n");
    printf("Installing entry for %s\n", cstring(mrib3->dest_prefix()));
    mrib_table.insert(*mrib3);
    printf("PASS\n");
    
    // Entries lookup
    printf("\n");
    printf("Searching for   %s\n", cstring(s4));
    t = mrib_table.find(s4);
    if (t != NULL) {
	printf("Found entry: %s\n", cstring(t->dest_prefix()));
	printf("PASS\n");
    } else {
	printf("Entry not found!\n");
	printf("FAIL\n");
    }

    printf("\n");
    printf("Searching for   %s\n", cstring(s5));
    t = mrib_table.find(s5);
    if (t != NULL) {
	printf("Found entry: %s\n", cstring(t->dest_prefix()));
	printf("PASS\n");
    } else {
	printf("Entry not found!\n");
	printf("FAIL\n");
    }
    
    // All entries printout
    printf("\n");
    printf("List of all entries:\n");
    for (MribTable::iterator iter = mrib_table.begin();
	 iter != mrib_table.end(); ++iter) {
	t = *iter;
	if (t != NULL)
	    printf("%s\n", cstring(t->dest_prefix()));
    }
    printf("PASS\n");
    
    return (0);
}

static int
run_test3()
{
    Mrib *t;
    MribTable mrib_table(AF_INET);
    IPvX s1(IPv4("1.1.0.0"));
    IPvX s2(IPv4("1.2.1.0"));
    IPvX s3(IPv4("1.2.0.0"));
    Mrib  *mrib1 = new Mrib(IPvXNet(s1, 16));
    Mrib  *mrib2 = new Mrib(IPvXNet(s2, 24));
    Mrib  *mrib3 = new Mrib(IPvXNet(s3, 16));
    IPvX s4(IPv4("1.2.0.1"));
    IPvX s5(IPv4("1.2.1.1"));

    printf("\n\n");
    printf("RUNNING TEST3\n");
    
    // Entries insertion
    printf("\n");
    printf("Installing entry for %s\n", cstring(mrib1->dest_prefix()));
    mrib_table.insert(*mrib1);
    printf("PASS\n");
    printf("Installing entry for %s\n", cstring(mrib2->dest_prefix()));
    mrib_table.insert(*mrib2);
    printf("PASS\n");
    printf("Installing entry for %s\n", cstring(mrib3->dest_prefix()));
    mrib_table.insert(*mrib3);
    printf("PASS\n");
    
    // Entries lookup
    printf("\n");
    printf("Searching for   %s\n", cstring(s4));
    t = mrib_table.find(s4);
    if (t != NULL) {
	printf("Found entry: %s\n", cstring(t->dest_prefix()));
	printf("PASS\n");
    } else {
	printf("Entry not found!\n");
	printf("FAIL\n");
    }

    printf("\n");
    printf("Searching for   %s\n", cstring(s5));
    t = mrib_table.find(s5);
    if (t != NULL) {
	printf("Found entry: %s\n", cstring(t->dest_prefix()));
	printf("PASS\n");
    } else {
	printf("Entry not found!\n");
	printf("FAIL\n");
    }
    
    // All entries printout
    printf("\n");
    printf("List of all entries:\n");
    for (MribTable::iterator iter = mrib_table.begin();
	 iter != mrib_table.end(); ++iter) {
	t = *iter;
	if (t != NULL)
	    printf("%s\n", cstring(t->dest_prefix()));
    }
    printf("PASS\n");
    
    return (0);
}
