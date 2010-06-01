// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2007-2009 XORP, Inc.
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

// $XORP: xorp/fea/xrl_fea_target.hh,v 1.40 2008/10/10 01:23:53 pavlin Exp $


#ifndef __FEA_XRL_FEA_TARGET_HH__
#define __FEA_XRL_FEA_TARGET_HH__


//
// FEA (Forwarding Engine Abstraction) XRL target implementation.
//

#include "xrl/targets/fea_base.hh"

#include "xrl_fib_client_manager.hh"

class EventLoop;
#ifdef XORP_USE_CLICK
class FeaDataPlaneManagerClick;
#endif
class FeaNode;
class FibConfig;
class FirewallManager;
class IfConfig;
class IoLinkManager;
class IoIpManager;
class IoTcpUdpManager;
class LibFeaClientBridge;
class XrlFibClientManager;
#ifndef XORP_DISABLE_PROFILE
class Profile;
#endif

/**
 * @short FEA (Forwarding Engine Abstraction) XRL target class.
 */
class XrlFeaTarget : public XrlFeaTargetBase {
public:
    /**
     * Constructor.
     *
     * @param eventloop the event loop to use.
     */
    XrlFeaTarget(EventLoop&		eventloop,
		 FeaNode&		fea_node,
		 XrlRouter&		xrl_router,
#ifndef XORP_DISABLE_PROFILE
		 Profile&		profile,
#endif
		 XrlFibClientManager&	xrl_fib_client_manager,
		 LibFeaClientBridge&	lib_fea_client_bridge);

    /**
     * Destructor
     */
    virtual	~XrlFeaTarget();

    /**
     * Startup the service operation.
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int		startup();

    /**
     * Shutdown the service operation.
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
     * Test whether a shutdown XRL request has been received.
     *
     * @return true if shutdown XRL request has been received, otherwise false.
     */
    bool	is_shutdown_received() const { return (_is_shutdown_received); }

    /**
     * Get the event loop this service is added to.
     * 
     * @return the event loop this service is added to.
     */
    EventLoop&	eventloop() { return (_eventloop); }

    XrlCmdError common_0_1_get_target_name(
	// Output values,
	string&	name);

    XrlCmdError common_0_1_get_version(
	// Output values,
	string&	version);

    /**
     *  Get status from Xrl Target
     */
    XrlCmdError common_0_1_get_status(
	// Output values,
        uint32_t& status,
	string&	reason);

    /**
     * Shutdown FEA cleanly
     */
    XrlCmdError common_0_1_shutdown();

    XrlCmdError common_0_1_startup() { return XrlCmdError::OKAY(); }

    /** Does nothing, but allows us to have rtrmgr verify startup. */
    virtual XrlCmdError ifmgr_0_1_startup_ifmgr();

    /**
     *  Announce target birth to observer.
     *
     *  @param target_class the target class name.
     *
     *  @param target_instance the target instance name.
     */
    XrlCmdError finder_event_observer_0_1_xrl_target_birth(
	// Input values,
	const string&	target_class,
	const string&	target_instance);

    /**
     *  Announce target death to observer.
     *
     *  @param target_class the target class name.
     *
     *  @param target_instance the target instance name.
     */
    XrlCmdError finder_event_observer_0_1_xrl_target_death(
	// Input values,
	const string&	target_class,
	const string&	target_instance);

#ifdef XORP_USE_CLICK
    /**
     *  Load Click FEA support.
     */
    XrlCmdError fea_click_0_1_load_click();

    /**
     *  Unload Click FEA support.
     */
    XrlCmdError fea_click_0_1_unload_click();

    /**
     *  Enable/disable Click FEA support.
     *
     *  @param enable if true, then enable the Click FEA support, otherwise
     *  disable it.
     */
    XrlCmdError fea_click_0_1_enable_click(
	// Input values,
	const bool&	enable);

    /**
     *  Start Click FEA support.
     */
    XrlCmdError fea_click_0_1_start_click();

    /**
     *  Stop Click FEA support.
     */
    XrlCmdError fea_click_0_1_stop_click();

    /**
     *  Enable/disable duplicating the Click routes to the system kernel.
     *
     *  @param enable if true, then enable duplicating the Click routes to the
     *  system kernel, otherwise disable it.
     */
    XrlCmdError fea_click_0_1_enable_duplicate_routes_to_kernel(
	// Input values,
	const bool&	enable);

    /**
     *  Enable/disable kernel-level Click FEA support.
     *
     *  @param enable if true, then enable the kernel-level Click FEA support,
     *  otherwise disable it.
     */
    XrlCmdError fea_click_0_1_enable_kernel_click(
	// Input values,
	const bool&	enable);

    /**
     *  Enable/disable installing kernel-level Click on startup.
     *
     *  @param enable if true, then install kernel-level Click on startup.
     */
    XrlCmdError fea_click_0_1_enable_kernel_click_install_on_startup(
	// Input values,
	const bool&	enable);

    /**
     *  Specify the list of kernel Click modules to load on startup if
     *  installing kernel-level Click on startup is enabled. The file names of
     *  the kernel modules are separated by colon.
     *
     *  @param modules the list of kernel Click modules (separated by colon) to
     *  load.
     */
    XrlCmdError fea_click_0_1_set_kernel_click_modules(
	// Input values,
	const string&	modules);

    /**
     *  Specify the kernel-level Click mount directory.
     *
     *  @param directory the kernel-level Click mount directory.
     */
    XrlCmdError fea_click_0_1_set_kernel_click_mount_directory(
	// Input values,
	const string&	directory);

    /**
     *  Specify the external program to generate the kernel-level Click
     *  configuration.
     *
     *  @param kernel_click_config_generator_file the name of the external
     *  program to generate the kernel-level Click configuration.
     */
    XrlCmdError fea_click_0_1_set_kernel_click_config_generator_file(
	// Input values,
	const string&	kernel_click_config_generator_file);

    /**
     *  Enable/disable user-level Click FEA support.
     *
     *  @param enable if true, then enable the user-level Click FEA support,
     *  otherwise disable it.
     */
    XrlCmdError fea_click_0_1_enable_user_click(
	// Input values,
	const bool&	enable);

    /**
     *  Specify the user-level Click command file.
     *
     *  @param user_click_command_file the name of the user-level Click command
     *  file.
     */
    XrlCmdError fea_click_0_1_set_user_click_command_file(
	// Input values,
	const string&	user_click_command_file);

    /**
     *  Specify the extra arguments to the user-level Click command.
     *
     *  @param user_click_command_extra_arguments the extra arguments to the
     *  user-level Click command.
     */
    XrlCmdError fea_click_0_1_set_user_click_command_extra_arguments(
	// Input values,
	const string&	user_click_command_extra_arguments);

    /**
     *  Specify whether to execute on startup the user-level Click command.
     *
     *  @param user_click_command_execute_on_startup if true, then execute the
     *  user-level Click command on startup.
     */
    XrlCmdError fea_click_0_1_set_user_click_command_execute_on_startup(
	// Input values,
	const bool&	user_click_command_execute_on_startup);

    /**
     *  Specify the socket port to use for control access to the user-level
     *  Click.
     *
     *  @param user_click_control_socket_port the socket port to use for
     *  control access to the user-level Click.
     */
    XrlCmdError fea_click_0_1_set_user_click_control_socket_port(
	// Input values,
	const uint32_t&	user_click_control_socket_port);

    /**
     *  Specify the address to use for control access to the user-level
     *  Click.
     *
     *  @param user_click_control_address the address to use for
     *  control access to the user-level Click.
     */
    XrlCmdError fea_click_0_1_set_user_click_control_address(
	// Input values,
	const IPv4&	user_click_control_address);

    /**
     *  Specify the configuration file to be used by user-level Click on
     *  startup.
     *
     *  @param user_click_startup_config_file the name of the configuration
     *  file to be used by user-level Click on startup.
     */
    XrlCmdError fea_click_0_1_set_user_click_startup_config_file(
	// Input values,
	const string&	user_click_startup_config_file);

    /**
     *  Specify the external program to generate the user-level Click
     *  configuration.
     *
     *  @param user_click_config_generator_file the name of the external
     *  program to generate the user-level Click configuration.
     */
    XrlCmdError fea_click_0_1_set_user_click_config_generator_file(
	// Input values,
	const string&	user_click_config_generator_file);

#endif // click

    /**
     *  Add a FIB client.
     *
     *  @param client_target_name the target name of the FIB client to add.
     *  @param send_updates whether updates should be sent.
     *  @param send_resolves whether resolve requests should be sent.
     */
    XrlCmdError fea_fib_0_1_add_fib_client4(
	// Input values,
	const string&	client_target_name,
	const bool&	send_updates,
	const bool&	send_resolves);

    /**
     *  Delete a FIB client.
     *
     *  @param target_name the target name of the FIB client to delete.
     */
    XrlCmdError fea_fib_0_1_delete_fib_client4(
	// Input values,
	const string&	client_target_name);


#ifdef HAVE_IPV6
    XrlCmdError fea_fib_0_1_add_fib_client6(
	// Input values,
	const string&	client_target_name,
	const bool&	send_updates,
	const bool&	send_resolves);

    XrlCmdError fea_fib_0_1_delete_fib_client6(
	// Input values,
	const string&	client_target_name);
#endif

#ifndef XORP_DISABLE_FIREWALL
    //
    // FEA firewall interface
    //

    /** Does nothing, but allows us to have rtrmgr verify startup. */
    virtual XrlCmdError fea_firewall_0_1_startup_firewall();


