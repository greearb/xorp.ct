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

// $XORP: xorp/libxipc/permits.hh,v 1.1 2003/02/25 18:58:50 hodson Exp $

#ifndef __LIBXIPC_PERMITS_HH__
#define __LIBXIPC_PERMITS_HH__

#include <list>

#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv6net.hh"

typedef list<IPv4>	IPv4Hosts;
typedef list<IPv4Net>	IPv4Nets;
typedef list<IPv6>	IPv6Hosts;
typedef list<IPv6Net>	IPv6Nets;

/**
 * Add a host to be permitted amongst hosts allowed to participate in IPC
 * between XORP processes.
 */
bool add_permitted_host(const IPv4& host);

/**
 * Add a network to be permitted amongst hosts allowed to participate in IPC
 * between XORP processes.
 */
bool add_permitted_net(const IPv4Net& net);

/**
 * Add a host to be permitted amongst hosts allowed to participate in IPC
 * between XORP processes.
 */
bool add_permitted_host(const IPv6Net& host);

/**
 * Add a network to be permitted amongst hosts allowed to participate in IPC
 * between XORP processes.
 */
bool add_permitted_net(const IPv6Net& net);

/**
 * Test if host is permitted.
 * @param host address to be test amongst permitted IPv4 hosts and nets.
 * @return true if host is permitted.
 */
bool host_is_permitted(const IPv4& host);

/**
 * Test if host is permitted.
 * @param host address to be test amongst permitted IPv6 hosts and nets.
 * @return true if host is permitted.
 */
bool host_is_permitted(const IPv6& host);

/**
 * Get list of permitted IPv4 hosts.
 */
const IPv4Hosts& permitted_ipv4_hosts();

/**
 * Get list of permitted IPv4 nets.
 */
const IPv4Nets& permitted_ipv4_nets();

/**
 * Get list of permitted IPv6 hosts.
 */
const IPv6Hosts& permitted_ipv6_hosts();

/**
 * Get list of permitted IPv6 nets.
 */
const IPv6Nets& permitted_ipv6_nets();

/**
 * Clear all IPv4 host related permissions.
 */
void clear_permitted_ipv4_hosts();

/**
 * Clear all IPv6 host related permissions.
 */
void clear_permitted_ipv6_hosts();

/**
 * Clear all IPv4 net related permissions.
 */
void clear_permitted_ipv4_nets();

/**
 * Clear all IPv6 net related permissions.
 */
void clear_permitted_ipv6_nets();


#endif // __LIBXIPC_PERMITS_HH__
