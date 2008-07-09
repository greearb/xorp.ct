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

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif
#ifdef HAVE_NET_PFVAR_H
#include <net/pfvar.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif


#include "fea/firewall_manager.hh"

#include "firewall_get_pf.hh"


//
// Get information about firewall entries from the underlying system.
//
// The mechanism to obtain the information is PF.
//

#ifdef HAVE_FIREWALL_PF

const string FirewallGetPf::_pf_device_name = "/dev/pf";

FirewallGetPf::FirewallGetPf(FeaDataPlaneManager& fea_data_plane_manager)
    : FirewallGet(fea_data_plane_manager),
      _fd(-1)
{
}

FirewallGetPf::~FirewallGetPf()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the PF mechanism to get "
		   "information about firewall entries from the underlying "
		   "system: %s",
		   error_msg.c_str());
    }
}

int
FirewallGetPf::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    //
    // Open the PF device
    //
    _fd = open(_pf_device_name.c_str(), O_RDONLY);
    if (_fd < 0) {
	error_msg = c_format("Could not open device %s for PF firewall: %s",
			     _pf_device_name.c_str(), strerror(errno));
	return (XORP_ERROR);
    }

    _is_running = true;

    return (XORP_OK);
}

int
FirewallGetPf::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (close(_fd) < 0) {
	error_msg = c_format("Could not close device %s for PF firewall: %s",
			     _pf_device_name.c_str(),
			     strerror(errno));
	_fd = -1;
	return (XORP_ERROR);
    }

    _fd = -1;
    _is_running = false;

    return (XORP_OK);
}

int
FirewallGetPf::get_table4(list<FirewallEntry>& firewall_entry_list,
			  string& error_msg)
{
    return (get_table(AF_INET, firewall_entry_list, error_msg));
}

