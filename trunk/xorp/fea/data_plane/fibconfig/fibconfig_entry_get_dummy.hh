// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/fea/data_plane/fibconfig/fibconfig_entry_get_dummy.hh,v 1.5 2008/01/03 22:59:36 pavlin Exp $

#ifndef __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_ENTRY_GET_DUMMY_HH__
#define __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_ENTRY_GET_DUMMY_HH__

#include "fea/fibconfig_entry_get.hh"


class FibConfigEntryGetDummy : public FibConfigEntryGet {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    FibConfigEntryGetDummy(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
    virtual ~FibConfigEntryGetDummy();

    /**
     * Start operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start(string& error_msg);
    
    /**
     * Stop operation.
     * 
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop(string& error_msg);

    /**
     * Lookup a route by destination address.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int lookup_route_by_dest4(const IPv4& dst, Fte4& fte);

    /**
     * Lookup route by network address.
     *
     * @param dst network address to resolve.
     * @param fte return-by-reference forwarding table entry.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int lookup_route_by_network4(const IPv4Net& dst, Fte4& fte);

    /**
     * Lookup a route by destination address.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int lookup_route_by_dest6(const IPv6& dst, Fte6& fte);

    /**
     * Lookup route by network address.
     *
     * @param dst network address to resolve.
     * @param fte return-by-reference forwarding table entry.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int lookup_route_by_network6(const IPv6Net& dst, Fte6& fte);

private:
};

#endif // __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_ENTRY_GET_DUMMY_HH__
