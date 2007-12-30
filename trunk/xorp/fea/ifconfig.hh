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

// $XORP: xorp/fea/ifconfig.hh,v 1.75 2007/12/28 05:12:35 pavlin Exp $

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
     * @return a reference to the IfConfigUpdateReplicator instance.
     * @see IfConfigUpdateReplicator.
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
     * Get a reference to the live interface configuration.
     *
     * @return a reference to the live interface configuration.
     */
    IfTree& live_config() { return (_live_config); }

    /**
     * Set the live interface configuration.
     *
     * @param iftree the live interface configuration.
     */
    void set_live_config(const IfTree& iftree) { _live_config = iftree; }

    /**
     * Get a reference to the pulled interface configuration from the system.
     *
     * @return a reference to the pulled interface configuration from the
     * system.
     */
    const IfTree& pulled_config()	{ return (_pulled_config); }

    /**
     * Get a reference to the pushed interface configuration.
     *
     * @return a reference to the pushed interface configuration.
     */
    IfTree& pushed_config()		{ return (_pushed_config); }

    /**
     * Get a reference to the original interface configuration on startup.
     *
     * @return a reference to the original interface configuration on startup.
     */
    const IfTree& original_config()	{ return (_original_config); }

    /**
     * Get a reference to the local interface configuration.
     *
     * @return a reference to the local interface configuration.
     */
    IfTree& local_config()		{ return (_local_config); }

    /**
     * Set the local interface configuration.
     *
     * @param iftree the local interface configuration.
     */
    void set_local_config(const IfTree& iftree) { _local_config = iftree; }

    /**
     * Get a reference to the old local interface configuration.
     *
     * @return a reference to the old local interface configuration.
     */
    IfTree& old_local_config()		{ return (_old_local_config); }

    /**
     * Set the old local interface configuration.
     *
     * @param iftree the old local interface configuration.
     */
    void set_old_local_config(const IfTree& iftree) {
	_old_local_config = iftree;
    }

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
     * Pull up current interface configuration from the system.
     *
     * @return the current interface configuration from the system.
     */
    const IfTree& pull_config();

    /**
     * Flush the live interface configuration.
     */
    void flush_config() { _live_config.clear(); }

    /**
     * Get the error message associated with a push operation.
     *
     * @return the error message associated with a push operation.
     */
    const string& push_error() const;

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
    FeaNode&			_fea_node;
    EventLoop&			_eventloop;
    NexthopPortMapper&		_nexthop_port_mapper;
    IfConfigTransactionManager* _itm;	// The interface transaction manager

    IfTree		_live_config;	// The IfTree with live config
    IfTree		_pulled_config;	// The IfTree when we pull the config
    IfTree		_pushed_config;	// The IfTree when we push the config
    IfTree		_original_config; // The IfTree on startup
    bool		_restore_original_config_on_shutdown; // If true, then
				//  restore the original config on shutdown
    IfTree		_local_config;	// The IfTree with the local config
    IfTree		_old_local_config; // The IfTree with the old local config

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
