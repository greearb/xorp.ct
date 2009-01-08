// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/fea/data_plane/managers/fea_data_plane_manager_click.cc,v 1.10 2008/10/02 21:57:12 bms Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/ifconfig.hh"
#include "fea/fibconfig.hh"
#include "fea/firewall_manager.hh"

#include "fea/data_plane/ifconfig/ifconfig_property_dummy.hh"
#include "fea/data_plane/ifconfig/ifconfig_get_click.hh"
#include "fea/data_plane/ifconfig/ifconfig_set_click.hh"
#include "fea/data_plane/firewall/firewall_get_dummy.hh"
#include "fea/data_plane/firewall/firewall_set_dummy.hh"
#include "fea/data_plane/fibconfig/fibconfig_forwarding_dummy.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_get_click.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_set_click.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_get_click.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_set_click.hh"
#include "fea/data_plane/io/io_link_dummy.hh"
#include "fea/data_plane/io/io_ip_dummy.hh"
#include "fea/data_plane/io/io_tcpudp_dummy.hh"

#include "fea_data_plane_manager_click.hh"


//
// FEA data plane manager class for Click:
//   http://www.read.cs.ucla.edu/click/
//

#if 0
extern "C" FeaDataPlaneManager* create(FeaNode& fea_node)
{
    return (new FeaDataPlaneManagerClick(fea_node));
}

extern "C" void destroy(FeaDataPlaneManager* fea_data_plane_manager)
{
    delete fea_data_plane_manager;
}
#endif // 0


FeaDataPlaneManagerClick::FeaDataPlaneManagerClick(FeaNode& fea_node)
    : FeaDataPlaneManager(fea_node, "Click"),
      _ifconfig_property_dummy(NULL),
      _ifconfig_get_click(NULL),
      _ifconfig_set_click(NULL),
      _firewall_get_dummy(NULL),
      _firewall_set_dummy(NULL),
      _fibconfig_forwarding_dummy(NULL),
      _fibconfig_entry_get_click(NULL),
      _fibconfig_entry_set_click(NULL),
      _fibconfig_table_get_click(NULL),
      _fibconfig_table_set_click(NULL)
{
}

FeaDataPlaneManagerClick::~FeaDataPlaneManagerClick()
{
}

int
FeaDataPlaneManagerClick::load_plugins(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_loaded_plugins)
	return (XORP_OK);

    XLOG_ASSERT(_ifconfig_property_dummy == NULL);
    XLOG_ASSERT(_ifconfig_get_click == NULL);
    XLOG_ASSERT(_ifconfig_set_click == NULL);
    XLOG_ASSERT(_firewall_get_dummy == NULL);
    XLOG_ASSERT(_firewall_set_dummy == NULL);
    XLOG_ASSERT(_fibconfig_forwarding_dummy == NULL);
    XLOG_ASSERT(_fibconfig_entry_get_click == NULL);
    XLOG_ASSERT(_fibconfig_entry_set_click == NULL);
    XLOG_ASSERT(_fibconfig_table_get_click == NULL);
    XLOG_ASSERT(_fibconfig_table_set_click == NULL);

    //
    // Load the plugins
    //
    //
    // TODO: XXX: For the time being some of the plugins
    // used by Click are dummy.
    //
    _ifconfig_property_dummy = new IfConfigPropertyDummy(*this);
    _ifconfig_get_click = new IfConfigGetClick(*this);
    _ifconfig_set_click = new IfConfigSetClick(*this);
    //
    _firewall_get_dummy = new FirewallGetDummy(*this);
    _firewall_set_dummy = new FirewallSetDummy(*this);
    //
    _fibconfig_forwarding_dummy = new FibConfigForwardingDummy(*this);
    _fibconfig_entry_get_click = new FibConfigEntryGetClick(*this);
    _fibconfig_entry_set_click = new FibConfigEntrySetClick(*this);
    _fibconfig_table_get_click = new FibConfigTableGetClick(*this);
    _fibconfig_table_set_click = new FibConfigTableSetClick(*this);
    //
    _ifconfig_property = _ifconfig_property_dummy;
    _ifconfig_get = _ifconfig_get_click;
    _ifconfig_set = _ifconfig_set_click;
    _firewall_get = _firewall_get_dummy;
    _firewall_set = _firewall_set_dummy;
    _fibconfig_forwarding = _fibconfig_forwarding_dummy;
    _fibconfig_entry_get = _fibconfig_entry_get_click;
    _fibconfig_entry_set = _fibconfig_entry_set_click;
    _fibconfig_table_get = _fibconfig_table_get_click;
    _fibconfig_table_set = _fibconfig_table_set_click;

    _is_loaded_plugins = true;

    return (XORP_OK);
}

