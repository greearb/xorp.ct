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



#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea_node.hh"
#include "fea_data_plane_manager.hh"


//
// FEA data plane manager base class implementation.
//

FeaDataPlaneManager::FeaDataPlaneManager(FeaNode& fea_node,
					 const string& manager_name)
    : _fea_node(fea_node),
      _ifconfig_property(NULL),
      _ifconfig_get(NULL),
      _ifconfig_set(NULL),
      _ifconfig_observer(NULL),
      _ifconfig_vlan_get(NULL),
      _ifconfig_vlan_set(NULL),
      _firewall_get(NULL),
      _firewall_set(NULL),
      _fibconfig_forwarding(NULL),
      _fibconfig_entry_get(NULL),
      _fibconfig_entry_set(NULL),
      _fibconfig_entry_observer(NULL),
      _fibconfig_table_get(NULL),
      _fibconfig_table_set(NULL),
      _fibconfig_table_observer(NULL),
      _manager_name(manager_name),
      _is_loaded_plugins(false),
      _is_running_manager(false),
      _is_running_plugins(false)
{
}

FeaDataPlaneManager::~FeaDataPlaneManager()
{
    string error_msg;

    if (stop_manager(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop data plane manager %s: %s",
		   manager_name().c_str(), error_msg.c_str());
    }
}

int
FeaDataPlaneManager::start_manager(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_running_manager)
	return (XORP_OK);

    if (load_plugins(error_msg) != XORP_OK)
	return (XORP_ERROR);

    _is_running_manager = true;

    return (XORP_OK);
}

int
FeaDataPlaneManager::stop_manager(string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (! _is_running_manager)
	return (XORP_OK);

    if (unload_plugins(error_msg2) != XORP_OK) {
	ret_value = XORP_ERROR;
	if (! error_msg.empty())
	    error_msg += " ";
	error_msg += error_msg2;
    }

    _is_running_manager = false;

    return (ret_value);
}

int
FeaDataPlaneManager::unload_plugins(string& error_msg)
{
    string error_msg2;

    UNUSED(error_msg);

    if (! _is_loaded_plugins)
	return (XORP_OK);

    if (stop_plugins(error_msg2) != XORP_OK) {
	XLOG_WARNING("Error during unloading the plugins for %s data plane "
		     "manager while stopping the plugins: %s. "
		     "Error ignored.",
		     manager_name().c_str(),
		     error_msg2.c_str());
    }

    //
    // Unload the plugins
    //

    //
    // IfConfig plugins
    //
    if (_ifconfig_property != NULL) {
	delete _ifconfig_property;
	_ifconfig_property = NULL;
    }
    if (_ifconfig_get != NULL) {
	delete _ifconfig_get;
	_ifconfig_get = NULL;
    }
    if (_ifconfig_set != NULL) {
	delete _ifconfig_set;
	_ifconfig_set = NULL;
    }
    if (_ifconfig_observer != NULL) {
	delete _ifconfig_observer;
	_ifconfig_observer = NULL;
    }
    if (_ifconfig_vlan_get != NULL) {
	delete _ifconfig_vlan_get;
	_ifconfig_vlan_get = NULL;
    }
    if (_ifconfig_vlan_set != NULL) {
	delete _ifconfig_vlan_set;
	_ifconfig_vlan_set = NULL;
    }

    //
    // Firewall plugins
    //
    if (_firewall_get != NULL) {
	delete _firewall_get;
	_firewall_get = NULL;
    }
    if (_firewall_set != NULL) {
	delete _firewall_set;
	_firewall_set = NULL;
    }

    //
    // FibConfig plugins
    //
    if (_fibconfig_forwarding != NULL) {
	delete _fibconfig_forwarding;
	_fibconfig_forwarding = NULL;
    }
    if (_fibconfig_entry_get != NULL) {
	delete _fibconfig_entry_get;
	_fibconfig_entry_get = NULL;
    }
    if (_fibconfig_entry_set != NULL) {
	delete _fibconfig_entry_set;
	_fibconfig_entry_set = NULL;
    }
    if (_fibconfig_entry_observer != NULL) {
	delete _fibconfig_entry_observer;
	_fibconfig_entry_observer = NULL;
    }
    if (_fibconfig_table_get != NULL) {
	delete _fibconfig_table_get;
	_fibconfig_table_get = NULL;
    }
    if (_fibconfig_table_set != NULL) {
	delete _fibconfig_table_set;
	_fibconfig_table_set = NULL;
    }
    if (_fibconfig_table_observer != NULL) {
	delete _fibconfig_table_observer;
	_fibconfig_table_observer = NULL;
    }

    //
    // I/O plugins
    //
    delete_pointers_list(_io_link_list);
    delete_pointers_list(_io_ip_list);
    delete_pointers_list(_io_tcpudp_list);

    _is_loaded_plugins = false;

    return (XORP_OK);
}

