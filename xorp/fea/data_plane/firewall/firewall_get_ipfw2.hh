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

// $XORP: xorp/fea/data_plane/firewall/firewall_get_ipfw2.hh,v 1.3 2008/10/02 21:57:02 bms Exp $

#ifndef __FEA_DATA_PLANE_FIREWALL_FIREWALL_GET_IPFW2_HH__
#define __FEA_DATA_PLANE_FIREWALL_FIREWALL_GET_IPFW2_HH__

#include "fea/firewall_get.hh"


class FirewallGetIpfw2 : public FirewallGet {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    FirewallGetIpfw2(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
    virtual ~FirewallGetIpfw2();

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
    /**
     * Obtain the firewall table for a specific address family.
     *
     * @param family the address family.
     * @param firewall_entry_list the return-by-reference list with all
     * entries in the firewall table for the given address family.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int get_table(int family, list<FirewallEntry>& firewall_entry_list,
		  string& error_msg);

    int		_s4;		// The socket for firewall access
};

#endif // __FEA_DATA_PLANE_FIREWALL_FIREWALL_GET_IPFW2_HH__
