// -*- c-basic-offset: 8; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2004 International Computer Science Institute
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

// $XORP: xorp/fea/firewall_ipfw.cc,v 1.8 2004/09/21 21:31:02 pavlin Exp $

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipvx.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/ipvxnet.hh"

#include <sys/socket.h>
#ifdef HAVE_SYS_SOCKIO_H
#include <sys/sockio.h>
#endif
#include <sys/sysctl.h>

#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/ip.h>
#ifdef HAVE_FIREWALL_IPFW
#include <netinet/ip_fw.h>
#endif
#include <arpa/inet.h>
#include <netinet/tcp.h>

#include "fea/firewall.hh"
#include "fea/firewall_ipfw.hh"

//
// IPFW support
//
// This is for IPFW, *NOT* IPFW2.
// This provider does not support IPv6.
// We define a 'safe' IPFW rule range for XORP to manage;
// whilst IPFW2 can support separate rule tables, IPFW1 cannot.
//
#ifdef HAVE_FIREWALL_IPFW
IpfwFwProvider::IpfwFwProvider(FirewallManager& m)
    throw(InvalidFwProvider)
    : FwProvider(m)
{
	// Attempt to probe for IPFW with an operation which doesn't
	// change any of the state.
	int system_rule_count = IpfwFwProvider::get_ipfw_static_rule_count();
	if (system_rule_count == -1)
		throw InvalidFwProvider();

	// IPFW uses a private raw IPv4 socket for communication. Grab one.
	_s = ::socket(AF_INET, SOCK_RAW, IPPROTO_RAW);
	if (_s == -1)
		throw InvalidFwProvider();

	// Setup rule tag counters.
	_ipfw_next_free_idx = _ipfw_xorp_start_idx = IPFW_FIRST_XORP_IDX;
	_ipfw_xorp_end_idx = IPFW_LAST_XORP_IDX;
}
#endif /* HAVE_FIREWALL_IPFW */

IpfwFwProvider::~IpfwFwProvider()
{
#ifdef HAVE_FIREWALL_IPFW
	// XXX: Should we remove loaded XORP rules from IPFW?

	if (_s != -1)
		::close(_s);
#endif /* HAVE_FIREWALL_IPFW */
}

//
// Retrieve the state of the global 'firewall on/off' switch for
// the IPFW universe. This is done via sysctl.
//
bool
IpfwFwProvider::get_enabled() const
{
#ifdef HAVE_FIREWALL_IPFW
	uint32_t	ipfw_enabled = 0;
	size_t		ipfw_enabled_size = sizeof(ipfw_enabled);

	int	ret = sysctlbyname("net.inet.ip.fw.enable",
	    &ipfw_enabled, &ipfw_enabled_size, NULL, 0);

	// If we could not retrieve the sysctl, then assume IPFW is disabled.
	if (ret == -1)
		return (false);

	return (ipfw_enabled != 0 ? true : false);
#else
	return (false);
#endif /* HAVE_FIREWALL_IPFW */
}

//
// Set the state of the global 'firewall on/off' switch for
// the IPFW universe. This is done via sysctl.
//
int
IpfwFwProvider::set_enabled(bool enabled)
{
#ifdef HAVE_FIREWALL_IPFW
	uint32_t	ipfw_enabled;

	ipfw_enabled = (enabled == true ? 1 : 0);

	int ret = sysctlbyname("net.inet.ip.fw.enable", NULL, 0,
	    &ipfw_enabled, sizeof(ipfw_enabled));

	return (ret == -1 ? XORP_ERROR : XORP_OK);
#else
	UNUSED(enabled);
	return (XORP_ERROR);
#endif /* HAVE_FIREWALL_IPFW */
}

int
IpfwFwProvider::take_table_ownership()
{
	FwTable4::iterator	fi4;
	FwTable4&		ft4 = _m._fwtable4;

	// Take ownership of the IPv4 table by converting all of the
	// FwRules to IpfwFwRules.

	for (fi4 = ft4.begin(); fi4 != ft4.end(); ++fi4) {
		FwRule4* rp4 = *fi4;
		IpfwFwRule4* nrp4 = new IpfwFwRule4(*rp4);
		delete rp4;
		*fi4 = nrp4;
	}

	// TODO: Synchronize in-kernel table with XORP table:
	//  TODO: Remove all pre-existing IPFW rules in the managed range.
	//  TODO: Add all XORP rules to the kernel.

	// Disregard the IPv6 table for the time being.

	return (XORP_OK);
}

