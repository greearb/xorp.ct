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

// $XORP: xorp/libxipc/sockutil.hh,v 1.6 2003/06/01 21:37:28 hodson Exp $

/* Some helper functions for sockets */

#ifndef __LIBXIPC_SOCKUTIL_HH__
#define __LIBXIPC_SOCKUTIL_HH__

#include <string>

#include <sys/types.h>

#include <netinet/in.h>
#include <sys/param.h>
#include <sys/socket.h>

#include "libxorp/xorp.h"

enum IPSocketType { UDP = IPPROTO_UDP, TCP = IPPROTO_TCP };

/**
 * @return fd of socket or -1 if unsuccessful.
 */
int create_connected_ip_socket(IPSocketType t, const string& addr_slash_port);

/**
 * @return fd of socket or -1 if unsuccessful.
 */
int create_connected_ip_socket(IPSocketType t,
			       const string& addr, uint16_t port);

/**
 * @return fd of socket or -1 if unsuccessful.
 */
int create_listening_ip_socket(IPSocketType ist, uint16_t port = 0);

/**
 * Release state associated with socket and close it.
 */
void close_socket(int fd);

/**
 * @return true if addr and port were filled in.
 */
bool get_local_socket_details(int fd, string& addr, uint16_t& port);

/**
 * @return true if addr and port were filled in.
 */
bool get_remote_socket_details(int fd, string& addr, uint16_t& port);

/**
 * Get socket send buffer size.
 *
 * @return size or -1 if size could not be read.
 */
int get_socket_sndbuf_bytes(int fd);

/**
 * Get socket receive buffer size.
 *
 * @return size or -1 if size could not be read.
 */
int get_socket_rcvbuf_bytes(int fd);

/**
 * Set socket send buffer size.
 *
 * @return size or -1 if size could not be set.
 */
int set_socket_sndbuf_bytes(int fd, uint32_t bytes);

/**
 * Set socket receive buffer size.
 *
 * @return size or -1 if size could not be set.
 */
int set_socket_rcvbuf_bytes(int fd, uint32_t bytes);

/**
 * @return address slash port
 */
string address_slash_port(const string& addr, uint16_t port);

/**
 * @return true if address_slash_port was split successfully.
 */
bool split_address_slash_port(const string& address_slash_port,
			      string& address, uint16_t& port);

/**
 * @param address to be resolved.
 * @param network address.
 * @return true if address resolves.
 */

bool address_lookup(const string& addr, in_addr& ia);

/**
 * @return number of network interfaces reported by OS.
 */
uint32_t if_count();

/**
 * Determine if address is an interface address on host.
 *
 * @param ia address to be checked.
 *
 * @return true if address is a valid interface address, false otherwise.
 */
bool if_valid(const in_addr& ia);

/**
 * Query interface parameters.
 *
 * @param index of interface [1..if_count()].
 * @param reference to string to take interface's name.
 * @param reference to in_addr to take interface's IPv4 address.
 * @param reference to uint16_t to take interface's flags.
 *
 * @return true on success, false on failure.
 */
bool if_probe(uint32_t index, string& name, in_addr& addr, uint16_t& flags);

/**
 * Set preferred interface for IPC communication.
 *
 * @return true on success, false if addr is not a valid interface address or
 * is marked as down.
 */
bool if_set_preferred(in_addr addr);

/**
 * Get preferred interface for IPC communication.  If not set by
 * if_set_preferred, it is the first valid non-loopback interface.
 * Otherwise it is the loopback interface.
 */
in_addr if_get_preferred();

#endif // __LIBXIPC_SOCKUTIL_HH__
