// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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



#include "rtrmgr_error.hh"

//
// Configuration file operators.
// XXX: Comparators must be less than modifiers.
//
enum ConfigOperator {
    OP_NONE	= 0,
    OP_EQ,
    OP_NE,
    OP_LT,
    OP_LTE,
    OP_GT,
    OP_GTE,
    OP_IPNET_EQ,
    OP_IPNET_NE,
    OP_IPNET_LT,
    OP_IPNET_GT,
    OP_IPNET_LE,
    OP_IPNET_GE,
    MAX_COMPARATOR	= OP_IPNET_GE,
    OP_ASSIGN	= 101,
    OP_ADD,
    OP_ADD_EQ,
    OP_SUB,
    OP_SUB_EQ,
    OP_MUL,
    OP_MUL_EQ,
    OP_DIV,
    OP_DIV_EQ,
    OP_LSHIFT,
    OP_LSHIFT_EQ,
    OP_RSHIFT,
    OP_RSHIFT_EQ,
    OP_BITAND,
    OP_BITAND_EQ,
    OP_BITOR,
    OP_BITOR_EQ,
    OP_BITXOR,
    OP_BITXOR_EQ,
    OP_DEL,
    MAX_MODIFIER	= OP_DEL
};

extern string operator_to_str(ConfigOperator op);
extern ConfigOperator lookup_operator(const string& s) throw (ParseError);

#endif // __RTRMGR_CONFIG_OPERATORS_HH__
