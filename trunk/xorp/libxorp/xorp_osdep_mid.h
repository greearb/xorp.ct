/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2007 International Computer Science Institute
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software")
 * to deal in the Software without restriction, subject to the conditions
 * listed in the XORP LICENSE file. These conditions include: you must
 * preserve this copyright notice, and you cannot mention the copyright
 * holders in advertising related to the Software without their permission.
 * The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
 * notice is a summary of the XORP LICENSE file; the license in that file is
 * legally binding.
 */

/*
 * $XORP: xorp/libxorp/xorp_osdep_mid.h,v 1.9 2007/04/22 01:57:09 pavlin Exp $
 */

#ifndef __LIBXORP_XORP_OSDEP_MID_H__
#define __LIBXORP_XORP_OSDEP_MID_H__

/*------------------------------------------------------------------------*/

/*
 * Windows-specific preprocessor definitions.
 */
#if defined(HOST_OS_WINDOWS)

#ifdef HAVE_WINDOWS_H
#include <windows.h>
#endif
#ifdef HAVE_WINSOCK2_H
#include <winsock2.h>
#endif
#ifdef HAVE_WS2TCPIP_H
#include <ws2tcpip.h>
#endif
#ifdef HAVE_IPHLPAPI_H
#include <iphlpapi.h>
#endif

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

/*
 * iovec is common enough that we ship our own for Windows,
 * with the members swapped around to be compatible with
 * Winsock2's scatter/gather send functions.
 */
struct iovec {
	u_long	iov_len;	/* Length. (len in WSABUF) */
	char	*iov_base;	/* Base address. (buf in WSABUF) */
};

/*------------------------------------------------------------------------*/

#ifdef __cplusplus
}
#endif

typedef HANDLE xfd_t;
typedef SOCKET xsock_t;

typedef uint32_t gid_t;		/* XXX: This is a hack. */
typedef uint32_t uid_t;		/* XXX: This is a hack. */

#ifdef _NO_OLDNAMES
/*
 * When _NO_OLDNAMES is defined, we need to put certain things
 * back into the namespace.
 *
 * XXX: Workaround a problem whereby some of the tests which ship with
 * GNU autoconf will try to define pid_t in an environment which
 * does not have it.
 */
#ifdef pid_t
#undef pid_t
#endif
typedef _pid_t pid_t;
typedef long ssize_t;		/* XXX: This is a hack. */
typedef _sigset_t sigset_t;	/* XXX: Appease libtecla. */

/* XXX: This is a hack. */
#define getpid()		( (pid_t) GetCurrentProcessId() )

#define isascii(c)		__isascii((c))
#define snprintf		_snprintf
#define strdup(s)		_strdup((s))
#define strcasecmp(s1, s2)	_stricmp((s1),(s2))
#define vsnprintf		_vsnprintf

#endif /* !_NO_OLDNAMES */


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
