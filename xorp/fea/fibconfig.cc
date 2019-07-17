// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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



#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libxorp/profile.hh"

#ifndef XORP_DISABLE_PROFILE
#include "profile_vars.hh"
#endif

#include "fibconfig.hh"
#include "fibconfig_transaction.hh"
#include "fea_node.hh"


//
// Unicast forwarding table related configuration.
//


FibConfig::FibConfig(FeaNode& fea_node, const IfTree& system_config_iftree,
		     const IfTree& merged_config_iftree)
	: vrf_queried(false), _eventloop(fea_node.eventloop()),
#ifndef XORP_DISABLE_PROFILE
      _profile(fea_node.profile()),
#endif
      _nexthop_port_mapper(fea_node.nexthop_port_mapper()),
      _system_config_iftree(system_config_iftree),
      _merged_config_iftree(merged_config_iftree),
      _ftm(NULL),
      _unicast_forwarding_entries_retain_on_startup4(false),
      _unicast_forwarding_entries_retain_on_shutdown4(false),
      _unicast_forwarding_entries_retain_on_startup6(false),
      _unicast_forwarding_entries_retain_on_shutdown6(false),
      _unicast_forwarding_table_id4(0),
      _unicast_forwarding_table_id4_is_configured(false),
      _unicast_forwarding_table_id6(0),
      _unicast_forwarding_table_id6_is_configured(false),
      _is_running(false)
{
    _ftm = new FibConfigTransactionManager(_eventloop, *this);
}

FibConfig::~FibConfig()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the mechanism for manipulating "
		   "the forwarding table information: %s",
		   error_msg.c_str());
    }

    if (_ftm != NULL) {
	delete _ftm;
	_ftm = NULL;
    }
}

const string& FibConfig::get_vrf_name() const {
    if (!vrf_queried) {
	/* Nasty hack, try to bind to VRF if it exists */
	char cmd[100];
	char fname[50];
	int table_id;
	vrf_name = "";
	if (unicast_forwarding_table_id4_is_configured())
	    table_id = unicast_forwarding_table_id4();
	else if (unicast_forwarding_table_id6_is_configured())
	    table_id = unicast_forwarding_table_id6();
	else
	    goto out;
	{
	    sprintf(fname, "/tmp/xorp_vrf_%d.txt", table_id);
	    sprintf(cmd, "ip vrf show | grep \" %d$\" > %s", table_id, fname);
	    XLOG_INFO("Trying to detect vrf name, table-id: %d\n", table_id);
	    system(cmd);
	    ifstream tmp(fname);
	    if (tmp) {
		string local_dev;
		tmp >> local_dev; /* read in the vrf name */
		if (local_dev.size()) {
		    snprintf(cmd, sizeof(cmd), "/sys/class/net/%s/address", local_dev.c_str());
		    ifstream tst2(cmd);
		    if (tst2) {
			// OK, device seems to have been probed correctly, save for later.
			vrf_name = local_dev;
			XLOG_INFO("Found vrf: %s for table-id: %d\n", vrf_name.c_str(), table_id);
		    }
		}
	    }
	}
      out:
	vrf_queried = true;
    }
    return vrf_name;
}

ProcessStatus
FibConfig::status(string& reason) const
{
    if (_ftm->pending() > 0) {
	reason = "There are transactions pending";
	return (PROC_NOT_READY);
    }
    return (PROC_READY);
}

uint32_t FibConfig::get_netlink_filter_table_id() const {
    uint32_t tbl_id = 0;
    if (unicast_forwarding_table_id4_is_configured() ||
	unicast_forwarding_table_id6_is_configured()) {
	if (unicast_forwarding_table_id4_is_configured()) {
	    tbl_id = unicast_forwarding_table_id4();
	    if (unicast_forwarding_table_id6_is_configured()) {
		if (unicast_forwarding_table_id6() != tbl_id) {
		    XLOG_WARNING("WARNING:  IPv4 and v6 tables are configured and are different.  Cannot filter on netlink table-id, will use default behaviour and listen to all tables.\n");
		    tbl_id = 0;
		}
	    }
	}
	else {
	    tbl_id = unicast_forwarding_table_id6();
	}
    }
    //XLOG_WARNING("IPv4 configured: %i %u  IPv6: %i %u  filter-table: %u",
    //	 unicast_forwarding_table_id4_is_configured(),
    //	 unicast_forwarding_table_id4(),
    //	 unicast_forwarding_table_id6_is_configured(),
    //	 unicast_forwarding_table_id6(), tbl_id);
    return tbl_id;
}

void FibConfig::propagate_table_id_change() {
    vrf_queried = false; /* need to re-query */
    uint32_t tbl_id = get_netlink_filter_table_id();
    // Tell the various plugins our configured table changed in case they can filter.
    {
	list<FibConfigEntryGet*>::iterator iter = _fibconfig_entry_gets.begin();
	for (; iter != _fibconfig_entry_gets.end(); iter++) {
	    (*iter)->notify_table_id_change(tbl_id);
	}
    }
    {
	list<FibConfigEntrySet*>::iterator iter = _fibconfig_entry_sets.begin();
	for (; iter != _fibconfig_entry_sets.end(); iter++) {
	    (*iter)->notify_table_id_change(tbl_id);
	}
    }
    {
	list<FibConfigEntryObserver*>::iterator iter = _fibconfig_entry_observers.begin();
	for (; iter != _fibconfig_entry_observers.end(); iter++) {
	    (*iter)->notify_table_id_change(tbl_id);
	}
    }
    {
	list<FibConfigTableGet*>::iterator iter = _fibconfig_table_gets.begin();
	for (; iter != _fibconfig_table_gets.end(); iter++) {
	    (*iter)->notify_table_id_change(tbl_id);
	}
    }
    {
	list<FibConfigTableSet*>::iterator iter = _fibconfig_table_sets.begin();
	for (; iter != _fibconfig_table_sets.end(); iter++) {
	    (*iter)->notify_table_id_change(tbl_id);
	}
    }
    {
	list<FibConfigTableObserver*>::iterator iter = _fibconfig_table_observers.begin();
	for (; iter != _fibconfig_table_observers.end(); iter++) {
	    (*iter)->notify_table_id_change(tbl_id);
	}
    }
}

