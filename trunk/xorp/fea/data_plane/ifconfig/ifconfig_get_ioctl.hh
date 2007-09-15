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

// $XORP: xorp/fea/data_plane/ifconfig/ifconfig_get_ioctl.hh,v 1.6 2007/09/15 05:10:21 pavlin Exp $

#ifndef __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_GET_IOCTL_HH__
#define __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_GET_IOCTL_HH__

#include "fea/ifconfig_get.hh"


class IfConfigGetIoctl : public IfConfigGet {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@see FeaDataPlaneManager).
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
    virtual int pull_config(IfTree& iftree);

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
