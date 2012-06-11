// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others
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

#ifndef __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_GET_NETLINK_SOCKET_HH__
#define __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_GET_NETLINK_SOCKET_HH__

#include <xorp_config.h>

#ifdef HAVE_NETLINK_SOCKETS

#include "fea/ifconfig_get.hh"
#include "fea/data_plane/control_socket/netlink_socket.hh"


class IfConfigGetNetlinkSocket : public IfConfigGet,
				 public NetlinkSocket {
public:
    /**
     * Constructor.
     *
     * @param fea_data_plane_manager the corresponding data plane manager
     * (@ref FeaDataPlaneManager).
     */
    IfConfigGetNetlinkSocket(FeaDataPlaneManager& fea_data_plane_manager);

    /**
     * Virtual destructor.
     */
    virtual ~IfConfigGetNetlinkSocket();

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
    virtual int pull_config(const IfTree* local_cfg, IfTree& iftree);


    virtual bool can_pull_one() { return can_get_single != 0; }

    /** If_index can be -1 if unknown:  We will try to resolve it from ifname.
     */
    virtual int pull_config_one(IfTree& iftree, const char* ifname, int if_index);

    /**
     * Parse information about network interface configuration change from
     * the underlying system.
     *
     * The information to parse is in NETLINK format
     * (e.g., obtained by netlink(7) sockets mechanism).
     *
     * @param ifconfig the IfConfig instance.
     * @param iftree the IfTree storage to store the parsed information.
     * @param buffer the buffer with the data to parse.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     * @see IfTree.
     */
    static int parse_buffer_netlink_socket(IfConfig& ifconfig, IfTree& iftree,
					   vector<uint8_t>& buffer) {
	bool modified = false;
	int nl_errno = 0;
	return parse_buffer_netlink_socket(ifconfig, iftree, buffer, modified, nl_errno);
    }

    /** Same as above, but also return whether or not something was actually modified in iftree.
     */
    static int parse_buffer_netlink_socket(IfConfig& ifconfig, IfTree& iftree,
					   vector<uint8_t>& buffer,
					   bool& modified, int& nl_errno);
    
private:
    int read_config(const IfTree* local_config, IfTree& iftree);
    int read_config_all(IfTree& iftree);
    int read_config_one(IfTree& iftree, const char* ifname, int if_index,
			int& nl_errno);
    int try_read_config_one(IfTree& iftree, const char* ifname, int if_index);

    int can_get_single;

    NetlinkSocketReader	_ns_reader;
};

#endif
#endif // __FEA_DATA_PLANE_IFCONFIG_IFCONFIG_GET_NETLINK_SOCKET_HH__