int
FirewallGetPf::get_table6(list<FirewallEntry>& firewall_entry_list,
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
FirewallGetPf::get_table(int family, list<FirewallEntry>& firewall_entry_list,
			 string& error_msg)
{
    struct pfioc_rule pr;
    uint32_t nr, rules_n;

    //
    // Get a ticket and find out how many rules there are
    //
    memset(&pr, 0, sizeof(pr));
    pr.rule.action = PF_PASS;
    if (ioctl(_fd, DIOCGETRULES, &pr) < 0) {
	error_msg = c_format("Could not get the PF firewall table: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }
    rules_n = pr.nr;

    //
    // Get all rules
    //
    for (nr = 0; nr < rules_n; nr++) {
	pr.nr = nr;
	if (ioctl(_fd, DIOCGETRULE, &pr) < 0) {
	    error_msg = c_format("Could not get entry %u from the PF firewall "
				 "table: %s",
				 nr, strerror(errno));
	    return (XORP_ERROR);
	}

	//
	// Ignore entries that don't match the address family
	//
	if (pr.rule.af != family)
	    continue;

	//
	// XXX: Consider only network prefix entries
	//
	if ((pr.rule.src.addr.type != PF_ADDR_ADDRMASK)
	    || (pr.rule.src.addr.type != PF_ADDR_ADDRMASK)) {
	    continue;
	}

	//
	// Extract various information from the rule
	//
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
	bool is_ignored = false;

	//
	// Get the rule number
	//
	// XXX: Do nothing, because PF doesn't have rule numbers

	//
	// Get the action
	//
	switch (pr.rule.action) {
	case PF_PASS:
	    action = FirewallEntry::ACTION_PASS;
	    break;
	case PF_DROP:
	    action = FirewallEntry::ACTION_DROP;
	    break;
	default:
	    XLOG_WARNING("Ingoring firewall entry with action %u: "
			 "unknown action",
			 pr.rule.action);
	    break;
	}
	if (action == FirewallEntry::ACTION_INVALID)
	    continue;

	//
	// Get the protocol
	//
	if (pr.rule.proto != 0)
	    ip_protocol = pr.rule.proto;

	//
	// Get the source and destination network addresses
	//
	IPvX src_addr(family), src_mask(family);
	IPvX dst_addr(family), dst_mask(family);
	src_addr.copy_in(family, pr.rule.src.addr.v.a.addr.addr8);
	src_mask.copy_in(family, pr.rule.src.addr.v.a.mask.addr8);
	dst_addr.copy_in(family, pr.rule.dst.addr.v.a.addr.addr8);
	dst_mask.copy_in(family, pr.rule.dst.addr.v.a.mask.addr8);
	src_network = IPvXNet(src_addr, src_mask.mask_len());
	dst_network = IPvXNet(dst_addr, dst_mask.mask_len());

	//
	// Get the source port number range
	//
	is_ignored = false;
	switch (pr.rule.src.port_op) {
	case PF_OP_NONE:
	    // No-op
	    break;
	case PF_OP_RRG:
	    // Range including boundaries operator: ":"
	    src_port_begin = pr.rule.src.port[0];
	    src_port_end = pr.rule.src.port[1];
	    break;
	case PF_OP_IRG:
	    // Range excluding boundaries operator: "><"
	    src_port_begin = pr.rule.src.port[0] + 1;
	    src_port_end = pr.rule.src.port[1] - 1;
	    break;
	case PF_OP_EQ:
	    // Equal operator: "="
	    src_port_begin = pr.rule.src.port[0];
	    src_port_end = src_port_begin;
	    break;
	case PF_OP_LT:
	    // Less-than operator: "<"
	    src_port_end = pr.rule.src.port[0] - 1;
	    break;
	case PF_OP_LE:
	    // Less-than or equal operator: "<="
	    src_port_end = pr.rule.src.port[0];
	    break;
	case PF_OP_GT:
	    // Greather-than operator: ">"
	    src_port_begin = pr.rule.src.port[0] + 1;
	    break;
	case PF_OP_GE:
	    // Greather-than or equal operator: ">="
	    src_port_begin = pr.rule.src.port[0];
	    break;
	case PF_OP_NE:
	    // Unequal operator: "!="
	    // FALLTHROUGH
	case PF_OP_XRG:
	    // Except range: <>
	    // FALLTHROUGH
	default:
	    XLOG_WARNING("Ingoring firewall entry with source port range "
			 "operator %u: unknown operator",
			 pr.rule.src.port_op);
	    is_ignored = true;
	    break;
	}
	if (is_ignored)
	    continue;

	//
	// Get the destination port number range
	//
	is_ignored = false;
	switch (pr.rule.dst.port_op) {
	case PF_OP_NONE:
	    // No-op
	    break;
	case PF_OP_RRG:
	    // Range including boundaries operator: ":"
	    dst_port_begin = pr.rule.dst.port[0];
	    dst_port_end = pr.rule.dst.port[1];
	    break;
	case PF_OP_IRG:
	    // Range excluding boundaries operator: "><"
	    dst_port_begin = pr.rule.dst.port[0] + 1;
	    dst_port_end = pr.rule.dst.port[1] - 1;
	    break;
	case PF_OP_EQ:
	    // Equal operator: "="
	    dst_port_begin = pr.rule.dst.port[0];
	    dst_port_end = dst_port_begin;
	    break;
	case PF_OP_LT:
	    // Less-than operator: "<"
	    dst_port_end = pr.rule.dst.port[0] - 1;
	    break;
	case PF_OP_LE:
	    // Less-than or equal operator: "<="
	    dst_port_end = pr.rule.dst.port[0];
	    break;
	case PF_OP_GT:
	    // Greather-than operator: ">"
	    dst_port_begin = pr.rule.dst.port[0] + 1;
	    break;
	case PF_OP_GE:
	    // Greather-than or equal operator: ">="
	    dst_port_begin = pr.rule.dst.port[0];
	    break;
	case PF_OP_NE:
	    // Unequal operator: "!="
	    // FALLTHROUGH
	case PF_OP_XRG:
	    // Except range: <>
	    // FALLTHROUGH
	default:
	    XLOG_WARNING("Ingoring firewall entry with destination port range "
			 "operator %u: unknown operator",
			 pr.rule.dst.port_op);
	    is_ignored = true;
	    break;
	}
	if (is_ignored)
	    continue;

	//
	// The interface and vif
	//
	if (pr.rule.ifname[0] != '\0') {
	    ifname = string(pr.rule.ifname);
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

#endif // HAVE_FIREWALL_PF
