/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

/*
 * Copyright (c) 2001-2008 International Computer Science Institute
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
 * $XORP: xorp/libxorp/xorp_osdep_begin.h,v 1.4 2007/02/16 22:46:29 pavlin Exp $
 */

#ifndef __LIBXORP_XORP_OSDEP_BEGIN_H__
#define __LIBXORP_XORP_OSDEP_BEGIN_H__

#ifdef HOST_OS_WINDOWS

#define	WIN32_LEAN_AND_MEAN		/* Do not include most headers */

#define	_UNICODE			/* Use Unicode APIs */
#define	UNICODE

#define	FD_SETSIZE	1024		/* Default is 64, use BSD default */

#define	WINVER		0x502		/* Windows Server 2003 target */
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
