/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2008 XORP, Inc.
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
 * $XORP: xorp/libxorp/random.h,v 1.9 2008/10/01 22:17:51 pavlin Exp $
 */

#ifndef __LIBXORP_RANDOM_H__
#define __LIBXORP_RANDOM_H__

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Define the maximum bound of the random() function
 * as per 4.2BSD / SUSV2:
 * http://www.opengroup.org/onlinepubs/007908799/xsh/initstate.html
 */
#define XORP_RANDOM_MAX		0x7FFFFFFF  /* 2^31 - 1 in 2's complement */

long	xorp_random(void);
void	xorp_srandom(unsigned long);
char*	xorp_initstate(unsigned long, char *, long);
char*	xorp_setstate(char *);
void	xorp_srandomdev(void);

#ifdef __cplusplus
}
#endif

#endif /* __LIBXORP_RANDOM_H__ */
