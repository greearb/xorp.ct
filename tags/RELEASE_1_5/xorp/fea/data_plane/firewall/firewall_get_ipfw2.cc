// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

#ident "$XORP: xorp/fea/data_plane/firewall/firewall_get_ipfw2.cc,v 1.3 2008/04/26 02:07:27 pavlin Exp $"

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"

#include "libcomm/comm_api.h"

#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif
#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif

//
// XXX: RELENG_4 branches of FreeBSD after RELENG_4_9_RELEASE require
// that IPFW2 support be explicitly requested by defining the
// preprocessor symbol IPFW2 to a non-zero value.
//
#define IPFW2 1
#ifdef HAVE_NETINET_IP_FW_H
#include <netinet/ip_fw.h>
#endif


#include "fea/firewall_manager.hh"

#include "firewall_get_ipfw2.hh"


//
// Get information about firewall entries from the underlying system.
//
// The mechanism to obtain the information is IPFW2.
//

#ifdef HAVE_FIREWALL_IPFW2

FirewallGetIpfw2::FirewallGetIpfw2(FeaDataPlaneManager& fea_data_plane_manager)
    : FirewallGet(fea_data_plane_manager),
      _s4(-1)
{
}

FirewallGetIpfw2::~FirewallGetIpfw2()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the IPFW2 mechanism to get "
		   "information about firewall entries from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FirewallGetIpfw2::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    //
    // Open a raw IPv4 socket that IPFW2 uses for communication
    //
    _s4 = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (_s4 < 0) {
	error_msg = c_format("Could not open a raw socket for IPFW2 firewall: "
			     "%s", strerror(errno));
	return (XORP_ERROR);
    }

    _is_running = true;

    return (XORP_OK);
}

int
FirewallGetIpfw2::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (comm_close(_s4) != XORP_OK) {
	error_msg = c_format("Could not close raw socket for IPFW2 "
			     "firewall: %s",
			     strerror(errno));
	_s4 = -1;
	return (XORP_ERROR);
    }

    _s4 = -1;
    _is_running = false;

    return (XORP_OK);
}

#define NEXT_IP_FW(ip_fwp) ((struct ip_fw *)((char *)ip_fwp + RULESIZE(ip_fwp)))

int
FirewallGetIpfw2::get_table4(list<FirewallEntry>& firewall_entry_list,
			     string& error_msg)
{
    return (get_table(AF_INET, firewall_entry_list, error_msg));
}

int
FirewallGetIpfw2::get_table6(list<FirewallEntry>& firewall_entry_list,
			     string& error_msg)
{
#ifdef HAVE_IPV6
    return (get_table(AF_INET6, firewall_entry_list, error_msg));
#else
    error_msg = c_format("Cannot get the IPv6 firewall table: "
			 "IPv6 is not supported");
    return (XORP_ERROR);
#endif
}

