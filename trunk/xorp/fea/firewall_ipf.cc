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

// $XORP: xorp/fea/firewall_ipf.cc,v 1.2 2004/09/03 20:52:30 bms Exp $

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipvx.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/ipvxnet.hh"

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <sys/time.h>

#include <netinet/in.h>
#include <netinet/in_systm.h>
#include <net/if.h>
#ifdef HAVE_NET_IF_VAR_H
#include <net/if_var.h>
#endif
#include <netinet/ip.h>
#ifdef HAVE_FIREWALL_IPF
#include <netinet/ip_compat.h>
#include <netinet/ip_fil.h>
#endif

#include <fcntl.h>
#include <unistd.h>

#include "fea/firewall.hh"
#include "fea/firewall_ipf.hh"

#ifdef HAVE_FIREWALL_IPF
static int xorp_rule4_to_ipf(IpfFwRule4* prule, frentry_t& ipfrule);
#endif

/***************************************************************************/

//
// IPF support (IPFILTER by Darren Reed).
//
// This provider does not support IPv6 at this time.
// This provider is intended to be cross-platform.
//

IpfFwProvider::IpfFwProvider(FirewallManager& m)
    throw(InvalidFwProvider)
    : FwProvider(m), _ipfname(IPL_NAME)
{
#ifdef HAVE_FIREWALL_IPF
	//
	// Probe for IPF by attempting to open the platform's
	// default IPF device.
	//
	_fd = ::open(_ipfname.c_str(), O_RDWR);
	if (_fd == -1)
		throw InvalidFwProvider();

	// Attempt to retrieve global flags; if we can't, then
	// clearly something is wrong.
	int	i;
	if (::ioctl(_fd, SIOCGETFF, &i) == -1)
		throw InvalidFwProvider();
#else
	throw InvalidFwProvider();
#endif // HAVE_FIREWALL_IPF
}

IpfFwProvider::~IpfFwProvider()
{
#ifdef HAVE_FIREWALL_IPF
	// XXX: Should we get rid of XORP rules on shutdown?

	if (_fd != -1)
		::close(_fd);
#endif // HAVE_FIREWALL_IPF
}

//
// Retrieve the state of the global 'firewall on/off' switch for
// the IPF universe.
//
bool
IpfFwProvider::get_enabled() const
{
#ifdef HAVE_FIREWALL_IPF
	struct friostat fio;
	memset(&fio, 0, sizeof(fio));

	// Assume not running if the ioctl call fails.
	if (::ioctl(_fd, SIOCGETFS, &fio) == -1)
		return (false);

	return (fio.f_running == 1 ? true : false);
#else
	return (false);
#endif // HAVE_FIREWALL_IPF
}

//
// Set the state of the global 'firewall on/off' switch for
// the IPF universe.
//
int
IpfFwProvider::set_enabled(bool enabled)
{
#ifdef HAVE_FIREWALL_IPF
	uint32_t i_enabled = (enabled == true ? 1 : 0);

	if (::ioctl(_fd, SIOCFRENB, &i_enabled) == -1)
		return (XORP_ERROR);

	return (XORP_OK);
#else
	return (XORP_ERROR);
#endif // HAVE_FIREWALL_IPF
}

//
// IPv4 firewall provider interface
//

int
IpfFwProvider::add_rule4(FwRule4& rule)
{
#ifdef HAVE_FIREWALL_IPF
	frentry_t ipfrule;
	memset(&ipfrule, 0, sizeof(ipfrule));

	// ... allocate a rule tag number and fix it to the XORP rule...

	// XXX: what about a managed range hack?

	// Convert from intermediate representation to IPF rule format
	xorp_rule4_to_ipf(dynamic_cast<IpfFwRule4*>(&rule), ipfrule);

	// Add it to ipf's in-kernel list; SIOCADDFR actually wants a
	// pointer-to-pointer-to-frentry.
	frentry_t* pipfrule = &ipfrule;
	if (::ioctl(_fd, SIOCADDFR, &pipfrule) == -1)
		return (XORP_ERROR);

	return (XORP_OK);
#else
	return (XORP_ERROR);
#endif // HAVE_FIREWALL_IPF
}

int
IpfFwProvider::delete_rule4(FwRule4& rule)
{
#ifdef HAVE_FIREWALL_IPF
	frentry_t ipfrule;
	memset(&ipfrule, 0, sizeof(ipfrule));

	// XXX ... what does ipf expect here?
	UNUSED(rule);

	if (::ioctl(_fd, SIOCDELFR, &ipfrule) == -1)
		return (XORP_ERROR);

	return (XORP_OK);
#else
	return (XORP_ERROR);
#endif // HAVE_FIREWALL_IPF
}

/***************************************************************************/

#ifdef HAVE_FIREWALL_IPF

// Assume ipfrule has been zeroed first (or else results are undefined)
static int
xorp_rule4_to_ipf(IpfFwRule4* prule, frentry_t& ipfrule)
{
	// Match IPv4 only.
	ipfrule.fr_ip.fi_v = 4;
	ipfrule.fr_mip.fi_v = 0xF;

	// ifname is copied, vifname is discarded.
	// XXX: is strlcpy() available in the build environment?
	(void)::strlcpy(ipfrule.fr_ifname, prule->ifname().c_str(), IFNAMSIZ);

	// XXX: ipf *requires* that a rule be tied to *either* the input
	// or the output side of an interface. Therefore in order to
	// preserve XORP's illusion of rules matching both 'directions'
	// of packet flow, we'd need to create two rules. For now,
	// only attach to the input queue.
	ipfrule.fr_flags |= FR_INQUE;

	// Fill out the source/destination network addresses and mask.
	prule->src().masked_addr().copy_out(ipfrule.fr_ip.fi_src.in4);
	prule->src().netmask().copy_out(ipfrule.fr_mip.fi_src.in4);
	prule->dst().masked_addr().copy_out(ipfrule.fr_ip.fi_dst.in4);
	prule->dst().netmask().copy_out(ipfrule.fr_mip.fi_dst.in4);

	// Protocol. Wildcard processing is done by masking.
	if (prule->proto() == FwRule4::IP_PROTO_ANY) {
		ipfrule.fr_proto = 0x0;
		ipfrule.fr_mip.fi_p = 0x0;	// Wildcard: mask off all bits
	} else {
		ipfrule.fr_proto = prule->proto();
		ipfrule.fr_mip.fi_p = 0xFF;	// Match proto field exactly
	}

	// Port.
	if (prule->proto() == IPPROTO_TCP || prule->proto() == IPPROTO_UDP) {
		if (prule->sport() == FwRule4::PORT_ANY) {
			ipfrule.fr_scmp = FR_NONE;
		} else {
			ipfrule.fr_scmp = FR_EQUAL;
			ipfrule.fr_sport = prule->sport();
		}

		if (prule->dport() == FwRule4::PORT_ANY) {
			ipfrule.fr_dcmp = FR_NONE;
		} else {
			ipfrule.fr_dcmp = FR_EQUAL;
			ipfrule.fr_dport = prule->dport();
		}
	}

	// Action.
	switch (prule->action()) {
	case FwRule4::ACTION_PASS:
		ipfrule.fr_flags |= FR_PASS;
		break;
	case FwRule4::ACTION_DROP:
		ipfrule.fr_flags |= FR_BLOCK;
		break;
	case FwRule4::ACTION_NONE:
		ipfrule.fr_flags |= FR_LOG;
		break;
	default:
		return (XORP_ERROR);
	}

	return (XORP_OK);
}

#endif // HAVE_FIREWALL_IPF
