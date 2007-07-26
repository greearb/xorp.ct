// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007 International Computer Science Institute
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

// $XORP: xorp/fea/fea_data_plane_manager.hh,v 1.3 2007/07/18 01:30:22 pavlin Exp $

#ifndef __FEA_FEA_DATA_PLANE_MANAGER_HH__
#define __FEA_FEA_DATA_PLANE_MANAGER_HH__

#include <list>

class EventLoop;
class FeaNode;
class FibConfig;
class FibConfigEntryGet;
class FibConfigEntryObserver;
class FibConfigEntrySet;
class FibConfigForwarding;
class FibConfigTableGet;
class FibConfigTableObserver;
class FibConfigTableSet;
class IfConfig;
class IfConfigGet;
class IfConfigObserver;
class IfConfigSet;
class IfTree;
class IoLink;
class IoLinkManager;
class IoIp;
class IoIpManager;


/**
 * FEA data plane manager base class.
 */
class FeaDataPlaneManager {
public:
    /**
     * Constructor.
     *
     * @param fea_node the @ref FeaNode this manager belongs to.
     * @param manager_name the data plane manager name.
     */
    FeaDataPlaneManager(FeaNode& fea_node, const string& manager_name);

    /**
     * Virtual destructor.
     */
    virtual ~FeaDataPlaneManager();

    /**
     * Get the data plane manager name.
     *
     * @return the data plane name.
     */
    const string& manager_name() const { return _manager_name; }

    /**
     * Start data plane manager operation.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start_manager(string& error_msg);

    /**
     * Stop data plane manager operation.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop_manager(string& error_msg);

    /**
     * Load the plugins.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int load_plugins(string& error_msg) = 0;

    /**
     * Unload the plugins.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int unload_plugins(string& error_msg);

    /**
     * Register the plugins.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int register_plugins(string& error_msg) = 0;

    /**
     * Unregister the plugins.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int unregister_plugins(string& error_msg);

    /**
     * Start plugins operation.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int start_plugins(string& error_msg);

    /**
     * Stop plugins operation.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    virtual int stop_plugins(string& error_msg);

    /**
     * Get the event loop this instance is added to.
     * 
     * @return the event loop this instance is added to.
     */
    EventLoop&	eventloop();

    /**
     * Return true if the underlying system supports IPv4.
     * 
     * @return true if the underlying system supports IPv4, otherwise false.
     */
    virtual bool have_ipv4() const { return (_have_ipv4); }

    /**
     * Return true if the underlying system supports IPv6.
     * 
     * @return true if the underlying system supports IPv6, otherwise false.
     */
    virtual bool have_ipv6() const { return (_have_ipv6); }

    /**
     * Get the @ref IfConfig instance.
     *
     * @return the @ref IfConfig instance.
     */
    IfConfig& ifconfig();

    /**
     * Get the @ref FibConfig instance.
     *
     * @return the @ref FibConfig instance.
     */
    FibConfig& fibconfig();

    /**
     * Get the @ref IoLinkManager instance.
     *
     * @return the @ref IoLinkManager instance.
     */
    IoLinkManager& io_link_manager();

    /**
     * Get the @ref IoIpManager instance.
     *
     * @return the @ref IoIpManager instance.
     */
    IoIpManager& io_ip_manager();

    /**
     * Get the IfConfigGet plugin.
     *
     * @return the @ref IfConfigGet plugin.
     */
    IfConfigGet* ifconfig_get() { return _ifconfig_get; }

    /**
     * Get the IfConfigSet plugin.
     *
     * @return the @ref IfConfigSet plugin.
     */
    IfConfigSet* ifconfig_set() { return _ifconfig_set; }

    /**
     * Get the IfConfigObserver plugin.
     *
     * @return the @ref IfConfigObserver plugin.
     */
    IfConfigObserver* ifconfig_observer() { return _ifconfig_observer; }

    /**
     * Get the FibConfigForwarding plugin.
     *
     * @return the @ref FibConfigForwarding plugin.
     */
    FibConfigForwarding* fibconfig_forwarding() { return _fibconfig_forwarding; }

    /**
     * Get the FibConfigEntryGet plugin.
     *
     * @return the @ref FibConfigEntryGet plugin.
     */
    FibConfigEntryGet* fibconfig_entry_get() { return _fibconfig_entry_get; }

