// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/ifconfig.cc,v 1.69 2007/08/09 00:46:55 pavlin Exp $"

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
      _restore_original_config_on_shutdown(false),
      _ifconfig_update_replicator(_local_config),
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

int
IfConfig::commit_transaction(uint32_t tid, string& error_msg)
{
    bool is_error = false;

    //
    // XXX: Pull in advance the current config, in case it is needed
    // by some of the transaction operations.
    //
    pull_config();

    if (_itm->commit(tid) != true) {
	error_msg = c_format("Expired or invalid transaction ID presented");
	return (XORP_ERROR);
    }

    if (_itm->error().empty() != true) {
	error_msg = _itm->error();
	return (XORP_ERROR);
    }

    //
    // If we get here we have updated the local copy of the config
    // successfully.
    //

    //
    // Push the configuration
    //
    do {
	if (push_config(local_config()) == true)
	    break;		// Success

	error_msg = push_error();
	is_error = true;

	// Reverse-back to the previously working configuration
	IfTree restore_config = old_local_config();
	restore_config.prepare_replacement_state(local_config());
	set_local_config(restore_config);
	if (push_config(local_config()) == true)
	    break;		// Continue with finalizing the reverse-back

	// Failed to reverse back
	string tmp_error_msg = push_error();
	tmp_error_msg = c_format("[Also, failed to reverse-back to the "
				 "previous config: %s]",
				 tmp_error_msg.c_str());
	error_msg = error_msg + " " + tmp_error_msg;
	break;
    } while (false);

    //
    // Pull the new device configuration
    //
    const IfTree& dev_config = pull_config();
    debug_msg("DEV CONFIG %s\n", dev_config.str().c_str());
    debug_msg("LOCAL CONFIG %s\n", local_config().str().c_str());

    //
    // Align with device configuration, so that any stuff that failed
    // in push is not held over in config.
    //
    local_config().align_with(dev_config);
    debug_msg("LOCAL CONFIG AFTER ALIGN %s\n", local_config().str().c_str());

    //
    // Prune deleted state that was never added earlier
    //
    local_config().prune_bogus_deleted_state(old_local_config());

    //
    // Propagate the configuration changes to all listeners.
    //
    if (! is_error)
	report_updates(local_config(), false);

    //
    // Flush-out config state
    //
    local_config().finalize_state();

    //
    // Save a backup copy of the configuration state
    //
    if (! is_error) {
	set_old_local_config(local_config());
	old_local_config().finalize_state();
    }

    if (is_error)
	return (XORP_ERROR);

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
	    ifconfig_set->push_config(pushed_config());
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
	if (ifconfig_vlan_set->is_running())
	    ifconfig_vlan_set->push_config(pushed_config());
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
    // Start the IfConfigGet methods
    //
    for (ifconfig_get_iter = _ifconfig_gets.begin();
	 ifconfig_get_iter != _ifconfig_gets.end();
	 ++ifconfig_get_iter) {
	IfConfigGet* ifconfig_get = *ifconfig_get_iter;
	if (ifconfig_get->start(error_msg) < 0)
	    return (XORP_ERROR);
    }

    //
    // Start the IfConfigSet methods
    //
    for (ifconfig_set_iter = _ifconfig_sets.begin();
	 ifconfig_set_iter != _ifconfig_sets.end();
	 ++ifconfig_set_iter) {
	IfConfigSet* ifconfig_set = *ifconfig_set_iter;
	if (ifconfig_set->start(error_msg) < 0)
	    return (XORP_ERROR);
    }

    //
    // Start the IfConfigObserver methods
    //
    for (ifconfig_observer_iter = _ifconfig_observers.begin();
	 ifconfig_observer_iter != _ifconfig_observers.end();
	 ++ifconfig_observer_iter) {
	IfConfigObserver* ifconfig_observer = *ifconfig_observer_iter;
	if (ifconfig_observer->start(error_msg) < 0)
	    return (XORP_ERROR);
    }

    //
    // Start the IfConfigVlanGet methods
    //
    for (ifconfig_vlan_get_iter = _ifconfig_vlan_gets.begin();
	 ifconfig_vlan_get_iter != _ifconfig_vlan_gets.end();
	 ++ifconfig_vlan_get_iter) {
	IfConfigVlanGet* ifconfig_vlan_get = *ifconfig_vlan_get_iter;
	if (ifconfig_vlan_get->start(error_msg) < 0)
	    return (XORP_ERROR);
    }

    //
    // Start the IfConfigSetSet methods
    //
    for (ifconfig_vlan_set_iter = _ifconfig_vlan_sets.begin();
	 ifconfig_vlan_set_iter != _ifconfig_vlan_sets.end();
	 ++ifconfig_vlan_set_iter) {
	IfConfigVlanSet* ifconfig_vlan_set = *ifconfig_vlan_set_iter;
	if (ifconfig_vlan_set->start(error_msg) < 0)
	    return (XORP_ERROR);
    }

    _live_config = pull_config();
    _live_config.finalize_state();

    _original_config = _live_config;
    _original_config.finalize_state();

    debug_msg("Start configuration read: %s\n", _live_config.str().c_str());
    debug_msg("\nEnd configuration read.\n");

    _is_running = true;

    return (XORP_OK);
}

