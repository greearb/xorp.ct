// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP$"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"

#include "ospf_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipnet.hh"

#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"

#include "ospf.hh"
#include "vlink.hh"

template <typename A>
bool
Vlink<A>::create_vlink(OspfTypes::RouterID rid)
{
    if (0 != _vlinks.count(rid)) {
	XLOG_WARNING("Virtual link to %s already exists", pr_id(rid).c_str());
	return false;
    }

    Vstate v;
    _vlinks[rid] = v;

    return true;
}

template <typename A>
bool
Vlink<A>::delete_vlink(OspfTypes::RouterID rid)
{
    if (0 == _vlinks.count(rid)) {
	XLOG_WARNING("Virtual link to %s doesn't exist", pr_id(rid).c_str());
	return false;
    }

    _vlinks.erase(_vlinks.find(rid));

    return true;
}

template <typename A>
bool
Vlink<A>::set_transit_area(OspfTypes::RouterID rid,
			   OspfTypes::AreaID transit_area)
{
    if (0 == _vlinks.count(rid)) {
	XLOG_WARNING("Virtual link to %s doesn't exist", pr_id(rid).c_str());
	return false;
    }

    typename map <OspfTypes::RouterID, Vstate>::iterator i = _vlinks.find(rid);
    XLOG_ASSERT(_vlinks.end() != i);

    i->second._transit_area = transit_area;

    return true;
}

template <typename A>
bool
Vlink<A>::get_transit_area(OspfTypes::RouterID rid,
			   OspfTypes::AreaID& transit_area)
{
    if (0 == _vlinks.count(rid)) {
	XLOG_WARNING("Virtual link to %s doesn't exist", pr_id(rid).c_str());
	return false;
    }

    typename map <OspfTypes::RouterID, Vstate>::iterator i = _vlinks.find(rid);
    XLOG_ASSERT(_vlinks.end() != i);

    transit_area = i->second._transit_area;

    return true;
}

template class Vlink<IPv4>;
template class Vlink<IPv6>;
