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

#ident "$XORP: xorp/libxipc/sockutil.cc,v 1.20 2002/12/09 18:29:05 hodson Exp $"

#include "config.h"

#include <arpa/inet.h>
#include <netdb.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#include "ipc_module.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/c_format.hh"

#include "sockutil.hh"

//
// XXX: BEGIN DUPLICATED CODE FROM 'libcomm'
// TODO: remove it when libxipc starts using libcomm
//
#define SO_RCV_BUF_SIZE_MIN	(48*1024)  /* min. rcv socket buf size	     */
#define SO_RCV_BUF_SIZE_MAX	(256*1024) /* desired rcv socket buf size    */
#define SO_SND_BUF_SIZE_MIN	(48*1024)  /* min. snd socket buf size	     */
#define SO_SND_BUF_SIZE_MAX	(256*1024) /* desired snd socket buf size    */

/**
 * comm_sock_set_sndbuf:
 * @sock: The socket whose sending buffer size to set.
 * @desired_bufsize: The preferred buffer size.
 * @min_bufsize: The smallest acceptable buffer size.
 * 
 * Set the sending buffer size of a socket.
 * 
 * Return value: the successfully set buffer size on success,
 * otherwise 0.
 **/
int
x_comm_sock_set_sndbuf(int sock, int desired_bufsize, int min_bufsize)
{
    int delta = desired_bufsize / 2;
    
    /*
     * Set the socket buffer size.  If we can't set it as large as we
     * want, search around to try to find the highest acceptable
     * value.  The highest acceptable value being smaller than
     * minsize is a fatal error.
     */
    if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF,
		   (void *)&desired_bufsize, sizeof(desired_bufsize)) < 0) {
	desired_bufsize -= delta;
	while (true) {
	    if (delta > 1)
		delta /= 2;
	    
	    if (setsockopt(sock, SOL_SOCKET, SO_SNDBUF,
			   (void *)&desired_bufsize, sizeof(desired_bufsize))
		< 0) {
		desired_bufsize -= delta;
	    } else {
		if (delta < 1024)
		    break;
		desired_bufsize += delta;
	    }
	}
	if (desired_bufsize < min_bufsize) {
	    XLOG_ERROR("Cannot set sending buffer size of socket %d: "
		       "desired buffer size %u < minimum allowed %u",
		       sock, desired_bufsize, min_bufsize);
	    return 0;
	}
    }
    
    return (desired_bufsize);
}

/**
 * comm_sock_set_rcvbuf:
 * @sock: The socket whose receiving buffer size to set.
 * @desired_bufsize: The preferred buffer size.
 * @min_bufsize: The smallest acceptable buffer size.
 * 
 * Set the receiving buffer size of a socket.
 * 
 * Return value: the successfully set buffer size on success,
 * otherwise 0.
 **/
int
x_comm_sock_set_rcvbuf(int sock, int desired_bufsize, int min_bufsize)
{
    int delta = desired_bufsize / 2;
    
    /*
     * Set the socket buffer size.  If we can't set it as large as we
     * want, search around to try to find the highest acceptable
     * value.  The highest acceptable value being smaller than
     * minsize is a fatal error.
     */
    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF,
		   (void *)&desired_bufsize, sizeof(desired_bufsize)) < 0) {
	desired_bufsize -= delta;
	while (true) {
	    if (delta > 1)
		delta /= 2;
	    
	    if (setsockopt(sock, SOL_SOCKET, SO_RCVBUF,
			   (void *)&desired_bufsize, sizeof(desired_bufsize))
		< 0) {
		desired_bufsize -= delta;
	    } else {
		if (delta < 1024)
		    break;
		desired_bufsize += delta;
	    }
	}
	if (desired_bufsize < min_bufsize) {
	    XLOG_ERROR("Cannot set receiving buffer size of socket %d: "
		       "desired buffer size %u < minimum allowed %u",
		       sock, desired_bufsize, min_bufsize);
	    return 0;
	}
    }
    
    return (desired_bufsize);
}
//
// XXX: END DUPLICATED CODE FROM 'libcomm'
//

