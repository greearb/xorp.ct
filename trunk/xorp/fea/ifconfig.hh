// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/fea/ifconfig.hh,v 1.80 2008/07/23 05:10:09 pavlin Exp $

#ifndef __FEA_IFCONFIG_HH__
#define __FEA_IFCONFIG_HH__

#include "libxorp/status_codes.h"
#include "libxorp/transaction.hh"

#include "ifconfig_property.hh"
#include "ifconfig_get.hh"
#include "ifconfig_set.hh"
#include "ifconfig_observer.hh"
#include "ifconfig_vlan_get.hh"
#include "ifconfig_vlan_set.hh"
#include "ifconfig_reporter.hh"
#include "iftree.hh"

class EventLoop;
class FeaNode;
class IfConfigErrorReporterBase;
class IfConfigTransactionManager;
class IfConfigUpdateReporterBase;
class NexthopPortMapper;


/**
 * Base class for pushing and pulling interface configurations down to
 * the particular system.
 */
class IfConfig {
public:
    /**
     * Constructor.
     *
     * @param fea_node the FEA node.
     */
    IfConfig(FeaNode& fea_node);

    /**
     * Virtual destructor (in case this class is used as a base class).
     */
    virtual ~IfConfig();

    /**
     * Get a reference to the @ref EventLoop instance.
     *
     * @return a reference to the @ref EventLoop instance.
     */
    EventLoop& eventloop() { return _eventloop; }

    /**
     * Get the status code.
     *
     * @param reason the human-readable reason for any failure.
     * @return the status code.
     */
    ProcessStatus status(string& reason) const;

    /**
     * Start interface-related transaction.
     *
     * @param tid the return-by-reference new transaction ID.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int start_transaction(uint32_t& tid, string& error_msg);

    /**
     * Commit interface-related transaction.
     *
     * @param tid the transaction ID.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int commit_transaction(uint32_t tid, string& error_msg);

    /**
     * Abort interface-related transaction.
     *
     * @param tid the transaction ID.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int abort_transaction(uint32_t tid, string& error_msg);

    /**
     * Add operation to interface-related transaction.
     *
     * @param tid the transaction ID.
     * @param op the operation to add.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_transaction_operation(uint32_t tid,
				  const TransactionManager::Operation& op,
				  string& error_msg);

    /**
     * Get a reference to the @ref NexthopPortMapper instance.
     *
     * @return a reference to the @ref NexthopPortMapper instance.
     */
    NexthopPortMapper& nexthop_port_mapper() { return (_nexthop_port_mapper); }

    /**
     * Get the IfConfigUpdateReplicator instance.
     *
     * @return a reference to the IfConfigUpdateReplicator instance
     * (@ref IfConfigUpdateReplicator).
     */
    IfConfigUpdateReplicator& ifconfig_update_replicator() {
	return (_ifconfig_update_replicator);
    }

    /**
     * Get the error reporter associated with IfConfig.
     *
     * @return the error reporter associated with IfConfig.
     */
    IfConfigErrorReporterBase& ifconfig_error_reporter() {
	return (_ifconfig_error_reporter);
    }

    /**
     * Get a reference to the system interface configuration.
     *
     * @return a reference to the system interface configuration.
     */
    IfTree& system_config() { return (_system_config); }

    /**
     * Set the system interface configuration.
     *
     * @param iftree the system interface configuration.
     */
    void set_system_config(const IfTree& iftree) { _system_config = iftree; }

    /**
     * Get a reference to the user interface configuration.
     *
     * @return a reference to the user interface configuration.
     */
    IfTree& user_config()		{ return (_user_config); }

    /**
     * Set the user interface configuration.
     *
     * @param iftree the user interface configuration.
     */
    void set_user_config(const IfTree& iftree) { _user_config = iftree; }

    /**
     * Get a reference to the merged system-user configuration.
     *
     * @return a reference to the merged system-user configuration.
     */
    IfTree& merged_config()		{ return (_merged_config); }

    /**
     * Set the merged system-user configuration.
     *
     * @param iftree the merged system-user configuration.
     */
    void set_merged_config(const IfTree& iftree) { _merged_config = iftree; }

