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

#ident "$XORP: xorp/rib/main_routemap.cc,v 1.10 2002/12/09 18:29:32 hodson Exp $"

#include "urib_module.h"
#include "libxorp/xlog.h"
#include "routemap.hh"
#include "route.hh"

int main(int /* argc */, char *argv[]) {
    RouteMap *rm;
    rm = new RouteMap("testmap");
    
    RMRule *rr;
    RMMatchIPAddr *rmatchip;
    RMAction *ra;

    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_LOW);		// Least verbose messages
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();
    
    
    IPv4 ip1("192.150.187.0");
    IPv4Net ipnet1(ip1, 24);
    rmatchip = new RMMatchIPAddr(ipnet1);
    ra = new RMAction();

    rr = new RMRule(10, rmatchip, ra);

    rm->add_rule(rr);



    RMRule *rr2;
    RMMatchIPAddr *rmatchip2;
    RMAction *ra2;

    IPv4 ip2("192.150.186.0");
    IPv4Net ipnet2(ip2, 24);
    rmatchip2 = new RMMatchIPAddr(ipnet2);
    ra2 = new RMAction();

    rr2 = new RMRule(20, rmatchip2, ra2);

    rm->add_rule(rr2);



    RMRule *rr3;
    RMMatchIPAddr *rmatchip3;
    RMAction *ra3;

    IPv4 ip3("193.150.186.0");
    IPv4Net ipnet3(ip3, 24);
    rmatchip3 = new RMMatchIPAddr(ipnet3);
    ra3 = new RMAction();

    rr3 = new RMRule(15, rmatchip3, ra3);

    rm->add_rule(rr3);

    cout << rm->str() << "\n";

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();

    exit(0);
}
