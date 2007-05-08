// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

// $XORP: xorp/libfeaclient/ifmgr_atoms.hh,v 1.28 2007/04/23 23:05:09 pavlin Exp $

#ifndef __LIBFEACLIENT_IFMGR_ATOMS_HH__
#define __LIBFEACLIENT_IFMGR_ATOMS_HH__

#include "libxorp/xorp.h"

#include <map>
#include <string>

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/ipvx.hh"
#include "libxorp/ipv4net.hh"
#include "libxorp/ipv6net.hh"
#include "libxorp/ipvxnet.hh"
#include "libxorp/mac.hh"

class IfMgrIfAtom;
class IfMgrVifAtom;
class IfMgrIPv4Atom;
class IfMgrIPv6Atom;

/**
 * @short Interface configuration tree container.
 *
 * The IfMgrIfTree is the top-level container of interface
 * configuration state.  The tree contains a collection of @ref
 * IfMgrIfAtom objects, each of which represents the configuration
 * state of a physical interface.
 */
class IfMgrIfTree {
public:
    typedef map<const string, IfMgrIfAtom> IfMap;

public:

    /**
     * Interface collection accessor.
     */
    inline const IfMap& interfaces() const		{ return _interfaces; }

    /**
     * Interface collection accessor.
     */
    inline IfMap& interfaces()				{ return _interfaces; }

    /**
     * Clear all interface state.
     */
    inline void clear();

    /**
     * Find interface.
     * @param ifname name of interface to find.
     * @return pointer to interface structure on success, 0 otherwise.
     */
    const IfMgrIfAtom* find_interface(const string& ifname) const;

    /**
     * Find interface.
     * @param ifname name of interface to find.
     * @return pointer to interface structure on success, 0 otherwise.
     */
    IfMgrIfAtom* find_interface(const string& ifname);

    /**
     * Find virtual interface.
     * @param ifname name of interface responsible for virtual interface.
     * @param vifname name of virtual interface.
     * @return pointer to virtual interface structure on success, 0 otherwise.
     */
    const IfMgrVifAtom* find_vif(const string& ifname,
				 const string& vifname) const;

    /**
     * Find virtual interface.
     * @param ifname name of interface responsible for virtual interface.
     * @param vifname name of virtual interface.
     * @return pointer to virtual interface structure on success, 0 otherwise.
     */
    IfMgrVifAtom* find_vif(const string& ifname,
			   const string& vifname);

    /**
     * Find IPv4 address structure.
     * @param ifname name of interface responsible for address.
     * @param vifname name of virtual interface responsible for address.
     * @param addr IPv4 address.
     * @return pointer to virtual interface structure on success, 0 otherwise.
     */
    const IfMgrIPv4Atom* find_addr(const string& ifname,
				   const string& vifname,
				   const IPv4&	 addr) const;

    /**
     * Find IPv4 address structure.
     * @param ifname name of interface responsible for address.
     * @param vifname name of virtual interface responsible for address.
     * @param addr IPv4 address.
     * @return pointer to virtual interface structure on success, 0 otherwise.
     */
    IfMgrIPv4Atom* find_addr(const string&	ifname,
			     const string&	vifname,
			     const IPv4&	addr);

    /**
     * Find IPv6 address structure.
     * @param ifname name of interface responsible for address.
     * @param vifname name of virtual interface responsible for address.
     * @param addr IPv6 address.
     * @return pointer to virtual interface structure on success, 0 otherwise.
     */
    const IfMgrIPv6Atom* find_addr(const string& ifname,
				   const string& vifname,
				   const IPv6&	 addr) const;

    /**
     * Find IPv6 address structure.
     * @param ifname name of interface responsible for address.
     * @param vifname name of virtual interface responsible for address.
     * @param addr IPv6 address.
     * @return pointer to virtual interface structure on success, 0 otherwise.
     */
    IfMgrIPv6Atom* find_addr(const string&	ifname,
			     const string&	vifname,
			     const IPv6&	addr);

