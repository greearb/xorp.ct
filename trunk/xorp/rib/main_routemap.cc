// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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



#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "routemap.hh"
#include "route.hh"


int main(int /* argc */, char* argv[]) {
    RouteMap* rm;
    rm = new RouteMap("testmap");
    
    RMRule* rr;
    RMMatchIPAddr* rmatchip;
    RMAction* ra;

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



    RMRule* rr2;
    RMMatchIPAddr* rmatchip2;
    RMAction* ra2;

    IPv4 ip2("192.150.186.0");
    IPv4Net ipnet2(ip2, 24);
    rmatchip2 = new RMMatchIPAddr(ipnet2);
    ra2 = new RMAction();

    rr2 = new RMRule(20, rmatchip2, ra2);

    rm->add_rule(rr2);



    RMRule* rr3;
    RMMatchIPAddr* rmatchip3;
    RMAction* ra3;

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
