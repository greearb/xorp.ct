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

#ident "$XORP$"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvxnet.hh"

#include "fticonfig.hh"


// TODO: XXX: PAVPAVPAV: temporary here
// #define DEBUG_CLICK

//
// Get single-entry information from the unicast forwarding table.
//
// The mechanism to obtain the information is click(1)
// (e.g., see http://www.pdos.lcs.mit.edu/click/).
//


FtiConfigEntryGetClick::FtiConfigEntryGetClick(FtiConfig& ftic)
    : FtiConfigEntryGet(ftic),
      ClickSocket(ftic.eventloop()),
      _cs_reader(*(ClickSocket *)this)
{
#ifdef DEBUG_CLICK      // TODO: XXX: PAVPAVPAV
    register_ftic_secondary();
    ClickSocket::enable_user_click(true);
#endif
}

FtiConfigEntryGetClick::~FtiConfigEntryGetClick()
{
    stop();
}

int
FtiConfigEntryGetClick::start()
{
    if (_is_running)
	return (XORP_OK);

#ifndef DEBUG_CLICK     // TODO: XXX: PAVPAVPAV
    register_ftic_secondary();
#endif

    if (ClickSocket::start() < 0)
	return (XORP_ERROR);

    _is_running = true;

    return (XORP_OK);
}

int
FtiConfigEntryGetClick::stop()
{
    int ret_value = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    ret_value = ClickSocket::stop();

    _is_running = false;

    return (ret_value);
}

/**
 * Lookup a route by destination address.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FtiConfigEntryGetClick::lookup_route_by_dest4(const IPv4& dst, Fte4& fte)
{
    FteX ftex(dst.af());
    bool ret_value = false;

    ret_value = lookup_route_by_dest(IPvX(dst), ftex);
    
    fte = Fte4(ftex.net().get_ipv4net(), ftex.nexthop().get_ipv4(),
	       ftex.ifname(), ftex.vifname(), ftex.metric(),
	       ftex.admin_distance(), ftex.xorp_route());
    
    return (ret_value);
}

/**
 * Lookup route by network address.
 *
 * @param dst network address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FtiConfigEntryGetClick::lookup_route_by_network4(const IPv4Net& dst, Fte4& fte)
{
    list<Fte4> fte_list4;

    if (ftic().get_table4(fte_list4) != true)
	return (false);

    list<Fte4>::iterator iter4;
    for (iter4 = fte_list4.begin(); iter4 != fte_list4.end(); ++iter4) {
	Fte4& fte4 = *iter4;
	if (fte4.net() == dst) {
	    fte = fte4;
	    return (true);
	}
    }

    return (false);
}

/**
 * Lookup a route by destination address.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FtiConfigEntryGetClick::lookup_route_by_dest6(const IPv6& dst, Fte6& fte)
{
    FteX ftex(dst.af());
    bool ret_value = false;

    ret_value = lookup_route_by_dest(IPvX(dst), ftex);
    
    fte = Fte6(ftex.net().get_ipv6net(), ftex.nexthop().get_ipv6(),
	       ftex.ifname(), ftex.vifname(), ftex.metric(),
	       ftex.admin_distance(), ftex.xorp_route());
    
    return (ret_value);
}

/**
 * Lookup route by network address.
 *
 * @param dst network address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FtiConfigEntryGetClick::lookup_route_by_network6(const IPv6Net& dst, Fte6& fte)
{ 
    list<Fte6> fte_list6;

    if (ftic().get_table6(fte_list6) != true)
	return (false);

    list<Fte6>::iterator iter6;
    for (iter6 = fte_list6.begin(); iter6 != fte_list6.end(); ++iter6) {
	Fte6& fte6 = *iter6;
	if (fte6.net() == dst) {
	    fte = fte6;
	    return (true);
	}
    }

    return (false);
}

/**
 * Lookup a route by destination address.
 *
 * @param dst host address to resolve.
 * @param fte return-by-reference forwarding table entry.
 *
 * @return true on success, otherwise false.
 */
bool
FtiConfigEntryGetClick::lookup_route_by_dest(const IPvX& dst, FteX& fte)
{
    // TODO: XXX: PAVPAVPAV: implement it!!
    UNUSED(dst);
    UNUSED(fte);

    return (false);
}