//
// IPv4 firewall provider interface
//

int
IpfwFwProvider::add_rule4(FwRule4& rule)
{
#ifdef HAVE_FIREWALL_IPFW
	IpfwFwRule4* prule = dynamic_cast<IpfwFwRule4*>(&rule);

	int ruleno = alloc_ruleno();
	if (ruleno == 0)
		return (XORP_ERROR);

	prule->_index = ruleno;
	//prule->_ip_fw.idx = ruleno;

	// .. rule was converted to ipfw format when table ownership
	// was taken?

	// Attempt to add the converted rule to the kernel.
	socklen_t	i = sizeof(prule->_ipfw);
	int	r = ::getsockopt(_s, IPPROTO_IP, IP_FW_ADD, &prule->_ipfw, &i);

	return (r == -1 ? XORP_ERROR : XORP_OK);
#else
	UNUSED(rule);
	return (XORP_ERROR);
#endif
}

//
// Propagate a delete down to the provider level.
//
int
IpfwFwProvider::delete_rule4(FwRule4& rule)
{
#ifdef HAVE_FIREWALL_IPFW
	IpfwFwRule4* prule = dynamic_cast<IpfwFwRule4*>(&rule);

	// XXX: Mark as deleted. It's the FwManager's responsibility
	// to actually remove it from the XORP table.
	rule.mark_deleted();

#if 0
	// XXX: Check to see if there are any rules after this one
	// in the table. If there are not, we can decrement the
	// next-free marker. If there are, never mind. Whilst this
	// mapping scheme of XORP->IPFW table space is not the
	// smartest, it is very simple to implement.
	if (new_rule._ipfw.fw_number >= _ipfw_next_free_idx)
#endif

	// Actually attempt to remove the rule from the kernel.
	int	ret = ::setsockopt(_s, IPPROTO_IP, IP_FW_DEL, &prule->_ipfw,
	    sizeof(prule->_ipfw));

	return (ret != 0 ? XORP_ERROR : XORP_OK);
#else
	UNUSED(rule);
	return (XORP_ERROR);
#endif
}

//
// Get number of XORP rules actually installed in system tables
//
uint32_t
IpfwFwProvider::get_num_xorp_rules4() const
{
	// XXX: I'm a bit lost right now. How do we go about doing this?
	return (0);
}

//
// Get total number of rules installed in system tables.
//
uint32_t
IpfwFwProvider::get_num_system_rules4() const
{
#ifdef HAVE_FIREWALL_IPFW
	uint32_t ipfw_rulecount =
	    IpfwFwProvider::get_ipfw_static_rule_count();
	return (ipfw_rulecount);
#else
	return (0);
#endif
}

//
// Private helper method: retrieve static rule count from kernel.
//
#ifdef HAVE_FIREWALL_IPFW
int
IpfwFwProvider::get_ipfw_static_rule_count()
{
	uint32_t	ipfw_rulecount = 0;
	uint32_t	ipfw_rulecount_size = sizeof(ipfw_rulecount);

	int	ret = sysctlbyname("net.inet.ip.fw.static_count",
	    &ipfw_rulecount, &ipfw_rulecount_size, NULL, 0);

	// Return -1 on error, otherwise, return the rule count.
	return (ret != -1 ? ipfw_rulecount : ret);
}
#endif

#ifdef HAVE_FIREWALL_IPFW
//
// Allocate an IPFW rule number for a XORP rule which is about to be added.
// Return 0 if unsuccessful. Any non-zero value is a rule index.
//
uint16_t
IpfwFwProvider::alloc_ruleno()
{
	// Check if our slice of the IPFW table is full.
	if (_ipfw_next_free_idx >= _ipfw_xorp_end_idx)
		return (0);

	return (_ipfw_next_free_idx++);
}

// Private helper method: Convert an ifname string to an ip_fw_if
// union embedded in an IPFW rule.
static int
ifname_to_ifu(const string& ifname, union ip_fw_if& ifu)
{
	int	pos = ifname.find_first_of("0123456789");
	if (pos >= FW_IFNLEN)
		return (XORP_ERROR);	// Interface name too long for IPFW.

	// ip_fw_if needs name and unit split.
	string	s_driver = ifname.substr(0, pos);
	string	s_unit = ifname.substr(pos, ifname.length() - pos);

	ifu.fu_via_if.unit = atoi(s_unit.c_str());
	strlcpy(ifu.fu_via_if.name, s_driver.c_str(),
	    sizeof(ifu.fu_via_if.name));	// XXX unchecked

	return (XORP_OK);
}

