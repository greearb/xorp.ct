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

#ident "$XORP: xorp/fea/ifconfig.cc,v 1.63 2007/04/30 23:40:28 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libcomm/comm_api.h"

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif

#include "ifconfig.hh"
#include "ifconfig_transaction.hh"


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

IfConfig::IfConfig(EventLoop& eventloop,
		   IfConfigUpdateReporterBase& ur,
		   IfConfigErrorReporterBase& er,
		   NexthopPortMapper& nexthop_port_mapper)
    : _eventloop(eventloop), _ur(ur), _er(er),
      _nexthop_port_mapper(nexthop_port_mapper),
      _itm(NULL),
      _restore_original_config_on_shutdown(false),
      _ifconfig_get_primary(NULL),
      _ifconfig_set_primary(NULL),
      _ifconfig_observer_primary(NULL),
      _ifconfig_get_dummy(*this),
      _ifconfig_get_ioctl(*this),
      _ifconfig_get_sysctl(*this),
      _ifconfig_get_getifaddrs(*this),
      _ifconfig_get_proc_linux(*this),
      _ifconfig_get_netlink_socket(*this),
      _ifconfig_get_iphelper(*this),
      _ifconfig_get_click(*this),
      _ifconfig_set_dummy(*this),
      _ifconfig_set_ioctl(*this),
      _ifconfig_set_netlink_socket(*this),
      _ifconfig_set_iphelper(*this),
      _ifconfig_set_click(*this),
      _ifconfig_observer_dummy(*this),
      _ifconfig_observer_routing_socket(*this),
      _ifconfig_observer_netlink_socket(*this),
      _ifconfig_observer_iphelper(*this),
      _have_ipv4(false),
      _have_ipv6(false),
      _is_dummy(false),
      _is_running(false)
{
    _itm = new IfConfigTransactionManager(_eventloop);

    //
    // Check that all necessary mechanisms to interact with the
    // underlying system are in place.
    //
    if (_ifconfig_get_primary == NULL) {
	XLOG_FATAL("No primary mechanism to get the network interface "
		   "information from the underlying system");
    }
    if (_ifconfig_set_primary == NULL) {
	XLOG_FATAL("No primary mechanism to set the network interface "
		   "information into the underlying system");
    }
    if (_ifconfig_observer_primary == NULL) {
	XLOG_FATAL("No primary mechanism to observe the network interface "
		   "information from the underlying system");
    }

    //
    // Test if the system supports IPv4 and IPv6 respectively
    //
    _have_ipv4 = test_have_ipv4();
    _have_ipv6 = test_have_ipv6();
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
IfConfig::register_ifconfig_get_primary(IfConfigGet *ifconfig_get)
{
    _ifconfig_get_primary = ifconfig_get;
    if (ifconfig_get != NULL)
	ifconfig_get->set_primary();

    return (XORP_OK);
}

int
IfConfig::register_ifconfig_set_primary(IfConfigSet *ifconfig_set)
{
    _ifconfig_set_primary = ifconfig_set;
    if (ifconfig_set != NULL)
	ifconfig_set->set_primary();

    return (XORP_OK);
}

int
IfConfig::register_ifconfig_observer_primary(IfConfigObserver *ifconfig_observer)
{
    _ifconfig_observer_primary = ifconfig_observer;
    if (ifconfig_observer != NULL)
	ifconfig_observer->set_primary();

    return (XORP_OK);
}

int
IfConfig::register_ifconfig_get_secondary(IfConfigGet *ifconfig_get)
{
    if (ifconfig_get != NULL) {
	_ifconfig_gets_secondary.push_back(ifconfig_get);
	ifconfig_get->set_secondary();
    }

    return (XORP_OK);
}

int
IfConfig::register_ifconfig_set_secondary(IfConfigSet *ifconfig_set)
{
    if (ifconfig_set != NULL) {
	_ifconfig_sets_secondary.push_back(ifconfig_set);
	ifconfig_set->set_secondary();
	if (ifconfig_set->is_running())
	    ifconfig_set->push_config(pushed_config());
    }

    return (XORP_OK);
}

int
IfConfig::register_ifconfig_observer_secondary(IfConfigObserver *ifconfig_observer)
{
    if (ifconfig_observer != NULL) {
	_ifconfig_observers_secondary.push_back(ifconfig_observer);
	ifconfig_observer->set_secondary();
    }

    return (XORP_OK);
}

int
IfConfig::set_dummy()
{
    register_ifconfig_get_primary(&_ifconfig_get_dummy);
    register_ifconfig_set_primary(&_ifconfig_set_dummy);
    register_ifconfig_observer_primary(&_ifconfig_observer_dummy);

    //
    // XXX: if we are dummy FEA, then we always have IPv4 and IPv6
    //
    _have_ipv4 = true;
    _have_ipv6 = true;

    _is_dummy = true;

    return (XORP_OK);
}

int
IfConfig::start(string& error_msg)
{
    list<IfConfigGet*>::iterator ifconfig_get_iter;
    list<IfConfigSet*>::iterator ifconfig_set_iter;
    list<IfConfigObserver*>::iterator ifconfig_observer_iter;

    if (_is_running)
	return (XORP_OK);

    //
    // Start the IfConfigGet methods
    //
    if (_ifconfig_get_primary != NULL) {
	if (_ifconfig_get_primary->start(error_msg) < 0)
	    return (XORP_ERROR);
    }
    for (ifconfig_get_iter = _ifconfig_gets_secondary.begin();
	 ifconfig_get_iter != _ifconfig_gets_secondary.end();
	 ++ifconfig_get_iter) {
	IfConfigGet* ifconfig_get = *ifconfig_get_iter;
	if (ifconfig_get->start(error_msg) < 0)
	    return (XORP_ERROR);
    }

    //
    // Start the IfConfigSet methods
    //
    if (_ifconfig_set_primary != NULL) {
	if (_ifconfig_set_primary->start(error_msg) < 0)
	    return (XORP_ERROR);
    }
    for (ifconfig_set_iter = _ifconfig_sets_secondary.begin();
	 ifconfig_set_iter != _ifconfig_sets_secondary.end();
	 ++ifconfig_set_iter) {
	IfConfigSet* ifconfig_set = *ifconfig_set_iter;
	if (ifconfig_set->start(error_msg) < 0)
	    return (XORP_ERROR);
    }

    //
    // Start the IfConfigObserver methods
    //
    if (_ifconfig_observer_primary != NULL) {
	if (_ifconfig_observer_primary->start(error_msg) < 0)
	    return (XORP_ERROR);
    }
    for (ifconfig_observer_iter = _ifconfig_observers_secondary.begin();
	 ifconfig_observer_iter != _ifconfig_observers_secondary.end();
	 ++ifconfig_observer_iter) {
	IfConfigObserver* ifconfig_observer = *ifconfig_observer_iter;
	if (ifconfig_observer->start(error_msg) < 0)
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
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }

    //
    // Stop the IfConfigObserver methods
    //
    for (ifconfig_observer_iter = _ifconfig_observers_secondary.begin();
	 ifconfig_observer_iter != _ifconfig_observers_secondary.end();
	 ++ifconfig_observer_iter) {
	IfConfigObserver* ifconfig_observer = *ifconfig_observer_iter;
	if (ifconfig_observer->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }
    if (_ifconfig_observer_primary != NULL) {
	if (_ifconfig_observer_primary->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }

    //
    // Stop the IfConfigSet methods
    //
    for (ifconfig_set_iter = _ifconfig_sets_secondary.begin();
	 ifconfig_set_iter != _ifconfig_sets_secondary.end();
	 ++ifconfig_set_iter) {
	IfConfigSet* ifconfig_set = *ifconfig_set_iter;
	if (ifconfig_set->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }
    if (_ifconfig_set_primary != NULL) {
	if (_ifconfig_set_primary->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }

    //
    // Stop the IfConfigGet methods
    //
    for (ifconfig_get_iter = _ifconfig_gets_secondary.begin();
	 ifconfig_get_iter != _ifconfig_gets_secondary.end();
	 ++ifconfig_get_iter) {
	IfConfigGet* ifconfig_get = *ifconfig_get_iter;
	if (ifconfig_get->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }
    if (_ifconfig_get_primary != NULL) {
	if (_ifconfig_get_primary->stop(error_msg2) < 0) {
	    ret_value = XORP_ERROR;
	    if (error_msg.empty())
		error_msg = error_msg2;
	}
    }

    _is_running = false;

    return (ret_value);
}

/**
 * Test if the underlying system supports IPv4.
 * 
 * @return true if the underlying system supports IPv4, otherwise false.
 */
bool
IfConfig::test_have_ipv4() const
{
    // XXX: always return true if running in dummy mode
    if (is_dummy())
	return true;

    XorpFd s = comm_sock_open(AF_INET, SOCK_DGRAM, 0, 0);
    if (!s.is_valid())
	return (false);
    
    comm_close(s);

    return (true);
}

/**
 * Test if the underlying system supports IPv6.
 * 
 * @return true if the underlying system supports IPv6, otherwise false.
 */
bool
IfConfig::test_have_ipv6() const
{
    // XXX: always return true if running in dummy mode
    if (is_dummy())
	return true;

#ifndef HAVE_IPV6
    return (false);
#else
    XorpFd s = comm_sock_open(AF_INET6, SOCK_DGRAM, 0, 0);
    if (!s.is_valid())
	return (false);
    
    comm_close(s);
    return (true);
#endif // HAVE_IPV6
}

/**
 * Enable/disable Click support.
 *
 * @param enable if true, then enable Click support, otherwise disable it.
 */
void
IfConfig::enable_click(bool enable)
{
    _ifconfig_get_click.enable_click(enable);
    _ifconfig_set_click.enable_click(enable);
}

/**
 * Start Click support.
 *
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
IfConfig::start_click(string& error_msg)
{
    if (_ifconfig_get_click.start(error_msg) < 0) {
	return (XORP_ERROR);
    }
    if (_ifconfig_set_click.start(error_msg) < 0) {
	string error_msg2;
	_ifconfig_get_click.stop(error_msg2);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * Stop Click support.
 *
 * @param error_msg the error message (if error).
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
IfConfig::stop_click(string& error_msg)
{
    if (_ifconfig_get_click.stop(error_msg) < 0) {
	string error_msg2;
	_ifconfig_set_click.stop(error_msg2);
	return (XORP_ERROR);
    }
    if (_ifconfig_set_click.stop(error_msg) < 0) {
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

/**
 * Specify the external program to generate the kernel-level Click
 * configuration.
 *
 * @param v the name of the external program to generate the kernel-level
 * Click configuration.
 */
void
IfConfig::set_kernel_click_config_generator_file(const string& v)
{
    _ifconfig_get_click.set_kernel_click_config_generator_file(v);
    _ifconfig_set_click.set_kernel_click_config_generator_file(v);
}

/**
 * Enable/disable kernel-level Click support.
 *
 * @param enable if true, then enable the kernel-level Click support,
 * otherwise disable it.
 */
void
IfConfig::enable_kernel_click(bool enable)
{
    _ifconfig_get_click.enable_kernel_click(enable);
    _ifconfig_set_click.enable_kernel_click(enable);
}

/**
 * Enable/disable installing kernel-level Click on startup.
 *
 * @param enable if true, then install kernel-level Click on startup.
 */
void
IfConfig::enable_kernel_click_install_on_startup(bool enable)
{
    // XXX: only IfConfigGet should install the kernel-level Click
    _ifconfig_get_click.enable_kernel_click_install_on_startup(enable);
    _ifconfig_set_click.enable_kernel_click_install_on_startup(false);
}

/**
 * Specify the list of kernel Click modules to load on startup if
 * installing kernel-level Click on startup is enabled.
 *
 * @param modules the list of kernel Click modules to load.
 */
void
IfConfig::set_kernel_click_modules(const list<string>& modules)
{
    _ifconfig_get_click.set_kernel_click_modules(modules);
    _ifconfig_set_click.set_kernel_click_modules(modules);
}

/**
 * Specify the kernel-level Click mount directory.
 *
 * @param directory the kernel-level Click mount directory.
 */
void
IfConfig::set_kernel_click_mount_directory(const string& directory)
{
    _ifconfig_get_click.set_kernel_click_mount_directory(directory);
    _ifconfig_set_click.set_kernel_click_mount_directory(directory);
}

/**
 * Enable/disable user-level Click support.
 *
 * @param enable if true, then enable the user-level Click support,
 * otherwise disable it.
 */
void
IfConfig::enable_user_click(bool enable)
{
    _ifconfig_get_click.enable_user_click(enable);
    _ifconfig_set_click.enable_user_click(enable);
}

/**
 * Specify the user-level Click command file.
 *
 * @param v the name of the user-level Click command file.
 */
void
IfConfig::set_user_click_command_file(const string& v)
{
    _ifconfig_get_click.set_user_click_command_file(v);
    _ifconfig_set_click.set_user_click_command_file(v);
}

/**
 * Specify the extra arguments to the user-level Click command.
 *
 * @param v the extra arguments to the user-level Click command.
 */
void
IfConfig::set_user_click_command_extra_arguments(const string& v)
{
    _ifconfig_get_click.set_user_click_command_extra_arguments(v);
    _ifconfig_set_click.set_user_click_command_extra_arguments(v);
}

/**
 * Specify whether to execute on startup the user-level Click command.
 *
 * @param v if true, then execute the user-level Click command on startup.
 */
void
IfConfig::set_user_click_command_execute_on_startup(bool v)
{
    // XXX: only IfConfigGet should execute the user-level Click command
    _ifconfig_get_click.set_user_click_command_execute_on_startup(v);
    _ifconfig_set_click.set_user_click_command_execute_on_startup(false);
}

/**
 * Specify the address to use for control access to the user-level
 * Click.
 *
 * @param v the address to use for control access to the user-level Click.
 */
void
IfConfig::set_user_click_control_address(const IPv4& v)
{
    _ifconfig_get_click.set_user_click_control_address(v);
    _ifconfig_set_click.set_user_click_control_address(v);
}

/**
 * Specify the socket port to use for control access to the user-level
 * Click.
 *
 * @param v the socket port to use for control access to the user-level
 * Click.
 */
void
IfConfig::set_user_click_control_socket_port(uint32_t v)
{
    _ifconfig_get_click.set_user_click_control_socket_port(v);
    _ifconfig_set_click.set_user_click_control_socket_port(v);
}

/**
 * Specify the configuration file to be used by user-level Click on
 * startup.
 *
 * @param v the name of the configuration file to be used by user-level
 * Click on startup.
 */
void
IfConfig::set_user_click_startup_config_file(const string& v)
{
    _ifconfig_get_click.set_user_click_startup_config_file(v);
    _ifconfig_set_click.set_user_click_startup_config_file(v);
}

/**
 * Specify the external program to generate the user-level Click
 * configuration.
 *
 * @param v the name of the external program to generate the user-level
 * Click configuration.
 */
void
IfConfig::set_user_click_config_generator_file(const string& v)
{
    _ifconfig_get_click.set_user_click_config_generator_file(v);
    _ifconfig_set_click.set_user_click_config_generator_file(v);
}

bool
IfConfig::push_config(IfTree& config)
{
    bool ret_value = false;
    list<IfConfigSet*>::iterator ifconfig_set_iter;

    if ((_ifconfig_set_primary == NULL) && _ifconfig_sets_secondary.empty())
	goto ret_label;

    //
    // XXX: explicitly pull the current config so we can align
    // the new config with the current config.
    //
    pull_config();

    if (_ifconfig_set_primary != NULL) {
	if (_ifconfig_set_primary->push_config(config) != true)
	    goto ret_label;
    }
    for (ifconfig_set_iter = _ifconfig_sets_secondary.begin();
	 ifconfig_set_iter != _ifconfig_sets_secondary.end();
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

    if (_ifconfig_get_primary != NULL)
	_ifconfig_get_primary->pull_config(_pulled_config);

    return _pulled_config;
}

bool
IfConfig::report_update(const IfTreeInterface& fi)
{
    IfConfigUpdateReporterBase::Update u;
    if (map_changes(fi.state(), u)) {
	_ur.interface_update(fi.ifname(), u);
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
	_ur.vif_update(fi.ifname(), fv.vifname(), u);
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
	_ur.vifaddr4_update(fi.ifname(), fv.vifname(), fa.addr(), u);
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
	_ur.vifaddr6_update(fi.ifname(), fv.ifname(), fa.addr(), u);
	return true;
    }

    return false;
}

void
IfConfig::report_updates_completed()
{
    _ur.updates_completed();
}

void
IfConfig::report_updates(IfTree& it, bool is_system_interfaces_reportee)
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
    for (IfTree::IfMap::const_iterator ii = it.ifs().begin();
	 ii != it.ifs().end(); ++ii) {

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
    for (IfTree::IfMap::iterator ii = it.ifs().begin();
	 ii != it.ifs().end(); ++ii) {
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
    for (IfTree::IfMap::iterator ii = it.ifs().begin();
	 ii != it.ifs().end(); ++ii) {
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
    return _er.first_error();
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
