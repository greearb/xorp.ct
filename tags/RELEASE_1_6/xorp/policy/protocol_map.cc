// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/policy/protocol_map.cc,v 1.7 2008/10/02 21:58:00 bms Exp $"

#include "policy_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"

#include "protocol_map.hh"

ProtocolMap::ProtocolMap()
{
}

const string&
ProtocolMap::xrl_target(const string& protocol)
{
    Map::iterator i = _map.find(protocol);

    // by default, the protocol has the same XRL target name.
    if (i == _map.end()) {
	set_xrl_target(protocol, protocol);
	i = _map.find(protocol);
	XLOG_ASSERT(i != _map.end());
    }

    return i->second;
}

void
ProtocolMap::set_xrl_target(const string& protocol, const string& target)
{
    _map[protocol] = target;
}

const string&
ProtocolMap::protocol(const string& target)
{
    // XXX lame
    for (Map::iterator i = _map.begin(); i != _map.end(); ++i) {
	string& t = i->second;

	if (target == t)
	    return i->first;
    }

    // by default protocol = target
    // The case in which a protocol called target exists is probably bad...
    XLOG_ASSERT(_map.find(target) == _map.end());

    set_xrl_target(target, target);
    return protocol(target); // an assert that item was added would be good, in
			     // order to avoid infinite recursion...
}
