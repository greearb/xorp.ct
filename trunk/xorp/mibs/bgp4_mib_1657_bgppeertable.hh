// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software")
// to deal in the Software without restriction, subject to the conditions
// listed in the XORP LICENSE file. These conditions include: you must
// preserve this copyright notice, and you cannot mention the copyright
// holders in advertising related to the Software without their permission.
// The Software is provided WITHOUT ANY WARRANTY, EXPRESS OR IMPLIED. This
// notice is a summary of the XORP LICENSE file; the license in that file is
// legally binding.


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
