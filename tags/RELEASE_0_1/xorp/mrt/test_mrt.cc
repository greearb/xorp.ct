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

#ident "$XORP: xorp/mrt/test_mrt.cc,v 1.19 2002/12/09 18:29:23 hodson Exp $"


//
// Multicast Routing Table test program.
//


#include "mrt_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "mrt/mrt.hh"


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

class MyMre : public Mre<MyMre> {
public:
    MyMre(const IPvX& source, const IPvX& group)
	: Mre<MyMre>(source, group) {}
};

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
	Mrt<MyMre> mrt;
	IPvX s1(IPv4("123.45.0.1")),   g1(IPv4("224.0.0.2"));
	IPvX s2(IPv4("123.45.0.2")),   g2(IPv4("224.0.0.1"));
	IPvX s3(IPv4("123.45.0.255")), g3(IPv4("224.0.0.255"));
	IPvX s4(IPv4("123.46.0.1")),   g4("225.0.0.1");
	IPvX s5(IPv4("123.46.0.1")),   g5("225.0.0.2");
	MyMre *mre1 = new MyMre(s1, g1);
	MyMre *mre11 = new MyMre(s1, g1);
	MyMre *mre2 = new MyMre(s2, g2);
	MyMre *mre4 = new MyMre(s4, g4);
	MyMre *mre5 = new MyMre(s5, g5);
	MyMre *t;
	
	// Entries insertion
	printf("\n");
	printf("Installing entry for %s\n", cstring(*mre1));
	mre1 = mrt.insert(mre1);
	printf("PASS\n");
	printf("Installing entry for %s\n", cstring(*mre11));
	mre11 = mrt.insert(mre11);
	printf("PASS\n");
	printf("Installing entry for %s\n", cstring(*mre2));
	mre2 = mrt.insert(mre2);
	printf("PASS\n");
	printf("Installing entry for %s\n", cstring(*mre4));
	mre4 = mrt.insert(mre4);
	printf("PASS\n");
	printf("Installing entry for %s\n", cstring(*mre5));
	mre5 = mrt.insert(mre5);
	printf("PASS\n");
	
	// Entries lookup
	printf("\n");
	printf("Searching for   (%s, %s)\n", cstring(s1), cstring(g1));
	t = mrt.find(s1, g1);
	if (t != NULL) {
	    printf("Found entry for %s\n", cstring(*t));
	    printf("PASS\n");
	} else {
	    printf("Entry not found!\n");
	    printf("FAIL\n");
	}
	
	// Lookup for non-existing entry
	printf("\n");
	printf("Searching for   (%s, %s)\n", cstring(s3), cstring(g3));
	t = mrt.find(s3, g3);
	if (t == NULL) {
	    printf("PASS\n");
	} else {
	    printf("Found entry for %s\n", cstring(*t));
	    printf("FAIL\n");
	}
	
	// Mrt size
	printf("\n");
	printf("Mrt size = %d\n", mrt.size());
	printf("PASS\n");
	
	// All entries printout
	printf("\n");
	printf("List of all entries ordered by source address first:\n");
	for (Mrt<MyMre>::const_sg_iterator iter = mrt.sg_begin();
	     iter != mrt.sg_end(); ++iter) {
	    t = iter->second;
	    // printf("%s\n", cstring(*t));
	    printf("%s\n", cstring(*t));
	}
	printf("PASS\n");
	
	printf("\n");
	printf("List of all entries ordered by group address first :\n");
	for (Mrt<MyMre>::const_gs_iterator iter = mrt.gs_begin();
	     iter != mrt.gs_end(); ++iter) {
	    t = iter->second;
	    // printf("%s\n", cstring(*t));
	    printf("%s\n", cstring(*t));
	}
	printf("PASS\n");
	
	IPvXNet s_prefix1(s1, 15);
	printf("List of all entries that match source prefix %s :\n",
	       s_prefix1.str().c_str());
	Mrt<MyMre>::const_sg_iterator iter_end1 = mrt.source_by_prefix_end(s_prefix1);
	for (Mrt<MyMre>::const_sg_iterator iter = mrt.source_by_prefix_begin(s_prefix1);
	     iter != iter_end1; ++iter) {
	    t = iter->second;
	    printf("%s\n", cstring(*t));
	}
	printf("PASS\n");
	
	IPvXNet s_prefix2(s1, 0);
	printf("List of all entries that match source prefix %s :\n",
	       s_prefix2.str().c_str());
	Mrt<MyMre>::const_sg_iterator iter_end2 = mrt.source_by_prefix_end(s_prefix2);
	for (Mrt<MyMre>::const_sg_iterator iter = mrt.source_by_prefix_begin(s_prefix2);
	     iter != iter_end2; ++iter) {
	    t = iter->second;
	    printf("%s\n", cstring(*t));
	}
	printf("PASS\n");
	
    } catch(...) {
	xorp_catch_standard_exceptions();
    }
    
    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();
    
    exit (0);
}
