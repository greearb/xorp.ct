// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

#ident "$XORP$"

/*
 * EXIT CODES:
 *
 * 0: filter rejected route
 * 1: filter accepted route
 * 2: Policy exception
 * 3: error
 *
 */

#include "config.h"

#include <sys/time.h>
#include <string>
#include <iostream>

#include "policy/backend/policy_filter.hh"
#include "policy/common/policy_utils.hh"

#include "file_varrw.hh"


using namespace policy_utils;


int main(int argc, char *argv[]) {

    if(argc < 3) {
	cout << "Usage: " << argv[0] << " <policy> <varfile>" << endl;
	exit(3);
    }

    string conf = "";

    bool accepted = true;

    struct timeval tv_start;

    if(gettimeofday(&tv_start,NULL)) {
	perror("gettimeofday()");
	exit(3);
    }

try {
    read_file(argv[1],conf);
} catch(const PolicyException& e) {
    cout << "May not read file: " << e.str() << endl;
    exit(3);
}

try {
    cout << "Configuring varrw..." << endl;
    FileVarRW varrw(argv[2]);

    PolicyFilter filter;
    cout << "Configuring filter..." << endl;

    filter.configure(conf);

  
    cout << "Running filter..." << endl;
    accepted = filter.acceptRoute(varrw,&cout);

    cout << "Filter " << ( accepted ? "accepted" : "rejected") 
	 << " route" << endl;


} catch(const FileVarRW::Error& e) {
    cout << "May not configure filevarrw " << e.str() << endl;
    exit(3);
} catch(const PolicyException& e) {
    cout << "PolicyException: " << e.str() << endl;
    exit(2);
}

    struct timeval tv_end;

    if(gettimeofday(&tv_end,NULL)) {
	perror("gettimeofday()");
	exit(3);
    }

    long usec = tv_end.tv_usec - tv_start.tv_usec;
    long sec = tv_end.tv_sec - tv_start.tv_sec;

    double speed = ((double)usec/1000) + ((double)sec*1000);

    printf("Execution successful in %.3f milliseconds\n",speed);
 
    if(accepted)
	exit(1);
    exit(0);
}
