// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/libxipc/sockutil.hh,v 1.13 2007/02/16 22:46:07 pavlin Exp $

/* Some helper functions for sockets */

#ifndef __LIBXIPC_SOCKUTIL_HH__
#define __LIBXIPC_SOCKUTIL_HH__

#include "libxorp/xorp.h"

#include <string>
#include <vector>

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#include "libxorp/ipv4.hh"

/**
 * Create a socket and connect to a given remote endpoint.
 *
 * @param addr_slash_port the address and port of the remote endpoint.
 * @return socket (invalid if unsuccessful).
 */
XorpFd create_connected_tcp4_socket(const string& addr_slash_port);

/**
 * Parse getsockname() result for a socket.
 *
 * @param addr reference to a string to be filled out with local address
 * @param port reference to a port to be filled out with local port
 * @return true if addr and port were filled in.
 */
bool get_local_socket_details(XorpFd fd, string& addr, uint16_t& port);

/**
 * Parse getpeername() result for a connected socket.
 *
 * @param fd socket file descriptor
 * @param addr reference to a string to be filled out with peer address
 * @param port reference to a port to be filled out with peer remote port
 * @return true if addr and port were filled in.
 */
bool get_remote_socket_details(XorpFd fd, string& addr, uint16_t& port);

/**
 * Return a string in addr:port format.
 *
 * @param addr address portion of string
 * @param port port number to be formatted as part of string
 * @return address slash port string
 */
string address_slash_port(const string& addr, uint16_t port);

/**
 * @return true if address_slash_port was split successfully.
 */
bool split_address_slash_port(const string& address_slash_port,
			      string& address, uint16_t& port);

/**
 * Lookup an IPv4 address by name.
 *
 * @param addr address to be resolved.
 * @param ia network address.
 * @return true if address resolves.
 */
bool address_lookup(const string& addr, in_addr& ia);

/**
 * Determine if an IPv4 address is configured and active within this
 * host's networking stack.
 *
 * @param ia IPv4 address to be checked.
 * @return true if address is an IP address belonging to a configured and
 * administratively up interface on the system, false otherwise.
 */
bool is_ip_configured(const in_addr& ia);

/**
 * Set preferred IPv4 address for IPC communication.
 *
 * @param addr preferred IPv4 address
 * @return true on success, false if addr is not an active IPv4 address
 * configured on the system.
 */
bool set_preferred_ipv4_addr(in_addr addr);

/**
 * Get preferred interface for XRL communication.  If not set by
 * set_preferred_ipv4_addr, it is the first valid loopback interface.
 * If there is no valid loopback interface, then it is the first valid
 * interface.
 *
 * @return IPv4 address in use for XRL communucation.
 */
in_addr get_preferred_ipv4_addr();

/*
 * Get a list of of active and configured IPv4 addresses on the system.
 *
 * @param addrs empty vector of IPv4 addresses
 */
void get_active_ipv4_addrs(vector<IPv4>& addrs);

#endif // __LIBXIPC_SOCKUTIL_HH__