    /**
     *  Start firewall configuration transaction.
     *
     *  @param tid the transaction ID returned by this operation.
     */
    XrlCmdError fea_firewall_0_1_start_transaction(
	// Output values,
	uint32_t&	tid);

    /**
     *  Commit firewall configuration transaction.
     *
     *  @param tid the transaction ID for this operation.
     */
    XrlCmdError fea_firewall_0_1_commit_transaction(
	// Input values,
	const uint32_t&	tid);

    /**
     *  Abort firewall configuration transaction.
     *
     *  @param tid the transaction ID for this operation.
     */
    XrlCmdError fea_firewall_0_1_abort_transaction(
	// Input values,
	const uint32_t&	tid);

    /**
     *  Add an IPv4 firewall entry.
     *
     *  @param tid the transaction ID for this operation.
     *
     *  @param rule_number the rule number for this entry.
     *
     *  @param ifname the name of the interface where this filter is to be
     *  applied.
     *
     *  @param vifname the name of the vif where this filter is to be applied.
     *
     *  @param src_network the source IPv4 network address prefix.
     *
     *  @param dst_network the destination IPv4 network address prefix.
     *
     *  @param ip_protocol the IP protocol number (1-255, or 0 if wildcard).
     *
     *  @param src_port_begin the source TCP/UDP begin port (0-65535).
     *
     *  @param src_port_end the source TCP/UDP end port (0-65535).
     *
     *  @param dst_port_begin the destination TCP/UDP begin port (0-65535).
     *
     *  @param dst_port_end the destination TCP/UDP end port (0-65535).
     *
     *  @param action the action to be taken when this filter is matched. It is
     *  one of the following keywords: "none", "pass", "drop", "reject".
     */
    XrlCmdError fea_firewall_0_1_add_entry4(
	// Input values,
	const uint32_t&	tid,
	const uint32_t&	rule_number,
	const string&	ifname,
	const string&	vifname,
	const IPv4Net&	src_network,
	const IPv4Net&	dst_network,
	const uint32_t&	ip_protocol,
	const uint32_t&	src_port_begin,
	const uint32_t&	src_port_end,
	const uint32_t&	dst_port_begin,
	const uint32_t&	dst_port_end,
	const string&	action);

    /**
     *  Replace an IPv4 firewall entry.
     *
     *  @param tid the transaction ID for this operation.
     *
     *  @param rule_number the rule number for this entry.
     *
     *  @param ifname the name of the interface where this filter is to be
     *  applied.
     *
     *  @param vifname the name of the vif where this filter is to be applied.
     *
     *  @param src_network the source IPv4 network address prefix.
     *
     *  @param dst_network the destination IPv4 network address prefix.
     *
     *  @param ip_protocol the IP protocol number (1-255, or 0 if wildcard).
     *
     *  @param src_port_begin the source TCP/UDP begin port (0-65535).
     *
     *  @param src_port_end the source TCP/UDP end port (0-65535).
     *
     *  @param dst_port_begin the destination TCP/UDP begin port (0-65535).
     *
     *  @param dst_port_end the destination TCP/UDP end port (0-65535).
     *
     *  @param action the action to be taken when this filter is matched. It is
     *  one of the following keywords: "none", "pass", "drop", "reject".
     */
    XrlCmdError fea_firewall_0_1_replace_entry4(
	// Input values,
	const uint32_t&	tid,
	const uint32_t&	rule_number,
	const string&	ifname,
	const string&	vifname,
	const IPv4Net&	src_network,
	const IPv4Net&	dst_network,
	const uint32_t&	ip_protocol,
	const uint32_t&	src_port_begin,
	const uint32_t&	src_port_end,
	const uint32_t&	dst_port_begin,
	const uint32_t&	dst_port_end,
	const string&	action);

    /**
     *  Delete an IPv4 firewall entry.
     *
     *  @param tid the transaction ID for this operation.
     *
     *  @param rule_number the rule number for this entry.
     *
     *  @param ifname the name of the interface where this filter is to be
     *  deleted.
     *
     *  @param vifname the name of the vif where this filter is to be deleted.
     *
     *  @param src_network the source IPv4 network address prefix.
     *
     *  @param dst_network the destination IPv4 network address prefix.
     *
     *  @param ip_protocol the IP protocol number (1-255, or 0 if wildcard).
     *
     *  @param src_port_begin the source TCP/UDP begin port (0-65535).
     *
     *  @param src_port_end the source TCP/UDP end port (0-65535).
     *
     *  @param dst_port_begin the destination TCP/UDP begin port (0-65535).
     *
     *  @param dst_port_end the destination TCP/UDP end port (0-65535).
     */
    XrlCmdError fea_firewall_0_1_delete_entry4(
	// Input values,
	const uint32_t&	tid,
	const uint32_t&	rule_number,
	const string&	ifname,
	const string&	vifname,
	const IPv4Net&	src_network,
	const IPv4Net&	dst_network,
	const uint32_t&	ip_protocol,
	const uint32_t&	src_port_begin,
	const uint32_t&	src_port_end,
	const uint32_t&	dst_port_begin,
	const uint32_t&	dst_port_end);

    /**
     *  Delete all IPv4 firewall entries.
     *
     *  @param tid the transaction ID for this operation.
     */
    XrlCmdError fea_firewall_0_1_delete_all_entries4(
	// Input values,
	const uint32_t&	tid);

    /**
     *  Get a token for a list of IPv4 firewall entries.
     *
     *  @param token to be provided when calling get_entry_list_next4.
     *
     *  @param more true if the list is not empty.
     */
    XrlCmdError fea_firewall_0_1_get_entry_list_start4(
	// Output values,
	uint32_t&	token,
	bool&		more);

    /**
     *  Get the next item in a list of IPv4 firewall entries.
     *
     *  @param token returned by a previous call to get_entry_list_start4.
     *
     *  @param rule_number the rule number for this entry.
     *
     *  @param ifname the name of the interface where this filter exists.
     *
     *  @param vifname the name of the vif where this filter exists.
     *
     *  @param src_network the source IPv4 network address prefix.
     *
     *  @param dst_network the destination IPv4 network address prefix.
     *
     *  @param ip_protocol the IP protocol number (1-255, or 0 if wildcard).
     *
     *  @param src_port_begin the source TCP/UDP begin port (0-65535).
     *
     *  @param src_port_end the source TCP/UDP end port (0-65535).
     *
     *  @param dst_port_begin the destination TCP/UDP begin port (0-65535).
     *
     *  @param dst_port_end the destination TCP/UDP end port (0-65535).
     *
     *  @param action the action taken when this filter is matched. It is one
     *  of the following keywords: "none", "pass", "drop", "reject".
     *
     *  @param more true if the list has more items remaining.
     */
    XrlCmdError fea_firewall_0_1_get_entry_list_next4(
	// Input values,
	const uint32_t&	token,
	// Output values,
	uint32_t&	rule_number,
	string&		ifname,
	string&		vifname,
	IPv4Net&	src_network,
	IPv4Net&	dst_network,
	uint32_t&	ip_protocol,
	uint32_t&	src_port_begin,
	uint32_t&	src_port_end,
	uint32_t&	dst_port_begin,
	uint32_t&	dst_port_end,
	string&		action,
	bool&		more);

#ifdef HAVE_IPV6

    /**
     *  Add an IPv6 firewall entry.
     *
     *  @param tid the transaction ID for this operation.
     *
     *  @param rule_number the rule number for this entry.
     *
     *  @param ifname the name of the interface where this filter is to be
     *  applied.
     *
     *  @param vifname the name of the vif where this filter is to be applied.
     *
     *  @param src_network the source IPv6 network address prefix.
     *
     *  @param dst_network the destination IPv6 network address prefix.
     *
     *  @param ip_protocol the IP protocol number (1-255, or 0 if wildcard).
     *
     *  @param src_port_begin the source TCP/UDP begin port (0-65535).
     *
     *  @param src_port_end the source TCP/UDP end port (0-65535).
     *
     *  @param dst_port_begin the destination TCP/UDP begin port (0-65535).
     *
     *  @param dst_port_end the destination TCP/UDP end port (0-65535).
     *
     *  @param action the action to be taken when this filter is matched. It is
     *  one of the following keywords: "none", "pass", "drop", "reject".
     */
    XrlCmdError fea_firewall_0_1_add_entry6(
	// Input values,
	const uint32_t&	tid,
	const uint32_t&	rule_number,
	const string&	ifname,
	const string&	vifname,
	const IPv6Net&	src_network,
	const IPv6Net&	dst_network,
	const uint32_t&	ip_protocol,
	const uint32_t&	src_port_begin,
	const uint32_t&	src_port_end,
	const uint32_t&	dst_port_begin,
	const uint32_t&	dst_port_end,
	const string&	action);

    /**
     *  Replace an IPv6 firewall entry.
     *
     *  @param tid the transaction ID for this operation.
     *
     *  @param rule_number the rule number for this entry.
     *
     *  @param ifname the name of the interface where this filter is to be
     *  applied.
     *
     *  @param vifname the name of the vif where this filter is to be applied.
     *
     *  @param src_network the source IPv6 network address prefix.
     *
     *  @param dst_network the destination IPv6 network address prefix.
     *
     *  @param ip_protocol the IP protocol number (1-255, or 0 if wildcard).
     *
     *  @param src_port_begin the source TCP/UDP begin port (0-65535).
     *
     *  @param src_port_end the source TCP/UDP end port (0-65535).
     *
     *  @param dst_port_begin the destination TCP/UDP begin port (0-65535).
     *
     *  @param dst_port_end the destination TCP/UDP end port (0-65535).
     *
     *  @param action the action to be taken when this filter is matched. It is
     *  one of the following keywords: "none", "pass", "drop", "reject".
     */
    XrlCmdError fea_firewall_0_1_replace_entry6(
	// Input values,
	const uint32_t&	tid,
	const uint32_t&	rule_number,
	const string&	ifname,
	const string&	vifname,
	const IPv6Net&	src_network,
	const IPv6Net&	dst_network,
	const uint32_t&	ip_protocol,
	const uint32_t&	src_port_begin,
	const uint32_t&	src_port_end,
	const uint32_t&	dst_port_begin,
	const uint32_t&	dst_port_end,
	const string&	action);

