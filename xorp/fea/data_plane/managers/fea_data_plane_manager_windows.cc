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



#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#ifdef HOST_OS_WINDOWS
#include "fea/data_plane/control_socket/windows_rras_support.hh"
#endif
#include "fea/data_plane/ifconfig/ifconfig_property_windows.hh"
#include "fea/data_plane/ifconfig/ifconfig_get_iphelper.hh"
#include "fea/data_plane/ifconfig/ifconfig_set_iphelper.hh"
#include "fea/data_plane/ifconfig/ifconfig_observer_iphelper.hh"
#include "fea/data_plane/fibconfig/fibconfig_forwarding_windows.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_get_iphelper.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_get_rtmv2.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_set_iphelper.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_set_rtmv2.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_observer_iphelper.hh"
#include "fea/data_plane/fibconfig/fibconfig_entry_observer_rtmv2.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_get_iphelper.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_set_iphelper.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_set_rtmv2.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_observer_iphelper.hh"
#include "fea/data_plane/fibconfig/fibconfig_table_observer_rtmv2.hh"
#include "fea/data_plane/io/io_link_pcap.hh"
#include "fea/data_plane/io/io_ip_socket.hh"
#include "fea/data_plane/io/io_tcpudp_socket.hh"

#include "fea_data_plane_manager_windows.hh"


//
// FEA data plane manager class for Windows FEA.
//

#if 0
extern "C" FeaDataPlaneManager* create(FeaNode& fea_node)
{
    return (new FeaDataPlaneManagerWindows(fea_node));
}

extern "C" void destroy(FeaDataPlaneManager* fea_data_plane_manager)
{
    delete fea_data_plane_manager;
}
#endif // 0


FeaDataPlaneManagerWindows::FeaDataPlaneManagerWindows(FeaNode& fea_node)
    : FeaDataPlaneManager(fea_node, "Windows")
{
}

FeaDataPlaneManagerWindows::~FeaDataPlaneManagerWindows()
{
}

int
FeaDataPlaneManagerWindows::start_manager(string& error_msg)
{
#if defined(HOST_OS_WINDOWS)
    //
    // Load support code for Windows Router Manager V2, if it
    // is detected as running and/or configured.
    //
    if (WinSupport::is_rras_running()) {
	WinSupport::add_protocol_to_registry(AF_INET);
#if 0
	WinSupport::add_protocol_to_registry(AF_INET6);
#endif
	WinSupport::restart_rras();
	WinSupport::add_protocol_to_rras(AF_INET);
#if 0
	WinSupport::add_protocol_to_rras(AF_INET6);
#endif
	TimerList::system_sleep(TimeVal(2, 0));
    }
#endif // HOST_OS_WINDOWS

    return (FeaDataPlaneManager::start_manager(error_msg));
}

int
FeaDataPlaneManagerWindows::load_plugins(string& error_msg)
{
    UNUSED(error_msg);

    if (_is_loaded_plugins)
	return (XORP_OK);

    XLOG_ASSERT(_ifconfig_property == NULL);
    XLOG_ASSERT(_ifconfig_get == NULL);
    XLOG_ASSERT(_ifconfig_set == NULL);
    XLOG_ASSERT(_ifconfig_observer == NULL);
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
#if defined(HOST_OS_WINDOWS)
    bool is_rras_running = WinSupport::is_rras_running();

    _ifconfig_property = new IfConfigPropertyWindows(*this);
    _ifconfig_get = new IfConfigGetIPHelper(*this);
    _ifconfig_set = new IfConfigSetIPHelper(*this);
    _ifconfig_observer = new IfConfigObserverIPHelper(*this);
    _fibconfig_forwarding = new FibConfigForwardingWindows(*this);
    _fibconfig_entry_get = new FibConfigEntryGetIPHelper(*this);
    // _fibconfig_entry_get = new FibConfigEntryGetRtmV2(*this);
    if (is_rras_running) {
	_fibconfig_entry_set = new FibConfigEntrySetRtmV2(*this);
    } else {
	_fibconfig_entry_set = new FibConfigEntrySetIPHelper(*this);
    }
    _fibconfig_entry_observer = new FibConfigEntryObserverIPHelper(*this);
    // _fibconfig_entry_observer = new FibConfigEntryObserverRtmV2(*this);
    _fibconfig_table_get = new FibConfigTableGetIPHelper(*this);
    _fibconfig_table_set = new FibConfigTableSetIPHelper(*this);
    // _fibconfig_table_set = new FibConfigTableSetRtmV2(*this);
    if (is_rras_running) {
	_fibconfig_table_observer = new FibConfigTableObserverRtmV2(*this);
    } else {
	_fibconfig_table_observer = new FibConfigTableObserverIPHelper(*this);
    }
#endif // HOST_OS_WINDOWS

    _is_loaded_plugins = true;

    return (XORP_OK);
}

int
FeaDataPlaneManagerWindows::register_plugins(string& error_msg)
{
    return (FeaDataPlaneManager::register_all_plugins(true, error_msg));
}

IoLink*
FeaDataPlaneManagerWindows::allocate_io_link(const IfTree& iftree,
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

#ifdef HAVE_PCAP
    io_link = new IoLinkPcap(*this, iftree, if_name, vif_name, ether_type,
			     filter_program);
    _io_link_list.push_back(io_link);
#endif

    return (io_link);
}

IoIp*
FeaDataPlaneManagerWindows::allocate_io_ip(const IfTree& iftree, int family,
					   uint8_t ip_protocol)
{
    IoIp* io_ip = NULL;

#ifdef HAVE_IP_RAW_SOCKETS
    io_ip = new IoIpSocket(*this, iftree, family, ip_protocol);
    _io_ip_list.push_back(io_ip);
#else
    UNUSED(iftree);
    UNUSED(family);
    UNUSED(ip_protocol);
#endif

    return (io_ip);
}

IoTcpUdp*
FeaDataPlaneManagerWindows::allocate_io_tcpudp(const IfTree& iftree,
					       int family, bool is_tcp)
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
