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

#ident "$XORP: xorp/fea/ifconfig_get.cc,v 1.2 2004/06/02 22:52:39 pavlin Exp $"


#include "fea_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include <net/if.h>

#include "ifconfig.hh"
#include "ifconfig_get.hh"


//
// Get information about network interfaces configuration from the
// underlying system.
//


IfConfigGet::IfConfigGet(IfConfig& ifc)
    : _s4(-1),
      _s6(-1),
      _is_running(false),
      _ifc(ifc)
{
    
}

IfConfigGet::~IfConfigGet()
{
    if (_s4 >= 0)
	close(_s4);
    if (_s6 >= 0)
	close(_s6);
}

void
IfConfigGet::register_ifc()
{
    _ifc.register_ifc_get(this);
}

int
IfConfigGet::sock(int family)
{
    switch (family) {
    case AF_INET:
	return _s4;
#ifdef HAVE_IPV6
    case AF_INET6:
	return _s6;
#endif // HAVE_IPV6
    default:
	XLOG_FATAL("Unknown address family %d", family);
    }
    return (-1);
}

/**
 * Generate ifconfig like flags string
 * @param flags interface flags value from routing socket message.
 * @return ifconfig like flags string
 */
string
IfConfigGet::iff_flags(uint32_t flags)
{
    struct {
	uint32_t 	value;
	const char*	name;
    } iff_fl[] = {
#define IFF_FLAG_ENTRY(X) { IFF_##X, #X }
#ifdef IFF_UP
	IFF_FLAG_ENTRY(UP),
#endif
#ifdef IFF_BROADCAST
	IFF_FLAG_ENTRY(BROADCAST),
#endif
#ifdef IFF_DEBUG
	IFF_FLAG_ENTRY(DEBUG),
#endif
#ifdef IFF_LOOPBACK
	IFF_FLAG_ENTRY(LOOPBACK),
#endif
#ifdef IFF_POINTOPOINT
	IFF_FLAG_ENTRY(POINTOPOINT),
#endif
#ifdef IFF_SMART
	IFF_FLAG_ENTRY(SMART),
#endif
#ifdef IFF_RUNNING
	IFF_FLAG_ENTRY(RUNNING),
#endif
#ifdef IFF_NOARP
	IFF_FLAG_ENTRY(NOARP),
#endif
#ifdef IFF_PROMISC
	IFF_FLAG_ENTRY(PROMISC),
#endif
#ifdef IFF_ALLMULTI
	IFF_FLAG_ENTRY(ALLMULTI),
#endif
#ifdef IFF_OACTIVE
	IFF_FLAG_ENTRY(OACTIVE),
#endif
#ifdef IFF_SIMPLEX
	IFF_FLAG_ENTRY(SIMPLEX),
#endif
#ifdef IFF_LINK0
	IFF_FLAG_ENTRY(LINK0),
#endif
#ifdef IFF_LINK1
	IFF_FLAG_ENTRY(LINK1),
#endif
#ifdef IFF_LINK2
	IFF_FLAG_ENTRY(LINK2),
#endif
#ifdef IFF_ALTPHYS
	IFF_FLAG_ENTRY(ALTPHYS),
#endif
#ifdef IFF_MULTICAST
	IFF_FLAG_ENTRY(MULTICAST),
#endif
	{ 0, "" }  // for nitty compilers that don't like trailing ","
    };
    const size_t n_iff_fl = sizeof(iff_fl) / sizeof(iff_fl[0]);

    string ret("<");
    for (size_t i = 0; i < n_iff_fl; i++) {
	if (0 == (flags & iff_fl[i].value))
	    continue;
	flags &= ~iff_fl[i].value;
	ret += iff_fl[i].name;
	if (0 == flags)
	    break;
	ret += ",";
    }
    ret += ">";
    return ret;
}
