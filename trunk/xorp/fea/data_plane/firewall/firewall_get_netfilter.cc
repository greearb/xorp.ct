// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

#ident "$XORP: xorp/fea/data_plane/firewall/firewall_get_netfilter.cc,v 1.2 2008/07/23 05:10:25 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libcomm/comm_api.h"

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/stat.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif

#ifdef HAVE_LINUX_NETFILTER_IPV4_IP_TABLES_H
#include <linux/netfilter_ipv4/ip_tables.h>
#endif
#ifdef HAVE_LINUX_NETFILTER_IPV6_IP6_TABLES_H
#include <linux/netfilter_ipv6/ip6_tables.h>
#endif

#include "fea/firewall_manager.hh"

#include "firewall_get_netfilter.hh"


//
// Get information about firewall entries from the underlying system.
//
// The mechanism to obtain the information is NETFILTER.
//

#ifdef HAVE_FIREWALL_NETFILTER

const string FirewallGetNetfilter::_netfilter_table_name = "filter";

FirewallGetNetfilter::FirewallGetNetfilter(FeaDataPlaneManager& fea_data_plane_manager)
    : FirewallGet(fea_data_plane_manager),
      _s4(-1),
      _s6(-1)
{
}

FirewallGetNetfilter::~FirewallGetNetfilter()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the NETFILTER mechanism to get "
		   "information about firewall entries from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FirewallGetNetfilter::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    //
    // Check if we have the necessary permission.
    // If not, print a warning and continue without firewall.
    //
    if (geteuid() != 0) {
	XLOG_WARNING("Cannot start the NETFILTER firewall mechanism: "
		     "must be root; continuing without firewall setup");
	return (XORP_OK);
    }

    //
    // Open a raw IPv4 socket that NETFILTER uses for communication
    //
    _s4 = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (_s4 < 0) {
	error_msg = c_format("Could not open a raw IPv4 socket for NETFILTER "
			     "firewall: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }

#ifdef HAVE_IPV6
    //
    // Open a raw IPv6 socket that NETFILTER uses for communication
    //
    _s6 = socket(AF_INET6, SOCK_RAW, IPPROTO_RAW);
    if (_s6 < 0) {
	error_msg = c_format("Could not open a raw IPv6 socket for NETFILTER "
			     "firewall: %s",
			     strerror(errno));
	comm_close(_s4);
	_s4 = -1;
	return (XORP_ERROR);
    }
#endif // HAVE_IPV6

    _is_running = true;

    return (XORP_OK);
}

int
FirewallGetNetfilter::stop(string& error_msg)
{
    int ret_value4 = XORP_OK;
    int ret_value6 = XORP_OK;

    if (! _is_running)
	return (XORP_OK);

    if (_s4 >= 0) {
	ret_value4 = comm_close(_s4);
	_s4 = -1;
	if (ret_value4 != XORP_OK) {
	    error_msg = c_format("Could not close raw IPv4 socket for "
				 "NETFILTER firewall: %s",
				 strerror(errno));
	}
    }

    if (_s6 >= 0) {
	ret_value6 = comm_close(_s6);
	_s6 = -1;
	if ((ret_value6 != XORP_OK) && (ret_value4 == XORP_OK)) {
	    error_msg = c_format("Could not close raw IPv6 socket for "
				 "NETFILTER firewall: %s",
				 strerror(errno));
	}
    }

    if ((ret_value4 != XORP_OK) || (ret_value6 != XORP_OK))
	return (XORP_ERROR);

    _is_running = false;

    return (XORP_OK);
}

int
FirewallGetNetfilter::get_table4(list<FirewallEntry>& firewall_entry_list,
				 string& error_msg)
{
    struct ipt_getinfo info4;
    struct ipt_get_entries* entries4_p;
    socklen_t socklen;

    memset(&info4, 0, sizeof(info4));
    strlcpy(info4.name, _netfilter_table_name.c_str(), sizeof(info4.name));

    //
    // Get information about the size of the table
    //
    socklen = sizeof(info4);
    if (getsockopt(_s4, IPPROTO_IP, IPT_SO_GET_INFO, &info4, &socklen) < 0) {
        error_msg = c_format("Could not get the NETFILTER IPv4 firewall table: "
			     "%s", strerror(errno));
	return (XORP_ERROR);
    }

    //
    // Get the entries
    //
    vector<uint8_t> entries4(sizeof(*entries4_p) + info4.size);
    socklen = entries4.size();
    if (getsockopt(_s4, IPPROTO_IP, IPT_SO_GET_ENTRIES, &entries4, &socklen)
	< 0) {
        error_msg = c_format("Could not get the NETFILTER IPv4 firewall table "
			     "entries: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }

    //
    // Parse buffer contents for all rules
    //
    for (size_t i = 0; i < entries4.size(); ) {
	struct ipt_entry* ipt;
	struct ipt_entry_match* iem;
	struct ipt_entry_target* iet;
	struct ipt_standard_target* ist;
	uint8_t* ptr;
	
	ipt = reinterpret_cast<struct ipt_entry *>(&entries4[i]);
	i += ipt->next_offset;

        //
        // Extract various information from the rule
        //
	uint32_t rule_number = FirewallEntry::RULE_NUMBER_DEFAULT;
	string ifname, vifname;
	IPvXNet src_network = IPvXNet(IPv4::af());
	IPvXNet dst_network = IPvXNet(IPv4::af());
	uint32_t ip_protocol = FirewallEntry::IP_PROTOCOL_ANY;
	uint32_t src_port_begin = FirewallEntry::PORT_MIN;
	uint32_t src_port_end = FirewallEntry::PORT_MAX;
	uint32_t dst_port_begin = FirewallEntry::PORT_MIN;
	uint32_t dst_port_end = FirewallEntry::PORT_MAX;
	FirewallEntry::Action action = FirewallEntry::ACTION_INVALID;

	//
	// Get the rule number
	//
	// XXX: Do nothing, because NETFILTER doesn't have rule numbers
	// at the kernel API level.

	//
	// Get the action
	//
	ptr = reinterpret_cast<uint8_t *>(ipt);
	ptr += ipt->target_offset;
	iet = reinterpret_cast<struct ipt_entry_target *>(ptr);
	// XXX: Only standard targets are processed
	if (strncmp(iet->u.user.name, IPT_STANDARD_TARGET,
		    sizeof(iet->u.user.name))
	    != 0) {
	    XLOG_WARNING("Ingoring firewall entry with non-standard target %s",
			 iet->u.user.name);
	    continue;
	}
	ist = reinterpret_cast<struct ipt_standard_target *>(ptr);
	switch (ist->verdict) {
	case (-NF_ACCEPT - 1):
	    action = FirewallEntry::ACTION_PASS;
	    break;
	case (-NF_DROP - 1):
	    action = FirewallEntry::ACTION_DROP;
	    break;
	default:
	    XLOG_WARNING("Ingoring firewall entry with action %u: "
			 "unknown action",
			 ist->verdict);
	    break;
	}

	//
	// Get the protocol
	//
	if (ipt->ip.proto != 0)
	    ip_protocol = ipt->ip.proto;

	//
	// Get the source and destination network addresses
	//
	IPv4 src_addr, dst_addr, src_mask, dst_mask;
	src_addr.copy_in(ipt->ip.src);
	dst_addr.copy_in(ipt->ip.dst);
	src_mask.copy_in(ipt->ip.smsk);
	src_mask.copy_in(ipt->ip.dmsk);

	src_network = IPvXNet(IPvX(src_addr), src_mask.mask_len());
	dst_network = IPvXNet(IPvX(dst_addr), dst_mask.mask_len());

	//
	// Get the source and destination port number range
	//
	for (size_t j = sizeof(*ipt); j < ipt->target_offset; ) {
	    ptr = reinterpret_cast<uint8_t *>(ipt);
	    ptr += j;
	    iem = reinterpret_cast<struct ipt_entry_match *>(ptr);
	    j += iem->u.match_size;
	    switch (ipt->ip.proto) {
	    case IPPROTO_TCP:
	    {
		struct ipt_tcp* ip_tcp;
		ip_tcp = reinterpret_cast<struct ipt_tcp *>(iem->data);
		src_port_begin = ip_tcp->spts[0];
		src_port_end = ip_tcp->spts[1];
		dst_port_begin = ip_tcp->dpts[0];
		dst_port_end = ip_tcp->dpts[1];
		break;
	    }
	    case IPPROTO_UDP:
	    {
		struct ipt_udp* ip_udp;
		ip_udp = reinterpret_cast<struct ipt_udp *>(iem->data);
		src_port_begin = ip_udp->spts[0];
		src_port_end = ip_udp->spts[1];
		dst_port_begin = ip_udp->dpts[0];
		dst_port_end = ip_udp->dpts[1];
		break;
	    }
	    default:
		break;
	    }
	}

	//
	// The interface and vif
	//
	if (ipt->ip.iniface[0] != '\0') {
	    ifname = string(ipt->ip.iniface);
	    vifname = ifname;		// XXX: ifname == vifname
	}

	// Add the entry to the list
	FirewallEntry firewall_entry(rule_number, ifname, vifname, src_network,
				     dst_network, ip_protocol, src_port_begin,
				     src_port_end, dst_port_begin,
				     dst_port_end, action);
	firewall_entry_list.push_back(firewall_entry);
    }

    return (XORP_OK);
}

int
FirewallGetNetfilter::get_table6(list<FirewallEntry>& firewall_entry_list,
				 string& error_msg)
{
    struct ip6t_getinfo info6;
    struct ip6t_get_entries* entries6_p;
    socklen_t socklen;

    memset(&info6, 0, sizeof(info6));
    strlcpy(info6.name, _netfilter_table_name.c_str(), sizeof(info6.name));

    //
    // Get information about the size of the table
    //
    socklen = sizeof(info6);
    if (getsockopt(_s6, IPPROTO_IPV6, IP6T_SO_GET_INFO, &info6, &socklen) < 0) {
        error_msg = c_format("Could not get the NETFILTER IPv6 firewall table: "
			     "%s", strerror(errno));
	return (XORP_ERROR);
    }

    //
    // Get the entries
    //
    vector<uint8_t> entries6(sizeof(*entries6_p) + info6.size);
    socklen = entries6.size();
    if (getsockopt(_s6, IPPROTO_IPV6, IP6T_SO_GET_ENTRIES, &entries6, &socklen)
	< 0) {
        error_msg = c_format("Could not get the NETFILTER IPv6 firewall table "
			     "entries: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }

    //
    // Parse buffer contents for all rules
    //
    for (size_t i = 0; i < entries6.size(); ) {
	struct ip6t_entry* ipt;
	struct ip6t_entry_match* iem;
	struct ip6t_entry_target* iet;
	struct ip6t_standard_target* ist;
	uint8_t* ptr;
	
	ipt = reinterpret_cast<struct ip6t_entry *>(&entries6[i]);
	i += ipt->next_offset;

        //
        // Extract various information from the rule
        //
	uint32_t rule_number = FirewallEntry::RULE_NUMBER_DEFAULT;
	string ifname, vifname;
	IPvXNet src_network = IPvXNet(IPv6::af());
	IPvXNet dst_network = IPvXNet(IPv6::af());
	uint32_t ip_protocol = FirewallEntry::IP_PROTOCOL_ANY;
	uint32_t src_port_begin = FirewallEntry::PORT_MIN;
	uint32_t src_port_end = FirewallEntry::PORT_MAX;
	uint32_t dst_port_begin = FirewallEntry::PORT_MIN;
	uint32_t dst_port_end = FirewallEntry::PORT_MAX;
	FirewallEntry::Action action = FirewallEntry::ACTION_INVALID;

	//
	// Get the rule number
	//
	// XXX: Do nothing, because NETFILTER doesn't have rule numbers
	// at the kernel API level.

	//
	// Get the action
	//
	ptr = reinterpret_cast<uint8_t *>(ipt);
	ptr += ipt->target_offset;
	iet = reinterpret_cast<struct ip6t_entry_target *>(ptr);
	// XXX: Only standard targets are processed
	if (strncmp(iet->u.user.name, IP6T_STANDARD_TARGET,
		    sizeof(iet->u.user.name))
	    != 0) {
	    XLOG_WARNING("Ingoring firewall entry with non-standard target %s",
			 iet->u.user.name);
	    continue;
	}
	ist = reinterpret_cast<struct ip6t_standard_target *>(iet->data);
	switch (ist->verdict) {
	case (-NF_ACCEPT - 1):
	    action = FirewallEntry::ACTION_PASS;
	    break;
	case (-NF_DROP - 1):
	    action = FirewallEntry::ACTION_DROP;
	    break;
	default:
	    XLOG_WARNING("Ingoring firewall entry with action %u: "
			 "unknown action",
			 ist->verdict);
	    break;
	}

	//
	// Get the protocol
	//
	if (ipt->ipv6.proto != 0)
	    ip_protocol = ipt->ipv6.proto;

	//
	// Get the source and destination network addresses
	//
	IPv6 src_addr, dst_addr, src_mask, dst_mask;
	src_addr.copy_in(ipt->ipv6.src);
	dst_addr.copy_in(ipt->ipv6.dst);
	src_mask.copy_in(ipt->ipv6.smsk);
	src_mask.copy_in(ipt->ipv6.dmsk);

	src_network = IPvXNet(IPvX(src_addr), src_mask.mask_len());
	dst_network = IPvXNet(IPvX(dst_addr), dst_mask.mask_len());

	//
	// Get the source and destination port number range
	//
	for (size_t j = sizeof(*ipt); j < ipt->target_offset; ) {
	    ptr = reinterpret_cast<uint8_t *>(ipt);
	    ptr += j;
	    iem = reinterpret_cast<struct ip6t_entry_match *>(ptr);
	    j += iem->u.match_size;
	    switch (ipt->ipv6.proto) {
	    case IPPROTO_TCP:
	    {
		struct ip6t_tcp* ip_tcp;
		ip_tcp = reinterpret_cast<struct ip6t_tcp *>(iem->data);
		src_port_begin = ip_tcp->spts[0];
		src_port_end = ip_tcp->spts[1];
		dst_port_begin = ip_tcp->dpts[0];
		dst_port_end = ip_tcp->dpts[1];
		break;
	    }
	    case IPPROTO_UDP:
	    {
		struct ip6t_udp* ip_udp;
		ip_udp = reinterpret_cast<struct ip6t_udp *>(iem->data);
		src_port_begin = ip_udp->spts[0];
		src_port_end = ip_udp->spts[1];
		dst_port_begin = ip_udp->dpts[0];
		dst_port_end = ip_udp->dpts[1];
		break;
	    }
	    default:
		break;
	    }
	}

	//
	// The interface and vif
	//
	if (ipt->ipv6.iniface[0] != '\0') {
	    ifname = string(ipt->ipv6.iniface);
	    vifname = ifname;		// XXX: ifname == vifname
	}

	// Add the entry to the list
	FirewallEntry firewall_entry(rule_number, ifname, vifname, src_network,
				     dst_network, ip_protocol, src_port_begin,
				     src_port_end, dst_port_begin,
				     dst_port_end, action);
	firewall_entry_list.push_back(firewall_entry);
    }

    return (XORP_OK);
}

#endif // HAVE_FIREWALL_NETFILTER
