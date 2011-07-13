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

#ifndef __LIBXORP_XORP_OSDEP_END_H__
#define __LIBXORP_XORP_OSDEP_END_H__

#ifdef HOST_OS_WINDOWS

#include "win_io.h"

/*
 * Numerous kludges for purging items from the Windows namespace
 * which collide with the XORP namespace exist here.
 */

#ifdef __cplusplus
extern "C" {
#endif

/*
 * XXX: <libxorp/service.hh> ServiceStatus enum members collide
 * with some of the definitions in <winsvc.h>, which we don't use
 * [yet], so undefine some of them out of the way for now.
 */
#undef SERVICE_RUNNING
#undef SERVICE_PAUSED

/*
 * XXX: <libproto/spt.hh> RouteCmd::Cmd enum collides with the
 * preprocessor define DELETE in <winnt.h>.
 */
#undef DELETE

/*
 * XXX: <policy/filter_manager.hh> _export member name collides with
 * the preprocessor define _export in <windef.h>.
 */
#undef _export

/*
 * XXX: <bgp/socket.hh> enums collide with the preprocessor define
 * ERROR in <wingdi.h>.
 */
#undef ERROR

/*
 * XXX: gai_strerror() is #define'd to one of two prototyped functions
 * by both the MinGW w32api version of the SDK, and the PSDK itself.
 * However the functions don't actually exist.
 * The Windows PSDK itself defines both functions as inline C functions.
 * Here we try to deal with both cases by explicitly defining a single
 * function prototype for the Win32 case, and purging the preprocessor macro.
 */
#ifdef gai_strerror
#undef gai_strerror
#endif

char *gai_strerror(int ecode);

#ifdef __cplusplus
}
#endif

#define XSTRERROR win_strerror(GetLastError())

/* End of windows code */
#else

/* Non windows code */
#define XSTRERROR strerror(errno)

#endif

#endif /* __LIBXORP_XORP_OSDEP_END_H__ */
