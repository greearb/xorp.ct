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

// $XORP: xorp/rtrmgr/config_operators.hh,v 1.10 2008/10/02 21:58:22 bms Exp $

#ifndef __RTRMGR_CONFIG_OPERATORS_HH__
#define __RTRMGR_CONFIG_OPERATORS_HH__

#include <string>

#include "rtrmgr_error.hh"

//
// Configuration file operators.
// XXX: Comparators must be less than modifiers.
//
enum ConfigOperator {
    OP_NONE		= 0,
    OP_EQ		= 1,
    OP_NE		= 2,
    OP_LT		= 3,
    OP_LTE		= 4,
    OP_GT		= 5,
    OP_GTE		= 6,
    OP_IPNET_EQ		= 7,
    OP_IPNET_NE		= 8,
    OP_IPNET_LT		= 9,
    OP_IPNET_GT		= 10,
    OP_IPNET_LE		= 11,
    OP_IPNET_GE		= 12,
    MAX_COMPARATOR	= OP_IPNET_GE,
    OP_ASSIGN		= 101,
    OP_ADD		= 102,
    OP_SUB		= 103,
    OP_DEL		= 104,
    MAX_MODIFIER	= OP_DEL
};

extern string operator_to_str(ConfigOperator op);
extern ConfigOperator lookup_operator(const string& s) throw (ParseError);

#endif // __RTRMGR_CONFIG_OPERATORS_HH__
