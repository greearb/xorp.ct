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

// $XORP: xorp/fea/firewall.hh,v 1.7 2004/09/14 16:47:26 bms Exp $

#ifndef __FEA_FIREWALL_HH__
#define __FEA_FIREWALL_HH__

#include "libxorp/xorp.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipvx.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/ipvxnet.hh"

#include <vector>

/**************************************************************************/

/**
 * @short Firewall rule.
 *
 * Intermediate representation of a XORP firewall rule entry.
 */
template<typename N>
class FwRule {
public:
	static const uint8_t	IP_PROTO_ANY = 255;
	static const uint16_t	PORT_ANY = 0;
	typedef enum Action	{ ACTION_NONE = 0, ACTION_PASS, ACTION_DROP };

	FwRule() { zero(); }
	FwRule(const string&	ifname,
	    const string&	vifname,
	    const N&		src,
	    const N&		dst,
	    uint8_t		proto,
	    uint16_t		sport,
	    uint16_t		dport,
	    Action		action)
	    : _ifname(ifname), _src(src), _dst(dst), _proto(proto),
	      _sport(sport), _dport(dport), _action(action),
	      _is_deleted(false) {}

	virtual ~FwRule() {}

	// accessors
	const string&	ifname() const		{ return _ifname; }
	const string&	vifname() const		{ return _vifname; }
	const N&	src() const		{ return _src; }
	const N&	dst() const		{ return _dst; }
	uint8_t		proto() const		{ return _proto; }
	uint16_t	sport() const		{ return _sport; }
	uint16_t	dport() const		{ return _dport; }
	Action		action() const		{ return _action; }
	bool		is_deleted() const	{ return _is_deleted; }

	// mutators
	void		mark_deleted()		{ _is_deleted = true; }

	/**
	 * Reset all members.
	 */
	void zero() {
		_ifname.erase();
		_src = N(N::ZERO(_src.af()), 0);
		_dst = N(N::ZERO(_dst.af()), 0);
		_proto = IP_PROTO_ANY;
		_sport = PORT_ANY;
		_dport = PORT_ANY;
		_action = ACTION_NONE;
		_is_deleted = false;
	}

	// helper functions

	/**
	 * @return A string representation of the entry.
	 *
	 * Example:
	 * src = 127.0.0.1/32 dst = 0.0.0.0/0 ifname = fxp0 proto = 255
	 * sport = 0 dport = 0 action = drop is_deleted = false
	 */
	string str() const {
		return (c_format("ifname = %s vifname = %s src = %s dst = %s"
		    "proto = %u sport = %u dport = %u action = %s "
		    "is_deleted = %s",
		    _ifname, _vifname, _src.str().c_str(), _dst.str().c_str(),
		    _proto, _sport, _dport, action_to_str(_action),
		    _is_deleted ? "true" : "false"));
	}

	/* XXX: Firewall action as enum? */
	static inline const string& action_to_str(const Action a) {
		switch (a) {
		case ACTION_PASS:
			return ("pass");
			break;
		case ACTION_DROP:
			return ("drop");
			break;
		}
		return ("none");
	}

	// Forbid explicit copy-by-assignment, by declaring a private
	// copy constructor.
protected:
	FwRule(const FwRule& rule) {};

protected:
	string		_ifname;	// Interface where applied
	string		_vifname;	// Interface where applied
	N		_src;		// Source address (with mask)
	N		_dst;		// Destination address (with mask)
	uint8_t		_proto;		// Protocol to match, or IP_PROTO_ANY
	uint16_t	_sport;		// Source port match, or PORT_ANY
	uint16_t	_dport;		// Destination port match, or PORT_ANY
	Action		_action;	// Action to take on match
	bool		_is_deleted;	// True if the entry was deleted
};

typedef FwRule<IPv4Net> FwRule4;
typedef FwRule<IPv6Net> FwRule6;
typedef FwRule<IPvXNet> FwRuleX;

/**************************************************************************/

typedef vector<FwRule4*> FwTable4;
typedef vector<FwRule6*> FwTable6;

class FwProvider;
class XrlFirewallTarget;

/**
 * @short Firewall manager.
 *
 * Concrete class which manages XORP's firewall abstraction.
 */
class FirewallManager {
	friend class FwProvider;
	friend class XrlFirewallTarget;
public:
	FirewallManager() : _fwp(0) {}
	virtual ~FirewallManager() {}

private:
	// Underlying firewall provider interface in use, or NULL
	// if one hasn't been set up yet.
	FwProvider*	_fwp;

	// XORP's idea of the firewall rule tables. There are two
	// separate tables; one for the IPv4 family, and one for
	// the IPv6 family.
	FwTable4	_fwtable4;
	FwTable6	_fwtable6;

private:
	// Attempt to create a new instance of a firewall provider,
	// and migrate all rules to this provider at runtime.
	int set_fw_provider(const char* name);
};

/**************************************************************************/

// Exceptions which can be thrown by FwProvider.
class InvalidFwProvider {};
class InvalidFwProviderCmd {};

/**
 * @short Firewall provider interface.
 *
 * Abstract class defining the interface to a platform firewall provider.
 */
class FwProvider {
public:
	FwProvider(FirewallManager& m)
	    throw(InvalidFwProvider)
	    : _m(m) { throw InvalidFwProvider(); }
	virtual ~FwProvider() {};

	/* General provider interface */
	virtual bool get_enabled() const = 0;
	virtual int set_enabled(bool enabled) = 0;
	virtual const char* get_provider_name() const = 0;
	virtual const char* get_provider_version() const = 0;
	virtual int take_table_ownership() = 0;

	/* IPv4 firewall provider interface */
	virtual int add_rule4(FwRule4& rule) = 0;
	virtual int delete_rule4(FwRule4& rule) = 0;
	virtual uint32_t get_num_xorp_rules4() const = 0;
	virtual uint32_t get_num_system_rules4() const = 0;

	/* IPv6 firewall provider interface */
	virtual int add_rule6(FwRule6& rule) = 0;
	virtual int delete_rule6(FwRule6& rule) = 0;
	virtual uint32_t get_num_xorp_rules6() const = 0;
	virtual uint32_t get_num_system_rules6() const = 0;

protected:
	FirewallManager&	_m;	// Back-reference to XRL target and
					// rule tables.
};

/**************************************************************************/

#endif // __FEA_FIREWALL_HH__
