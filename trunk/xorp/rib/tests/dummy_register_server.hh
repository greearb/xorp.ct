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

// $XORP: xorp/rib/dummy_register_server.hh,v 1.14 2008/10/02 21:58:09 bms Exp $

#ifndef __RIB_DUMMY_REGISTER_SERVER_HH__
#define __RIB_DUMMY_REGISTER_SERVER_HH__

#include <set>

#include "register_server.hh"


class DummyRegisterServer : public RegisterServer {
public:
    DummyRegisterServer();
    void send_route_changed(const string& module_name,
			    const IPv4Net& net,
			    const IPv4& nexthop,
			    uint32_t metric,
			    uint32_t admin_distance,
			    const string& protocol_origin,
			    bool multicast);
    void send_invalidate(const string& module_name,
			 const IPv4Net& net,
			 bool multicast);
    void send_route_changed(const string& module_name,
			    const IPv6Net& net, 
			    const IPv6& nexthop,
			    uint32_t metric,
			    uint32_t admin_distance,
			    const string& protocol_origin,
			    bool multicast);
    void send_invalidate(const string& module_name,
			 const IPv6Net& net,
			 bool multicast);
    void flush() {}
    bool verify_invalidated(const string& invalid);
    bool verify_changed(const string& changed);
    bool verify_no_info();

private:
    set<string> _invalidated;
    set<string> _changed;
};

#endif // __RIB_DUMMY_REGISTER_SERVER_HH__
