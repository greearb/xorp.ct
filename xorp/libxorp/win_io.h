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
 * $XORP: xorp/libxorp/win_io.h,v 1.10 2008/10/02 21:57:37 bms Exp $
 */

#ifndef __LIBXORP_WIN_IO_H__
#define __LIBXORP_WIN_IO_H__

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HOST_OS_WINDOWS

int win_strerror_r(DWORD errnum, char *strerrbuf, size_t buflen);
char *win_strerror(DWORD errnum);

#define WINIO_ERROR_IOERROR	(-1)	/* An I/O error on the pipe */
#define WINIO_ERROR_HASINPUT	(-2)	/* Data is ready to be read */
#define WINIO_ERROR_DISCONNECT	(-3)	/* The pipe was disconnected */

ssize_t	win_con_read(HANDLE h, void *buf, size_t bufsize);
ssize_t	win_pipe_read(HANDLE h, void *buf, size_t bufsize);

#endif /* HOST_OS_WINDOWS */

#ifdef __cplusplus
}
#endif

#endif /* __LIBXORP_WIN_IO_H__ */