bool
get_local_socket_details(int fd, string& addr, uint16_t& port)
{
    struct sockaddr_in sin;
    socklen_t slen = sizeof(sin);
    sin.sin_family = AF_INET;

    if (getsockname(fd, (sockaddr*)&sin, &slen) < 0) {
        XLOG_ERROR("getsockname failed: %s", strerror(errno));
	return false;
    }

    /* Get address */
    if (sin.sin_addr.s_addr != 0) {
	addr = inet_ntoa(sin.sin_addr);
    } else {
	static in_addr haddr;
	if (haddr.s_addr == 0) {
	    /* Socket is not associated with any particular to anything...
	     * ... this is not great. 
	     */
	    char hname[MAXHOSTNAMELEN + 1];
	    hname[MAXHOSTNAMELEN] = '\0';
	    if (gethostname(hname, MAXHOSTNAMELEN) < 0) {
		XLOG_ERROR("gethostname failed: %s", strerror(errno));
		return false;
	    }

	    /* Check hostname resolves otherwise anything that relies on
	     *  this info is going to have problems. 
	     */
	    if (address_lookup(hname, haddr) == false) {
		XLOG_ERROR("Local hostname %s does not resolve", hname);
		return false;
	    }
	}	
	addr = inet_ntoa(haddr);
    }
    port = htons(sin.sin_port);

    return true;
}

bool
get_remote_socket_details(int fd, string& addr, string& port)
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

int
create_connected_ip_socket(IPSocketType ist, const string& addr_slash_port)
{
    string addr;
    uint16_t port;
    if (split_address_slash_port(addr_slash_port, addr, port) == false) {
	XLOG_ERROR("bad address slash port: %s", addr_slash_port.c_str());
	return -1;
    }
    return create_connected_ip_socket(ist, addr, port);
}

int
create_connected_ip_socket(IPSocketType ist, const string& addr, uint16_t port)
{
    int stype = (ist == UDP) ? SOCK_DGRAM : SOCK_STREAM;

    int fd = socket(PF_INET, stype, 0);
    if (fd < 0) {
	XLOG_ERROR("failed to open socket: %s", strerror(errno));
	return -1;
    }
    
    // Set the receiving socket buffer size in the kernel
    if (x_comm_sock_set_rcvbuf(fd, SO_RCV_BUF_SIZE_MAX, SO_RCV_BUF_SIZE_MIN)
	< SO_RCV_BUF_SIZE_MIN) {
	close_socket(fd);
	return -1;
    }
    
    // Set the sending socket buffer size in the kernel
    if (x_comm_sock_set_sndbuf(fd, SO_SND_BUF_SIZE_MAX, SO_SND_BUF_SIZE_MIN)
	< SO_SND_BUF_SIZE_MIN) {
	close_socket(fd);
	return -1;
    }
    
    // Resolve address
    struct in_addr ia;
    if (address_lookup(addr, ia) == false) {
	XLOG_ERROR("Can't resolve IP address for %s", addr.c_str());
	return -1;
    }
    
    // Connect
    struct sockaddr_in sin;
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port);
    sin.sin_addr.s_addr = ia.s_addr;
    if (connect(fd, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        XLOG_ERROR("failed to connect to %s port %d: %s", 
                  addr.c_str(), port, strerror(errno));
	close_socket(fd);
	return -1;
    }

    return fd;
}

