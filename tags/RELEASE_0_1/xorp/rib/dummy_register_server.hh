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

// $XORP: xorp/rib/dummy_register_server.hh,v 1.4 2002/12/09 18:29:32 hodson Exp $

#ifndef __RIB_DUMMY_REGISTER_SERVER_HH__
#define __RIB_DUMMY_REGISTER_SERVER_HH__

#include <set>
#include "register_server.hh"

class DummyRegisterServer : public RegisterServer {
public:
    DummyRegisterServer();
    void send_route_changed(const string& modname,
			    IPNet<IPv4> net, 
			    IPv4 nexthop,
			    uint32_t metric,
			    bool multicast);
    void send_invalidate(const string& modname,
			 IPNet<IPv4> net,
			 bool multicast);

    void send_route_changed(const string& modname,
			    IPNet<IPv6> net, 
			    IPv6 nexthop,
			    uint32_t metric,
			    bool multicast);
    void send_invalidate(const string& modname,
			 IPNet<IPv6> net,
			 bool multicast);
    void flush() {};
    bool verify_invalidated(const string& invalid);
    bool verify_changed(const string& changed);
    bool verify_no_info();
protected:
    set <string> _invalidated;
    set <string> _changed;
};

#endif // __RIB_DUMMY_REGISTER_SERVER_HH__
