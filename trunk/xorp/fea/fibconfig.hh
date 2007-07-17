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

// $XORP: xorp/fea/fibconfig.hh,v 1.9 2007/07/11 22:18:02 pavlin Exp $

#ifndef	__FEA_FIBCONFIG_HH__
#define __FEA_FIBCONFIG_HH__

#include "libxorp/xorp.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/status_codes.h"
#include "libxorp/transaction.hh"
#include "libxorp/trie.hh"

#include "fte.hh"
#include "fibconfig_forwarding.hh"
#include "fibconfig_entry_get.hh"
#include "fibconfig_entry_set.hh"
#include "fibconfig_entry_observer.hh"
#include "fibconfig_table_get.hh"
#include "fibconfig_table_set.hh"
#include "fibconfig_table_observer.hh"

class EventLoop;
class FeaNode;
class FibConfigTransactionManager;
class FibTableObserverBase;
class IfTree;
class NexthopPortMapper;
class Profile;

typedef Trie<IPv4, Fte4> Trie4;
typedef Trie<IPv6, Fte6> Trie6;

/**
 * @short Forwarding Table Interface.
 *
 * Abstract class.
 */
class FibConfig {
public:

    /**
     * Constructor.
     * 
     * @param fea_node the FEA node.
     * @param iftree the interface tree to use.
     */
    FibConfig(FeaNode& fea_node, const IfTree& iftree);

    /**
     * Virtual destructor (in case this class is used as a base class).
     */
    virtual ~FibConfig();

    /**
     * Get a reference to the @ref EventLoop instance.
     *
     * @return a reference to the @ref EventLoop instance.
     */
    EventLoop& eventloop() { return _eventloop; }

    /**
     * Get a reference to the @ref NexthopPortMapper instance.
     *
     * @return a reference to the @ref NexthopPortMapper instance.
     */
    NexthopPortMapper& nexthop_port_mapper() { return _nexthop_port_mapper; }

    /**
     * Get a reference to the @ref IfTree instance.
     *
     * @return a reference to the @ref IfTree instance.
     */
    const IfTree& iftree() const { return _iftree; }

    /**
     * Get the status code
     *
     * @param reason the human-readable reason for any failure.
     * @return the status code.
     */
    ProcessStatus status(string& reason) const;

