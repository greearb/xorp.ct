// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/libxipc/sockutil.cc,v 1.25 2007/02/16 22:46:07 pavlin Exp $"

#include "ipc_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/c_format.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/ipv4.hh"
#ifdef HOST_OS_WINDOWS
#include "libxorp/win_io.h"
#endif

#include <vector>

#ifdef HAVE_IPHLPAPI_H
#include <iphlpapi.h>
#endif
#ifdef HAVE_IPTYPES_H
#include <iptypes.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif

#ifdef HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif

#include "libcomm/comm_api.h"

#include "sockutil.hh"


#ifndef HOST_OS_WINDOWS
static uint32_t if_count();
static bool if_probe(uint32_t index, string& name, in_addr& addr,
		     uint16_t& flags);
#endif

bool
get_local_socket_details(XorpFd fd, string& addr, uint16_t& port)
{
    struct sockaddr_in sin;
    socklen_t slen = sizeof(sin);
    sin.sin_family = AF_INET;

    if (getsockname(fd, (sockaddr*)&sin, &slen) < 0) {
        XLOG_ERROR("getsockname failed: %s", strerror(errno));
	return false;
    }

    // Get address
    if (sin.sin_addr.s_addr != 0) {
	addr = inet_ntoa(sin.sin_addr);
    } else {
	static in_addr haddr;
	if (haddr.s_addr == 0) {
	    //
	    // Socket is not associated with any particular to anything...
	    // ... this is not great.
	    //
	    char hname[MAXHOSTNAMELEN + 1];
	    hname[MAXHOSTNAMELEN] = '\0';
	    if (gethostname(hname, MAXHOSTNAMELEN) < 0) {
		XLOG_ERROR("gethostname failed: %s",
			   comm_get_last_error_str());
		return false;
	    }

	    //
	    // Check hostname resolves otherwise anything that relies on
	    // this info is going to have problems.
	    //
	    if (address_lookup(hname, haddr) == false) {
		XLOG_ERROR("Local hostname %s does not resolve", hname);
		return false;
	    }
	}
	addr = inet_ntoa(haddr);
    }
    port = ntohs(sin.sin_port);

    return true;
}

bool
get_remote_socket_details(XorpFd fd, string& addr, string& port)
{
    struct sockaddr_in sin;
    socklen_t slen = sizeof(sin);
    sin.sin_family = AF_INET;

    if (getpeername(fd, (sockaddr*)&sin, &slen) < 0) {
        XLOG_ERROR("getsockname failed: %s", strerror(errno));
	return false;
    }

    addr = inet_ntoa(sin.sin_addr);

    char pbuf[8];
    snprintf(pbuf, sizeof(pbuf), "%d", ntohs(sin.sin_port));
    port = pbuf;

    return true;
}

// XXX: This will go away eventually.
XorpFd
create_connected_tcp4_socket(const string& addr_slash_port)
{
    XorpFd sock;
    string addr;
    struct in_addr ia;
    uint16_t port;
    int in_progress;

    if (split_address_slash_port(addr_slash_port, addr, port) == false) {
	XLOG_ERROR("bad address slash port: %s", addr_slash_port.c_str());
	return sock;
    }

    if (address_lookup(addr, ia) == false) {
	XLOG_ERROR("Can't resolve IP address for %s", addr.c_str());
	return sock;
    }

    sock = comm_connect_tcp4(&ia, htons(port), COMM_SOCK_NONBLOCKING,
			     &in_progress);
    if (!sock.is_valid()) {
	return sock;
    }

    // Set the receiving socket buffer size in the kernel
    if (comm_sock_set_rcvbuf(sock, SO_RCV_BUF_SIZE_MAX, SO_RCV_BUF_SIZE_MIN)
	< SO_RCV_BUF_SIZE_MIN) {
	comm_close(sock);
	sock.clear();
	return sock;
    }

    // Set the sending socket buffer size in the kernel
    if (comm_sock_set_sndbuf(sock, SO_SND_BUF_SIZE_MAX, SO_SND_BUF_SIZE_MIN)
	< SO_SND_BUF_SIZE_MIN) {
	comm_close(sock);
	sock.clear();
	return sock;
    }

    return sock;
}

