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

// $XORP: xorp/fea/firewall_iptables.hh,v 1.4 2004/09/14 21:53:43 bms Exp $

#ifndef	__FEA_FIREWALL_IPTABLES_HH__
#define	__FEA_FIREWALL_IPTABLES_HH__

#ifdef HAVE_FIREWALL_IPTABLES
#include "libiptc/libiptc.h"
#include "iptables.h"
#endif

#include "fea/firewall.hh"

/****************************************************************************/

class IptablesFwProvider;

template <typename N>
class IptablesFwRule : public FwRule<N> {
	friend class IptablesFwProvider;
protected:
#ifdef HAVE_FIREWALL_IPTABLES
	iptc_entry_t	_entry;		// libiptc rule entry handle.
#endif
};

typedef IptablesFwRule<IPv4Net> IptablesFwRule4;
typedef IptablesFwRule<IPv6Net> IptablesFwRule6;
typedef IptablesFwRule<IPvXNet> IptablesFwRuleX;

/****************************************************************************/

/**
 * @short Firewall provider interface.
 *
 * Concrete class defining the interface to IPTABLES; using libiptc as a shim.
 * This class does NOT support IPV6 filtering at this time.
 */
class IptablesFwProvider : public FwProvider {
public:
	IptablesFwProvider(FirewallManager& m)
	    throw(InvalidFwProvider);

	virtual ~IptablesFwProvider();

	//---------------------------------
	// General provider interface
	//---------------------------------

	bool get_enabled() const;
	int set_enabled(bool enabled);

	inline const char* get_provider_name() const {
		return ("iptables");
	}

	inline const char* get_provider_version() const {
		return ("0.1");
	}

	//---------------------------------
	// IPv4 firewall provider interface
	//---------------------------------

	int add_rule4(FwRule4& rule);
	int delete_rule4(FwRule4& rule);

	uint32_t get_num_xorp_rules4() const;
	uint32_t get_num_system_rules4() const;

	//---------------------------------
	// IPv6 firewall provider interface
	//---------------------------------

	inline int add_rule6(FwRule6& rule) {
		UNUSED(rule);
		return (XORP_ERROR);
	}

	inline int delete_rule6(FwRule6& rule) {
		UNUSED(rule);
		return (XORP_ERROR);
	}

	inline uint32_t get_num_xorp_rules6() const {
		return (0);
	}

	inline uint32_t get_num_system_rules6() const {
		return (0);
	}

protected:
#ifdef HAVE_FIREWALL_IPTABLES
	string		_tablename;	// Name of the XORP-managed table.
	iptc_handle_t	_handle;	// Handle to libiptc library.
#endif
};

#endif // __FEA_FIREWALL_IPTABLES_HH__
