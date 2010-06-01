// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif

#include "ifconfig.hh"
#include "ifconfig_transaction.hh"
#include "fea_node.hh"


//
// Network interfaces related configuration.
//

static bool
map_changes(const IfTreeItem::State&		fci,
	    IfConfigUpdateReporterBase::Update& u)
{
    switch (fci) {
    case IfTreeItem::NO_CHANGE:
	return false;
    case IfTreeItem::CREATED:
	u = IfConfigUpdateReporterBase::CREATED;
	break;
    case IfTreeItem::DELETED:
	u = IfConfigUpdateReporterBase::DELETED;
	break;
    case IfTreeItem::CHANGED:
	u = IfConfigUpdateReporterBase::CHANGED;
	break;
    default:
	XLOG_FATAL("Unknown IfTreeItem::State");
	break;
    }  
    return true;
}

IfConfig::IfConfig(FeaNode& fea_node)
    : _fea_node(fea_node),
      _eventloop(fea_node.eventloop()),
      _nexthop_port_mapper(fea_node.nexthop_port_mapper()),
      _itm(NULL),
      //_live_config("live-config"),
      _user_config("user-config"),
      _system_config("system-config"),
      _merged_config("pushed-config"),
      _original_config("original-config"),
      _restore_original_config_on_shutdown(false),
      _ifconfig_update_replicator(merged_config()),
      _is_running(false)
{
    _itm = new IfConfigTransactionManager(_eventloop);
}

IfConfig::~IfConfig()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the mechanism for manipulating "
		   "the network interfaces: %s",
		   error_msg.c_str());
    }

    if (_itm != NULL) {
	delete _itm;
	_itm = NULL;
    }
}

ProcessStatus
IfConfig::status(string& reason) const
{
    if (_itm->pending() > 0) {
	reason = "There are transactions pending";
	return (PROC_NOT_READY);
    }
    return (PROC_READY);
}

