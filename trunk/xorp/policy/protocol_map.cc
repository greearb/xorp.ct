// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/policy/protocol_map.cc,v 1.5 2008/01/04 03:17:12 pavlin Exp $"

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