    /**
     * Get the FibConfigEntrySet plugin.
     *
     * @return the @ref FibConfigEntrySet plugin.
     */
    FibConfigEntrySet* fibconfig_entry_set() { return _fibconfig_entry_set; }

    /**
     * Get the FibConfigEntryObserver plugin.
     *
     * @return the @ref FibConfigEntryObserver plugin.
     */
    FibConfigEntryObserver* fibconfig_entry_observer() { return _fibconfig_entry_observer; }

    /**
     * Get the FibConfigTableGet plugin.
     *
     * @return the @ref FibConfigEntryGet plugin.
     */
    FibConfigTableGet* fibconfig_table_get() { return _fibconfig_table_get; }

    /**
     * Get the FibConfigTableSet plugin.
     *
     * @return the @ref FibConfigEntryGet plugin.
     */
    FibConfigTableSet* fibconfig_table_set() { return _fibconfig_table_set; }

    /**
     * Get the FibConfigTableObserver plugin.
     *
     * @return the @ref FibConfigEntryObserver plugin.
     */
    FibConfigTableObserver* fibconfig_table_observer() { return _fibconfig_table_observer; }

    /**
     * Allocate IoLink plugin instance.
     *
     * @param iftree the interface tree to use.
     * @param if_name the interface name.
     * @param vif_name the vif name.
     * @param ether_type the EtherType protocol number. If it is 0 then
     * it is unused.
     * @param filter_program the option filter program to be applied on the
     * received packets. The program uses tcpdump(1) style expression.
     * @return a new instance of @ref IoLink plugin on success, otherwise NULL.
     */
    virtual IoLink* allocate_io_link(const IfTree& iftree,
				     const string& if_name,
				     const string& vif_name,
				     uint16_t ether_type,
				     const string& filter_program) = 0;

    /**
     * De-allocate IoLink plugin.
     *
     * @param io_link the IoLink plugin to deallocate.
     */
    virtual void deallocate_io_link(IoLink* io_link);

    /**
     * Allocate IoIp plugin instance.
     *
     * @param iftree the interface tree to use.
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param ip_protocol the IP protocol number (IPPROTO_*).
     * @return a new instance of @ref IoIp plugin on success, otherwise NULL.
     */
    virtual IoIp* allocate_io_ip(const IfTree& iftree, int family,
				 uint8_t ip_protocol) = 0;

    /**
     * De-allocate IoIp plugin.
     *
     * @param io_ip the IoIp plugin to deallocate.
     */
    virtual void deallocate_io_ip(IoIp* io_ip);

protected:
    /**
     * Register all plugins.
     *
     * @param is_exclusive if true, the plugins are registered as the
     * exclusive plugins, otherwise are added to the lists of plugins.
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int register_all_plugins(bool is_exclusive, string& error_msg);

    /**
     * Stop all plugins operation.
     *
     * @param error_msg the error message (if error).
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int stop_all_plugins(string& error_msg);

    FeaNode&	_fea_node;

    //
    // The plugins
    //
    IfConfigGet*		_ifconfig_get;
    IfConfigSet*		_ifconfig_set;
    IfConfigObserver*		_ifconfig_observer;
    FibConfigForwarding*	_fibconfig_forwarding;
    FibConfigEntryGet*		_fibconfig_entry_get;
    FibConfigEntrySet*		_fibconfig_entry_set;
    FibConfigEntryObserver*	_fibconfig_entry_observer;
    FibConfigTableGet*		_fibconfig_table_get;
    FibConfigTableSet*		_fibconfig_table_set;
    FibConfigTableObserver*	_fibconfig_table_observer;
    list<IoLink *>		_io_link_list;
    list<IoIp *>		_io_ip_list;

    //
    // Data plane properties
    //
    bool		_have_ipv4;
    bool		_have_ipv6;

    //
    // Misc other state
    //
    const string	_manager_name;		// The data plane manager name
    bool		_is_loaded_plugins;	// True if plugins are loaded
    bool		_is_running_manager;	// True if manager is running
    bool		_is_running_plugins;	// True if plugins are running
};

#endif // __FEA_FEA_DATA_PLANE_MANAGER_HH__