int
FeaDataPlaneManager::unregister_plugins(string& error_msg)
{
    UNUSED(error_msg);

    //
    // Unregister the plugins in the reverse order they were registered
    //

    //
    // I/O plugins
    //
    io_link_manager().unregister_data_plane_manager(this);
    io_ip_manager().unregister_data_plane_manager(this);
    io_tcpudp_manager().unregister_data_plane_manager(this);

    //
    // FibConfig plugins
    //
    if (_fibconfig_table_observer != NULL)
	fibconfig().unregister_fibconfig_table_observer(_fibconfig_table_observer);
    if (_fibconfig_table_set != NULL)
	fibconfig().unregister_fibconfig_table_set(_fibconfig_table_set);
    if (_fibconfig_table_get != NULL)
	fibconfig().unregister_fibconfig_table_get(_fibconfig_table_get);
    if (_fibconfig_entry_observer != NULL)
	fibconfig().unregister_fibconfig_entry_observer(_fibconfig_entry_observer);
    if (_fibconfig_entry_set != NULL)
	fibconfig().unregister_fibconfig_entry_set(_fibconfig_entry_set);
    if (_fibconfig_entry_get != NULL)
	fibconfig().unregister_fibconfig_entry_get(_fibconfig_entry_get);
    if (_fibconfig_forwarding != NULL)
	fibconfig().unregister_fibconfig_forwarding(_fibconfig_forwarding);

    //
    // Firewall plugins
    //
    if (_firewall_set != NULL)
	firewall_manager().unregister_firewall_set(_firewall_set);
    if (_firewall_get != NULL)
	firewall_manager().unregister_firewall_get(_firewall_get);

    //
    // IfConfig plugins
    //
    if (_ifconfig_vlan_set != NULL)
	ifconfig().unregister_ifconfig_vlan_set(_ifconfig_vlan_set);
    if (_ifconfig_vlan_get != NULL)
	ifconfig().unregister_ifconfig_vlan_get(_ifconfig_vlan_get);
    if (_ifconfig_observer != NULL)
	ifconfig().unregister_ifconfig_observer(_ifconfig_observer);
    if (_ifconfig_set != NULL)
	ifconfig().unregister_ifconfig_set(_ifconfig_set);
    if (_ifconfig_get != NULL)
	ifconfig().unregister_ifconfig_get(_ifconfig_get);
    if (_ifconfig_property != NULL)
	ifconfig().unregister_ifconfig_property(_ifconfig_property);

    return (XORP_OK);
}

