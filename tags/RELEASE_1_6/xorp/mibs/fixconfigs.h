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
 * $XORP: xorp/mibs/fixconfigs.h,v 1.11 2008/10/02 21:57:41 bms Exp $
 */

/*
 * MIB modules include autoconf generated config.h files from both xorp and
 * net-snmp.  This header file resolves the conflicts between the two.  It
 * should be included between the net-snmp and the xorp includes.
 */

#ifdef PACKAGE_BUGREPORT
#undef PACKAGE_BUGREPORT
#endif // PACKAGE_BUGREPORT

#ifdef PACKAGE_NAME
#undef PACKAGE_NAME
#endif // PACKAGE_NAME

#ifdef PACKAGE_STRING
#undef PACKAGE_STRING
#endif // PACKAGE_STRING

#ifdef PACKAGE_TARNAME
#undef PACKAGE_TARNAME
#endif // PACKAGE_TARNAME

#ifdef PACKAGE_VERSION
#undef PACKAGE_VERSION
#endif // PACKAGE_VERSION
