// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2009 XORP, Inc.
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

#include <xorp_config.h>
#if	defined(HOST_OS_BSDI)			\
	|| defined(HOST_OS_DRAGONFLYBSD)	\
	|| defined(HOST_OS_FREEBSD)		\
	|| defined(HOST_OS_MACOSX)		\
	|| defined(HOST_OS_NETBSD)		\
	|| defined(HOST_OS_OPENBSD)


#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "fea/data_plane/ifconfig/ifconfig_property_bsd.hh"
#include "fea/data_plane/ifconfig/ifconfig_get_getifaddrs.hh"
#include "fea/data_plane/ifconfig/ifconfig_get_ioctl.hh"
#include "fea/data_plane/ifconfig/ifconfig_get_sysctl.hh"
#include "fea/data_plane/ifconfig/ifconfig_set_ioctl.hh"
#include "fea/data_plane/ifconfig/ifconfig_observer_routing_socket.hh"
#include "fea/data_plane/ifconfig/ifconfig_vlan_get_bsd.hh"
#include "fea/data_plane/ifconfig/ifconfig_vlan_set_bsd.hh"
#include "fea/data_plane/firewall/firewall_get_ipfw2.hh"
#include "fea/data_plane/firewall/firewall_get_pf.hh"
#include "fea/data_plane/firewall/firewall_set_ipfw2.hh"
#include "fea/data_plane/firewall/firewall_set_pf.hh"
#include "fea/data_plane/fibconfig/fibconfig_forwarding_sysctl.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_get_routing_socket.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_set_routing_socket.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_observer_routing_socket.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_get_sysctl.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_set_routing_socket.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_observer_routing_socket.hh"
#include "fea/data_plane/io/io_link_pcap.hh"
#include "fea/data_plane/io/io_ip_socket.hh"
#include "fea/data_plane/io/io_tcpudp_socket.hh"

#include "fea_data_plane_manager_bsd.hh"


//
// FEA data plane manager class for BSD FEA.
//

#if 0
extern "C" FeaDataPlaneManager* create(FeaNode& fea_node)
{
    return (new FeaDataPlaneManagerBsd(fea_node));
}

extern "C" void destroy(FeaDataPlaneManager* fea_data_plane_manager)
{
    delete fea_data_plane_manager;
}
#endif // 0


FeaDataPlaneManagerBsd::FeaDataPlaneManagerBsd(FeaNode& fea_node)
    : FeaDataPlaneManager(fea_node, "BSD")
{
}

FeaDataPlaneManagerBsd::~FeaDataPlaneManagerBsd()
{
}

int
FeaDataPlaneManagerBsd::load_plugins(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_loaded_plugins)
	return (XORP_OK);

    XLOG_ASSERT(_ifconfig_property == NULL);
    XLOG_ASSERT(_ifconfig_get == NULL);
    XLOG_ASSERT(_ifconfig_set == NULL);
    XLOG_ASSERT(_ifconfig_observer == NULL);
    XLOG_ASSERT(_ifconfig_vlan_get == NULL);
    XLOG_ASSERT(_ifconfig_vlan_set == NULL);
    XLOG_ASSERT(_firewall_get == NULL);
    XLOG_ASSERT(_firewall_set == NULL);
    XLOG_ASSERT(_fibconfig_forwarding == NULL);
    XLOG_ASSERT(_fibconfig_entry_get == NULL);
    XLOG_ASSERT(_fibconfig_entry_set == NULL);
    XLOG_ASSERT(_fibconfig_entry_observer == NULL);
    XLOG_ASSERT(_fibconfig_table_get == NULL);
    XLOG_ASSERT(_fibconfig_table_set == NULL);
    XLOG_ASSERT(_fibconfig_table_observer == NULL);

    //
    // Load the plugins
    //
    _ifconfig_property = new IfConfigPropertyBsd(*this);

#if defined(HAVE_GETIFADDRS)
    _ifconfig_get = new IfConfigGetGetifaddrs(*this);
#elif defined(HAVE_SYSCTL_NET_RT_IFLIST)
    _ifconfig_get = new IfConfigGetSysctl(*this);
#elif defined(HAVE_IOCTL_SIOCGIFCONF)
    _ifconfig_get = new IfConfigGetIoctl(*this);
#endif

