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

// $XORP: xorp/fea/fibconfig.hh,v 1.6 2007/05/01 08:21:55 pavlin Exp $

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

//
// Flag values used to tell underlying FIB message parsing routines
// which messages the caller is interested in.
//
// TODO: Those values and FibMsgSet are temporarily defined here.
// In the future they should be moved inside FibConfig.
//
namespace FibMsg {
    enum {
	UPDATES		= 1 << 0,
	GETS		= 1 << 1,
	RESOLVES	= 1 << 2
    };
};
typedef uint32_t FibMsgSet;

#include "fte.hh"

#include "fibconfig_entry_get.hh"
#include "fibconfig_entry_set.hh"
#include "fibconfig_entry_observer.hh"
#include "fibconfig_table_get.hh"
#include "fibconfig_table_set.hh"
#include "fibconfig_table_observer.hh"
#include "iftree.hh"
#include "fea/data_plane/control_socket/windows_rras_support.hh"

class EventLoop;
class FibConfigEntryGet;
class FibConfigEntrySet;
class FibConfigEntryObserver;
class FibConfigTableGet;
class FibConfigTableSet;
class FibConfigTableObserver;
class FibConfigTransactionManager;
class FibTableObserverBase;
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
     * @param eventloop the event loop.
     * @param profile the profile entity.
     * @param nexthop_port_mapper the next-hop port mapper.
     */
    FibConfig(EventLoop& eventloop, Profile& profile, const IfTree& iftree,
	      NexthopPortMapper& nexthop_port_mapper);

    /**
     * Virtual destructor (in case this class is used as base class).
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

    int register_fibconfig_entry_get_primary(FibConfigEntryGet *fibconfig_entry_get);
    int register_fibconfig_entry_set_primary(FibConfigEntrySet *fibconfig_entry_set);
    int register_fibconfig_entry_observer_primary(FibConfigEntryObserver *fibconfig_entry_observer);
    int register_fibconfig_table_get_primary(FibConfigTableGet *fibconfig_table_get);
    int register_fibconfig_table_set_primary(FibConfigTableSet *fibconfig_table_set);
    int register_fibconfig_table_observer_primary(FibConfigTableObserver *fibconfig_table_observer);
    int register_fibconfig_entry_get_secondary(FibConfigEntryGet *fibconfig_entry_get);
    int register_fibconfig_entry_set_secondary(FibConfigEntrySet *fibconfig_entry_set);
    int register_fibconfig_entry_observer_secondary(FibConfigEntryObserver *fibconfig_entry_observer);
    int register_fibconfig_table_get_secondary(FibConfigTableGet *fibconfig_table_get);
    int register_fibconfig_table_set_secondary(FibConfigTableSet *fibconfig_table_set);
    int register_fibconfig_table_observer_secondary(FibConfigTableObserver *fibconfig_table_observer);
    
    FibConfigEntryGet&		fibconfig_entry_get_primary() { return *_fibconfig_entry_get_primary; }
    FibConfigEntrySet&		fibconfig_entry_set_primary() { return *_fibconfig_entry_set_primary; }
    FibConfigEntryObserver&	fibconfig_entry_observer_primary() { return *_fibconfig_entry_observer_primary; }
    FibConfigTableGet&		fibconfig_table_get_primary() { return *_fibconfig_table_get_primary; }
    FibConfigTableSet&		fibconfig_table_set_primary() { return *_fibconfig_table_set_primary; }
    FibConfigTableObserver&	fibconfig_table_observer_primary() { return *_fibconfig_table_observer_primary; }

    FibConfigEntrySetClick&	fibconfig_entry_set_click() { return _fibconfig_entry_set_click; }

    /**
     * Setup the unit to behave as dummy (for testing purpose).
     * 
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_dummy();

    /**
     * Test if running in dummy mode.
     * 
     * @return true if running in dummy mode, otherwise false.
     */
    bool is_dummy() const { return _is_dummy; }
    
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
     * Enable/disable Click support.
     *
     * @param enable if true, then enable Click support, otherwise disable it.
     */
    void enable_click(bool enable);

    /**
     * Start Click support.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int start_click(string& error_msg);

    /**
     * Stop Click support.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int stop_click(string& error_msg);

    /**
     * Enable/disable duplicating the Click routes to the system kernel.
     *
     * @param enable if true, then enable duplicating the Click routes to the
     * system kernel, otherwise disable it.
     */
    void enable_duplicate_routes_to_kernel(bool enable);

    /**
     * Enable/disable kernel-level Click support.
     *
     * @param enable if true, then enable the kernel-level Click support,
     * otherwise disable it.
     */
    void enable_kernel_click(bool enable);

    /**
     * Enable/disable installing kernel-level Click on startup.
     *
     * @param enable if true, then install kernel-level Click on startup.
     */
    void enable_kernel_click_install_on_startup(bool enable);

    /**
     * Specify the list of kernel Click modules to load on startup if
     * installing kernel-level Click on startup is enabled.
     *
     * @param modules the list of kernel Click modules to load.
     */
    void set_kernel_click_modules(const list<string>& modules);

    /**
     * Specify the kernel-level Click mount directory.
     *
     * @param directory the kernel-level Click mount directory.
     */
    void set_kernel_click_mount_directory(const string& directory);

    /**
     * Specify the external program to generate the kernel-level Click
     * configuration.
     *
     * @param v the name of the external program to generate the kernel-level
     * Click configuration.
     */
    void set_kernel_click_config_generator_file(const string& v);

    /**
     * Enable/disable user-level Click support.
     *
     * @param enable if true, then enable the user-level Click support,
     * otherwise disable it.
     */
    void enable_user_click(bool enable);

    /**
     * Specify the user-level Click command file.
     *
     * @param v the name of the user-level Click command file.
     */
    void set_user_click_command_file(const string& v);

    /**
     * Specify the extra arguments to the user-level Click command.
     *
     * @param v the extra arguments to the user-level Click command.
     */
    void set_user_click_command_extra_arguments(const string& v);

    /**
     * Specify whether to execute on startup the user-level Click command.
     *
     * @param v if true, then execute the user-level Click command on startup.
     */
    void set_user_click_command_execute_on_startup(bool v);

    /**
     * Specify the address to use for control access to the user-level
     * Click.
     *
     * @param v the address to use for control access to the user-level Click.
     */
    void set_user_click_control_address(const IPv4& v);

    /**
     * Specify the socket port to use for control access to the user-level
     * Click.
     *
     * @param v the socket port to use for control access to the user-level
     * Click.
     */
    void set_user_click_control_socket_port(uint32_t v);

    /**
     * Specify the configuration file to be used by user-level Click on
     * startup.
     *
     * @param v the name of the configuration file to be used by user-level
     * Click on startup.
     */
    void set_user_click_startup_config_file(const string& v);

    /**
     * Specify the external program to generate the user-level Click
     * configuration.
     *
     * @param v the name of the external program to generate the user-level
     * Click configuration.
     */
    void set_user_click_config_generator_file(const string& v);

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
    EventLoop&				_eventloop;
    Profile&				_profile;
    NexthopPortMapper&			_nexthop_port_mapper;
    const IfTree&			_iftree;

    //
    // The FIB transaction manager
    //
    FibConfigTransactionManager*	_ftm;

    FibConfigEntryGet*			_fibconfig_entry_get_primary;
    FibConfigEntrySet*			_fibconfig_entry_set_primary;
    FibConfigEntryObserver*		_fibconfig_entry_observer_primary;
    FibConfigTableGet*			_fibconfig_table_get_primary;
    FibConfigTableSet*			_fibconfig_table_set_primary;
    FibConfigTableObserver*		_fibconfig_table_observer_primary;

    list<FibConfigEntryGet*>		_fibconfig_entry_gets_secondary;
    list<FibConfigEntrySet*>		_fibconfig_entry_sets_secondary;
    list<FibConfigEntryObserver*>	_fibconfig_entry_observers_secondary;
    list<FibConfigTableGet*>		_fibconfig_table_gets_secondary;
    list<FibConfigTableSet*>		_fibconfig_table_sets_secondary;
    list<FibConfigTableObserver*>	_fibconfig_table_observers_secondary;
    
    //
    // The primary mechanisms to get single-entry information
    // from the unicast forwarding table.
    //
    // XXX: Ordering is important: the last that is supported
    // is the one to use.
    //
    FibConfigEntryGetDummy	_fibconfig_entry_get_dummy;
    FibConfigEntryGetRtsock	_fibconfig_entry_get_rtsock;
    FibConfigEntryGetNetlink	_fibconfig_entry_get_netlink;
    FibConfigEntryGetIPHelper	_fibconfig_entry_get_iphelper;
    //FibConfigEntryGetRtmV2	_fibconfig_entry_get_rtmv2;

    //
    // The secondary mechanisms to get single-entry information
    // from the unicast forwarding table.
    //
    // XXX: Ordering is not important.
    //
    FibConfigEntryGetClick	_fibconfig_entry_get_click;
    
    //
    // The primary mechanisms to set single-entry information
    // into the unicast forwarding table.
    //
    // XXX: Ordering is important: the last that is supported
    // is the one to use.
    //
    FibConfigEntrySetDummy	_fibconfig_entry_set_dummy;
    FibConfigEntrySetRtsock	_fibconfig_entry_set_rtsock;
    FibConfigEntrySetNetlink	_fibconfig_entry_set_netlink;
    FibConfigEntrySetIPHelper	_fibconfig_entry_set_iphelper;
    FibConfigEntrySetRtmV2	_fibconfig_entry_set_rtmv2;

    //
    // The secondary mechanisms to set single-entry information
    // into the unicast forwarding table.
    //
    // XXX: Ordering is not important.
    //
    FibConfigEntrySetClick	_fibconfig_entry_set_click;
    
    //
    // The primary mechanisms to observe single-entry information change
    // about the unicast forwarding table.
    // E.g., if the forwarding table has changed, then the information
    // received by the observer would specify the particular entry that
    // has changed.
    //
    // XXX: Ordering is important: the last that is supported
    // is the one to use.
    //
    FibConfigEntryObserverDummy		_fibconfig_entry_observer_dummy;
    FibConfigEntryObserverRtsock	_fibconfig_entry_observer_rtsock;
    FibConfigEntryObserverNetlink	_fibconfig_entry_observer_netlink;
    FibConfigEntryObserverIPHelper	_fibconfig_entry_observer_iphelper;
    //FibConfigEntryObserverRtmV2	_fibconfig_entry_observer_rtmv2;

    //
    // The primary mechanisms to get the whole table information
    // from the unicast forwarding table.
    //
    // XXX: Ordering is important: the last that is supported
    // is the one to use.
    //
    FibConfigTableGetDummy	_fibconfig_table_get_dummy;
    FibConfigTableGetSysctl	_fibconfig_table_get_sysctl;
    FibConfigTableGetNetlink	_fibconfig_table_get_netlink;
    FibConfigTableGetIPHelper	_fibconfig_table_get_iphelper;
    
    //
    // The secondary mechanisms to get the whole table information
    // from the unicast forwarding table.
    //
    // XXX: Ordering is not important.
    //
    FibConfigTableGetClick	_fibconfig_table_get_click;

    //
    // The primary mechanisms to set the whole table information
    // into the unicast forwarding table.
    //
    // XXX: Ordering is important: the last that is supported
    // is the one to use.
    //
    FibConfigTableSetDummy	_fibconfig_table_set_dummy;
    FibConfigTableSetRtsock	_fibconfig_table_set_rtsock;
    FibConfigTableSetNetlink	_fibconfig_table_set_netlink;
    FibConfigTableSetIPHelper	_fibconfig_table_set_iphelper;
    //FibConfigTableSetRtmV2	_fibconfig_table_set_rtmv2;

    //
    // The secondary mechanisms to set the whole table information
    // into the unicast forwarding table.
    //
    // XXX: Ordering is not important.
    //
    FibConfigTableSetClick	_fibconfig_table_set_click;
    
    //
    // The primary mechanisms to observe the whole-table information change
    // about the unicast forwarding table.
    // E.g., if the forwarding table has changed, then the information
    // received by the observer would NOT specify the particular entry that
    // has changed.
    //
    // XXX: Ordering is important: the last that is supported
    // is the one to use.
    //
    FibConfigTableObserverDummy		_fibconfig_table_observer_dummy;
    FibConfigTableObserverRtsock	_fibconfig_table_observer_rtsock;
    FibConfigTableObserverNetlink	_fibconfig_table_observer_netlink;
    FibConfigTableObserverIPHelper	_fibconfig_table_observer_iphelper;
    FibConfigTableObserverRtmV2		_fibconfig_table_observer_rtmv2;
    
    //
    // Original state from the underlying system before the FEA was started
    //
    bool	_unicast_forwarding_enabled4;
    bool	_unicast_forwarding_enabled6;
    bool	_accept_rtadv_enabled6;

    //
    // Unicast forwarding entries properties
    //
    bool	_unicast_forwarding_entries_retain_on_startup4;
    bool	_unicast_forwarding_entries_retain_on_shutdown4;
    bool	_unicast_forwarding_entries_retain_on_startup6;
    bool	_unicast_forwarding_entries_retain_on_shutdown6;

    //
    // Misc other state
    //
    bool	_have_ipv4;
    bool	_have_ipv6;
    bool	_is_dummy;
    bool	_is_running;
    list<FibTableObserverBase* > _fib_table_observers;

#ifdef HOST_OS_WINDOWS
    //
    // State for Windows EnableRouter() API function.
    //
    HANDLE	_event;
    OVERLAPPED  _overlapped;
    int		_enablecnt;
#endif
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
