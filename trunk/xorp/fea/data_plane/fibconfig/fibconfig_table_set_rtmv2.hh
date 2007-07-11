// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

// $XORP: xorp/fea/data_plane/fibconfig/fibconfig_table_set_rtmv2.hh,v 1.1 2007/06/07 01:28:42 pavlin Exp $

#ifndef __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_TABLE_SET_RTMV2_HH__
#define __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_TABLE_SET_RTMV2_HH__

#include "fea/fibconfig_table_set.hh"


class FibConfigTableSetRtmV2 : public FibConfigTableSet {
public:
    FibConfigTableSetRtmV2(FeaDataPlaneManager& fea_data_plane_manager);
    virtual ~FibConfigTableSetRtmV2();

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
     * Set the unicast forwarding table.
     *
     * @param fte_list the list with all entries to install into
     * the unicast forwarding table.
     *
     * @return true on success, otherwise false.
     */
    virtual bool set_table4(const list<Fte4>& fte_list);

    /**
     * Delete all entries in the routing table. Must be within a
     * configuration interval.
     *
     * @return true on success, otherwise false.
     */
    virtual bool delete_all_entries4();

    /**
     * Set the unicast forwarding table.
     *
     * @param fte_list the list with all entries to install into
     * the unicast forwarding table.
     *
     * @return true on success, otherwise false.
     */
    virtual bool set_table6(const list<Fte6>& fte_list);
    
    /**
     * Delete all entries in the routing table. Must be within a
     * configuration interval.
     *
     * @return true on success, otherwise false.
     */
    virtual bool delete_all_entries6();
    
private:
};

#endif // __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_TABLE_SET_RTMV2_HH__
