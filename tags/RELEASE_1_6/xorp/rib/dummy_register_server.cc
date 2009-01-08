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

#ident "$XORP: xorp/rib/dummy_register_server.cc,v 1.17 2008/10/02 21:58:09 bms Exp $"

#include "rib_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "dummy_register_server.hh"


extern bool verbose;


DummyRegisterServer::DummyRegisterServer()
    : RegisterServer(NULL)
{
}


void 
DummyRegisterServer::send_route_changed(const string& module_name,
					const IPv4Net& net, 
					const IPv4& nexthop,
					uint32_t metric,
					uint32_t admin_distance,
					const string& protocol_origin,
					bool multicast)
{
    string s;
    
    if (verbose) {
	printf("DummyRegisterServer::send_route_changed module=%s net=%s nexthop=%s, metric=%u admin_distance=%u protocol_origin=%s",
	       module_name.c_str(), net.str().c_str(), nexthop.str().c_str(),
	       XORP_UINT_CAST(metric), XORP_UINT_CAST(admin_distance),
	       protocol_origin.c_str());
    }
    
    s = module_name + " " + net.str() + " " + nexthop.str();
    s += " " + c_format("%u", XORP_UINT_CAST(metric));
    if (multicast) {
	if (verbose)
	    printf("mcast=true\n");
	s += " mcast:true";
    } else {
	if (verbose)
	    printf("mcast=false\n");
	s += " mcast:false";
    }
    _changed.insert(s);
}

void
DummyRegisterServer::send_invalidate(const string& module_name,
				     const IPv4Net& net,
				     bool multicast)
{
    string s;
    
    if (verbose) {
	printf("DummyRegisterServer::send_invalidate module=%s net=%s ",
	       module_name.c_str(), net.str().c_str());
    }
    
    s = module_name + " " + net.str();
    if (multicast) {
	if (verbose)
	    printf("mcast=true\n");
	s += " mcast:true";
    } else {
	if (verbose)
	    printf("mcast=false\n");
	s += " mcast:false";
    }
    _invalidated.insert(s);
}

void
DummyRegisterServer::send_route_changed(const string& module_name,
					const IPv6Net& net, 
					const IPv6& nexthop,
					uint32_t metric,
					uint32_t admin_distance,
					const string& protocol_origin,
					bool multicast)
{
    string s;
    
    printf("DummyRegisterServer::send_route_changed module=%s net=%s nexthop=%s, metric=%u admin_distance=%u protocol_origin=%s",
	   module_name.c_str(), net.str().c_str(), nexthop.str().c_str(),
	   XORP_UINT_CAST(metric), XORP_UINT_CAST(admin_distance),
	   protocol_origin.c_str());
    
    s = module_name + " " + net.str() + " " + nexthop.str();
    s += " " + c_format("%u", XORP_UINT_CAST(metric));
    if (multicast) {
	printf("mcast=true\n");
	s += " mcast:true";
    } else {
	printf("mcast=false\n");
	s += " mcast:false";
    }
    _changed.insert(s);
}

void
DummyRegisterServer::send_invalidate(const string& module_name,
				     const IPv6Net& net,
				     bool multicast)
{
    string s;
    
    if (verbose) {
	printf("DummyRegisterServer::send_invalidate module=%s net=%s ",
	       module_name.c_str(), net.str().c_str());
    }
    
    s = module_name + " " + net.str();
    if (multicast) {
	if (verbose)
	    printf("mcast=true\n");
	s += " mcast:true";
    } else {
	if (verbose)
	    printf("mcast=false\n");
	s += " mcast:false";
    }
    _invalidated.insert(s);
}

bool
DummyRegisterServer::verify_invalidated(const string& invalid)
{
    set<string>::iterator iter;
    
    iter = _invalidated.find(invalid);
    if (iter == _invalidated.end()) {
	if (verbose)
	    printf("EXPECTED: >%s<\n", invalid.c_str());
	for (iter = _invalidated.begin(); iter !=  _invalidated.end(); ++iter)
	    if (verbose)
		printf("INVALIDATED: >%s<\n", iter->c_str());
	return false;
    }
    _invalidated.erase(iter);
    return true;
}

bool
DummyRegisterServer::verify_changed(const string& changed)
{
    set<string>::iterator iter;
    
    iter = _changed.find(changed);
    if (iter == _changed.end()) {
	if (verbose)
	    printf("EXPECTED: >%s<\n", changed.c_str());
	for (iter = _invalidated.begin(); iter !=  _invalidated.end(); ++iter)
	    if (verbose)
		printf("CHANGED: >%s<\n", iter->c_str());
	return false;
    }
    _changed.erase(iter);
    return true;
}

bool
DummyRegisterServer::verify_no_info()
{
    if (_changed.empty() && _invalidated.empty())
	return true;
    return false;
}
