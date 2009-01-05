/* -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*- */
/* vim:set sts=4 ts=8: */

/*
 * Copyright (c) 2001-2009 XORP, Inc.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, Version 2, June
 * 1991 as published by the Free Software Foundation. Redistribution
 * and/or modification of this program under the terms of any other
 * version of the GNU General Public License is not permitted.
 * 
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
 * see the GNU General Public License, Version 2, a copy of which can be
 * found in the XORP LICENSE.gpl file.
 * 
 * XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
 * http://xorp.net
 */

/*
 * $XORP: xorp/contrib/win32/xorprtm/mibmgr.h,v 1.7 2008/10/02 21:56:40 bms Exp $
 */

/*
 * This file is derived from code which is under the following copyright:
 *
 * Copyright (c) 1999 - 2000 Microsoft Corporation.
 *
 */

#ifndef _MIBMANAGER_H_
#define _MIBMANAGER_H_

typedef enum { GET_EXACT, GET_FIRST, GET_NEXT } MODE;

DWORD
WINAPI
MM_MibSet (PXORPRTM_MIB_SET_INPUT_DATA    pimsid);

DWORD
WINAPI
MM_MibGet (PXORPRTM_MIB_GET_INPUT_DATA    pimgid,
    PXORPRTM_MIB_GET_OUTPUT_DATA   pimgod,
    PULONG	                        pulOutputSize,
    MODE                            mMode);

#endif