    /**
     * Equality operator.
     * @param o tree to compare against.
     * @return true if this instance and o are the same, false otherwise.
     */
    bool operator==(const IfMgrIfTree& o) const;

    /**
     * Test if an IPv4 address belongs to an interface.
     * 
     * If an interface with the address is down, then the address is not
     * considered to belong to that interface.
     * 
     * @param addr the address to test.
     * @param ifname the return-by-reference name of the interface
     * the address belongs to, or an empty string.
     * @param vifname the return-by-reference name of the vif
     * the address belongs to, or an empty string.
     * @return true if the address belongs to an interface, otherwise false.
     */
    bool is_my_addr(const IPv4& addr, string& ifname, string& vifname) const;

    /**
     * Test if an IPv6 address belongs to an interface.
     * 
     * If an interface with the address is down, then the address is not
     * considered to belong to that interface.
     * 
     * @param addr the address to test.
     * @param ifname the return-by-reference name of the interface
     * the address belongs to, or an empty string.
     * @param vifname the return-by-reference name of the vif
     * the address belongs to, or an empty string.
     * @return true if the address belongs to an interface, otherwise false.
     */
    bool is_my_addr(const IPv6& addr, string& ifname, string& vifname) const;

    /**
     * Test if an IPvX address belongs to an interface.
     * 
     * If an interface with the address is down, then the address is not
     * considered to belong to that interface.
     * 
     * @param addr the address to test.
     * @param ifname the return-by-reference name of the interface
     * the address belongs to, or an empty string.
     * @param vifname the return-by-reference name of the vif
     * the address belongs to, or an empty string.
     * @return true if the address belongs to an interface, otherwise false.
     */
    bool is_my_addr(const IPvX& addr, string& ifname, string& vifname) const;

    /**
     * Test if an IPv4 address is directly connected to an interface.
     * 
     * If an interface toward an address is down, then the address is not
     * considered as directly connected.
     * 
     * @param addr the address to test.
     * @param ifname the return-by-reference name of the interface toward
     * the address if the address is directly connected otherwise an empty
     * string.
     * @param vifname the return-by-reference name of the vif toward
     * the address if the address is directly connected otherwise an empty
     * string.
     * @return true if the address is directly connected, otherwise false.
     */
    bool is_directly_connected(const IPv4& addr, string& ifname,
			       string& vifname) const;

    /**
     * Test if an IPv6 address is directly connected to an interface.
     * 
     * If an interface toward an address is down, then the address is not
     * considered as directly connected.
     * 
     * @param addr the address to test.
     * @param ifname the return-by-reference name of the interface toward
     * the address if the address is directly connected otherwise an empty
     * string.
     * @param vifname the return-by-reference name of the vif toward
     * the address if the address is directly connected otherwise an empty
     * string.
     * @return true if the address is directly connected, otherwise false.
     */
    bool is_directly_connected(const IPv6& addr, string& ifname,
			       string& vifname) const;

    /**
     * Test if an IPvX address is directly connected to an interface.
     * 
     * If an interface toward an address is down, then the address is not
     * considered as directly connected.
     * 
     * @param addr the address to test.
     * @param ifname the return-by-reference name of the interface toward
     * the address if the address is directly connected otherwise an empty
     * string.
     * @param vifname the return-by-reference name of the vif toward
     * the address if the address is directly connected otherwise an empty
     * string.
     * @return true if the address is directly connected, otherwise false.
     */
    bool is_directly_connected(const IPvX& addr, string& ifname,
			       string& vifname) const;

protected:
    IfMap	_interfaces;		// The interface configuration state
};


/**
 * @short Interface configuration atom.
 *
 * Represents a physical interface in XORP's model of forwarding h/w.
 * The configuration state includes attributes of the interface and a
 * collection of @ref IfMgrVifAtom objects representing the virtual
 * interfaces associated with the physical interface.
 */
class IfMgrIfAtom {
public:
    typedef map<const string, IfMgrVifAtom> VifMap;

public:
    inline IfMgrIfAtom(const string& name);

    inline const string& name() const			{ return _name; }

