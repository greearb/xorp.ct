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

// $XORP: xorp/fea/firewall_ipf.hh,v 1.4 2004/09/14 17:04:11 bms Exp $

#ifndef __FEA_FIREWALL_IPF_HH__
#define __FEA_FIREWALL_IPF_HH__

#ifdef HAVE_FIREWALL_IPF
// XXX includes
#endif

#include "fea/firewall.hh"

/****************************************************************************/

class IpfFwProvider;

template <typename N>
class IpfFwRule : public FwRule<N> {
	friend class IpfFwProvider;
#ifdef HAVE_FIREWALL_IPF
protected:
	uint32_t	_ruleno;
#endif
};

typedef IpfFwRule<IPv4> IpfFwRule4;
typedef IpfFwRule<IPv6> IpfFwRule6;
typedef IpfFwRule<IPvX> IpfFwRuleX;

/****************************************************************************/

/**
 * @short Firewall provider interface.
 *
 * Concrete class defining the interface to IPF.
 * This class does NOT support IPV6 filtering at this time.
 *
 * XXX: Need to check IPF_VERSION define... what have I written this shim
 * against?
 */
class IpfFwProvider : public FwProvider {
public:
	IpfFwProvider(FirewallManager& m)
	    throw(InvalidFwProvider);

	virtual ~IpfFwProvider();

	//---------------------------------
	// General provider interface
	//---------------------------------

	bool get_enabled() const;
	int set_enabled(bool enabled);

	inline const char* get_provider_name() const {
		return ("ipfw");
	}

	inline const char* get_provider_version() const {
		return ("0.1");
	}

	int take_table_ownership();

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

#ifdef HAVE_FIREWALL_IPF
private:
	string	_providername;
	string	_providerversion;
	string	_ipfname;	// Full pathname of platform IPF device node
	int	_fd;		// Private file descriptor for IPF device
#endif // HAVE_FIREWALL_IPF
};

/****************************************************************************/

#endif // __FEA_FIREWALL_IPF_HH__
