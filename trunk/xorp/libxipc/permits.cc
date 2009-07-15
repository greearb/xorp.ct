// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



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

bool host_is_permitted(const IPv4& host)
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

bool host_is_permitted(const IPv6& host)
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

void
clear_permitted_ip4_hosts()
{
    ipv4_hosts.clear();
}

void
clear_permitted_ip6_hosts()
{
    ipv6_hosts.clear();
}

void
clear_permitted_ip4_nets()
{
    ipv4_nets.clear();
}

void
clear_permitted_ip6_nets()
{
    ipv6_nets.clear();
}
