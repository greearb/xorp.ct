// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/policy/test/execpolicy.cc,v 1.10 2008/07/23 05:11:28 pavlin Exp $"

/*
 * EXIT CODES:
 *
 * 0: filter rejected route
 * 1: filter accepted route
 * 2: Policy exception
 * 3: error
 *
 */

#include "policy/policy_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/timeval.hh"
#include "libxorp/clock.hh"
#include "libxorp/timer.hh"

#include <iostream>

#include "policy/common/policy_utils.hh"
#include "policy/backend/policy_filter.hh"

#include "file_varrw.hh"


using namespace policy_utils;

int main(int argc, char *argv[]) {

    if(argc < 3) {
	cout << "Usage: " << argv[0] << " <policy> <varfile>" << endl;
	exit(3);
    }

    string conf = "";

    bool accepted = true;

    SystemClock sc;
    TimerList timerlist(&sc);

    TimeVal start;
    TimerList::system_gettimeofday(&start);

    xlog_init(argv[0], 0);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

try {
    read_file(argv[1],conf);
} catch(const PolicyException& e) {
    cout << "May not read file: " << e.str() << endl;
    exit(3);
}

try {
    cout << "Configuring varrw..." << endl;
    FileVarRW varrw;
    
    varrw.load(argv[2]);

    PolicyFilter filter;
    cout << "Configuring filter..." << endl;

    filter.configure(conf);

  
    cout << "Running filter..." << endl;
    accepted = filter.acceptRoute(varrw);

    cout << "Filter " << ( accepted ? "accepted" : "rejected") 
	 << " route" << endl;


} catch(const FileVarRW::Error& e) {
    cout << "May not configure filevarrw " << e.str() << endl;
    exit(3);
} catch(const PolicyException& e) {
    cout << "PolicyException: " << e.str() << endl;
    exit(2);
}

    TimeVal elapsed;
    TimerList::system_gettimeofday(&elapsed);
    elapsed -= start;

    printf("Execution successful in %d milliseconds\n", elapsed.to_ms());

    xlog_stop();
    xlog_exit();
 
    if(accepted)
	exit(1);
    exit(0);
}
