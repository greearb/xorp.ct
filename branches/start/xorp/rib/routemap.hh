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

// $XORP: xorp/rib/routemap.hh,v 1.7 2002/12/10 06:56:08 mjh Exp $

#ifndef __RIB_ROUTEMAP_HH__
#define __RIB_ROUTEMAP_HH__

#include "config.h"
#include "libxorp/xorp.h"
#include <list>
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/nexthop.hh"
#include "route.hh"

class RMAction;
class RMMatch;
class RMRule;

/**
 * @short RouteMap route filter (not yet working)
 */
class RouteMap {
public:
    RouteMap(string mapname);
    int add_rule(RMRule *const rule);
    std::string str() const;
private:
    string _mapname;
    list <RMRule*> _ruleset;
};

/**
 * @short RouteMap rule (not yet working)
 */
class RMRule {
public:
    RMRule(int seq, RMMatch *match, RMAction *action);
    int seq() const {return _seq;}
    std::string str() const;
private:
    int _seq;
    RMMatch *_match;
    RMAction *_action;
};

inline bool
operator<(RMRule a, RMRule b) {
    return (a.seq() < b.seq());
};

/**
 * @short RouteMap conditional (not yet working)
 */
class RMMatch {
public:
    RMMatch();
    virtual ~RMMatch() {};
    virtual std::string str() const = 0;
    virtual bool match_route(RouteEntry &re) = 0;
private:
};

/**
 * @short RouteMap conditional (not yet working)
 */
class RMMatchIPAddr : public RMMatch {
public:
    RMMatchIPAddr(IPv4Net &ipv4net);
    ~RMMatchIPAddr() {};
    std::string str() const;
    bool match_route(RouteEntry &re);
private:
    IPv4Net _ipv4net;
};

/**
 * @short RouteMap action (not yet working)
 */
class RMAction {
public:
    RMAction();
    std::string str() const;
private:
};

#endif // __RIB_ROUTEMAP_HH__