    /**
     * Start FIB-related transaction.
     *
     * @param tid the return-by-reference new transaction ID.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int start_transaction(uint32_t& tid, string& error_msg);

    /**
     * Commit FIB-related transaction.
     *
     * @param tid the transaction ID.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int commit_transaction(uint32_t tid, string& error_msg);

    /**
     * Abort FIB-related transaction.
     *
     * @param tid the transaction ID.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int abort_transaction(uint32_t tid, string& error_msg);

    /**
     * Add operation to FIB-related transaction.
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
     * Register @ref FibConfigForwarding plugin.
     *
     * @param fibconfig_forwarding the plugin to register.
     * @param is_exclusive if true, the plugin is registered as the
     * exclusive plugin, otherwise is added to the list of plugins.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_fibconfig_forwarding(FibConfigForwarding *fibconfig_forwarding,
				      bool is_exclusive);

    /**
     * Unregister @ref FibConfigForwarding plugin.
     *
     * @param fibconfig_forwarding the plugin to unregister.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_fibconfig_forwarding(FibConfigForwarding* fibconfig_forwarding);

    /**
     * Register @ref FibConfigEntryGet plugin.
     *
     * @param fibconfig_entry_get the plugin to register.
     * @param is_exclusive if true, the plugin is registered as the
     * exclusive plugin, otherwise is added to the list of plugins.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_fibconfig_entry_get(FibConfigEntryGet *fibconfig_entry_get,
				     bool is_exclusive);

    /**
     * Unregister @ref FibConfigEntryGet plugin.
     *
     * @param fibconfig_entry_get the plugin to unregister.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_fibconfig_entry_get(FibConfigEntryGet* fibconfig_entry_get);

    /**
     * Register @ref FibConfigEntrySet plugin.
     *
     * @param fibconfig_entry_set the plugin to register.
     * @param is_exclusive if true, the plugin is registered as the
     * exclusive plugin, otherwise is added to the list of plugins.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_fibconfig_entry_set(FibConfigEntrySet *fibconfig_entry_set,
				     bool is_exclusive);

    /**
     * Unregister @ref FibConfigEntrySet plugin.
     *
     * @param fibconfig_entry_set the plugin to unregister.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_fibconfig_entry_set(FibConfigEntrySet* fibconfig_entry_set);

    /**
     * Register @ref FibConfigEntryObserver plugin.
     *
     * @param fibconfig_entry_observer the plugin to register.
     * @param is_exclusive if true, the plugin is registered as the
     * exclusive plugin, otherwise is added to the list of plugins.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_fibconfig_entry_observer(FibConfigEntryObserver *fibconfig_entry_observer,
					  bool is_exclusive);

    /**
     * Unregister @ref FibConfigEntryObserver plugin.
     *
     * @param fibconfig_entry_observer the plugin to unregister.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_fibconfig_entry_observer(FibConfigEntryObserver* fibconfig_entry_observer);

    /**
     * Register @ref FibConfigTableGet plugin.
     *
     * @param fibconfig_table_get the plugin to register.
     * @param is_exclusive if true, the plugin is registered as the
     * exclusive plugin, otherwise is added to the list of plugins.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_fibconfig_table_get(FibConfigTableGet *fibconfig_table_get,
				     bool is_exclusive);

    /**
     * Unregister @ref FibConfigTableGet plugin.
     *
     * @param fibconfig_table_get the plugin to unregister.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_fibconfig_table_get(FibConfigTableGet* fibconfig_table_get);

    /**
     * Register @ref FibConfigTableSet plugin.
     *
     * @param fibconfig_table_set the plugin to register.
     * @param is_exclusive if true, the plugin is registered as the
     * exclusive plugin, otherwise is added to the list of plugins.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_fibconfig_table_set(FibConfigTableSet *fibconfig_table_set,
				     bool is_exclusive);

    /**
     * Unregister @ref FibConfigTableSet plugin.
     *
     * @param fibconfig_table_set the plugin to unregister.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_fibconfig_table_set(FibConfigTableSet* fibconfig_table_set);

    /**
     * Register @ref FibConfigTableObserver plugin.
     *
     * @param fibconfig_table_observer the plugin to register.
     * @param is_exclusive if true, the plugin is registered as the
     * exclusive plugin, otherwise is added to the list of plugins.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_fibconfig_table_observer(FibConfigTableObserver *fibconfig_table_observer,
					  bool is_exclusive);

    /**
     * Unregister @ref FibConfigTableObserver plugin.
     *
     * @param fibconfig_table_observer the plugin to unregister.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unregister_fibconfig_table_observer(FibConfigTableObserver* fibconfig_table_observer);
    
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
     * Start a configuration interval. All modifications must be
     * within a marked "configuration" interval.
     *
     * @param error_msg the error message (if error).
     * @return true if configuration successfully started.
     */
    bool start_configuration(string& error_msg);
    
    /**
     * End of configuration interval.
     *
     * @param error_msg the error message (if error).
     * @return true configuration success pushed down into forwarding table.
     */
    bool end_configuration(string& error_msg);

    /**
     * Return true if the underlying system supports IPv4.
     * 
     * @return true if the underlying system supports IPv4, otherwise false.
     */
    bool have_ipv4() const;

    /**
     * Return true if the underlying system supports IPv6.
     * 
     * @return true if the underlying system supports IPv6, otherwise false.
     */
    bool have_ipv6() const;

    /**
     * Test whether the IPv4 unicast forwarding engine retains existing
     * XORP forwarding entries on startup.
     *
     * @return true if the XORP unicast forwarding entries are retained,
     * otherwise false.
     */
    bool unicast_forwarding_entries_retain_on_startup4() const {
	return (_unicast_forwarding_entries_retain_on_startup4);
    }

    /**
     * Test whether the IPv4 unicast forwarding engine retains existing
     * XORP forwarding entries on shutdown.
     *
     * @return true if the XORP unicast forwarding entries are retained,
     * otherwise false.
     */
    bool unicast_forwarding_entries_retain_on_shutdown4() const {
	return (_unicast_forwarding_entries_retain_on_shutdown4);
    }

    /**
     * Test whether the IPv6 unicast forwarding engine retains existing
     * XORP forwarding entries on startup.
     *
     * @return true if the XORP unicast forwarding entries are retained,
     * otherwise false.
     */
    bool unicast_forwarding_entries_retain_on_startup6() const {
	return (_unicast_forwarding_entries_retain_on_startup6);
    }

    /**
     * Test whether the IPv6 unicast forwarding engine retains existing
     * XORP forwarding entries on shutdown.
     *
     * @return true if the XORP unicast forwarding entries are retained,
     * otherwise false.
     */
    bool unicast_forwarding_entries_retain_on_shutdown6() const {
	return (_unicast_forwarding_entries_retain_on_shutdown6);
    }

