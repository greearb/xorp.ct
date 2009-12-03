/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2009 XORP, Inc.
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

/*
 * $XORP: xorp/libxorp/xorp_osdep_mid.h,v 1.14 2008/10/02 21:57:37 bms Exp $
 */

#ifndef __LIBXORP_XORP_OSDEP_MID_H__
#define __LIBXORP_XORP_OSDEP_MID_H__

/*------------------------------------------------------------------------*/

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
