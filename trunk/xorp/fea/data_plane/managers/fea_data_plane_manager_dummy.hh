// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007 International Computer Science Institute
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

// $XORP: xorp/fea/data_plane/managers/fea_data_plane_manager_dummy.hh,v 1.3 2007/07/16 23:56:13 pavlin Exp $

#ifndef __FEA_DATA_PLANE_MANAGERS_FEA_DATA_PLANE_MANAGER_DUMMY_HH__
#define __FEA_DATA_PLANE_MANAGERS_FEA_DATA_PLANE_MANAGER_DUMMY_HH__

#include "fea/fea_data_plane_manager.hh"

class FibConfigEntryGetDummy;
class FibConfigEntryObserverDummy;
class FibConfigEntrySetDummy;
class FibConfigTableGetDummy;
class FibConfigTableObserverDummy;
class FibConfigTableSetDummy;
class IfConfigGetDummy;
class IfConfigObserverDummy;
class IfConfigSetDummy;


/**
 * FEA data plane manager class for Dummy FEA.
 */
class FeaDataPlaneManagerDummy : public FeaDataPlaneManager {
public:
    /**
     * Constructor.
     *
     * @param fea_node the @ref FeaNode this manager belongs to.
     */
    FeaDataPlaneManagerDummy(FeaNode& fea_node);

    /**
     * Virtual destructor.
     */
    virtual ~FeaDataPlaneManagerDummy();

    /**
     * Load the plugins.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int load_plugins(string& error_msg);

    /**
     * Register the plugins.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_plugins(string& error_msg);

    /**
     * Allocate IoLink plugin instance.
     *
     * @param iftree the interface tree to use.
     * @param if_name the interface name.
     * @param vif_name the vif name.
     * @param ether_type the EtherType protocol number. If it is 0 then
     * it is unused.
     * @param filter_program the option filter program to be applied on the
     * received packets. The program uses tcpdump(1) style expression.
     * @return a new instance of @ref IoLink plugin on success, otherwise NULL.
     */
    IoLink* allocate_io_link(const IfTree& iftree,
			     const string& if_name,
			     const string& vif_name,
			     uint16_t ether_type,
			     const string& filter_program);

    /**
     * Allocate IoIp plugin instance.
     *
     * @param iftree the interface tree to use.
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param ip_protocol the IP protocol number (IPPROTO_*).
     * @return a new instance of @ref IoIp plugin on success, otherwise NULL.
     */
    IoIp* allocate_io_ip(const IfTree& iftree, int family,
			 uint8_t ip_protocol);

private:
};

#endif // __FEA_DATA_PLANE_MANAGERS_FEA_DATA_PLANE_MANAGER_DUMMY_HH__
