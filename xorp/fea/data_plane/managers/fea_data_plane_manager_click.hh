// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2011 XORP, Inc and Others
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


#ifndef __FEA_DATA_PLANE_MANAGERS_FEA_DATA_PLANE_MANAGER_CLICK_HH__
#define __FEA_DATA_PLANE_MANAGERS_FEA_DATA_PLANE_MANAGER_CLICK_HH__

#include <xorp_config.h>
#ifdef XORP_USE_CLICK

#include "fea/fea_data_plane_manager.hh"

class FibConfigEntryGetClick;
class FibConfigEntrySetClick;
class FibConfigForwardingDummy;
class FibConfigTableGetClick;
class FibConfigTableSetClick;
class FirewallGetDummy;
class FirewallSetDummy;
class IfConfigGetClick;
class IfConfigPropertyDummy;
class IfConfigSetClick;
class IPv4;


/**
 * FEA data plane manager class for Click.
 */
class FeaDataPlaneManagerClick : public FeaDataPlaneManager {
public:
    /**
     * Constructor.
     *
     * @param fea_node the @ref FeaNode this manager belongs to.
     */
    FeaDataPlaneManagerClick(FeaNode& fea_node);

    /**
     * Virtual destructor.
     */
    virtual ~FeaDataPlaneManagerClick();

    /**
     * Load the plugins.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int load_plugins(string& error_msg);

    /**
     * Unload the plugins.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unload_plugins(string& error_msg);

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

    /**
     * Allocate IoTcpUdp plugin instance.
     *
     * @param iftree the interface tree to use.
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param is_tcp if true allocate a TCP entry, otherwise UDP.
     * @return a new instance of @ref IoTcpUdp plugin on success,
     * otherwise NULL.
     */
    IoTcpUdp* allocate_io_tcpudp(const IfTree& iftree, int family,
				 bool is_tcp);

    //
    // Click-specific methods
    //

    /**
     * Enable/disable Click support.
     *
     * @param enable if true, then enable Click support, otherwise disable it.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int enable_click(bool enable, string& error_msg);

    /**
     * Enable/disable kernel-level Click support.
     *
     * @param enable if true, then enable the kernel-level Click support,
     * otherwise disable it.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int enable_kernel_click(bool enable, string& error_msg);

    /**
     * Specify the external program to generate the kernel-level Click
     * configuration.
     *
     * @param v the name of the external program to generate the kernel-level
     * Click configuration.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_kernel_click_config_generator_file(const string& v,
					       string& error_msg);

    /**
     * Enable/disable duplicating the Click routes to the system kernel.
     *
     * @param enable if true, then enable duplicating the Click routes to the
     * system kernel, otherwise disable it.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int enable_duplicate_routes_to_kernel(bool enable, string& error_msg);

    /**
     * Enable/disable installing kernel-level Click on startup.
     *
     * @param enable if true, then install kernel-level Click on startup.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int enable_kernel_click_install_on_startup(bool enable, string& error_msg);

    /**
     * Specify the list of kernel Click modules to load on startup if
     * installing kernel-level Click on startup is enabled.
     *
     * @param modules the list of kernel Click modules to load.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_kernel_click_modules(const list<string>& modules,
				 string& error_msg);

    /**
     * Specify the kernel-level Click mount directory.
     *
     * @param directory the kernel-level Click mount directory.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_kernel_click_mount_directory(const string& directory,
					 string& error_msg);

    /**
     * Enable/disable user-level Click support.
     *
     * @param enable if true, then enable the user-level Click support,
     * otherwise disable it.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int enable_user_click(bool enable, string& error_msg);

    /**
     * Specify the user-level Click command file.
     *
     * @param v the name of the user-level Click command file.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_user_click_command_file(const string& v, string& error_msg);

    /**
     * Specify the extra arguments to the user-level Click command.
     *
     * @param v the extra arguments to the user-level Click command.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_user_click_command_extra_arguments(const string& v,
					       string& error_msg);

    /**
     * Specify whether to execute on startup the user-level Click command.
     *
     * @param v if true, then execute the user-level Click command on startup.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_user_click_command_execute_on_startup(bool v, string& error_msg);

    /**
     * Specify the address to use for control access to the user-level
     * Click.
     *
     * @param v the address to use for control access to the user-level Click.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_user_click_control_address(const IPv4& v, string& error_msg);

    /**
     * Specify the socket port to use for control access to the user-level
     * Click.
     *
     * @param v the socket port to use for control access to the user-level
     * Click.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_user_click_control_socket_port(uint32_t v, string& error_msg);

    /**
     * Specify the configuration file to be used by user-level Click on
     * startup.
     *
     * @param v the name of the configuration file to be used by user-level
     * Click on startup.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_user_click_startup_config_file(const string& v, string& error_msg);

    /**
     * Specify the external program to generate the user-level Click
     * configuration.
     *
     * @param v the name of the external program to generate the user-level
     * Click configuration.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_user_click_config_generator_file(const string& v,
					     string& error_msg);

private:
    //
    // The plugins
    //
    //
    // TODO: XXX: For the time being some of the plugins
    // used by Click are dummy.
    //
    IfConfigPropertyDummy*	_ifconfig_property_dummy;
    IfConfigGetClick*		_ifconfig_get_click;
    IfConfigSetClick*		_ifconfig_set_click;
    FirewallGetDummy*		_firewall_get_dummy;
    FirewallSetDummy*		_firewall_set_dummy;
    FibConfigForwardingDummy*	_fibconfig_forwarding_dummy;
    FibConfigEntryGetClick*	_fibconfig_entry_get_click;
    FibConfigEntrySetClick*	_fibconfig_entry_set_click;
    FibConfigTableGetClick*	_fibconfig_table_get_click;
    FibConfigTableSetClick*	_fibconfig_table_set_click;
};

#endif // click
#endif // _FEA_DATA_PLANE_MANAGERS_FEA_DATA_PLANE_MANAGER_CLICK_HH__
