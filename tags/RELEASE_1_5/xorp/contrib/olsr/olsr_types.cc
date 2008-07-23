// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/contrib/olsr/olsr_types.cc,v 1.2 2008/05/25 22:38:33 pavlin Exp $"

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
