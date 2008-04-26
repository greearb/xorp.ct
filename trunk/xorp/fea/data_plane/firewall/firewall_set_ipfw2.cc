// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP$"

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

#ifdef HAVE_FIREWALL_IPFW2
#if __FreeBSD_version < 500000
define IPFW2 1
#endif
#endif
#ifdef HAVE_NETINET_IP_FW_H
#include <netinet/ip_fw.h>
#endif


#include "fea/firewall_manager.hh"

#include "firewall_set_ipfw2.hh"


//
// Set firewall information into the underlying system.
//
// The mechanism to set the information is IPFW2.
//

#ifdef HAVE_FIREWALL_IPFW2

FirewallSetIpfw2::FirewallSetIpfw2(FeaDataPlaneManager& fea_data_plane_manager)
    : FirewallSet(fea_data_plane_manager),
      _s4(-1),
      _saved_autoinc_step(0)
{
}

FirewallSetIpfw2::~FirewallSetIpfw2()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the IPFW2 mechanism to set "
		   "firewall information into the underlying system: %s",
		   error_msg.c_str());
    }
}

int
FirewallSetIpfw2::start(string& error_msg)
{
    uint32_t step;
    size_t step_size = sizeof(step);

    if (_is_running)
	return (XORP_OK);

    //
    // Retrieve the auto-increment step size. This only exists in IPFW2
    // and is used to tell it apart from IPFW1, as well as telling if IPFW1
    // is loaded at runtime.
    //
    if (sysctlbyname("net.inet.ip.fw.autoinc_step", &step, &step_size, NULL, 0)
	< 0) {
	error_msg = c_format("Could not get autoinc value and start "
			     "IPFW2 firewall: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }
    _saved_autoinc_step = step;

    //
    // Open a raw IPv4 socket that IPFW2 uses for communication
    //
    _s4 = socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
    if (_s4 < 0) {
	error_msg = c_format("Could not open a raw socket for IPFW2 firewall: "
			     "%s", strerror(errno));
	return (XORP_ERROR);
    }

    //
    // Set autoinc step to 1, so that XORP rules being pushed can be done
    // with no rule number book-keeping.
    //
    step = 1;
    if (sysctlbyname("net.inet.ip.fw.autoinc_step", NULL, NULL,
		     reinterpret_cast<void *>(&step), step_size)
	< 0) {
	error_msg = c_format("Could not set autoinc value and start "
			     "IPFW2 firewall: %s",
			     strerror(errno));
	comm_close(_s4);
	_s4 = -1;
	return (XORP_ERROR);
    }

    _is_running = true;

    return (XORP_OK);
}

int
FirewallSetIpfw2::stop(string& error_msg)
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

int
FirewallSetIpfw2::add_entry(const FirewallEntry& firewall_entry,
			    string& error_msg)
{
    uint32_t rulebuf[255];
    uint32_t* rulep;

    //
    // Layout of buffer:
    //
    //   +------- start --------+   <- &rulebuf[0]
    //   |   struct ip_fw       |
    //   +---- match opcodes ---+
    //   |   O_VIA              |
    //   |   O_PROTO            |
    //   |   O_IP_SRC_MASK      |
    //   |   O_IP_DST_MASK      |
    //   |   O_IP_SRCPORT       |
    //   |   O_IP_DSTPORT       |
    //   +--- action opcode(s)--+
    //   |   O_ACCEPT or O_DENY |
    //   +----------------------+
    //   |      free space      |
    //   +-------- end ---------+
    //

    //
    // Micro-assemble an IPFW2 instruction sequence
    //
    struct ip_fw* ip_fw = reinterpret_cast<struct ip_fw *>(&rulebuf[0]);
    rulep = &rulebuf[0];
    memset(rulebuf, 0, sizeof(rulebuf));

    ip_fw->rulenum = firewall_entry.rule_number();

    //
    // Advance past the rule structure we instantiate first in vector space
    //
    rulep = reinterpret_cast<uint32_t *>(ip_fw->cmd);

    //
    // Begin adding match opcodes
    //

    //
    // Add the interface match opcode
    //
    // XXX: On this platform, ifname == vifname.
    //
    if (! firewall_entry.vifname().empty()) {
	ipfw_insn_if* ifinsp = reinterpret_cast<ipfw_insn_if *>(rulep);
	ifinsp->o.opcode = O_VIA;
	strlcpy(ifinsp->name, firewall_entry.vifname().c_str(),
		sizeof(ifinsp->name));
	ifinsp->o.len |= F_INSN_SIZE(ipfw_insn_if);
	rulep += F_INSN_SIZE(ipfw_insn_if);
    }

    //
    // Add the protocol match opcode
    //
    if (firewall_entry.ip_protocol() != FirewallEntry::IP_PROTOCOL_ANY) {
	ipfw_insn* pinsp = reinterpret_cast<ipfw_insn *>(rulep);
	pinsp->opcode = O_PROTO;
	pinsp->len |= F_INSN_SIZE(ipfw_insn);
	pinsp->arg1 = firewall_entry.ip_protocol();
	rulep += F_INSN_SIZE(ipfw_insn);
    }

    //
    // Add a match source address and mask instruction (mandatory)
    //
    if (firewall_entry.is_ipv4()) {
	ipfw_insn_ip* srcinsp = reinterpret_cast<ipfw_insn_ip *>(rulep);
	srcinsp->o.opcode = O_IP_SRC_MASK;
	srcinsp->o.len |= F_INSN_SIZE(ipfw_insn_ip);

	firewall_entry.src_network().masked_addr().copy_out(srcinsp->addr);
	uint32_t prefix_len = firewall_entry.src_network().prefix_len();
	IPv4 prefix = IPv4::make_prefix(prefix_len);
	prefix.copy_out(srcinsp->mask);

	rulep += F_INSN_SIZE(ipfw_insn_ip);
    }
    if (firewall_entry.is_ipv6()) {
	ipfw_insn_ip6* srcinsp = reinterpret_cast<ipfw_insn_ip6 *>(rulep);
	srcinsp->o.opcode = O_IP6_SRC_MASK;
	srcinsp->o.len |= F_INSN_SIZE(ipfw_insn_ip6);

	firewall_entry.src_network().masked_addr().copy_out(srcinsp->addr6);
	uint32_t prefix_len = firewall_entry.src_network().prefix_len();
	IPv6 prefix = IPv6::make_prefix(prefix_len);
	prefix.copy_out(srcinsp->mask6);

	rulep += F_INSN_SIZE(ipfw_insn_ip6);
    }

    //
    // Add a match destination address and mask instruction (mandatory)
    //
    if (firewall_entry.is_ipv4()) {
	ipfw_insn_ip* dstinsp = reinterpret_cast<ipfw_insn_ip *>(rulep);
	dstinsp->o.opcode = O_IP_DST_MASK;
	dstinsp->o.len |= F_INSN_SIZE(ipfw_insn_ip);

	firewall_entry.dst_network().masked_addr().copy_out(dstinsp->addr);
	uint32_t prefix_len = firewall_entry.dst_network().prefix_len();
	IPv4 prefix = IPv4::make_prefix(prefix_len);
	prefix.copy_out(dstinsp->mask);

	rulep += F_INSN_SIZE(ipfw_insn_ip);
    }
    if (firewall_entry.is_ipv6()) {
	ipfw_insn_ip6* dstinsp = reinterpret_cast<ipfw_insn_ip6 *>(rulep);
	dstinsp->o.opcode = O_IP6_DST_MASK;
	dstinsp->o.len |= F_INSN_SIZE(ipfw_insn_ip6);

	firewall_entry.dst_network().masked_addr().copy_out(dstinsp->addr6);
	uint32_t prefix_len = firewall_entry.dst_network().prefix_len();
	IPv6 prefix = IPv6::make_prefix(prefix_len);
	prefix.copy_out(dstinsp->mask6);

	rulep += F_INSN_SIZE(ipfw_insn_ip6);
    }

    //
    // Insert port match instructions, if required.
    //
    if ((firewall_entry.ip_protocol() == IPPROTO_TCP)
	|| (firewall_entry.ip_protocol() == IPPROTO_UDP)) {

	// Add source port match instruction
	{
	    ipfw_insn_u16* spinsp = (ipfw_insn_u16 *)rulep;
	    spinsp->o.opcode = O_IP_SRCPORT;
	    spinsp->o.len = F_INSN_SIZE(ipfw_insn_u16);
	    spinsp->ports[0] = firewall_entry.src_port_begin();
	    spinsp->ports[1] = firewall_entry.src_port_end();
	    rulep += F_INSN_SIZE(ipfw_insn_u16);
	}

	// Add destination port match instruction
	{
	    ipfw_insn_u16* dpinsp = (ipfw_insn_u16 *)rulep;
	    dpinsp->o.opcode = O_IP_DSTPORT;
	    dpinsp->o.len = F_INSN_SIZE(ipfw_insn_u16);
	    dpinsp->ports[0] = firewall_entry.dst_port_begin();
	    dpinsp->ports[1] = firewall_entry.dst_port_end();
	    rulep += F_INSN_SIZE(ipfw_insn_u16);
	}
    }

    //
    // Add the appropriate action instruction
    //
    {
	// Fill out offset of action in 32-bit words
	ip_fw->act_ofs = (reinterpret_cast<uint8_t *>(rulep)
			  - reinterpret_cast<uint8_t *>(ip_fw->cmd))
	    / sizeof(uint32_t);

	ipfw_insn* actinsp = reinterpret_cast<ipfw_insn *>(rulep);
	switch (firewall_entry.action()) {
	case FirewallEntry::ACTION_PASS:
	    actinsp->opcode = O_ACCEPT;
	    break;
	case FirewallEntry::ACTION_DROP:
	    actinsp->opcode = O_DENY;
	    break;
	case FirewallEntry::ACTION_REJECT:
	    actinsp->opcode = O_REJECT;
	    break;
	case FirewallEntry::ACTION_ANY:
	case FirewallEntry::ACTION_NONE:
	    actinsp->opcode = O_NOP;
	    break;
	default:
	    XLOG_FATAL("Unknown firewall entry action code: %u",
		       firewall_entry.action());
	    break;
	}
	actinsp->len |= F_INSN_SIZE(ipfw_insn);
	rulep += F_INSN_SIZE(ipfw_insn);
    }

    //
    // Compute and store the number of 32-bit words in ip_fw.cmd_len
    //
    ip_fw->cmd_len = (reinterpret_cast<uint8_t *>(rulep)
		      - reinterpret_cast<uint8_t *>(ip_fw->cmd))
	/ sizeof(uint32_t);

    //
    // Compute the total size, in bytes, of the rule structure
    //
    socklen_t size_used = (reinterpret_cast<uint8_t *>(rulep)
			   - reinterpret_cast<uint8_t *>(&rulebuf[0]));

    //
    // Install the entry
    //
    if (getsockopt(_s4, IPPROTO_IP, IP_FW_ADD, &rulebuf[0], &size_used) < 0) {
	error_msg = c_format("Could not set IPFW2 firewall entry: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FirewallSetIpfw2::delete_entry(const FirewallEntry& firewall_entry,
			       string& error_msg)
{
    uint32_t rulenum = firewall_entry.rule_number();

    //
    // Delete the entry
    //
    if (setsockopt(_s4, IPPROTO_IP, IP_FW_DEL, &rulenum, sizeof(rulenum))
	< 0) {
	error_msg = c_format("Could not delete IPFW2 firewall entry: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FirewallSetIpfw2::set_table4(const list<FirewallEntry>& firewall_entry_list,
			     string& error_msg)
{
    list<FirewallEntry>::const_iterator iter;

    if (delete_all_entries4(error_msg) != XORP_OK)
	return (XORP_ERROR);

    // Add all entries one-by-one
    for (iter = firewall_entry_list.begin();
	 iter != firewall_entry_list.end();
	 ++iter) {
	const FirewallEntry& firewall_entry = *iter;
	if (add_entry(firewall_entry, error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FirewallSetIpfw2::delete_all_entries4(string& error_msg)
{
    //
    // Delete all entries
    //
    if (setsockopt(_s4, IPPROTO_IP, IP_FW_FLUSH, NULL, 0) < 0) {
	error_msg = c_format("Could not delete all IPFW2 firewall entries: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FirewallSetIpfw2::set_table6(const list<FirewallEntry>& firewall_entry_list,
			     string& error_msg)
{
    list<FirewallEntry>::const_iterator iter;

    if (delete_all_entries6(error_msg) != XORP_OK)
	return (XORP_ERROR);

    // Add all entries one-by-one
    for (iter = firewall_entry_list.begin();
	 iter != firewall_entry_list.end();
	 ++iter) {
	const FirewallEntry& firewall_entry = *iter;
	if (add_entry(firewall_entry, error_msg) != XORP_OK)
	    return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
FirewallSetIpfw2::delete_all_entries6(string& error_msg)
{
    //
    // Delete all entries
    //
    if (setsockopt(_s4, IPPROTO_IP, IP_FW_FLUSH, NULL, 0) < 0) {
	error_msg = c_format("Could not delete all IPFW2 firewall entries: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

#endif // HAVE_FIREWALL_IPFW2
