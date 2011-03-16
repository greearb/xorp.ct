// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/rib/routemap.hh,v 1.12 2008/10/02 21:58:12 bms Exp $

#ifndef __RIB_ROUTEMAP_HH__
#define __RIB_ROUTEMAP_HH__



#include "libxorp/xorp.h"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/nexthop.hh"

#include "route.hh"


class RMAction;
class RMMatch;
class RMRule;

/**
 * @short RouteMap route filter (not yet working).
 */
class RouteMap {
public:
    RouteMap(const string& mapname);
    int add_rule(RMRule* rule);
    string str() const;

private:
    string		_mapname;
    list<RMRule* >	_ruleset;
};

/**
 * @short RouteMap rule (not yet working).
 */
class RMRule {
public:
    RMRule(int seq, RMMatch* match, RMAction* action);
    int seq() const { return _seq; }
    string str() const;

    bool operator<(const RMRule& other) const { return (seq() < other.seq()); }

private:
    int		_seq;
    RMMatch*	_match;
    RMAction*	_action;
};

/**
 * @short RouteMap conditional (not yet working).
 */
class RMMatch {
public:
    RMMatch();
    virtual ~RMMatch() {};
    virtual string str() const = 0;
    virtual bool match_route(const RouteEntry& re) const = 0;

private:
};

/**
 * @short RouteMap conditional (not yet working).
 */
class RMMatchIPAddr : public RMMatch {
public:
    RMMatchIPAddr(const IPv4Net& ipv4net);
    ~RMMatchIPAddr() {};
    string str() const;
    bool match_route(const RouteEntry& re) const;

private:
    IPv4Net _ipv4net;
};

/**
 * @short RouteMap action (not yet working).
 */
class RMAction {
public:
    RMAction();
    string str() const;

private:
};

#endif // __RIB_ROUTEMAP_HH__
