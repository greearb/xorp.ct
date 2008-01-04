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

// $XORP: xorp/fea/pa_entry.hh,v 1.5 2007/05/23 12:12:34 pavlin Exp $

#ifndef __FEA_PA_ENTRY_HH__
#define __FEA_PA_ENTRY_HH__

#include "libxorp/xorp.h"
#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipvx.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/ipvxnet.hh"

/* ------------------------------------------------------------------------- */

/**
 * @short Packet filter action.
 *
 * Enumeration representing possible actions for XORP packet filtering rules.
 */
enum PaAction {
    ACTION_ANY = 0x00,		// Used for match comparison during delete.
    ACTION_NONE = 0x01,
    ACTION_PASS = 0x02,
    ACTION_DROP = 0x03,
    MAX_ACTIONS = 4,		// Number of possible actions
    ACTION_INVALID = 0xFF	// Invalid string conversion.
};

// Exceptions which can be thrown during PaAction construction.
class PaInvalidActionException {};

/**
 * @short Packet filter action.
 *
 * Wrapper class for PaAction offering string conversion functions.
 */
class PaActionWrapper {
public:
    /* Class methods */
    static PaAction convert(const string& act_str);
    static string convert(const PaAction act);
#ifdef notyet
    /*
     * Object methods; only really needed if we intend to initialize
     * PaActionWrapper objects within another class's constructor.
     */
public:
    PaActionWrapper(PaAction act) : _action(act) {}

    PaActionWrapper(const string& act_str) throw(PaInvalidActionException) {
	_action = convert(act_str);
	if (_action == PaAction(ACTION_INVALID))
	    throw PaInvalidActionException();
    };

    /**
     * @return A string representation of the action.
     */
    string str() const { return convert(_action); }
    PaAction action() const { return _action; }

protected:
    PaAction	_action;
#endif
};

/* ------------------------------------------------------------------------- */

/**
 * @short Packet filter entry.
 *
 * Intermediate representation of a XORP packet filter entry.
 */
template <typename N>
class PaEntry {
public:
    enum { IP_PROTO_ANY = 255, PORT_ANY = 0 };

    PaEntry() { zero(); }

    PaEntry(const string&	ifname,
	    const string&	vifname,
	    const IPNet<N>&	src,
	    const IPNet<N>&	dst,
	    uint8_t		proto,
	    uint16_t		sport,
	    uint16_t		dport,
	    PaAction		action)
    : _ifname(ifname), _vifname(vifname), _src(src), _dst(dst),
    _proto(proto), _sport(sport), _dport(dport), _action(action)
    {}

    virtual ~PaEntry() {}

    const string&	ifname() const		{ return _ifname; }
    const string&	vifname() const		{ return _vifname; }
    const IPNet<N>&	src() const		{ return _src; }
    const IPNet<N>&	dst() const		{ return _dst; }
    uint8_t		proto() const		{ return _proto; }
    uint16_t		sport() const		{ return _sport; }
    uint16_t		dport() const		{ return _dport; }
    PaAction		action() const		{ return _action; }

    /**
     * Reset all members.
     */
    void zero() {
	_ifname.erase();
	_vifname.erase();
	_src = IPNet<N>(N::ZERO(_src.af()), 0);
	_dst = IPNet<N>(N::ZERO(_dst.af()), 0);
	_proto = IP_PROTO_ANY;
	_sport = PORT_ANY;
	_dport = PORT_ANY;
	_action = PaAction(ACTION_NONE);
    }

    /*
     * Comparison function for an exact match with the entry.
     * Masks off the action; only the rule-match part of the tuple
     * is evaluated.
     *
     * @return true if the rule-match portion of the PaEntry is matched.
     */
    bool match(const PaEntry<N>& e) const {
	return (_ifname == e.ifname() &&
		_vifname == e.vifname() &&
		_src == e.src() &&
		_dst == e.dst() &&
		_proto == e.proto() &&
		_sport == e.sport() &&
		_dport == e.dport());
    }

    /**
     * @return A string representation of the entry.
     *
     * Example:
     * src = 127.0.0.1/32 dst = 0.0.0.0/0 ifname = fxp0 proto = 255
     * sport = 0 dport = 0 action = drop
     */
     string str() const {
	return (c_format("ifname = %s vifname = %s src = %s dst = %s"
	"proto = %u sport = %u dport = %u action = %s",
	_ifname.c_str(), _vifname.c_str(),
	_src.str().c_str(), _dst.str().c_str(),
	_proto, _sport, _dport,
	PaActionWrapper::convert(_action).c_str()));
     }

protected:
    string	_ifname;	// Interface where applied
    string	_vifname;	// Interface where applied
    IPNet<N>	_src;		// Source address (with mask)
    IPNet<N>	_dst;		// Destination address (with mask)
    uint8_t	_proto;		// Protocol to match, or IP_PROTO_ANY
    uint16_t	_sport;		// Source port match, or PORT_ANY
    uint16_t	_dport;		// Destination port match, or PORT_ANY
    PaAction	_action;	// Action to take on match
};

typedef PaEntry<IPv4> PaEntry4;
typedef PaEntry<IPv6> PaEntry6;
typedef PaEntry<IPvX> PaEntryX;

/* ------------------------------------------------------------------------- */
#endif // __FEA_PA_ENTRY_HH__