int
FeaDataPlaneManagerClick::unload_plugins(string& error_msg)
{
    if (! _is_loaded_plugins)
	return (XORP_OK);

    XLOG_ASSERT(_ifconfig_property_dummy != NULL);
    XLOG_ASSERT(_ifconfig_get_click != NULL);
    XLOG_ASSERT(_ifconfig_set_click != NULL);
    XLOG_ASSERT(_firewall_get_dummy != NULL);
    XLOG_ASSERT(_firewall_set_dummy != NULL);
    XLOG_ASSERT(_fibconfig_forwarding_dummy != NULL);
    XLOG_ASSERT(_fibconfig_entry_get_click != NULL);
    XLOG_ASSERT(_fibconfig_entry_set_click != NULL);
    XLOG_ASSERT(_fibconfig_table_get_click != NULL);
    XLOG_ASSERT(_fibconfig_table_set_click != NULL);

    //
    // Unload the plugins
    //
    if (FeaDataPlaneManager::unload_plugins(error_msg) != XORP_OK)
	return (XORP_ERROR);

    //
    // Reset the state
    //
    _ifconfig_property_dummy = NULL;
    _ifconfig_get_click = NULL;
    _ifconfig_set_click = NULL;
    _firewall_get_dummy = NULL;
    _firewall_set_dummy = NULL;
    _fibconfig_forwarding_dummy = NULL;
    _fibconfig_entry_get_click = NULL;
    _fibconfig_entry_set_click = NULL;
    _fibconfig_table_get_click = NULL;
    _fibconfig_table_set_click = NULL;

    return (XORP_OK);
}

