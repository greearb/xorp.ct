// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/fea/data_plane/firewall/firewall_get_dummy.hh,v 1.2 2008/07/23 05:10:25 pavlin Exp $

#ifndef __FEA_DATA_PLANE_FIREWALL_FIREWALL_GET_DUMMY_HH__
#define __FEA_DATA_PLANE_FIREWALL_FIREWALL_GET_DUMMY_HH__

#include "fea/firewall_get.hh"


class FirewallGetDummy : public FirewallGet {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    FirewallGetDummy(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
    virtual ~FirewallGetDummy();

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
     * Obtain the IPv4 firewall table.
     *
     * @param firewall_entry_list the return-by-reference list with all
     * entries in the IPv4 firewall table.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int get_table4(list<FirewallEntry>& firewall_entry_list,
			   string& error_msg);

    /**
     * Obtain the IPv6 firewall table.
     *
     * @param firewall_entry_list the return-by-reference list with all
     * entries in the IPv6 firewall table.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int get_table6(list<FirewallEntry>& firewall_entry_list,
			   string& error_msg);

private:
};

#endif // __FEA_DATA_PLANE_FIREWALL_FIREWALL_GET_DUMMY_HH__
