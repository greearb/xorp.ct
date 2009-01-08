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


#ifndef __MIBS_BGP4_MIB_1657_BGPPEERTABLE_HH__
#define __MIBS_BGP4_MIB_1657_BGPPEERTABLE_HH__

/* function declarations */
void init_bgp4_mib_1657_bgppeertable(void);
void initialize_table_bgpPeerTable(void);
Netsnmp_Node_Handler bgpPeerTable_handler;

Netsnmp_First_Data_Point  bgpPeerTable_get_first_data_point;
Netsnmp_Next_Data_Point   bgpPeerTable_get_next_data_point;

/* column number definitions for table bgpPeerTable */
       #define COLUMN_BGPPEERIDENTIFIER				1
       #define COLUMN_BGPPEERSTATE				2
       #define COLUMN_BGPPEERADMINSTATUS			3
       #define COLUMN_BGPPEERNEGOTIATEDVERSION			4
       #define COLUMN_BGPPEERLOCALADDR				5
       #define COLUMN_BGPPEERLOCALPORT				6
       #define COLUMN_BGPPEERREMOTEADDR				7
       #define COLUMN_BGPPEERREMOTEPORT				8
       #define COLUMN_BGPPEERREMOTEAS				9
       #define COLUMN_BGPPEERINUPDATES				10
       #define COLUMN_BGPPEEROUTUPDATES				11
       #define COLUMN_BGPPEERINTOTALMESSAGES			12
       #define COLUMN_BGPPEEROUTTOTALMESSAGES			13
       #define COLUMN_BGPPEERLASTERROR				14
       #define COLUMN_BGPPEERFSMESTABLISHEDTRANSITIONS		15
       #define COLUMN_BGPPEERFSMESTABLISHEDTIME			16
       #define COLUMN_BGPPEERCONNECTRETRYINTERVAL		17
       #define COLUMN_BGPPEERHOLDTIME				18
       #define COLUMN_BGPPEERKEEPALIVE				19
       #define COLUMN_BGPPEERHOLDTIMECONFIGURED			20
       #define COLUMN_BGPPEERKEEPALIVECONFIGURED		21
       #define COLUMN_BGPPEERMINASORIGINATIONINTERVAL		22
       #define COLUMN_BGPPEERMINROUTEADVERTISEMENTINTERVAL	23
       #define COLUMN_BGPPEERINUPDATEELAPSEDTIME		24

#endif // __MIBS_BGP4_MIB_1657_BGPPEERTABLE_HH__
