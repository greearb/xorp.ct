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
 * $XORP: xorp/libxorp/xorp_osdep_end.h,v 1.4 2007/02/16 22:46:29 pavlin Exp $
 */

#ifndef __LIBXORP_XORP_OSDEP_END_H__
#define __LIBXORP_XORP_OSDEP_END_H__

#ifdef HOST_OS_WINDOWS

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

#endif /* HOST_OS_WINDOWS */

#endif /* __LIBXORP_XORP_OSDEP_END_H__ */
