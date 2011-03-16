/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2011 XORP, Inc and Others
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License, Version
 * 2.1, June 1999 as published by the Free Software Foundation.
 * Redistribution and/or modification of this program under the terms of
 * any other version of the GNU Lesser General Public License is not
 * permitted.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
 * see the GNU Lesser General Public License, Version 2.1, a copy of
 * which can be found in the XORP LICENSE.lgpl file.
 * 
 * XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
 * http://xorp.net
 */

#ifndef __LIBXORP_XORP_OSDEP_MID_H__
#define __LIBXORP_XORP_OSDEP_MID_H__

/*------------------------------------------------------------------------*/

/*
 * Windows-specific preprocessor definitions.
 */
#if defined(HOST_OS_WINDOWS)

#include <windows.h>
#include <winbase.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <process.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef IPPROTO_PIM
#define IPPROTO_PIM	103
#endif

#ifndef IN_EXPERIMENTAL
#define	IN_EXPERIMENTAL(i)	(((uint32_t)(i) & 0xf0000000) == 0xf0000000)
#endif

#ifndef IN_BADCLASS
#define	IN_BADCLASS(i)		(((uint32_t)(i) & 0xf0000000) == 0xf0000000)
#endif

#ifndef INADDR_MAX_LOCAL_GROUP
#define	INADDR_MAX_LOCAL_GROUP	(uint32_t)0xe00000ff	/* 224.0.0.255 */
#endif

#ifndef IN_LOOPBACKNET
#define	IN_LOOPBACKNET	127
#endif

#ifndef IFNAMSIZ
#define	IFNAMSIZ	255
#endif

#ifndef IF_NAMESIZE
#define	IF_NAMESIZE	255
#endif

#ifndef IP_MAXPACKET
#define IP_MAXPACKET	65535
#endif

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN MAX_HOSTNAME_LEN
#endif

#ifndef IPVERSION
#define IPVERSION 4
#endif

/*
 * iovec is common enough that we ship our own for Windows,
 * with the members swapped around to be compatible with
 * Winsock2's scatter/gather send functions.
 */
struct iovec {
	u_long	iov_len;	/* Length. (len in WSABUF) */
	char	*iov_base;	/* Base address. (buf in WSABUF) */
};

/* From netinet/ip.h */
/*
 * Structure of an internet header, naked of options.
 */
struct ip {
    uint8_t ip_h_v; /* header & version..which is first depends on byte order. */
//#if __BYTE_ORDER == __LITTLE_ENDIAN
//    unsigned int ip_hl:4;               /* header length */
//    unsigned int ip_v:4;                /* version */
//#endif
//#if __BYTE_ORDER == __BIG_ENDIAN
//    unsigned int ip_v:4;                /* version */
//    unsigned int ip_hl:4;               /* header length */
//#endif
    uint8_t ip_tos;                    /* type of service */
    unsigned short ip_len;                     /* total length */
    unsigned short ip_id;                      /* identification */
    unsigned short ip_off;                     /* fragment offset field */
#define IP_RF 0x8000                    /* reserved fragment flag */
#define IP_DF 0x4000                    /* dont fragment flag */
#define IP_MF 0x2000                    /* more fragments flag */
#define IP_OFFMASK 0x1fff               /* mask for fragmenting bits */
    uint8_t ip_ttl;                    /* time to live */
    uint8_t ip_p;                      /* protocol */
    unsigned short ip_sum;                     /* checksum */
    struct in_addr ip_src, ip_dst;      /* source and dest address */
};

struct ip6_hdr {
    union {
        struct ip6_hdrctl {
            uint32_t ip6_un1_flow;   /* 4 bits version, 8 bits TC,
                                        20 bits flow-ID */
            uint16_t ip6_un1_plen;   /* payload length */
            uint8_t  ip6_un1_nxt;    /* next header */
            uint8_t  ip6_un1_hlim;   /* hop limit */
	} ip6_un1;
        uint8_t ip6_un2_vfc;       /* 4 bits version, top 4 bits tclass */
    } ip6_ctlun;
    struct in6_addr ip6_src;      /* source address */
    struct in6_addr ip6_dst;      /* destination address */
};
#define ip6_flow  ip6_ctlun.ip6_un1.ip6_un1_flow