int
FeaDataPlaneManagerClick::register_plugins(string& error_msg)
{
    string dummy_error_msg;

    if (_ifconfig_property != NULL) {
	if (ifconfig().register_ifconfig_property(_ifconfig_property, false) != XORP_OK) {
	    error_msg = c_format("Cannot register IfConfigProperty plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }

    if (_ifconfig_get != NULL) {
	if (ifconfig().register_ifconfig_get(_ifconfig_get, false) != XORP_OK) {
	    error_msg = c_format("Cannot register IfConfigGet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_ifconfig_set != NULL) {
	if (ifconfig().register_ifconfig_set(_ifconfig_set, false) != XORP_OK) {
	    error_msg = c_format("Cannot register IfConfigSet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_firewall_get != NULL) {
	if (firewall_manager().register_firewall_get(_firewall_get, false) != XORP_OK) {
	    error_msg = c_format("Cannot register FirewallGet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_firewall_set != NULL) {
	if (firewall_manager().register_firewall_set(_firewall_set, false) != XORP_OK) {
	    error_msg = c_format("Cannot register FirewallSet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_fibconfig_forwarding != NULL) {
	if (fibconfig().register_fibconfig_forwarding(_fibconfig_forwarding, false) != XORP_OK) {
	    error_msg = c_format("Cannot register FibConfigForwarding plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_fibconfig_entry_get != NULL) {
	//
	// XXX: We register the FibConfigEntryGet plugin as the exclusive "get"
	// method, because Click should be the ultimate place to read the route
	// info from.
	// The kernel itself may contain some left-over stuff.
	//
	if (fibconfig().register_fibconfig_entry_get(_fibconfig_entry_get,
						     true)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register FibConfigEntryGet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_fibconfig_entry_set != NULL) {
	XLOG_ASSERT(_fibconfig_entry_set_click != NULL);
	bool is_exclusive = true;
	if (_fibconfig_entry_set_click->is_duplicate_routes_to_kernel_enabled())
	    is_exclusive = false;
	if (fibconfig().register_fibconfig_entry_set(_fibconfig_entry_set,
						     is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register FibConfigEntrySet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_fibconfig_table_get != NULL) {
	//
	// XXX: We register the FibConfigTableGet plugin as the exclusive "get"
	// method, because Click should be the ultimate place to read the route
	// info from.
	// The kernel itself may contain some left-over stuff.
	//
	if (fibconfig().register_fibconfig_table_get(_fibconfig_table_get,
						     true)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register FibConfigTableGet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_fibconfig_table_set != NULL) {
	XLOG_ASSERT(_fibconfig_table_set_click != NULL);
	bool is_exclusive = true;
	if (_fibconfig_table_set_click->is_duplicate_routes_to_kernel_enabled())
	    is_exclusive = false;
	if (fibconfig().register_fibconfig_table_set(_fibconfig_table_set,
						     is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register FibConfigTableSet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }

    return (XORP_OK);
}

IoLink*
FeaDataPlaneManagerClick::allocate_io_link(const IfTree& iftree,
					   const string& if_name,
					   const string& vif_name,
					   uint16_t ether_type,
					   const string& filter_program)
{
    IoLink* io_link = NULL;

    //
    // TODO: XXX: For the time being Click uses the IoLinkDummy plugin.
    //
    io_link = new IoLinkDummy(*this, iftree, if_name, vif_name, ether_type,
			      filter_program);
    _io_link_list.push_back(io_link);

    return (io_link);
}

IoIp*
FeaDataPlaneManagerClick::allocate_io_ip(const IfTree& iftree, int family,
					 uint8_t ip_protocol)
{
    IoIp* io_ip = NULL;

    //
    // TODO: XXX: For the time being Click uses the IoIpDummy plugin.
    //
    io_ip = new IoIpDummy(*this, iftree, family, ip_protocol);
    _io_ip_list.push_back(io_ip);

    return (io_ip);
}

IoTcpUdp*
FeaDataPlaneManagerClick::allocate_io_tcpudp(const IfTree& iftree, int family,
					     bool is_tcp)
{
    IoTcpUdp* io_tcpudp = NULL;

    //
    // TODO: XXX: For the time being Click uses the IoTcpUdpDummy plugin.
    //
    io_tcpudp = new IoTcpUdpDummy(*this, iftree, family, is_tcp);
    _io_tcpudp_list.push_back(io_tcpudp);

    return (io_tcpudp);
}

int
FeaDataPlaneManagerClick::enable_click(bool enable, string& error_msg)
{
    if (! _is_loaded_plugins) {
	error_msg = c_format("Data plane manager %s plugins are not loaded",
			     manager_name().c_str());
	return (XORP_ERROR);
    }

    _ifconfig_get_click->enable_click(enable);
    _ifconfig_set_click->enable_click(enable);
    _fibconfig_entry_get_click->enable_click(enable);
    _fibconfig_entry_set_click->enable_click(enable);
    _fibconfig_table_get_click->enable_click(enable);
    _fibconfig_table_set_click->enable_click(enable);

    return (XORP_OK);
}

int
FeaDataPlaneManagerClick::enable_kernel_click(bool enable, string& error_msg)
{
    if (! _is_loaded_plugins) {
	error_msg = c_format("Data plane manager %s plugins are not loaded",
			     manager_name().c_str());
	return (XORP_ERROR);
    }

    _ifconfig_get_click->enable_kernel_click(enable);
    _ifconfig_set_click->enable_kernel_click(enable);
    _fibconfig_entry_get_click->enable_kernel_click(enable);
    _fibconfig_entry_set_click->enable_kernel_click(enable);
    _fibconfig_table_get_click->enable_kernel_click(enable);
    _fibconfig_table_set_click->enable_kernel_click(enable);

    return (XORP_OK);
}

int
FeaDataPlaneManagerClick::set_kernel_click_config_generator_file(
    const string& v,
    string& error_msg)
{
    if (! _is_loaded_plugins) {
	error_msg = c_format("Data plane manager %s plugins are not loaded",
			     manager_name().c_str());
	return (XORP_ERROR);
    }

    _ifconfig_get_click->set_kernel_click_config_generator_file(v);
    _ifconfig_set_click->set_kernel_click_config_generator_file(v);
    _fibconfig_entry_get_click->set_kernel_click_config_generator_file(v);
    _fibconfig_entry_set_click->set_kernel_click_config_generator_file(v);
    _fibconfig_table_get_click->set_kernel_click_config_generator_file(v);
    _fibconfig_table_set_click->set_kernel_click_config_generator_file(v);

    return (XORP_OK);
}

int
FeaDataPlaneManagerClick::enable_duplicate_routes_to_kernel(bool enable,
							    string& error_msg)
{
    if (! _is_loaded_plugins) {
	error_msg = c_format("Data plane manager %s plugins are not loaded",
			     manager_name().c_str());
	return (XORP_ERROR);
    }

    _ifconfig_get_click->enable_duplicate_routes_to_kernel(enable);
    _ifconfig_set_click->enable_duplicate_routes_to_kernel(enable);
    _fibconfig_entry_get_click->enable_duplicate_routes_to_kernel(enable);
    _fibconfig_entry_set_click->enable_duplicate_routes_to_kernel(enable);
    _fibconfig_table_get_click->enable_duplicate_routes_to_kernel(enable);
    _fibconfig_table_set_click->enable_duplicate_routes_to_kernel(enable);

    return (XORP_OK);
}

int
FeaDataPlaneManagerClick::enable_kernel_click_install_on_startup(bool enable,
								 string& error_msg)
{
    if (! _is_loaded_plugins) {
	error_msg = c_format("Data plane manager %s plugins are not loaded",
			     manager_name().c_str());
	return (XORP_ERROR);
    }

    // XXX: only IfConfigGet should install the kernel-level Click
    _ifconfig_get_click->enable_kernel_click_install_on_startup(enable);
    _ifconfig_set_click->enable_kernel_click_install_on_startup(false);
    _fibconfig_entry_get_click->enable_kernel_click_install_on_startup(false);
    _fibconfig_entry_set_click->enable_kernel_click_install_on_startup(false);
    _fibconfig_table_get_click->enable_kernel_click_install_on_startup(false);
    _fibconfig_table_set_click->enable_kernel_click_install_on_startup(false);

    return (XORP_OK);
}

int
FeaDataPlaneManagerClick::set_kernel_click_modules(const list<string>& modules,
						   string& error_msg)
{
    if (! _is_loaded_plugins) {
	error_msg = c_format("Data plane manager %s plugins are not loaded",
			     manager_name().c_str());
	return (XORP_ERROR);
    }

    _ifconfig_get_click->set_kernel_click_modules(modules);
    _ifconfig_set_click->set_kernel_click_modules(modules);
    _fibconfig_entry_get_click->set_kernel_click_modules(modules);
    _fibconfig_entry_set_click->set_kernel_click_modules(modules);
    _fibconfig_table_get_click->set_kernel_click_modules(modules);
    _fibconfig_table_set_click->set_kernel_click_modules(modules);

    return (XORP_OK);
}

int
FeaDataPlaneManagerClick::set_kernel_click_mount_directory(
    const string& directory,
    string& error_msg)
{
    if (! _is_loaded_plugins) {
	error_msg = c_format("Data plane manager %s plugins are not loaded",
			     manager_name().c_str());
	return (XORP_ERROR);
    }

    _ifconfig_get_click->set_kernel_click_mount_directory(directory);
    _ifconfig_set_click->set_kernel_click_mount_directory(directory);
    _fibconfig_entry_get_click->set_kernel_click_mount_directory(directory);
    _fibconfig_entry_set_click->set_kernel_click_mount_directory(directory);
    _fibconfig_table_get_click->set_kernel_click_mount_directory(directory);
    _fibconfig_table_set_click->set_kernel_click_mount_directory(directory);

    return (XORP_OK);
}

int
FeaDataPlaneManagerClick::enable_user_click(bool enable, string& error_msg)
{
    if (! _is_loaded_plugins) {
	error_msg = c_format("Data plane manager %s plugins are not loaded",
			     manager_name().c_str());
	return (XORP_ERROR);
    }

    _ifconfig_get_click->enable_user_click(enable);
    _ifconfig_set_click->enable_user_click(enable);
    _fibconfig_entry_get_click->enable_user_click(enable);
    _fibconfig_entry_set_click->enable_user_click(enable);
    _fibconfig_table_get_click->enable_user_click(enable);
    _fibconfig_table_set_click->enable_user_click(enable);

    return (XORP_OK);
}

int
FeaDataPlaneManagerClick::set_user_click_command_file(const string& v,
						      string& error_msg)
{
    if (! _is_loaded_plugins) {
	error_msg = c_format("Data plane manager %s plugins are not loaded",
			     manager_name().c_str());
	return (XORP_ERROR);
    }

    _ifconfig_get_click->set_user_click_command_file(v);
    _ifconfig_set_click->set_user_click_command_file(v);
    _fibconfig_entry_get_click->set_user_click_command_file(v);
    _fibconfig_entry_set_click->set_user_click_command_file(v);
    _fibconfig_table_get_click->set_user_click_command_file(v);
    _fibconfig_table_set_click->set_user_click_command_file(v);

    return (XORP_OK);
}

int
FeaDataPlaneManagerClick::set_user_click_command_extra_arguments(
    const string& v,
    string& error_msg)
{
    if (! _is_loaded_plugins) {
	error_msg = c_format("Data plane manager %s plugins are not loaded",
			     manager_name().c_str());
	return (XORP_ERROR);
    }

    _ifconfig_get_click->set_user_click_command_extra_arguments(v);
    _ifconfig_set_click->set_user_click_command_extra_arguments(v);
    _fibconfig_entry_get_click->set_user_click_command_extra_arguments(v);
    _fibconfig_entry_set_click->set_user_click_command_extra_arguments(v);
    _fibconfig_table_get_click->set_user_click_command_extra_arguments(v);
    _fibconfig_table_set_click->set_user_click_command_extra_arguments(v);

    return (XORP_OK);
}

int
FeaDataPlaneManagerClick::set_user_click_command_execute_on_startup(
    bool v,
    string& error_msg)
{
    if (! _is_loaded_plugins) {
	error_msg = c_format("Data plane manager %s plugins are not loaded",
			     manager_name().c_str());
	return (XORP_ERROR);
    }

    // XXX: only IfConfigGet should execute the user-level Click command
    _ifconfig_get_click->set_user_click_command_execute_on_startup(v);
    _ifconfig_set_click->set_user_click_command_execute_on_startup(false);
    _fibconfig_entry_get_click->set_user_click_command_execute_on_startup(false);
    _fibconfig_entry_set_click->set_user_click_command_execute_on_startup(false);
    _fibconfig_table_get_click->set_user_click_command_execute_on_startup(false);
    _fibconfig_table_set_click->set_user_click_command_execute_on_startup(false);

    return (XORP_OK);
}

int
FeaDataPlaneManagerClick::set_user_click_control_address(const IPv4& v,
							 string& error_msg)
{
    if (! _is_loaded_plugins) {
	error_msg = c_format("Data plane manager %s plugins are not loaded",
			     manager_name().c_str());
	return (XORP_ERROR);
    }

    _ifconfig_get_click->set_user_click_control_address(v);
    _ifconfig_set_click->set_user_click_control_address(v);
    _fibconfig_entry_get_click->set_user_click_control_address(v);
    _fibconfig_entry_set_click->set_user_click_control_address(v);
    _fibconfig_table_get_click->set_user_click_control_address(v);
    _fibconfig_table_set_click->set_user_click_control_address(v);

    return (XORP_OK);
}

int
FeaDataPlaneManagerClick::set_user_click_control_socket_port(uint32_t v,
							     string& error_msg)
{
    if (! _is_loaded_plugins) {
	error_msg = c_format("Data plane manager %s plugins are not loaded",
			     manager_name().c_str());
	return (XORP_ERROR);
    }

    _ifconfig_get_click->set_user_click_control_socket_port(v);
    _ifconfig_set_click->set_user_click_control_socket_port(v);
    _fibconfig_entry_get_click->set_user_click_control_socket_port(v);
    _fibconfig_entry_set_click->set_user_click_control_socket_port(v);
    _fibconfig_table_get_click->set_user_click_control_socket_port(v);
    _fibconfig_table_set_click->set_user_click_control_socket_port(v);

    return (XORP_OK);
}

int
FeaDataPlaneManagerClick::set_user_click_startup_config_file(const string& v,
							     string& error_msg)
{
    if (! _is_loaded_plugins) {
	error_msg = c_format("Data plane manager %s plugins are not loaded",
			     manager_name().c_str());
	return (XORP_ERROR);
    }

    _ifconfig_get_click->set_user_click_startup_config_file(v);
    _ifconfig_set_click->set_user_click_startup_config_file(v);
    _fibconfig_entry_get_click->set_user_click_startup_config_file(v);
    _fibconfig_entry_set_click->set_user_click_startup_config_file(v);
    _fibconfig_table_get_click->set_user_click_startup_config_file(v);
    _fibconfig_table_set_click->set_user_click_startup_config_file(v);

    return (XORP_OK);
}

int
FeaDataPlaneManagerClick::set_user_click_config_generator_file(const string& v,
							       string& error_msg)
{
    if (! _is_loaded_plugins) {
	error_msg = c_format("Data plane manager %s plugins are not loaded",
			     manager_name().c_str());
	return (XORP_ERROR);
    }

    _ifconfig_get_click->set_user_click_config_generator_file(v);
    _ifconfig_set_click->set_user_click_config_generator_file(v);
    _fibconfig_entry_get_click->set_user_click_config_generator_file(v);
    _fibconfig_entry_set_click->set_user_click_config_generator_file(v);
    _fibconfig_table_get_click->set_user_click_config_generator_file(v);
    _fibconfig_table_set_click->set_user_click_config_generator_file(v);

    return (XORP_OK);
}
