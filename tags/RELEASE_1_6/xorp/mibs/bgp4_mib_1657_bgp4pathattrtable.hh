// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/mibs/bgp4_mib_1657_bgp4pathattrtable.hh,v 1.15 2008/10/02 21:57:40 bms Exp $


#ifndef __MIBS_BGP4_MIB_1657_PATHATTRTABLE_HH__
#define __MIBS_BGP4_MIB_1657_PATHATTRTABLE_HH__

#ifdef __cplusplus
extern "C" {
#endif

    
#include <net-snmp/net-snmp-config.h>

#ifdef HAVE_NET_SNMP_LIBRARY_CONTAINER_H
#include <net-snmp/library/container.h>
#else
#include "patched_container.h"
#endif

#include <net-snmp/agent/table_array.h>
#include "fixconfigs.h"


/**
 * initializes the bgp4PathAttrTable
 */
void init_bgp4_mib_1657_bgp4pathattrtable(void);

/**
 * unregisters the bgp4PathAttrTable
 */
void deinit_bgp4_mib_1657_bgp4pathattrtable(void);

/*************************************************************
 * oid declarations
 */
extern oid bgp4PathAttrTable_oid[];
extern size_t bgp4PathAttrTable_oid_len;

#define bgp4PathAttrTable_TABLE_OID 1,3,6,1,2,1,15,6
    
/*************************************************************
 * column number definitions for table bgp4PathAttrTable
 */
#define COLUMN_BGP4PATHATTRPEER 1
#define COLUMN_BGP4PATHATTRIPADDRPREFIXLEN 2
#define COLUMN_BGP4PATHATTRIPADDRPREFIX 3
#define COLUMN_BGP4PATHATTRORIGIN 4
#define COLUMN_BGP4PATHATTRASPATHSEGMENT 5
#define COLUMN_BGP4PATHATTRNEXTHOP 6
#define COLUMN_BGP4PATHATTRMULTIEXITDISC 7
#define COLUMN_BGP4PATHATTRLOCALPREF 8
#define COLUMN_BGP4PATHATTRATOMICAGGREGATE 9
#define COLUMN_BGP4PATHATTRAGGREGATORAS 10
#define COLUMN_BGP4PATHATTRAGGREGATORADDR 11
#define COLUMN_BGP4PATHATTRCALCLOCALPREF 12
#define COLUMN_BGP4PATHATTRBEST 13
#define COLUMN_BGP4PATHATTRUNKNOWN 14
#define bgp4PathAttrTable_COL_MIN 1
#define bgp4PathAttrTable_COL_MAX 14

#define UPDATE_REST_INTERVAL_ms 1000

#ifdef __cplusplus
};
#endif

#endif // __MIBS_BGP4_MIB_1657_PATHATTRTABLE_HH__
