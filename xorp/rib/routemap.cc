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


RouteMap::RouteMap(const string& mapname)
    : _mapname(mapname)
{
}

int
RouteMap::add_rule(RMRule* rule)
{
#if 0
    if (_ruleset[rule->seq()] != NULL) {
	cerr << "Attempt to add duplicate rule number " << rule->seq() 
	     << " to RouteMap " << _mapname << "\n";
	return XORP_ERROR;
    }
#endif
    
    list<RMRule* >::iterator iter;
    for (iter = _ruleset.begin(); iter != _ruleset.end(); ++iter) {
	cout << "comparing new rule " << rule->seq() << " with " 
	     << (*iter)->seq() << "\n";
	if (rule->seq() < (*iter)->seq()) {
	    cout << "here1\n";
	    _ruleset.insert(iter, rule);
	    return XORP_OK;
	}
    }
    cout << "here2\n";
    _ruleset.push_back(rule);
    return XORP_OK;
}

string
RouteMap::str() const
{
    string result;
    
    list<RMRule* >::const_iterator iter;
    for (iter = _ruleset.begin(); iter != _ruleset.end(); ++iter) {
	result += "route-map " + _mapname + (*iter)->str() + "!\n";
    }
    return result;
}

RMRule::RMRule(int seq, RMMatch* match, RMAction* action)
{
  _seq = seq;
  _match = match;
  _action = action;
}

string 
RMRule::str() const
{
    string result;
    char buf[20];
    
    snprintf(buf, 20, "%d", _seq);
    result = " permit ";
    result += buf;
    result += "\n";
    result += "  " + _match->str() + "\n";
    result += "  " + _action->str() + "\n";
    return result;
}

RMMatch::RMMatch()
{
}

RMMatchIPAddr::RMMatchIPAddr(const IPv4Net& ipv4net)
{
    _ipv4net = ipv4net;
}

string
RMMatchIPAddr::str() const
{
    string result;
    
    result = "match ip-address " + _ipv4net.str();
    return result;
}

bool
RMMatchIPAddr::match_route(const RouteEntry& re) const
{
    cout << "comparing " << re.str() << "\n";
    return true;
}

RMAction::RMAction()
{
    // nothing happens here
}

string 
RMAction::str() const
{
    string result;
    
    result = "no modification";
    return result;
}