int
FibConfig::start_transaction(uint32_t& tid, string& error_msg)
{
    if (_ftm->start(tid) != true) {
	error_msg = c_format("Resource limit on number of pending transactions hit");
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::abort_transaction(uint32_t tid, string& error_msg)
{
    if (_ftm->abort(tid) != true) {
	error_msg = c_format("Expired or invalid transaction ID presented");
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::add_transaction_operation(uint32_t tid,
				     const TransactionManager::Operation& op,
				     string& error_msg)
{
    uint32_t n_ops = 0;

    if (_ftm->retrieve_size(tid, n_ops) != true) {
	error_msg = c_format("Expired or invalid transaction ID presented");
	return (XORP_ERROR);
    }

    if (_ftm->max_ops() <= n_ops) {
	error_msg = c_format("Resource limit on number of operations in a transaction hit");
	return (XORP_ERROR);
    }

    //
    // In theory, resource shortage is the only thing that could get us here
    //
    if (_ftm->add(tid, op) != true) {
	error_msg = c_format("Unknown resource shortage");
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::commit_transaction(uint32_t tid, string& error_msg)
{
    if (_ftm->commit(tid) != true) {
	error_msg = c_format("Expired or invalid transaction ID presented");
	return (XORP_ERROR);
    }

    const string& ftm_error_msg = _ftm->error();
    if (! ftm_error_msg.empty()) {
	error_msg = ftm_error_msg;
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::register_fibconfig_forwarding(FibConfigForwarding* fibconfig_forwarding,
					bool is_exclusive)
{
    if (is_exclusive)
	_fibconfig_forwarding_plugins.clear();

    if ((fibconfig_forwarding != NULL)
	&& (find(_fibconfig_forwarding_plugins.begin(),
		 _fibconfig_forwarding_plugins.end(),
		 fibconfig_forwarding)
	    == _fibconfig_forwarding_plugins.end())) {
	_fibconfig_forwarding_plugins.push_back(fibconfig_forwarding);

	//
	// XXX: Push the current config into the new method
	//
	if (fibconfig_forwarding->is_running()) {
	    bool v = false;
	    string error_msg;
	    string manager_name = fibconfig_forwarding->fea_data_plane_manager().manager_name();

	    if (fibconfig_forwarding->fea_data_plane_manager().have_ipv4()) {
		if (unicast_forwarding_enabled4(v, error_msg) != XORP_OK) {
		    XLOG_ERROR("Cannot push the current IPv4 forwarding "
			       "information state into the %s mechanism, "
			       "because failed to obtain the current state: %s",
			       manager_name.c_str(), error_msg.c_str());
		} else {
		    if (fibconfig_forwarding->set_unicast_forwarding_enabled4(v, error_msg)
			!= XORP_OK) {
			XLOG_ERROR("Cannot push the current IPv4 forwarding "
				   "information state into the %s mechanism: %s",
				   manager_name.c_str(), error_msg.c_str());
		    }
		}
	    }

#ifdef HAVE_IPV6
	    if (fibconfig_forwarding->fea_data_plane_manager().have_ipv6()) {
		if (unicast_forwarding_enabled6(v, error_msg) != XORP_OK) {
		    XLOG_ERROR("Cannot push the current IPv6 forwarding "
			       "information state into the %s mechanism, "
			       "because failed to obtain the current state: %s",
			       manager_name.c_str(), error_msg.c_str());
		} else {
		    if (fibconfig_forwarding->set_unicast_forwarding_enabled6(v, error_msg)
			!= XORP_OK) {
			XLOG_ERROR("Cannot push the current IPv6 forwarding "
				   "information state into the %s mechanism: %s",
				   manager_name.c_str(), error_msg.c_str());
		    }
		}

		if (accept_rtadv_enabled6(v, error_msg) != XORP_OK) {
		    XLOG_ERROR("Cannot push the current IPv6 forwarding "
			       "information state into the %s mechanism, "
			       "because failed to obtain the current state: %s",
			       manager_name.c_str(), error_msg.c_str());
		} else {
		    if (fibconfig_forwarding->set_accept_rtadv_enabled6(v, error_msg)
			!= XORP_OK) {
			XLOG_ERROR("Cannot push the current IPv6 forwarding "
				   "information state into the %s mechanism: %s",
				   manager_name.c_str(), error_msg.c_str());
		    }
		}
	    }
#endif // HAVE_IPV6
	}
    }

    return (XORP_OK);
}

int
FibConfig::unregister_fibconfig_forwarding(FibConfigForwarding* fibconfig_forwarding)
{
    if (fibconfig_forwarding == NULL)
	return (XORP_ERROR);

    list<FibConfigForwarding*>::iterator iter;
    iter = find(_fibconfig_forwarding_plugins.begin(),
		_fibconfig_forwarding_plugins.end(),
		fibconfig_forwarding);
    if (iter == _fibconfig_forwarding_plugins.end())
	return (XORP_ERROR);
    _fibconfig_forwarding_plugins.erase(iter);

    return (XORP_OK);
}

int
FibConfig::register_fibconfig_entry_get(FibConfigEntryGet* fibconfig_entry_get,
					bool is_exclusive)
{
    if (is_exclusive)
	_fibconfig_entry_gets.clear();

    if ((fibconfig_entry_get != NULL)
	&& (find(_fibconfig_entry_gets.begin(), _fibconfig_entry_gets.end(),
		 fibconfig_entry_get)
	    == _fibconfig_entry_gets.end())) {
	_fibconfig_entry_gets.push_back(fibconfig_entry_get);
    }

    return (XORP_OK);
}

int
FibConfig::unregister_fibconfig_entry_get(FibConfigEntryGet* fibconfig_entry_get)
{
    if (fibconfig_entry_get == NULL)
	return (XORP_ERROR);

    list<FibConfigEntryGet*>::iterator iter;
    iter = find(_fibconfig_entry_gets.begin(), _fibconfig_entry_gets.end(),
		fibconfig_entry_get);
    if (iter == _fibconfig_entry_gets.end())
	return (XORP_ERROR);
    _fibconfig_entry_gets.erase(iter);

    return (XORP_OK);
}

int
FibConfig::register_fibconfig_entry_set(FibConfigEntrySet* fibconfig_entry_set,
					bool is_exclusive)
{
    if (is_exclusive)
	_fibconfig_entry_sets.clear();

    if ((fibconfig_entry_set != NULL) 
	&& (find(_fibconfig_entry_sets.begin(), _fibconfig_entry_sets.end(),
		 fibconfig_entry_set)
	    == _fibconfig_entry_sets.end())) {
	_fibconfig_entry_sets.push_back(fibconfig_entry_set);

	//
	// XXX: Push the current config into the new method
	//
	if (fibconfig_entry_set->is_running()) {
	    // XXX: nothing to do.
	    //
	    // XXX: note that the forwarding table state is pushed by method
	    // FibConfig::register_fibconfig_table_set(),
	    // hence we don't need to do it here again. However, this is based
	    // on the assumption that for each FibConfigEntrySet method
	    // there is a corresponding FibConfigTableSet method.
	    //
	}
    }

    return (XORP_OK);
}

int
FibConfig::unregister_fibconfig_entry_set(FibConfigEntrySet* fibconfig_entry_set)
{
    if (fibconfig_entry_set == NULL)
	return (XORP_ERROR);

    list<FibConfigEntrySet*>::iterator iter;
    iter = find(_fibconfig_entry_sets.begin(), _fibconfig_entry_sets.end(),
		fibconfig_entry_set);
    if (iter == _fibconfig_entry_sets.end())
	return (XORP_ERROR);
    _fibconfig_entry_sets.erase(iter);

    return (XORP_OK);
}

int
FibConfig::register_fibconfig_entry_observer(FibConfigEntryObserver* fibconfig_entry_observer,
					     bool is_exclusive)
{
    if (is_exclusive)
	_fibconfig_entry_observers.clear();

    if ((fibconfig_entry_observer != NULL) 
	&& (find(_fibconfig_entry_observers.begin(),
		 _fibconfig_entry_observers.end(),
		 fibconfig_entry_observer)
	    == _fibconfig_entry_observers.end())) {
	_fibconfig_entry_observers.push_back(fibconfig_entry_observer);
    }
    
    return (XORP_OK);
}

int
FibConfig::unregister_fibconfig_entry_observer(FibConfigEntryObserver* fibconfig_entry_observer)
{
    if (fibconfig_entry_observer == NULL)
	return (XORP_ERROR);

    list<FibConfigEntryObserver*>::iterator iter;
    iter = find(_fibconfig_entry_observers.begin(),
		_fibconfig_entry_observers.end(),
		fibconfig_entry_observer);
    if (iter == _fibconfig_entry_observers.end())
	return (XORP_ERROR);
    _fibconfig_entry_observers.erase(iter);

    return (XORP_OK);
}

int
FibConfig::register_fibconfig_table_get(FibConfigTableGet* fibconfig_table_get,
					bool is_exclusive)
{
    if (is_exclusive)
	_fibconfig_table_gets.clear();

    if ((fibconfig_table_get != NULL)
	&& (find(_fibconfig_table_gets.begin(), _fibconfig_table_gets.end(),
		 fibconfig_table_get)
	    == _fibconfig_table_gets.end())) {
	_fibconfig_table_gets.push_back(fibconfig_table_get);
    }
    
    return (XORP_OK);
}

int
FibConfig::unregister_fibconfig_table_get(FibConfigTableGet* fibconfig_table_get)
{
    if (fibconfig_table_get == NULL)
	return (XORP_ERROR);

    list<FibConfigTableGet*>::iterator iter;
    iter = find(_fibconfig_table_gets.begin(), _fibconfig_table_gets.end(),
		fibconfig_table_get);
    if (iter == _fibconfig_table_gets.end())
	return (XORP_ERROR);
    _fibconfig_table_gets.erase(iter);

    return (XORP_OK);
}

int
FibConfig::register_fibconfig_table_set(FibConfigTableSet* fibconfig_table_set,
					bool is_exclusive)
{
    if (is_exclusive)
	_fibconfig_table_sets.clear();

    if ((fibconfig_table_set != NULL)
	&& (find(_fibconfig_table_sets.begin(), _fibconfig_table_sets.end(),
		 fibconfig_table_set)
	    == _fibconfig_table_sets.end())) {
	_fibconfig_table_sets.push_back(fibconfig_table_set);

	//
	// XXX: Push the current config into the new method
	//
	if (fibconfig_table_set->is_running()) {
	    list<Fte4> fte_list4;

	    if (get_table4(fte_list4) == XORP_OK) {
		if (fibconfig_table_set->set_table4(fte_list4) != XORP_OK) {
		    XLOG_ERROR("Cannot push the current IPv4 forwarding table "
			       "into a new mechanism for setting the "
			       "forwarding table");
		}
	    }

#ifdef HAVE_IPV6
	    list<Fte6> fte_list6;

	    if (get_table6(fte_list6) == XORP_OK) {
		if (fibconfig_table_set->set_table6(fte_list6) != XORP_OK) {
		    XLOG_ERROR("Cannot push the current IPv6 forwarding table "
			       "into a new mechanism for setting the "
			       "forwarding table");
		}
	    }
#endif // HAVE_IPV6
	}
    }

    return (XORP_OK);
}

int
FibConfig::unregister_fibconfig_table_set(FibConfigTableSet* fibconfig_table_set)
{
    if (fibconfig_table_set == NULL)
	return (XORP_ERROR);

    list<FibConfigTableSet*>::iterator iter;
    iter = find(_fibconfig_table_sets.begin(), _fibconfig_table_sets.end(),
		fibconfig_table_set);
    if (iter == _fibconfig_table_sets.end())
	return (XORP_ERROR);
    _fibconfig_table_sets.erase(iter);

    return (XORP_OK);
}

int
FibConfig::register_fibconfig_table_observer(FibConfigTableObserver* fibconfig_table_observer,
					     bool is_exclusive)
{
    if (is_exclusive)
	_fibconfig_table_observers.clear();

    if ((fibconfig_table_observer != NULL)
	&& (find(_fibconfig_table_observers.begin(),
		 _fibconfig_table_observers.end(),
		 fibconfig_table_observer)
	    == _fibconfig_table_observers.end())) {
	_fibconfig_table_observers.push_back(fibconfig_table_observer);
    }

    return (XORP_OK);
}

int
FibConfig::unregister_fibconfig_table_observer(FibConfigTableObserver* fibconfig_table_observer)
{
    if (fibconfig_table_observer == NULL)
	return (XORP_ERROR);

    list<FibConfigTableObserver*>::iterator iter;
    iter = find(_fibconfig_table_observers.begin(),
		_fibconfig_table_observers.end(),
		fibconfig_table_observer);
    if (iter == _fibconfig_table_observers.end())
	return (XORP_ERROR);
    _fibconfig_table_observers.erase(iter);

    return (XORP_OK);
}

int
FibConfig::start(string& error_msg)
{
    list<FibConfigForwarding*>::iterator fibconfig_forwarding_iter;
    list<FibConfigEntryGet*>::iterator fibconfig_entry_get_iter;
    list<FibConfigEntrySet*>::iterator fibconfig_entry_set_iter;
    list<FibConfigEntryObserver*>::iterator fibconfig_entry_observer_iter;
    list<FibConfigTableGet*>::iterator fibconfig_table_get_iter;
    list<FibConfigTableSet*>::iterator fibconfig_table_set_iter;
    list<FibConfigTableObserver*>::iterator fibconfig_table_observer_iter;

    if (_is_running)
	return (XORP_OK);

    //
    // Check whether all mechanisms are available
    //
    if (_fibconfig_forwarding_plugins.empty()) {
	error_msg = c_format("No mechanism to configure unicast forwarding");
	return (XORP_ERROR);
    }
    if (_fibconfig_entry_gets.empty()) {
	error_msg = c_format("No mechanism to get forwarding table entries");
	return (XORP_ERROR);
    }
    if (_fibconfig_entry_sets.empty()) {
	error_msg = c_format("No mechanism to set forwarding table entries");
	return (XORP_ERROR);
    }
    if (_fibconfig_entry_observers.empty()) {
	error_msg = c_format("No mechanism to observe forwarding table entries");
	return (XORP_ERROR);
    }
    if (_fibconfig_table_gets.empty()) {
	error_msg = c_format("No mechanism to get the forwarding table");
	return (XORP_ERROR);
    }
    if (_fibconfig_table_sets.empty()) {
	error_msg = c_format("No mechanism to set the forwarding table");
	return (XORP_ERROR);
    }
    if (_fibconfig_table_observers.empty()) {
	error_msg = c_format("No mechanism to observe the forwarding table");
	return (XORP_ERROR);
    }

    //
    // Start the FibConfigForwarding methods
    //
    for (fibconfig_forwarding_iter = _fibconfig_forwarding_plugins.begin();
	 fibconfig_forwarding_iter != _fibconfig_forwarding_plugins.end();
	 ++fibconfig_forwarding_iter) {
	FibConfigForwarding* fibconfig_forwarding = *fibconfig_forwarding_iter;
	if (fibconfig_forwarding->start(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    //
    // Start the FibConfigEntryGet methods
    //
    for (fibconfig_entry_get_iter = _fibconfig_entry_gets.begin();
	 fibconfig_entry_get_iter != _fibconfig_entry_gets.end();
	 ++fibconfig_entry_get_iter) {
	FibConfigEntryGet* fibconfig_entry_get = *fibconfig_entry_get_iter;
	if (fibconfig_entry_get->start(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    //
    // Start the FibConfigEntrySet methods
    //
    for (fibconfig_entry_set_iter = _fibconfig_entry_sets.begin();
	 fibconfig_entry_set_iter != _fibconfig_entry_sets.end();
	 ++fibconfig_entry_set_iter) {
	FibConfigEntrySet* fibconfig_entry_set = *fibconfig_entry_set_iter;
	if (fibconfig_entry_set->start(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    //
    // Start the FibConfigEntryObserver methods
    //
    for (fibconfig_entry_observer_iter = _fibconfig_entry_observers.begin();
	 fibconfig_entry_observer_iter != _fibconfig_entry_observers.end();
	 ++fibconfig_entry_observer_iter) {
	FibConfigEntryObserver* fibconfig_entry_observer = *fibconfig_entry_observer_iter;
	if (fibconfig_entry_observer->start(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    //
    // Start the FibConfigTableGet methods
    //
    for (fibconfig_table_get_iter = _fibconfig_table_gets.begin();
	 fibconfig_table_get_iter != _fibconfig_table_gets.end();
	 ++fibconfig_table_get_iter) {
	FibConfigTableGet* fibconfig_table_get = *fibconfig_table_get_iter;
	if (fibconfig_table_get->start(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    //
    // Start the FibConfigTableSet methods
    //
    for (fibconfig_table_set_iter = _fibconfig_table_sets.begin();
	 fibconfig_table_set_iter != _fibconfig_table_sets.end();
	 ++fibconfig_table_set_iter) {
	FibConfigTableSet* fibconfig_table_set = *fibconfig_table_set_iter;
	if (fibconfig_table_set->start(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    //
    // Start the FibConfigTableObserver methods
    //
    for (fibconfig_table_observer_iter = _fibconfig_table_observers.begin();
	 fibconfig_table_observer_iter != _fibconfig_table_observers.end();
	 ++fibconfig_table_observer_iter) {
	FibConfigTableObserver* fibconfig_table_observer = *fibconfig_table_observer_iter;
	if (fibconfig_table_observer->start(error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    _is_running = true;

    return (XORP_OK);
}

int
FibConfig::stop(string& error_msg)
{
    list<FibConfigForwarding*>::iterator fibconfig_forwarding_iter;
    list<FibConfigEntryGet*>::iterator fibconfig_entry_get_iter;
    list<FibConfigEntrySet*>::iterator fibconfig_entry_set_iter;
    list<FibConfigEntryObserver*>::iterator fibconfig_entry_observer_iter;
    list<FibConfigTableGet*>::iterator fibconfig_table_get_iter;
    list<FibConfigTableSet*>::iterator fibconfig_table_set_iter;
    list<FibConfigTableObserver*>::iterator fibconfig_table_observer_iter;
    int ret_value = XORP_OK;
    string error_msg2;

    if (! _is_running)
	return (XORP_OK);

    error_msg.erase();

    //
    // Stop the FibConfigTableObserver methods
    //
    for (fibconfig_table_observer_iter = _fibconfig_table_observers.begin();
	 fibconfig_table_observer_iter != _fibconfig_table_observers.end();
	 ++fibconfig_table_observer_iter) {
	FibConfigTableObserver* fibconfig_table_observer = *fibconfig_table_observer_iter;
	if (fibconfig_table_observer->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // Stop the FibConfigTableSet methods
    //
    for (fibconfig_table_set_iter = _fibconfig_table_sets.begin();
	 fibconfig_table_set_iter != _fibconfig_table_sets.end();
	 ++fibconfig_table_set_iter) {
	FibConfigTableSet* fibconfig_table_set = *fibconfig_table_set_iter;
	if (fibconfig_table_set->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // Stop the FibConfigTableGet methods
    //
    for (fibconfig_table_get_iter = _fibconfig_table_gets.begin();
	 fibconfig_table_get_iter != _fibconfig_table_gets.end();
	 ++fibconfig_table_get_iter) {
	FibConfigTableGet* fibconfig_table_get = *fibconfig_table_get_iter;
	if (fibconfig_table_get->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // Stop the FibConfigEntryObserver methods
    //
    for (fibconfig_entry_observer_iter = _fibconfig_entry_observers.begin();
	 fibconfig_entry_observer_iter != _fibconfig_entry_observers.end();
	 ++fibconfig_entry_observer_iter) {
	FibConfigEntryObserver* fibconfig_entry_observer = *fibconfig_entry_observer_iter;
	if (fibconfig_entry_observer->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // Stop the FibConfigEntrySet methods
    //
    for (fibconfig_entry_set_iter = _fibconfig_entry_sets.begin();
	 fibconfig_entry_set_iter != _fibconfig_entry_sets.end();
	 ++fibconfig_entry_set_iter) {
	FibConfigEntrySet* fibconfig_entry_set = *fibconfig_entry_set_iter;
	if (fibconfig_entry_set->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // Stop the FibConfigEntryGet methods
    //
    for (fibconfig_entry_get_iter = _fibconfig_entry_gets.begin();
	 fibconfig_entry_get_iter != _fibconfig_entry_gets.end();
	 ++fibconfig_entry_get_iter) {
	FibConfigEntryGet* fibconfig_entry_get = *fibconfig_entry_get_iter;
	if (fibconfig_entry_get->stop(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    //
    // Stop the FibConfigForwarding methods
    //
    for (fibconfig_forwarding_iter = _fibconfig_forwarding_plugins.begin();
	 fibconfig_forwarding_iter != _fibconfig_forwarding_plugins.end();
	 ++fibconfig_forwarding_iter) {
	FibConfigForwarding* fibconfig_forwarding = *fibconfig_forwarding_iter;
	if (fibconfig_forwarding->stop(error_msg2) != XORP_OK) {
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
FibConfig::start_configuration(string& error_msg)
{
    list<FibConfigEntrySet*>::iterator fibconfig_entry_set_iter;
    list<FibConfigTableSet*>::iterator fibconfig_table_set_iter;
    int ret_value = XORP_OK;
    string error_msg2;

    error_msg.erase();

    //
    // XXX: We need to call start_configuration() for "entry" and "table",
    // because the top-level start/end configuration interface
    // does not distinguish between "entry" and "table" modification.
    //
    for (fibconfig_entry_set_iter = _fibconfig_entry_sets.begin();
	 fibconfig_entry_set_iter != _fibconfig_entry_sets.end();
	 ++fibconfig_entry_set_iter) {
	FibConfigEntrySet* fibconfig_entry_set = *fibconfig_entry_set_iter;
	if (fibconfig_entry_set->start_configuration(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    for (fibconfig_table_set_iter = _fibconfig_table_sets.begin();
	 fibconfig_table_set_iter != _fibconfig_table_sets.end();
	 ++fibconfig_table_set_iter) {
	FibConfigTableSet* fibconfig_table_set = *fibconfig_table_set_iter;
	if (fibconfig_table_set->start_configuration(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    
    return (ret_value);
}

int
FibConfig::end_configuration(string& error_msg)
{
    list<FibConfigEntrySet*>::iterator fibconfig_entry_set_iter;
    list<FibConfigTableSet*>::iterator fibconfig_table_set_iter;
    int ret_value = XORP_OK;
    string error_msg2;

    error_msg.erase();

    //
    // XXX: We need to call end_configuration() for "entry" and "table",
    // because the top-level start/end configuration interface
    // does not distinguish between "entry" and "table" modification.
    //
    for (fibconfig_entry_set_iter = _fibconfig_entry_sets.begin();
	 fibconfig_entry_set_iter != _fibconfig_entry_sets.end();
	 ++fibconfig_entry_set_iter) {
	FibConfigEntrySet* fibconfig_entry_set = *fibconfig_entry_set_iter;
	if (fibconfig_entry_set->end_configuration(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    for (fibconfig_table_set_iter = _fibconfig_table_sets.begin();
	 fibconfig_table_set_iter != _fibconfig_table_sets.end();
	 ++fibconfig_table_set_iter) {
	FibConfigTableSet* fibconfig_table_set = *fibconfig_table_set_iter;
	if (fibconfig_table_set->end_configuration(error_msg2) != XORP_OK) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }
    
    return (ret_value);
}

int
FibConfig::set_unicast_forwarding_entries_retain_on_startup4(bool retain,
							     string& error_msg)
{
    _unicast_forwarding_entries_retain_on_startup4 = retain;

    error_msg = "";		// XXX: reset
    return (XORP_OK);
}

int
FibConfig::set_unicast_forwarding_entries_retain_on_shutdown4(bool retain,
							      string& error_msg)
{
    _unicast_forwarding_entries_retain_on_shutdown4 = retain;

    error_msg = "";		// XXX: reset
    return (XORP_OK);
}

int
FibConfig::set_unicast_forwarding_entries_retain_on_startup6(bool retain,
							     string& error_msg)
{
    _unicast_forwarding_entries_retain_on_startup6 = retain;

    error_msg = "";		// XXX: reset
    return (XORP_OK);
}

int
FibConfig::set_unicast_forwarding_entries_retain_on_shutdown6(bool retain,
							      string& error_msg)
{
    _unicast_forwarding_entries_retain_on_shutdown6 = retain;

    error_msg = "";		// XXX: reset
    return (XORP_OK);
}

bool
FibConfig::unicast_forwarding_table_id_is_configured(int family) const
{
    switch (family) {
    case AF_INET:
	return (unicast_forwarding_table_id4_is_configured());
#ifdef HAVE_IPV6
    case AF_INET6:
	return (unicast_forwarding_table_id6_is_configured());
#endif
    default:
	XLOG_UNREACHABLE();
	break;
    }

    return (false);
}

uint32_t
FibConfig::unicast_forwarding_table_id(int family) const
{
    switch (family) {
    case AF_INET:
	return (unicast_forwarding_table_id4());
#ifdef HAVE_IPV6
    case AF_INET6:
	return (unicast_forwarding_table_id6());
#endif
    default:
	XLOG_UNREACHABLE();
	break;
    }

    return (0);
}

int
FibConfig::set_unicast_forwarding_table_id4(bool is_configured,
					    uint32_t table_id,
					    string& error_msg)
{
    if ((_unicast_forwarding_table_id4_is_configured != is_configured) ||
	(_unicast_forwarding_table_id4 != table_id)) {
	_unicast_forwarding_table_id4_is_configured = is_configured;
	_unicast_forwarding_table_id4 = table_id;
	propagate_table_id_change();
    }

    error_msg = "";		// XXX: reset
    return (XORP_OK);
}

int
FibConfig::set_unicast_forwarding_table_id6(bool is_configured,
					    uint32_t table_id,
					    string& error_msg)
{
    if ((_unicast_forwarding_table_id6_is_configured != is_configured) ||
	(_unicast_forwarding_table_id6 != table_id)) {
	_unicast_forwarding_table_id6_is_configured = is_configured;
	_unicast_forwarding_table_id6 = table_id;
	propagate_table_id_change();
    }

    error_msg = "";		// XXX: reset
    return (XORP_OK);
}

int
FibConfig::unicast_forwarding_enabled4(bool& ret_value,
				       string& error_msg) const
{
    if (_fibconfig_forwarding_plugins.empty()) {
	error_msg = c_format("No plugin to test whether IPv4 unicast "
			     "forwarding is enabled");
	return (XORP_ERROR);
    }

    //
    // XXX: We pull the information by using only the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (_fibconfig_forwarding_plugins.front()->unicast_forwarding_enabled4(
	    ret_value, error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::unicast_forwarding_enabled6(bool& ret_value,
				       string& error_msg) const
{
    if (_fibconfig_forwarding_plugins.empty()) {
	error_msg = c_format("No plugin to test whether IPv6 unicast "
			     "forwarding is enabled");
	return (XORP_ERROR);
    }

    //
    // XXX: We pull the information by using only the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (_fibconfig_forwarding_plugins.front()->unicast_forwarding_enabled6(
	    ret_value, error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::accept_rtadv_enabled6(bool& ret_value, string& error_msg) const
{
    if (_fibconfig_forwarding_plugins.empty()) {
	error_msg = c_format("No plugin to test whether IPv6 Router "
			     "Advertisement messages are accepted");
	return (XORP_ERROR);
    }

    //
    // XXX: We pull the information by using only the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (_fibconfig_forwarding_plugins.front()->accept_rtadv_enabled6(
		ret_value, error_msg)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::set_unicast_forwarding_enabled4(bool v, string& error_msg)
{
    list<FibConfigForwarding*>::iterator fibconfig_forwarding_iter;

    if (_fibconfig_forwarding_plugins.empty()) {
	error_msg = c_format("No plugin to configure the IPv4 unicast "
			     "forwarding");
	return (XORP_ERROR);
    }

    for (fibconfig_forwarding_iter = _fibconfig_forwarding_plugins.begin();
	 fibconfig_forwarding_iter != _fibconfig_forwarding_plugins.end();
	 ++fibconfig_forwarding_iter) {
	FibConfigForwarding* fibconfig_forwarding = *fibconfig_forwarding_iter;
	if (fibconfig_forwarding->set_unicast_forwarding_enabled4(v, error_msg)
	    != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::set_unicast_forwarding_enabled6(bool v, string& error_msg)
{
    list<FibConfigForwarding*>::iterator fibconfig_forwarding_iter;

    if (_fibconfig_forwarding_plugins.empty()) {
	error_msg = c_format("No plugin to configure the IPv6 unicast "
			     "forwarding");
	return (XORP_ERROR);
    }

    for (fibconfig_forwarding_iter = _fibconfig_forwarding_plugins.begin();
	 fibconfig_forwarding_iter != _fibconfig_forwarding_plugins.end();
	 ++fibconfig_forwarding_iter) {
	FibConfigForwarding* fibconfig_forwarding = *fibconfig_forwarding_iter;
	if (fibconfig_forwarding->set_unicast_forwarding_enabled6(v, error_msg)
	    != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::set_accept_rtadv_enabled6(bool v, string& error_msg)
{
    list<FibConfigForwarding*>::iterator fibconfig_forwarding_iter;

    if (_fibconfig_forwarding_plugins.empty()) {
	error_msg = c_format("No plugin to configure IPv6 Router "
			     "Advertisement messages acceptance");
	return (XORP_ERROR);
    }

    for (fibconfig_forwarding_iter = _fibconfig_forwarding_plugins.begin();
	 fibconfig_forwarding_iter != _fibconfig_forwarding_plugins.end();
	 ++fibconfig_forwarding_iter) {
	FibConfigForwarding* fibconfig_forwarding = *fibconfig_forwarding_iter;
	if (fibconfig_forwarding->set_accept_rtadv_enabled6(v, error_msg)
	    != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::add_entry4(const Fte4& fte)
{
    list<FibConfigEntrySet*>::iterator fibconfig_entry_set_iter;

    if (_fibconfig_entry_sets.empty())
	return (XORP_ERROR);

    PROFILE(if (_profile.enabled(profile_route_out))
		_profile.log(profile_route_out,
			     c_format("add %s", fte.net().str().c_str())));

    for (fibconfig_entry_set_iter = _fibconfig_entry_sets.begin();
	 fibconfig_entry_set_iter != _fibconfig_entry_sets.end();
	 ++fibconfig_entry_set_iter) {
	FibConfigEntrySet* fibconfig_entry_set = *fibconfig_entry_set_iter;
	if (fibconfig_entry_set->add_entry4(fte) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::delete_entry4(const Fte4& fte)
{
    list<FibConfigEntrySet*>::iterator fibconfig_entry_set_iter;

    if (_fibconfig_entry_sets.empty())
	return (XORP_ERROR);

    PROFILE(if (_profile.enabled(profile_route_out))
		_profile.log(profile_route_out,
			     c_format("delete %s", fte.net().str().c_str())));

    for (fibconfig_entry_set_iter = _fibconfig_entry_sets.begin();
	 fibconfig_entry_set_iter != _fibconfig_entry_sets.end();
	 ++fibconfig_entry_set_iter) {
	FibConfigEntrySet* fibconfig_entry_set = *fibconfig_entry_set_iter;
	if (fibconfig_entry_set->delete_entry4(fte) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::set_table4(const list<Fte4>& fte_list)
{
    list<FibConfigTableSet*>::iterator fibconfig_table_set_iter;

    if (_fibconfig_table_sets.empty())
	return (XORP_ERROR);

    for (fibconfig_table_set_iter = _fibconfig_table_sets.begin();
	 fibconfig_table_set_iter != _fibconfig_table_sets.end();
	 ++fibconfig_table_set_iter) {
	FibConfigTableSet* fibconfig_table_set = *fibconfig_table_set_iter;
	if (fibconfig_table_set->set_table4(fte_list) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::delete_all_entries4()
{
    list<FibConfigTableSet*>::iterator fibconfig_table_set_iter;

    if (_fibconfig_table_sets.empty())
	return (XORP_ERROR);

    for (fibconfig_table_set_iter = _fibconfig_table_sets.begin();
	 fibconfig_table_set_iter != _fibconfig_table_sets.end();
	 ++fibconfig_table_set_iter) {
	FibConfigTableSet* fibconfig_table_set = *fibconfig_table_set_iter;
	if (fibconfig_table_set->delete_all_entries4() != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::lookup_route_by_dest4(const IPv4& dst, Fte4& fte)
{
    if (_fibconfig_entry_gets.empty())
	return (XORP_ERROR);

    //
    // XXX: We pull the information by using only the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (_fibconfig_entry_gets.front()->lookup_route_by_dest4(dst, fte)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::lookup_route_by_network4(const IPv4Net& dst, Fte4& fte)
{
    if (_fibconfig_entry_gets.empty())
	return (XORP_ERROR);

    //
    // XXX: We pull the information by using only the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (_fibconfig_entry_gets.front()->lookup_route_by_network4(dst, fte)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::get_table4(list<Fte4>& fte_list)
{
    if (_fibconfig_table_gets.empty())
	return (XORP_ERROR);

    //
    // XXX: We pull the information by using only the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (_fibconfig_table_gets.front()->get_table4(fte_list) != XORP_OK)
	return (XORP_ERROR);

    return (XORP_OK);
}

int
FibConfig::add_entry6(const Fte6& fte)
{
    list<FibConfigEntrySet*>::iterator fibconfig_entry_set_iter;

    if (_fibconfig_entry_sets.empty())
	return (XORP_ERROR);

    PROFILE(if (_profile.enabled(profile_route_out))
		_profile.log(profile_route_out,
			     c_format("add %s", fte.net().str().c_str())));

    for (fibconfig_entry_set_iter = _fibconfig_entry_sets.begin();
	 fibconfig_entry_set_iter != _fibconfig_entry_sets.end();
	 ++fibconfig_entry_set_iter) {
	FibConfigEntrySet* fibconfig_entry_set = *fibconfig_entry_set_iter;
	if (fibconfig_entry_set->add_entry6(fte) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::delete_entry6(const Fte6& fte)
{
    list<FibConfigEntrySet*>::iterator fibconfig_entry_set_iter;

    if (_fibconfig_entry_sets.empty())
	return (XORP_ERROR);

    PROFILE(if (_profile.enabled(profile_route_out))
		_profile.log(profile_route_out,
			     c_format("delete %s", fte.net().str().c_str())));

    for (fibconfig_entry_set_iter = _fibconfig_entry_sets.begin();
	 fibconfig_entry_set_iter != _fibconfig_entry_sets.end();
	 ++fibconfig_entry_set_iter) {
	FibConfigEntrySet* fibconfig_entry_set = *fibconfig_entry_set_iter;
	if (fibconfig_entry_set->delete_entry6(fte) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::set_table6(const list<Fte6>& fte_list)
{
    list<FibConfigTableSet*>::iterator fibconfig_table_set_iter;

    if (_fibconfig_table_sets.empty())
	return (XORP_ERROR);

    for (fibconfig_table_set_iter = _fibconfig_table_sets.begin();
	 fibconfig_table_set_iter != _fibconfig_table_sets.end();
	 ++fibconfig_table_set_iter) {
	FibConfigTableSet* fibconfig_table_set = *fibconfig_table_set_iter;
	if (fibconfig_table_set->set_table6(fte_list) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::delete_all_entries6()
{
    list<FibConfigTableSet*>::iterator fibconfig_table_set_iter;

    if (_fibconfig_table_sets.empty())
	return (XORP_ERROR);

    for (fibconfig_table_set_iter = _fibconfig_table_sets.begin();
	 fibconfig_table_set_iter != _fibconfig_table_sets.end();
	 ++fibconfig_table_set_iter) {
	FibConfigTableSet* fibconfig_table_set = *fibconfig_table_set_iter;
	if (fibconfig_table_set->delete_all_entries6() != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::lookup_route_by_dest6(const IPv6& dst, Fte6& fte)
{
    if (_fibconfig_entry_gets.empty())
	return (XORP_ERROR);

    //
    // XXX: We pull the information by using only the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (_fibconfig_entry_gets.front()->lookup_route_by_dest6(dst, fte)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::lookup_route_by_network6(const IPv6Net& dst, Fte6& fte)
{
    if (_fibconfig_entry_gets.empty())
	return (XORP_ERROR);

    //
    // XXX: We pull the information by using only the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (_fibconfig_entry_gets.front()->lookup_route_by_network6(dst, fte)
	!= XORP_OK) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FibConfig::get_table6(list<Fte6>& fte_list)
{
    if (_fibconfig_table_gets.empty())
	return (XORP_ERROR);

    //
    // XXX: We pull the information by using only the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (_fibconfig_table_gets.front()->get_table6(fte_list) != XORP_OK)
	return (XORP_ERROR);

    return (XORP_OK);
}

int
FibConfig::add_fib_table_observer(FibTableObserverBase* fib_table_observer)
{
    if (find(_fib_table_observers.begin(),
	     _fib_table_observers.end(),
	     fib_table_observer)
	!= _fib_table_observers.end()) {
	// XXX: we have already added that observer
	return (XORP_OK);
    }

    _fib_table_observers.push_back(fib_table_observer);

    return (XORP_OK);
}

int
FibConfig::delete_fib_table_observer(FibTableObserverBase* fib_table_observer)
{
    list<FibTableObserverBase*>::iterator iter;

    iter = find(_fib_table_observers.begin(),
		_fib_table_observers.end(),
		fib_table_observer);
    if (iter == _fib_table_observers.end()) {
	// XXX: observer not found
	return (XORP_ERROR);
    }

    _fib_table_observers.erase(iter);

    return (XORP_OK);
}

void
FibConfig::propagate_fib_changes(const list<FteX>& fte_list,
				 const FibConfigTableObserver* fibconfig_table_observer)
{
    list<Fte4> fte_list4;
#ifdef HAVE_IPV6
    list<Fte6> fte_list6;
#endif
    list<FteX>::const_iterator ftex_iter;

    //
    // XXX: propagate the changes only from the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (_fibconfig_table_observers.empty()
	|| (_fibconfig_table_observers.front() != fibconfig_table_observer)) {
	return;
    }

    if (fte_list.empty())
	return;

    // Copy the FteX list into Fte4 and Fte6 lists
    for (ftex_iter = fte_list.begin();
	 ftex_iter != fte_list.end();
	 ++ftex_iter) {
	const FteX& ftex = *ftex_iter;
	if (ftex.net().is_ipv4()) {
	    // IPv4 entry
	    Fte4 fte4 = ftex.get_fte4();
	    fte_list4.push_back(fte4);
	}

#ifdef HAVE_IPV6
	if (ftex.net().is_ipv6()) {
	    // IPv6 entry
	    Fte6 fte6 = ftex.get_fte6();
	    fte_list6.push_back(fte6);
	}
#endif
    }

    // Inform all observers about the changes
    list<FibTableObserverBase*>::iterator iter;
    for (iter = _fib_table_observers.begin();
	 iter != _fib_table_observers.end();
	 ++iter) {
	FibTableObserverBase* fib_table_observer = *iter;
	if (! fte_list4.empty())
	    fib_table_observer->process_fib_changes(fte_list4);
#ifdef HAVE_IPV6
	if (! fte_list6.empty())
	    fib_table_observer->process_fib_changes(fte_list6);
#endif
    }
}
