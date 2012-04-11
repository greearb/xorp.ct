// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// $XORP: xorp/policy/backend/policy_redist_map.hh,v 1.8 2008/10/02 21:58:04 bms Exp $

#ifndef __POLICY_BACKEND_POLICY_REDIST_MAP_HH__
#define __POLICY_BACKEND_POLICY_REDIST_MAP_HH__






#include "policytags.hh"

/**
 * @short A Map between policytags and where the route should be redistributed.
 *
 * This map normally resides in the RIB. As routes pass through the rib, their
 * policytags need to be analyzed. According to these tags, the route must be
 * sent to various routing protocols to enable export policies.
 */
class PolicyRedistMap :
    public NONCOPYABLE
{
public:
    PolicyRedistMap();
    ~PolicyRedistMap();

    /**
     * Configure redistribution to a protcol for these tags.
     *
     * @param protocol destination protocol for these tags.
     * @param tags policytags which need to be redistributed to the protocol.
     */
    void insert(const string& protocol, const PolicyTags& tags);

    /**
     * Reset the redistribution map
     */
    void reset();

    /**
     * Obtain which protocols the route containing these tags should be sent to.
     *
     * @param out will be filled with protocols route should be sent to.
     * @param tags policytags that need to be resolved.
     */
    void get_protocols(set<string>& out, const PolicyTags& tags);

private:
    // XXX: this should be the other way around for faster lookups
    typedef map<string,PolicyTags*> Map;
    Map _map;
};

#endif // __POLICY_BACKEND_POLICY_REDIST_MAP_HH__
