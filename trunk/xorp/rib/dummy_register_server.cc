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

#ident "$XORP: xorp/rib/dummy_register_server.cc,v 1.5 2003/03/29 19:03:10 pavlin Exp $"

#include "rib_module.h"
#include "dummy_register_server.hh"

extern bool verbose;


DummyRegisterServer::DummyRegisterServer() 
    : RegisterServer(NULL) {
}


void 
DummyRegisterServer::send_route_changed(const string& modname,
					const IPv4Net& net, 
					const IPv4& nexthop,
					uint32_t metric,
					uint32_t admin_distance,
					const string& protocol_origin,
					bool multicast)
{
    string s;
    
    if (verbose) {
	printf("DummyRegisterServer::send_route_changed module=%s net=%s nexthop=%s, metric=%d admin_distance=%d protocol_origin=%s",
	       modname.c_str(), net.str().c_str(), nexthop.str().c_str(), 
	       metric, admin_distance, protocol_origin.c_str());
    }
    
    s = modname + " " + net.str() + " " + nexthop.str();
    s += " " + c_format("%d", metric);
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
DummyRegisterServer::send_invalidate(const string& modname,
				     const IPv4Net& net,
				     bool multicast)
{
    string s;
    
    if (verbose) {
	printf("DummyRegisterServer::send_invalidate module=%s net=%s ",
	       modname.c_str(), net.str().c_str());
    }
    
    s = modname + " " + net.str();
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
DummyRegisterServer::send_route_changed(const string& modname,
					const IPv6Net& net, 
					const IPv6& nexthop,
					uint32_t metric,
					uint32_t admin_distance,
					const string& protocol_origin,
					bool multicast)
{
    string s;
    
    printf("DummyRegisterServer::send_route_changed module=%s net=%s nexthop=%s, metric=%d admin_distance=%d protocol_origin=%s",
	   modname.c_str(), net.str().c_str(), nexthop.str().c_str(), metric,
	   admin_distance, protocol_origin.c_str());
    
    s = modname + " " + net.str() + " " + nexthop.str();
    s += " " + c_format("%d", metric);
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
DummyRegisterServer::send_invalidate(const string& modname,
				     const IPv6Net& net,
				     bool multicast)
{
    string s;
    
    if (verbose) {
	printf("DummyRegisterServer::send_invalidate module=%s net=%s ",
	       modname.c_str(), net.str().c_str());
    }
    
    s = modname + " " + net.str();
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
    set<string>::iterator i;
    
    i = _invalidated.find(invalid);
    if (i == _invalidated.end()) {
	if (verbose)
	    printf("EXPECTED: >%s<\n", invalid.c_str());
	for (i = _invalidated.begin(); i !=  _invalidated.end(); ++i)
	    if (verbose)
		printf("INVALIDATED: >%s<\n", i->c_str());
	return false;
    }
    _invalidated.erase(i);
    return true;
}

bool 
DummyRegisterServer::verify_changed(const string& changed)
{
    set<string>::iterator i;
    
    i = _changed.find(changed);
    if (i == _changed.end()) {
	if (verbose)
	    printf("EXPECTED: >%s<\n", changed.c_str());
	for (i = _invalidated.begin(); i !=  _invalidated.end(); ++i)
	    if (verbose)
		printf("CHANGED: >%s<\n", i->c_str());
	return false;
    }
    _changed.erase(i);
    return true;
}

bool
DummyRegisterServer::verify_no_info()
{
    if (_changed.empty() && _invalidated.empty())
	return true;
    return false;
}