    /**
     *  Delete an IPv6 firewall entry.
     *
     *  @param tid the transaction ID for this operation.
     *
     *  @param rule_number the rule number for this entry.
     *
     *  @param ifname the name of the interface where this filter is to be
     *  deleted.
     *
     *  @param vifname the name of the vif where this filter is to be deleted.
     *
     *  @param src_network the source IPv6 network address prefix.
     *
     *  @param dst_network the destination IPv6 network address prefix.
     *
     *  @param ip_protocol the IP protocol number (1-255, or 0 if wildcard).
     *
     *  @param src_port_begin the source TCP/UDP begin port (0-65535).
     *
     *  @param src_port_end the source TCP/UDP end port (0-65535).
     *
     *  @param dst_port_begin the destination TCP/UDP begin port (0-65535).
     *
     *  @param dst_port_end the destination TCP/UDP end port (0-65535).
     */
    XrlCmdError fea_firewall_0_1_delete_entry6(
	// Input values,
	const uint32_t&	tid,
	const uint32_t&	rule_number,
	const string&	ifname,
	const string&	vifname,
	const IPv6Net&	src_network,
	const IPv6Net&	dst_network,
	const uint32_t&	ip_protocol,
	const uint32_t&	src_port_begin,
	const uint32_t&	src_port_end,
	const uint32_t&	dst_port_begin,
	const uint32_t&	dst_port_end);

    /**
     *  Delete all IPv6 firewall entries.
     *
     *  @param tid the transaction ID for this operation.
     */
    XrlCmdError fea_firewall_0_1_delete_all_entries6(
	// Input values,
	const uint32_t&	tid);

    /**
     *  Get a token for a list of IPv6 firewall entries.
     *
     *  @param token to be provided when calling get_entry_list_next6.
     *
     *  @param more true if the list is not empty.
     */
    XrlCmdError fea_firewall_0_1_get_entry_list_start6(
	// Output values,
	uint32_t&	token,
	bool&		more);

    /**
     *  Get the next item in a list of IPv6 firewall entries.
     *
     *  @param token returned by a previous call to get_entry_list_start6.
     *
     *  @param rule_number the rule number for this entry.
     *
     *  @param ifname the name of the interface where this filter exists.
     *
     *  @param vifname the name of the vif where this filter exists.
     *
     *  @param src_network the source IPv6 network address prefix.
     *
     *  @param dst_network the destination IPv6 network address prefix.
     *
     *  @param ip_protocol the IP protocol number (1-255, or 0 if wildcard).
     *
     *  @param src_port_begin the source TCP/UDP begin port (0-65535).
     *
     *  @param src_port_end the source TCP/UDP end port (0-65535).
     *
     *  @param dst_port_begin the destination TCP/UDP begin port (0-65535).
     *
     *  @param dst_port_end the destination TCP/UDP end port (0-65535).
     *
     *  @param action the action taken when this filter is matched. It is one
     *  of the following keywords: "none", "pass", "drop", "reject".
     *
     *  @param more true if the list has more items remaining.
     */
    XrlCmdError fea_firewall_0_1_get_entry_list_next6(
	// Input values,
	const uint32_t&	token,
	// Output values,
	uint32_t&	rule_number,
	string&		ifname,
	string&		vifname,
	IPv6Net&	src_network,
	IPv6Net&	dst_network,
	uint32_t&	ip_protocol,
	uint32_t&	src_port_begin,
	uint32_t&	src_port_end,
	uint32_t&	dst_port_begin,
	uint32_t&	dst_port_end,
	string&		action,
	bool&		more);
#endif //ipv6
#endif //firewall


    //
    // FEA network interface management interface
    //

    XrlCmdError ifmgr_0_1_set_restore_original_config_on_shutdown(
	// Input values,
	const bool&	enable);

    XrlCmdError ifmgr_0_1_get_configured_interface_names(
	// Output values,
	XrlAtomList&	ifnames);

    XrlCmdError ifmgr_0_1_get_configured_vif_names(
	const string&	ifname,
	// Output values,
	XrlAtomList&		vifs);

    XrlCmdError ifmgr_0_1_get_configured_vif_flags(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	bool&	enabled,
	bool&	broadcast,
	bool&	loopback,
	bool&	point_to_point,
	bool&	multicast);

    XrlCmdError ifmgr_0_1_get_configured_vif_pif_index(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	uint32_t&	pif_index);

    XrlCmdError ifmgr_0_1_start_transaction(
	// Output values,
	uint32_t&	tid);

    XrlCmdError ifmgr_0_1_commit_transaction(
	// Input values,
	const uint32_t&	tid);

    XrlCmdError ifmgr_0_1_abort_transaction(
	// Input values,
	const uint32_t&	tid);

    XrlCmdError ifmgr_0_1_create_interface(
	// Input values,
	const uint32_t& tid,
	const string&	ifname);

    XrlCmdError ifmgr_0_1_delete_interface(
	// Input values,
	const uint32_t& tid,
	const string&	ifname);

    /**
     *  Implicitly configure all interfaces within the FEA by using information
     *  from the underlying system.
     *
     *  @param tid the transaction ID.
     *  @param enable if true, then enable the implicit configuration,
     *  otherwise disable it.
     */
    XrlCmdError ifmgr_0_1_configure_all_interfaces_from_system(
	// Input values,
	const uint32_t&	tid,
	const bool&	enable);

    /**
     *  Implicitly configure an interface within the FEA by using information
     *  from the underlying system.
     *
     *  @param tid the transaction ID.
     *  @param ifname the name of the interface to configure.
     *  @param enable if true, then enable the implicit configuration,
     *  otherwise disable it.
     */
    XrlCmdError ifmgr_0_1_configure_interface_from_system(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const bool&	enable);

    XrlCmdError ifmgr_0_1_set_interface_enabled(
	// Input values,
	const uint32_t& tid,
	const string&	ifname,
	const bool&	enabled);

    XrlCmdError ifmgr_0_1_get_configured_interface_enabled(
	// Input values,
	const string&	ifname,
	// Output values,
	bool&		enabled);

    XrlCmdError ifmgr_0_1_set_interface_discard(
	// Input values,
	const uint32_t& tid,
	const string&	ifname,
	const bool&	discard);

    XrlCmdError ifmgr_0_1_get_configured_interface_discard(
	// Input values,
	const string&	ifname,
	// Output values,
	bool&		discard);

    XrlCmdError ifmgr_0_1_set_interface_unreachable(
	// Input values,
	const uint32_t& tid,
	const string&	ifname,
	const bool&	unreachable);

    XrlCmdError ifmgr_0_1_get_configured_interface_unreachable(
	// Input values,
	const string&	ifname,
	// Output values,
	bool&		unreachable);

    XrlCmdError ifmgr_0_1_set_interface_management(
	// Input values,
	const uint32_t& tid,
	const string&	ifname,
	const bool&	management);

    XrlCmdError ifmgr_0_1_get_configured_interface_management(
	// Input values,
	const string&	ifname,
	// Output values,
	bool&		management);

    XrlCmdError ifmgr_0_1_set_mac(
	// Input values,
	const uint32_t& tid,
	const string&	ifname,
	const Mac&	mac);

    XrlCmdError ifmgr_0_1_create_mac(
        // Input values,
        const string&   ifname,
        const Mac&      mac);

    
    XrlCmdError ifmgr_0_1_create_address_atomic(
	// Input values,
	const string&   ifname,
	const string&   vifname,
	const IPv4& ip,
	const uint32_t& prefix_length);

    XrlCmdError ifmgr_0_1_delete_address_atomic(
	// Input values,
	const string&   ifname,
	const string&   vifname,
	const IPv4& ip);

    XrlCmdError ifmgr_0_1_delete_mac(
        // Input values,
        const string&   ifname,
        const Mac&      mac);

    XrlCmdError ifmgr_0_1_restore_original_mac(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname);

    XrlCmdError ifmgr_0_1_get_configured_mac(
	// Input values,
	const string&	ifname,
	// Output values,
	Mac&	mac);

    XrlCmdError ifmgr_0_1_set_mtu(
	// Input values,
	const uint32_t& tid,
	const string&	ifname,
	const uint32_t&	mtu);

    XrlCmdError ifmgr_0_1_restore_original_mtu(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname);

    XrlCmdError ifmgr_0_1_get_configured_mtu(
	// Input values,
	const string&	ifname,
	// Output values,
	uint32_t&	mtu);

    XrlCmdError ifmgr_0_1_get_configured_no_carrier(
	// Input values,
	const string&	ifname,
	// Output values,
	bool&		no_carrier);

    XrlCmdError ifmgr_0_1_get_configured_baudrate(
	// Input values,
	const string&	ifname,
	// Output values,
	uint64_t&	baudrate);

    XrlCmdError ifmgr_0_1_get_configured_address_flags4(
	// Input values,
	const string& ifname,
	const string& vif,
	const IPv4&   address,
	// Output values,
	bool& up,
	bool& broadcast,
	bool& loopback,
	bool& point_to_point,
	bool& multicast);

