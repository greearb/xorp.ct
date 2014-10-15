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


#ifndef __LIBXORP_XORP_OSDEP_BEGIN_H__
#define __LIBXORP_XORP_OSDEP_BEGIN_H__

#ifdef HOST_OS_WINDOWS

#define USE_WIN_DISPATCHER 1
#define	WIN32_LEAN_AND_MEAN		/* Do not include most headers */

#define	_UNICODE			/* Use Unicode APIs */
#define	UNICODE

#define	FD_SETSIZE	1024		/* Default is 64, use BSD default */

#define	WINVER		0x0502		/* Windows Server 2003 target */
#define MPR50           1
#define	_WIN32_WINNT	0x502

/*
 * Reduce the size of the Windows namespace, by leaving out
 * that which we do not need.
 * (Source: Windows Systems Programming 3e, Hart, 2004)
 */
#define NOATOM
#define NOCLIPBOARD
#define NOCOMM
#define NOCTLMGR
#define NOCOLOR
#define NODEFERWINDOWPOS
#define NODESKTOP
#define NODRAWTEXT
#define NOEXTAPI
#define NOGDICAPMASKS
#define NOHELP
#define NOICONS
#define NOTIME
#define NOIMM
#define NOKANJI
#define NOKERNEL
#define NOKEYSTATES
#define NOMCX
#define NOMEMMGR
#define NOMENUS
#define NOMETAFILE
#define NOMSG
#define NONCMESSAGES
#define NOPROFILER
#define NORASTEROPS
#define NORESOURCE
#define NOSCROLL
#define NOSERVICE
#define NOSHOWWINDOW
#define NOSOUND
#define NOSYSCOMMANDS
#define NOSYSMETRICS
#define NOSYSPARAMS
#define NOTEXTMETRIC
#define NOVIRTUALKEYCODES
#define NOWH
#define NOWINDOWSTATION
#define NOWINMESSAGES
#define NOWINOFFSETS
#define NOWINSTTLES
#define OEMRESOURCE

#endif /* HOST_OS_WINDOWS */

#endif /* __LIBXORP_XORP_OSDEP_BEGIN_H__ */
