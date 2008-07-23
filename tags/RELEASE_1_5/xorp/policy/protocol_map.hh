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

// $XORP: xorp/policy/protocol_map.hh,v 1.4 2008/01/04 03:17:12 pavlin Exp $

#ifndef __POLICY_PROTOCOL_MAP_HH__
#define __POLICY_PROTOCOL_MAP_HH__

#include <string>
#include <map>

/**
 * @short Maps protocols to the XORP process name.
 *
 * By default the mapping is the protocol name itself unless an entry has been
 * explicitly added.  This class is used to map user configuration directives
 * regarding protocols to the actual name of the XORP process for that protocol.
 */
class ProtocolMap {
public:
    ProtocolMap();

    /**
     * Determine the XRL target for a protocol.
     *
     * @return the XRL target for the protocol.
     * @param protocol the protocol for which the XRL target is wanted.
     */
    const string& xrl_target(const string& protocol);

    /**
     * Set the XRL target for a protocol.
     *
     * @param protocol the protocol for which the XRL target needs to be set.
     * @param target the XRL target for the protocol.
     */
    void set_xrl_target(const string& protocol, const string& target);

    /**
     * Return internal protocol name based on XRL target.
     *
     * @return protocol name.
     * @param target the XRL target for the protocol.
     */
    const string& protocol(const string& target);

private:
    typedef map<string, string> Map;

    ProtocolMap(const ProtocolMap&); // not impl
    ProtocolMap& operator=(const ProtocolMap&); // not impl

    Map _map;
};

#endif // __POLICY_PROTOCOL_MAP_HH__
