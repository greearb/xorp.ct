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

#ident "$XORP: xorp/devnotes/template.cc,v 1.2 2003/01/16 19:08:48 mjh Exp $"

#include "permits.hh"

static IPv4Hosts ipv4_hosts;
static IPv4Nets  ipv4_nets;
static IPv6Hosts ipv6_hosts;
static IPv6Nets  ipv6_nets;

bool
add_permitted_host(const IPv4& host)
{
    if (find(ipv4_hosts.begin(), ipv4_hosts.end(), host) == ipv4_hosts.end()) {
	ipv4_hosts.push_back(host);
	return true;
    }
    return false;
}

bool
add_permitted_net(const IPv4Net& net)
{
    if (find(ipv4_nets.begin(), ipv4_nets.end(), net) == ipv4_nets.end()) {
	ipv4_nets.push_back(net);
	return true;
    }
    return false;
}

bool
add_permitted_host(const IPv6& host)
{
    if (find(ipv6_hosts.begin(), ipv6_hosts.end(), host) == ipv6_hosts.end()) {
	ipv6_hosts.push_back(host);
	return true;
    }
    return false;
}

bool
add_permitted_net(const IPv6Net& net)
{
    if (find(ipv6_nets.begin(), ipv6_nets.end(), net) == ipv6_nets.end()) {
	ipv6_nets.push_back(net);
	return true;
    }
    return false;
}

bool is_host_permitted(const IPv4& host)
{
    if (find(ipv4_hosts.begin(), ipv4_hosts.end(), host) != ipv4_hosts.end()) {
	return true;
    }

    for (IPv4Nets::const_iterator n = ipv4_nets.begin();
	 n != ipv4_nets.end(); ++n) {
	if (n->contains(host)) {
	    return true;
	}
    }
    return false;
}

bool is_host_permitted(const IPv6& host)
{
    if (find(ipv6_hosts.begin(), ipv6_hosts.end(), host) != ipv6_hosts.end()) {
	return true;
    }

    for (IPv6Nets::const_iterator n = ipv6_nets.begin();
	 n != ipv6_nets.end(); ++n) {
	if (n->contains(host)) {
	    return true;
	}
    }
    return false;
}

const IPv4Hosts& permitted_ipv4_hosts()	{ return ipv4_hosts; }
const IPv4Nets&	 permitted_ipv4_nets()	{ return ipv4_nets; }
const IPv6Hosts& permitted_ipv6_hosts()	{ return ipv6_hosts; }
const IPv6Nets&	 permitted_ipv6_nets()	{ return ipv6_nets; }
