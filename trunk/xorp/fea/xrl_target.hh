// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/fea/xrl_target.hh,v 1.44 2004/11/29 00:32:03 bms Exp $

#ifndef __FEA_XRL_TARGET_HH__
#define __FEA_XRL_TARGET_HH__

#include "xrl/targets/fea_base.hh"
#include "xrl_fti.hh"
#include "xrl_ifmanager.hh"

class FtiConfig;
class InterfaceManager;
class LibFeaClientBridge;
class XrlIfConfigUpdateReporter;
class XrlRawSocket4Manager;
class XrlSocketServer;
class Profile;

class XrlFeaTarget : public XrlFeaTargetBase {
public:
    XrlFeaTarget(EventLoop&			e,
		 XrlRouter&			rtr,
		 FtiConfig& 			ftic,
		 InterfaceManager& 		ifmgr,
		 XrlIfConfigUpdateReporter&	ifupd,
		 Profile&			profile,
		 XrlRawSocket4Manager*		xrsm4	= 0,
		 LibFeaClientBridge*		lfbr	= 0,
		 XrlSocketServer*		xss	= 0);

    bool done() const { return _done; }

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
     *  Specify the external program to generate the Click configuration.
     *
     *  @param click_config_generator_file the name of the external program to
     *  generate the Click configuration.
     */
    XrlCmdError fea_click_0_1_set_click_config_generator_file(
	// Input values,
	const string&	click_config_generator_file);

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
     *  Add a FIB client.
     *
     *  @param target_name the target name of the FIB client to add.
     *  @param send_updates whether updates should be sent.
     *  @param send_resolves whether resolve requests should be sent.
     */
    XrlCmdError fea_fib_0_1_add_fib_client4(
	// Input values,
	const string&	target_name,
	const bool&	send_updates,
	const bool&	send_resolves);

    XrlCmdError fea_fib_0_1_add_fib_client6(
	// Input values,
	const string&	target_name,
	const bool&	send_updates,
	const bool&	send_resolves);

    /**
     *  Delete a FIB client.
     *
     *  @param target_name the target name of the FIB client to delete.
     */
    XrlCmdError fea_fib_0_1_delete_fib_client4(
	// Input values,
	const string&	target_name);

    XrlCmdError fea_fib_0_1_delete_fib_client6(
	// Input values,
	const string&	target_name);

    //
    // FEA network interface management interface
    //

    XrlCmdError ifmgr_0_1_get_system_interface_names(
	// Output values,
	XrlAtomList&	ifnames);

    XrlCmdError ifmgr_0_1_get_configured_interface_names(
	// Output values,
	XrlAtomList&	ifnames);

    XrlCmdError ifmgr_0_1_get_system_vif_names(
	const string&		ifname,
	// Output values,
	XrlAtomList&		ifnames);

    XrlCmdError ifmgr_0_1_get_configured_vif_names(
	const string&	ifname,
	// Output values,
	XrlAtomList&		ifnames);

    XrlCmdError ifmgr_0_1_get_system_vif_flags(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	bool&	enabled,
	bool&	broadcast,
	bool&	loopback,
	bool&	point_to_point,
	bool&	multicast);

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

    XrlCmdError ifmgr_0_1_get_system_vif_pif_index(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	uint32_t&	pif_index);

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
     *  Implicitly configure an interface within the FEA by using information
     *  from the underlying system.
     *
     *  @param tid the transaction ID.
     *
     *  @param ifname the name of the interface to configure.
     */
    XrlCmdError ifmgr_0_1_configure_interface_from_system(
	// Input values,
	const uint32_t&	tid,
	const string&	ifname);

    XrlCmdError ifmgr_0_1_set_interface_enabled(
	// Input values,
	const uint32_t& tid,
	const string&	ifname,
	const bool&	enabled);

    XrlCmdError ifmgr_0_1_get_system_interface_enabled(
	// Input values,
	const string&	ifname,
	// Output values,
	bool&		enabled);

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

    XrlCmdError ifmgr_0_1_get_system_interface_discard(
	// Input values,
	const string&	ifname,
	// Output values,
	bool&		discard);

    XrlCmdError ifmgr_0_1_set_mac(
	// Input values,
	const uint32_t& tid,
	const string&	ifname,
	const Mac&	mac);

    XrlCmdError ifmgr_0_1_get_system_mac(
	// Input values,
	const string&	ifname,
	// Output values,
	Mac&	mac);

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

    XrlCmdError ifmgr_0_1_get_system_mtu(
	// Input values,
	const string&	ifname,
	// Output values,
	uint32_t&	mtu);

    XrlCmdError ifmgr_0_1_get_configured_mtu(
	// Input values,
	const string&	ifname,
	// Output values,
	uint32_t&	mtu);

    XrlCmdError ifmgr_0_1_get_system_address_flags4(
	// Input values,
	const string& ifname,
	const string& vifname,
	const IPv4&   address,
	// Output values,
	bool& up,
	bool& broadcast,
	bool& loopback,
	bool& point_to_point,
	bool& multicast);

    XrlCmdError ifmgr_0_1_get_system_address_flags6(
	// Input values,
	const string& ifname,
	const string& vifname,
	const IPv6&   address,
	// Output values,
	bool& up,
	bool& loopback,
	bool& point_to_point,
	bool& multicast);

    XrlCmdError ifmgr_0_1_get_configured_address_flags4(
	// Input values,
	const string& ifname,
	const string& vifname,
	const IPv4&   address,
	// Output values,
	bool& up,
	bool& broadcast,
	bool& loopback,
	bool& point_to_point,
	bool& multicast);

