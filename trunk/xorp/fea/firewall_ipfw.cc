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

// $XORP: xorp/fea/firewall_ipfw.cc,v 1.2 2004/09/03 20:52:30 bms Exp $

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipvx.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/ipvxnet.hh"

#include <sys/socket.h>
#include <sys/sockio.h>
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

IpfwFwProvider::IpfwFwProvider(FirewallManager& m)
    throw(InvalidFwProvider)
    : FwProvider(m)
{
#ifdef HAVE_FIREWALL_IPFW
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
#endif /* HAVE_FIREWALL_IPFW */
}

IpfwFwProvider::~IpfwFwProvider()
{
#ifdef HAVE_FIREWALL_IPFW
	// XXX: Should we get rid of XORP rules on shutdown?

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

	// If we could not retrieve the sysctl, then assume ipfw is disabled.
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
	return (XORP_ERROR);
#endif /* HAVE_FIREWALL_IPFW */
}

//
// IPv4 firewall provider interface
//

int
IpfwFwProvider::add_rule4(FwRule4& rule)
{
#ifdef HAVE_FIREWALL_IPFW
	// Map provider-private rule tag to an IPFW rule number.
	// XXX: If we can't cast to our underlying derived representation,
	// then this cast will throw an exception.
	IpfwFwRule4* prule = dynamic_cast<IpfwFwRule4*>(&rule);
	UNUSED(prule);

	struct ip_fw	ipfwrule;
	memset(&ipfwrule, 0, sizeof(struct ip_fw));

	if (alloc_ipfw_rule(ipfwrule) != XORP_OK)
		return (XORP_ERROR);

	// Cache the assigned IPFW rule number in the derived FwRule.
	//prule->set_index(ipfwrule.fw_number);

	// Convert XORP intermediate representation to IPFW representation.
	xorp_rule4_to_ipfw1(rule, ipfwrule);

	// It's the FwManager's responsibility to add this rule to
	// any table it's meant to belong to; it may already have
	// been added when we were called.

	// Attempt to add the converted rule to the kernel.
	socklen_t	i = sizeof(ipfwrule);
	int	r = ::getsockopt(_s, IPPROTO_IP, IP_FW_ADD, &ipfwrule, &i);

	return (r == -1 ? XORP_ERROR : XORP_OK);
#else
	return (XORP_ERROR);
#endif
}