//
// Deferred templatized function definitions, static, for a function
// which deals with translating FwRule's intermediate representation
// to an IPFW specific representation.
//
// Templatization is necessary because templatized class constructors
// cannot have deferred definitions. Therefore we use an additional
// static function. Generic instantiation makes no sense.
// Therefore we specialize the template for IPv4-IPv4 and IPv6-IPv6
// conversions only.
//

template <>
static void
convert_to_ipfw(IpfwFwRule<IPv4>& new_rule, const FwRule<IPv4>& old_rule)
{
	new_rule = old_rule;	// XXX: Will this do shallow assignment
				// correctly of base class fields?

	// ifname is always specified (as for 'via' ipfw syntax).
	// vifname is silently ignored and discarded.
	union ip_fw_if	ifu;

	ifname_to_ifu(new_rule.ifname(), ifu);

	new_rule._ipfw.fw_out_if = new_rule._ipfw.fw_in_if = ifu;
	new_rule._ipfw.fw_flg |= (IP_FW_F_IIFNAME | IP_FW_F_OIFNAME);

	// Fill out the source/destination network addresses and mask.
	new_rule.src().masked_addr().copy_out(new_rule._ipfw.fw_src);
	new_rule.src().netmask().copy_out(new_rule._ipfw.fw_smsk);
	new_rule.dst().masked_addr().copy_out(new_rule._ipfw.fw_dst);
	new_rule.dst().netmask().copy_out(new_rule._ipfw.fw_dmsk);

	//
	// Fill out the protocol field.
	//
	switch (new_rule.proto()) {
	case FwRule4::IP_PROTO_ANY:
		// IPFW uses IPPROTO_IP as a wildcard.  We use IP_PROTO_ANY.
		new_rule._ipfw.fw_prot = IPPROTO_IP;
		break;

	case IPPROTO_TCP:
	case IPPROTO_UDP:
	{
		//
		// Fill out the source port and destination port.
		//
		// We've already zeroed out the rule structure, so 0 is the
		// wildcard; we need only do this if people specified ports.
		//
		// Both the source and destination port specifications share
		// space in the ipfw rule representation to keep size down,
		// so we have to take heed of this when setting port numbers.
		//
		int	nports = 0;	// Number of ports in the array

		if (new_rule.sport() != FwRule4::PORT_ANY) {
			new_rule._ipfw.fw_uar.fw_pts[nports++] = new_rule.sport();
			IP_FW_SETNSRCP(&new_rule._ipfw, 1);
		}

		if (new_rule.dport() != FwRule4::PORT_ANY) {
			new_rule._ipfw.fw_uar.fw_pts[nports++] = new_rule.dport();
			IP_FW_SETNDSTP(&new_rule._ipfw, 1);
		}

		new_rule._ipfw.fw_prot = new_rule.proto();
		break;
	}

	default:
		new_rule._ipfw.fw_prot = new_rule.proto();
		break;
	}

	//
	// Convert XORP action to IPFW action. IPFW allows multiple
	// actions coalesced into the same rule with certain restrictions.
	// XORP only allows a single action per rule.
	// 'pass' in XORP means 'accept' to IPFW.
	// 'drop' in XORP means 'deny' to IPFW.
	// 'none' in XORP means 'count' to IPFW.
	//
	switch (new_rule.action()) {
	case FwRule4::ACTION_PASS:
		new_rule._ipfw.fw_flg |= IP_FW_F_ACCEPT;
		break;
	case FwRule4::ACTION_DROP:
		new_rule._ipfw.fw_flg |= IP_FW_F_DENY;
		break;
	case FwRule4::ACTION_NONE:
		new_rule._ipfw.fw_flg |= IP_FW_F_COUNT;
		break;
	}
}

template <>
static void
convert_to_ipfw(IpfwFwRule<IPv6>& new_rule, const FwRule<IPv6>& old_rule)
{
	UNUSED(new_rule);
	UNUSED(old_rule);
}

#endif /* HAVE_FIREWALL_IPFW */
