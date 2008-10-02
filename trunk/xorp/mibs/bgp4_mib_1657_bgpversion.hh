// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/mibs/bgp4_mib_1657_bgpversion.hh,v 1.9 2008/07/23 05:11:01 pavlin Exp $


#ifndef __MIBS_BGP4_MIB_1657_BGPVERSION_HH__
#define __MIBS_BGP4_MIB_1657_BGPVERSION_HH__

void init_bgp4_mib_1657_bgpversion(void);

/* callback functions that will be called from snmpd, thus the
   C linkage */
extern "C" {

Netsnmp_Node_Handler get_bgpVersion;

}

#endif // __MIBS_BGP4_MIB_1657_BGPVERSION_HH__
