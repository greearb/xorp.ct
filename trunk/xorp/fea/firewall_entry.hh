// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2008 XORP, Inc.
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

// $XORP: xorp/fea/firewall_entry.hh,v 1.1 2008/04/26 00:59:42 pavlin Exp $

#ifndef	__FEA_FIREWALL_ENTRY_HH__
#define __FEA_FIREWALL_ENTRY_HH__

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipvx.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/ipvxnet.hh"


/**
 * @short Firewall Table Entry.
 *
 * Representation of a firewall table entry.
 */
class FirewallEntry {
public:
    // Possible actions for firewall rules
    enum Action {
	ACTION_MIN	= 0x00,		// Min value for action
	ACTION_ANY	= 0x00,		// For match comparison during delete
	ACTION_NONE	= 0x01,
	ACTION_PASS	= 0x02,
	ACTION_DROP	= 0x03,
	ACTION_REJECT	= 0x04,
	ACTION_MAX	= 0x05,		// Max number of possible actions
	ACTION_INVALID	= 0xff		// Invalid string conversion
    };

    // Matching values for firewall rules
    enum {
	RULE_NUMBER_DEFAULT	= 0,
	IP_PROTOCOL_MIN		= 0,
	IP_PROTOCOL_MAX		= 255,
	IP_PROTOCOL_ANY		= 0,
	PORT_MIN		= 0,
	PORT_MAX		= 65535,
    };

    explicit FirewallEntry(int family)
	: _rule_number(RULE_NUMBER_DEFAULT), _src_network(family),
	  _dst_network(family), _ip_protocol(IP_PROTOCOL_ANY),
	  _src_port_begin(PORT_MIN), _src_port_end(PORT_MAX),
	  _dst_port_begin(PORT_MIN), _dst_port_end(PORT_MAX),
	  _action(ACTION_INVALID) {}

    FirewallEntry(uint32_t		rule_number,
		  const string&		ifname,
		  const string&		vifname,
		  const IPvXNet&	src_network,
		  const IPvXNet&	dst_network,
		  uint8_t		ip_protocol,
		  uint16_t		src_port_begin,
		  uint16_t		src_port_end,
		  uint16_t		dst_port_begin,
		  uint16_t		dst_port_end,
		  FirewallEntry::Action	action)
	: _rule_number(rule_number), _ifname(ifname), _vifname(vifname),
	  _src_network(src_network), _dst_network(dst_network),
	  _ip_protocol(ip_protocol), _src_port_begin(src_port_begin),
	  _src_port_end(src_port_end), _dst_port_begin(dst_port_begin),
	  _dst_port_end(dst_port_end), _action(action) {}

    /**
     * Test whether this is an IPv4 entry.
     *
     * @return true if this is an IPv4 entry, otherwise false.
     */
    bool is_ipv4() const { return _src_network.is_ipv4(); }

    /**
     * Test whether this is an IPv6 entry.
     *
     * @return true if this is an IPv6 entry, otherwise false.
     */
    bool is_ipv6() const { return _src_network.is_ipv6(); }

    uint32_t rule_number() const	{ return _rule_number; }
    const string& ifname() const	{ return _ifname; }
    const string& vifname() const	{ return _vifname; }
    const IPvXNet& src_network() const	{ return _src_network; }
    const IPvXNet& dst_network() const	{ return _dst_network; }
    uint8_t ip_protocol() const		{ return _ip_protocol; }
    uint32_t src_port_begin() const	{ return _src_port_begin; }
    uint32_t src_port_end() const	{ return _src_port_end; }
    uint32_t dst_port_begin() const	{ return _dst_port_begin; }
    uint32_t dst_port_end() const	{ return _dst_port_end; }
    FirewallEntry::Action action() const { return _action; }

    /**
     * Reset all members.
     */
    void zero() {
	_rule_number = RULE_NUMBER_DEFAULT;
	_ifname.erase();
	_vifname.erase();
	_src_network = IPvXNet(IPvX::ZERO(_src_network.af()), 0);
	_dst_network = IPvXNet(IPvX::ZERO(_dst_network.af()), 0);
	_ip_protocol = IP_PROTOCOL_ANY;
	_src_port_begin = PORT_MIN;
	_src_port_end = PORT_MAX;
	_dst_port_begin = PORT_MIN;
	_dst_port_end = PORT_MAX;
	_action = ACTION_INVALID;
    }

    /**
     * Comparison function for an exact match with the entry.
     *
     * Note that the action is masked off in the comparison, and only
     * the rule-match part of the tuple is evaluated.
     *
     * @return true if the rule-match portion of the entry is matched,
     * otherwise false.
     */
    bool match(const FirewallEntry& other) const {
	return ((_rule_number == other.rule_number())
		&& (_ifname == other.ifname())
		&& (_vifname == other.vifname())
		&& (_src_network == other.src_network())
		&& (_dst_network == other.dst_network())
		&& (_ip_protocol == other.ip_protocol())
		&& (_src_port_begin == other.src_port_begin())
		&& (_src_port_end == other.src_port_end())
		&& (_dst_port_begin == other.dst_port_begin())
		&& (_dst_port_end == other.dst_port_end()));
    }

    /**
     * Convert firewall entry action value to a string representation.
     *
     * @param action the action to convert.
     * @return the string representation of the action value.
     */
    static string action2str(FirewallEntry::Action action);

    /**
     * Convert string representation to a firewall entry action value.
     *
     * @param name the name of the action. It is one of the following
     * keywords: "none", "pass", "drop", "reject".
     * @return the firewall entry action value if the name is valid,
     * otherwise ACTION_INVALID.
     */
    static FirewallEntry::Action str2action(const string& name);

    /**
     * @return a string representation of the entry.
     */
    string str() const {
	return c_format("rule number = %u ifname = %s vifname = %s "
			"source network = %s destination network = %s "
			"IP protocol = %d source port begin = %u "
			"source port end = %u destination port begin = %u "
			"destination port end = %u action = %s",
			_rule_number, _ifname.c_str(), _vifname.c_str(),
			_src_network.str().c_str(),
			_dst_network.str().c_str(),
			_ip_protocol, _src_port_begin, _src_port_end,
			_dst_port_begin, _dst_port_end,
			action2str(_action).c_str());
    }

private:
    uint32_t	_rule_number;		// The rule number
    string	_ifname;		// Interface name
    string	_vifname;		// Virtual interface name
    IPvXNet	_src_network;		// Source network address prefix
    IPvXNet	_dst_network;		// Destination network address prefix
    uint8_t	_ip_protocol;		// IP protocol number: 1-255,
					// or 0 if wildcard
    uint16_t	_src_port_begin;	// Source TCP/UDP begin port: 0-65535
    uint32_t	_src_port_end;		// Source TCP/UDP end port: 0-65535
    uint32_t	_dst_port_begin;	// Dest. TCP/UDP begin port: 0-65535
    uint32_t	_dst_port_end;		// Dest. TCP/UDP end port: 0-65535
    FirewallEntry::Action _action;	// The action
};

#endif	// __FEA_FIREWALL_ENTRY_HH__