bool
split_address_slash_port(const string& address_slash_port,
			 string& address, uint16_t& port)
{
    string::size_type slash = address_slash_port.find(":");

    if (slash == string::npos || 			// no slash
	slash == address_slash_port.size() - 1 ||	// slash is last char
	slash != address_slash_port.rfind(":")		// multiple slashes
	) {
	return false;
    }

    address = string(address_slash_port, 0, slash);
    port = atoi(address_slash_port.c_str() + slash + 1);

    return true;
}

string
address_slash_port(const string& addr, uint16_t port)
{
    return c_format("%s:%d", addr.c_str(), port);
}

bool
address_lookup(const string& addr, in_addr& ia)
{
    if (inet_pton(AF_INET, addr.c_str(), &ia) == 1) {
	return true;
    }

    int retry = 0;
    do {
	struct hostent *h = gethostbyname(addr.c_str());
	if (h == NULL) {
#ifdef HAVE_HSTRERROR
	    int err = h_errno;
	    const char *strerr = hstrerror(err);
#else
	    int err = comm_get_last_error();
	    const char *strerr = comm_get_error_str(err);
#endif
	    XLOG_ERROR("Can't resolve IP address for %s: %s %d",
		       addr.c_str(), strerr, err);
	    return false;
	} else {
	    memcpy(&ia, h->h_addr_list[0], sizeof(ia));
	    return true;
	}
	TimerList::system_sleep(TimeVal(0, 100000));
	retry++;
    } while (h_errno == TRY_AGAIN && retry <= 3) ;

    return false;
}

//
// Return a vector containing the IPv4 addresses which are currently
// configured and administratively up on the local host.
//
void
get_active_ipv4_addrs(vector<IPv4>& addrs)
{
#ifdef HOST_OS_WINDOWS
    PMIB_IPADDRTABLE	pAddrTable;
    DWORD		result, tries;
    ULONG		dwSize;

    tries = 0;
    result = ERROR_INSUFFICIENT_BUFFER;
    dwSize = sizeof(MIB_IPADDRTABLE);
    do {
	pAddrTable = (PMIB_IPADDRTABLE) ((tries == 0) ? malloc(dwSize) :
				     realloc(pAddrTable, dwSize));
	if (pAddrTable == NULL)
	    break;
	result = GetIpAddrTable(pAddrTable, &dwSize, TRUE);
    } while ((++tries < 3) || (result == ERROR_INSUFFICIENT_BUFFER));

    if (result != NO_ERROR) {
	XLOG_FATAL("GetIpAddrTable(): %s\n", win_strerror(result));
    }
    XLOG_ASSERT(pAddrTable->dwNumEntries != 0);

    // Loopback is always listed last in the table, according to MSDN.
    // They lie. We want it at the front so don't do anything different.

    for (uint32_t i = 0; i < pAddrTable->dwNumEntries; i++) {
	uint32_t iaddr = pAddrTable->table[i].dwAddr;
	if (iaddr != INADDR_ANY)
	    addrs.push_back(IPv4(iaddr));
    }

    free(pAddrTable);

#else // ! HOST_OS_WINDOWS
    uint32_t n = if_count();

    string if_name;
    in_addr if_addr;
    uint16_t if_flags = 0;

    // Always push loopback first.
    addrs.push_back(IPv4::LOOPBACK());

    for (uint32_t i = 1; i <= n; i++) {
	if (if_probe(i, if_name, if_addr, if_flags) &&
	    (if_addr.s_addr != INADDR_LOOPBACK) && (if_flags & IFF_UP)) {
	    addrs.push_back(IPv4(if_addr));
	}
    }
#endif // ! HOST_OS_WINDOWS
}

// Return true if a given IPv4 address is currently configured in the system.
bool
is_ip_configured(const in_addr& a)
{
    vector<IPv4> addrs;
    get_active_ipv4_addrs(addrs);

    if (addrs.empty())
	return false;

    vector<IPv4>::const_iterator i;
    for (i = addrs.begin(); i != addrs.end(); i++) {
	if (*i == IPv4(a))
	    return true;
    }

    return false;
}