int
IfConfig::stop(string& error_msg)
{
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
	pull_config();
	IfTree tmp_push_tree = _original_config;
	tmp_push_tree.prepare_replacement_state(_pulled_config);
	if (push_config(tmp_push_tree) != true) {
	    error_msg2 = push_error();
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
	if (ifconfig_vlan_set->stop(error_msg2) < 0) {
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
	if (ifconfig_vlan_get->stop(error_msg2) < 0) {
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
	if (ifconfig_observer->stop(error_msg2) < 0) {
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
	if (ifconfig_set->stop(error_msg2) < 0) {
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
	if (ifconfig_get->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (! error_msg.empty())
		error_msg += " ";
	    error_msg += error_msg2;
	}
    }

    _is_running = false;

    return (ret_value);
}

bool
IfConfig::push_config(IfTree& config)
{
    bool ret_value = false;
    list<IfConfigSet*>::iterator ifconfig_set_iter;

    if (_ifconfig_sets.empty())
	goto ret_label;

    //
    // XXX: explicitly pull the current config so we can align
    // the new config with the current config.
    //
    pull_config();

    for (ifconfig_set_iter = _ifconfig_sets.begin();
	 ifconfig_set_iter != _ifconfig_sets.end();
	 ++ifconfig_set_iter) {
	IfConfigSet* ifconfig_set = *ifconfig_set_iter;
	if (ifconfig_set->push_config(config) != true)
	    goto ret_label;
    }
    ret_value = true;		// Success

 ret_label:
    //
    // Save a copy of the pushed config
    //
    if (ret_value == true)
	_pushed_config = config;

    return ret_value;
}

const IfTree&
IfConfig::pull_config()
{
    // Clear the old state
    _pulled_config.clear();

    //
    // XXX: We pull the configuration by using only the first method.
    // In the future we need to rething this and be more flexible.
    //
    if (! _ifconfig_gets.empty()) {
	IfConfigGet* ifconfig_get = _ifconfig_gets.front();
	ifconfig_get->pull_config(_pulled_config);
    }

    return _pulled_config;
}

bool
IfConfig::report_update(const IfTreeInterface& fi)
{
    IfConfigUpdateReporterBase::Update u;
    if (map_changes(fi.state(), u)) {
	_ifconfig_update_replicator.interface_update(fi.ifname(), u);
	return true;
    }

    return false;
}

bool
IfConfig::report_update(const IfTreeInterface&	fi,
			const IfTreeVif&	fv)
{
    IfConfigUpdateReporterBase::Update u;
    if (map_changes(fv.state(), u)) {
	_ifconfig_update_replicator.vif_update(fi.ifname(), fv.vifname(), u);
	return true;
    }

    return false;
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
	return true;
    }

    return false;
}

bool
IfConfig::report_update(const IfTreeInterface&	fi,
			const IfTreeVif&	fv,
			const IfTreeAddr6&	fa)
{
    IfConfigUpdateReporterBase::Update u;
    if (map_changes(fa.state(), u)) {
	_ifconfig_update_replicator.vifaddr6_update(fi.ifname(), fv.ifname(),
						    fa.addr(), u);
	return true;
    }

    return false;
}

void
IfConfig::report_updates_completed()
{
    _ifconfig_update_replicator.updates_completed();
}

void
IfConfig::report_updates(IfTree& iftree, bool is_system_interfaces_reportee)
{
    bool updated = false;

    //
    // XXX: For now don't propagate any changes from from the kernel.
    // TODO: We need to propagate the changes for interfaces that
    // have been configured with the default-system-config configuration
    // statemeent.
    //
    if (is_system_interfaces_reportee)
	return;

    //
    // Walk config looking for changes to report
    //
    for (IfTree::IfMap::const_iterator ii = iftree.interfaces().begin();
	 ii != iftree.interfaces().end(); ++ii) {

	const IfTreeInterface& interface = ii->second;
	updated |= report_update(interface);

	IfTreeInterface::VifMap::const_iterator vi;
	for (vi = interface.vifs().begin();
	     vi != interface.vifs().end(); ++vi) {

	    const IfTreeVif& vif = vi->second;
	    updated |= report_update(interface, vif);

	    for (IfTreeVif::IPv4Map::const_iterator ai = vif.ipv4addrs().begin();
		 ai != vif.ipv4addrs().end(); ai++) {
		const IfTreeAddr4& addr = ai->second;
		updated |= report_update(interface, vif, addr);
	    }

	    for (IfTreeVif::IPv6Map::const_iterator ai = vif.ipv6addrs().begin();
		 ai != vif.ipv6addrs().end(); ai++) {
		const IfTreeAddr6& addr = ai->second;
		updated |= report_update(interface, vif, addr);
	    }
	}
    }
    if (updated) {
	// Complete the update
	report_updates_completed();
    }

    //
    // Walk again and flip the enable flag for the corresponding interfaces.
    //
    // First, disable all interfaces that were flipped, and are still enabled,
    // and report the changes.
    // Then, enable those interfaces and report again the changes.
    // The main idea is to inform all interested parties about the fact
    // that an interface was disabled and enabled internally during the
    // process of writing the information to the kernel.
    //
    // Disable all flipped interfaces
    updated = false;
    for (IfTree::IfMap::iterator ii = iftree.interfaces().begin();
	 ii != iftree.interfaces().end(); ++ii) {
	IfTreeInterface& interface = ii->second;
	if (! interface.flipped())
	    continue;
	if (! interface.enabled()) {
	    interface.set_flipped(false);
	    continue;
	}
	interface.set_enabled(false);
	updated |= report_update(interface);
    }
    if (updated) {
	// Complete the update
	report_updates_completed();
    }

    // Enable all flipped interfaces
    updated = false;
    for (IfTree::IfMap::iterator ii = iftree.interfaces().begin();
	 ii != iftree.interfaces().end(); ++ii) {
	IfTreeInterface& interface = ii->second;

	if (! interface.flipped())
	    continue;
	interface.set_flipped(false);
	if (interface.enabled()) {
	    continue;
	}
	interface.set_enabled(true);
	updated |= report_update(interface);
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

void
IfConfig::map_ifindex(uint32_t ifindex, const string& ifname)
{
    _ifnames[ifindex] = ifname;
}

void
IfConfig::unmap_ifindex(uint32_t ifindex)
{
    IfIndex2NameMap::iterator i = _ifnames.find(ifindex);
    if (_ifnames.end() != i)
	_ifnames.erase(i);
}

const char *
IfConfig::find_ifname(uint32_t ifindex) const
{
    IfIndex2NameMap::const_iterator i = _ifnames.find(ifindex);

    if (_ifnames.end() == i)
	return NULL;

    return i->second.c_str();
}

uint32_t
IfConfig::find_ifindex(const string& ifname) const
{
    IfIndex2NameMap::const_iterator i;

    for (i = _ifnames.begin(); i != _ifnames.end(); ++i) {
	if (i->second == ifname)
	    return i->first;
    }

    return 0;
}

const char *
IfConfig::get_insert_ifname(uint32_t ifindex)
{
    IfIndex2NameMap::const_iterator i = _ifnames.find(ifindex);

    if (_ifnames.end() == i) {
#ifdef HAVE_IF_INDEXTONAME
	char name_buf[IF_NAMESIZE];
	const char* ifname = if_indextoname(ifindex, name_buf);

	if (ifname != NULL)
	    map_ifindex(ifindex, ifname);

	return ifname;
#else
	return NULL;
#endif // ! HAVE_IF_INDEXTONAME
    }
    return i->second.c_str();
}

uint32_t
IfConfig::get_insert_ifindex(const string& ifname)
{
    IfIndex2NameMap::const_iterator i;

    for (i = _ifnames.begin(); i != _ifnames.end(); ++i) {
	if (i->second == ifname)
	    return i->first;
    }

#ifdef HAVE_IF_NAMETOINDEX
    uint32_t ifindex = if_nametoindex(ifname.c_str());
    if (ifindex > 0)
	map_ifindex(ifindex, ifname);
    return ifindex;
#else
    return 0;
#endif // ! HAVE_IF_NAMETOINDEX
}