    XrlCmdError ifmgr_0_1_create_vif(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif);

    XrlCmdError ifmgr_0_1_delete_vif(
	// Input values,
	const uint32_t& tid,
	const string& 	ifname,
	const string&	vif);

    XrlCmdError ifmgr_0_1_set_vif_enabled(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const bool&	enabled);

    XrlCmdError ifmgr_0_1_get_configured_vif_enabled(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	bool&		enabled);

    XrlCmdError ifmgr_0_1_get_configured_vif_addresses4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	XrlAtomList&	addresses);

    /**
     *  Set VLAN vif.
     *
     *  @param ifname the name of the physical interface associated with the
     *  VLAN.
     *  @param vif the name of the VLAN.
     *  @param vlan_id the VLAN ID. It must be in the range 0 through 4095.
     */
    XrlCmdError ifmgr_0_1_set_vif_vlan(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const uint32_t&	vlan_id);

    XrlCmdError ifmgr_0_1_create_address4(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address);

    XrlCmdError ifmgr_0_1_delete_address4(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address);

    XrlCmdError ifmgr_0_1_set_address4(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address);

    XrlCmdError ifmgr_0_1_set_address_enabled4(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	const bool&	enabled);

    XrlCmdError ifmgr_0_1_get_configured_address_enabled4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	bool&		enabled);

    XrlCmdError ifmgr_0_1_set_prefix4(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	const uint32_t&	prefix_len);

    XrlCmdError ifmgr_0_1_get_configured_prefix4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	// Output values,
	uint32_t&	prefix_len);

    XrlCmdError ifmgr_0_1_set_broadcast4(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	const IPv4&	broadcast);

    XrlCmdError ifmgr_0_1_get_configured_broadcast4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	// Output values,
	IPv4&		broadcast);

    XrlCmdError ifmgr_0_1_set_endpoint4(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	const IPv4&	endpoint);

    XrlCmdError ifmgr_0_1_get_configured_endpoint4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	// Output values,
	IPv4&	endpoint);

#ifdef HAVE_IPV6
    XrlCmdError ifmgr_0_1_get_configured_address_flags6(
	// Input values,
	const string& ifname,
	const string& vif,
	const IPv6&   address,
	// Output values,
	bool& up,
	bool& loopback,
	bool& point_to_point,
	bool& multicast);

    XrlCmdError ifmgr_0_1_get_configured_vif_addresses6(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	XrlAtomList&	addresses);

    XrlCmdError ifmgr_0_1_create_address6(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address);

    XrlCmdError ifmgr_0_1_delete_address6(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address);

    XrlCmdError ifmgr_0_1_set_address_enabled6(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address,
	const bool&	enabled);

    XrlCmdError ifmgr_0_1_get_configured_address_enabled6(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address,
	bool&		enabled);

    XrlCmdError ifmgr_0_1_set_prefix6(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address,
	const uint32_t&	prefix_len);

    XrlCmdError ifmgr_0_1_get_configured_prefix6(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address,
	// Output values,
	uint32_t&	prefix_len);

    XrlCmdError ifmgr_0_1_set_endpoint6(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address,
	const IPv6&	endpoint);

    XrlCmdError ifmgr_0_1_get_configured_endpoint6(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address,
	// Output values,
	IPv6&		endpoint);
#endif

    XrlCmdError ifmgr_replicator_0_1_register_ifmgr_mirror(
	// Input values,
	const string&	clientname);

    XrlCmdError ifmgr_replicator_0_1_unregister_ifmgr_mirror(
	// Input values,
	const string&	clientname);

    //
    // Forwarding Table Interface
    //

    XrlCmdError fti_0_2_lookup_route_by_dest4(
	// Input values,
	const IPv4&	host,
	// Output values,
	IPv4Net&	netmask,
	IPv4&		nexthop,
	string&		ifname,
	string&		vifname,
	uint32_t&	metric,
	uint32_t&	admin_distance,
	string&		protocol_origin);

    XrlCmdError fti_0_2_lookup_route_by_network4(
	// Input values,
	const IPv4Net&	dst,
	// Output values,
	IPv4&		nexthop,
	string&		ifname,
	string&		vifname,
	uint32_t&	metric,
	uint32_t&	admin_distance,
	string&		protocol_origin);

    XrlCmdError fti_0_2_have_ipv4(
	// Output values, 
	bool&	result);

    XrlCmdError fti_0_2_get_unicast_forwarding_enabled4(
	// Output values,
	bool&	enabled);

    XrlCmdError fti_0_2_set_unicast_forwarding_enabled4(
	// Input values,
	const bool&	enabled);

    /**
     *  Set the IPv4 unicast forwarding engine whether to retain existing XORP
     *  forwarding entries on startup.
     *
     *  @param retain if true, then retain the XORP forwarding entries,
     *  otherwise delete them.
     */
    XrlCmdError fti_0_2_set_unicast_forwarding_entries_retain_on_startup4(
	// Input values,
	const bool&	retain);

    /**
     *  Set the IPv4 unicast forwarding engine whether to retain existing XORP
     *  forwarding entries on shutdown.
     *
     *  @param retain if true, then retain the XORP forwarding entries,
     *  otherwise delete them.
     */
    XrlCmdError fti_0_2_set_unicast_forwarding_entries_retain_on_shutdown4(
	// Input values,
	const bool&	retain);

    /**
     *  Set the IPv4 unicast forwarding table ID to be used.
     *
     *  @param is_configured if true, the forwarding table ID is configured,
     *  otherwise the default table should be used.
     *
     *  @param table_id the IPv4 unicast forwarding table ID to be used.
     */
    XrlCmdError fti_0_2_set_unicast_forwarding_table_id4(
	// Input values,
	const bool&	is_configured,
	const uint32_t&	table_id);

#ifdef HAVE_IPV6
    XrlCmdError fti_0_2_lookup_route_by_dest6(
	// Input values,
	const IPv6&	host,
	// Output values,
	IPv6Net&	netmask,
	IPv6&		nexthop,
	string&		ifname,
	string&		vifname,
	uint32_t&	metric,
	uint32_t&	admin_distance,
	string&		protocol_origin);

    XrlCmdError fti_0_2_lookup_route_by_network6(
	// Input values,
	const IPv6Net&	dst,
	// Output values,
	IPv6&		nexthop,
	string&		ifname,
	string&		vifname,
	uint32_t&	metric,
	uint32_t&	admin_distance,
	string&		protocol_origin);


    XrlCmdError fti_0_2_have_ipv6(
	// Output values, 
	bool&	result);

    XrlCmdError fti_0_2_get_unicast_forwarding_enabled6(
	// Output values,
	bool&	enabled);

    XrlCmdError fti_0_2_set_unicast_forwarding_enabled6(
	// Input values,
	const bool&	enabled);

    /**
     *  Set the IPv6 unicast forwarding engine whether to retain existing XORP
     *  forwarding entries on startup.
     *
     *  @param retain if true, then retain the XORP forwarding entries,
     *  otherwise delete them.
     */
    XrlCmdError fti_0_2_set_unicast_forwarding_entries_retain_on_startup6(
	// Input values,
	const bool&	retain);

    /**
     *  Set the IPv6 unicast forwarding engine whether to retain existing XORP
     *  forwarding entries on shutdown.
     *
     *  @param retain if true, then retain the XORP forwarding entries,
     *  otherwise delete them.
     */
    XrlCmdError fti_0_2_set_unicast_forwarding_entries_retain_on_shutdown6(
	// Input values,
	const bool&	retain);

    /**
     *  Set the IPv6 unicast forwarding table ID to be used.
     *
     *  @param is_configured if true, the forwarding table ID is configured,
     *  otherwise the default table should be used.
     *
     *  @param table_id the IPv6 unicast forwarding table ID to be used.
     */
    XrlCmdError fti_0_2_set_unicast_forwarding_table_id6(
	// Input values,
	const bool&	is_configured,
	const uint32_t&	table_id);
#endif

    //
    // RIB routes redistribution transaction-based XRL interface
    //

    /**
     *  Start transaction.
     *
     *  @param tid the transaction ID to use for this transaction.
     */
    XrlCmdError redist_transaction4_0_1_start_transaction(
	// Output values,
	uint32_t&	tid);

    /**
     *  Commit transaction.
     *
     *  @param tid the transaction ID of this transaction.
     */
    XrlCmdError redist_transaction4_0_1_commit_transaction(
	// Input values,
	const uint32_t&	tid);

    /**
     *  Abort transaction.
     *
     *  @param tid the transaction ID of this transaction.
     */
    XrlCmdError redist_transaction4_0_1_abort_transaction(
	// Input values,
	const uint32_t&	tid);

    /**
     *  Add/delete a routing entry.
     *
     *  @param tid the transaction ID of this transaction.
     *
     *  @param dst destination network.
     *
     *  @param nexthop nexthop router address.
     *
     *  @param ifname interface name associated with nexthop.
     *
     *  @param vifname virtual interface name with nexthop.
     *
     *  @param metric origin routing protocol metric for route.
     *
     *  @param admin_distance administrative distance of origin routing
     *  protocol.
     *
     *  @param cookie value set by the requestor to identify redistribution
     *  source. Typical value is the originating protocol name.
     *
     *  @param protocol_origin the name of the protocol that originated this
     *  routing entry.
     */
    XrlCmdError redist_transaction4_0_1_add_route(
	// Input values,
	const uint32_t&	tid,
	const IPv4Net&	dst,
	const IPv4&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	cookie,
	const string&	protocol_origin);

    XrlCmdError redist_transaction4_0_1_delete_route(
	// Input values,
	const uint32_t&	tid,
	const IPv4Net&	dst,
	const IPv4&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	cookie,
	const string&	protocol_origin);

    /**
     *  Delete all routing entries.
     *
     *  @param tid the transaction ID of this transaction.
     *
     *  @param cookie value set by the requestor to identify redistribution
     *  source. Typical value is the originating protocol name.
     */
    XrlCmdError redist_transaction4_0_1_delete_all_routes(
	// Input values,
	const uint32_t&	tid,
	const string&	cookie);


