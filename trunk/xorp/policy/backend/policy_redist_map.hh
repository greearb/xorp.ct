// vim:set sts=4 ts=8:

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

// $XORP$

#ifndef __POLICY_BACKEND_POLICY_REDIST_MAP_HH__
#define __POLICY_BACKEND_POLICY_REDIST_MAP_HH__

#include "policytags.hh"
#include <map>
#include <string>

/**
 * @short A Map between policytags and where the route should be redistributed.
 *
 * This map normally resides in the RIB. As routes pass through the rib, their
 * policytags need to be analyzed. According to these tags, the route must be
 * sent to various routing protocols to enable export policies.
 */
class PolicyRedistMap {
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


    // not impl
    PolicyRedistMap(const PolicyRedistMap&);
    PolicyRedistMap& operator=(const PolicyRedistMap&);
};

#endif // __POLICY_BACKEND_POLICY_REDIST_MAP_HH__
