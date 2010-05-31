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

// $XORP: xorp/fea/data_plane/ifconfig/ifconfig_get_ioctl.hh,v 1.11 2008/10/02 21:57:04 bms Exp $

#ifndef __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_GET_IOCTL_HH__
#define __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_GET_IOCTL_HH__

#include "fea/ifconfig_get.hh"


class IfConfigGetIoctl : public IfConfigGet {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    IfConfigGetIoctl(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
    virtual ~IfConfigGetIoctl();
    
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
     * Pull the network interface information from the underlying system.
     * 
     * @param iftree the IfTree storage to store the pulled information.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int pull_config(const IfTree* local_config, IfTree& iftree);

    /**
     * Parse information about network interface configuration change from
     * the underlying system.
     * 
     * The information to parse is in "struct ifreq" format
     * (e.g., obtained by ioctl(SIOCGIFCONF) mechanism).
     * 
     * @param ifconfig the IfConfig instance.
     * @param iftree the IfTree storage to store the parsed information.
     * @param family the address family to consider only (e.g., AF_INET
     * or AF_INET6 for IPv4 and IPv6 respectively).
     * @param buffer the buffer with the data to parse.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     * @see IfTree.
     */
    static int parse_buffer_ioctl(IfConfig& ifconfig, IfTree& iftree,
				  int family, const vector<uint8_t>& buffer);

private:
    int read_config(IfTree& iftree);

    int _s4;
    int _s6;
};

#endif // __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_GET_IOCTL_HH__