#ifdef HAVE_IPV6
    /**
     *  Start transaction.
     *
     *  @param tid the transaction ID to use for this transaction.
     */
    XrlCmdError redist_transaction6_0_1_start_transaction(
	// Output values,
	uint32_t&	tid);

    /**
     *  Commit transaction.
     *
     *  @param tid the transaction ID of this transaction.
     */
    XrlCmdError redist_transaction6_0_1_commit_transaction(
	// Input values,
	const uint32_t&	tid);

    /**
     *  Abort transaction.
     *
     *  @param tid the transaction ID of this transaction.
     */
    XrlCmdError redist_transaction6_0_1_abort_transaction(
	// Input values,
	const uint32_t&	tid);

    /**
     *  Add/delete a routing entry.
     *
     *  @param tid the transaction ID of this transaction.
     *
     *  @param dst destination network.
     *
     *  @param nexthop nexthop router address.
     *
     *  @param ifname interface name associated with nexthop.
     *
     *  @param vifname virtual interface name with nexthop.
     *
     *  @param metric origin routing protocol metric for route.
     *
     *  @param admin_distance administrative distance of origin routing
     *  protocol.
     *
     *  @param cookie value set by the requestor to identify redistribution
     *  source. Typical value is the originating protocol name.
     *
     *  @param protocol_origin the name of the protocol that originated this
     *  routing entry.
     */
    XrlCmdError redist_transaction6_0_1_add_route(
	// Input values,
	const uint32_t&	tid,
	const IPv6Net&	dst,
	const IPv6&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	cookie,
	const string&	protocol_origin);

    XrlCmdError redist_transaction6_0_1_delete_route(
	// Input values,
	const uint32_t&	tid,
	const IPv6Net&	dst,
	const IPv6&	nexthop,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	metric,
	const uint32_t&	admin_distance,
	const string&	cookie,
	const string&	protocol_origin);

    /**
     *  Delete all routing entries.
     *
     *  @param tid the transaction ID of this transaction.
     *
     *  @param cookie value set by the requestor to identify redistribution
     *  source. Typical value is the originating protocol name.
     */
    XrlCmdError redist_transaction6_0_1_delete_all_routes(
	// Input values,
	const uint32_t&	tid,
	const string&	cookie);

#endif //ipv6


    //
    // Raw Link-Level Server Interface
    //

    /**
     *  Send a raw link-level packet on an interface.
     *
     *  @param if_name the interface to send the packet on.
     *
     *  @param vif_name the vif to send the packet on.
     *
     *  @param src_address the MAC source address.
     *
     *  @param dst_address the MAC destination address.
     *
     *  @param ether_type the EtherType protocol type or the Destination SAP.
     *  It must be between 1536 and 65535 to specify the EtherType, or between
     *  1 and 255 to specify the Destination SAP IEEE 802.2 LLC frames.
     *
     *  @param payload the payload, everything after the MAC header.
     */
    XrlCmdError raw_link_0_1_send(
	// Input values,
	const string&	if_name,
	const string&	vif_name,
	const Mac&	src_address,
	const Mac&	dst_address,
	const uint32_t&	ether_type,
	const vector<uint8_t>&	payload);

    /**
     *  Register to receive raw link-level packets. The receiver is expected to
     *  support raw_link_client/0.1 interface.
     *
     *  @param xrl_target_instance_name the receiver's XRL target instance
     *  name.
     *
     *  @param if_name the interface through which packets should be accepted.
     *
     *  @param vif_name the vif through which packets should be accepted.
     *
     *  @param ether_type the EtherType protocol number or the Destination SAP
     *  that the receiver is interested in. It must be between 1536 and 65535
     *  to specify the EtherType, or between 1 and 255 to specify the
     *  Destination SAP for IEEE 802.2 LLC frames. A protocol number of 0 is
     *  used to specify all protocols.
     *
     *  @param filter_program the filter program to be applied on the received
     *  packets. The program uses tcpdump(1) style expression.
     *
     *  @param enable_multicast_loopback if true then enable delivering of
     *  multicast datagrams back to this host (assuming the host is a member of
     *  the same multicast group).
     */
    XrlCmdError raw_link_0_1_register_receiver(
	// Input values,
	const string&	xrl_target_instance_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ether_type,
	const string&	filter_program,
	const bool&	enable_multicast_loopback);

    /**
     *  Unregister to receive raw link-level packets.
     *
     *  @param xrl_target_instance_name the receiver's XRL target instance
     *  name.
     *
     *  @param if_name the interface through which packets should not be
     *  accepted.
     *
     *  @param vif_name the vif through which packets should not be accepted.
     *
     *  @param ether_type the EtherType protocol number or the Destination SAP
     *  that the receiver is not interested in anymore. It must be between 1536
     *  and 65535 to specify the EtherType, or between 1 and 255 to specify the
     *  Destination SAP for IEEE 802.2 LLC frames. A protocol number of 0 is
     *  used to specify all protocols.
     *
     *  @param filter_program the filter program that was applied on the
     *  received packets. The program uses tcpdump(1) style expression.
     */
    XrlCmdError raw_link_0_1_unregister_receiver(
	// Input values,
	const string&	xrl_target_instance_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ether_type,
	const string&	filter_program);

    /**
     *  Join a MAC multicast group.
     *
     *  @param xrl_target_instance_name the receiver's XRL target instance
     *  name.
     *
     *  @param if_name the interface through which packets should be accepted.
     *
     *  @param vif_name the vif through which packets should be accepted.
     *
     *  @param ether_type the EtherType protocol number or the Destination SAP
     *  that the receiver is interested in. It must be between 1536 and 65535
     *  to specify the EtherType, or between 1 and 255 to specify the
     *  Destination SAP for IEEE 802.2 LLC frames. A protocol number of 0 is
     *  used to specify all protocols.
     *
     *  @param filter_program the filter program to be applied on the received
     *  packets. The program uses tcpdump(1) style expression.
     *
     *  @param group_address the multicast group address to join.
     */
    XrlCmdError raw_link_0_1_join_multicast_group(
	// Input values,
	const string&	xrl_target_instance_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ether_type,
	const string&	filter_program,
	const Mac&	group_address);

    /**
     *  Leave a MAC multicast group.
     *
     *  @param xrl_target_instance_name the receiver's XRL target instance
     *  name.
     *
     *  @param if_name the interface through which packets should not be
     *  accepted.
     *
     *  @param vif_name the vif through which packets should not be accepted.
     *
     *  @param ether_type the EtherType protocol number or the Destination SAP
     *  that the receiver is not interested in anymore. It must be between 1536
     *  and 65535 to specify the EtherType, or between 1 and 255 to specify the
     *  Destination SAP for IEEE 802.2 LLC frames. A protocol number of 0 is
     *  used to specify all protocols.
     *
     *  @param filter_program the filter program that was applied on the
     *  received packets. The program uses tcpdump(1) style expression.
     *
     *  @param group_address the multicast group address to leave.
     */
    XrlCmdError raw_link_0_1_leave_multicast_group(
	// Input values,
	const string&	xrl_target_instance_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ether_type,
	const string&	filter_program,
	const Mac&	group_address);


    //
    // IPv4 Raw Socket Server Interface
    //

    /**
     *  Send an IPv4 packet on a raw socket.
     *
     *  @param if_name the interface to send the packet on. It is essential for
     *  multicast. In the unicast case this field may be empty.
     *
     *  @param vif_name the vif to send the packet on. It is essential for
     *  multicast. In the unicast case this field may be empty.
     *
     *  @param src_address the IP source address.
     *
     *  @param dst_address the IP destination address.
     *
     *  @param ip_protocol the IP protocol number. It must be between 1 and
     *  255.
     *
     *  @param ip_ttl the IP TTL (hop-limit). If it has a negative value, the
     *  TTL will be set internally before transmission.
     *
     *  @param ip_tos the Type Of Service (Diffserv/ECN bits for IPv4). If it
     *  has a negative value, the TOS will be set internally before
     *  transmission.
     *
     *  @param ip_router_alert if true, then add the IP Router Alert option to
     *  the IP packet.
     *
     *  @param ip_internet_control if true, then this is IP control traffic.
     *
     *  @param payload the payload, everything after the IP header and options.
     */
    XrlCmdError raw_packet4_0_1_send(
	// Input values,
	const string&	if_name,
	const string&	vif_name,
	const IPv4&	src_address,
	const IPv4&	dst_address,
	const uint32_t&	ip_protocol,
	const int32_t&	ip_ttl,
	const int32_t&	ip_tos,
	const bool&	ip_router_alert,
	const bool&	ip_internet_control,
	const vector<uint8_t>&	payload);

    /**
     *  Register to receive IPv4 packets. The receiver is expected to support
     *  raw_packet4_client/0.1 interface.
     *
     *  @param xrl_target_instance_name the receiver's XRL target instance
     *  name.
     *
     *  @param if_name the interface through which packets should be accepted.
     *
     *  @param vif_name the vif through which packets should be accepted.
     *
     *  @param ip_protocol the IP protocol number that the receiver is
     *  interested in. It must be between 0 and 255. A protocol number of 0 is
     *  used to specify all protocols.
     *
     *  @param enable_multicast_loopback if true then enable delivering of
     *  multicast datagrams back to this host (assuming the host is a member of
     *  the same multicast group.
     */
    XrlCmdError raw_packet4_0_1_register_receiver(
	// Input values,
	const string&	xrl_target_instance_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ip_protocol,
	const bool&	enable_multicast_loopback);

    /**
     *  Unregister to receive IPv4 packets.
     *
     *  @param xrl_target_instance_name the receiver's XRL target instance
     *  name.
     *
     *  @param if_name the interface through which packets should not be
     *  accepted.
     *
     *  @param vif_name the vif through which packets should not be accepted.
     *
     *  @param ip_protocol the IP Protocol number that the receiver is not
     *  interested in anymore. It must be between 0 and 255. A protocol number
     *  of 0 is used to specify all protocols.
     */
    XrlCmdError raw_packet4_0_1_unregister_receiver(
	// Input values,
	const string&	xrl_target_instance_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ip_protocol);

    /**
     *  Join an IPv4 multicast group.
     *
     *  @param xrl_target_instance_name the receiver's XRL target instance
     *  name.
     *
     *  @param if_name the interface through which packets should be accepted.
     *
     *  @param vif_name the vif through which packets should be accepted.
     *
     *  @param ip_protocol the IP protocol number that the receiver is
     *  interested in. It must be between 0 and 255. A protocol number of 0 is
     *  used to specify all protocols.
     *
     *  @param group_address the multicast group address to join.
     */
    XrlCmdError raw_packet4_0_1_join_multicast_group(
	// Input values,
	const string&	xrl_target_instance_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ip_protocol,
	const IPv4&	group_address);

    /**
     *  Leave an IPv4 multicast group.
     *
     *  @param xrl_target_instance_name the receiver's XRL target instance
     *  name.
     *
     *  @param if_name the interface through which packets should not be
     *  accepted.
     *
     *  @param vif_name the vif through which packets should not be accepted.
     *
     *  @param ip_protocol the IP protocol number that the receiver is not
     *  interested in anymore. It must be between 0 and 255. A protocol number
     *  of 0 is used to specify all protocols.
     *
     *  @param group_address the multicast group address to leave.
     */
    XrlCmdError raw_packet4_0_1_leave_multicast_group(
	// Input values,
	const string&	xrl_target_instance_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ip_protocol,
	const IPv4&	group_address);