int
create_listening_ip_socket(IPSocketType ist, uint16_t port)
{
    int stype = (ist == UDP) ? SOCK_DGRAM : SOCK_STREAM;
    int fd = socket(PF_INET, stype, ist);
    if (fd < 0) {
	XLOG_ERROR("failed to open socket: %s", strerror(errno));
	return -1;
    }
    
    // Set the receiving socket buffer size in the kernel
    if (x_comm_sock_set_rcvbuf(fd, SO_RCV_BUF_SIZE_MAX, SO_RCV_BUF_SIZE_MIN)
	< SO_RCV_BUF_SIZE_MIN) {
	close_socket(fd);
	return -1;
    }
    
    // Set the sending socket buffer size in the kernel
    if (x_comm_sock_set_sndbuf(fd, SO_SND_BUF_SIZE_MAX, SO_SND_BUF_SIZE_MIN)
	< SO_SND_BUF_SIZE_MIN) {
	close_socket(fd);
	return -1;
    }

    // reuse addr
    int opt = 1;
    if (stype == SOCK_STREAM && setsockopt(fd, SOL_SOCKET, SO_REUSEADDR,
					   &opt, sizeof(opt)) < 0) {
        XLOG_ERROR("failed to setsockopt SO_REUSEADDR: %s", strerror(errno));
        close_socket(fd);
	return -1;
    }

    // bind 
    struct sockaddr_in sin;
    memset((void *)&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_port = htons(port); 
    sin.sin_addr = if_get_preferred();
    if (bind(fd, (sockaddr*)&sin, sizeof(sin)) < 0) {
        XLOG_ERROR("failed to bind socket: %s (%s)",
		   strerror(errno), inet_ntoa(sin.sin_addr));
        close_socket(fd);
	return -1;
    }
    
    // listen
    if (stype == SOCK_STREAM && listen(fd, 5) < 0) {
        XLOG_ERROR("failed to listen: %s", strerror(errno));
        close_socket(fd);
	return -1;
    }

    return fd;
}

void
close_socket(int fd)
{
    // set_socket_sndbuf_bytes(fd, 2048);
    // set_socket_rcvbuf_bytes(fd, 2048);
    close(fd);
}

int
get_socket_sndbuf_bytes(int fd)
{
    int sz;
    if (getsockopt(fd, SOL_SOCKET, SO_SNDBUF, &sz, 0) < 0) {
	XLOG_ERROR("getsockopt(%d, SO_SNDBUF) failed (errno %d, %s)\n",
		   fd, errno, strerror(errno));
	return -1;
    }
    return sz;
}

int
get_socket_rcvbuf_bytes(int fd)
{
    int sz;
    if (getsockopt(fd, SOL_SOCKET, SO_RCVBUF, &sz, 0) < 0) {
	XLOG_ERROR("getsockopt(%d, SO_RCVBUF) failed (errno %d, %s)\n",
		   fd, errno, strerror(errno));
	return -1;
    }
    return sz;
}

int
set_socket_sndbuf_bytes(int fd, uint32_t sz)
{
    int isz = int(sz);
    if (setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &isz, sizeof(isz)) < 0) {
	XLOG_ERROR("setsockopt(%d, SO_SNDBUF, %d) failed (errno %d, %s)\n",
		   fd, isz, errno, strerror(errno));
	return -1;
    }
    return isz;
}

int
set_socket_rcvbuf_bytes(int fd, uint32_t sz)
{
    int isz = int(sz);
    if (setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &isz, sizeof(isz)) < 0) {
	XLOG_ERROR("setsockopt(%d, SO_RCVBUF, %d) failed (errno %d, %s)\n",
		   fd, isz, errno, strerror(errno));
	return -1;
    }
    return isz;
}

bool
split_address_slash_port(const string& address_slash_port,
			 string& address, uint16_t& port)
{
    size_t slash = address_slash_port.find("/");

    if (slash == string::npos || 			// no slash
	slash == address_slash_port.size() - 1 ||	// slash is last char
	slash != address_slash_port.rfind("/")		// multiple slashes
	) {
	return false;
    }

    address = string(address_slash_port, 0, slash);
    port = atoi(address_slash_port.c_str() + slash + 1);
    
    return true;
}

string address_slash_port(const string& addr, uint16_t port)
{
    return c_format("%s/%d", addr.c_str(), port);
}

