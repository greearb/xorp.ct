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

#ident "$XORP: xorp/libxipc/test_header.cc,v 1.2 2002/12/19 01:29:10 hodson Exp $"

#include <string>
#include <stdio.h>

#include "xrl_module.h"
#include "libxorp/xlog.h"
#include "header.hh"

void
run_test()
{
    HeaderWriter w;

    const string src_text = "containing some text";
    w.add("a string", src_text);

    const int src_int = 3;
    w.add("an int", src_int);

    const double src_double = 4.33;
    w.add("a double", src_double);

    string wsrep = w.str();
    {
	HeaderReader r(wsrep);
	string s;
	int i;
	double d;

	r.get("a string", s).get("an int", i).get("a double", d);

	if (s != src_text) {
	    fprintf(stderr, "failed string %s != %s",
		    s.c_str(), src_text.c_str());
	    exit(-1);
	} else if (i != src_int) {
	    fprintf(stderr, "failed int %d != %d", i, src_int);
	    exit(-1);
	} else if (d != src_double) {
	    fprintf(stderr, "failed double %f != %f", d, src_double);
	    exit(-1);
	}

	try {
	    r.get("foo", s);
	    printf("failed: incorrect match\n");
	    exit(-1);
	} catch (const HeaderReader::NotFound& e) {
	    //	    printf("okay\n");
	}
    }

    try {
	wsrep += "XXXXXX";
	HeaderReader r(wsrep);
	//	printf("okay\n");
    } catch (const HeaderReader::InvalidString &e) {
	printf("failed: junk stopped operation\n.");
	exit(-1);
    }

    try {
	wsrep.resize(10);
	HeaderReader r(wsrep);
	printf("failed: accepted incomplete string\n");
	exit(-1);
    } catch (const HeaderReader::InvalidString &e) {
	//	printf("okay\n");
    }
}

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

    run_test();

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    return 0;
}