#ifdef HAVE_IPV6
    //
    // IPv6 Raw Socket Server Interface
    //

    /**
     *  Send an IPv6 packet on a raw socket.
     *
     *  @param if_name the interface to send the packet on. It is essential for
     *  multicast. In the unicast case this field may be empty.
     *
     *  @param vif_name the vif to send the packet on. It is essential for
     *  multicast. In the unicast case this field may be empty.
     *
     *  @param src_address the IP source address.
     *
     *  @param dst_address the IP destination address.
     *
     *  @param ip_protocol the IP protocol number. It must be between 1 and
     *  255.
     *
     *  @param ip_ttl the IP TTL (hop-limit). If it has a negative value, the
     *  TTL will be set internally before transmission.
     *
     *  @param ip_tos the Type Of Service (IP traffic class for IPv6). If it
     *  has a negative value, the TOS will be set internally before
     *  transmission.
     *
     *  @param ip_router_alert if true, then add the IP Router Alert option to
     *  the IP packet.
     *
     *  @param ip_internet_control if true, then this is IP control traffic.
     *
     *  @param ext_headers_type a list of u32 integers with the types of the
     *  optional extention headers.
     *
     *  @param ext_headers_payload a list of payload data, one for each
     *  optional extention header. The number of entries must match
     *  ext_headers_type.
     *
     *  @param payload the payload, everything after the IP header and options.
     */
    XrlCmdError raw_packet6_0_1_send(
	// Input values,
	const string&	if_name,
	const string&	vif_name,
	const IPv6&	src_address,
	const IPv6&	dst_address,
	const uint32_t&	ip_protocol,
	const int32_t&	ip_ttl,
	const int32_t&	ip_tos,
	const bool&	ip_router_alert,
	const bool&	ip_internet_control,
	const XrlAtomList&	ext_headers_type,
	const XrlAtomList&	ext_headers_payload,
	const vector<uint8_t>&	payload);

    /**
     *  Register to receive IPv6 packets. The receiver is expected to support
     *  raw_packet6_client/0.1 interface.
     *
     *  @param xrl_target_instance_name the receiver's XRL target instance
     *  name.
     *
     *  @param if_name the interface through which packets should be accepted.
     *
     *  @param vif_name the vif through which packets should be accepted.
     *
     *  @param ip_protocol the IP protocol number that the receiver is
     *  interested in. It must be between 0 and 255. A protocol number of 0 is
     *  used to specify all protocols.
     *
     *  @param enable_multicast_loopback if true then enable delivering of
     *  multicast datagrams back to this host (assuming the host is a member of
     *  the same multicast group.
     */
    XrlCmdError raw_packet6_0_1_register_receiver(
	// Input values,
	const string&	xrl_target_instance_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ip_protocol,
	const bool&	enable_multicast_loopback);

    /**
     *  Unregister to receive IPv6 packets.
     *
     *  @param xrl_target_instance_name the receiver's XRL target instance
     *  name.
     *
     *  @param if_name the interface through which packets should not be
     *  accepted.
     *
     *  @param vif_name the vif through which packets should not be accepted.
     *
     *  @param ip_protocol the IP Protocol number that the receiver is not
     *  interested in anymore. It must be between 0 and 255. A protocol number
     *  of 0 is used to specify all protocols.
     */
    XrlCmdError raw_packet6_0_1_unregister_receiver(
	// Input values,
	const string&	xrl_target_instance_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ip_protocol);

    /**
     *  Join an IPv6 multicast group.
     *
     *  @param xrl_target_instance_name the receiver's XRL target instance
     *  name.
     *
     *  @param if_name the interface through which packets should be accepted.
     *
     *  @param vif_name the vif through which packets should be accepted.
     *
     *  @param ip_protocol the IP protocol number that the receiver is
     *  interested in. It must be between 0 and 255. A protocol number of 0 is
     *  used to specify all protocols.
     *
     *  @param group_address the multicast group address to join.
     */
    XrlCmdError raw_packet6_0_1_join_multicast_group(
	// Input values,
	const string&	xrl_target_instance_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ip_protocol,
	const IPv6&	group_address);

    /**
     *  Leave an IPv6 multicast group.
     *
     *  @param xrl_target_instance_name the receiver's XRL target instance
     *  name.
     *
     *  @param if_name the interface through which packets should not be
     *  accepted.
     *
     *  @param vif_name the vif through which packets should not be accepted.
     *
     *  @param ip_protocol the IP protocol number that the receiver is not
     *  interested in anymore. It must be between 0 and 255. A protocol number
     *  of 0 is used to specify all protocols.
     *
     *  @param group_address the multicast group address to leave.
     */
    XrlCmdError raw_packet6_0_1_leave_multicast_group(
	// Input values,
	const string&	xrl_target_instance_name,
	const string&	if_name,
	const string&	vif_name,
	const uint32_t&	ip_protocol,
	const IPv6&	group_address);