bool
address_lookup(const string& addr, in_addr& ia)
{
    if (inet_pton(AF_INET, addr.c_str(), &ia) == 1) {
	return true;
    }

    if (strcasecmp(addr.c_str(), "default") == 0) {
	ia = if_get_preferred();
	return true;
    }

    // XXX I don't like doing this...
    if (strcasecmp(addr.c_str(), "localhost") == 0) {
	ia = if_get_preferred();
	return true;
    }
    
    int retry = 0;
    do {
	struct hostent *h = gethostbyname(addr.c_str());
	if (h == NULL) {
	    XLOG_ERROR("Can't resolve IP address for %s: %s %d", addr.c_str(),
		       hstrerror(h_errno), h_errno);
	    return false;
	} else {
	    memcpy(&ia, h->h_addr_list[0], sizeof(ia));	    
	    return true;
	}
	usleep(100000);
	retry++;
    } while (h_errno == TRY_AGAIN && retry <= 3) ;

    return false;
}

uint32_t
if_count()
{
    static uint32_t cnt = 0;
    if (cnt == 0) {
	char buf[IF_NAMESIZE];
	char* ifname = 0;
	while ((ifname = if_indextoname(cnt + 1, buf)) != 0) 
	    cnt++;
    }
    return cnt;
}

bool
if_probe(uint32_t index, string& name, in_addr& addr, uint16_t& flags)
{
    ifreq ifr;

    if (if_indextoname(index, ifr.ifr_name) == 0) {
	return false;
    }
    name = ifr.ifr_name;

    int s = socket(PF_INET, SOCK_DGRAM, 0);
    if (s < 0) {
	debug_msg("Failed to open socket\n");
	return false;
    }
	      
    if (ioctl(s, SIOCGIFADDR, &ifr) < 0) {
	debug_msg("Failed to get interface address\n");
	close(s);
	return false;
    }
    sockaddr_in& sin = reinterpret_cast<sockaddr_in&>(ifr.ifr_addr);
    addr.s_addr = sin.sin_addr.s_addr;
    
    if (ioctl(s, SIOCGIFFLAGS, &ifr) < 0) {
	debug_msg("Failed to get interface flags\n");
	close(s);
	return false;
    }
    flags = ifr.ifr_flags;
    
    close(s);
    return true;
}


static in_addr s_if_preferred;

bool
if_set_preferred(in_addr new_addr)
{
    uint32_t n = if_count();
    for (uint32_t i = 1; i <= n; i++) {
	string name;
	in_addr addr;
	uint16_t flags;
	if (if_probe(i, name, addr, flags) &&
	    addr.s_addr == new_addr.s_addr &&
	    (flags & IFF_UP)) {
	    XLOG_INFO("Changing to interface %s addr %s for IP based XRL "
		      "communication.", name.c_str(), inet_ntoa(addr));
	    s_if_preferred = addr;
	    return true;
	}
    }
    return false;
}

in_addr if_get_preferred()
{
    if (s_if_preferred.s_addr)
	return s_if_preferred;

    uint32_t n = if_count();
    
    string	name;
    in_addr	addr;
    uint16_t	flags;

    for (uint32_t i = 1; i <= n; i++) {
	if (if_probe(i, name, addr, flags) &&
	    (flags & ~IFF_LOOPBACK) &&
	    (flags & IFF_UP)) {
	    goto done;
	}
    }

    for (uint32_t i = 1; i <= n; i++) {
    	if (if_probe(i, name, addr, flags) &&
	    (flags & IFF_UP)) {
	    goto done;
	}
    }

    // Random badness
    name = "any";
    addr.s_addr = INADDR_ANY;
    
 done:
    //    XLOG_INFO("Using interface %s addr %s for IP based XRL "
    //	      "communication.", name.c_str(), inet_ntoa(addr));

    s_if_preferred = addr;
    return s_if_preferred;
}
