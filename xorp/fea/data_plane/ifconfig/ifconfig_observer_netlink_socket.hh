// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2010 XORP, Inc and Others
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

// $XORP: xorp/fea/data_plane/ifconfig/ifconfig_observer_netlink_socket.hh,v 1.9 2008/10/02 21:57:06 bms Exp $

#ifndef __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_OBSERVER_NETLINK_SOCKET_HH__
#define __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_OBSERVER_NETLINK_SOCKET_HH__

#include <xorp_config.h>

#ifdef HAVE_NETLINK_SOCKETS

#include "fea/ifconfig_observer.hh"
#include "fea/data_plane/control_socket/netlink_socket.hh"


class IfConfigObserverNetlinkSocket : public IfConfigObserver,
				      public NetlinkSocket,
				      public NetlinkSocketObserver {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    IfConfigObserverNetlinkSocket(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
    virtual ~IfConfigObserverNetlinkSocket();

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
     * Receive data from the underlying system.
     * 
     * @param buffer the buffer with the received data.
     */
    virtual void receive_data(const vector<uint8_t>& buffer);
    
    void netlink_socket_data(const vector<uint8_t>& buffer);
    
private:
};

#endif
#endif // __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_OBSERVER_NETLINK_SOCKET_HH__