    inline bool		enabled() const			{ return _enabled; }
    inline void		set_enabled(bool v)		{ _enabled = v; }

    inline bool		discard() const			{ return _discard; }
    inline void		set_discard(bool v)		{ _discard = v; }

    inline uint32_t	mtu() const			{ return _mtu; }
    inline void		set_mtu(uint32_t v)		{ _mtu = v; }

    inline const Mac&	mac() const			{ return _mac; }
    inline void		set_mac(const Mac& v)		{ _mac = v; }

    inline uint32_t	pif_index() const		{ return _pif_index; }
    inline void		set_pif_index(uint32_t v)	{ _pif_index = v; }

    inline bool		no_carrier() const		{ return _no_carrier; }
    inline void		set_no_carrier(bool v)		{ _no_carrier = v; }

    inline const VifMap& vifs() const			{ return _vifs; }
    inline VifMap& vifs()				{ return _vifs; }
    const IfMgrVifAtom*	find_vif(const string& vifname) const;
    IfMgrVifAtom*	find_vif(const string& vifname);

    bool operator==(const IfMgrIfAtom& o) const;

private:
    IfMgrIfAtom();					// not implemented

protected:
    string	_name;		// The interface name

    bool	_enabled;	// True if enabled
    bool	_discard;	// True if a discard interface
    uint32_t	_mtu;		// The interface MTU (in bytes)
    Mac		_mac;		// The interface MAC address
    uint32_t	_pif_index;	// Physical interface index
    bool	_no_carrier;	// True if no carrier

    VifMap	_vifs;		// The vif configuration state
};


/**
 * @short Virtual Interface configuration atom.
 *
 * Represents a virtual interface in XORP's model of forwarding h/w.
 */
class IfMgrVifAtom {
public:
    typedef map<const IPv4, IfMgrIPv4Atom> IPv4Map;
    typedef map<const IPv6, IfMgrIPv6Atom> IPv6Map;

public:
    inline IfMgrVifAtom(const string& name);

    inline const string& name() const			{ return _name; }

    inline bool		enabled() const			{ return _enabled; }
    inline void		set_enabled(bool v)		{ _enabled = v; }

    inline bool		multicast_capable() const 	{ return _multicast_capable; }
    inline void		set_multicast_capable(bool v)	{ _multicast_capable = v; }

    inline bool		broadcast_capable() const 	{ return _broadcast_capable; }
    inline void		set_broadcast_capable(bool v)	{ _broadcast_capable = v; }

    inline bool		p2p_capable() const		{ return _p2p_capable; }
    inline void		set_p2p_capable(bool v)		{ _p2p_capable = v; }


    inline bool		loopback() const		{ return _loopback; }
    inline void		set_loopback(bool v)		{ _loopback = v; }

    inline bool		pim_register() const		{ return _pim_register; }
    inline void		set_pim_register(bool v)	{ _pim_register = v; }

    inline uint32_t	pif_index() const		{ return _pif_index; }
    inline uint32_t	set_pif_index(uint32_t v) 	{ return _pif_index = v; }

    inline const IPv4Map& ipv4addrs() const		{ return _ipv4addrs; }
    inline IPv4Map&	ipv4addrs() 			{ return _ipv4addrs; }
    const IfMgrIPv4Atom* find_addr(const IPv4& addr) const;
    IfMgrIPv4Atom*	find_addr(const IPv4& addr);

    inline const IPv6Map& ipv6addrs() const		{ return _ipv6addrs; }
    inline IPv6Map&	ipv6addrs() 			{ return _ipv6addrs; }
    const IfMgrIPv6Atom* find_addr(const IPv6& addr) const;
    IfMgrIPv6Atom*	find_addr(const IPv6& addr);

    bool 		operator==(const IfMgrVifAtom& o) const;

private:
    IfMgrVifAtom();			// Not implemented

protected:
    string	_name;			// The vif name