#endif

    //
    // TCP/UDP I/O Socket Server Interface
    //

    /**
     *  Open a TCP socket.
     *
     *  @param creator the Xrl Target instance name of the socket creator. The
     *  named target must implement socket4_user/0.1.
     *
     *  @param sockid return parameter that contains unique socket ID when
     *  socket instantiation is successful.
     */
    XrlCmdError socket4_0_1_tcp_open(
	// Input values,
	const string&	creator,
	// Output values,
	string&	sockid);

    /**
     *  Open an UDP socket.
     *
     *  @param creator the Xrl Target instance name of the socket creator. The
     *  named target must implement socket4_user/0.1.
     *
     *  @param sockid return parameter that contains unique socket ID when
     *  socket instantiation is successful.
     */
    XrlCmdError socket4_0_1_udp_open(
	// Input values,
	const string&	creator,
	// Output values,
	string&	sockid);

    /**
     *  Create a bound TCP socket.
     *
     *  @param creator the Xrl Target instance name of the socket creator. The
     *  named target must implement socket4_user/0.1.
     *
     *  @param local_addr the interface address to bind socket to.
     *
     *  @param local_port the port to bind socket to.
     *
     *  @param sockid return parameter that contains unique socket ID when
     *  socket instantiation is successful.
     */
    XrlCmdError socket4_0_1_tcp_open_and_bind(
	// Input values,
	const string&	creator,
	const IPv4&	local_addr,
	const uint32_t&	local_port,
	// Output values,
	string&	sockid);

    /**
     *  Create a bound UDP socket.
     *
     *  @param creator the Xrl Target instance name of the socket creator. The
     *  named target must implement socket4_user/0.1.
     *
     *  @param local_addr the interface address to bind socket to.
     *
     *  @param local_port the port to bind socket to.
     *
     *  @param sockid return parameter that contains unique socket ID when
     *  socket instantiation is successful.
     */
    XrlCmdError socket4_0_1_udp_open_and_bind(
	// Input values,
	const string&	creator,
	const IPv4&	local_addr,
	const uint32_t&	local_port,
	const string&   local_dev,
	const uint32_t& reuse,
	// Output values,
	string&	sockid);

    /**
     *  Create a bound UDP multicast socket.
     *
     *  @param creator the Xrl Target instance name of the socket creator. The
     *  named target must implement socket4_user/0.1.
     *
     *  @param local_addr the interface address to bind socket to.
     *
     *  @param local_port the port to bind socket to.
     *
     *  @param mcast_addr the multicast group address to join.
     *
     *  @param ttl the TTL to use for this multicast socket.
     *
     *  @param reuse allow other sockets to bind to same multicast group.
     *
     *  @param sockid return parameter that contains unique socket ID when
     *  socket instantiation is successful.
     */
    XrlCmdError socket4_0_1_udp_open_bind_join(
	// Input values,
	const string&	creator,
	const IPv4&	local_addr,
	const uint32_t&	local_port,
	const IPv4&	mcast_addr,
	const uint32_t&	ttl,
	const bool&	reuse,
	// Output values,
	string&	sockid);

    /**
     *  Create a bound and connected TCP socket.
     *
     *  @param creator the Xrl Target instance name of the socket creator. The
     *  named target must implement socket4_user/0.1.
     *
     *  @param local_addr the interface address to bind socket to.
     *
     *  @param local_port the port to bind socket to.
     *
     *  @param remote_addr the address to connect to.
     *
     *  @param remote_port the remote port to connect to.
     *
     *  @param sockid return parameter that contains unique socket ID when
     *  socket instantiation is successful.
     */
    XrlCmdError socket4_0_1_tcp_open_bind_connect(
	// Input values,
	const string&	creator,
	const IPv4&	local_addr,
	const uint32_t&	local_port,
	const IPv4&	remote_addr,
	const uint32_t&	remote_port,
	// Output values,
	string&	sockid);

    /**
     *  Create a bound and connected UDP socket.
     *
     *  @param creator the Xrl Target instance name of the socket creator. The
     *  named target must implement socket4_user/0.1.
     *
     *  @param local_addr the interface address to bind socket to.
     *
     *  @param local_port the port to bind socket to.
     *
     *  @param remote_addr the address to connect to.
     *
     *  @param remote_port the remote port to connect to.
     *
     *  @param sockid return parameter that contains unique socket ID when
     *  socket instantiation is successful.
     */
    XrlCmdError socket4_0_1_udp_open_bind_connect(
	// Input values,
	const string&	creator,
	const IPv4&	local_addr,
	const uint32_t&	local_port,
	const IPv4&	remote_addr,
	const uint32_t&	remote_port,
	// Output values,
	string&	sockid);

    /**
     * Create a bound and connected UDP broadcast socket.
     *
     * This socket may be used for sending and receiving IPv4 broadcasts
     * on a named if/vif. The TTL is always set to 1 on creation. The
     * creator must specify if this socket is to be used for limited
     * broadcasts (255.255.255.255) as this is a special case on many
     * platforms.
     *
     * @param creator the Xrl Target instance name of the socket
     *        creator.  The named target must implement socket4_user/0.1.
     *
     * @param ifname the interface name to bind socket to.
     *
     * @param vifname the vif to bind socket to.
     *
     * @param local_port the port to bind socket to.
     *
     * @param remote_port the remote port to connect to.
     *
     * @param reuse allow other sockets to bind to same port.
     *
     * @param limited set the socket up for transmission to the limited
     * broadcast address 255.255.255.255.
     *
     * @param connected connect the socket for use with send() not sendto().
     *
     * @param sockid return parameter that contains unique socket ID when
     *        socket instantiation is successful.
     */
    XrlCmdError socket4_0_1_udp_open_bind_broadcast(
	// Input values,
	const string&	creator,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	local_port,
	const uint32_t&	remote_port,
	const bool&	reuse,
	const bool&	limited,
	const bool&	connected,
	// Output values,
	string&	sockid);

    /**
     *  Bind a socket.
     *
     *  @param sockid the socket ID of the socket to bind.
     *
     *  @param local_addr the interface address to bind socket to.
     *
     *  @param local_port the port to bind socket to.
     */
    XrlCmdError socket4_0_1_bind(
	// Input values,
	const string&	sockid,
	const IPv4&	local_addr,
	const uint32_t&	local_port);

    /**
     *  Join multicast group on already bound socket.
     *
     *  @param sockid unique socket ID.
     *
     *  @param mcast_addr group to join.
     *
     *  @param join_if_addr interface address to perform join on.
     */
    XrlCmdError socket4_0_1_udp_join_group(
	// Input values,
	const string&	sockid,
	const IPv4&	mcast_addr,
	const IPv4&	join_if_addr);

    /**
     *  Leave multicast group on already bound socket.
     *
     *  @param sockid unique socket ID.
     *
     *  @param mcast_addr group to leave.
     *
     *  @param leave_if_addr interface address to perform leave on.
     */
    XrlCmdError socket4_0_1_udp_leave_group(
	// Input values,
	const string&	sockid,
	const IPv4&	mcast_addr,
	const IPv4&	leave_if_addr);

    /**
     *  Close socket.
     *
     *  @param sockid unique socket ID of socket to be closed.
     */
    XrlCmdError socket4_0_1_close(
	// Input values,
	const string&	sockid);

    /**
     *  Listen for inbound connections on socket. When a connection request
     *  received the socket creator will receive notification through
     *  socket4_user/0.1/connect_event.
     *
     *  @param sockid the unique socket ID of the socket to perform listen.
     *
     *  @param backlog the maximum number of pending connections.
     */
    XrlCmdError socket4_0_1_tcp_listen(
	// Input values,
	const string&	sockid,
	const uint32_t&	backlog);

    /**
     *  Enable a UDP socket for datagram reception.
     *
     *  If a UDP socket has been created without using the usual convenience
     *  XRLs, it is necessary to hook up its FEA internal input path by
     *  calling this XRL. It is similar in intent to tcp_listen, but named
     *  differently as it never uses the listen() socket API.
     *
     *  @param sockid the unique socket ID of the socket to enable for
     *  datagram reception.
     */
    XrlCmdError socket4_0_1_udp_enable_recv(
	// Input values,
	const string&	sockid);

    /**
     *  Send data on socket.
     *
     *  @param sockid unique socket ID.
     *
     *  @param data block of data to be sent.
     */
    XrlCmdError socket4_0_1_send(
	// Input values,
	const string&	sockid,
	const vector<uint8_t>&	data);

    /**
     *  Send data on socket to a given destination. The packet is not routed as
     *  the forwarding engine sending the packet may not have access to the
     *  full routing table.
     *
     *  @param sockid unique socket ID.
     *
     *  @param remote_addr destination address for data.
     *
     *  @param remote_port destination port for data.
     *
     *  @param data block of data to be sent.
     */
    XrlCmdError socket4_0_1_send_to(
	// Input values,
	const string&	sockid,
	const IPv4&	remote_addr,
	const uint32_t&	remote_port,
	const vector<uint8_t>&	data);

    /**
     *  Send data on socket to a given multicast group from a given interface.
     *
     *  @param sockid unique socket ID.
     *
     *  @param group_addr destination address for data.
     *
     *  @param group_port destination port for data.
     *
     *  @param ifaddr interface address
     */
    XrlCmdError socket4_0_1_send_from_multicast_if(
	// Input values,
	const string&	sockid,
	const IPv4&	group_addr,
	const uint32_t&	group_port,
	const IPv4&	ifaddr,
	const vector<uint8_t>&	data);

    /**
     *  Set a named socket option with an integer value.
     *
     *  @param sockid unique socket ID.
     *
     *  @param optname name of option to be set. Valid values are:
     *  "onesbcast"
     *  "receive_broadcast"
     *  "reuseport"
     *  "send_broadcast"
     *  "tos"
     *  "ttl"
     *  "multicast_loopback"
     *  "multicast_ttl"
     *
     *  @param optval value of option to be set. If value is logically boolean
     *  then zero represents false and any non-zero value true.
     */
    XrlCmdError socket4_0_1_set_socket_option(
	// Input values,
	const string&	sockid,
	const string&	optname,
	const uint32_t&	optval);

    /**
     *  Set a named socket option with a text value.
     *
     * XXX: The "bindtodevice" option exists to workaround an architectural
     * issue in the Linux Ipv4 stack. It SHOULD NOT be used for new code.
     *
     *  @param sockid unique socket ID.
     *
     *  @param optname name of option to be set. Valid values are:
     *  "bindtodevice"
     *
     *  @param optval value of option to be set. If value is logically boolean
     *  then zero represents false and any non-zero value true.
     */
    XrlCmdError socket4_0_1_set_socket_option_txt(
	// Input values,
	const string&	sockid,
	const string&	optname,
	const string&	optval);

