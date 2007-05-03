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

// $XORP: xorp/fea/ifconfig.hh,v 1.57 2007/04/27 21:24:38 pavlin Exp $

#ifndef __FEA_IFCONFIG_HH__
#define __FEA_IFCONFIG_HH__

#include "libxorp/status_codes.h"
#include "libxorp/transaction.hh"

#include "ifconfig_get.hh"
#include "ifconfig_set.hh"
#include "ifconfig_observer.hh"
#include "ifconfig_reporter.hh"
#include "iftree.hh"

class EventLoop;
class IfConfigGet;
class IfConfigErrorReporterBase;
class IfConfigObserver;
class IfConfigSet;
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
     * @param eventloop the event loop.
     * @param ur update reporter that receives updates through when
     *           configurations are pushed down and when triggered
     *		 spontaneously on the underlying platform.
     * @param er error reporter that errors are propagated through when
     *           configurations are pushed down.
     * @param nexthop_port_mapper the next-hop port mapper.
     */
    IfConfig(EventLoop& eventloop, IfConfigUpdateReporterBase& ur,
	     IfConfigErrorReporterBase& er,
	     NexthopPortMapper& nexthop_port_mapper);

    /**
     * Virtual destructor (in case this class is used as base class).
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
     * Get error reporter associated with IfConfig.
     */
    inline IfConfigErrorReporterBase&	er() { return _er; }

    IfTree& live_config() { return (_live_config); }
    void    set_live_config(const IfTree& it) { _live_config = it; }

    const IfTree& pulled_config()	{ return (_pulled_config); }
    IfTree& pushed_config()		{ return (_pushed_config); }
    const IfTree& original_config()	{ return (_original_config); }
    IfTree& local_config()		{ return (_local_config); }
    void set_local_config(const IfTree& it) { _local_config = it; }
    IfTree& old_local_config()		{ return (_old_local_config); }
    void set_old_local_config(const IfTree& it) { _old_local_config = it; }

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

    int register_ifconfig_get_primary(IfConfigGet *ifconfig_get);
    int register_ifconfig_set_primary(IfConfigSet *ifconfig_set);
    int register_ifconfig_observer_primary(IfConfigObserver *ifconfig_observer);
    int register_ifconfig_get_secondary(IfConfigGet *ifconfig_get);
    int register_ifconfig_set_secondary(IfConfigSet *ifconfig_set);
    int register_ifconfig_observer_secondary(IfConfigObserver *ifconfig_observer);

    IfConfigGet&	ifconfig_get_primary() { return *_ifconfig_get_primary; }
    IfConfigSet&	ifconfig_set_primary() { return *_ifconfig_set_primary; }
    IfConfigObserver&	ifconfig_observer_primary() { return *_ifconfig_observer_primary; }

    IfConfigGet&	ifconfig_get_ioctl() { return _ifconfig_get_ioctl; }
    IfConfigSetClick&	ifconfig_set_click() { return _ifconfig_set_click; }

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
     * Specify the external program to generate the kernel-level Click
     * configuration.
     *
     * @param v the name of the external program to generate the kernel-level
     * Click configuration.
     */
    void set_kernel_click_config_generator_file(const string& v);

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
    void   report_updates(IfTree& it, bool is_system_interfaces_reportee);

    void	map_ifindex(uint32_t if_index, const string& ifname);
    void	unmap_ifindex(uint32_t if_index);
    const char*	find_ifname(uint32_t if_index) const;
    uint32_t	find_ifindex(const string& ifname) const;

    const char*	get_insert_ifname(uint32_t if_index);
    uint32_t	get_insert_ifindex(const string& ifname);

private:

    EventLoop&			_eventloop;
    IfConfigUpdateReporterBase&	_ur;
    IfConfigErrorReporterBase&	_er;
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

    IfConfigGet*		_ifconfig_get_primary;
    IfConfigSet*		_ifconfig_set_primary;
    IfConfigObserver*		_ifconfig_observer_primary;
    list<IfConfigGet*>		_ifconfig_gets_secondary;
    list<IfConfigSet*>		_ifconfig_sets_secondary;
    list<IfConfigObserver*>	_ifconfig_observers_secondary;

    //
    // The primary mechanisms to get interface-related information
    // from the underlying system.
    //
    // XXX: Ordering is important: the last that is supported
    // is the one to use.
    //
    IfConfigGetDummy		_ifconfig_get_dummy;
    IfConfigGetIoctl		_ifconfig_get_ioctl;
    IfConfigGetSysctl		_ifconfig_get_sysctl;
    IfConfigGetGetifaddrs	_ifconfig_get_getifaddrs;
    IfConfigGetProcLinux	_ifconfig_get_proc_linux;
    IfConfigGetNetlinkSocket	_ifconfig_get_netlink_socket;
    IfConfigGetIPHelper		_ifconfig_get_iphelper;

    //
    // The secondary mechanisms to get interface-related information
    // from the underlying system.
    //
    // XXX: Ordering is not important.
    //
    IfConfigGetClick		_ifconfig_get_click;

    //
    // The primary mechanisms to set interface-related information
    // within the underlying system.
    //
    // XXX: Ordering is important: the last that is supported
    // is the one to use.
    //
    IfConfigSetDummy		_ifconfig_set_dummy;
    IfConfigSetIoctl		_ifconfig_set_ioctl;
    IfConfigSetNetlinkSocket	_ifconfig_set_netlink_socket;
    IfConfigSetIPHelper		_ifconfig_set_iphelper;

    //
    // The secondary mechanisms to get interface-related information
    // from the underlying system.
    //
    // XXX: Ordering is not important.
    //
    IfConfigSetClick		_ifconfig_set_click;

    //
    // The primary mechanisms to observe whether the interface-related
    // information within the underlying system has changed.
    //
    // XXX: Ordering is important: the last that is supported
    // is the one to use.
    //
    IfConfigObserverDummy		_ifconfig_observer_dummy;
    IfConfigObserverRoutingSocket	_ifconfig_observer_routing_socket;
    IfConfigObserverNetlinkSocket	_ifconfig_observer_netlink_socket;
    IfConfigObserverIPHelper		_ifconfig_observer_iphelper;

    //
    // Misc other state
    //
    bool	_have_ipv4;
    bool	_have_ipv6;
    bool	_is_dummy;
    bool	_is_running;
};

#endif // __FEA_IFCONFIG_HH__