    /**
     * Set the IPv4 unicast forwarding engine whether to retain existing
     * XORP forwarding entries on startup.
     *
     * @param retain if true, then retain the XORP forwarding entries,
     * otherwise delete them.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_unicast_forwarding_entries_retain_on_startup4(bool retain,
							  string& error_msg);

    /**
     * Set the IPv4 unicast forwarding engine whether to retain existing
     * XORP forwarding entries on shutdown.
     *
     * @param retain if true, then retain the XORP forwarding entries,
     * otherwise delete them.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_unicast_forwarding_entries_retain_on_shutdown4(bool retain,
							   string& error_msg);

    /**
     * Set the IPv6 unicast forwarding engine whether to retain existing
     * XORP forwarding entries on startup.
     *
     * @param retain if true, then retain the XORP forwarding entries,
     * otherwise delete them.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_unicast_forwarding_entries_retain_on_startup6(bool retain,
							  string& error_msg);

    /**
     * Set the IPv6 unicast forwarding engine whether to retain existing
     * XORP forwarding entries on shutdown.
     *
     * @param retain if true, then retain the XORP forwarding entries,
     * otherwise delete them.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_unicast_forwarding_entries_retain_on_shutdown6(bool retain,
							   string& error_msg);

    /**
     * Test whether the IPv4 unicast forwarding engine is enabled or disabled
     * to forward packets.
     * 
     * @param ret_value if true on return, then the IPv4 unicast forwarding
     * is enabled, otherwise is disabled.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unicast_forwarding_enabled4(bool& ret_value, string& error_msg) const;

    /**
     * Test whether the IPv6 unicast forwarding engine is enabled or disabled
     * to forward packets.
     * 
     * @param ret_value if true on return, then the IPv6 unicast forwarding
     * is enabled, otherwise is disabled.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int unicast_forwarding_enabled6(bool& ret_value, string& error_msg) const;
    
    /**
     * Test whether the acceptance of IPv6 Router Advertisement messages is
     * enabled or disabled.
     * 
     * @param ret_value if true on return, then the acceptance of IPv6 Router
     * Advertisement messages is enabled, otherwise is disabled.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int accept_rtadv_enabled6(bool& ret_value, string& error_msg) const;
    
    /**
     * Set the IPv4 unicast forwarding engine to enable or disable forwarding
     * of packets.
     * 
     * @param v if true, then enable IPv4 unicast forwarding, otherwise
     * disable it.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_unicast_forwarding_enabled4(bool v, string& error_msg);

    /**
     * Set the IPv6 unicast forwarding engine to enable or disable forwarding
     * of packets.
     * 
     * @param v if true, then enable IPv6 unicast forwarding, otherwise
     * disable it.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_unicast_forwarding_enabled6(bool v, string& error_msg);
    
    /**
     * Enable or disable the acceptance of IPv6 Router Advertisement messages
     * from other routers. It should be enabled for hosts, and disabled for
     * routers.
     * 
     * @param v if true, then enable the acceptance of IPv6 Router
     * Advertisement messages, otherwise disable it.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_accept_rtadv_enabled6(bool v, string& error_msg);

    /**
     * Add a single routing entry. Must be within a configuration interval.
     *
     * @param fte the entry to add.
     *
     * @return true on success, otherwise false.
     */
    virtual bool add_entry4(const Fte4& fte);

    /**
     * Delete a single routing entry. Must be with a configuration interval.
     *
     * @param fte the entry to delete. Only destination and netmask are used.
     *
     * @return true on success, otherwise false.
     */
    virtual bool delete_entry4(const Fte4& fte);

    /**
     * Set the unicast forwarding table.
     *
     * @param fte_list the list with all entries to install into
     * the unicast forwarding table.
     *
     * @return true on success, otherwise false.
     */
    virtual bool set_table4(const list<Fte4>& fte_list);

    /**
     * Delete all entries in the routing table. Must be within a
     * configuration interval.
     *
     * @return true on success, otherwise false.
     */
    virtual bool delete_all_entries4();

    /**
     * Lookup a route by destination address.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_route_by_dest4(const IPv4& dst, Fte4& fte);

    /**
     * Lookup route by network address.
     *
     * @param dst network address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_route_by_network4(const IPv4Net& dst, Fte4& fte);

    /**
     * Obtain the unicast forwarding table.
     *
     * @param fte_list the return-by-reference list with all entries in
     * the unicast forwarding table.
     *
     * @return true on success, otherwise false.
     */
    virtual bool get_table4(list<Fte4>& fte_list);

    /**
     * Add a single routing entry. Must be within a configuration interval.
     *
     * @param fte the entry to add.
     *
     * @return true on success, otherwise false.
     */
    virtual bool add_entry6(const Fte6& fte);

    /**
     * Set the unicast forwarding table.
     *
     * @param fte_list the list with all entries to install into
     * the unicast forwarding table.
     *
     * @return true on success, otherwise false.
     */
    virtual bool set_table6(const list<Fte6>& fte_list);

