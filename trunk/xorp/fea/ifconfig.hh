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

// $XORP: xorp/fea/ifconfig.hh,v 1.66 2007/07/11 22:18:03 pavlin Exp $

#ifndef __FEA_IFCONFIG_HH__
#define __FEA_IFCONFIG_HH__

#include "libxorp/status_codes.h"
#include "libxorp/transaction.hh"

#include "ifconfig_addr_table.hh"
#include "ifconfig_get.hh"
#include "ifconfig_set.hh"
#include "ifconfig_observer.hh"
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
 * the particular platform.
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
     * Get the status code
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
    NexthopPortMapper& nexthop_port_mapper() { return _nexthop_port_mapper; }

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
     * Get error reporter associated with IfConfig.
     */
    IfConfigErrorReporterBase& ifconfig_error_reporter() {
	return _ifconfig_error_reporter;
    }

    /**
     * Get the IfConfigAddressTable instance.
     *
     * @return a reference to the IfConfigAddressTable instance.
     * @see IfConfigAddressTable.
     */
    IfConfigAddressTable& ifconfig_address_table() {
	return (_ifconfig_address_table);
    }

    IfTree& live_config() { return (_live_config); }
    void    set_live_config(const IfTree& iftree) { _live_config = iftree; }

    const IfTree& pulled_config()	{ return (_pulled_config); }
    IfTree& pushed_config()		{ return (_pushed_config); }
    const IfTree& original_config()	{ return (_original_config); }
    IfTree& local_config()		{ return (_local_config); }
    void set_local_config(const IfTree& iftree) { _local_config = iftree; }
    IfTree& old_local_config()		{ return (_old_local_config); }
    void set_old_local_config(const IfTree& iftree) { _old_local_config = iftree; }

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
     * Return true if the underlying system supports IPv4.
     * 
     * @return true if the underlying system supports IPv4, otherwise false.
     */
    bool have_ipv4() const { return _have_ipv4; }

    /**
     * Return true if the underlying system supports IPv6.
     * 
     * @return true if the underlying system supports IPv6, otherwise false.
     */
    bool have_ipv6() const { return _have_ipv6; }

    /**
     * Test if the underlying system supports IPv4.
     * 
     * @return true if the underlying system supports IPv4, otherwise false.
     */
    bool test_have_ipv4() const;

    /**
     * Test if the underlying system supports IPv6.
     * 
     * @return true if the underlying system supports IPv6, otherwise false.
     */
    bool test_have_ipv6() const;

    /**
     * Push IfTree structure down to platform.  Errors are reported
     * via the constructor supplied ErrorReporter instance.
     *
     * Note that on return some of the interface tree configuration state
     * may be modified.
     *
     * @param config the configuration to be pushed down.
     * @return true on success, otherwise false.
     */
    bool push_config(IfTree& config);

    /**
     * Pull up current config from platform.
     *
     * @return the platform IfTree.
     */
    const IfTree& pull_config();

    void flush_config() { _live_config = IfTree(); }

    /**
     * Get error message associated with push operation.
     */
    const string& push_error() const;

    /**
     * Check IfTreeInterface and report updates to IfConfigUpdateReporter.
     *
     * @return true if there were updates to report, otherwise false.
     */
    bool   report_update(const IfTreeInterface& fi);

    /**
     * Check IfTreeVif and report updates to IfConfigUpdateReporter.
     *
     * @return true if there were updates to report, otherwise false.
     */
    bool   report_update(const IfTreeInterface& fi, const IfTreeVif& fv);

    /**
     * Check IfTreeAddr4 and report updates to IfConfigUpdateReporter.
     *
     * @return true if there were updates to report, otherwise false.
     */
    bool   report_update(const IfTreeInterface&	fi,
			 const IfTreeVif&	fv,
			 const IfTreeAddr4&     fa);

    /**
     * Check IfTreeAddr6 and report updates to IfConfigUpdateReporter.
     *
     * @return true if there were updates to report, otherwise false.
     */
    bool   report_update(const IfTreeInterface&	fi,
			 const IfTreeVif&	fv,
			 const IfTreeAddr6&     fa);

    /**
     * Report that updates were completed to IfConfigUpdateReporter.
     */
    void   report_updates_completed();

    /**
     * Check every item within IfTree and report updates to
     * IfConfigUpdateReporter.
     */
    void   report_updates(IfTree& iftree, bool is_system_interfaces_reportee);

    void	map_ifindex(uint32_t if_index, const string& ifname);
    void	unmap_ifindex(uint32_t if_index);
    const char*	find_ifname(uint32_t if_index) const;
    uint32_t	find_ifindex(const string& ifname) const;

    const char*	get_insert_ifname(uint32_t if_index);
    uint32_t	get_insert_ifindex(const string& ifname);

private:
    FeaNode&			_fea_node;
    EventLoop&			_eventloop;
    NexthopPortMapper&		_nexthop_port_mapper;

    //
    // The interface transaction manager
    //
    IfConfigTransactionManager* _itm;

    //
    // A cache of associative array of interface names to interface index.
    // Needed because the RTM_IFANNOUNCE upcall is called after the interface
    // name is deleted from the kernel.
    //
    typedef map<uint32_t, string> IfIndex2NameMap;
    IfIndex2NameMap	_ifnames;

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
    IfConfigAddressTable	_ifconfig_address_table;

    list<IfConfigGet*>		_ifconfig_gets;
    list<IfConfigSet*>		_ifconfig_sets;
    list<IfConfigObserver*>	_ifconfig_observers;

    //
    // Misc other state
    //
    bool	_have_ipv4;
    bool	_have_ipv6;
    bool	_is_running;
};

#endif // __FEA_IFCONFIG_HH__