int
FirewallGetIpfw2::get_table(int family,
			    list<FirewallEntry>& firewall_entry_list,
			    string& error_msg)
{
    vector<uint8_t> chainbuf;
    socklen_t nbytes;
    size_t nalloc, nstat, n;
    struct ip_fw* ip_fw;

    nbytes = 1024;
    nalloc = 1024;

    //
    // Play elastic buffer games to get a snapshot of the entire rule chain
    //
    chainbuf.reserve(nalloc);
    while (nbytes >= nalloc) {
	nbytes = nalloc = nalloc * 2 + 200;
	chainbuf.reserve(nalloc);
	if (getsockopt(_s4, IPPROTO_IP, IP_FW_GET, &chainbuf[0], &nbytes) < 0) {
	    error_msg = c_format("Could not get the IPFW2 firewall table: %s",
				 strerror(errno));
	    return (XORP_ERROR);
	}
    }
    chainbuf.resize(nbytes);

    //
    // Count the static rules.
    // We need to scan the list because they have variable size.
    //
    for (nstat = 1, ip_fw = reinterpret_cast<struct ip_fw *>(&chainbuf[0]);
	 (ip_fw->rulenum < 65535)
	     && (ip_fw < reinterpret_cast<struct ip_fw *>(&chainbuf[nbytes]));
	     ++nstat, ip_fw = NEXT_IP_FW(ip_fw)) {
	;	// XXX: Do nothing
    }

    //
    // Parse buffer contents for all static rules
    //
    for (n = 0, ip_fw = reinterpret_cast<struct ip_fw *>(&chainbuf[0]);
	 n < nstat;
	 n++, ip_fw = NEXT_IP_FW(ip_fw)) {
	size_t len;
	ipfw_insn* cmd;
	uint32_t rule_number = FirewallEntry::RULE_NUMBER_DEFAULT;
	string ifname, vifname;
	IPvXNet src_network = IPvXNet(family);
	IPvXNet dst_network = IPvXNet(family);
	uint32_t ip_protocol = FirewallEntry::IP_PROTOCOL_ANY;
	uint32_t src_port_begin = FirewallEntry::PORT_MIN;
	uint32_t src_port_end = FirewallEntry::PORT_MAX;
	uint32_t dst_port_begin = FirewallEntry::PORT_MIN;
	uint32_t dst_port_end = FirewallEntry::PORT_MAX;
	FirewallEntry::Action action = FirewallEntry::ACTION_INVALID;

	//
	// Get the rule number
	//
	rule_number = ip_fw->rulenum;

	//
	// Get the action
	//
	cmd = ACTION_PTR(ip_fw);
	for (len = ip_fw->cmd_len - ip_fw->act_ofs; len > 0;
	     len -= F_LEN(cmd), cmd += F_LEN(cmd)) {
	    switch(cmd->opcode) {
	    case O_ACCEPT:
		action = FirewallEntry::ACTION_PASS;
		break;
	    case O_DENY:
		action = FirewallEntry::ACTION_DROP;
		break;
	    case O_REJECT:
		action = FirewallEntry::ACTION_REJECT;
		break;
	    default:
		XLOG_WARNING("Ingoring firewall entry with type %u",
			     cmd->opcode);
		break;
	    }
	    //
	    // XXX: There could be more than one actions, but for now
	    // we care only about the first one.
	    //
	    if (action != FirewallEntry::ACTION_INVALID)
		break;
	}
	if (action == FirewallEntry::ACTION_INVALID) {
	    XLOG_WARNING("Ignoring firewall entry: unknown action");
	    continue;
	}

	//
	// Get the rest of the entry
	//
	bool have_ipv4 = false;
	bool have_ipv6 = false;
	for (len = ip_fw->act_ofs, cmd = ip_fw->cmd;
	     len > 0;
	     len -= F_LEN(cmd), cmd += F_LEN(cmd)) {
	    if ((cmd->len & F_OR) || (cmd->len & F_NOT))
		continue;
	    switch (cmd->opcode) {
	    case O_IP4:
	    {
		// The IPv4 address family
		have_ipv4 = true;
		break;
	    }
	    case O_IP6:
	    {
		// The IPv6 address family
		have_ipv6 = true;
		break;
	    }
	    case O_IP_SRC_MASK:
	    {
		// The IPv4 source address and mask
		ipfw_insn_ip* cmd_ip = reinterpret_cast<ipfw_insn_ip *>(cmd);
		//
		// XXX: There could be a list of address-mask pairs, but
		// we consider only the first one.
		//
		IPv4 addr(cmd_ip->addr);
		IPv4 mask(cmd_ip->mask);
		src_network = IPvXNet(IPvX(addr), mask.mask_len());
		break;
	    }
	    case O_IP_DST_MASK:
	    {
		// The IPv4 destination address and mask
		ipfw_insn_ip* cmd_ip = reinterpret_cast<ipfw_insn_ip *>(cmd);
		//
		// XXX: There could be a list of address-mask pairs, but
		// we consider only the first one.
		//
		IPv4 addr(cmd_ip->addr);
		IPv4 mask(cmd_ip->mask);
		dst_network = IPvXNet(IPvX(addr), mask.mask_len());
		break;
	    }
	    case O_IP6_SRC_MASK:
	    {
		// The IPv6 source address and mask
		ipfw_insn_ip6* cmd_ip6 = reinterpret_cast<ipfw_insn_ip6 *>(cmd);
		//
		// XXX: There could be a list of address-mask pairs, but
		// we consider only the first one.
		//
		IPv6 addr(cmd_ip6->addr6);
		IPv6 mask(cmd_ip6->mask6);
		src_network = IPvXNet(IPvX(addr), mask.mask_len());
		break;
	    }
	    case O_IP6_DST_MASK:
	    {
		// The IPv6 destination address and mask
		ipfw_insn_ip6* cmd_ip6 = reinterpret_cast<ipfw_insn_ip6 *>(cmd);
		//
		// XXX: There could be a list of address-mask pairs, but
		// we consider only the first one.
		//
		IPv6 addr(cmd_ip6->addr6);
		IPv6 mask(cmd_ip6->mask6);
		dst_network = IPvXNet(IPvX(addr), mask.mask_len());
		break;
	    }
	    case O_IP_SRCPORT:
	    {
		// The TCP/UDP source port range
		ipfw_insn_u16* cmd_u16 = reinterpret_cast<ipfw_insn_u16 *>(cmd);
		//
		// XXX: There could be a list of port pairs, but
		// we consider only the first one.
		//
		src_port_begin = cmd_u16->ports[0];
		src_port_end = cmd_u16->ports[1];
		break;
	    }
	    case O_IP_DSTPORT:
	    {
		// The TCP/UDP destination port range
		ipfw_insn_u16* cmd_u16 = reinterpret_cast<ipfw_insn_u16 *>(cmd);
		//
		// XXX: There could be a list of port pairs, but
		// we consider only the first one.
		//
		dst_port_begin = cmd_u16->ports[0];
		dst_port_end = cmd_u16->ports[1];
		break;
	    }
	    case O_PROTO:
	    {
		// The protocol
		ip_protocol = cmd->arg1;
		break;
	    }
	    case O_VIA:
	    {
		// The interface and vif
		ipfw_insn_if* cmd_if = reinterpret_cast<ipfw_insn_if *>(cmd);
		if (cmd_if->name[0] == '\0') {
		    //
		    // XXX: The "via" could be an IP address, but for now
		    // we support only interface names.
		    //
		    XLOG_WARNING("Ignoring \"via\" firewall option that "
				 "is not an interface name");
		    break;
		}
		ifname = string(cmd_if->name);
		vifname = ifname;	// XXX: ifname == vifname
		break;
	    }
	    default:
		break;
	    }
	}

	// Ignore entries that don't match the address family
	bool accept_entry = false;
	switch (family) {
	case AF_INET:
	    if (have_ipv4)
		accept_entry = true;
	    break;
#ifdef HAVE_IPV6
	case AF_INET6:
	    if (have_ipv6)
		accept_entry = true;
	    break;
#endif
	default:
	    XLOG_FATAL("Unknown family %d", family);
	    break;
	}
	if (! accept_entry)
	    continue;

	// Add the entry to the list
	FirewallEntry firewall_entry(rule_number, ifname, vifname, src_network,
				     dst_network, ip_protocol, src_port_begin,
				     src_port_end, dst_port_begin,
				     dst_port_end, action);
	firewall_entry_list.push_back(firewall_entry);
    }

    return (XORP_OK);
}

#endif // HAVE_FIREWALL_IPFW2