int
IpfwFwProvider::delete_rule4(FwRule4& rule)
{
#ifdef HAVE_FIREWALL_IPFW
	struct ip_fw	ipfwrule;
	memset(&ipfwrule, 0, sizeof(ipfwrule));

#if 0
	// Map provider-private rule tag to an IPFW rule number.
	// XXX: If we can't cast to our underlying derived representation,
	// then this cast will throw an exception.
	IpfwFwRule4* prule = dynamic_cast<IpfwFwRule4*>(&rule);
	//ipfwrule.fw_number = prule->get_index();
#endif

	// XXX: Mark as deleted. It's the FwManager's responsibility
	// to actually remove it from the XORP table.
	rule.mark_deleted();

#if 0
	// XXX: Check to see if there are any rules after this one
	// in the table. If there are not, we can decrement the
	// next-free marker. If there are, never mind. Whilst this
	// mapping scheme of XORP->IPFW table space is not the
	// smartest, it is very simple to implement.
	if (ipfwrule.fw_number >= _ipfw_next_free_idx)
#endif

	// Actually attempt to remove the rule from the kernel.
	int	ret = ::setsockopt(_s, IPPROTO_IP, IP_FW_DEL, &ipfwrule,
	    sizeof(ipfwrule));

	return (ret != 0 ? XORP_ERROR : XORP_OK);
#else
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
int
IpfwFwProvider::get_ipfw_static_rule_count()
{
#ifdef HAVE_FIREWALL_IPFW
	uint32_t	ipfw_rulecount = 0;
	uint32_t	ipfw_rulecount_size = sizeof(ipfw_rulecount);

	int	ret = sysctlbyname("net.inet.ip.fw.static_count",
	    &ipfw_rulecount, &ipfw_rulecount_size, NULL, 0);

	// Return -1 on error, otherwise, return the rule count.
	return (ret != -1 ? ipfw_rulecount : ret);
#else
	return (0);
#endif
}

#ifdef HAVE_FIREWALL_IPFW
//
// Private helper method: map the representation of a XORP firewall rule
// into an IPFW firewall rule representation before the kernel sees it.
//
// This requires that all members except rule_number are zeroed first
// or else the result may be undefined.
//
int
IpfwFwProvider::xorp_rule4_to_ipfw1(FwRule4& rule,
    struct ip_fw& ipfwrule) const
{
	FwRule4* prule = &rule;		// XXX!

	// ifname is always specified (as for 'via' ipfw syntax).
	// vifname is silently ignored and discarded.
	union ip_fw_if	ifu;

	IpfwFwProvider::ifname_to_ifu(prule->ifname(), ifu);
	ipfwrule.fw_out_if = ipfwrule.fw_in_if = ifu;
	ipfwrule.fw_flg |= (IP_FW_F_IIFNAME | IP_FW_F_OIFNAME);

	// Fill out the source/destination network addresses and mask.
	prule->src().masked_addr().copy_out(ipfwrule.fw_src);
	prule->src().netmask().copy_out(ipfwrule.fw_smsk);
	prule->dst().masked_addr().copy_out(ipfwrule.fw_dst);
	prule->dst().netmask().copy_out(ipfwrule.fw_dmsk);

	//
	// Fill out the protocol field.
	//
	switch (prule->proto()) {
	case FwRule4::IP_PROTO_ANY:
		// IPFW uses IPPROTO_IP as a wildcard.  We use IP_PROTO_ANY.
		ipfwrule.fw_prot = IPPROTO_IP;
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

		if (prule->sport() != FwRule4::PORT_ANY) {
			ipfwrule.fw_uar.fw_pts[nports++] = prule->sport();
			IP_FW_SETNSRCP(&ipfwrule, 1);
		}

		if (prule->dport() != FwRule4::PORT_ANY) {
			ipfwrule.fw_uar.fw_pts[nports++] = prule->dport();
			IP_FW_SETNDSTP(&ipfwrule, 1);
		}

		ipfwrule.fw_prot = prule->proto();
		break;
	}

	default:
		ipfwrule.fw_prot = prule->proto();
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
	switch (prule->action()) {
	case FwRule4::ACTION_PASS:
		ipfwrule.fw_flg |= IP_FW_F_ACCEPT;
		break;
	case FwRule4::ACTION_DROP:
		ipfwrule.fw_flg |= IP_FW_F_DENY;
		break;
	case FwRule4::ACTION_NONE:
		ipfwrule.fw_flg |= IP_FW_F_COUNT;
		break;
	default:
		return (XORP_ERROR);
	}

	return (XORP_OK);
}

//
// Allocate an IPFW rule number for a XORP rule which is about to be added.
// The ipfwrule.fw_number member will be set accordingly.
//
int
IpfwFwProvider::alloc_ipfw_rule(struct ip_fw& ipfwrule)
{
	// Check if our slice of the IPFW table is full.
	if (_ipfw_next_free_idx >= _ipfw_xorp_end_idx)
		return (XORP_ERROR);

	ipfwrule.fw_number = _ipfw_next_free_idx++;

	return (XORP_OK);
}

int
IpfwFwProvider::ifname_to_ifu(const string& ifname, union ip_fw_if& ifu)
{
	int	pos = ifname.find_first_of("0123456789");
	if (pos >= FW_IFNLEN)
		return (XORP_ERROR);	// Interface name too long for IPFW.

	// ip_fw_if needs name and unit split.
	string	s_driver = ifname.substr(0, pos);
	string	s_unit = ifname.substr(pos, ifname.length() - pos);

	ifu.fu_via_if.unit = atoi(s_unit.c_str());
	strlcpy(ifu.fu_via_if.name, s_driver.c_str(),
	    sizeof(ifu.fu_via_if.name)); // XXX unchecked

	return (XORP_OK);
}
#endif /* HAVE_FIREWALL_IPFW */
