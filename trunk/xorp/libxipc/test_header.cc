// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/libxipc/test_header.cc,v 1.12 2008/10/02 21:57:22 bms Exp $"

#include "xrl_module.h"
#include "libxorp/xorp.h"

#include "libxorp/xlog.h"

#include "header.hh"

void
run_test()
{
    HeaderWriter w;

    const string src_text = "containing some text";
    w.add("a string", src_text);

    const int32_t src_int = 3;
    w.add("an int", src_int);

    const double src_double = 4.33;
    w.add("a double", src_double);

    string wsrep = w.str();
    {
	HeaderReader r(wsrep);
	string s;
	int32_t i;
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