int
IfConfig::start_transaction(uint32_t& tid, string& error_msg)
{
    if (_itm->start(tid) != true) {
	error_msg = c_format("Resource limit on number of pending transactions hit");
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfig::abort_transaction(uint32_t tid, string& error_msg)
{
    if (_itm->abort(tid) != true) {
	error_msg = c_format("Expired or invalid transaction ID presented");
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IfConfig::add_transaction_operation(uint32_t tid,
				    const TransactionManager::Operation& op,
				    string& error_msg)
{
    uint32_t n_ops = 0;

    if (_itm->retrieve_size(tid, n_ops) != true) {
	error_msg = c_format("Expired or invalid transaction ID presented");
	return (XORP_ERROR);
    }

    // XXX: If necessary, check whether n_ops is above a pre-defined limit.

    //
    // In theory, resource shortage is the only thing that could get us here
    //
    if (_itm->add(tid, op) != true) {
	error_msg = c_format("Unknown resource shortage");
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int IfConfig::add_interface(const char* ifname) {
    IfTreeInterface* ifpl = user_config().find_interface(ifname);
    //XLOG_WARNING("Add interface: %s  current local_cfg: %p\n", ifname, ifpl);
    if (!ifpl) {
	// Add to our local config
	user_config().add_interface(ifname);

	// Read in the OS's information for this interface.
	pull_config(ifname, -1);

	IfTreeInterface* ifp = system_config().find_interface(ifname);
	if (ifp) {
	    //XLOG_WARNING("Added new interface: %s, found it in pulled config.\n", ifname);
	    user_config().update_interface(*ifp);
	}
	else {
	    //XLOG_WARNING("Added new interface: %s, but not found in pulled config.\n", ifname);
	}
    }

    // Add this to our original config if it's not there already.
    if (!_original_config.find_interface(ifname)) {
	IfTreeInterface* ifp = _system_config.find_interface(ifname);
	if (ifp) {
	    _original_config.update_interface(*ifp);
	}
    }
    return XORP_OK;
}   
    
int IfConfig::remove_interface(const char* ifname) {
    //XLOG_WARNING("Remove interface: %s\n", ifname);
    user_config().remove_interface(ifname);
    system_config().remove_interface(ifname);
    return XORP_OK;
}

int IfConfig::update_interface(const IfTreeInterface& iface) {
    return _user_config.update_interface(iface);
}


int
IfConfig::commit_transaction(uint32_t tid, string& error_msg)
{
    IfTree old_user_config = user_config();	// Copy to restore config
    IfTree old_merged_config = merged_config(); // Copy to compare changes

    //
    // XXX: Pull in advance the current system config, in case it is needed
    // by some of the transaction operations.
    //
    IfTree old_system_config = pull_config(NULL, -1);

    if (_itm->commit(tid) != true) {
	error_msg += c_format("Expired or invalid transaction ID presented\n");
	return (XORP_ERROR);
    }

    if (_itm->error().empty() != true) {
	error_msg += "IfConfig::commit_transaction: _itm had non-empty error:\n";
	error_msg += _itm->error();
	return (XORP_ERROR);
    }

    //
    // If we get here we have successfully updated the local copy of the config
    //

    //
    // Prune deleted state that was never added earlier
    //
    user_config().prune_bogus_deleted_state(old_user_config);

    //
    // Push the configuration
    //
    merged_config().align_with_user_config(user_config());
    if (push_config(merged_config()) != XORP_OK) {
	string error_msg2;
	error_msg += " push_config failed: ";
	error_msg += push_error();
	error_msg += "\n";

	// Reverse-back to the previously working configuration
	if (restore_config(old_user_config, old_system_config,
			   error_msg2) != XORP_OK) {
	    // Failed to reverse back
	    error_msg += c_format("%s [Also, failed to reverse-back to the "
				  "previous config: %s]\n",
				  error_msg.c_str(), error_msg2.c_str());
	}

	return (XORP_ERROR);
    }

    //
    // Pull the new device configuration
    //
    pull_config(NULL, -1);
    debug_msg("SYSTEM CONFIG %s\n", system_config().str().c_str());
    debug_msg("USER CONFIG %s\n", user_config().str().c_str());
    debug_msg("MERGED CONFIG %s\n", merged_config().str().c_str());

    //
    // Align with system configuration, so that any stuff that failed
    // in push is not held over in merged config.
    //
    merged_config().align_with_pulled_changes(system_config(), user_config());
    debug_msg("MERGED CONFIG AFTER ALIGN %s\n", merged_config().str().c_str());

    //
    // Propagate the configuration changes to all listeners
    //
    report_updates(merged_config());

    //
    // Flush-out config state
    //
    user_config().finalize_state();
    merged_config().finalize_state();

    return (XORP_OK);
}

int
IfConfig::restore_config(const IfTree& old_user_config,
			 const IfTree& old_system_config,
			 string& error_msg)
{
    IfTree iftree = old_system_config;

    // Restore the config
    set_user_config(old_user_config);
    set_merged_config(old_user_config);
    pull_config(NULL, -1);	// Get the current system config
    iftree.prepare_replacement_state(system_config());

    // Push the config
    if (push_config(iftree) != XORP_OK) {
	error_msg = push_error();
	return (XORP_ERROR);
    }

    // Align the state
    pull_config(NULL, -1);
    merged_config().align_with_pulled_changes(system_config(), user_config());
    user_config().finalize_state();
    merged_config().finalize_state();

    return (XORP_OK);
}

int
IfConfig::register_ifconfig_property(IfConfigProperty* ifconfig_property,
				     bool is_exclusive)
{
    if (is_exclusive)
	_ifconfig_property_plugins.clear();

    if ((ifconfig_property != NULL)
	&& (find(_ifconfig_property_plugins.begin(),
		 _ifconfig_property_plugins.end(),
		 ifconfig_property)
	    == _ifconfig_property_plugins.end())) {
	_ifconfig_property_plugins.push_back(ifconfig_property);
    }

    return (XORP_OK);
}

int
IfConfig::unregister_ifconfig_property(IfConfigProperty* ifconfig_property)
{
    if (ifconfig_property == NULL)
	return (XORP_ERROR);

    list<IfConfigProperty*>::iterator iter;
    iter = find(_ifconfig_property_plugins.begin(),
		_ifconfig_property_plugins.end(),
		ifconfig_property);
    if (iter == _ifconfig_property_plugins.end())
	return (XORP_ERROR);
    _ifconfig_property_plugins.erase(iter);

    return (XORP_OK);
}

int
IfConfig::register_ifconfig_get(IfConfigGet* ifconfig_get, bool is_exclusive)
{
    if (is_exclusive)
	_ifconfig_gets.clear();

    if ((ifconfig_get != NULL)
	&& (find(_ifconfig_gets.begin(), _ifconfig_gets.end(), ifconfig_get)
	    == _ifconfig_gets.end())) {
	_ifconfig_gets.push_back(ifconfig_get);
    }

    return (XORP_OK);
}

int
IfConfig::unregister_ifconfig_get(IfConfigGet* ifconfig_get)
{
    if (ifconfig_get == NULL)
	return (XORP_ERROR);

    list<IfConfigGet*>::iterator iter;
    iter = find(_ifconfig_gets.begin(), _ifconfig_gets.end(), ifconfig_get);
    if (iter == _ifconfig_gets.end())
	return (XORP_ERROR);
    _ifconfig_gets.erase(iter);

    return (XORP_OK);
}

int
IfConfig::register_ifconfig_set(IfConfigSet* ifconfig_set, bool is_exclusive)
{
    if (is_exclusive)
	_ifconfig_sets.clear();

    if ((ifconfig_set != NULL)
	&& (find(_ifconfig_sets.begin(), _ifconfig_sets.end(), ifconfig_set)
	    == _ifconfig_sets.end())) {
	_ifconfig_sets.push_back(ifconfig_set);

	//
	// XXX: Push the current config into the new method
	//
	if (ifconfig_set->is_running())
	    ifconfig_set->push_config(merged_config());
    }

    return (XORP_OK);
}

int
IfConfig::unregister_ifconfig_set(IfConfigSet* ifconfig_set)
{
    if (ifconfig_set == NULL)
	return (XORP_ERROR);

    list<IfConfigSet*>::iterator iter;
    iter = find(_ifconfig_sets.begin(), _ifconfig_sets.end(), ifconfig_set);
    if (iter == _ifconfig_sets.end())
	return (XORP_ERROR);
    _ifconfig_sets.erase(iter);

    return (XORP_OK);
}

int
IfConfig::register_ifconfig_observer(IfConfigObserver* ifconfig_observer,
				     bool is_exclusive)
{
    if (is_exclusive)
	_ifconfig_observers.clear();

    if ((ifconfig_observer != NULL)
	&& (find(_ifconfig_observers.begin(), _ifconfig_observers.end(),
		 ifconfig_observer)
	    == _ifconfig_observers.end())) {
	_ifconfig_observers.push_back(ifconfig_observer);
    }

    return (XORP_OK);
}

int
IfConfig::unregister_ifconfig_observer(IfConfigObserver* ifconfig_observer)
{
    if (ifconfig_observer == NULL)
	return (XORP_ERROR);

    list<IfConfigObserver*>::iterator iter;
    iter = find(_ifconfig_observers.begin(), _ifconfig_observers.end(),
		ifconfig_observer);
    if (iter == _ifconfig_observers.end())
	return (XORP_ERROR);
    _ifconfig_observers.erase(iter);

    return (XORP_OK);
}

int
IfConfig::register_ifconfig_vlan_get(IfConfigVlanGet* ifconfig_vlan_get,
				     bool is_exclusive)
{
    if (is_exclusive)
	_ifconfig_vlan_gets.clear();

    if ((ifconfig_vlan_get != NULL)
	&& (find(_ifconfig_vlan_gets.begin(), _ifconfig_vlan_gets.end(),
		 ifconfig_vlan_get)
	    == _ifconfig_vlan_gets.end())) {
	_ifconfig_vlan_gets.push_back(ifconfig_vlan_get);
    }

    return (XORP_OK);
}

int
IfConfig::unregister_ifconfig_vlan_get(IfConfigVlanGet* ifconfig_vlan_get)
{
    if (ifconfig_vlan_get == NULL)
	return (XORP_ERROR);

    list<IfConfigVlanGet*>::iterator iter;
    iter = find(_ifconfig_vlan_gets.begin(), _ifconfig_vlan_gets.end(),
		ifconfig_vlan_get);
    if (iter == _ifconfig_vlan_gets.end())
	return (XORP_ERROR);
    _ifconfig_vlan_gets.erase(iter);

    return (XORP_OK);
}

int
IfConfig::register_ifconfig_vlan_set(IfConfigVlanSet* ifconfig_vlan_set,
				     bool is_exclusive)
{
    if (is_exclusive)
	_ifconfig_vlan_sets.clear();

    if ((ifconfig_vlan_set != NULL)
	&& (find(_ifconfig_vlan_sets.begin(), _ifconfig_vlan_sets.end(),
		 ifconfig_vlan_set)
	    == _ifconfig_vlan_sets.end())) {
	_ifconfig_vlan_sets.push_back(ifconfig_vlan_set);

	//
	// XXX: Push the current config into the new method
	//
	// Note that we use the corresponding IfConfigSet plugin,
	// because it is the entry poing for pushing interface configuration,
	// while the IfConfigVlanSet plugin is supporting the IfConfigSet
	// plugin.
	//
	if (ifconfig_vlan_set->is_running()) {
	    FeaDataPlaneManager& m = ifconfig_vlan_set->fea_data_plane_manager();
	    IfConfigSet* ifconfig_set = m.ifconfig_set();
	    if (ifconfig_set->is_running())
		ifconfig_set->push_config(merged_config());
	}
    }

    return (XORP_OK);
}

int
IfConfig::unregister_ifconfig_vlan_set(IfConfigVlanSet* ifconfig_vlan_set)
{
    if (ifconfig_vlan_set == NULL)
	return (XORP_ERROR);

    list<IfConfigVlanSet*>::iterator iter;
    iter = find(_ifconfig_vlan_sets.begin(), _ifconfig_vlan_sets.end(),
		ifconfig_vlan_set);
    if (iter == _ifconfig_vlan_sets.end())
	return (XORP_ERROR);
    _ifconfig_vlan_sets.erase(iter);

    return (XORP_OK);
}

int
IfConfig::start(string& error_msg)
{
    list<IfConfigProperty*>::iterator ifconfig_property_iter;
    list<IfConfigGet*>::iterator ifconfig_get_iter;
    list<IfConfigSet*>::iterator ifconfig_set_iter;
    list<IfConfigObserver*>::iterator ifconfig_observer_iter;
    list<IfConfigVlanGet*>::iterator ifconfig_vlan_get_iter;
    list<IfConfigVlanSet*>::iterator ifconfig_vlan_set_iter;

    if (_is_running)
	return (XORP_OK);

    //
    // Check whether all mechanisms are available
    //
    if (_ifconfig_property_plugins.empty()) {
	error_msg = c_format("No mechanism to test the data plane properties");
	return (XORP_ERROR);
    }
    if (_ifconfig_gets.empty()) {
	error_msg = c_format("No mechanism to get the interface information");
	return (XORP_ERROR);
    }
    if (_ifconfig_sets.empty()) {
	error_msg = c_format("No mechanism to set the interface information");
	return (XORP_ERROR);
    }
    if (_ifconfig_observers.empty()) {
	error_msg = c_format("No mechanism to observe the interface information");
	return (XORP_ERROR);
    }
    //
    // XXX: Don't check the IfConfigVlanGet and IfConfigVlanSet mechanisms,
    // because they are optional.
    //

    //
    // Start the IfConfigProperty methods
    //
    for (ifconfig_property_iter = _ifconfig_property_plugins.begin();
	 ifconfig_property_iter != _ifconfig_property_plugins.end();
	 ++ifconfig_property_iter) {
	IfConfigProperty* ifconfig_property = *ifconfig_property_iter;
	if (ifconfig_property->start(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    //
    // Start the IfConfigGet methods
    //
    for (ifconfig_get_iter = _ifconfig_gets.begin();
	 ifconfig_get_iter != _ifconfig_gets.end();
	 ++ifconfig_get_iter) {
	IfConfigGet* ifconfig_get = *ifconfig_get_iter;
	if (ifconfig_get->start(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    //
    // Start the IfConfigSet methods
    //
    for (ifconfig_set_iter = _ifconfig_sets.begin();
	 ifconfig_set_iter != _ifconfig_sets.end();
	 ++ifconfig_set_iter) {
	IfConfigSet* ifconfig_set = *ifconfig_set_iter;
	if (ifconfig_set->start(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    //
    // Start the IfConfigObserver methods
    //
    for (ifconfig_observer_iter = _ifconfig_observers.begin();
	 ifconfig_observer_iter != _ifconfig_observers.end();
	 ++ifconfig_observer_iter) {
	IfConfigObserver* ifconfig_observer = *ifconfig_observer_iter;
	if (ifconfig_observer->start(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    //
    // Start the IfConfigVlanGet methods
    //
    for (ifconfig_vlan_get_iter = _ifconfig_vlan_gets.begin();
	 ifconfig_vlan_get_iter != _ifconfig_vlan_gets.end();
	 ++ifconfig_vlan_get_iter) {
	IfConfigVlanGet* ifconfig_vlan_get = *ifconfig_vlan_get_iter;
	if (ifconfig_vlan_get->start(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    //
    // Start the IfConfigSetSet methods
    //
    for (ifconfig_vlan_set_iter = _ifconfig_vlan_sets.begin();
	 ifconfig_vlan_set_iter != _ifconfig_vlan_sets.end();
	 ++ifconfig_vlan_set_iter) {
	IfConfigVlanSet* ifconfig_vlan_set = *ifconfig_vlan_set_iter;
	if (ifconfig_vlan_set->start(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    pull_config(NULL, -1);
    _system_config.finalize_state();

    _original_config = _system_config;
    _original_config.finalize_state();

    debug_msg("Start configuration read: %s\n", _system_config.str().c_str());
    debug_msg("\nEnd configuration read.\n");

    _is_running = true;

    return (XORP_OK);
}

const IfTree& IfConfig::full_pulled_config() {
    // XXX: We pull the configuration by using only the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (! _ifconfig_gets.empty()) {
	IfConfigGet* ifconfig_get = _ifconfig_gets.front();

	ifconfig_get->pull_config(NULL, _system_config);
    }
    return _system_config;
}


int
IfConfig::stop(string& error_msg)
{
    list<IfConfigProperty*>::iterator ifconfig_property_iter;
    list<IfConfigGet*>::iterator ifconfig_get_iter;
    list<IfConfigSet*>::iterator ifconfig_set_iter;
    list<IfConfigObserver*>::iterator ifconfig_observer_iter;
    list<IfConfigVlanGet*>::iterator ifconfig_vlan_get_iter;
    list<IfConfigVlanSet*>::iterator ifconfig_vlan_set_iter;
    int ret_value = XORP_OK;
    string error_msg2;

    if (! _is_running)
	return (XORP_OK);

    error_msg.erase();

    //
    // Restore the original config
    //
    if (restore_original_config_on_shutdown()) {
	IfTree tmp_push_tree = _original_config;
	if (restore_config(tmp_push_tree, tmp_push_tree, error_msg2)
	    != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // Stop the IfConfigVlanSet methods
    //
    for (ifconfig_vlan_set_iter = _ifconfig_vlan_sets.begin();
	 ifconfig_vlan_set_iter != _ifconfig_vlan_sets.end();
	 ++ifconfig_vlan_set_iter) {
	IfConfigVlanSet* ifconfig_vlan_set = *ifconfig_vlan_set_iter;
	if (ifconfig_vlan_set->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // Stop the IfConfigVlanGet methods
    //
    for (ifconfig_vlan_get_iter = _ifconfig_vlan_gets.begin();
	 ifconfig_vlan_get_iter != _ifconfig_vlan_gets.end();
	 ++ifconfig_vlan_get_iter) {
	IfConfigVlanGet* ifconfig_vlan_get = *ifconfig_vlan_get_iter;
	if (ifconfig_vlan_get->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // Stop the IfConfigObserver methods
    //
    for (ifconfig_observer_iter = _ifconfig_observers.begin();
	 ifconfig_observer_iter != _ifconfig_observers.end();
	 ++ifconfig_observer_iter) {
	IfConfigObserver* ifconfig_observer = *ifconfig_observer_iter;
	if (ifconfig_observer->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // Stop the IfConfigSet methods
    //
    for (ifconfig_set_iter = _ifconfig_sets.begin();
	 ifconfig_set_iter != _ifconfig_sets.end();
	 ++ifconfig_set_iter) {
	IfConfigSet* ifconfig_set = *ifconfig_set_iter;
	if (ifconfig_set->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // Stop the IfConfigGet methods
    //
    for (ifconfig_get_iter = _ifconfig_gets.begin();
	 ifconfig_get_iter != _ifconfig_gets.end();
	 ++ifconfig_get_iter) {
	IfConfigGet* ifconfig_get = *ifconfig_get_iter;
	if (ifconfig_get->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // Stop the IfConfigProperty methods
    //
    for (ifconfig_property_iter = _ifconfig_property_plugins.begin();
	 ifconfig_property_iter != _ifconfig_property_plugins.end();
	 ++ifconfig_property_iter) {
	IfConfigProperty* ifconfig_property = *ifconfig_property_iter;
	if (ifconfig_property->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    _is_running = false;

    return (ret_value);
}

int
IfConfig::push_config(const IfTree& iftree)
{
    list<IfConfigSet*>::const_iterator ifconfig_set_iter;

    if (_ifconfig_sets.empty())
	return (XORP_ERROR);		// XXX: No plugins

    for (ifconfig_set_iter = _ifconfig_sets.begin();
	 ifconfig_set_iter != _ifconfig_sets.end();
	 ++ifconfig_set_iter) {
	IfConfigSet* ifconfig_set = *ifconfig_set_iter;
	if (ifconfig_set->push_config(iftree) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

// TODO:  This can be costly when you have lots of interfaces, so try to
// optimize things so this is called rarely.  Currently it is called
// several times on startup of fea.
const IfTree&
IfConfig::pull_config(const char* ifname, int if_index)
{
    //
    // XXX: We pull the configuration by using only the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (! _ifconfig_gets.empty()) {
	IfConfigGet* ifconfig_get = _ifconfig_gets.front();

	if (ifname && ifconfig_get->can_pull_one()) {
	    // Just request a pull of this interface
	    //
	    // Can't pull my_discard, it's a fake device.
	    if (strcmp(ifname, "my_discard") != 0) {
		//XLOG_WARNING("Pull_config_one for interface: %s\n", ifname);
		int rv = ifconfig_get->pull_config_one(_system_config, ifname, if_index);
		if (rv != XORP_OK) {
		    XLOG_WARNING("ERROR:  pull_config_one for interface: %s failed: %i\n", ifname, rv);
		}
		if (!_system_config.find_interface(ifname)) {
		    XLOG_WARNING("ERROR:  Could not find interface: %s after pull_config_one.\n", ifname);
		}
	    }
	}
	else {
	    // Pull all
	    // Clear the old state
	    _system_config.clear();
	    
	    ifconfig_get->pull_config(&_user_config, _system_config);
	}
    }

    return _system_config;
}

bool
IfConfig::report_update(const IfTreeInterface& fi)
{
    IfConfigUpdateReporterBase::Update u;
    if (map_changes(fi.state(), u)) {
	_ifconfig_update_replicator.interface_update(fi.ifname(), u);
	return (true);
    }

    return (false);
}

bool
IfConfig::report_update(const IfTreeInterface&	fi,
			const IfTreeVif&	fv)
{
    IfConfigUpdateReporterBase::Update u;
    if (map_changes(fv.state(), u)) {
	_ifconfig_update_replicator.vif_update(fi.ifname(), fv.vifname(), u);
	return (true);
    }

    return (false);
}

bool
IfConfig::report_update(const IfTreeInterface&	fi,
			const IfTreeVif&	fv,
			const IfTreeAddr4&	fa)

{
    IfConfigUpdateReporterBase::Update u;
    if (map_changes(fa.state(), u)) {
	_ifconfig_update_replicator.vifaddr4_update(fi.ifname(), fv.vifname(),
						    fa.addr(), u);
	return (true);
    }

    return (false);
}

#ifdef HAVE_IPV6
bool
IfConfig::report_update(const IfTreeInterface&	fi,
			const IfTreeVif&	fv,
			const IfTreeAddr6&	fa)
{
    IfConfigUpdateReporterBase::Update u;
    if (map_changes(fa.state(), u)) {
	_ifconfig_update_replicator.vifaddr6_update(fi.ifname(), fv.vifname(),
						    fa.addr(), u);
	return (true);
    }

    return (false);
}
#endif

void
IfConfig::report_updates_completed()
{
    _ifconfig_update_replicator.updates_completed();
}

void
IfConfig::report_updates(IfTree& iftree)
{
    bool updated = false;

    //
    // Walk config looking for changes to report
    //
    for (IfTree::IfMap::const_iterator ii = iftree.interfaces().begin();
	 ii != iftree.interfaces().end(); ++ii) {

	const IfTreeInterface& interface = *(ii->second);
	updated |= report_update(interface);

	IfTreeInterface::VifMap::const_iterator vi;
	for (vi = interface.vifs().begin();
	     vi != interface.vifs().end(); ++vi) {

	    const IfTreeVif& vif = *(vi->second);
	    updated |= report_update(interface, vif);

	    for (IfTreeVif::IPv4Map::const_iterator ai = vif.ipv4addrs().begin();
		 ai != vif.ipv4addrs().end(); ai++) {
		const IfTreeAddr4& addr = *(ai->second);
		updated |= report_update(interface, vif, addr);
	    }

#ifdef HAVE_IPV6
	    for (IfTreeVif::IPv6Map::const_iterator ai = vif.ipv6addrs().begin();
		 ai != vif.ipv6addrs().end(); ai++) {
		const IfTreeAddr6& addr = *(ai->second);
		updated |= report_update(interface, vif, addr);
	    }
#endif
	}
    }
    if (updated) {
	// Complete the update
	report_updates_completed();
    }
}

const string&
IfConfig::push_error() const
{
    return _ifconfig_error_reporter.first_error();
}
