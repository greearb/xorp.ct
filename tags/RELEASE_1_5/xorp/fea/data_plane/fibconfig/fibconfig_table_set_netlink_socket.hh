// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/fea/data_plane/fibconfig/fibconfig_table_set_netlink_socket.hh,v 1.7 2008/04/11 02:36:04 pavlin Exp $

#ifndef __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_TABLE_SET_NETLINK_SOCKET_HH__
#define __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_TABLE_SET_NETLINK_SOCKET_HH__

#include "fea/fibconfig_table_set.hh"
#include "fea/data_plane/control_socket/netlink_socket.hh"


class FibConfigTableSetNetlinkSocket : public FibConfigTableSet {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    FibConfigTableSetNetlinkSocket(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
    virtual ~FibConfigTableSetNetlinkSocket();

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
     * Set the IPv4 unicast forwarding table.
     *
     * @param fte_list the list with all entries to install into
     * the IPv4 unicast forwarding table.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_table4(const list<Fte4>& fte_list);

    /**
     * Delete all entries in the IPv4 unicast forwarding table.
     *
     * Must be within a configuration interval.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_all_entries4();

    /**
     * Set the IPv6 unicast forwarding table.
     *
     * @param fte_list the list with all entries to install into
     * the IPv6 unicast forwarding table.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int set_table6(const list<Fte6>& fte_list);

    /**
     * Delete all entries in the IPv6 unicast forwarding table.
     *
     * Must be within a configuration interval.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int delete_all_entries6();
    
private:
};

#endif // __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_TABLE_SET_NETLINK_SOCKET_HH__
