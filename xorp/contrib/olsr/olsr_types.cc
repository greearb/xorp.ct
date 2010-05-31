// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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



#include "olsr_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ref_ptr.hh"
#include "libxorp/timeval.hh"

#include "olsr_types.hh"

#define NAME_CASE(x) case OlsrTypes:: x : return x##_NAME ;
#define NUM_CASE(x) case x : return #x ;

const double EightBitTime::_scaling_factor = 0.0625f;

static const char* TCR_MPRS_IN_NAME = "mprs_in";
static const char* TCR_MPRS_INOUT_NAME = "mprs_inout";
static const char* TCR_ALL_NAME = "all";

const char*
tcr_to_str(const OlsrTypes::TcRedundancyType t)
{
    switch (t) {
	NAME_CASE(TCR_MPRS_IN);
	NAME_CASE(TCR_MPRS_INOUT);
	NAME_CASE(TCR_ALL);
    }
    XLOG_UNREACHABLE();
    return 0;
}

static const char* WILL_NEVER_NAME = "never";
static const char* WILL_LOW_NAME = "low";
static const char* WILL_DEFAULT_NAME = "default";
static const char* WILL_HIGH_NAME = "high";
static const char* WILL_ALWAYS_NAME = "always";

const char*
will_to_str(const OlsrTypes::WillType t)
{
    switch (t) {
	NAME_CASE(WILL_NEVER);
	NUM_CASE(2);
	NAME_CASE(WILL_LOW);
	NAME_CASE(WILL_DEFAULT);
	NUM_CASE(4);
	NUM_CASE(5);
	NAME_CASE(WILL_HIGH);
	NAME_CASE(WILL_ALWAYS);
    }
    XLOG_UNREACHABLE();
    return 0;
}

static const char* VT_UNKNOWN_NAME = "UNKNOWN";
static const char* VT_NEIGHBOR_NAME = "N";
static const char* VT_TWOHOP_NAME = "N2";
static const char* VT_TOPOLOGY_NAME = "TC";
static const char* VT_MID_NAME = "MID";
static const char* VT_HNA_NAME = "HNA";

const char*
vt_to_str(const OlsrTypes::VertexType vt)
{
    switch (vt) {
	NAME_CASE(VT_UNKNOWN);
	NAME_CASE(VT_NEIGHBOR);
	NAME_CASE(VT_TWOHOP);
	NAME_CASE(VT_TOPOLOGY);
	NAME_CASE(VT_MID);
	NAME_CASE(VT_HNA);
    }
    XLOG_UNREACHABLE();
    return 0;
}

#undef NUM_CASE
#undef NAME_CASE
