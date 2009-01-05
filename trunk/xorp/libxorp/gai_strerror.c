/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */

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
 * $XORP: xorp/libxorp/gai_strerror.c,v 1.9 2008/10/02 21:57:30 bms Exp $
 */

#include "xorp.h"

#ifdef HOST_OS_WINDOWS

#include "win_io.h"

/*
 * gai_strerror() is actually implemented as an inline on Windows; it is
 * not present in the Winsock2 DLLs. The 'real' inline functions are missing
 * from the MinGW copy of the PSDK, presumably for licensing reasons.
 *
 * There is an accompanying kludge to purge the namespace for this function
 * to be visible in <libxorp/xorp_osdep_end.h>.
 *
 * This now simply calls the win_strerror() utility routine in win_io.c.
 */

char *
gai_strerror(int ecode)
{
     return (win_strerror(ecode));
}

#endif /* HOST_OS_WINDOWS */
