// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/withdrawn_route.cc,v 1.12 2002/12/09 18:28:51 hodson Exp $"


#include "bgp_module.h"
#include "config.h"
#include <string>
#include "withdrawn_route.hh"

#if 0
BGPWithdrawnRoute::BGPWithdrawnRoute(uint8_t* d, uint8_t s) :
    BGPUpdateAttrib(d,s) {};
#endif

BGPWithdrawnRoute::BGPWithdrawnRoute(uint32_t d, uint8_t s) :
    BGPUpdateAttrib(d,s) {};

BGPWithdrawnRoute::BGPWithdrawnRoute(const IPv4Net& p) :
    BGPUpdateAttrib(p) {};

string BGPWithdrawnRoute::str() const
{
    string s;
    s = "Withdrawn route " + _net.str();
    return s;
}

bool BGPWithdrawnRoute::operator<(const BGPWithdrawnRoute &him) const
{
    return _net < him.net();
}
    
bool BGPWithdrawnRoute::operator==(const BGPWithdrawnRoute &him) const
{
    return _net == him.net();
}
