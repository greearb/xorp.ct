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

#ident "$XORP: xorp/rib/routemap.cc,v 1.4 2003/03/16 07:18:58 pavlin Exp $"

#include "rib_module.h"
#include "routemap.hh"

RouteMap::RouteMap(const string& mapname)
{
    _mapname = mapname;
}

int
RouteMap::add_rule(RMRule *rule)
{
#ifdef NOTDEF
    if (_ruleset[rule->seq()] != NULL) {
	cerr << "Attempt to add duplicate rule number " << rule->seq() 
	     << " to RouteMap " << _mapname << "\n";
	return XORP_ERROR;
    }
#endif
    
    typedef list<RMRule*>::iterator CI;
    CI ptr = _ruleset.begin();
    while (ptr != _ruleset.end()) {
	cout << "comparing new rule " << rule->seq() << " with " 
	     << (*ptr)->seq() << "\n";
	if (rule->seq() < (*ptr)->seq()) {
	    cout << "here1\n";
	    _ruleset.insert(ptr, rule);
	    return XORP_OK;
	}
	ptr++;
    }
    cout << "here2\n";
    _ruleset.insert(ptr, rule);
    return XORP_OK;
}

string
RouteMap::str() const
{
    string result;
    
    typedef list<RMRule*>::const_iterator CI;
    CI ptr = _ruleset.begin();
    while (ptr != _ruleset.end()) {
	result += "route-map " + _mapname + (*ptr)->str() + "!\n";
	ptr++;
    }
    return result;
}

RMRule::RMRule(int seq, RMMatch *match, RMAction *action)
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