static in_addr s_if_preferred;

//
// Set the preferred endpoint address for IPv4 based XRL communication.
//
// The specified address must be configured on an interface which is
// administratively up.
//
bool
set_preferred_ipv4_addr(in_addr new_addr)
{
    vector<IPv4> addrs;
    get_active_ipv4_addrs(addrs);

    if (addrs.empty())
	return false;

    vector<IPv4>::const_iterator i;
    for (i = addrs.begin(); i != addrs.end(); i++) {
	if (*i == IPv4(new_addr)) {
	    XLOG_INFO(
		"Changing to address %s for IPv4 based XRL communication.",
		i->str().c_str());
	    i->copy_out(s_if_preferred);
	    return true;
	}
    }

    return false;
}

//
// Return the preferred endpoint address for IPv4 based XRL communication.
//
in_addr
get_preferred_ipv4_addr()
{
    if (s_if_preferred.s_addr != INADDR_ANY)
	return (s_if_preferred);

    // No address has been set or specified; attempt to use the first
    // available IPv4 address in the system.
    vector<IPv4> addrs;
    get_active_ipv4_addrs(addrs);

    if (!addrs.empty())
	addrs[0].copy_out(s_if_preferred);

//     XLOG_INFO("Using address %s for IPv4 based XRL communication.\n",
// 	      inet_ntoa(s_if_preferred));

    return (s_if_preferred);
}

#ifndef HOST_OS_WINDOWS
//
// Return the number of network interfaces currently configured in the system.
// NOTE: Only used under UNIX.
//
static uint32_t
if_count()
{
#if defined(HOST_OS_FREEBSD) || defined(HOST_OS_DRAGONFLYBSD)
    int cnt, error;
    size_t cntlen = sizeof(cnt);
    error = sysctlbyname("net.link.generic.system.ifcount",
			 (void *)&cnt, &cntlen, NULL, 0);
    if (error == 0)
	return ((uint32_t)cnt);

    return (0);
#elif defined(HAVE_IF_INDEXTONAME)
    static uint32_t cnt = 0;	// XXX: This is a bug
    if (cnt == 0) {
	char buf[IF_NAMESIZE];
	char* ifname = 0;
	while ((ifname = if_indextoname(cnt + 1, buf)) != 0)
	    cnt++;
    }
    return cnt;
#else
    return (0);
#endif
}
#endif // !HOST_OS_WINDOWS

#ifndef HOST_OS_WINDOWS
//
// Probe for an interface given its index.
// Return its name, IPv4 address, and interface flags.
// NOTE: Only used under UNIX.
//
static bool
if_probe(uint32_t index, string& name, in_addr& addr, uint16_t& flags)
{
#if !defined(HAVE_IF_INDEXTONAME)
    return false;
    UNUSED(index);
    UNUSED(name);
    UNUSED(addr);
    UNUSED(flags);
#else // HAVE_IF_INDEXTONAME
    ifreq ifr;

    if (if_indextoname(index, ifr.ifr_name) == 0) {
	return false;
    }
    name = ifr.ifr_name;

    XorpFd s = comm_open_udp(AF_INET, COMM_SOCK_BLOCKING);
    if (!s.is_valid()) {
	debug_msg("Failed to open socket\n");
	return false;
    }

    if (ioctl(s, SIOCGIFADDR, &ifr) < 0) {
	debug_msg("Failed to get interface address\n");
	comm_close(s);
	return false;
    }
    sockaddr_in* sin = sockaddr2sockaddr_in(&ifr.ifr_addr);
    addr.s_addr = sin->sin_addr.s_addr;

    if (ioctl(s, SIOCGIFFLAGS, &ifr) < 0) {
	debug_msg("Failed to get interface flags\n");
	comm_close(s);
	return false;
    }
    flags = ifr.ifr_flags;

    comm_close(s);
    return true;
#endif // HAVE_IF_INDEXTONAME
}
#endif // !HOST_OS_WINDOWS
