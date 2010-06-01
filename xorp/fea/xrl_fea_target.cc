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
#include "libproto/packet.hh"

#include "libxipc/xrl_std_router.hh"

#ifndef XORP_DISABLE_PROFILE
#include "xrl/interfaces/profile_client_xif.hh"
#endif

//
// XXX: file "libxorp/profile.hh" must be included after
// "libxipc/xrl_std_router.hh" and "xrl/interfaces/profile_client_xif.hh"
// Sigh...
//
#include "libxorp/profile.hh"

#include "fea_node.hh"
#include "fibconfig_transaction.hh"
#ifndef XORP_DISABLE_FIREWALL
#include "firewall_transaction.hh"
#endif
#include "ifconfig_transaction.hh"
#include "libfeaclient_bridge.hh"
#ifndef XORP_DISABLE_PROFILE
#include "profile_vars.hh"
#endif
#include "xrl_fea_target.hh"

#ifdef XORP_USE_CLICK
#include "fea/data_plane/managers/fea_data_plane_manager_click.hh"
#endif

XrlFeaTarget::XrlFeaTarget(EventLoop&			eventloop,
			   FeaNode&			fea_node,
			   XrlRouter&			xrl_router,
#ifndef XORP_DISABLE_PROFILE
			   Profile&			profile,
#endif
			   XrlFibClientManager&		xrl_fib_client_manager,
			   LibFeaClientBridge&		lib_fea_client_bridge)
    : XrlFeaTargetBase(&xrl_router),
      _eventloop(eventloop),
      _fea_node(fea_node),
      _xrl_router(xrl_router),
#ifndef XORP_DISABLE_PROFILE
      _profile(profile),
#endif
      _xrl_fib_client_manager(xrl_fib_client_manager),
      _ifconfig(fea_node.ifconfig()),
#ifndef XORP_DISABLE_FIREWALL
      _firewall_manager(fea_node.firewall_manager()),
#endif
      _fibconfig(fea_node.fibconfig()),
      _io_link_manager(fea_node.io_link_manager()),
      _io_ip_manager(fea_node.io_ip_manager()),
      _io_tcpudp_manager(fea_node.io_tcpudp_manager()),
      _lib_fea_client_bridge(lib_fea_client_bridge),
      _is_running(false),
      _is_shutdown_received(false)
#ifdef XORP_USE_CLICK
    , _fea_data_plane_manager_click(NULL)
#endif
{
}

XrlFeaTarget::~XrlFeaTarget()
{
    shutdown();
}