    /**
     * Get a reference to the original interface configuration on startup.
     *
     * @return a reference to the original interface configuration on startup.
     */
    const IfTree& original_config()	{ return (_original_config); }

    /**
     * Test whether the original configuration should be restored on shutdown.
     *
     * @return true of the original configuration should be restored on
     * shutdown, otherwise false.
     */
    bool restore_original_config_on_shutdown() const {
	return (_restore_original_config_on_shutdown);
    }

    /**
     * Set the flag whether the original configuration should be restored
     * on shutdown.
     *
     * @param v if true the original configuration should be restored on
     * shutdown.
     */
    void set_restore_original_config_on_shutdown(bool v) {
	_restore_original_config_on_shutdown = v;
    }

    /**
     * Register @ref IfConfigProperty plugin.
     *
     * @param ifconfig_property the plugin to register.
     * @param is_exclusive if true, the plugin is registered as the
     * exclusive plugin, otherwise is added to the list of plugins.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_ifconfig_property(IfConfigProperty* ifconfig_property,
				   bool is_exclusive);

    /**
     * Unregister @ref IfConfigProperty plugin.
     *
     * @param ifconfig_property the plugin to unregister.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_ifconfig_property(IfConfigProperty* ifconfig_property);

    /**
     * Register @ref IfConfigGet plugin.
     *
     * @param ifconfig_get the plugin to register.
     * @param is_exclusive if true, the plugin is registered as the
     * exclusive plugin, otherwise is added to the list of plugins.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_ifconfig_get(IfConfigGet* ifconfig_get, bool is_exclusive);

    /**
     * Unregister @ref IfConfigGet plugin.
     *
     * @param ifconfig_get the plugin to unregister.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_ifconfig_get(IfConfigGet* ifconfig_get);

    /**
     * Register @ref IfConfigSet plugin.
     *
     * @param ifconfig_set the plugin to register.
     * @param is_exclusive if true, the plugin is registered as the
     * exclusive plugin, otherwise is added to the list of plugins.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_ifconfig_set(IfConfigSet* ifconfig_set, bool is_exclusive);

    /**
     * Unregister @ref IfConfigSet plugin.
     *
     * @param ifconfig_set the plugin to unregister.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_ifconfig_set(IfConfigSet* ifconfig_set);

    /**
     * Register @ref IfConfigObserver plugin.
     *
     * @param ifconfig_observer the plugin to register.
     * @param is_exclusive if true, the plugin is registered as the
     * exclusive plugin, otherwise is added to the list of plugins.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_ifconfig_observer(IfConfigObserver* ifconfig_observer,
				   bool is_exclusive);

    /**
     * Unregister @ref IfConfigObserver plugin.
     *
     * @param ifconfig_observer the plugin to unregister.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_ifconfig_observer(IfConfigObserver* ifconfig_observer);

    /**
     * Register @ref IfConfigVlanGet plugin.
     *
     * @param ifconfig_vlan_get the plugin to register.
     * @param is_exclusive if true, the plugin is registered as the
     * exclusive plugin, otherwise is added to the list of plugins.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_ifconfig_vlan_get(IfConfigVlanGet* ifconfig_vlan_get,
				   bool is_exclusive);

    /**
     * Unregister @ref IfConfigVlanGet plugin.
     *
     * @param ifconfig_vlan_get the plugin to unregister.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_ifconfig_vlan_get(IfConfigVlanGet* ifconfig_vlan_get);

    /**
     * Register @ref IfConfigVlanSet plugin.
     *
     * @param ifconfig_vlan_set the plugin to register.
     * @param is_exclusive if true, the plugin is registered as the
     * exclusive plugin, otherwise is added to the list of plugins.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_ifconfig_vlan_set(IfConfigVlanSet* ifconfig_vlan_set,
				   bool is_exclusive);

    /**
     * Unregister @ref IfConfigVlanSet plugin.
     *
     * @param ifconfig_vlan_set the plugin to unregister.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_ifconfig_vlan_set(IfConfigVlanSet* ifconfig_vlan_set);

    /**
     * Start operation.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int start(string& error_msg);

    /**
     * Stop operation.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int stop(string& error_msg);

    /**
     * Push interface configuration down to the system.
     *
     * Errors are reported via the ifconfig_error_reporter() instance.
     * Note that on return some of the interface configuration state
     * may be modified.
     *
     * @param iftree the interface configuration to be pushed down.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int push_config(IfTree& iftree);

    /**
     * Get the error message associated with a push operation.
     *
     * @return the error message associated with a push operation.
     */
    const string& push_error() const;