#if defined(HAVE_IOCTL_SIOCGIFCONF)
    _ifconfig_set = new IfConfigSetIoctl(*this);
#endif

#if defined(HAVE_ROUTING_SOCKETS)
    _ifconfig_observer = new IfConfigObserverRoutingSocket(*this);
#endif

#if defined(HAVE_VLAN_BSD)
    _ifconfig_vlan_get = new IfConfigVlanGetBsd(*this);
    _ifconfig_vlan_set = new IfConfigVlanSetBsd(*this);
#endif

    //
    // XXX: FreeBSD provides both IPFW2 and PF.
    // Currently the preferred one is IPFW2 (for no particular reason).
    //
#if defined(HAVE_FIREWALL_IPFW2)
    _firewall_get = new FirewallGetIpfw2(*this);
    _firewall_set = new FirewallSetIpfw2(*this);
#elif defined(HAVE_FIREWALL_PF)
    _firewall_get = new FirewallGetPf(*this);
    _firewall_set = new FirewallSetPf(*this);
#endif

#if defined(HAVE_SYSCTL_IPCTL_FORWARDING) || defined(HAVE_SYSCTL_IPV6CTL_FORWARDING) || defined(HAVE_SYSCTL_IPV6CTL_ACCEPT_RTADV)
    _fibconfig_forwarding = new FibConfigForwardingSysctl(*this);
#endif

#if defined(HAVE_ROUTING_SOCKETS)
    _fibconfig_entry_get = new FibConfigEntryGetRoutingSocket(*this);
#endif

#if defined(HAVE_ROUTING_SOCKETS)
    _fibconfig_entry_set = new FibConfigEntrySetRoutingSocket(*this);
#endif

#if defined(HAVE_ROUTING_SOCKETS)
    _fibconfig_entry_observer = new FibConfigEntryObserverRoutingSocket(*this);
#endif

#if defined(HAVE_SYSCTL_NET_RT_DUMP)
    _fibconfig_table_get = new FibConfigTableGetSysctl(*this);
#endif

#if defined(HAVE_ROUTING_SOCKETS)
    _fibconfig_table_set = new FibConfigTableSetRoutingSocket(*this);
#endif

#if defined(HAVE_ROUTING_SOCKETS)
    _fibconfig_table_observer = new FibConfigTableObserverRoutingSocket(*this);
#endif

    _is_loaded_plugins = true;

    return (XORP_OK);
}

int
FeaDataPlaneManagerBsd::register_plugins(string& error_msg)
{
    return (FeaDataPlaneManager::register_all_plugins(true, error_msg));
}

IoLink*
FeaDataPlaneManagerBsd::allocate_io_link(const IfTree& iftree,
					 const string& if_name,
					 const string& vif_name,
					 uint16_t ether_type,
					 const string& filter_program)
{
    IoLink* io_link = NULL;

    UNUSED(iftree);
    UNUSED(if_name);
    UNUSED(vif_name);
    UNUSED(ether_type);
    UNUSED(filter_program);

#ifdef HAVE_PCAP_H
    io_link = new IoLinkPcap(*this, iftree, if_name, vif_name, ether_type,
			     filter_program);
    _io_link_list.push_back(io_link);
#endif

    return (io_link);
}

IoIp*
FeaDataPlaneManagerBsd::allocate_io_ip(const IfTree& iftree, int family,
				       uint8_t ip_protocol)
{
    IoIp* io_ip = NULL;

    UNUSED(iftree);
    UNUSED(family);
    UNUSED(ip_protocol);

#ifdef HAVE_IP_RAW_SOCKETS
    io_ip = new IoIpSocket(*this, iftree, family, ip_protocol);
    _io_ip_list.push_back(io_ip);
#endif

    return (io_ip);
}

IoTcpUdp*
FeaDataPlaneManagerBsd::allocate_io_tcpudp(const IfTree& iftree, int family,
					   bool is_tcp)
{
    IoTcpUdp* io_tcpudp = NULL;

    UNUSED(iftree);
    UNUSED(family);

#ifdef HAVE_TCPUDP_UNIX_SOCKETS
    io_tcpudp = new IoTcpUdpSocket(*this, iftree, family, is_tcp);
    _io_tcpudp_list.push_back(io_tcpudp);
#endif

    return (io_tcpudp);
}


#endif // bsd