    /**
     * Delete a single routing entry. Must be within a configuration interval.
     *
     * @param fte the entry to delete. Only destination and netmask are used.
     *
     * @return true on success, otherwise false.
     */
    virtual bool delete_entry6(const Fte6& fte);

    /**
     * Delete all entries in the routing table. Must be within a
     * configuration interval.
     *
     * @return true on success, otherwise false.
     */
    virtual bool delete_all_entries6();

    /**
     * Lookup a route by destination address.
     *
     * @param dst host address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_route_by_dest6(const IPv6& dst, Fte6& fte);

    /**
     * Lookup route by network address.
     *
     * @param dst network address to resolve.
     * @param fte return-by-reference forwarding table entry.
     *
     * @return true on success, otherwise false.
     */
    virtual bool lookup_route_by_network6(const IPv6Net& dst, Fte6& fte);

    /**
     * Obtain the unicast forwarding table.
     *
     * @param fte_list the return-by-reference list with all entries in
     * the unicast forwarding table.
     *
     * @return true on success, otherwise false.
     */
    virtual bool get_table6(list<Fte6>& fte_list);

    /**
     * Add a FIB table observer.
     * 
     * @param fib_table_observer the FIB table observer to add.
     * @return true on success, otherwise false.
     */
    bool add_fib_table_observer(FibTableObserverBase* fib_table_observer);
    
    /**
     * Delete a FIB table observer.
     * 
     * @param fib_table_observer the FIB table observer to delete.
     * @return true on success, otherwise false.
     */
    bool delete_fib_table_observer(FibTableObserverBase* fib_table_observer);

    /**
     * Propagate FIB changes to all FIB table observers.
     * 
     * @param fte_list the list with the FIB changes.
     * @param fibconfig_table_observer the method that reports the FIB changes.
     */
    void propagate_fib_changes(const list<FteX>& fte_list,
			       const FibConfigTableObserver* fibconfig_table_observer);

    /**
     * Get the IPv4 Trie (used for testing purpose).
     * 
     * @return the IPv4 Trie.
     */
    Trie4& trie4() { return _trie4; }

    /**
     * Get the IPv6 Trie (used for testing purpose).
     * 
     * @return the IPv6 Trie.
     */
    Trie6& trie6() { return _trie6; }

protected:
    Trie4	_trie4;		// IPv4 trie (used for testing purpose)
    Trie6	_trie6;		// IPv6 trie (used for testing purpose)
    
private:
    FeaNode&				_fea_node;
    EventLoop&				_eventloop;
    Profile&				_profile;
    NexthopPortMapper&			_nexthop_port_mapper;
    const IfTree&			_iftree;

    //
    // The FIB transaction manager
    //
    FibConfigTransactionManager*	_ftm;

    list<FibConfigForwarding*>		_fibconfig_forwarding_plugins;
    list<FibConfigEntryGet*>		_fibconfig_entry_gets;
    list<FibConfigEntrySet*>		_fibconfig_entry_sets;
    list<FibConfigEntryObserver*>	_fibconfig_entry_observers;
    list<FibConfigTableGet*>		_fibconfig_table_gets;
    list<FibConfigTableSet*>		_fibconfig_table_sets;
    list<FibConfigTableObserver*>	_fibconfig_table_observers;
    
    //
    // Configured unicast forwarding entries properties
    //
    bool	_unicast_forwarding_entries_retain_on_startup4;
    bool	_unicast_forwarding_entries_retain_on_shutdown4;
    bool	_unicast_forwarding_entries_retain_on_startup6;
    bool	_unicast_forwarding_entries_retain_on_shutdown6;

    //
    // Misc other state
    //
    bool	_is_running;
    list<FibTableObserverBase* > _fib_table_observers;
};

/**
 * A base class that can be used by clients interested in observing
 * changes in the Forwarding Information Base.
 */
class FibTableObserverBase {
public:
    FibTableObserverBase() {}
    virtual ~FibTableObserverBase() {}

    /**
     * Process a list of IPv4 FIB route changes.
     * 
     * The FIB route changes come from the underlying system.
     * 
     * @param fte_list the list of Fte entries to add or delete.
     */
    virtual void process_fib_changes(const list<Fte4>& fte_list) = 0;

    /**
     * Process a list of IPv6 FIB route changes.
     * 
     * The FIB route changes come from the underlying system.
     * 
     * @param fte_list the list of Fte entries to add or delete.
     */
    virtual void process_fib_changes(const list<Fte6>& fte_list) = 0;

private:
};

#endif	// __FEA_FIBCONFIG_HH__
