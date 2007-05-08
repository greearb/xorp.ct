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

#ident "$XORP: xorp/fea/xrl_fea_target.cc,v 1.15 2007/04/28 00:49:12 pavlin Exp $"


//
// FEA (Forwarding Engine Abstraction) XRL target implementation.
//

#define PROFILE_UTILS_REQUIRED

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/status_codes.h"
#include "libxorp/eventloop.hh"

#include "libxipc/xrl_std_router.hh"

#include "xrl/interfaces/profile_client_xif.hh"
//
// XXX: file "libxorp/profile.hh" must be included after
// "libxipc/xrl_std_router.hh" and "xrl/interfaces/profile_client_xif.hh"
// Sigh...
//
#include "libxorp/profile.hh"

#include "fea_node.hh"
#include "ifconfig_transaction.hh"
#include "libfeaclient_bridge.hh"
#include "profile_vars.hh"
#include "xrl_fea_target.hh"
#include "xrl_rawsock4.hh"
#include "xrl_rawsock6.hh"
#include "xrl_socket_server.hh"


XrlFeaTarget::XrlFeaTarget(EventLoop&			eventloop,
			   FeaNode&			fea_node,
			   XrlRouter&			xrl_router,
			   Profile&			profile,
			   XrlFibClientManager&		xrl_fib_client_manager,
			   XrlRawSocket4Manager&	xrsm4,
			   XrlRawSocket6Manager&	xrsm6,
			   LibFeaClientBridge&		lib_fea_client_bridge,
			   XrlSocketServer&		xrl_socket_server)
    : XrlFeaTargetBase(&xrl_router),
      _eventloop(eventloop),
      _fea_node(fea_node),
      _xrl_router(xrl_router),
      _profile(profile),
      _xrl_fib_client_manager(xrl_fib_client_manager),
      _xrsm4(xrsm4),
      _xrsm6(xrsm6),
      _lib_fea_client_bridge(lib_fea_client_bridge),
      _xrl_socket_server(xrl_socket_server),
      _is_running(false),
      _is_shutdown_received(false),
      _have_ipv4(false),
      _have_ipv6(false)
{
}

XrlFeaTarget::~XrlFeaTarget()
{
    shutdown();
}

int
XrlFeaTarget::startup()
{
    _have_ipv4 = _fea_node.fibconfig().have_ipv4();
    _have_ipv6 = _fea_node.fibconfig().have_ipv6();

    _is_running = true;

    return (XORP_OK);
}

int
XrlFeaTarget::shutdown()
{
    _is_running = false;

    return (XORP_OK);
}

bool
XrlFeaTarget::is_running() const
{
    return (_is_running);
}

IfConfig&
XrlFeaTarget::ifconfig()
{
    return (_fea_node.ifconfig());
}

FibConfig&
XrlFeaTarget::fibconfig()
{
    return (_fea_node.fibconfig());
}

