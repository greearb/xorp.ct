// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2008 XORP, Inc.
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

// $XORP: xorp/fea/fea_node.hh,v 1.16 2008/07/23 05:10:07 pavlin Exp $


#ifndef __FEA_FEA_NODE_HH__
#define __FEA_FEA_NODE_HH__


//
// FEA (Forwarding Engine Abstraction) node implementation.
//

#include "libxorp/profile.hh"

#include "fibconfig.hh"
#include "firewall_manager.hh"
#include "ifconfig.hh"
#include "io_link_manager.hh"
#include "io_ip_manager.hh"
#include "io_tcpudp_manager.hh"
#include "nexthop_port_mapper.hh"

class EventLoop;
class FeaIo;

/**
 * @short The FEA (Forwarding Engine Abstraction) node class.
 * 
 * There should be one node per FEA instance.
 */
class FeaNode {
public:
    /**
     * Constructor for a given event loop.
     *
     * @param eventloop the event loop to use.
     * @param fea_io the FeaIo instance to use.
     * @param is_dummy if true, then run the FEA in dummy mode.
     */
    FeaNode(EventLoop& eventloop, FeaIo& fea_io, bool is_dummy);

    /**
     * Destructor
     */
    virtual	~FeaNode();

    /**
     * Startup the service operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		startup();

    /**
     * Shutdown the service operation.
     *
     * Gracefully shutdown the FEA.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		shutdown();

    /**
     * Test whether the service is running.
     *
     * @return true if the service is still running, otherwise false.
     */
    bool	is_running() const;

    /**
     * Return true if the underlying system supports IPv4.
     * 
     * @return true if the underlying system supports IPv4, otherwise false.
     */
    bool have_ipv4() const;

    /**
     * Return true if the underlying system supports IPv6.
     * 
     * @return true if the underlying system supports IPv6, otherwise false.
     */
    bool have_ipv6() const;

    /**
     * Test if running in dummy mode.
     * 
     * @return true if running in dummy mode, otherwise false.
     */
    bool	is_dummy() const { return _is_dummy; }

    /**
     * Get the event loop this service is added to.
     * 
     * @return the event loop this service is added to.
     */
    EventLoop& eventloop() { return (_eventloop); }

    /**
     * Get the Profile instance.
     *
     * @return a reference to the Profile instance.
     * @see Profile.
     */
    Profile& profile() { return (_profile); }

    /**
     * Get the NexthopPortMapper instance.
     *
     * @return a reference to the NexthopPortMapper instance.
     * @see NexthopPortMapper.
     */
    NexthopPortMapper& nexthop_port_mapper() { return (_nexthop_port_mapper); }

    /**
     * Get the IfConfig instance.
     *
     * @return a reference to the IfConfig instance.
     * @see IfConfig.
     */
    IfConfig& ifconfig() { return (_ifconfig); }

    /**
     * Get the FirewallManager instance.
     *
     * @return a reference to the FirewallManager instance.
     * @see FirewallManager.
     */
    FirewallManager& firewall_manager() { return (_firewall_manager); }

    /**
     * Get the FibConfig instance.
     *
     * @return a reference to the FibConfig instance.
     * @see FibConfig.
     */
    FibConfig& fibconfig() { return (_fibconfig); }

    /**
     * Get the IoLinkManager instance.
     *
     * @return a reference to the IoLinkManager instance.
     * @see IoLinkManager.
     */
    IoLinkManager& io_link_manager() { return (_io_link_manager); }

    /**
     * Get the IoIpManager instance.
     *
     * @return a reference to the IoIpManager instance.
     * @see IoIpManager.
     */
    IoIpManager& io_ip_manager() { return (_io_ip_manager); }

    /**
     * Get the IoTcpUdpManager instance.
     *
     * @return a reference to the IoTcpUdpManager instance.
     * @see IoTcpUdpManager.
     */
    IoTcpUdpManager& io_tcpudp_manager() { return (_io_tcpudp_manager); }

    /**
     * Register @ref FeaDataPlaneManager data plane manager.
     *
     * @param fea_data_plane_manager the data plane manager to register.
     * @param is_exclusive if true, the manager is registered as the
     * exclusive manager, otherwise is added to the list of managers.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_data_plane_manager(FeaDataPlaneManager* fea_data_plane_manager,
				    bool is_exclusive);

    /**
     * Unregister @ref FeaDataPlaneManager data plane manager.
     *
     * @param fea_data_plane_manager the data plane manager to unregister.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_data_plane_manager(FeaDataPlaneManager* fea_data_plane_manager);

    /**
     * Get the FEA I/O instance.
     *
     * @return reference to the FEA I/O instance.
     */
    FeaIo& fea_io() { return (_fea_io); }

private:
    /**
     * Load the data plane managers.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int load_data_plane_managers(string& error_msg);

    /**
     * Unload the data plane managers.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unload_data_plane_managers(string& error_msg);

    EventLoop&	_eventloop;	// The event loop to use
    bool	_is_running;	// True if the service is running
    bool	_is_dummy;	// True if running in dummy node
    Profile	_profile;	// Profile entity
    NexthopPortMapper		_nexthop_port_mapper;	// Next-hop port mapper

    IfConfig			_ifconfig;
    FirewallManager		_firewall_manager;
    FibConfig			_fibconfig;

    IoLinkManager		_io_link_manager;
    IoIpManager			_io_ip_manager;
    IoTcpUdpManager		_io_tcpudp_manager;

    list<FeaDataPlaneManager*>	_fea_data_plane_managers;

    FeaIo&			_fea_io;	// The FeaIo entry to use
};

#endif // __FEA_FEA_NODE_HH__
