// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/rib/routemap.cc,v 1.7 2004/02/11 08:48:47 pavlin Exp $"

#include "rib_module.h"

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