    bool	_enabled;		// True if enabled
    bool	_multicast_capable;	// True if multicast capable
    bool	_broadcast_capable;	// True if broadcast capable
    bool	_p2p_capable;		// True if point-to-point capable
    bool	_loopback;		// True if loopback vif
    bool	_pim_register;		// True if PIM Register vif
    uint32_t	_pif_index;		// Physical interface index

    IPv4Map	_ipv4addrs;		// The IPv4 addresses
    IPv6Map	_ipv6addrs;		// The IPv6 addresses
};


/**
 * @short IPv4 configuration atom.
 *
 * Represents an address associated with a virtual interface in XORP's model
 * of forwarding h/w.
 */
class IfMgrIPv4Atom {
public:
    inline IfMgrIPv4Atom(const IPv4& addr);

    inline const IPv4&	addr() const			{ return _addr; }

    inline uint32_t	prefix_len() const		{ return _prefix_len; }
    inline void		set_prefix_len(uint32_t v)	{ _prefix_len = v; }

    inline bool		enabled() const			{ return _enabled; }
    inline void		set_enabled(bool v)		{ _enabled = v; }

    inline bool		multicast_capable() const 	{ return _multicast_capable; }
    inline void		set_multicast_capable(bool v)	{ _multicast_capable = v; }

    inline bool		loopback() const		{ return _loopback; }
    inline void		set_loopback(bool v) 		{ _loopback = v; }

    inline bool		has_broadcast() const		{ return _broadcast; }
    inline void		remove_broadcast()		{ _broadcast = false; }
    inline void		set_broadcast_addr(const IPv4& baddr);
    inline const IPv4&	broadcast_addr() const;

    inline bool		has_endpoint() const		{ return _p2p; }
    inline void		remove_endpoint()		{ _p2p = false; }
    inline void		set_endpoint_addr(const IPv4& endpoint);
    inline const IPv4&	endpoint_addr() const;

    bool 		operator==(const IfMgrIPv4Atom& o) const;

private:
    IfMgrIPv4Atom();			// Not implemented

protected:
    IPv4	_addr;			// The address
    uint32_t	_prefix_len;		// The network prefix length
    bool	_enabled;		// True if enabled
    bool	_multicast_capable;	// True if multicast capable
    bool	_loopback;		// True if a loopback address
    bool	_broadcast;	// True if _other_addr is a broadcast address
    bool	_p2p;		// True if _other_addr is a p2p address

    IPv4	_other_addr;	// The "other" address [broadcast | p2p]
    static const IPv4	_ZERO_ADDR;	// IPv4::ZERO() address
};


/**
 * @short IPv6 configuration atom.
 *
 * Represents an address associated with a virtual interface in XORP's model
 * of forwarding h/w.
 */

class IfMgrIPv6Atom {
public:
    inline IfMgrIPv6Atom(const IPv6& addr);

    inline const IPv6&  addr() const			{ return _addr; }

    inline bool		enabled() const			{ return _enabled; }
    inline void		set_enabled(bool v)		{ _enabled = v; }

    inline uint32_t	prefix_len() const		{ return _prefix_len; }
    inline void		set_prefix_len(uint32_t v)	{ _prefix_len = v; }

    inline bool		multicast_capable() const 	{ return _multicast_capable; }
    inline void		set_multicast_capable(bool v)	{ _multicast_capable = v; }

    inline bool		loopback() const		{ return _loopback; }
    inline void		set_loopback(bool v) 		{ _loopback = v; }

    inline bool		has_endpoint() const		{ return _p2p; }
    inline void		remove_endpoint()		{ _p2p = false; }
    inline void		set_endpoint_addr(const IPv6& endpoint);
    inline const IPv6&	endpoint_addr() const;

    bool 		operator==(const IfMgrIPv6Atom& o) const;

private:
    IfMgrIPv6Atom();			// Not implemented

protected:
    IPv6	_addr;			// The address
    uint32_t	_prefix_len;		// The network prefix length
    bool	_enabled;		// True if enabled
    bool	_multicast_capable;	// True if multicast capable
    bool	_loopback;		// True if a loopback address
    bool	_p2p;		// True if _other_addr is a p2p2 address