int
XrlFeaTarget::startup()
{
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

XrlCmdError
XrlFeaTarget::common_0_1_get_target_name(
    // Output values,
    string&	name)
{
    name = this->name();
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::common_0_1_get_version(
    // Output values,
    string&   version)
{
    version = this->version();
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

    s = _ifconfig.status(r);
    // If it's bad news, don't bother to ask any other modules.
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

XrlCmdError
XrlFeaTarget::finder_event_observer_0_1_xrl_target_birth(
    // Input values,
    const string&	target_class,
    const string&	target_instance)
{
    debug_msg("XRL target birth event %s/%s\n",
	      target_class.c_str(), target_instance.c_str());

    _fea_node.fea_io().instance_birth(target_instance);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::finder_event_observer_0_1_xrl_target_death(
    // Input values,
    const string&	target_class,
    const string&	target_instance)
{
    debug_msg("XRL target death event %s/%s\n",
	      target_class.c_str(), target_instance.c_str());

    _fea_node.fea_io().instance_death(target_instance);

    return XrlCmdError::OKAY();
}


#ifdef XORP_USE_CLICK
/**
 *  Load Click FEA support.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_load_click()
{
    string error_msg;

    if (_fea_data_plane_manager_click != NULL) {
	error_msg = c_format("Data plane manager %s is already loaded",
			     _fea_data_plane_manager_click->manager_name().c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    _fea_data_plane_manager_click = new FeaDataPlaneManagerClick(_fea_node);

    if (_fea_node.register_data_plane_manager(_fea_data_plane_manager_click, false)
	!= XORP_OK) {
	error_msg = c_format("Cannot register data plane manager %s",
			     _fea_data_plane_manager_click->manager_name().c_str());
	delete _fea_data_plane_manager_click;
	_fea_data_plane_manager_click = NULL;
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    if (_fea_data_plane_manager_click->start_manager(error_msg) != XORP_OK) {
	error_msg = c_format("Cannot start data plane manager %s: %s",
			     _fea_data_plane_manager_click->manager_name().c_str(),
			     error_msg.c_str());
	delete _fea_data_plane_manager_click;
	_fea_data_plane_manager_click = NULL;
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

/**
 *  Unload Click FEA support.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_unload_click()
{
    string error_msg;

    if (_fea_data_plane_manager_click == NULL) {
	error_msg = c_format("Data plane manager Click is not loaded");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    if (_fea_node.unregister_data_plane_manager(_fea_data_plane_manager_click)
	!= XORP_OK) {
	error_msg = c_format("Cannot unregister data plane manager %s",
			     _fea_data_plane_manager_click->manager_name().c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    // XXX: The entry is deleted when unregistered
    _fea_data_plane_manager_click = NULL;

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
    string error_msg;

    if (_fea_data_plane_manager_click == NULL) {
	error_msg = c_format("Data plane manager Click is not loaded");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_fea_data_plane_manager_click->enable_click(enable, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

/**
 *  Start Click FEA support.
 */
XrlCmdError
XrlFeaTarget::fea_click_0_1_start_click()
{
    string error_msg;

    if (_fea_data_plane_manager_click == NULL) {
	error_msg = c_format("Data plane manager Click is not loaded");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_fea_data_plane_manager_click->start_plugins(error_msg) != XORP_OK) {
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
    string error_msg;

    if (_fea_data_plane_manager_click == NULL) {
	error_msg = c_format("Data plane manager Click is not loaded");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_fea_data_plane_manager_click->stop_plugins(error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
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
    string error_msg;

    if (_fea_data_plane_manager_click == NULL) {
	error_msg = c_format("Data plane manager Click is not loaded");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_fea_data_plane_manager_click->enable_duplicate_routes_to_kernel(
	    enable, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

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
    string error_msg;

    if (_fea_data_plane_manager_click == NULL) {
	error_msg = c_format("Data plane manager Click is not loaded");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_fea_data_plane_manager_click->enable_kernel_click(enable, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

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
    string error_msg;

    if (_fea_data_plane_manager_click == NULL) {
	error_msg = c_format("Data plane manager Click is not loaded");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_fea_data_plane_manager_click->enable_kernel_click_install_on_startup(
	    enable, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

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
    string error_msg;

    if (_fea_data_plane_manager_click == NULL) {
	error_msg = c_format("Data plane manager Click is not loaded");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

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

    if (_fea_data_plane_manager_click->set_kernel_click_modules(modules_list,
								error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
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
    string error_msg;

    if (_fea_data_plane_manager_click == NULL) {
	error_msg = c_format("Data plane manager Click is not loaded");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_fea_data_plane_manager_click->set_kernel_click_mount_directory(
	    directory, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

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
    string error_msg;

    if (_fea_data_plane_manager_click == NULL) {
	error_msg = c_format("Data plane manager Click is not loaded");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_fea_data_plane_manager_click->set_kernel_click_config_generator_file(
	    kernel_click_config_generator_file, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

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
    string error_msg;

    if (_fea_data_plane_manager_click == NULL) {
	error_msg = c_format("Data plane manager Click is not loaded");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_fea_data_plane_manager_click->enable_user_click(enable, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

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
    string error_msg;

    if (_fea_data_plane_manager_click == NULL) {
	error_msg = c_format("Data plane manager Click is not loaded");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_fea_data_plane_manager_click->set_user_click_command_file(
	    user_click_command_file, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

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
    string error_msg;

    if (_fea_data_plane_manager_click == NULL) {
	error_msg = c_format("Data plane manager Click is not loaded");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_fea_data_plane_manager_click->set_user_click_command_extra_arguments(
	    user_click_command_extra_arguments, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

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
    string error_msg;

    if (_fea_data_plane_manager_click == NULL) {
	error_msg = c_format("Data plane manager Click is not loaded");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_fea_data_plane_manager_click->set_user_click_command_execute_on_startup(
	    user_click_command_execute_on_startup, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

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
    string error_msg;

    if (_fea_data_plane_manager_click == NULL) {
	error_msg = c_format("Data plane manager Click is not loaded");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_fea_data_plane_manager_click->set_user_click_control_address(
	    user_click_control_address, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

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
    string error_msg;

    if (user_click_control_socket_port > 0xffff) {
	error_msg = c_format("Click control socket port %u is out of range",
			     user_click_control_socket_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_fea_data_plane_manager_click == NULL) {
	error_msg = c_format("Data plane manager Click is not loaded");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_fea_data_plane_manager_click->set_user_click_control_socket_port(
	    user_click_control_socket_port, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

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
    string error_msg;
    if (_fea_data_plane_manager_click == NULL) {
	error_msg = c_format("Data plane manager Click is not loaded");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_fea_data_plane_manager_click->set_user_click_startup_config_file(
	    user_click_startup_config_file, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

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
    string error_msg;
    if (_fea_data_plane_manager_click == NULL) {
	error_msg = c_format("Data plane manager Click is not loaded");
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_fea_data_plane_manager_click->set_user_click_config_generator_file(
	    user_click_config_generator_file, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

#endif //click

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
    return _xrl_fib_client_manager.add_fib_client4(client_target_name,
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
    return _xrl_fib_client_manager.delete_fib_client4(client_target_name);
}


#ifdef HAVE_IPV6
XrlCmdError
XrlFeaTarget::fea_fib_0_1_add_fib_client6(
    // Input values,
    const string&	client_target_name,
    const bool&		send_updates,
    const bool&		send_resolves)
{
    return _xrl_fib_client_manager.add_fib_client6(client_target_name,
						   send_updates,
						   send_resolves);
}

XrlCmdError
XrlFeaTarget::fea_fib_0_1_delete_fib_client6(
    // Input values,
    const string&	client_target_name)
{
    return _xrl_fib_client_manager.delete_fib_client6(client_target_name);
}
#endif


#ifndef XORP_DISABLE_FIREWALL

XrlCmdError
XrlFeaTarget::fea_firewall_0_1_startup_firewall() {
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fea_firewall_0_1_start_transaction(
    // Output values,
    uint32_t&	tid)
{
    string error_msg;

    if (_firewall_manager.start_transaction(tid, error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fea_firewall_0_1_commit_transaction(
    // Input values,
    const uint32_t&	tid)
{
    string error_msg;

    if (_firewall_manager.commit_transaction(tid, error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fea_firewall_0_1_abort_transaction(
    // Input values,
    const uint32_t&	tid)
{
    string error_msg;

    if (_firewall_manager.abort_transaction(tid, error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fea_firewall_0_1_add_entry4(
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
    const string&	action)
{
    FirewallEntry::Action action_value = FirewallEntry::str2action(action);
    string error_msg;

    if (action_value == FirewallEntry::ACTION_INVALID) {
	error_msg = c_format("Invalid firewall action: %s", action.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    FirewallEntry firewall_entry(rule_number, ifname, vifname,
				 IPvXNet(src_network), IPvXNet(dst_network),
				 ip_protocol, src_port_begin, src_port_end,
				 dst_port_begin, dst_port_end, action_value);

    if (_firewall_manager.add_transaction_operation(
	    tid,
	    new FirewallAddEntry4(_firewall_manager, firewall_entry),
	    error_msg)
	    != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fea_firewall_0_1_replace_entry4(
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
    const string&	action)
{
    FirewallEntry::Action action_value = FirewallEntry::str2action(action);
    string error_msg;

    if (action_value == FirewallEntry::ACTION_INVALID) {
	error_msg = c_format("Invalid firewall action: %s", action.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    FirewallEntry firewall_entry(rule_number, ifname, vifname,
				 IPvXNet(src_network), IPvXNet(dst_network),
				 ip_protocol, src_port_begin, src_port_end,
				 dst_port_begin, dst_port_end, action_value);

    if (_firewall_manager.add_transaction_operation(
	    tid,
	    new FirewallReplaceEntry4(_firewall_manager, firewall_entry),
	    error_msg)
	    != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fea_firewall_0_1_delete_entry4(
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
    const uint32_t&	dst_port_end)
{
    string error_msg;

    FirewallEntry::Action action_value = FirewallEntry::ACTION_ANY;

    FirewallEntry firewall_entry(rule_number, ifname, vifname,
				 IPvXNet(src_network), IPvXNet(dst_network),
				 ip_protocol, src_port_begin, src_port_end,
				 dst_port_begin, dst_port_end, action_value);

    if (_firewall_manager.add_transaction_operation(
	    tid,
	    new FirewallDeleteEntry4(_firewall_manager, firewall_entry),
	    error_msg)
	    != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fea_firewall_0_1_delete_all_entries4(
    // Input values,
    const uint32_t&	tid)
{
    string error_msg;

    if (_firewall_manager.add_transaction_operation(
	    tid,
	    new FirewallDeleteAllEntries4(_firewall_manager),
	    error_msg)
	    != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fea_firewall_0_1_get_entry_list_start4(
    // Output values,
    uint32_t&	token,
    bool&	more)
{
    string error_msg;

    if (_firewall_manager.get_entry_list_start4(token, more, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fea_firewall_0_1_get_entry_list_next4(
    // Input values,
    const uint32_t&	token,
    // Output values,
    uint32_t&		rule_number,
    string&		ifname,
    string&		vifname,
    IPv4Net&		src_network,
    IPv4Net&		dst_network,
    uint32_t&		ip_protocol,
    uint32_t&		src_port_begin,
    uint32_t&		src_port_end,
    uint32_t&		dst_port_begin,
    uint32_t&		dst_port_end,
    string&		action,
    bool&		more)
{
    string error_msg;
    FirewallEntry firewall_entry(IPv4::af());

    if (_firewall_manager.get_entry_list_next4(token, firewall_entry, more,
					       error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    // Extract the fields
    rule_number = firewall_entry.rule_number();
    ifname = firewall_entry.ifname();
    vifname = firewall_entry.vifname();
    src_network = firewall_entry.src_network().get_ipv4net();
    dst_network = firewall_entry.dst_network().get_ipv4net();
    ip_protocol = firewall_entry.ip_protocol();
    src_port_begin = firewall_entry.src_port_begin();
    src_port_end = firewall_entry.src_port_end();
    dst_port_begin = firewall_entry.dst_port_begin();
    dst_port_end = firewall_entry.dst_port_end();
    action = firewall_entry.action();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fea_firewall_0_1_add_entry6(
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
    const string&	action)
{
    FirewallEntry::Action action_value = FirewallEntry::str2action(action);
    string error_msg;

    if (action_value == FirewallEntry::ACTION_INVALID) {
	error_msg = c_format("Invalid firewall action: %s", action.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    FirewallEntry firewall_entry(rule_number, ifname, vifname,
				 IPvXNet(src_network), IPvXNet(dst_network),
				 ip_protocol, src_port_begin, src_port_end,
				 dst_port_begin, dst_port_end, action_value);

    if (_firewall_manager.add_transaction_operation(
	    tid,
	    new FirewallAddEntry6(_firewall_manager, firewall_entry),
	    error_msg)
	    != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fea_firewall_0_1_replace_entry6(
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
    const string&	action)
{
    FirewallEntry::Action action_value = FirewallEntry::str2action(action);
    string error_msg;

    if (action_value == FirewallEntry::ACTION_INVALID) {
	error_msg = c_format("Invalid firewall action: %s", action.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    FirewallEntry firewall_entry(rule_number, ifname, vifname,
				 IPvXNet(src_network), IPvXNet(dst_network),
				 ip_protocol, src_port_begin, src_port_end,
				 dst_port_begin, dst_port_end, action_value);

    if (_firewall_manager.add_transaction_operation(
	    tid,
	    new FirewallReplaceEntry6(_firewall_manager, firewall_entry),
	    error_msg)
	    != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fea_firewall_0_1_delete_entry6(
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
    const uint32_t&	dst_port_end)
{
    string error_msg;

    FirewallEntry::Action action_value = FirewallEntry::ACTION_ANY;

    FirewallEntry firewall_entry(rule_number, ifname, vifname,
				 IPvXNet(src_network), IPvXNet(dst_network),
				 ip_protocol, src_port_begin, src_port_end,
				 dst_port_begin, dst_port_end, action_value);

    if (_firewall_manager.add_transaction_operation(
	    tid,
	    new FirewallDeleteEntry6(_firewall_manager, firewall_entry),
	    error_msg)
	    != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fea_firewall_0_1_delete_all_entries6(
    // Input values,
    const uint32_t&	tid)
{
    string error_msg;

    if (_firewall_manager.add_transaction_operation(
	    tid,
	    new FirewallDeleteAllEntries6(_firewall_manager),
	    error_msg)
	    != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fea_firewall_0_1_get_entry_list_start6(
    // Output values,
    uint32_t&	token,
    bool&	more)
{
    string error_msg;

    if (_firewall_manager.get_entry_list_start6(token, more, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fea_firewall_0_1_get_entry_list_next6(
    // Input values,
    const uint32_t&	token,
    // Output values,
    uint32_t&		rule_number,
    string&		ifname,
    string&		vifname,
    IPv6Net&		src_network,
    IPv6Net&		dst_network,
    uint32_t&		ip_protocol,
    uint32_t&		src_port_begin,
    uint32_t&		src_port_end,
    uint32_t&		dst_port_begin,
    uint32_t&		dst_port_end,
    string&		action,
    bool&		more)
{
    string error_msg;
    FirewallEntry firewall_entry(IPv6::af());

    if (_firewall_manager.get_entry_list_next6(token, firewall_entry, more,
					       error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    // Extract the fields
    rule_number = firewall_entry.rule_number();
    ifname = firewall_entry.ifname();
    vifname = firewall_entry.vifname();
    src_network = firewall_entry.src_network().get_ipv6net();
    dst_network = firewall_entry.dst_network().get_ipv6net();
    ip_protocol = firewall_entry.ip_protocol();
    src_port_begin = firewall_entry.src_port_begin();
    src_port_end = firewall_entry.src_port_end();
    dst_port_begin = firewall_entry.dst_port_begin();
    dst_port_end = firewall_entry.dst_port_end();
    action = firewall_entry.action();

    return XrlCmdError::OKAY();
}

#endif // firewall

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_restore_original_config_on_shutdown(
    // Input values,
    const bool&	enable)
{
    _ifconfig.set_restore_original_config_on_shutdown(enable);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_interface_names(
    // Output values,
    XrlAtomList&	ifnames)
{
    const IfTree& iftree = _ifconfig.merged_config();

    for (IfTree::IfMap::const_iterator ii = iftree.interfaces().begin();
	 ii != iftree.interfaces().end(); ++ii) {
	ifnames.append(XrlAtom(ii->second->ifname()));
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

    ifp = _ifconfig.merged_config().find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Interface %s not found", ifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    for (IfTreeInterface::VifMap::const_iterator vi = ifp->vifs().begin();
	 vi != ifp->vifs().end(); ++vi) {
	vifnames.append(XrlAtom(vi->second->vifname()));
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

    vifp = _ifconfig.merged_config().find_vif(ifname, vifname);
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

    vifp = _ifconfig.merged_config().find_vif(ifname, vifname);
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

    ifp = _ifconfig.merged_config().find_interface(ifname);
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

    ifp = _ifconfig.merged_config().find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Interface %s not found", ifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    discard = ifp->discard();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_interface_unreachable(
    // Input values,
    const string&	ifname,
    // Output values,
    bool&		unreachable)
{
    const IfTreeInterface* ifp = NULL;
    string error_msg;

    ifp = _ifconfig.merged_config().find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Interface %s not found", ifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    unreachable = ifp->unreachable();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_interface_management(
    // Input values,
    const string&	ifname,
    // Output values,
    bool&		management)
{
    const IfTreeInterface* ifp = NULL;
    string error_msg;

    ifp = _ifconfig.merged_config().find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Interface %s not found", ifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    management = ifp->management();

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

    ifp = _ifconfig.merged_config().find_interface(ifname);
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

    ifp = _ifconfig.merged_config().find_interface(ifname);
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

    ifp = _ifconfig.merged_config().find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Interface %s not found", ifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    no_carrier = ifp->no_carrier();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_get_configured_baudrate(
    // Input values,
    const string&	ifname,
    uint64_t&		baudrate)
{
    const IfTreeInterface* ifp = NULL;
    string error_msg;

    ifp = _ifconfig.merged_config().find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Interface %s not found", ifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    //
    // TODO: XXX: The returned baudrate variable must be changed to uint64_t
    // after the XRLs start support u64 type.
    //
    baudrate = ifp->baudrate();

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

    vifp = _ifconfig.merged_config().find_vif(ifname, vifname);
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

    ap = _ifconfig.merged_config().find_addr(ifname, vifname, address);
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

    ap = _ifconfig.merged_config().find_addr(ifname, vifname, address);
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

    ap = _ifconfig.merged_config().find_addr(ifname, vifname, address);
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

    vifp = _ifconfig.merged_config().find_vif(ifname, vifname);
    if (vifp == NULL) {
	error_msg = c_format("Interface %s vif %s not found",
			     ifname.c_str(), vifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    for (IfTreeVif::IPv4Map::const_iterator ai = vifp->ipv4addrs().begin();
	 ai != vifp->ipv4addrs().end(); ++ai) {
	addresses.append(XrlAtom(ai->second->addr()));
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

    ap = _ifconfig.merged_config().find_addr(ifname, vifname, address);
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

    ap = _ifconfig.merged_config().find_addr(ifname, vifname, address);
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

    vifp = _ifconfig.merged_config().find_vif(ifname, vifname);
    if (vifp == NULL) {
	error_msg = c_format("Interface %s vif %s not found",
			     ifname.c_str(), vifname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    for (IfTreeVif::IPv6Map::const_iterator ai = vifp->ipv6addrs().begin();
	 ai != vifp->ipv6addrs().end(); ++ai) {
	addresses.append(XrlAtom(ai->second->addr()));
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

    ap = _ifconfig.merged_config().find_addr(ifname, vifname, address);
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

    ap = _ifconfig.merged_config().find_addr(ifname, vifname, address);
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

    ap = _ifconfig.merged_config().find_addr(ifname, vifname, address);
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

    ap = _ifconfig.merged_config().find_addr(ifname, vifname, address);
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

    if (_ifconfig.start_transaction(tid, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_commit_transaction(
    // Input values,
    const uint32_t&	tid)
{
    string error_msg;

    if (_ifconfig.commit_transaction(tid, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_abort_transaction(
    // Input values,
    const uint32_t&	tid)
{
    string error_msg;

    if (_ifconfig.abort_transaction(tid, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_create_interface(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname)
{
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new AddInterface(_ifconfig, ifname),
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
    string error_msg;

    XLOG_INFO("Deleting interface, ifname: %s\n", ifname.c_str());

    // Hack:  Remove multicast addrs first. --Ben
    string empty;
    _io_ip_manager.leave_all_multicast_groups(ifname, empty, error_msg);
    if (error_msg.size()) {
	XLOG_ERROR("%s", error_msg.c_str());
    }

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new RemoveInterface(_ifconfig, ifname),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_configure_all_interfaces_from_system(
    // Input values,
    const uint32_t&	tid,
    const bool&		enable)
{
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new ConfigureAllInterfacesFromSystem(_ifconfig, enable),
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
    const string&	ifname,
    const bool&		enable)
{
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new ConfigureInterfaceFromSystem(_ifconfig, ifname, enable),
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
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new SetInterfaceEnabled(_ifconfig, ifname, enabled),
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
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new SetInterfaceDiscard(_ifconfig, ifname, discard),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_interface_unreachable(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const bool&		unreachable)
{
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new SetInterfaceUnreachable(_ifconfig, ifname, unreachable),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_interface_management(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const bool&		management)
{
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new SetInterfaceManagement(_ifconfig, ifname, management),
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
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new SetInterfaceMac(_ifconfig, ifname, mac),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_create_mac(
    // Input values,
    const string&   ifname,
    const Mac&      mac)
{
    string error_msg;

    if (add_remove_mac(true, ifname, mac, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_delete_mac(
    // Input values,
    const string&   ifname,
    const Mac&      mac)
{
    string error_msg;

    if (add_remove_mac(false, ifname, mac, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

int
XrlFeaTarget::add_remove_mac(bool add, const string& ifname, const Mac& mac,
			     string& error_msg)
{
    IfTreeInterface* ifp;

    //
    // XXX: This whole thing is a hack.
    //

    //
    // There are two entities:
    // 1) The MAC of the interface.
    // 2) The multicast MACs of an interface.
    //
    // To "add" a MAC, we set the interface's MAC to the newly added MAC, and
    // put the old MAC in the multicast ones.  To "delete", we kill a MAC from
    // the multicast ones, and stick it as the primary MAC.
    //

    //
    // Grab the set of configured MACs from somewhere.
    //
    // XXX This set should be stored elsewhere in process related
    // state - not the user config state.
    //
    ifp = _ifconfig.user_config().find_interface(ifname);
    if (ifp == NULL) {
	error_msg = c_format("Cannot %s MAC address %s on interface %s: "
			     "unknown interface",
			     (add) ? "add" : "remove",
			     mac.str().c_str(), ifname.c_str());
	return (XORP_ERROR);
    }

    // These are the multicast MACs.
    IfTreeInterface::MacSet& macs = ifp->macs();

    //
    // This is the real MAC.  
    //
    // XXX: Hopefully the user doesn't change it, otherwise this creates a
    // problem. To avoid the problem, we just need proper datastructures - a
    // stack of MACs. This will be fixed once we determine what is needed,
    // and this becomes less than a hack.
    //
    ifp = _ifconfig.merged_config().find_interface(ifname);
    XLOG_ASSERT(ifp != NULL);
    Mac current_mac = ifp->mac();

    //
    // One MAC is OK, because we can change the primary MAC.
    // More MACs rely on the multicast trick.
    //
    if (add && macs.size()) {
	error_msg = c_format("Cannot add MAC address %s on interface %s: "
			     "too many MACs",
			     mac.str().c_str(), ifname.c_str());
	return (XORP_ERROR);
    }

    if (add) {
	// Add the MAC

	// Sanity check
	if (macs.find(mac) != macs.end() || current_mac == mac) {
	    error_msg = c_format("Cannot add MAC address %s on interface %s: "
				 "MAC already exists, current_mac: %s  mac count: %i",
				 mac.str().c_str(), ifname.c_str(), current_mac.str().c_str(),
				 (int)(macs.size()));
	    // This doesn't seem so bad to me...going to log it and pass back success. --Ben
	    XLOG_WARNING("%s", error_msg.c_str());
	    return XORP_OK;
	}

	if (macs.size())
	    XLOG_WARNING("More than one MAC added - use at your own risk");

	if (set_mac(ifname, mac, error_msg) != XORP_OK) {
	    error_msg = c_format("Cannot add MAC address %s on interface %s: %s",
				 mac.str().c_str(), ifname.c_str(),
				 error_msg.c_str());
	    return (XORP_ERROR);
	}

	// Add previous MAC as multicast one
	macs.insert(current_mac);

	if (_io_link_manager.add_multicast_mac(ifname, current_mac, error_msg)
	    != XORP_OK) {
	    XLOG_WARNING("Cannot add multicast MAC address %s "
			 "on interface %s: %s",
			 current_mac.str().c_str(), ifname.c_str(),
			 error_msg.c_str());
	}
    } else {
	// Remove the MAC
	Mac candidate_mac;

	// Removing current mac
	if (mac == current_mac) {
	    if (macs.empty()) {
		error_msg = c_format("Cannot remove MAC address %s "
				     "on interface %s: last address",
				     mac.str().c_str(), ifname.c_str());
		XLOG_WARNING("%s", error_msg.c_str());
		return XORP_OK;
	    }
	    candidate_mac = *(macs.begin());

	    // XXX: should remove from multi first
	    if (set_mac(ifname, candidate_mac, error_msg) != XORP_OK) {
		error_msg = c_format("Cannot replace MAC address %s with %s "
				     "on interface %s: %s",
				     mac.str().c_str(),
				     candidate_mac.str().c_str(),
				     ifname.c_str(),
				     error_msg.c_str());
		return (XORP_ERROR);
	    }
	} else {
	    IfTreeInterface::MacSet::iterator i = macs.find(mac);
	    if (i == macs.end()) {
		error_msg = c_format("Cannot remove MAC address %s on "
				     "interface %s: unknown address",
				     mac.str().c_str(), ifname.c_str());
		return (XORP_ERROR);
	    }
	    candidate_mac = *i;
	}

	// Remove MAC from multicast list
	macs.erase(candidate_mac);

	if (_io_link_manager.remove_multicast_mac(ifname, candidate_mac,
						  error_msg)
	    != XORP_OK) {
	    XLOG_WARNING("Cannot remove multicast MAC address %s "
			 "on interface %s: %s",
			 candidate_mac.str().c_str(), ifname.c_str(),
			 error_msg.c_str());
	}
    }

    return (XORP_OK);
}

int
XrlFeaTarget::set_mac(const string& ifname, const Mac& mac, string& error_msg)
{
    //
    // TODO: XXX: We should delegate this to ifconfig, but perhaps lets see
    // what we need exactly before polluting the rest of the code with
    // this MAC hack.
    //
    uint32_t tid;

    if (ifmgr_0_1_start_transaction(tid) != XrlCmdError::OKAY()) {
	error_msg = c_format("Cannot set MAC address %s on interface %s: "
			     "cannot start the transaction",
			     mac.str().c_str(), ifname.c_str());
	return (XORP_ERROR);
    }

    if (ifmgr_0_1_set_mac(tid, ifname, mac) != XrlCmdError::OKAY()) {
	ifmgr_0_1_abort_transaction(tid);
	error_msg = c_format("Cannot set MAC address %s on interface %s: "
			     "cannot perform the operation",
			     mac.str().c_str(), ifname.c_str());
	return (XORP_ERROR);
    }

    if (ifmgr_0_1_commit_transaction(tid) != XrlCmdError::OKAY()) {
	error_msg = c_format("Cannot set MAC address %s on interface %s: "
			     "cannot commit the transaction",
			     mac.str().c_str(), ifname.c_str());
	return (XORP_ERROR);
    }

    if (send_gratuitous_arps(ifname, mac, error_msg) != XORP_OK) {
	error_msg = c_format("Cannot set MAC address %s on interface %s: %s",
			     mac.str().c_str(), ifname.c_str(),
			     error_msg.c_str());
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
XrlFeaTarget::send_gratuitous_arps(const string& ifname, const Mac& mac,
				   string& error_msg)
    {
    IfTreeInterface* ifp = _ifconfig.merged_config().find_interface(ifname);

    XLOG_ASSERT(ifp != NULL);

    if (! ifp->enabled()) {
	// TODO: Return success or an error?
	return (XORP_OK);
    }

    for (IfTreeInterface::VifMap::iterator i = ifp->vifs().begin(); 
	 i != ifp->vifs().end(); ++i) {

	const string& vifname = i->first;
	IfTreeVif* vifp	      = i->second;

	if (! vifp->enabled())
	    continue;

	for (IfTreeVif::IPv4Map::iterator j = vifp->ipv4addrs().begin();
	     j != vifp->ipv4addrs().end(); ++j) {

	    const IPv4& ip    = j->first;
	    IfTreeAddr4* addr = j->second;

	    if (! addr->enabled())
		continue;

	    vector<uint8_t> data;
	    ArpHeader::make_gratuitous(data, mac, ip);

	    XrlCmdError e = raw_link_0_1_send(ifname, vifname, mac,
					      Mac::BROADCAST(),
					      ETHERTYPE_ARP, data);
	    if (e != XrlCmdError::OKAY())
		error_msg = c_format("Cannot send gratuitous ARP "
				     "for MAC address %s on interface %s: %s",
				     mac.str().c_str(), ifname.c_str(),
				     e.str().c_str());
	}

	// TODO: XXX: IPv6?
    }

    return (XORP_OK);
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_restore_original_mac(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname)
{
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new RestoreInterfaceMac(_ifconfig, ifname),
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
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new SetInterfaceMtu(_ifconfig, ifname, mtu),
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
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new RestoreInterfaceMtu(_ifconfig, ifname),
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
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new AddInterfaceVif(_ifconfig, ifname, vifname),
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
    string error_msg;

    XLOG_ERROR("Deleting vif, ifname: %s  vif: %s\n", ifname.c_str(), vifname.c_str());

    // Hack:  Remove multicast addrs first. --Ben
    _io_ip_manager.leave_all_multicast_groups(ifname, vifname, error_msg);
    if (error_msg.size()) {
	XLOG_ERROR("%s", error_msg.c_str());
    }

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new RemoveInterfaceVif(_ifconfig, ifname, vifname),
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
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new SetVifEnabled(_ifconfig, ifname, vifname, enabled),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_set_vif_vlan(
    // Input values,
    const uint32_t&	tid,
    const string&	ifname,
    const string&	vif,
    const uint32_t&	vlan_id)
{
    string error_msg;
    const uint32_t max_vlan_id = 4096;

    // Check the VLAN ID value
    if (vlan_id >= max_vlan_id) {
	error_msg = c_format("Invalid VLAN ID %u for interface %s vif %s: "
			     "maximum value is %u",
			     vlan_id, ifname.c_str(), vif.c_str(),
			     max_vlan_id);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new SetVifVlan(_ifconfig, ifname, vif, vlan_id),
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
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new AddAddr4(_ifconfig, ifname, vifname, address),
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
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new RemoveAddr4(_ifconfig, ifname, vifname, address),
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
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new SetAddr4Enabled(_ifconfig, ifname, vifname, address, enabled),
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
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new SetAddr4Prefix(_ifconfig, ifname, vifname, address, prefix_len),
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
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new SetAddr4Broadcast(_ifconfig, ifname, vifname, address, broadcast),
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
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new SetAddr4Endpoint(_ifconfig, ifname, vifname, address, endpoint),
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
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new AddAddr6(_ifconfig, ifname, vifname, address),
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
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new RemoveAddr6(_ifconfig, ifname, vifname, address),
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
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new SetAddr6Enabled(_ifconfig, ifname, vifname, address, enabled),
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
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new SetAddr6Prefix(_ifconfig, ifname, vifname, address, prefix_len),
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
    string error_msg;

    if (_ifconfig.add_transaction_operation(
	    tid,
	    new SetAddr6Endpoint(_ifconfig, ifname, vifname, address, endpoint),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_0_1_startup_ifmgr() {
    return XrlCmdError::OKAY();
}


XrlCmdError
XrlFeaTarget::ifmgr_replicator_0_1_register_ifmgr_mirror(
    // Input values,
    const string&	clientname)
{
    string error_msg;

    if (_lib_fea_client_bridge.add_libfeaclient_mirror(clientname) != XORP_OK) {
	error_msg = c_format("Cannot register ifmgr mirror client %s",
			     clientname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::ifmgr_replicator_0_1_unregister_ifmgr_mirror(
    // Input values,
    const string&	clientname)
{
    string error_msg;

    if (_lib_fea_client_bridge.remove_libfeaclient_mirror(clientname)
	!= XORP_OK) {
	error_msg = c_format("Cannot unregister ifmgr mirror client %s",
			     clientname.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
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
    Fte4 fte;
    if (_fibconfig.lookup_route_by_dest4(dst, fte) == XORP_OK) {
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
    Fte6 fte;
    if (_fibconfig.lookup_route_by_dest6(dst, fte) == XORP_OK) {
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
    Fte4 fte;
    if (_fibconfig.lookup_route_by_network4(dst, fte) == XORP_OK) {
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
    Fte6 fte;
    if (_fibconfig.lookup_route_by_network6(dst, fte) == XORP_OK) {
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
    result = _fea_node.have_ipv4();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_have_ipv6(
    // Output values,
    bool&	result)
{
    result = _fea_node.have_ipv6();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_get_unicast_forwarding_enabled4(
    // Output values,
    bool&	enabled)
{
    string error_msg;

    if (_fibconfig.unicast_forwarding_enabled4(enabled, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_get_unicast_forwarding_enabled6(
    // Output values,
    bool&	enabled)
{
    string error_msg;

    if (_fibconfig.unicast_forwarding_enabled6(enabled, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_set_unicast_forwarding_enabled4(
    // Input values,
    const bool&	enabled)
{
    string error_msg;

    if (_fibconfig.set_unicast_forwarding_enabled4(enabled, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_set_unicast_forwarding_enabled6(
    // Input values,
    const bool&	enabled)
{
    string error_msg;

    if (_fibconfig.set_unicast_forwarding_enabled6(enabled, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_set_unicast_forwarding_entries_retain_on_startup4(
    // Input values,
    const bool&	retain)
{
    string error_msg;

    if (_fibconfig.set_unicast_forwarding_entries_retain_on_startup4(
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

    if (_fibconfig.set_unicast_forwarding_entries_retain_on_shutdown4(
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

    if (_fibconfig.set_unicast_forwarding_entries_retain_on_startup6(
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

    if (_fibconfig.set_unicast_forwarding_entries_retain_on_shutdown6(
	    retain,
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_set_unicast_forwarding_table_id4(
    // Input values,
    const bool&		is_configured,
    const uint32_t&	table_id)
{
    string error_msg;

    if (_fibconfig.set_unicast_forwarding_table_id4(
	    is_configured,
	    table_id,
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::fti_0_2_set_unicast_forwarding_table_id6(
    // Input values,
    const bool&		is_configured,
    const uint32_t&	table_id)
{
    string error_msg;

    if (_fibconfig.set_unicast_forwarding_table_id6(
	    is_configured,
	    table_id,
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

    if (_fibconfig.start_transaction(tid, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::redist_transaction4_0_1_commit_transaction(
    // Input values,
    const uint32_t&	tid)
{
    string error_msg;

    if (_fibconfig.commit_transaction(tid, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::redist_transaction4_0_1_abort_transaction(
    // Input values,
    const uint32_t&	tid)
{
    string error_msg;

    if (_fibconfig.abort_transaction(tid, error_msg) != XORP_OK)
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

    is_xorp_route = true;	// XXX: unconditionally set to true

    // TODO: XXX: get rid of the hard-coded "connected" string here
    if (protocol_origin == "connected")
	is_connected_route = true;

    PROFILE(if (_profile.enabled(profile_route_in))
		_profile.log(profile_route_in, c_format("add %s", dst.str().c_str())));

    if (_fibconfig.add_transaction_operation(
	    tid,
	    new FibAddEntry4(_fibconfig, dst, nexthop, ifname, vifname,
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

    is_xorp_route = true;	// XXX: unconditionally set to true

    // TODO: XXX: get rid of the hard-coded "connected" string here
    if (protocol_origin == "connected")
	is_connected_route = true;

    PROFILE(if (_profile.enabled(profile_route_in))
		_profile.log(profile_route_in,
			     c_format("delete %s", dst.str().c_str())));

    if (_fibconfig.add_transaction_operation(
	    tid,
	    new FibDeleteEntry4(_fibconfig, dst, nexthop, ifname, vifname,
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

    if (_fibconfig.add_transaction_operation(
	    tid,
	    new FibDeleteAllEntries4(_fibconfig),
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

    if (_fibconfig.start_transaction(tid, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::redist_transaction6_0_1_commit_transaction(
    // Input values,
    const uint32_t&	tid)
{
    string error_msg;

    if (_fibconfig.commit_transaction(tid, error_msg) != XORP_OK)
	return XrlCmdError::COMMAND_FAILED(error_msg);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::redist_transaction6_0_1_abort_transaction(
    // Input values,
    const uint32_t&	tid)
{
    string error_msg;

    if (_fibconfig.abort_transaction(tid, error_msg) != XORP_OK)
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

    is_xorp_route = true;	// XXX: unconditionally set to true

    // TODO: XXX: get rid of the hard-coded "connected" string here
    if (protocol_origin == "connected")
	is_connected_route = true;

    PROFILE(if (_profile.enabled(profile_route_in))
		_profile.log(profile_route_in, c_format("add %s", dst.str().c_str())));

    if (_fibconfig.add_transaction_operation(
	    tid,
	    new FibAddEntry6(_fibconfig, dst, nexthop, ifname, vifname,
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

    is_xorp_route = true;	// XXX: unconditionally set to true

    // TODO: XXX: get rid of the hard-coded "connected" string here
    if (protocol_origin == "connected")
	is_connected_route = true;

    PROFILE(if (_profile.enabled(profile_route_in))
		_profile.log(profile_route_in,
			     c_format("delete %s", dst.str().c_str())));

    if (_fibconfig.add_transaction_operation(
	    tid,
	    new FibDeleteEntry6(_fibconfig, dst, nexthop, ifname, vifname,
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

    if (_fibconfig.add_transaction_operation(
	    tid,
	    new FibDeleteAllEntries6(_fibconfig),
	    error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// Raw Link-Level Server Interface related

XrlCmdError
XrlFeaTarget::raw_link_0_1_send(
    // Input values,
    const string&		if_name,
    const string&		vif_name,
    const Mac&			src_address,
    const Mac&			dst_address,
    const uint32_t&		ether_type,
    const vector<uint8_t>&	payload)
{
    string error_msg;

    if (_io_link_manager.send(if_name, vif_name, src_address, dst_address,
			      ether_type, payload, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::raw_link_0_1_register_receiver(
    // Input values,
    const string&	xrl_target_instance_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ether_type,
    const string&	filter_program,
    const bool&		enable_multicast_loopback)
{
    string error_msg;

    XLOG_INFO("register receiver, target: %s iface: %s:%s ether: %i  filter: %s  mcast_loopback: %i\n",
	      xrl_target_instance_name.c_str(), if_name.c_str(), vif_name.c_str(),
	      ether_type, filter_program.c_str(), (int)(enable_multicast_loopback));

    if (_io_link_manager.register_receiver(xrl_target_instance_name,
					   if_name, vif_name,
					   ether_type, filter_program,
					   enable_multicast_loopback,
					   error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::raw_link_0_1_unregister_receiver(
    // Input values,
    const string&	xrl_target_instance_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ether_type,
    const string&	filter_program)
{
    string error_msg;

    XLOG_INFO("unregister receiver, target: %s iface: %s:%s ether: %i  filter: %s\n",
	      xrl_target_instance_name.c_str(), if_name.c_str(), vif_name.c_str(),
	      ether_type, filter_program.c_str());

    if (_io_link_manager.unregister_receiver(xrl_target_instance_name,
					     if_name, vif_name,
					     ether_type, filter_program,
					     error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::raw_link_0_1_join_multicast_group(
    // Input values,
    const string&	xrl_target_instance_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ether_type,
    const string&	filter_program,
    const Mac&		group_address)
{
    string error_msg;

    if (_io_link_manager.join_multicast_group(xrl_target_instance_name,
					      if_name, vif_name,
					      ether_type, filter_program,
					      group_address, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::raw_link_0_1_leave_multicast_group(
    // Input values,
    const string&	xrl_target_instance_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ether_type,
    const string&	filter_program,
    const Mac&		group_address)
{
    string error_msg;

    if (_io_link_manager.leave_multicast_group(xrl_target_instance_name,
					       if_name, vif_name,
					       ether_type, filter_program,
					       group_address, error_msg)
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
    const bool&			ip_internet_control,
    const vector<uint8_t>&	payload)
{
    string error_msg;

    vector<uint8_t> ext_headers_type_vector;
    vector<vector<uint8_t> > ext_headers_payload_vector;

    if (_io_ip_manager.send(if_name, vif_name, IPvX(src_address),
			    IPvX(dst_address), ip_protocol, ip_ttl, ip_tos,
			    ip_router_alert, ip_internet_control,
			    ext_headers_type_vector,
			    ext_headers_payload_vector,
			    payload, error_msg)
	!= XORP_OK) {
	assert(error_msg.size()); // don't allow empty error messages.
	//XLOG_ERROR("Failed raw_packet4_0_1_send, iface: %s/%s  error_msg: %s\n",
	//	   if_name.c_str(), vif_name.c_str(), error_msg.c_str());
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::raw_packet4_0_1_register_receiver(
    // Input values,
    const string&	xrl_target_instance_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ip_protocol,
    const bool&		enable_multicast_loopback)
{
    string error_msg;

    if (_io_ip_manager.register_receiver(IPv4::af(),
					 xrl_target_instance_name,
					 if_name, vif_name, ip_protocol,
					 enable_multicast_loopback,
					 error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::raw_packet4_0_1_unregister_receiver(
    // Input values,
    const string&	xrl_target_instance_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ip_protocol)
{
    string error_msg;

    if (_io_ip_manager.unregister_receiver(IPv4::af(),
					   xrl_target_instance_name,
					   if_name, vif_name, ip_protocol,
					   error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::raw_packet4_0_1_join_multicast_group(
    // Input values,
    const string&	xrl_target_instance_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ip_protocol,
    const IPv4&		group_address)
{
    string error_msg;

    if (_io_ip_manager.join_multicast_group(xrl_target_instance_name,
					    if_name, vif_name, ip_protocol,
					    IPvX(group_address), error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::raw_packet4_0_1_leave_multicast_group(
    // Input values,
    const string&	xrl_target_instance_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ip_protocol,
    const IPv4&		group_address)
{
    string error_msg;

    if (_io_ip_manager.leave_multicast_group(xrl_target_instance_name,
					     if_name, vif_name, ip_protocol,
					     IPvX(group_address), error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// IPv6 Raw Socket related

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
    const bool&		ip_internet_control,
    const XrlAtomList&	ext_headers_type,
    const XrlAtomList&	ext_headers_payload,
    const vector<uint8_t>&	payload)
{
    string error_msg;

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

    if (_io_ip_manager.send(if_name, vif_name,
			    IPvX(src_address), IPvX(dst_address),
			    ip_protocol, ip_ttl, ip_tos, ip_router_alert,
			    ip_internet_control, ext_headers_type_vector,
			    ext_headers_payload_vector, payload, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::raw_packet6_0_1_register_receiver(
    // Input values,
    const string&	xrl_target_instance_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ip_protocol,
    const bool&		enable_multicast_loopback)
{
    string error_msg;

    if ( _io_ip_manager.register_receiver(IPv6::af(),
					  xrl_target_instance_name,
					  if_name, vif_name, ip_protocol,
					  enable_multicast_loopback, error_msg)
	 != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::raw_packet6_0_1_unregister_receiver(
    // Input values,
    const string&	xrl_target_instance_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ip_protocol)
{
    string error_msg;

    if (_io_ip_manager.unregister_receiver(IPv6::af(),
					   xrl_target_instance_name,
					   if_name, vif_name, ip_protocol,
					   error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::raw_packet6_0_1_join_multicast_group(
    // Input values,
    const string&	xrl_target_instance_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ip_protocol,
    const IPv6&		group_address)
{
    string error_msg;

    if (_io_ip_manager.join_multicast_group(xrl_target_instance_name,
					    if_name, vif_name, ip_protocol,
					    IPvX(group_address), error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::raw_packet6_0_1_leave_multicast_group(
    // Input values,
    const string&	xrl_target_instance_name,
    const string&	if_name,
    const string&	vif_name,
    const uint32_t&	ip_protocol,
    const IPv6&		group_address)
{
    string error_msg;

    if (_io_ip_manager.leave_multicast_group(xrl_target_instance_name,
					     if_name, vif_name, ip_protocol,
					     IPvX(group_address), error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

// ----------------------------------------------------------------------------
// TCP/UDP I/O Socket Server Interface

XrlCmdError
XrlFeaTarget::socket4_0_1_tcp_open(
    // Input values,
    const string&	creator,
    // Output values,
    string&		sockid)
{
    string error_msg;

    if (_io_tcpudp_manager.tcp_open(IPv4::af(), creator, sockid, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket4_0_1_udp_open(
    // Input values,
    const string&	creator,
    // Output values,
    string&		sockid)
{
    string error_msg;

    if (_io_tcpudp_manager.udp_open(IPv4::af(), creator, sockid, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket4_0_1_tcp_open_and_bind(
    // Input values,
    const string&	creator,
    const IPv4&		local_addr,
    const uint32_t&	local_port,
    // Output values,
    string&		sockid)
{
    string error_msg;

    if (local_port > 0xffff) {
	error_msg = c_format("Local port %u is out of range", local_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_io_tcpudp_manager.tcp_open_and_bind(IPv4::af(), creator,
					     IPvX(local_addr), local_port,
					     sockid, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket4_0_1_udp_open_and_bind(
    // Input values,
    const string&	creator,
    const IPv4&		local_addr,
    const uint32_t&	local_port,
    const string&       local_dev,
    const uint32_t&     reuse,
    // Output values,
    string&		sockid)
{
    string error_msg;

    if (local_port > 0xffff) {
	error_msg = c_format("Local port %u is out of range", local_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_io_tcpudp_manager.udp_open_and_bind(IPv4::af(), creator,
					     IPvX(local_addr), local_port,
					     local_dev, reuse,
					     sockid, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket4_0_1_udp_open_bind_join(
    // Input values,
    const string&	creator,
    const IPv4&		local_addr,
    const uint32_t&	local_port,
    const IPv4&		mcast_addr,
    const uint32_t&	ttl,
    const bool&		reuse,
    // Output values,
    string&		sockid)
{
    string error_msg;

    if (local_port > 0xffff) {
	error_msg = c_format("Local port %u is out of range", local_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    if (ttl > 0xff) {
	error_msg = c_format("TTL %u is out of range", ttl);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_io_tcpudp_manager.udp_open_bind_join(IPv4::af(), creator,
					      IPvX(local_addr), local_port,
					      IPvX(mcast_addr), ttl, reuse,
					      sockid, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket4_0_1_tcp_open_bind_connect(
    // Input values,
    const string&	creator,
    const IPv4&		local_addr,
    const uint32_t&	local_port,
    const IPv4&		remote_addr,
    const uint32_t&	remote_port,
    // Output values,
    string&		sockid)
{
    string error_msg;

    if (local_port > 0xffff) {
	error_msg = c_format("Local port %u is out of range", local_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    if (remote_port > 0xffff) {
	error_msg = c_format("Remote port %u is out of range", remote_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_io_tcpudp_manager.tcp_open_bind_connect(IPv4::af(), creator,
						 IPvX(local_addr), local_port,
						 IPvX(remote_addr),
						 remote_port, sockid,
						 error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket4_0_1_udp_open_bind_connect(
    // Input values,
    const string&	creator,
    const IPv4&		local_addr,
    const uint32_t&	local_port,
    const IPv4&		remote_addr,
    const uint32_t&	remote_port,
    // Output values,
    string&		sockid)
{
    string error_msg;

    if (local_port > 0xffff) {
	error_msg = c_format("Local port %u is out of range", local_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    if (remote_port > 0xffff) {
	error_msg = c_format("Remote port %u is out of range", remote_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_io_tcpudp_manager.udp_open_bind_connect(IPv4::af(), creator,
						 IPvX(local_addr), local_port,
						 IPvX(remote_addr),
						 remote_port, sockid,
						 error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket4_0_1_udp_open_bind_broadcast(
    // Input values,
    const string&	creator,
    const string&	ifname,
    const string&	vifname,
    const uint32_t&	local_port,
    const uint32_t&	remote_port,
    const bool&	        reuse,
    const bool&	        limited,
    const bool&	        connected,
    // Output values,
    string&	sockid)
{
    string error_msg;

    if (local_port > 0xffff) {
	error_msg = c_format("Local port %u is out of range", local_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    if (remote_port > 0xffff) {
	error_msg = c_format("Remote port %u is out of range", remote_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_io_tcpudp_manager.udp_open_bind_broadcast(IPv4::af(), creator,
						   ifname,
						   vifname,
						   local_port,
						   remote_port,
						   reuse,
						   limited,
						   connected,
						   sockid,
						   error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket4_0_1_bind(
    // Input values,
    const string&	sockid,
    const IPv4&		local_addr,
    const uint32_t&	local_port)
{
    string error_msg;

    if (local_port > 0xffff) {
	error_msg = c_format("Local port %u is out of range", local_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_io_tcpudp_manager.bind(IPv4::af(), sockid, IPvX(local_addr),
				local_port, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket4_0_1_udp_join_group(
    // Input values,
    const string&	sockid,
    const IPv4&		mcast_addr,
    const IPv4&		join_if_addr)
{
    string error_msg;

    if (_io_tcpudp_manager.udp_join_group(IPv4::af(), sockid, IPvX(mcast_addr),
					  IPvX(join_if_addr), error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket4_0_1_udp_leave_group(
    // Input values,
    const string&	sockid,
    const IPv4&		mcast_addr,
    const IPv4&		leave_if_addr)
{
    string error_msg;

    if (_io_tcpudp_manager.udp_leave_group(IPv4::af(), sockid,
					   IPvX(mcast_addr),
					   IPvX(leave_if_addr), error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket4_0_1_close(
    // Input values,
    const string&	sockid)
{
    string error_msg;

    if (_io_tcpudp_manager.close(IPv4::af(), sockid, error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket4_0_1_tcp_listen(
    // Input values,
    const string&	sockid,
    const uint32_t&	backlog)
{
    string error_msg;

    if (_io_tcpudp_manager.tcp_listen(IPv4::af(), sockid, backlog, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket4_0_1_udp_enable_recv(
    // Input values,
    const string&	sockid)
{
    string error_msg;

    if (_io_tcpudp_manager.udp_enable_recv(IPv4::af(), sockid, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket4_0_1_send(
    // Input values,
    const string&	sockid,
    const vector<uint8_t>& data)
{
    string error_msg;

    if (_io_tcpudp_manager.send(IPv4::af(), sockid, data, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket4_0_1_send_to(
    // Input values,
    const string&	sockid,
    const IPv4&		remote_addr,
    const uint32_t&	remote_port,
    const vector<uint8_t>& data)
{
    string error_msg;

    if (remote_port > 0xffff) {
	error_msg = c_format("Remote port %u is out of range", remote_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_io_tcpudp_manager.send_to(IPv4::af(), sockid, IPvX(remote_addr),
				   remote_port, data, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket4_0_1_send_from_multicast_if(
    // Input values,
    const string&	sockid,
    const IPv4&		group_addr,
    const uint32_t&	group_port,
    const IPv4&		ifaddr,
    const vector<uint8_t>& data)
{
    string error_msg;

    if (group_port > 0xffff) {
	error_msg = c_format("Multicast group port %u is out of range",
			     group_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_io_tcpudp_manager.send_from_multicast_if(IPv4::af(), sockid,
						  IPvX(group_addr), group_port,
						  ifaddr, data, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket4_0_1_set_socket_option(
    // Input values,
    const string&	sockid,
    const string&	optname,
    const uint32_t&	optval)
{
    string error_msg;

    if (_io_tcpudp_manager.set_socket_option(IPv4::af(), sockid, optname,
					     optval, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket4_0_1_set_socket_option_txt(
    // Input values,
    const string&	sockid,
    const string&	optname,
    const string&	optval)
{
    string error_msg;

    if (_io_tcpudp_manager.set_socket_option(IPv4::af(), sockid, optname,
					     optval, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

#ifdef HAVE_IPV6
XrlCmdError
XrlFeaTarget::socket6_0_1_tcp_open(
    // Input values,
    const string&	creator,
    // Output values,
    string&		sockid)
{
    string error_msg;

    if (_io_tcpudp_manager.tcp_open(IPv6::af(), creator, sockid, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket6_0_1_udp_open(
    // Input values,
    const string&	creator,
    // Output values,
    string&		sockid)
{
    string error_msg;

    if (_io_tcpudp_manager.udp_open(IPv6::af(), creator, sockid, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket6_0_1_tcp_open_and_bind(
    // Input values,
    const string&	creator,
    const IPv6&		local_addr,
    const uint32_t&	local_port,
    // Output values,
    string&		sockid)
{
    string error_msg;

    if (local_port > 0xffff) {
	error_msg = c_format("Local port %u is out of range", local_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_io_tcpudp_manager.tcp_open_and_bind(IPv6::af(), creator,
					     IPvX(local_addr), local_port,
					     sockid, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket6_0_1_udp_open_and_bind(
    // Input values,
    const string&	creator,
    const IPv6&		local_addr,
    const uint32_t&	local_port,
    const string&       local_dev,
    const uint32_t&     reuse,
    // Output values,
    string&		sockid)
{
    string error_msg;

    if (local_port > 0xffff) {
	error_msg = c_format("Local port %u is out of range", local_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_io_tcpudp_manager.udp_open_and_bind(IPv6::af(), creator,
					     IPvX(local_addr), local_port,
					     local_dev, reuse,
					     sockid, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket6_0_1_udp_open_bind_join(
    // Input values,
    const string&	creator,
    const IPv6&		local_addr,
    const uint32_t&	local_port,
    const IPv6&		mcast_addr,
    const uint32_t&	ttl,
    const bool&		reuse,
    // Output values,
    string&		sockid)
{
    string error_msg;

    if (local_port > 0xffff) {
	error_msg = c_format("Local port %u is out of range", local_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    if (ttl > 0xff) {
	error_msg = c_format("TTL %u is out of range", ttl);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_io_tcpudp_manager.udp_open_bind_join(IPv6::af(), creator,
					      IPvX(local_addr), local_port,
					      IPvX(mcast_addr), ttl, reuse,
					      sockid, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket6_0_1_tcp_open_bind_connect(
    // Input values,
    const string&	creator,
    const IPv6&		local_addr,
    const uint32_t&	local_port,
    const IPv6&		remote_addr,
    const uint32_t&	remote_port,
    // Output values,
    string&		sockid)
{
    string error_msg;

    if (local_port > 0xffff) {
	error_msg = c_format("Local port %u is out of range", local_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    if (remote_port > 0xffff) {
	error_msg = c_format("Remote port %u is out of range", remote_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_io_tcpudp_manager.tcp_open_bind_connect(IPv6::af(), creator,
						 IPvX(local_addr), local_port,
						 IPvX(remote_addr),
						 remote_port, sockid,
						 error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket6_0_1_udp_open_bind_connect(
    // Input values,
    const string&	creator,
    const IPv6&		local_addr,
    const uint32_t&	local_port,
    const IPv6&		remote_addr,
    const uint32_t&	remote_port,
    // Output values,
    string&		sockid)
{
    string error_msg;

    if (local_port > 0xffff) {
	error_msg = c_format("Local port %u is out of range", local_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }
    if (remote_port > 0xffff) {
	error_msg = c_format("Remote port %u is out of range", remote_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_io_tcpudp_manager.udp_open_bind_connect(IPv6::af(), creator,
						 IPvX(local_addr), local_port,
						 IPvX(remote_addr),
						 remote_port, sockid,
						 error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket6_0_1_bind(
    // Input values,
    const string&	sockid,
    const IPv6&		local_addr,
    const uint32_t&	local_port)
{
    string error_msg;

    if (local_port > 0xffff) {
	error_msg = c_format("Local port %u is out of range", local_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_io_tcpudp_manager.bind(IPv6::af(), sockid, IPvX(local_addr),
				local_port, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket6_0_1_udp_join_group(
    // Input values,
    const string&	sockid,
    const IPv6&		mcast_addr,
    const IPv6&		join_if_addr)
{
    string error_msg;

    if (_io_tcpudp_manager.udp_join_group(IPv6::af(), sockid, IPvX(mcast_addr),
					  IPvX(join_if_addr), error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket6_0_1_udp_leave_group(
    // Input values,
    const string&	sockid,
    const IPv6&		mcast_addr,
    const IPv6&		leave_if_addr)
{
    string error_msg;

    if (_io_tcpudp_manager.udp_leave_group(IPv6::af(), sockid,
					   IPvX(mcast_addr),
					   IPvX(leave_if_addr), error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket6_0_1_close(
    // Input values,
    const string&	sockid)
{
    string error_msg;

    if (_io_tcpudp_manager.close(IPv6::af(), sockid, error_msg) != XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket6_0_1_tcp_listen(
    // Input values,
    const string&	sockid,
    const uint32_t&	backlog)
{
    string error_msg;

    if (_io_tcpudp_manager.tcp_listen(IPv6::af(), sockid, backlog, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket6_0_1_send(
    // Input values,
    const string&	sockid,
    const vector<uint8_t>& data)
{
    string error_msg;

    if (_io_tcpudp_manager.send(IPv6::af(), sockid, data, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket6_0_1_send_to(
    // Input values,
    const string&	sockid,
    const IPv6&		remote_addr,
    const uint32_t&	remote_port,
    const vector<uint8_t>& data)
{
    string error_msg;

    if (remote_port > 0xffff) {
	error_msg = c_format("Remote port %u is out of range", remote_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_io_tcpudp_manager.send_to(IPv6::af(), sockid, IPvX(remote_addr),
				   remote_port, data, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket6_0_1_send_from_multicast_if(
    // Input values,
    const string&	sockid,
    const IPv6&		group_addr,
    const uint32_t&	group_port,
    const IPv6&		ifaddr,
    const vector<uint8_t>& data)
{
    string error_msg;

    if (group_port > 0xffff) {
	error_msg = c_format("Multicast group port %u is out of range",
			     group_port);
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    if (_io_tcpudp_manager.send_from_multicast_if(IPv6::af(), sockid,
						  IPvX(group_addr), group_port,
						  ifaddr, data, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlFeaTarget::socket6_0_1_set_socket_option(
    // Input values,
    const string&	sockid,
    const string&	optname,
    const uint32_t&	optval)
{
    string error_msg;

    if (_io_tcpudp_manager.set_socket_option(IPv6::af(), sockid, optname,
					     optval, error_msg)
	!= XORP_OK) {
	return XrlCmdError::COMMAND_FAILED(error_msg);
    }

    return XrlCmdError::OKAY();
}
#endif

#ifndef XORP_DISABLE_PROFILE
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

#endif
