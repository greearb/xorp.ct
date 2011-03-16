// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



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