XrlCmdError
XrlFeaTarget::common_0_1_get_target_name(
    // Output values,
    string&	name)
{
    name = "fea";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::common_0_1_get_version(
    // Output values,
    string&   version)
{
    version = "0.1";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::common_0_1_get_status(
    // Output values,
    uint32_t&	status,
    string&	reason)
{
    ProcessStatus s;
    string r;
    s = ifconfig().status(r);
    //if it's bad news, don't bother to ask any other modules.
    switch (s) {
    case PROC_FAILED:
    case PROC_SHUTDOWN:
    case PROC_DONE:
	status = s;
	reason = r;
	return XrlCmdError::OKAY();
    case PROC_NOT_READY:
	reason = r;
	break;
    case PROC_READY:
	break;
    case PROC_NULL:
	//can't be running and in this state
	abort();
    case PROC_STARTUP:
	//can't be responding to an XRL and in this state
	abort();
    }
    status = s;

    if (_is_shutdown_received) {
	status = PROC_SHUTDOWN;	// XXX: the process received shutdown request
	reason = "";
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::common_0_1_shutdown()
{
    _is_shutdown_received = true;

    return XrlCmdError::OKAY();
}

/**
 *  Enable/disable Click FEA support.
 *
 *  @param enable if true, then enable the Click FEA support, otherwise
 *  disable it.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_enable_click(
    // Input values,
    const bool&	enable)
{
    ifconfig().enable_click(enable);
    fibconfig().enable_click(enable);

    return XrlCmdError::OKAY();
}

/**
 *  Start Click FEA support.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_start_click()
{
    string error_msg;

    if (ifconfig().start_click(error_msg) < 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    if (fibconfig().start_click(error_msg) < 0) {
	string dummy_msg;
	ifconfig().stop_click(error_msg);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

/**
 *  Stop Click FEA support.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_stop_click()
{
    string error_msg1, error_msg2;

    if (fibconfig().stop_click(error_msg1) < 0) {
	ifconfig().stop_click(error_msg2);
	return XrlCmdError::COMMAND_FAILED(error_msg1);
    }
    if (ifconfig().stop_click(error_msg1) < 0) {
	return XrlCmdError::COMMAND_FAILED(error_msg1);
    }

    return XrlCmdError::OKAY();
}

/**
 *  Enable/disable duplicating the Click routes to the system kernel.
 *
 *  @param enable if true, then enable duplicating the Click routes to the
 *  system kernel, otherwise disable it.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_enable_duplicate_routes_to_kernel(
    // Input values,
    const bool&	enable)
{
    fibconfig().enable_duplicate_routes_to_kernel(enable);

    return XrlCmdError::OKAY();
}

/**
 *  Enable/disable kernel-level Click FEA support.
 *
 *  @param enable if true, then enable the kernel-level Click FEA support,
 *  otherwise disable it.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_enable_kernel_click(
    // Input values,
    const bool&	enable)
{
    ifconfig().enable_kernel_click(enable);
    fibconfig().enable_kernel_click(enable);

    return XrlCmdError::OKAY();
}

/**
 *  Enable/disable installing kernel-level Click on startup.
 *
 *  @param enable if true, then install kernel-level Click on startup.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_enable_kernel_click_install_on_startup(
    // Input values,
    const bool&	enable)
{
    ifconfig().enable_kernel_click_install_on_startup(enable);
    fibconfig().enable_kernel_click_install_on_startup(enable);

    return XrlCmdError::OKAY();
}

/**
 *  Specify the list of kernel Click modules to load on startup if
 *  installing kernel-level Click on startup is enabled. The file names of
 *  the kernel modules are separated by colon.
 *
 *  @param modules the list of kernel Click modules (separated by colon) to
 *  load.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_set_kernel_click_modules(
    // Input values,
    const string&	modules)
{
    list<string> modules_list;

    //
    // Split the string with the names of the modules (separated by colon)
    //
    string modules_tmp = modules;
    string::size_type colon;
    string name;
    do {
	if (modules_tmp.empty())
	    break;
	colon = modules_tmp.find(':');
	name = modules_tmp.substr(0, colon);
	if (colon != string::npos)
	    modules_tmp = modules_tmp.substr(colon + 1);
	else
	    modules_tmp.erase();
	if (! name.empty())
	    modules_list.push_back(name);
    } while (true);

    ifconfig().set_kernel_click_modules(modules_list);
    fibconfig().set_kernel_click_modules(modules_list);

    return XrlCmdError::OKAY();
}

/**
 *  Specify the kernel-level Click mount directory.
 *
 *  @param directory the kernel-level Click mount directory.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_set_kernel_click_mount_directory(
    // Input values,
    const string&	directory)
{
    ifconfig().set_kernel_click_mount_directory(directory);
    fibconfig().set_kernel_click_mount_directory(directory);

    return XrlCmdError::OKAY();
}

/**
 *  Specify the external program to generate the kernel-level Click
 *  configuration.
 *
 *  @param kernel_click_config_generator_file the name of the external
 *  program to generate the kernel-level Click configuration.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_set_kernel_click_config_generator_file(
    // Input values,
    const string&	kernel_click_config_generator_file)
{
    ifconfig().set_kernel_click_config_generator_file(
	kernel_click_config_generator_file);
    fibconfig().set_kernel_click_config_generator_file(
	kernel_click_config_generator_file);

    return XrlCmdError::OKAY();
}

/**
 *  Enable/disable user-level Click FEA support.
 *
 *  @param enable if true, then enable the user-level Click FEA support,
 *  otherwise disable it.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_enable_user_click(
    // Input values,
    const bool&	enable)
{
    ifconfig().enable_user_click(enable);
    fibconfig().enable_user_click(enable);

    return XrlCmdError::OKAY();
}

/**
 *  Specify the user-level Click command file.
 *
 *  @param user_click_command_file the name of the user-level Click command
 *  file.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_set_user_click_command_file(
    // Input values,
    const string&	user_click_command_file)
{
    ifconfig().set_user_click_command_file(user_click_command_file);
    fibconfig().set_user_click_command_file(user_click_command_file);

    return XrlCmdError::OKAY();
}

/**
 *  Specify the extra arguments to the user-level Click command.
 *
 *  @param user_click_command_extra_arguments the extra arguments to the
 *  user-level Click command.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_set_user_click_command_extra_arguments(
    // Input values,
    const string&	user_click_command_extra_arguments)
{
    ifconfig().set_user_click_command_extra_arguments(
	user_click_command_extra_arguments);
    fibconfig().set_user_click_command_extra_arguments(
	user_click_command_extra_arguments);

    return XrlCmdError::OKAY();
}

/**
 *  Specify whether to execute on startup the user-level Click command.
 *
 *  @param user_click_command_execute_on_startup if true, then execute the
 *  user-level Click command on startup.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_set_user_click_command_execute_on_startup(
    // Input values,
    const bool&	user_click_command_execute_on_startup)
{
    ifconfig().set_user_click_command_execute_on_startup(
	user_click_command_execute_on_startup);
    fibconfig().set_user_click_command_execute_on_startup(
	user_click_command_execute_on_startup);

    return XrlCmdError::OKAY();
}

/**
 *  Specify the address to use for control access to the user-level
 *  Click.
 *
 *  @param user_click_control_address the address to use for
 *  control access to the user-level Click.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_set_user_click_control_address(
    // Input values,
    const IPv4&	user_click_control_address)
{
    ifconfig().set_user_click_control_address(user_click_control_address);
    fibconfig().set_user_click_control_address(user_click_control_address);

    return XrlCmdError::OKAY();
}

/**
 *  Specify the socket port to use for control access to the user-level
 *  Click.
 *
 *  @param user_click_control_socket_port the socket port to use for
 *  control access to the user-level Click.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_set_user_click_control_socket_port(
    // Input values,
    const uint32_t&	user_click_control_socket_port)
{
    ifconfig().set_user_click_control_socket_port(
	user_click_control_socket_port);
    fibconfig().set_user_click_control_socket_port(
	user_click_control_socket_port);

    return XrlCmdError::OKAY();
}

/**
 *  Specify the configuration file to be used by user-level Click on
 *  startup.
 *
 *  @param user_click_startup_config_file the name of the configuration
 *  file to be used by user-level Click on startup.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_set_user_click_startup_config_file(
    // Input values,
    const string&	user_click_startup_config_file)
{
    ifconfig().set_user_click_startup_config_file(
	user_click_startup_config_file);
    fibconfig().set_user_click_startup_config_file(
	user_click_startup_config_file);

    return XrlCmdError::OKAY();
}

/**
 *  Specify the external program to generate the user-level Click
 *  configuration.
 *
 *  @param user_click_config_generator_file the name of the external
 *  program to generate the user-level Click configuration.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_set_user_click_config_generator_file(
    // Input values,
    const string&	user_click_config_generator_file)
{
    ifconfig().set_user_click_config_generator_file(
	user_click_config_generator_file);
    fibconfig().set_user_click_config_generator_file(
	user_click_config_generator_file);

    return XrlCmdError::OKAY();
}

/**
 *  Add a FIB client.
 *
 *  @param target_name the target name of the FIB client to add.
 */
XrlCmdError
XrlFeaTarget::fea_fib_0_1_add_fib_client4(
    // Input values,
    const string&	client_target_name,
    const bool&		send_updates,
    const bool&		send_resolves)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    return _xrl_fib_client_manager.add_fib_client4(client_target_name,
						   send_updates,
						   send_resolves);
}

XrlCmdError
XrlFeaTarget::fea_fib_0_1_add_fib_client6(
    // Input values,
    const string&	client_target_name,
    const bool&		send_updates,
    const bool&		send_resolves)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    return _xrl_fib_client_manager.add_fib_client6(client_target_name,
						   send_updates,
						   send_resolves);
}

/**
 *  Delete a FIB client.
 *
 *  @param target_name the target name of the FIB client to delete.
 */
XrlCmdError
XrlFeaTarget::fea_fib_0_1_delete_fib_client4(
    // Input values,
    const string&	client_target_name)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    return _xrl_fib_client_manager.delete_fib_client4(client_target_name);
}

XrlCmdError
XrlFeaTarget::fea_fib_0_1_delete_fib_client6(
    // Input values,
    const string&	client_target_name)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    return _xrl_fib_client_manager.delete_fib_client6(client_target_name);
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_restore_original_config_on_shutdown(
    // Input values,
    const bool&	enable)
{
    ifconfig().set_restore_original_config_on_shutdown(enable);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_interface_names(
    // Output values,
    XrlAtomList&	ifnames)
{
    const IfTree& iftree = ifconfig().local_config();

    for (IfTree::IfMap::const_iterator ii = iftree.interfaces().begin();
	 ii != iftree.interfaces().end(); ++ii) {
	ifnames.append(XrlAtom(ii->second.ifname()));
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_vif_names(
    const string& ifname,
    // Output values,
    XrlAtomList&  vifnames)
{
    const IfTreeInterface* ifp = NULL;
    string error_msg;

    ifp = ifconfig().local_config().find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Interface %s not found", ifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    for (IfTreeInterface::VifMap::const_iterator vi = ifp->vifs().begin();
	 vi != ifp->vifs().end(); ++vi) {
	vifnames.append(XrlAtom(vi->second.vifname()));
    }

    return  XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_vif_flags(
    // Input values,
    const string&	ifname,
    const string&	vifname,
    // Output values,
    bool&		enabled,
    bool&		broadcast,
    bool&		loopback,
    bool&		point_to_point,
    bool&		multicast)
{
    const IfTreeVif* vifp = NULL;
    string error_msg;

    vifp = ifconfig().local_config().find_vif(ifname, vifname);
    if (vifp == NULL) {
	error_msg = c_format("Interface %s vif %s not found",
			     ifname.c_str(), vifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    enabled = vifp->enabled();
    broadcast = vifp->broadcast();
    loopback = vifp->loopback();
    point_to_point = vifp->point_to_point();
    multicast = vifp->multicast();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_vif_pif_index(
    // Input values,
    const string&	ifname,
    const string&	vifname,
    // Output values,
    uint32_t&		pif_index)
{
    const IfTreeVif* vifp = NULL;
    string error_msg;

    vifp = ifconfig().local_config().find_vif(ifname, vifname);
    if (vifp == NULL) {
	error_msg = c_format("Interface %s vif %s not found",
			     ifname.c_str(), vifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    pif_index = vifp->pif_index();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_interface_enabled(
    // Input values,
    const string&	ifname,
    // Output values,
    bool&		enabled)
{
    const IfTreeInterface* ifp = NULL;
    string error_msg;

    ifp = ifconfig().local_config().find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Interface %s not found", ifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    enabled = ifp->enabled();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_interface_discard(
    // Input values,
    const string&	ifname,
    // Output values,
    bool&		discard)
{
    const IfTreeInterface* ifp = NULL;
    string error_msg;

    ifp = ifconfig().local_config().find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Interface %s not found", ifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    discard = ifp->discard();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_mac(
    // Input values,
    const string& ifname,
    Mac&	    mac)
{
    const IfTreeInterface* ifp = NULL;
    string error_msg;

    ifp = ifconfig().local_config().find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Interface %s not found", ifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    mac = ifp->mac();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_mtu(
    // Input values,
    const string&	ifname,
    uint32_t&	mtu)
{
    const IfTreeInterface* ifp = NULL;
    string error_msg;

    ifp = ifconfig().local_config().find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Interface %s not found", ifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    mtu = ifp->mtu();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_no_carrier(
    // Input values,
    const string&	ifname,
    bool&		no_carrier)
{
    const IfTreeInterface* ifp = NULL;
    string error_msg;

    ifp = ifconfig().local_config().find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Interface %s not found", ifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    no_carrier = ifp->no_carrier();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_vif_enabled(
    // Input values,
    const string&	ifname,
    const string&	vifname,
    // Output values,
    bool&		enabled)
{
    const IfTreeVif* vifp = NULL;
    string error_msg;

    vifp = ifconfig().local_config().find_vif(ifname, vifname);
    if (vifp == NULL) {
	error_msg = c_format("Interface %s vif %s not found",
			     ifname.c_str(), vifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    enabled = vifp->enabled();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_prefix4(
    // Input values,
    const string&	ifname,
    const string&	vifname,
    const IPv4&		address,
    // Output values,
    uint32_t&		prefix_len)
{
    const IfTreeAddr4* ap = NULL;
    string error_msg;

    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    ap = ifconfig().local_config().find_addr(ifname, vifname, address);
    if (ap == NULL) {
	error_msg = c_format("Interface %s vif %s address %s not found",
			     ifname.c_str(), vifname.c_str(),
			     address.str().c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    prefix_len = ap->prefix_len();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_broadcast4(
    // Input values,
    const string&	ifname,
    const string&	vifname,
    const IPv4&		address,
    // Output values,
    IPv4&		broadcast)
{
    const IfTreeAddr4* ap = NULL;
    string error_msg;

    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    ap = ifconfig().local_config().find_addr(ifname, vifname, address);
    if (ap == NULL) {
	error_msg = c_format("Interface %s vif %s address %s not found",
			     ifname.c_str(), vifname.c_str(),
			     address.str().c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    broadcast = ap->bcast();
    if ((! ap->broadcast()) || (broadcast == IPv4::ZERO())) {
	error_msg = c_format("No broadcast address associated with "
			     "interface %s vif %s address %s",
			     ifname.c_str(), vifname.c_str(),
			     address.str().c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_endpoint4(
    // Input values,
    const string&	ifname,
    const string&	vifname,
    const IPv4&		address,
    // Output values,
    IPv4&		endpoint)
{
    const IfTreeAddr4* ap = NULL;
    string error_msg;

    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    ap = ifconfig().local_config().find_addr(ifname, vifname, address);
    if (ap == NULL) {
	error_msg = c_format("Interface %s vif %s address %s not found",
			     ifname.c_str(), vifname.c_str(),
			     address.str().c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    endpoint = ap->endpoint();
    if ((! ap->point_to_point()) || (endpoint == IPv4::ZERO())) {
	error_msg = c_format("No endpoint address associated with "
			     "interface %s vif %s address %s",
			     ifname.c_str(), vifname.c_str(),
			     address.str().c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);

    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_vif_addresses4(
    // Input values,
    const string&	ifname,
    const string&	vifname,
    // Output values,
    XrlAtomList&	addresses)
{
    const IfTreeVif* vifp = NULL;
    string error_msg;

    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    vifp = ifconfig().local_config().find_vif(ifname, vifname);
    if (vifp == NULL) {
	error_msg = c_format("Interface %s vif %s not found",
			     ifname.c_str(), vifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    for (IfTreeVif::IPv4Map::const_iterator ai = vifp->ipv4addrs().begin();
	 ai != vifp->ipv4addrs().end(); ++ai) {
	addresses.append(XrlAtom(ai->second.addr()));
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_prefix6(
    // Input values,
    const string&	ifname,
    const string&	vifname,
    const IPv6&		address,
    // Output values,
    uint32_t&		prefix_len)
{
    const IfTreeAddr6* ap = NULL;
    string error_msg;

    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    ap = ifconfig().local_config().find_addr(ifname, vifname, address);
    if (ap == NULL) {
	error_msg = c_format("Interface %s vif %s address %s not found",
			     ifname.c_str(), vifname.c_str(),
			     address.str().c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    prefix_len = ap->prefix_len();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_endpoint6(
    // Input values,
    const string&	ifname,
    const string&	vifname,
    const IPv6&		address,
    // Output values,
    IPv6&		endpoint)
{
    const IfTreeAddr6* ap = NULL;
    string error_msg;

    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    ap = ifconfig().local_config().find_addr(ifname, vifname, address);
    if (ap == NULL) {
	error_msg = c_format("Interface %s vif %s address %s not found",
			     ifname.c_str(), vifname.c_str(),
			     address.str().c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    endpoint = ap->endpoint();

    if ((! ap->point_to_point()) || (endpoint == IPv6::ZERO())) {
	error_msg = c_format("No endpoint address associated with "
			     "interface %s vif %s address %s",
			     ifname.c_str(), vifname.c_str(),
			     address.str().c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_vif_addresses6(
    // Input values,
    const string&	ifname,
    const string&	vifname,
    // Output values,
    XrlAtomList&	addresses)
{
    const IfTreeVif* vifp = NULL;
    string error_msg;

    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    vifp = ifconfig().local_config().find_vif(ifname, vifname);
    if (vifp == NULL) {
	error_msg = c_format("Interface %s vif %s not found",
			     ifname.c_str(), vifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    for (IfTreeVif::IPv6Map::const_iterator ai = vifp->ipv6addrs().begin();
	 ai != vifp->ipv6addrs().end(); ++ai) {
	addresses.append(XrlAtom(ai->second.addr()));
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_address_flags4(
    // Input values
    const string&	ifname,
    const string&	vifname,
    const IPv4&		address,
    // Output values
    bool&		up,
    bool&		broadcast,
    bool&		loopback,
    bool&		point_to_point,
    bool&		multicast)
{
    const IfTreeAddr4* ap = NULL;
    string error_msg;

    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    ap = ifconfig().local_config().find_addr(ifname, vifname, address);
    if (ap == NULL) {
	error_msg = c_format("Interface %s vif %s address %s not found",
			     ifname.c_str(), vifname.c_str(),
			     address.str().c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    up = ap->enabled();
    broadcast = ap->broadcast();
    loopback = ap->loopback();
    point_to_point = ap->point_to_point();
    multicast = ap->multicast();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_address_flags6(
    // Input values
    const string&	ifname,
    const string&	vifname,
    const IPv6&		address,
    // Output values
    bool&		up,
    bool&		loopback,
    bool&		point_to_point,
    bool&		multicast)
{
    const IfTreeAddr6* ap = NULL;
    string error_msg;

    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    ap = ifconfig().local_config().find_addr(ifname, vifname, address);
    if (ap == NULL) {
	error_msg = c_format("Interface %s vif %s address %s not found",
			     ifname.c_str(), vifname.c_str(),
			     address.str().c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    up = ap->enabled();
    loopback = ap->loopback();
    point_to_point = ap->point_to_point();
    multicast = ap->multicast();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_address_enabled4(
    // Input values,
    const string&	ifname,
    const string&	vifname,
    const IPv4&		address,
    bool&		enabled)
{
    const IfTreeAddr4* ap = NULL;
    string error_msg;

    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    ap = ifconfig().local_config().find_addr(ifname, vifname, address);
    if (ap == NULL) {
	error_msg = c_format("Interface %s vif %s address %s not found",
			     ifname.c_str(), vifname.c_str(),
			     address.str().c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    enabled = ap->enabled();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_address_enabled6(
    // Input values,
    const string&	ifname,
    const string&	vifname,
    const IPv6&		address,
    bool&		enabled)
{
    const IfTreeAddr6* ap = NULL;
    string error_msg;

    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    ap = ifconfig().local_config().find_addr(ifname, vifname, address);
    if (ap == NULL) {
	error_msg = c_format("Interface %s vif %s address %s not found",
			     ifname.c_str(), vifname.c_str(),
			     address.str().c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    enabled = ap->enabled();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_start_transaction(
    // Output values,
    uint32_t&	tid)
{
    string error_msg;

    if (ifconfig().start_transaction(tid, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_commit_transaction(
    // Input values,
    const uint32_t&	tid)
{
    string error_msg;

    if (ifconfig().commit_transaction(tid, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_abort_transaction(
    // Input values,
    const uint32_t&	tid)
{
    string error_msg;

    if (ifconfig().abort_transaction(tid, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_create_interface(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (ifconfig().add_transaction_operation(
	    tid,
	    new AddInterface(iftree, ifname),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_delete_interface(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (ifconfig().add_transaction_operation(
	    tid,
	    new RemoveInterface(iftree, ifname),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_configure_interface_from_system(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (ifconfig().add_transaction_operation(
	    tid,
	    new ConfigureInterfaceFromSystem(ifconfig(), iftree, ifname),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_interface_enabled(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const bool&		enabled)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (ifconfig().add_transaction_operation(
	    tid,
	    new SetInterfaceEnabled(iftree, ifname, enabled),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_interface_discard(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const bool&		discard)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (ifconfig().add_transaction_operation(
	    tid,
	    new SetInterfaceDiscard(iftree, ifname, discard),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_mac(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const Mac&		mac)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (ifconfig().add_transaction_operation(
	    tid,
	    new SetInterfaceMAC(iftree, ifname, mac),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_restore_original_mac(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname)
{
    IfTree& iftree = ifconfig().local_config();
    const IfTree& original_iftree = ifconfig().original_config();
    const IfTreeInterface* ifp = NULL;
    string error_msg;

    // Find the original MAC address
    ifp = original_iftree.find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Interface %s not found", ifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    const Mac& mac = ifp->mac();

    if (ifconfig().add_transaction_operation(
	    tid,
	    new SetInterfaceMAC(iftree, ifname, mac),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_mtu(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const uint32_t&	mtu)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (ifconfig().add_transaction_operation(
	    tid,
	    new SetInterfaceMTU(iftree, ifname, mtu),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_restore_original_mtu(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname)
{
    IfTree& iftree = ifconfig().local_config();
    const IfTree& original_iftree = ifconfig().original_config();
    const IfTreeInterface* ifp = NULL;
    string error_msg;

    // Find the original MTU
    ifp = original_iftree.find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Interface %s not found", ifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    uint32_t mtu = ifp->mtu();

    if (ifconfig().add_transaction_operation(
	    tid,
	    new SetInterfaceMTU(iftree, ifname, mtu),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_create_vif(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const string&	vifname)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (ifconfig().add_transaction_operation(
	    tid,
	    new AddInterfaceVif(iftree, ifname, vifname),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_delete_vif(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const string&	vifname)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (ifconfig().add_transaction_operation(
	    tid,
	    new RemoveInterfaceVif(iftree, ifname, vifname),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_vif_enabled(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const string&	vifname,
    const bool&		enabled)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (ifconfig().add_transaction_operation(
	    tid,
	    new SetVifEnabled(iftree, ifname, vifname, enabled),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_create_address4(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const string&	vifname,
    const IPv4&		address)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    if (ifconfig().add_transaction_operation(
	    tid,
	    new AddAddr4(iftree, ifname, vifname, address),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_delete_address4(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const string&	vifname,
    const IPv4&		address)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    if (ifconfig().add_transaction_operation(
	    tid,
	    new RemoveAddr4(iftree, ifname, vifname, address),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_address_enabled4(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const string&	vifname,
    const IPv4&		address,
    const bool&		enabled)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    if (ifconfig().add_transaction_operation(
	    tid,
	    new SetAddr4Enabled(iftree, ifname, vifname, address, enabled),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_prefix4(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const string&	vifname,
    const IPv4&		address,
    const uint32_t&	prefix_len)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    if (ifconfig().add_transaction_operation(
	    tid,
	    new SetAddr4Prefix(iftree, ifname, vifname, address, prefix_len),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_broadcast4(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const string&	vifname,
    const IPv4&		address,
    const IPv4&		broadcast)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    if (ifconfig().add_transaction_operation(
	    tid,
	    new SetAddr4Broadcast(iftree, ifname, vifname, address, broadcast),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_endpoint4(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const string&	vifname,
    const IPv4&		address,
    const IPv4&		endpoint)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    if (ifconfig().add_transaction_operation(
	    tid,
	    new SetAddr4Endpoint(iftree, ifname, vifname, address, endpoint),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_create_address6(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const string&	vifname,
    const IPv6&		address)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    if (ifconfig().add_transaction_operation(
	    tid,
	    new AddAddr6(iftree, ifname, vifname, address),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_delete_address6(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const string&	vifname,
    const IPv6&		address)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    if (ifconfig().add_transaction_operation(
	    tid,
	    new RemoveAddr6(iftree, ifname, vifname, address),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_address_enabled6(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const string&	vifname,
    const IPv6&		address,
    const bool&		enabled)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    if (ifconfig().add_transaction_operation(
	    tid,
	    new SetAddr6Enabled(iftree, ifname, vifname, address, enabled),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_prefix6(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const string&	vifname,
    const IPv6&	  	address,
    const uint32_t&	prefix_len)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    if (ifconfig().add_transaction_operation(
	    tid,
	    new SetAddr6Prefix(iftree, ifname, vifname, address, prefix_len),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_endpoint6(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const string&	vifname,
    const IPv6&		address,
    const IPv6&		endpoint)
{
    IfTree& iftree = ifconfig().local_config();
    string error_msg;

    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    if (ifconfig().add_transaction_operation(
	    tid,
	    new SetAddr6Endpoint(iftree, ifname, vifname, address, endpoint),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_replicator_0_1_register_ifmgr_mirror(
	const string& clientname
	)
{
    if (_lib_fea_client_bridge.add_libfeaclient_mirror(clientname) == false) {
	return XrlCmdError::COMMAND_FAILED();
    }
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_replicator_0_1_unregister_ifmgr_mirror(
	const string& clientname
	)
{
    if (_lib_fea_client_bridge.remove_libfeaclient_mirror(clientname) == false) {
	return XrlCmdError::COMMAND_FAILED();
    }
    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// FIB Related

XrlCmdError
XrlFeaTarget::fti_0_2_lookup_route_by_dest4(
    // Input values,
    const IPv4&		dst,
    // Output values,
    IPv4Net&		netmask,
    IPv4&		nexthop,
    string&		ifname,
    string&		vifname,
    uint32_t&		metric,
    uint32_t&		admin_distance,
    string&		protocol_origin)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    Fte4 fte;
    if (fibconfig().lookup_route_by_dest4(dst, fte) == true) {
	netmask = fte.net();
	nexthop = fte.nexthop();
	ifname = fte.ifname();
	vifname = fte.vifname();
	metric = fte.metric();
	admin_distance = fte.admin_distance();
	// TODO: set the value of protocol_origin to something meaningful
	protocol_origin = "NOT_SUPPORTED";
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED("No route for " + dst.str());
}

XrlCmdError
XrlFeaTarget::fti_0_2_lookup_route_by_dest6(
    // Input values,
    const IPv6&		dst,
    // Output values,
    IPv6Net&		netmask,
    IPv6&		nexthop,
    string&		ifname,
    string&		vifname,
    uint32_t&		metric,
    uint32_t&		admin_distance,
    string&		protocol_origin)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    Fte6 fte;
    if (fibconfig().lookup_route_by_dest6(dst, fte) == true) {
	netmask = fte.net();
	nexthop = fte.nexthop();
	ifname = fte.ifname();
	vifname = fte.vifname();
	metric = fte.metric();
	admin_distance = fte.admin_distance();
	// TODO: set the value of protocol_origin to something meaningful
	protocol_origin = "NOT_SUPPORTED";
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED("No route for " + dst.str());
}

XrlCmdError
XrlFeaTarget::fti_0_2_lookup_route_by_network4(
    // Input values,
    const IPv4Net&	dst,
    // Output values,
    IPv4&		nexthop,
    string&		ifname,
    string&		vifname,
    uint32_t&		metric,
    uint32_t&		admin_distance,
    string&		protocol_origin)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    Fte4 fte;
    if (fibconfig().lookup_route_by_network4(dst, fte)) {
	nexthop = fte.nexthop();
	ifname = fte.ifname();
	vifname = fte.vifname();
	metric = fte.metric();
	admin_distance = fte.admin_distance();
	// TODO: set the value of protocol_origin to something meaningful
	protocol_origin = "NOT_SUPPORTED";
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED("No entry for " + dst.str());
}

XrlCmdError
XrlFeaTarget::fti_0_2_lookup_route_by_network6(
    // Input values,
    const IPv6Net&	dst,
    // Output values,
    IPv6&		nexthop,
    string&		ifname,
    string&		vifname,
    uint32_t&		metric,
    uint32_t&		admin_distance,
    string&		protocol_origin)
{
    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    Fte6 fte;
    if (fibconfig().lookup_route_by_network6(dst, fte)) {
	nexthop = fte.nexthop();
	ifname = fte.ifname();
	vifname = fte.vifname();
	metric = fte.metric();
	admin_distance = fte.admin_distance();
	// TODO: set the value of protocol_origin to something meaningful
	protocol_origin = "NOT_SUPPORTED";
	return XrlCmdError::OKAY();
    }
    return XrlCmdError::COMMAND_FAILED("No entry for " + dst.str());
}

XrlCmdError
XrlFeaTarget::fti_0_2_have_ipv4(
    // Output values,
    bool&	result)
{
    result = fibconfig().have_ipv4();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_have_ipv6(
    // Output values,
    bool&	result)
{
    result = fibconfig().have_ipv6();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_get_unicast_forwarding_enabled4(
    // Output values,
    bool&	enabled)
{
    string error_msg;

    if (fibconfig().unicast_forwarding_enabled4(enabled, error_msg) < 0)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_get_unicast_forwarding_enabled6(
    // Output values,
    bool&	enabled)
{
    string error_msg;

    if (fibconfig().unicast_forwarding_enabled6(enabled, error_msg) < 0)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_set_unicast_forwarding_enabled4(
    // Input values,
    const bool&	enabled)
{
    string error_msg;

    if (fibconfig().set_unicast_forwarding_enabled4(enabled, error_msg) < 0)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_set_unicast_forwarding_enabled6(
    // Input values,
    const bool&	enabled)
{
    string error_msg;

    if (fibconfig().set_unicast_forwarding_enabled6(enabled, error_msg) < 0)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_set_unicast_forwarding_entries_retain_on_startup4(
    // Input values,
    const bool&	retain)
{
    string error_msg;

    if (fibconfig().set_unicast_forwarding_entries_retain_on_startup4(
	    retain,
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_set_unicast_forwarding_entries_retain_on_shutdown4(
    // Input values,
    const bool&	retain)
{
    string error_msg;

    if (fibconfig().set_unicast_forwarding_entries_retain_on_shutdown4(
	    retain,
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_set_unicast_forwarding_entries_retain_on_startup6(
    // Input values,
    const bool&	retain)
{
    string error_msg;

    if (fibconfig().set_unicast_forwarding_entries_retain_on_startup6(
	    retain,
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_set_unicast_forwarding_entries_retain_on_shutdown6(
    // Input values,
    const bool&	retain)
{
    string error_msg;

    if (fibconfig().set_unicast_forwarding_entries_retain_on_shutdown6(
	    retain,
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

//
// RIB routes redistribution transaction-based XRL interface
//

XrlCmdError
XrlFeaTarget::redist_transaction4_0_1_start_transaction(
    // Output values,
    uint32_t&	tid)
{
    string error_msg;

    if (fibconfig().start_transaction(tid, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::redist_transaction4_0_1_commit_transaction(
    // Input values,
    const uint32_t&	tid)
{
    string error_msg;

    if (fibconfig().commit_transaction(tid, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::redist_transaction4_0_1_abort_transaction(
    // Input values,
    const uint32_t&	tid)
{
    string error_msg;

    if (fibconfig().abort_transaction(tid, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::redist_transaction4_0_1_add_route(
    // Input values,
    const uint32_t&	tid,
    const IPv4Net&	dst,
    const IPv4&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	cookie,
    const string&	protocol_origin)
{
    bool is_xorp_route;
    bool is_connected_route = false;
    string error_msg;

    debug_msg("redist_transaction4_0_1_add_route(): "
	      "dst = %s nexthop = %s ifname = %s vifname = %s "
	      "metric = %u admin_distance = %u protocol_origin = %s\n",
	      dst.str().c_str(),
	      nexthop.str().c_str(),
	      ifname.c_str(),
	      vifname.c_str(),
	      XORP_UINT_CAST(metric),
	      XORP_UINT_CAST(admin_distance),
	      protocol_origin.c_str());

    UNUSED(cookie);

    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    is_xorp_route = true;	// XXX: unconditionally set to true

    // TODO: XXX: get rid of the hard-coded "connected" string here
    if (protocol_origin == "connected")
	is_connected_route = true;

    if (_profile.enabled(profile_route_in))
	_profile.log(profile_route_in, c_format("add %s", dst.str().c_str()));

    if (fibconfig().add_transaction_operation(
	    tid,
	    new FibAddEntry4(fibconfig(), dst, nexthop, ifname, vifname,
			     metric, admin_distance, is_xorp_route,
			     is_connected_route),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::redist_transaction4_0_1_delete_route(
    // Input values,
    const uint32_t&	tid,
    const IPv4Net&	dst,
    const IPv4&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	cookie,
    const string&	protocol_origin)
{
    bool is_xorp_route;
    bool is_connected_route = false;
    string error_msg;

    debug_msg("redist_transaction4_0_1_delete_route(): "
	      "dst = %s nexthop = %s ifname = %s vifname = %s "
	      "metric = %u admin_distance = %u protocol_origin = %s\n",
	      dst.str().c_str(),
	      nexthop.str().c_str(),
	      ifname.c_str(),
	      vifname.c_str(),
	      XORP_UINT_CAST(metric),
	      XORP_UINT_CAST(admin_distance),
	      protocol_origin.c_str());

    UNUSED(cookie);

    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    is_xorp_route = true;	// XXX: unconditionally set to true

    // TODO: XXX: get rid of the hard-coded "connected" string here
    if (protocol_origin == "connected")
	is_connected_route = true;

    if (_profile.enabled(profile_route_in))
	_profile.log(profile_route_in,
		     c_format("delete %s", dst.str().c_str()));

    if (fibconfig().add_transaction_operation(
	    tid,
	    new FibDeleteEntry4(fibconfig(), dst, nexthop, ifname, vifname,
				metric, admin_distance, is_xorp_route,
				is_connected_route),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::redist_transaction4_0_1_delete_all_routes(
    // Input values,
    const uint32_t&	tid,
    const string&	cookie)
{
    string error_msg;

    UNUSED(cookie);

    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    if (fibconfig().add_transaction_operation(
	    tid,
	    new FibDeleteAllEntries4(fibconfig()),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::redist_transaction6_0_1_start_transaction(
    // Output values,
    uint32_t&	tid)
{
    string error_msg;

    if (fibconfig().start_transaction(tid, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::redist_transaction6_0_1_commit_transaction(
    // Input values,
    const uint32_t&	tid)
{
    string error_msg;

    if (fibconfig().commit_transaction(tid, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::redist_transaction6_0_1_abort_transaction(
    // Input values,
    const uint32_t&	tid)
{
    string error_msg;

    if (fibconfig().abort_transaction(tid, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::redist_transaction6_0_1_add_route(
    // Input values,
    const uint32_t&	tid,
    const IPv6Net&	dst,
    const IPv6&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	cookie,
    const string&	protocol_origin)
{
    bool is_xorp_route;
    bool is_connected_route = false;
    string error_msg;

    debug_msg("redist_transaction6_0_1_add_route(): "
	      "dst = %s nexthop = %s ifname = %s vifname = %s "
	      "metric = %u admin_distance = %u protocol_origin = %s\n",
	      dst.str().c_str(),
	      nexthop.str().c_str(),
	      ifname.c_str(),
	      vifname.c_str(),
	      XORP_UINT_CAST(metric),
	      XORP_UINT_CAST(admin_distance),
	      protocol_origin.c_str());

    UNUSED(cookie);

    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    is_xorp_route = true;	// XXX: unconditionally set to true

    // TODO: XXX: get rid of the hard-coded "connected" string here
    if (protocol_origin == "connected")
	is_connected_route = true;

    if (_profile.enabled(profile_route_in))
	_profile.log(profile_route_in, c_format("add %s", dst.str().c_str()));

    if (fibconfig().add_transaction_operation(
	    tid,
	    new FibAddEntry6(fibconfig(), dst, nexthop, ifname, vifname,
			     metric, admin_distance, is_xorp_route,
			     is_connected_route),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::redist_transaction6_0_1_delete_route(
    // Input values,
    const uint32_t&	tid,
    const IPv6Net&	dst,
    const IPv6&		nexthop,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	metric,
    const uint32_t&	admin_distance,
    const string&	cookie,
    const string&	protocol_origin)
{
    bool is_xorp_route;
    bool is_connected_route = false;
    string error_msg;

    debug_msg("redist_transaction6_0_1_delete_route(): "
	      "dst = %s nexthop = %s ifname = %s vifname = %s "
	      "metric = %u admin_distance = %u protocol_origin = %s\n",
	      dst.str().c_str(),
	      nexthop.str().c_str(),
	      ifname.c_str(),
	      vifname.c_str(),
	      XORP_UINT_CAST(metric),
	      XORP_UINT_CAST(admin_distance),
	      protocol_origin.c_str());

    UNUSED(cookie);

    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    is_xorp_route = true;	// XXX: unconditionally set to true

    // TODO: XXX: get rid of the hard-coded "connected" string here
    if (protocol_origin == "connected")
	is_connected_route = true;

    if (_profile.enabled(profile_route_in))
	_profile.log(profile_route_in,
		     c_format("delete %s", dst.str().c_str()));

    if (fibconfig().add_transaction_operation(
	    tid,
	    new FibDeleteEntry6(fibconfig(), dst, nexthop, ifname, vifname,
				metric, admin_distance, is_xorp_route,
				is_connected_route),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::redist_transaction6_0_1_delete_all_routes(
    // Input values,
    const uint32_t&	tid,
    const string&	cookie)
{
    string error_msg;

    UNUSED(cookie);

    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    if (fibconfig().add_transaction_operation(
	    tid,
	    new FibDeleteAllEntries6(fibconfig()),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// IPv4 Raw Socket related

XrlCmdError
XrlFeaTarget::raw_packet4_0_1_send(
    // Input values,
    const string&		if_name,
    const string&		vif_name,
    const IPv4&			src_address,
    const IPv4&			dst_address,
    const uint32_t&		ip_protocol,
    const int32_t&		ip_ttl,
    const int32_t&		ip_tos,
    const bool&			ip_router_alert,
    const vector<uint8_t>&	payload)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    return _xrsm4.send(if_name, vif_name, src_address, dst_address,
		       ip_protocol, ip_ttl, ip_tos, ip_router_alert,
		       payload);
}

XrlCmdError
XrlFeaTarget::raw_packet4_0_1_register_receiver(
    // Input values,
    const string&	xrl_target_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ip_protocol,
    const bool&		enable_multicast_loopback)
{
    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    return _xrsm4.register_receiver(xrl_target_name, if_name, vif_name,
				    ip_protocol, enable_multicast_loopback);
}

XrlCmdError
XrlFeaTarget::raw_packet4_0_1_unregister_receiver(
    // Input values,
    const string&	xrl_target_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ip_protocol)
{
    return _xrsm4.unregister_receiver(xrl_target_name, if_name, vif_name,
				      ip_protocol);
}

XrlCmdError
XrlFeaTarget::raw_packet4_0_1_join_multicast_group(
    // Input values,
    const string&	xrl_target_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ip_protocol,
    const IPv4&		group_address)
{
    return _xrsm4.join_multicast_group(xrl_target_name, if_name, vif_name,
				       ip_protocol, group_address);
}

XrlCmdError
XrlFeaTarget::raw_packet4_0_1_leave_multicast_group(
    // Input values,
    const string&	xrl_target_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ip_protocol,
    const IPv4&		group_address)
{
    return _xrsm4.leave_multicast_group(xrl_target_name, if_name, vif_name,
					ip_protocol, group_address);
}

// ----------------------------------------------------------------------------
// IPv6 Raw Socket related

static const string XRL_RAW_SOCKET6_NULL = "XrlRawSocket6Manager not present";

XrlCmdError
XrlFeaTarget::raw_packet6_0_1_send(
    // Input values,
    const string&	if_name,
    const string&	vif_name,
    const IPv6&		src_address,
    const IPv6&		dst_address,
    const uint32_t&	ip_protocol,
    const int32_t&	ip_ttl,
    const int32_t&	ip_tos,
    const bool&		ip_router_alert,
    const XrlAtomList&	ext_headers_type,
    const XrlAtomList&	ext_headers_payload,
    const vector<uint8_t>&	payload)
{
    string error_msg;

    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    // Decompose the external headers info
    if (ext_headers_type.size() != ext_headers_payload.size()) {
	error_msg = c_format("External headers mismatch: %u type(s) "
			     "and %u payload(s)",
			     XORP_UINT_CAST(ext_headers_type.size()),
			     XORP_UINT_CAST(ext_headers_payload.size()));
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    size_t i;
    size_t ext_headers_size = ext_headers_type.size();
    vector<uint8_t> ext_headers_type_vector(ext_headers_size);
    vector<vector<uint8_t> > ext_headers_payload_vector(ext_headers_size);
    for (i = 0; i < ext_headers_size; i++) {
	const XrlAtom& atom_type = ext_headers_type.get(i);
	const XrlAtom& atom_payload = ext_headers_payload.get(i);
	if (atom_type.type() != xrlatom_uint32) {
	    error_msg = c_format("Element inside ext_headers_type isn't uint32");
	    return XrlCmdError::COMMAND_FAILED(error_msg);
	}
	if (atom_payload.type() != xrlatom_binary) {
	    error_msg = c_format("Element inside ext_headers_payload isn't binary");
	    return XrlCmdError::COMMAND_FAILED(error_msg);
	}
	ext_headers_type_vector[i] = atom_type.uint32();
	ext_headers_payload_vector[i] = atom_payload.binary();
    }

    return _xrsm6.send(if_name, vif_name, src_address, dst_address,
		       ip_protocol, ip_ttl, ip_tos, ip_router_alert,
		       ext_headers_type_vector, ext_headers_payload_vector,
		       payload);
}

XrlCmdError
XrlFeaTarget::raw_packet6_0_1_register_receiver(
    // Input values,
    const string&	xrl_target_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ip_protocol,
    const bool&		enable_multicast_loopback)
{
    return _xrsm6.register_receiver(xrl_target_name, if_name, vif_name,
				    ip_protocol, enable_multicast_loopback);
}

XrlCmdError
XrlFeaTarget::raw_packet6_0_1_unregister_receiver(
    // Input values,
    const string&	xrl_target_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ip_protocol)
{
    return _xrsm6.unregister_receiver(xrl_target_name, if_name, vif_name,
				      ip_protocol);
}

XrlCmdError
XrlFeaTarget::raw_packet6_0_1_join_multicast_group(
    // Input values,
    const string&	xrl_target_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ip_protocol,
    const IPv6&		group_address)
{
    return _xrsm6.join_multicast_group(xrl_target_name, if_name, vif_name,
				       ip_protocol, group_address);
}

XrlCmdError
XrlFeaTarget::raw_packet6_0_1_leave_multicast_group(
    // Input values,
    const string&	xrl_target_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ip_protocol,
    const IPv6&		group_address)
{
    return _xrsm6.leave_multicast_group(xrl_target_name, if_name, vif_name,
					ip_protocol, group_address);
}


// ----------------------------------------------------------------------------
// Socket Server related

XrlCmdError
XrlFeaTarget::socket4_locator_0_1_find_socket_server_for_addr(
							      const IPv4& addr,
							      string&	  svr
							      )
{
    UNUSED(addr);

    if (! have_ipv4())
	return XrlCmdError::COMMAND_FAILED("IPv4 is not available");

    // If we had multiple socket servers we'd look for the right one
    // to use.  At the present time we only have one so this is the
    // one to return
    if (_xrl_socket_server.status() != SERVICE_RUNNING) {
	return XrlCmdError::COMMAND_FAILED("Socket Server not running.");
    }
    svr = _xrl_socket_server.instance_name();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket6_locator_0_1_find_socket_server_for_addr(
							      const IPv6& addr,
							      string&	  svr
							      )
{
    UNUSED(addr);

    if (! have_ipv6())
	return XrlCmdError::COMMAND_FAILED("IPv6 is not available");

    // If we had multiple socket servers we'd look for the right one
    // to use.  At the present time we only have one so this is the
    // one to return

    if (_xrl_socket_server.status() != SERVICE_RUNNING) {
	return XrlCmdError::COMMAND_FAILED("Socket Server not running.");
    }

    svr = _xrl_socket_server.instance_name();
    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// Profiling related

XrlCmdError
XrlFeaTarget::profile_0_1_enable(const string& pname)
{
    debug_msg("enable profile variable %s\n", pname.c_str());

    try {
	_profile.enable(pname);
    } catch(PVariableUnknown& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    } catch(PVariableLocked& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::profile_0_1_disable(const string&	pname)
{
    debug_msg("disable profile variable %s\n", pname.c_str());

    try {
	_profile.disable(pname);
    } catch(PVariableUnknown& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlFeaTarget::profile_0_1_get_entries(const string& pname,
				      const string& instance_name)
{
    debug_msg("get profile variable %s instance %s\n", pname.c_str(),
	      instance_name.c_str());

    // Lock and initialize.
    try {
	_profile.lock_log(pname);
    } catch(PVariableUnknown& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    } catch(PVariableLocked& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    ProfileUtils::transmit_log(pname,
			       dynamic_cast<XrlStdRouter *>(&_xrl_router),
			       instance_name, &_profile);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::profile_0_1_clear(const string& pname)
{
    debug_msg("clear profile variable %s\n", pname.c_str());

    try {
	_profile.clear(pname);
    } catch(PVariableUnknown& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    } catch(PVariableLocked& e) {
	return XrlCmdError::COMMAND_FAILED(e.str());
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::profile_0_1_list(string& info)
{
    debug_msg("\n");
    
    info = _profile.list();
    return XrlCmdError::OKAY();
}
