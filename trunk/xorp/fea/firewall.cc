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

#ident "$XORP: xorp/fea/firewall.cc,v 1.1 2004/09/14 15:08:22 bms Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipvx.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/ipvxnet.hh"

#include "fea/firewall.hh"
#include "fea/firewall_dummy.hh"
#include "fea/firewall_ipf.hh"
#include "fea/firewall_ipfw.hh"
#include "fea/firewall_iptables.hh"

int
FirewallManager::set_fw_provider(const char* name)
{
	FwProvider* nfwp = NULL;

	if (name == NULL)
		return (XORP_ERROR);

	// I long for a nicer way of doing this. But short of using
	// linker sets, or static initializers, we have no easy
	// way out here.

	try {
		if (strcmp(name, "dummy")) {
			nfwp = new DummyFwProvider(*this);
#ifdef HAVE_FIREWALL_IPF
		} else if (strcmp(name, "ipf")) {
			nfwp = new IpfFwProvider(*this);
#endif
#ifdef HAVE_FIREWALL_IPFW
		} else if (strcmp(name, "ipfw")) {
			nfwp = new IpfwFwProvider(*this);
#endif
#ifdef HAVE_FIREWALL_IPTABLES
		} else if (strcmp(name, "iptables")) {
			nfwp = new IptablesFwProvider(*this);
#endif
		} else {
			return (XORP_ERROR);
		}
	} catch(InvalidFwProvider) {
		return (XORP_ERROR);
	}

	// Switch provider pointers.
	if (_fwp != NULL)
		delete _fwp;
	_fwp = nfwp;

	// Tell the provider to take ownership of the table, so that
	// it can maintain provider-specific state for each firewall
	// rule.
	_fwp->take_table_ownership();

	return (XORP_OK);
}
