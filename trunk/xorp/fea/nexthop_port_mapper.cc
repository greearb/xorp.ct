// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/fea/nexthop_port_mapper.cc,v 1.3 2004/10/25 23:29:28 pavlin Exp $"

#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"

#include "nexthop_port_mapper.hh"

NexthopPortMapper::NexthopPortMapper()
{

}

NexthopPortMapper::~NexthopPortMapper()
{

}

void
NexthopPortMapper::clear()
{
    //
    // Clear all maps
    //
    _interface_map.clear();
    _ipv4_map.clear();
    _ipv6_map.clear();
    _ipv4net_map.clear();
    _ipv6net_map.clear();

    notify_observers();
}

int
NexthopPortMapper::add_observer(NexthopPortMapperObserver* observer)
{
    if (find(_observers.begin(), _observers.end(), observer)
	!= _observers.end()) {
	return (XORP_ERROR);
    }

    _observers.push_back(observer);
    return (XORP_OK);
}

int
NexthopPortMapper::delete_observer(NexthopPortMapperObserver* observer)
{
    list<NexthopPortMapperObserver *>::iterator iter;

    iter = find(_observers.begin(), _observers.end(), observer);
    if (iter == _observers.end()) {
	return (XORP_ERROR);
    }

    _observers.erase(iter);
    return (XORP_OK);
}

int
NexthopPortMapper::lookup_nexthop_interface(const string& ifname,
					    const string& vifname) const
{
    if (ifname.empty() && vifname.empty())
	return (-1);

    map<pair<string, string>, int>::const_iterator iter;

    iter = _interface_map.find(make_pair(ifname, vifname));
    if (iter == _interface_map.end())
	return (-1);			// No such entry

    return (iter->second);
}

int
NexthopPortMapper::lookup_nexthop_ipv4(const IPv4& ipv4) const
{
    //
    // Check first the map with IPv4 addresses
    //
    map<IPv4, int>::const_iterator ipv4_iter;
    ipv4_iter = _ipv4_map.find(ipv4);
    if (ipv4_iter != _ipv4_map.end())
	return (ipv4_iter->second);

    //
    // Check the map with IPv4 subnets
    //
    map<IPv4Net, int>::const_iterator ipv4net_iter;
    for (ipv4net_iter = _ipv4net_map.begin();
	 ipv4net_iter != _ipv4net_map.end();
	 ++ipv4net_iter) {
	const IPv4Net& ipv4net = ipv4net_iter->first;
	if (ipv4net.contains(ipv4))
	    return (ipv4net_iter->second);
    }

    return (-1);	// Nothing found
}

int
NexthopPortMapper::lookup_nexthop_ipv6(const IPv6& ipv6) const
{
    //
    // Check first the map with IPv6 addresses
    //
    map<IPv6, int>::const_iterator ipv6_iter;
    ipv6_iter = _ipv6_map.find(ipv6);
    if (ipv6_iter != _ipv6_map.end())
	return (ipv6_iter->second);

    //
    // Check the map with IPv6 subnets
    //
    map<IPv6Net, int>::const_iterator ipv6net_iter;
    for (ipv6net_iter = _ipv6net_map.begin();
	 ipv6net_iter != _ipv6net_map.end();
	 ++ipv6net_iter) {
	const IPv6Net& ipv6net = ipv6net_iter->first;
	if (ipv6net.contains(ipv6))
	    return (ipv6net_iter->second);
    }

    return (-1);	// Nothing found
}

int
NexthopPortMapper::add_interface(const string& ifname, const string& vifname,
				 int port)
{
    if (ifname.empty() && vifname.empty())
	return (XORP_ERROR);

    map<pair<string, string>, int>::iterator iter;

    iter = _interface_map.find(make_pair(ifname, vifname));
    if (iter != _interface_map.end()) {
	// Update the port
	if (iter->second != port) {
	    iter->second = port;
	    notify_observers();
	}
    } else {
	// Add a new entry
	_interface_map.insert(make_pair(make_pair(ifname, vifname), port));
	notify_observers();
    }

    return (XORP_OK);
}

