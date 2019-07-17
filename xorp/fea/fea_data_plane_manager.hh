// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2011 XORP, Inc and Others
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


#ifndef __FEA_FEA_DATA_PLANE_MANAGER_HH__
#define __FEA_FEA_DATA_PLANE_MANAGER_HH__



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
#ifndef XORP_DISABLE_FIREWALL
class FirewallGet;
class FirewallManager;
class FirewallSet;
#endif
class IfConfig;
class IfConfigGet;
class IfConfigObserver;
class IfConfigProperty;
class IfConfigSet;
class IfConfigVlanGet;
class IfConfigVlanSet;
class IfTree;
class IoLink;
class IoLinkManager;
class IoIp;
class IoIpManager;
class IoTcpUdp;
class IoTcpUdpManager;


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
    virtual bool have_ipv4() const;

    /**
     * Return true if the underlying system supports IPv6.
     * 
     * @return true if the underlying system supports IPv6, otherwise false.
     */
    virtual bool have_ipv6() const;

    /**
     * Get the @ref IfConfig instance.
     *
     * @return the @ref IfConfig instance.
     */
    IfConfig& ifconfig();

#ifndef XORP_DISABLE_FIREWALL
    /**
     * Get the @ref FirewallManager instance.
     *
     * @return the @ref FirewallManager instance.
     */
    FirewallManager& firewall_manager();

    /**
     * Get the FirewallGet plugin.
     *
     * @return the @ref FirewallGet plugin.
     */
    FirewallGet* firewall_get() { return _firewall_get; }

    /**
     * Get the FirewallSet plugin.
     *
     * @return the @ref FirewallSet plugin.
     */
    FirewallSet* firewall_set() { return _firewall_set; }

#endif

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
     * Get the @ref IoTcpUdpManager instance.
     *
     * @return the @ref IoTcpUdpManager instance.
     */
    IoTcpUdpManager& io_tcpudp_manager();

    /**
     * Get the IfConfigProperty plugin.
     *
     * @return the @ref IfConfigGet plugin.
     */
    IfConfigProperty* ifconfig_property() { return _ifconfig_property; }

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
     * Get the IfConfigVlanGet plugin.
     *
     * @return the @ref IfConfigVlanGet plugin.
     */
    IfConfigVlanGet* ifconfig_vlan_get() { return _ifconfig_vlan_get; }

    /**
     * Get the IfConfigVlanSet plugin.
     *
     * @return the @ref IfConfigVlanSet plugin.
     */
    IfConfigVlanSet* ifconfig_vlan_set() { return _ifconfig_vlan_set; }

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

    /**
     * Allocate IoTcpUdp plugin instance.
     *
     * @param iftree the interface tree to use.
     * @param family the address family (AF_INET or AF_INET6 for IPv4 and IPv6
     * respectively).
     * @param is_tcp if true allocate a TCP entry, otherwise UDP.
     * @return a new instance of @ref IoTcpUdp plugin on success,
     * otherwise NULL.
     */
    virtual IoTcpUdp* allocate_io_tcpudp(const IfTree& iftree, int family,
					 bool is_tcp) = 0;

    /**
     * De-allocate IoTcpUdp plugin.
     *
     * @param io_tcpudp the IoTcpUdp plugin to deallocate.
     */
    virtual void deallocate_io_tcpudp(IoTcpUdp* io_tcpudp);

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
    IfConfigProperty*		_ifconfig_property;
    IfConfigGet*		_ifconfig_get;
    IfConfigSet*		_ifconfig_set;
    IfConfigObserver*		_ifconfig_observer;
    IfConfigVlanGet*		_ifconfig_vlan_get;
    IfConfigVlanSet*		_ifconfig_vlan_set;
#ifndef XORP_DISABLE_FIREWALL
    FirewallGet*		_firewall_get;
    FirewallSet*		_firewall_set;
#endif
    FibConfigForwarding*	_fibconfig_forwarding;
    FibConfigEntryGet*		_fibconfig_entry_get;
    FibConfigEntrySet*		_fibconfig_entry_set;
    FibConfigEntryObserver*	_fibconfig_entry_observer;
    FibConfigTableGet*		_fibconfig_table_get;
    FibConfigTableSet*		_fibconfig_table_set;
    FibConfigTableObserver*	_fibconfig_table_observer;
    list<IoLink *>		_io_link_list;
    list<IoIp *>		_io_ip_list;
    list<IoTcpUdp *>		_io_tcpudp_list;

    //
    // Misc other state
    //
    const string	_manager_name;		// The data plane manager name
    bool		_is_loaded_plugins;	// True if plugins are loaded
    bool		_is_running_manager;	// True if manager is running
    bool		_is_running_plugins;	// True if plugins are running
};

#endif // __FEA_FEA_DATA_PLANE_MANAGER_HH__
