// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2012 XORP, Inc and Others
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


bool
get_local_socket_details(XorpFd fd, string& addr, uint16_t& port)
{
    struct sockaddr_in sin;
    socklen_t slen = sizeof(sin);
    sin.sin_family = AF_INET;

    if (getsockname(fd.getSocket(), (sockaddr*)&sin, &slen) < 0) {
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

    if (getpeername(fd.getSocket(), (sockaddr*)&sin, &slen) < 0) {
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
create_connected_tcp4_socket(const string& addr_slash_port, const string& local_dev)
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
			     &in_progress, local_dev.c_str());
    if (!sock.is_valid()) {
	return sock;
    }

    // Set the receiving socket buffer size in the kernel
    if (comm_sock_set_rcvbuf(sock.getSocket(), SO_RCV_BUF_SIZE_MAX, SO_RCV_BUF_SIZE_MIN)
	< SO_RCV_BUF_SIZE_MIN) {
	comm_close(sock.getSocket());
	sock.clear();
	return sock;
    }

    // Set the sending socket buffer size in the kernel
    if (comm_sock_set_sndbuf(sock.getSocket(), SO_SND_BUF_SIZE_MAX, SO_SND_BUF_SIZE_MIN)
	< SO_SND_BUF_SIZE_MIN) {
	comm_close(sock.getSocket());
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
#ifdef L_ERROR
#ifdef HAVE_HSTRERROR
	    int err = h_errno;
	    const char *strerr = hstrerror(err);
#else
	    int err = comm_get_last_error();
	    const char *strerr = comm_get_error_str(err);
#endif
	    XLOG_ERROR("Can't resolve IP address for %s: %s %d",
		       addr.c_str(), strerr, err);
#endif
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
    //
    // Always push loopback first.
    // XXX: Note that this is uncoditional, so things will break
    // if the loopback interface/address are disabled.
    //
    addrs.push_back(IPv4::LOOPBACK());

#ifdef HOST_OS_WINDOWS
    PMIB_IPADDRTABLE	pAddrTable = NULL;
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

    int s, ifnum, lastlen;
    struct ifconf ifconf;
    
    s = socket(AF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
	XLOG_FATAL("Could not initialize ioctl() socket");
    }

    //
    // Get the interface information
    //
    ifnum = 32;			// XXX: initial guess
    ifconf.ifc_buf = NULL;
    lastlen = 0;
    // Loop until SIOCGIFCONF success.
    for ( ; ; ) {
	ifconf.ifc_len = ifnum * sizeof(struct ifreq);
	if (ifconf.ifc_buf != NULL)
	    free(ifconf.ifc_buf);
	ifconf.ifc_buf = (char*)(malloc(ifconf.ifc_len));
	if (ioctl(s, SIOCGIFCONF, &ifconf) < 0) {
	    // Check UNPv1, 2e, pp 435 for an explanation why we need this
	    if ((errno != EINVAL) || (lastlen != 0)) {
		XLOG_ERROR("ioctl(SIOCGIFCONF) failed: %s", strerror(errno));
		free(ifconf.ifc_buf);
		comm_close(s);
		return;
	    }
	} else {
	    if (ifconf.ifc_len == lastlen)
		break;		// success, len has not changed
	    lastlen = ifconf.ifc_len;
	}
	ifnum += 10;
    }

    //
    // Copy the interface information to a buffer
    //
    vector<uint8_t> buffer(ifconf.ifc_len);
    memcpy(&buffer[0], ifconf.ifc_buf, ifconf.ifc_len);
    free(ifconf.ifc_buf);

    //
    // Parse the interface information
    //
    string if_name;
    size_t offset;
    for (offset = 0; offset < buffer.size(); ) {
	size_t len = 0;
	struct ifreq ifreq, ifrcopy;

	memcpy(&ifreq, &buffer[offset], sizeof(ifreq));

	// Get the length of the ifreq entry
#ifdef HAVE_STRUCT_SOCKADDR_SA_LEN
	len = max(sizeof(struct sockaddr),
		  static_cast<size_t>(ifreq.ifr_addr.sa_len));
#else
	switch (ifreq.ifr_addr.sa_family) {
#ifdef HAVE_IPV6
	case AF_INET6:
	    len = sizeof(struct sockaddr_in6);
	    break;
#endif // HAVE_IPV6
	case AF_INET:
	default:
	    len = sizeof(struct sockaddr);
	    break;
	}
#endif // HAVE_STRUCT_SOCKADDR_SA_LEN
	len += sizeof(ifreq.ifr_name);
	len = max(len, sizeof(struct ifreq));
	offset += len;				// Point to the next entry
	
	//
	// Get the interface name
	//
	char tmp_if_name[IFNAMSIZ+1];
	strncpy(tmp_if_name, ifreq.ifr_name, sizeof(tmp_if_name) - 1);
	tmp_if_name[sizeof(tmp_if_name) - 1] = '\0';
	char* cptr;
	if ( (cptr = strchr(tmp_if_name, ':')) != NULL) {
	    // Replace colon with null. Needed because in Solaris and Linux
	    // the interface name changes for aliases.
	    *cptr = '\0';
	}
	if_name = string(ifreq.ifr_name);
	
	//
	// Get the flags
	//
	unsigned int flags = 0;
	memcpy(&ifrcopy, &ifreq, sizeof(ifrcopy));
	if (ioctl(s, SIOCGIFFLAGS, &ifrcopy) < 0) {
	    XLOG_ERROR("ioctl(SIOCGIFFLAGS) for interface %s failed: %s",
		       if_name.c_str(), strerror(errno));
	} else {
	    flags = ifrcopy.ifr_flags;
	}

	//
	// Get the interface addresses for the same address family only.
	// XXX: if the address family is zero, then we query the address.
	//
	if ((ifreq.ifr_addr.sa_family != AF_INET)
	    && (ifreq.ifr_addr.sa_family != 0)) {
	    continue;
	}
	
	//
	// Get the IP address
	//
        // The default values
	IPv4 lcl_addr = IPv4::ZERO();
	
	struct ifreq ip_ifrcopy;
	memcpy(&ip_ifrcopy, &ifreq, sizeof(ip_ifrcopy));
	ip_ifrcopy.ifr_addr.sa_family = AF_INET;
	
	// Get the IP address
	if (ifreq.ifr_addr.sa_family == AF_INET) {
	    lcl_addr.copy_in(ifreq.ifr_addr);
	    memcpy(&ip_ifrcopy, &ifreq, sizeof(ip_ifrcopy));
	} else {
	    // XXX: we need to query the local IP address
	    XLOG_ASSERT(ifreq.ifr_addr.sa_family == 0);
	    
#ifdef SIOCGIFADDR
	    memset(&ifrcopy, 0, sizeof(ifrcopy));
	    strncpy(ifrcopy.ifr_name, if_name.c_str(),
		    sizeof(ifrcopy.ifr_name) - 1);
	    ifrcopy.ifr_addr.sa_family = AF_INET;
	    if (ioctl(s, SIOCGIFADDR, &ifrcopy) < 0) {
		// XXX: the interface probably has no address. Ignore.
		continue;
	    } else {
		lcl_addr.copy_in(ifrcopy.ifr_addr);
		memcpy(&ip_ifrcopy, &ifrcopy, sizeof(ip_ifrcopy));
	    }
#endif // SIOCGIFADDR
	}

	if ((lcl_addr != IPv4::ZERO()) && (flags & IFF_UP)) {
	    addrs.push_back(lcl_addr);
	}
    }

    comm_close(s);
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