    XrlCmdError ifmgr_0_1_get_configured_address_flags6(
	// Input values,
	const string& ifname,
	const string& vifname,
	const IPv6&   address,
	// Output values,
	bool& up,
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

    XrlCmdError ifmgr_0_1_get_system_vif_enabled(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	bool&		enabled);

    XrlCmdError ifmgr_0_1_get_configured_vif_enabled(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	bool&		enabled);

    XrlCmdError ifmgr_0_1_get_system_vif_addresses4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	XrlAtomList&	addresses);

    XrlCmdError ifmgr_0_1_get_configured_vif_addresses4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	XrlAtomList&	addresses);

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
	const bool&	en);

    XrlCmdError ifmgr_0_1_get_system_address_enabled4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	bool&		enabled);

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

    XrlCmdError ifmgr_0_1_get_system_prefix4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	// Output values,
	uint32_t&	prefix_len);

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

    XrlCmdError ifmgr_0_1_get_system_broadcast4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	// Output values,
	IPv4&		broadcast);

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

    XrlCmdError ifmgr_0_1_get_system_endpoint4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	// Output values,
	IPv4&	endpoint);

    XrlCmdError ifmgr_0_1_get_configured_endpoint4(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv4&	address,
	// Output values,
	IPv4&	endpoint);

    XrlCmdError ifmgr_0_1_get_system_vif_addresses6(
	// Input values,
	const string&	ifname,
	const string&	vif,
	// Output values,
	XrlAtomList&	addresses);

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

    XrlCmdError ifmgr_0_1_get_system_address_enabled6(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address,
	bool&		enabled);

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

    XrlCmdError ifmgr_0_1_get_system_prefix6(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address,
	// Output values,
	uint32_t&	prefix_len);

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

    XrlCmdError ifmgr_0_1_get_system_endpoint6(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address,
	// Output values,
	IPv6&		endpoint);

    XrlCmdError ifmgr_0_1_get_configured_endpoint6(
	// Input values,
	const string&	ifname,
	const string&	vif,
	const IPv6&	address,
	// Output values,
	IPv6&		endpoint);

    XrlCmdError ifmgr_0_1_register_client(
	// Input values,
	const string&	spyname);

    XrlCmdError ifmgr_0_1_unregister_client(
	// Input values,
	const string&	spyname);

    XrlCmdError ifmgr_0_1_register_system_interfaces_client(
	// Input values,
	const string&	spyname);

    XrlCmdError ifmgr_0_1_unregister_system_interfaces_client(
	// Input values,
	const string&	spyname);

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

    XrlCmdError fti_0_2_have_ipv4(
	// Output values, 
	bool&	result);

    XrlCmdError fti_0_2_have_ipv6(
	// Output values, 
	bool&	result);

    XrlCmdError fti_0_2_get_unicast_forwarding_enabled4(
	// Output values,
	bool&	enabled);

    XrlCmdError fti_0_2_get_unicast_forwarding_enabled6(
	// Output values,
	bool&	enabled);

    XrlCmdError fti_0_2_set_unicast_forwarding_enabled4(
	// Input values,
	const bool&	enabled);

    XrlCmdError fti_0_2_set_unicast_forwarding_enabled6(
	// Input values,
	const bool&	enabled);

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
	const IPv4Net&	network,
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
	const IPv6Net&	network,
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

    //
    // IPv4 Raw Socket Server Interface
    //

    XrlCmdError raw_packet4_0_1_send(
	// Input values,
	const IPv4&		src_address,
	const IPv4&		dst_address,
	const string&		vifname,
	const uint32_t&		proto,
	const uint32_t&		ttl,
	const uint32_t&		tos,
	const vector<uint8_t>&	options,
	const vector<uint8_t>&	payload);

    XrlCmdError raw_packet4_0_1_send_raw(
	// Input values,
	const string&		vifname,
	const vector<uint8_t>&	packet);

    XrlCmdError raw_packet4_0_1_register_vif_receiver(
	// Input values,
	const string&	router_name,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	proto);

    XrlCmdError raw_packet4_0_1_unregister_vif_receiver(
	// Input values,
	const string&	router_name,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	proto);

    //
    // IPv6 Raw Socket Server Interface
    //

    XrlCmdError raw_packet6_0_1_send_raw(
	// Input values,
	const string&		vifname,
	const vector<uint8_t>&	pktinfo,
	const vector<uint8_t>&	packet);

    XrlCmdError raw_packet6_0_1_register_vif_receiver(
	// Input values,
	const string&	router_name,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	proto);

    XrlCmdError raw_packet6_0_1_unregister_vif_receiver(
	// Input values,
	const string&	router_name,
	const string&	ifname,
	const string&	vifname,
	const uint32_t&	proto);

    //
    // Socket Locator interface(s)
    //
    XrlCmdError socket4_locator_0_1_find_socket_server_for_addr(
	// Input value
	const IPv4& addr,
	// Output value
	string&	xrl_target);

    XrlCmdError socket6_locator_0_1_find_socket_server_for_addr(
	// Input value
	const IPv6& addr,
	// Output value
	string&	xrl_target);

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
private:
    bool have_ipv4() const { return (_have_ipv4); };
    bool have_ipv6() const { return (_have_ipv6); };

    XrlRouter&		       	_xrl_router;
    XrlFtiTransactionManager	_xftm;
    XrlInterfaceManager 	_xifmgr;
    XrlIfConfigUpdateReporter&	_xifcur;
    Profile&			_profile;
    XrlRawSocket4Manager*	_xrsm4;
    LibFeaClientBridge*		_lfcb;
    XrlSocketServer*		_xss;

    bool			_done;		// True if we are done
    bool			_have_ipv4;	// True if we have IPv4
    bool			_have_ipv6;	// True if we have IPv6
};

#endif //  __FEA_XRL_TARGET_HH__
