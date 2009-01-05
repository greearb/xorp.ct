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

// $XORP: xorp/policy/protocol_map.hh,v 1.6 2008/10/02 21:58:00 bms Exp $

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
