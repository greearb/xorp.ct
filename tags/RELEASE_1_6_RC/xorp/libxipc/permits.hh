// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/libxipc/permits.hh,v 1.8 2008/07/23 05:10:43 pavlin Exp $

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
