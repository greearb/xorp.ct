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

// $XORP: xorp/fea/data_plane/fibconfig/fibconfig_entry_set_routing_socket.hh,v 1.6 2008/01/04 03:16:00 pavlin Exp $

#ifndef __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_ENTRY_SET_ROUTING_SOCKET_HH__
#define __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_ENTRY_SET_ROUTING_SOCKET_HH__

#include "fea/fibconfig_entry_set.hh"
#include "fea/data_plane/control_socket/routing_socket.hh"


class FibConfigEntrySetRoutingSocket : public FibConfigEntrySet,
				       public RoutingSocket {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    FibConfigEntrySetRoutingSocket(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
    virtual ~FibConfigEntrySetRoutingSocket();

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

private:
    int add_entry(const FteX& fte);
    int delete_entry(const FteX& fte);
};

#endif // __FEA_DATA_PLANE_FIBCONFIG_FIBCONFIG_ENTRY_SET_ROUTING_SOCKET_HH__
