// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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


#ifndef __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_ENTRY_SET_RTMV2_HH__
#define __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_ENTRY_SET_RTMV2_HH__

#include "fea/fibconfig_entry_set.hh"

class WinRtmPipe;


class FibConfigEntrySetRtmV2 : public FibConfigEntrySet {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    FibConfigEntrySetRtmV2(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
    virtual ~FibConfigEntrySetRtmV2();

    /**
     * Start operation.
     *
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
     * Add a single IPv4 forwarding entry.
     *
     * Must be within a configuration interval.
     *
     * @param fte the entry to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_entry4(const Fte4& fte);

    /**
     * Delete a single IPv4 forwarding entry.
     *
     * Must be with a configuration interval.
     *
     * @param fte the entry to delete. Only destination and netmask are used.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_entry4(const Fte4& fte);

    /**
     * Add a single IPv6 forwarding entry.
     *
     * Must be within a configuration interval.
     *
     * @param fte the entry to add.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int add_entry6(const Fte6& fte);

    /**
     * Delete a single IPv6 forwarding entry.
     *
     * Must be within a configuration interval.
     *
     * @param fte the entry to delete. Only destination and netmask are used.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_entry6(const Fte6& fte);

    virtual int notify_table_id_change(uint32_t new_tbl) {
	UNUSED(new_tbl);
	return 0;
    }

private:
    int add_entry(const FteX& fte);
    int delete_entry(const FteX& fte);

    WinRtmPipe*		_rs4;
    WinRtmPipe*		_rs6;
};

#endif // __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_ENTRY_SET_RTMV2_HH__