    /**
     * Pull up current interface configuration from the system.
     *
     * @return the current interface configuration from the system.
     */
    const IfTree& pull_config();

    /**
     * Check IfTreeInterface and report updates to IfConfigUpdateReporter.
     *
     * @param fi the @ref IfTreeInterface interface instance to check.
     * @return true if there were updates to report, otherwise false.
     */
    bool report_update(const IfTreeInterface& fi);

    /**
     * Check IfTreeVif and report updates to IfConfigUpdateReporter.
     *
     * @param fi the @ref IfTreeInterface interface instance to check.
     * @param fv the @ref IfTreeVif vif instance to check.
     * @return true if there were updates to report, otherwise false.
     */
    bool report_update(const IfTreeInterface& fi, const IfTreeVif& fv);

    /**
     * Check IfTreeAddr4 and report updates to IfConfigUpdateReporter.
     *
     * @param fi the @ref IfTreeInterface interface instance to check.
     * @param fv the @ref IfTreeVif vif instance to check.
     * @param fa the @ref IfTreeAddr4 address instance to check.
     * @return true if there were updates to report, otherwise false.
     */
    bool report_update(const IfTreeInterface&	fi,
		       const IfTreeVif&		fv,
		       const IfTreeAddr4&	fa);

    /**
     * Check IfTreeAddr6 and report updates to IfConfigUpdateReporter.
     *
     * @param fi the @ref IfTreeInterface interface instance to check.
     * @param fv the @ref IfTreeVif vif instance to check.
     * @param fa the @ref IfTreeAddr6 address instance to check.
     * @return true if there were updates to report, otherwise false.
     */
    bool report_update(const IfTreeInterface&	fi,
		       const IfTreeVif&		fv,
		       const IfTreeAddr6&	fa);

    /**
     * Report that updates were completed to IfConfigUpdateReporter.
     */
    void report_updates_completed();

    /**
     * Check every item within IfTree and report updates to
     * IfConfigUpdateReporter.
     *
     * @param iftree the interface tree instance to check.
     */
    void report_updates(IfTree& iftree);

private:
    /**
     * Restore the interface configuration.
     *
     * @param old_user_config the old user configuration to restore.
     * @param old_system_config the old system configuration to restore.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int restore_config(const IfTree& old_user_config,
		       const IfTree& old_system_config,
		       string& error_msg);

    FeaNode&			_fea_node;
    EventLoop&			_eventloop;
    NexthopPortMapper&		_nexthop_port_mapper;
    IfConfigTransactionManager* _itm;	// The interface transaction manager

    IfTree		_user_config;	// The IfTree with the user config
    IfTree		_system_config;	// The IfTree with the system config
    IfTree		_merged_config; // The merged system-user config
    IfTree		_original_config; // The IfTree on startup
    bool		_restore_original_config_on_shutdown; // If true, then
				//  restore the original config on shutdown

    IfConfigUpdateReplicator	_ifconfig_update_replicator;
    IfConfigErrorReporter	_ifconfig_error_reporter;

    //
    // The registered plugins
    //
    list<IfConfigProperty*>	_ifconfig_property_plugins;
    list<IfConfigGet*>		_ifconfig_gets;
    list<IfConfigSet*>		_ifconfig_sets;
    list<IfConfigObserver*>	_ifconfig_observers;
    list<IfConfigVlanGet*>	_ifconfig_vlan_gets;
    list<IfConfigVlanSet*>	_ifconfig_vlan_sets;

    //
    // Misc other state
    //
    bool	_is_running;
};

#endif // __FEA_IFCONFIG_HH__