#ifdef HAVE_IPV6
    /**
     *  Open a TCP socket.
     *
     *  @param creator the Xrl Target instance name of the socket creator. The
     *  named target must implement socket4_user/0.1.
     *
     *  @param sockid return parameter that contains unique socket ID when
     *  socket instantiation is successful.
     */
    XrlCmdError socket6_0_1_tcp_open(
	// Input values,
	const string&	creator,
	// Output values,
	string&	sockid);

    /**
     *  Open an UDP socket.
     *
     *  @param creator the Xrl Target instance name of the socket creator. The
     *  named target must implement socket4_user/0.1.
     *
     *  @param sockid return parameter that contains unique socket ID when
     *  socket instantiation is successful.
     */
    XrlCmdError socket6_0_1_udp_open(
	// Input values,
	const string&	creator,
	// Output values,
	string&	sockid);

    /**
     *  Create a bound TCP socket.
     *
     *  @param creator the Xrl Target instance name of the socket creator. The
     *  named target must implement socket6_user/0.1.
     *
     *  @param local_addr the interface address to bind socket to.
     *
     *  @param local_port the port to bind socket to.
     *
     *  @param sockid return parameter that contains unique socket ID when
     *  socket instantiation is successful.
     */
    XrlCmdError socket6_0_1_tcp_open_and_bind(
	// Input values,
	const string&	creator,
	const IPv6&	local_addr,
	const uint32_t&	local_port,
	// Output values,
	string&	sockid);

    /**
     *  Create a bound UDP socket.
     *
     *  @param creator the Xrl Target instance name of the socket creator. The
     *  named target must implement socket6_user/0.1.
     *
     *  @param local_addr the interface address to bind socket to.
     *
     *  @param local_port the port to bind socket to.
     *
     *  @param sockid return parameter that contains unique socket ID when
     *  socket instantiation is successful.
     */
    XrlCmdError socket6_0_1_udp_open_and_bind(
	// Input values,
	const string&	creator,
	const IPv6&	local_addr,
	const uint32_t&	local_port,
	const string&   local_dev,
	const uint32_t& reuse,
	// Output values,
	string&	sockid);

    /**
     *  Create a bound UDP multicast socket.
     *
     *  @param creator the Xrl Target instance name of the socket creator. The
     *  named target must implement socket6_user/0.1.
     *
     *  @param local_addr the interface address to bind socket to.
     *
     *  @param local_port the port to bind socket to.
     *
     *  @param mcast_addr the multicast group address to join.
     *
     *  @param ttl the TTL to use for this multicast socket.
     *
     *  @param reuse allow other sockets to bind to same multicast group.
     *
     *  @param sockid return parameter that contains unique socket ID when
     *  socket instantiation is successful.
     */
    XrlCmdError socket6_0_1_udp_open_bind_join(
	// Input values,
	const string&	creator,
	const IPv6&	local_addr,
	const uint32_t&	local_port,
	const IPv6&	mcast_addr,
	const uint32_t&	ttl,
	const bool&	reuse,
	// Output values,
	string&	sockid);

    /**
     *  Create a bound and connected TCP socket.
     *
     *  @param creator the Xrl Target instance name of the socket creator. The
     *  named target must implement socket6_user/0.1.
     *
     *  @param local_addr the interface address to bind socket to.
     *
     *  @param local_port the port to bind socket to.
     *
     *  @param remote_addr the address to connect to.
     *
     *  @param remote_port the remote port to connect to.
     *
     *  @param sockid return parameter that contains unique socket ID when
     *  socket instantiation is successful.
     */
    XrlCmdError socket6_0_1_tcp_open_bind_connect(
	// Input values,
	const string&	creator,
	const IPv6&	local_addr,
	const uint32_t&	local_port,
	const IPv6&	remote_addr,
	const uint32_t&	remote_port,
	// Output values,
	string&	sockid);

    /**
     *  Create a bound and connected UDP socket.
     *
     *  @param creator the Xrl Target instance name of the socket creator. The
     *  named target must implement socket6_user/0.1.
     *
     *  @param local_addr the interface address to bind socket to.
     *
     *  @param local_port the port to bind socket to.
     *
     *  @param remote_addr the address to connect to.
     *
     *  @param remote_port the remote port to connect to.
     *
     *  @param sockid return parameter that contains unique socket ID when
     *  socket instantiation is successful.
     */
    XrlCmdError socket6_0_1_udp_open_bind_connect(
	// Input values,
	const string&	creator,
	const IPv6&	local_addr,
	const uint32_t&	local_port,
	const IPv6&	remote_addr,
	const uint32_t&	remote_port,
	// Output values,
	string&	sockid);

    /**
     *  Bind a socket.
     *
     *  @param sockid the socket ID of the socket to bind.
     *
     *  @param local_addr the interface address to bind socket to.
     *
     *  @param local_port the port to bind socket to.
     */
    XrlCmdError socket6_0_1_bind(
	// Input values,
	const string&	sockid,
	const IPv6&	local_addr,
	const uint32_t&	local_port);

    /**
     *  Join multicast group on already bound socket.
     *
     *  @param sockid unique socket ID.
     *
     *  @param mcast_addr group to join.
     *
     *  @param join_if_addr interface address to perform join on.
     */
    XrlCmdError socket6_0_1_udp_join_group(
	// Input values,
	const string&	sockid,
	const IPv6&	mcast_addr,
	const IPv6&	join_if_addr);

    /**
     *  Leave multicast group on already bound socket.
     *
     *  @param sockid unique socket ID.
     *
     *  @param mcast_addr group to leave.
     *
     *  @param leave_if_addr interface address to perform leave on.
     */
    XrlCmdError socket6_0_1_udp_leave_group(
	// Input values,
	const string&	sockid,
	const IPv6&	mcast_addr,
	const IPv6&	leave_if_addr);

    /**
     *  Close socket.
     *
     *  @param sockid unique socket ID of socket to be closed.
     */
    XrlCmdError socket6_0_1_close(
	// Input values,
	const string&	sockid);

    /**
     *  Listen for inbound connections on socket. When a connection request
     *  received the socket creator will receive notification through
     *  socket6_user/0.1/connect_event.
     *
     *  @param sockid the unique socket ID of the socket to perform listen.
     *
     *  @param backlog the maximum number of pending connections.
     */
    XrlCmdError socket6_0_1_tcp_listen(
	// Input values,
	const string&	sockid,
	const uint32_t&	backlog);

    /**
     *  Send data on socket.
     *
     *  @param sockid unique socket ID.
     *
     *  @param data block of data to be sent.
     */
    XrlCmdError socket6_0_1_send(
	// Input values,
	const string&	sockid,
	const vector<uint8_t>&	data);

    /**
     *  Send data on socket to a given destination. The packet is not routed as
     *  the forwarding engine sending the packet may not have access to the
     *  full routing table.
     *
     *  @param sockid unique socket ID.
     *
     *  @param remote_addr destination address for data.
     *
     *  @param remote_port destination port for data.
     *
     *  @param data block of data to be sent.
     */
    XrlCmdError socket6_0_1_send_to(
	// Input values,
	const string&	sockid,
	const IPv6&	remote_addr,
	const uint32_t&	remote_port,
	const vector<uint8_t>&	data);

    /**
     *  Send data on socket to a given multicast group from a given interface.
     *
     *  @param sockid unique socket ID.
     *
     *  @param group_addr destination address for data.
     *
     *  @param group_port destination port for data.
     *
     *  @param ifaddr interface address
     */
    XrlCmdError socket6_0_1_send_from_multicast_if(
	// Input values,
	const string&	sockid,
	const IPv6&	group_addr,
	const uint32_t&	group_port,
	const IPv6&	ifaddr,
	const vector<uint8_t>&	data);

    /**
     *  Set a named socket option.
     *
     *  @param sockid unique socket ID.
     *
     *  @param optname name of option to be set. Valid values are:
     *  "reuseport"
     *  "ttl"
     *  "multicast_loopback"
     *  "multicast_ttl"
     *
     *  @param optval value of option to be set. If value is logically boolean
     *  then zero represents false and any non-zero value true.
     */
    XrlCmdError socket6_0_1_set_socket_option(
	// Input values,
	const string&	sockid,
	const string&	optname,
	const uint32_t&	optval);
#endif

#ifndef XORP_DISABLE_PROFILE
    //
    // Profile-related interface
    //
    XrlCmdError profile_0_1_enable(
	// Input values,
	const string&	pname);

    XrlCmdError profile_0_1_disable(
	// Input values,
	const string&	pname);

    XrlCmdError profile_0_1_get_entries(
	// Input values,
	const string&	pname,
	const string&	instance_name);

    XrlCmdError profile_0_1_clear(
	// Input values,
	const string&	pname);

    XrlCmdError profile_0_1_list(
	// Output values,
	string&	info);
#endif

private:
    /**
     * Add/remove a multicast MAC address on an interface.
     *
     * @param add if true, then add the address, otherwise remove it.
     * @param ifname the interface name.
     * @param mac the address to add/remove.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_remove_mac(bool add, const string& ifname, const Mac& mac,
		       string& error_msg);

    /**
     * Set the MAC address on an interface.
     *
     * @param ifname the interface name.
     * @param mac the address to set.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_mac(const string& ifname, const Mac& mac, string& error_msg);

    int add_remove_address(bool add, const string& ifname, const string& vifname,
			   const IPv4& ip, uint32_t prefix, string& error_msg);


    /**
     * Send gratuitous ARP packets for all IP addresses on on an interface.
     *
     * @param ifname the interface name.
     * @param mac the MAC address that is to be updated.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int send_gratuitous_arps(const string& ifname, const Mac& mac,
			    string& error_msg);

    EventLoop&		_eventloop;	// The event loop to use
    FeaNode&		_fea_node;	// The corresponding FeaNode

    XrlRouter&		       	_xrl_router;
#ifndef XORP_DISABLE_PROFILE
    Profile&			_profile;
#endif
    XrlFibClientManager&	_xrl_fib_client_manager;
    IfConfig&			_ifconfig;
#ifndef XORP_DISABLE_FIREWALL
    FirewallManager&		_firewall_manager;
#endif
    FibConfig&			_fibconfig;
    IoLinkManager&		_io_link_manager;
    IoIpManager&		_io_ip_manager;
    IoTcpUdpManager&		_io_tcpudp_manager;
    LibFeaClientBridge&		_lib_fea_client_bridge;

    bool	_is_running;	// True if the service is running
    bool	_is_shutdown_received; // True if shutdown XRL request received

#ifdef XORP_USE_CLICK
    //
    // The externally loadable managers
    //
    FeaDataPlaneManagerClick*	_fea_data_plane_manager_click;
#endif
};

#endif // __FEA_XRL_FEA_TARGET_HH__