int
NexthopPortMapper::delete_interface(const string& ifname,
				    const string& vifname)
{
    if (ifname.empty() && vifname.empty())
	return (XORP_ERROR);

    map<pair<string, string>, int>::iterator iter;

    iter = _interface_map.find(make_pair(ifname, vifname));
    if (iter == _interface_map.end())
	return (XORP_ERROR);		// No such entry

    _interface_map.erase(iter);
    notify_observers();

    return (XORP_OK);
}

int
NexthopPortMapper::add_ipv4(const IPv4& ipv4, int port)
{
    map<IPv4, int>::iterator iter;

    iter = _ipv4_map.find(ipv4);
    if (iter != _ipv4_map.end()) {
	// Update the port
	if (iter->second != port) {
	    iter->second = port;
	    notify_observers();
	}
    } else {
	// Add a new entry
	_ipv4_map.insert(make_pair(ipv4, port));
	notify_observers();
    }

    return (XORP_OK);
}

int
NexthopPortMapper::delete_ipv4(const IPv4& ipv4)
{
    map<IPv4, int>::iterator iter;

    iter = _ipv4_map.find(ipv4);
    if (iter == _ipv4_map.end())
	return (XORP_ERROR);		// No such entry

    _ipv4_map.erase(iter);
    notify_observers();

    return (XORP_OK);
}

int
NexthopPortMapper::add_ipv6(const IPv6& ipv6, int port)
{
    map<IPv6, int>::iterator iter;

    iter = _ipv6_map.find(ipv6);
    if (iter != _ipv6_map.end()) {
	// Update the port
	if (iter->second != port) {
	    iter->second = port;
	    notify_observers();
	}
    } else {
	// Add a new entry
	_ipv6_map.insert(make_pair(ipv6, port));
	notify_observers();
    }

    return (XORP_OK);
}

int
NexthopPortMapper::delete_ipv6(const IPv6& ipv6)
{
    map<IPv6, int>::iterator iter;

    iter = _ipv6_map.find(ipv6);
    if (iter == _ipv6_map.end())
	return (XORP_ERROR);		// No such entry

    _ipv6_map.erase(iter);
    notify_observers();

    return (XORP_OK);
}

int
NexthopPortMapper::add_ipv4net(const IPv4Net& ipv4net, int port)
{
    map<IPv4Net, int>::iterator iter;

    iter = _ipv4net_map.find(ipv4net);
    if (iter != _ipv4net_map.end()) {
	// Update the port
	if (iter->second != port) {
	    iter->second = port;
	    notify_observers();
	}
    } else {
	// Add a new entry
	_ipv4net_map.insert(make_pair(ipv4net, port));
	notify_observers();
    }

    return (XORP_OK);
}

int
NexthopPortMapper::delete_ipv4net(const IPv4Net& ipv4net)
{
    map<IPv4Net, int>::iterator iter;

    iter = _ipv4net_map.find(ipv4net);
    if (iter == _ipv4net_map.end())
	return (XORP_ERROR);		// No such entry

    _ipv4net_map.erase(iter);
    notify_observers();

    return (XORP_OK);
}

int
NexthopPortMapper::add_ipv6net(const IPv6Net& ipv6net, int port)
{
    map<IPv6Net, int>::iterator iter;

    iter = _ipv6net_map.find(ipv6net);
    if (iter != _ipv6net_map.end()) {
	// Update the port
	if (iter->second != port) {
	    iter->second = port;
	    notify_observers();
	}
    } else {
	// Add a new entry
	_ipv6net_map.insert(make_pair(ipv6net, port));
	notify_observers();
    }

    return (XORP_OK);
}

int
NexthopPortMapper::delete_ipv6net(const IPv6Net& ipv6net)
{
    map<IPv6Net, int>::iterator iter;

    iter = _ipv6net_map.find(ipv6net);
    if (iter == _ipv6net_map.end())
	return (XORP_ERROR);		// No such entry

    _ipv6net_map.erase(iter);
    notify_observers();

    return (XORP_OK);
}

void
NexthopPortMapper::notify_observers()
{
    list<NexthopPortMapperObserver *>::iterator iter;

    for (iter = _observers.begin(); iter != _observers.end(); ++iter) {
	NexthopPortMapperObserver* observer = *iter;
	observer->nexthop_port_mapper_event();
    }
}

NexthopPortMapperObserver::~NexthopPortMapperObserver()
{

}
