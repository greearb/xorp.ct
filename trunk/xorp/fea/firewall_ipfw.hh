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

// $XORP$

#ifndef	__FEA_FIREWALL_IPFW_HH__
#define __FEA_FIREWALL_IPFW_HH__

#ifdef HAVE_FIREWALL_IPFW
#include <netinet/ip_fw.h>
#endif

#include "fea/firewall.hh"

/****************************************************************************/

class IpfwFwProvider;

template <typename N>
class IpfwFwRule : public FwRule<N> {
	friend class IpfwFwProvider;
#ifdef HAVE_FIREWALL_IPFW
protected:
	uint32_t	_ruleno;
#endif
};

typedef IpfwFwRule<IPv4Net> IpfwFwRule4;
typedef IpfwFwRule<IPv6Net> IpfwFwRule6;
typedef IpfwFwRule<IPvXNet> IpfwFwRuleX;

/****************************************************************************/

/**
 * @short Firewall provider interface.
 *
 * Concrete class defining the interface to IPFW1.
 *
 * This class does NOT support IPV6 filtering at this time.
 *
 * This class does NOT support IPFW2.
 * (Although a derived class could implement IPFW2 support in future.)
 * IPFW2 cannot be reliably told apart from IPFW at compile-time. Whether
 * or not the IPFW or IPFW2 ABI is in use *HAS* to be a user-defined
 * compile-time switch (which makes cross-compilation in *our* build a
 * real headache). Whilst we could spend time devising a heuristic it's
 * really not worth the pain.
 *
 * The port sysutils/ipa (IP Accounting for IPFW) is also worth looking at
 * as a further reference.
 */
class IpfwFwProvider : FwProvider {
public:
	IpfwFwProvider(FirewallManager& m) throw(InvalidFwProvider);
	virtual ~IpfwFwProvider();

	//---------------------------------
	// General provider interface
	//---------------------------------

	bool get_enabled();
	int set_enabled(bool enabled);

	inline const string& get_provider_name() const {
		return ("ipfw");
	}

	inline const string& get_provider_version() const {
		return ("0.1");
	}

	//---------------------------------
	// IPv4 firewall provider interface
	//---------------------------------

	int add_rule4(FwRule& rule);
	int delete_rule4(FwRule& rule);
	uint32_t get_num_xorp_rules4() const;
	uint32_t get_num_system_rules4() const;

	//---------------------------------
	// IPv6 firewall provider interface
	//---------------------------------

	inline int add_rule6(FwRule& rule) {
		UNUSED(rule);
		return (XORP_ERROR);
	}

	inline int delete_rule6(FwRule& rule) {
		UNUSED(rule);
		return (XORP_ERROR);
	}

	inline uint32_t get_num_xorp_rules6() const {
		return (0);
	}

	inline uint32_t get_num_system_rules6() const {
		return (0);
	}

#ifdef HAVE_FIREWALL_IPFW
	// Private but common to IPFW1 and IPFW2.
protected:
	FirewallManager&	_m;	// The firewall manager.
	int	_s;	// Private raw socket for communicating with IPFW.

	// Helper function to retrieve IPFW static rule count.
	// This is a class method; it doesn't per-instance need state.
	static int get_ipfw_static_rule_count() const;

	// Index of the first XORP-managed rule in the global ipfw list.
	static const int IPFW_FIRST_XORP_IDX = 30000;

	// Index of the last XORP-managed rule in the global ipfw list.
	static const int IPFW_LAST_XORP_IDX = 60000;

	// Specific to IPFW1.
private:
	int	_ipfw_next_free_idx;	// Next free rule index in IPFW table.
	int	_ipfw_xorp_start_idx;	// Beginning of XORP-managed range.
	int	_ipfw_xorp_end_idx;	// End of XORP-managed range;

	// Helper function to convert a XORP rule representation into an
	// IPFW one, in preparation for adding it to IPFW's table.
	int xorp_rule4_to_ipfw1(FwRule& rule, struct ip_fw& ipfwrule) const;

	// Helper function to allocate a slot in the IPWF table.
	int alloc_ipfw_rule(struct ip_fw& ipfwrule);

	// Helper function to convert an interface name 'driverNNN' to
	// the ip_fw_if structure which IPFW wants.
	static int ifname_to_ifu(const string& ifname, union ip_fw_if& ifu);
#endif // HAVE_FIREWALL_IPFW
};

/****************************************************************************/

#endif // __FEA_FIREWALL_IPFW_HH__