int
FeaDataPlaneManager::start_plugins(string& error_msg)
{
    string dummy_error_msg;
    list<IoLink *>::iterator link_iter;
    list<IoIp *>::iterator ip_iter;
    list<IoTcpUdp *>::iterator tcpudp_iter;

    if (_is_running_plugins)
	return (XORP_OK);

    if (! _is_loaded_plugins) {
	error_msg = c_format("Data plane manager %s plugins are not loaded",
			     manager_name().c_str());
	return (XORP_ERROR);
    }

    if (register_plugins(error_msg) != XORP_OK) {
	error_msg = c_format("Cannot register plugins for data plane "
			     "manager %s: %s",
			     manager_name().c_str(),
			     error_msg.c_str());
	return (XORP_ERROR);
    }

    //
    // IfConfig plugins
    //
    if (_ifconfig_property != NULL) {
	if (_ifconfig_property->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    if (_ifconfig_get != NULL) {
	if (_ifconfig_get->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    if (_ifconfig_set != NULL) {
	if (_ifconfig_set->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    if (_ifconfig_observer != NULL) {
	if (_ifconfig_observer->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    if (_ifconfig_vlan_get != NULL) {
	if (_ifconfig_vlan_get->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    if (_ifconfig_vlan_set != NULL) {
	if (_ifconfig_vlan_set->start(error_msg) != XORP_OK)
	    goto error_label;
    }

    //
    // Firewall plugins
    //
    if (_firewall_get != NULL) {
	if (_firewall_get->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    if (_firewall_set != NULL) {
	if (_firewall_set->start(error_msg) != XORP_OK)
	    goto error_label;
    }

    //
    // FibConfig plugins
    //
    if (_fibconfig_forwarding != NULL) {
	if (_fibconfig_forwarding->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    if (_fibconfig_entry_get != NULL) {
	if (_fibconfig_entry_get->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    if (_fibconfig_entry_set != NULL) {
	if (_fibconfig_entry_set->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    if (_fibconfig_entry_observer != NULL) {
	if (_fibconfig_entry_observer->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    if (_fibconfig_table_get != NULL) {
	if (_fibconfig_table_get->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    if (_fibconfig_table_set != NULL) {
	if (_fibconfig_table_set->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    if (_fibconfig_table_observer != NULL) {
	if (_fibconfig_table_observer->start(error_msg) != XORP_OK)
	    goto error_label;
    }

    //
    // I/O plugins
    //
    for (link_iter = _io_link_list.begin();
	 link_iter != _io_link_list.end();
	 ++link_iter) {
	IoLink* io_link = *link_iter;
	if (io_link->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    for (ip_iter = _io_ip_list.begin();
	 ip_iter != _io_ip_list.end();
	 ++ip_iter) {
	IoIp* io_ip = *ip_iter;
	if (io_ip->start(error_msg) != XORP_OK)
	    goto error_label;
    }
    for (tcpudp_iter = _io_tcpudp_list.begin();
	 tcpudp_iter != _io_tcpudp_list.end();
	 ++tcpudp_iter) {
	IoTcpUdp* io_tcpudp = *tcpudp_iter;
	if (io_tcpudp->start(error_msg) != XORP_OK)
	    goto error_label;
    }

    _is_running_plugins = true;

    return (XORP_OK);

 error_label:
    stop_all_plugins(dummy_error_msg);
    unregister_plugins(dummy_error_msg);
    return (XORP_ERROR);
}

int
FeaDataPlaneManager::stop_plugins(string& error_msg)
{
    int ret_value = XORP_OK;
    string error_msg2;

    if (! _is_running_plugins)
	return (XORP_OK);

    error_msg.erase();

    //
    // Stop the plugins
    //
    if (stop_all_plugins(error_msg2) != XORP_OK) {
	ret_value = XORP_ERROR;
	if (! error_msg.empty())
	    error_msg += " ";
	error_msg += error_msg2;
    }

    unregister_plugins(error_msg2);

    _is_running_plugins = false;

    return (ret_value);
}

int
FeaDataPlaneManager::register_all_plugins(bool is_exclusive, string& error_msg)
{
    string dummy_error_msg;

    //
    // IfConfig plugins
    //
    if (_ifconfig_property != NULL) {
	if (ifconfig().register_ifconfig_property(_ifconfig_property,
						  is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register IfConfigProperty plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_ifconfig_get != NULL) {
	if (ifconfig().register_ifconfig_get(_ifconfig_get, is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register IfConfigGet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_ifconfig_set != NULL) {
	if (ifconfig().register_ifconfig_set(_ifconfig_set, is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register IfConfigSet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_ifconfig_observer != NULL) {
	if (ifconfig().register_ifconfig_observer(_ifconfig_observer,
						  is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register IfConfigObserver plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_ifconfig_vlan_get != NULL) {
	if (ifconfig().register_ifconfig_vlan_get(_ifconfig_vlan_get,
						  is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register IfConfigVlanGet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_ifconfig_vlan_set != NULL) {
	if (ifconfig().register_ifconfig_vlan_set(_ifconfig_vlan_set,
						  is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register IfConfigVlanSet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }

    //
    // Firewall plugins
    //
    if (_firewall_get != NULL) {
	if (firewall_manager().register_firewall_get(_firewall_get,
						     is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register FirewallGet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_firewall_set != NULL) {
	if (firewall_manager().register_firewall_set(_firewall_set,
						     is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register FirewallSet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }

    //
    // FibConfig plugins
    //
    if (_fibconfig_forwarding != NULL) {
	if (fibconfig().register_fibconfig_forwarding(_fibconfig_forwarding,
						      is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register FibConfigForwarding plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_fibconfig_entry_get != NULL) {
	if (fibconfig().register_fibconfig_entry_get(_fibconfig_entry_get,
						     is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register FibConfigEntryGet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_fibconfig_entry_set != NULL) {
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
    if (_fibconfig_entry_observer != NULL) {
	if (fibconfig().register_fibconfig_entry_observer(_fibconfig_entry_observer,
							  is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register FibConfigEntryObserver plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_fibconfig_table_get != NULL) {
	if (fibconfig().register_fibconfig_table_get(_fibconfig_table_get,
						     is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register FibConfigTableGet plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }
    if (_fibconfig_table_set != NULL) {
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
    if (_fibconfig_table_observer != NULL) {
	if (fibconfig().register_fibconfig_table_observer(_fibconfig_table_observer,
							  is_exclusive)
	    != XORP_OK) {
	    error_msg = c_format("Cannot register FibConfigTableObserver plugin "
				 "for data plane manager %s",
				 manager_name().c_str());
	    unregister_plugins(dummy_error_msg);
	    return (XORP_ERROR);
	}
    }

    //
    // XXX: The I/O plugins (IoLink, IoIp and IoTcpUdp) are registered
    // on-demand when a new client is registered.
    //

    return (XORP_OK);
}

int
FeaDataPlaneManager::stop_all_plugins(string& error_msg)
{
    list<IoLink *>::iterator link_iter;
    list<IoIp *>::iterator ip_iter;
    list<IoTcpUdp *>::iterator tcpudp_iter;
    int ret_value = XORP_OK;
    string error_msg2;

    error_msg.erase();

    //
    // XXX: Stop the plugins in the reverse order they were started
    //

    //
    // I/O plugins
    //
    for (tcpudp_iter = _io_tcpudp_list.begin();
	 tcpudp_iter != _io_tcpudp_list.end();
	 ++tcpudp_iter) {
	IoTcpUdp* io_tcpudp = *tcpudp_iter;
	if (io_tcpudp->stop(error_msg) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    for (ip_iter = _io_ip_list.begin();
	 ip_iter != _io_ip_list.end();
	 ++ip_iter) {
	IoIp* io_ip = *ip_iter;
	if (io_ip->stop(error_msg) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    for (link_iter = _io_link_list.begin();
	 link_iter != _io_link_list.end();
	 ++link_iter) {
	IoLink* io_link = *link_iter;
	if (io_link->stop(error_msg) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // FibConfig plugins
    //
    if (_fibconfig_table_observer != NULL) {
	if (_fibconfig_table_observer->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_fibconfig_table_set != NULL) {
	if (_fibconfig_table_set->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_fibconfig_table_get != NULL) {
	if (_fibconfig_table_get->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_fibconfig_entry_observer != NULL) {
	if (_fibconfig_entry_observer->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_fibconfig_entry_set != NULL) {
	if (_fibconfig_entry_set->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_fibconfig_entry_get != NULL) {
	if (_fibconfig_entry_get->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_fibconfig_forwarding != NULL) {
	if (_fibconfig_forwarding->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // Firewall plugins
    //
    if (_firewall_set != NULL) {
	if (_firewall_set->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_firewall_get != NULL) {
	if (_firewall_get->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // IfConfig plugins
    //
    if (_ifconfig_vlan_set != NULL) {
	if (_ifconfig_vlan_set->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_ifconfig_vlan_get != NULL) {
	if (_ifconfig_vlan_get->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_ifconfig_observer != NULL) {
	if (_ifconfig_observer->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_ifconfig_set != NULL) {
	if (_ifconfig_set->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_ifconfig_get != NULL) {
	if (_ifconfig_get->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    if (_ifconfig_property != NULL) {
	if (_ifconfig_property->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    return (ret_value);
}

EventLoop&
FeaDataPlaneManager::eventloop()
{
    return (_fea_node.eventloop());
}

bool
FeaDataPlaneManager::have_ipv4() const
{
    if (_ifconfig_property != NULL)
	return (_ifconfig_property->have_ipv4());

    return (false);
}

bool
FeaDataPlaneManager::have_ipv6() const
{
    if (_ifconfig_property != NULL)
	return (_ifconfig_property->have_ipv6());

    return (false);
}

IfConfig&
FeaDataPlaneManager::ifconfig()
{
    return (_fea_node.ifconfig());
}

FirewallManager&
FeaDataPlaneManager::firewall_manager()
{
    return (_fea_node.firewall_manager());
}

FibConfig&
FeaDataPlaneManager::fibconfig()
{
    return (_fea_node.fibconfig());
}

IoLinkManager&
FeaDataPlaneManager::io_link_manager()
{
    return (_fea_node.io_link_manager());
}

IoIpManager&
FeaDataPlaneManager::io_ip_manager()
{
    return (_fea_node.io_ip_manager());
}

IoTcpUdpManager&
FeaDataPlaneManager::io_tcpudp_manager()
{
    return (_fea_node.io_tcpudp_manager());
}

void
FeaDataPlaneManager::deallocate_io_link(IoLink* io_link)
{
    list<IoLink *>::iterator iter;

    iter = find(_io_link_list.begin(), _io_link_list.end(), io_link);
    XLOG_ASSERT(iter != _io_link_list.end());
    _io_link_list.erase(iter);

    delete io_link;
}

void
FeaDataPlaneManager::deallocate_io_ip(IoIp* io_ip)
{
    list<IoIp *>::iterator iter;

    iter = find(_io_ip_list.begin(), _io_ip_list.end(), io_ip);
    XLOG_ASSERT(iter != _io_ip_list.end());
    _io_ip_list.erase(iter);

    delete io_ip;
}

void
FeaDataPlaneManager::deallocate_io_tcpudp(IoTcpUdp* io_tcpudp)
{
    list<IoTcpUdp *>::iterator iter;

    iter = find(_io_tcpudp_list.begin(), _io_tcpudp_list.end(), io_tcpudp);
    XLOG_ASSERT(iter != _io_tcpudp_list.end());
    _io_tcpudp_list.erase(iter);

    delete io_tcpudp;
}