    IPv6	_other_addr;	// The "other" address [p2p]
    static const IPv6	_ZERO_ADDR;	// IPv6::ZERO() address
};


/**
 * Class specialized to provide a way to find IfMgrIPv{4,6}Atom given
 * IPv{4,6} type.  This is useful for code that is solely interested
 * in common attributes and methods of IfMgrIPv4Atom and IfMgrIPv6Atom.
 *
 * Example usage:
 * <pre>
 * template <typename A>
 * bool addr_exists_and_enabled(IfMgrVifAtom& vif, const A& a)
 * {
 *     const typename IfMgrIP<A>::Atom* a = vif.find_addr(a);
 *     return a != 0 && a->enabled();
 * }
 * </pre>
 */
template <typename A>
struct IfMgrIP
{
};

template <>
struct IfMgrIP<IPv4>
{
    typedef IfMgrIPv4Atom Atom;
};

template <>
struct IfMgrIP<IPv6>
{
    typedef IfMgrIPv6Atom Atom;
};


// ----------------------------------------------------------------------------
// Inline IfMgrIfTree methods

inline void
IfMgrIfTree::clear()
{
    _interfaces.clear();
}


// ----------------------------------------------------------------------------
// Inline IfMgrIfAtom methods

inline
IfMgrIfAtom::IfMgrIfAtom(const string& name)
    : _name(name),
      _enabled(false),
      _discard(false),
      _mtu(0),
      _pif_index(0),
      _no_carrier(false)
{
}


// ----------------------------------------------------------------------------
// Inline IfMgrVifAtom methods

inline
IfMgrVifAtom::IfMgrVifAtom(const string& name)
    : _name(name),
      _enabled(false),
      _multicast_capable(false),
      _broadcast_capable(false),
      _p2p_capable(false),
      _loopback(false),
      _pim_register(false),
      _pif_index(0)
{
}

// ----------------------------------------------------------------------------
// Inline IfMgrIPv4Atom methods

inline
IfMgrIPv4Atom::IfMgrIPv4Atom(const IPv4& addr)
    : _addr(addr),
      _prefix_len(0),
      _enabled(false),
      _multicast_capable(false),
      _loopback(false),
      _broadcast(false),
      _p2p(false)
{
}

inline void
IfMgrIPv4Atom::set_broadcast_addr(const IPv4& broadcast_addr)
{
    if (broadcast_addr == IPv4::ZERO()) {
	_broadcast = false;
    } else {
	_broadcast = true;
	_p2p = false;
	_other_addr = broadcast_addr;
    }
}

inline const IPv4&
IfMgrIPv4Atom::broadcast_addr() const
{
    return _broadcast ? _other_addr : _ZERO_ADDR;
}

inline void
IfMgrIPv4Atom::set_endpoint_addr(const IPv4& p2p_addr)
{
    if (p2p_addr == IPv4::ZERO()) {
	_p2p = false;
    } else {
	_p2p = true;
	_broadcast = false;
	_other_addr = p2p_addr;
    }
}

inline const IPv4&
IfMgrIPv4Atom::endpoint_addr() const
{
    return _p2p ? _other_addr : _ZERO_ADDR;
}


// ----------------------------------------------------------------------------
// Inline IfMgrIPv6Atom methods

inline
IfMgrIPv6Atom::IfMgrIPv6Atom(const IPv6& addr)
    : _addr(addr),
      _prefix_len(0),
      _enabled(false),
      _multicast_capable(false),
      _loopback(false),
      _p2p(false)
{
}

inline void
IfMgrIPv6Atom::set_endpoint_addr(const IPv6& p2p_addr)
{
    if (p2p_addr == IPv6::ZERO()) {
	_p2p = false;
    } else {
	_p2p = true;
	_other_addr = p2p_addr;
    }
}

inline const IPv6&
IfMgrIPv6Atom::endpoint_addr() const
{
    return _p2p ? _other_addr : _ZERO_ADDR;
}

#endif // __LIBFEACLIENT_IFMGR_ATOMS_HH__