/*------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

typedef HANDLE xfd_t;
typedef SOCKET xsock_t;

typedef unsigned long gid_t;		/* XXX: This is a hack. */
typedef unsigned long uid_t;		/* XXX: This is a hack. */

#ifdef _NO_OLDNAMES
/*
 * When _NO_OLDNAMES is defined, we need to put certain things
 * back into the namespace.
 *
 * XXX: Workaround a problem whereby some of the tests which ship with
 * GNU autoconf will try to define pid_t in an environment which
 * does not have it.
 */
//#ifdef pid_t
//#undef pid_t
//#endif
//typedef _off_t off_t;
//typedef _pid_t pid_t;
typedef long ssize_t;		/* XXX: This is a hack. */
typedef _sigset_t sigset_t;	/* XXX: Appease libtecla. */

/* XXX: This is a hack. */
#define getpid()		( (pid_t) GetCurrentProcessId() )

#define isascii(c)		__isascii((c))
#define snprintf		_snprintf
#define sleep			_sleep
#define strdup(s)		_strdup((s))
#define strcasecmp(s1, s2)	_stricmp((s1),(s2))
#define vsnprintf		_vsnprintf

#else /* !_NO_OLDNAMES */
#define sleep			Sleep
#endif

#define XORP_BAD_FD		INVALID_HANDLE_VALUE
#define XORP_BAD_SOCKET		INVALID_SOCKET

/*
 * Windows expects (char *) and (const char *) pointers for transmitted
 * data and socket option data.
 */
#define XORP_BUF_CAST(x) ((char *)(x))
#define XORP_CONST_BUF_CAST(x) ((const char *)(x))
#define XORP_SOCKOPT_CAST(x) ((char *)(x))

/* I/O errors pertaining to non-blocking sockets. */
#ifndef EWOULDBLOCK
#define EWOULDBLOCK	WSAEWOULDBLOCK
#endif

#else /* !HOST_OS_WINDOWS */

/* Use a portable file descriptor type. */
typedef int xfd_t;
typedef int xsock_t;

/* Use Windows-a-likes for checking return values from I/O syscalls. */
#define XORP_BAD_FD		((xfd_t) -1)
#define XORP_BAD_SOCKET		XORP_BAD_FD

/*
 * The rest of the world expects void pointers to transmitted
 * data and socket option data.
 */
#define XORP_BUF_CAST(x) ((void *)(x))
#define XORP_CONST_BUF_CAST(x) ((const void *)(x))
#define XORP_SOCKOPT_CAST(x) (x)

#endif /* HOST_OS_WINDOWS */

/*------------------------------------------------------------------------*/

/*
 * Portable missing functions.
 */

#ifdef __cplusplus
extern "C" {
#endif

#ifndef HAVE_STRLCPY
size_t strlcpy(char *dst, const char *src, size_t size);
#endif

/*
 * IP address presentation routines.
 */
#ifndef HAVE_INET_PTON
int inet_pton(int af, const char *src, void *dst);
#endif
#ifndef HAVE_INET_NTOP
const char *inet_ntop(int af, const void *src, char *dst, size_t size);
#endif

/*
 * getopt.
 */
#ifdef NEED_GETOPT
extern char *optarg;
extern int optind;
extern int optopt;
extern int opterr;
extern int optreset;

int getopt(int argc, char * const argv[], const char *optstring);
#endif /* NEED_GETOPT */

#ifdef __cplusplus
}
#endif

#endif /* __LIBXORP_XORP_OSDEP_MID_H__ */
