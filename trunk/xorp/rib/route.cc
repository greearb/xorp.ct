// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/rib/route.cc,v 1.5 2003/03/16 07:18:58 pavlin Exp $"

#include "rib_module.h"

#include "route.hh"


RouteEntry::RouteEntry(Vif *vif, NextHop *nexthop, const Protocol& protocol, 
		       uint16_t metric)
    : _vif(vif),
      _nexthop(nexthop),
      _protocol(protocol),
      _admin_distance(255),
      _metric(metric)
{
}

RouteEntry::~RouteEntry()
{
}
