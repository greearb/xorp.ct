// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
//
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net


#ifndef __LIBXORP_VIF_HH__
#define __LIBXORP_VIF_HH__


#include "xorp.h"
#include "ipv4.hh"
#include "ipv6.hh"
#include "ipvx.hh"
#include "ipvxnet.hh"
#include "bug_catcher.hh"

/**
 * @short Virtual interface address class.
 *
 * VifAddr holds information about an address of a virtual interface.
 * A virtual interface may have more than one VifAddr.
 */
class VifAddr {
public:
    /**
     * Constructor for a given address.
     *
     * @param ipvx_addr the interface address.
     */
    explicit VifAddr(const IPvX& ipvx_addr);

    /**
     * Constructor for a given address, and its associated addresses.
     *
     * @param ipvx_addr the interface address.
     * @param ipvx_subnet_addr the subnet address.
     * @param ipvx_broadcast_addr the broadcast address (if the interface
     * is broadcast-capable).
     * @param ipvx_peer_addr the peer address (if the interface is
     * point-to-point).
     */
    VifAddr(const IPvX& ipvx_addr, const IPvXNet& ipvx_subnet_addr,
	    const IPvX& ipvx_broadcast_addr, const IPvX& ipvx_peer_addr);

#ifdef XORP_USE_USTL
    VifAddr(); // AF_INET
#endif

    /**
     * Get the interface address.
     *
     * @return the interface address.
     */
    const IPvX&		addr()		const	{ return (_addr);	}

    /**
     * Get the subnet address.
     *
     * @return the subnet address of the interface.
     */
    const IPvXNet&	subnet_addr()	const	{ return (_subnet_addr); }

    /**
     * Get the broadcast address.
     *
     * The broadcast address is valid only if the interface is broadcast
     * capable.
     *
     * @return the broadcast address of the interface.
     */
    const IPvX&		broadcast_addr() const	{ return (_broadcast_addr); }

    /**
     * Get the peer address.
     *
     * The peer address is valid only if the interface is point-to-point.
     *
     * @return the peer address of the interface.
     */
    const IPvX&		peer_addr()	const	{ return (_peer_addr);	}

    /**
     * Set the interface address.
     *
     * @param v the interface address to set to the interface.
     */
    void  set_addr(const IPvX& v)		{ _addr = v;		}

    /**
     * Set the subnet address.
     *
     * @param v the subnet address to set to the interface.
     */
    void  set_subnet_addr(const IPvXNet& v)	{ _subnet_addr = v;	}

    /**
     * Set the broadcast address.
     *
     * @param v the broadcast address to set to the interface.
     */
    void  set_broadcast_addr(const IPvX& v)	{ _broadcast_addr = v;	}

    /**
     * Set the peer address.
     *
     * @param v the peer address to set to the interface.
     */
    void  set_peer_addr(const IPvX& v)		{ _peer_addr = v;	}

    /**
     * Test whether if an IPvX address is same as my address.
     *
     * @param ipvx_addr the address to test whether is the same as my
     * interface address.
     *
     * @return true if @ref ipvx_addr is same as my interface address,
     * otherwise false.
     */
    bool  is_my_addr(const IPvX& ipvx_addr) const { return (addr() == ipvx_addr); }

    /**
     * Test if a given subnet address is a subset of my subnet address.
     *
     * @param ipvxnet the subnet address to test whether is a subset of
     * my subnet address.
     *
     * @return true if @ref ipvxnet is a subset of my subnet address,
     * otherwise false.
     */
    bool  is_same_subnet(const IPvXNet& ipvxnet) const;

    /**
     * Test whether an address belongs to my subnet address.
     *
     * @param ipvx_addr the address to test whether it belongs to my subnet
     * address.
     * @return true if @ref ipvx_addr belongs to my subnet address,
     * otherwise false.
     */
    bool  is_same_subnet(const IPvX& ipvx_addr) const;

    /**
     * Convert this address from binary form to presentation format.
     *
     * @return C++ string with the human-readable ASCII representation
     * of the address.
     */
    string str() const;

    /**
     * Equality Operator
     *
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically same as the
     * right-hand operand.
     */
    bool operator==(const VifAddr& other) const;

private:
    IPvX	_addr;				// IP address of the vif
    IPvXNet	_subnet_addr;			// Subnet address on the vif
    IPvX	_broadcast_addr;		// Network broadcast address
    IPvX	_peer_addr;			// Peer address (on p2p links)
};


class VifPermInfo {
public:
    VifPermInfo(const string& n, bool start, bool enable)
	    : vif_name(n), should_start(start), should_enable(enable) { }
    VifPermInfo() : should_start(false), should_enable(false) { }
    VifPermInfo(const VifPermInfo& p)
	    : vif_name(p.vif_name), should_start(p.should_start), should_enable(p.should_enable) { }

    VifPermInfo& operator=(const VifPermInfo& p) {
	if (this != &p) {
	    vif_name = p.vif_name;
	    should_start = p.should_start;
	    should_enable = p.should_enable;
	}
	return *this;
    }

    string vif_name; // name of VIF in question
    bool should_start; // Want to be started if possible
    bool should_enable; // Want to be enabled if possible
};


/**
 * @short Virtual Interface class.
 *
 * Vif holds information about a virtual interface.  A Vif may
 * represent a physical interface, or may represent more abstract
 * entities such as the Discard or Unreachable interface, or a VLAN
 * on a physical interface.
 */
class Vif : public BugCatcher {
public:
    /**
     * Constructor for a given virtual interface name.
     *
     * @param vifname string representation of the virtual interface
     * (e.g., "port 0").
     * @param ifname string representation of associated interface.
     */
    explicit Vif(const string& vifname, const string& ifname = string(""));

#ifdef XORP_USE_USTL
    Vif() { }
#endif

    /**
     * Constructor to clone a Vif.
     *
     * @param vif the virtual interface to clone.
     */
    Vif(const Vif& vif);

    Vif& operator=(const Vif& other);

    /**
     * Destructor
     */
    virtual ~Vif();

    /**
     * Convert this Vif from binary form to presentation format.
     *
     * @return C++ string with the human-readable ASCII representation
     * of the Vif.
     */
    string str() const;

    /**
     * Equality Operator
     *
     * @param other the right-hand operand to compare against.
     * @return true if the left-hand operand is numerically same as the
     * right-hand operand.
     */
    bool operator==(const Vif& other) const;

    /**
     * Get the vif name.
     *
     * @return a string representation of the vif name.
     */
    const string& name() const { return _name; }

    /**
     * Get the name of the physical interface associated with vif.
     *
     * @return string representation of interface name.
     */
    const string& ifname() const { return _ifname; }

    /**
     * Set the name of the physical interface associated with vif.
     *
     * @param ifname
     */
    void set_ifname(const string& ifname) { _ifname = ifname; }

    /**
     * Get the physical interface index.
     *
     * @return the physical interface index (if applicable).
     */
    uint32_t pif_index() const { return (_pif_index); }

    /**
     * Set the physical interface index.
     *
     * @param v the value of the physical interface index to set to.
     */
    void set_pif_index(uint32_t v) { _pif_index = v; }

    /**
     * Various vif_index related values.
     */
    enum {
	VIF_INDEX_INVALID	= ((uint32_t)~0),
	VIF_INDEX_MAX		= ((uint32_t)~0)
    };

    /**
     * Get the virtual interface index.
     *
     * @return the virtual interface index.
     */
    uint32_t vif_index() const { return (_vif_index); }

    /**
     * Set the virtual interface index.
     *
     * @param v the value of the virtual interface index to set to.
     */
    void set_vif_index(uint32_t v) { _vif_index = v; }

    /**
     * Test if this vif is a PIM Register interface.
     *
     * @return true if this vif is a PIM Register interface, otherwise false.
     */
    bool	is_pim_register()	const	{ return _is_pim_register; }

    /**
     * Test if this vif is a point-to-point interface.
     *
     * @return true if this vif is a point-to-point interface, otherwise false.
     */
    bool	is_p2p()		const	{ return _is_p2p; }

    /**
     * Test if this vif is a loopback interface.
     *
     * @return true if this vif is a loopback interface, otherwise false.
     */
    bool	is_loopback()		const	{ return _is_loopback; }

    /**
     * Test if this vif is a discard interface.
     *
     * @return true if this vif is a discard interface, otherwise false.
     */
    bool	is_discard()		const	{ return _is_discard; }

    /**
     * Test if this vif is an unreachable interface.
     *
     * @return true if this vif is an unreachable interface, otherwise false.
     */
    bool	is_unreachable()	const	{ return _is_unreachable; }

    /**
     * Test if this vif is a management interface.
     *
     * @return true if this vif is a management interface, otherwise false.
     */
    bool	is_management()	const	{ return _is_management; }

    /**
     * Test if this vif is multicast capable.
     *
     * @return true if this vif is multicast capable, otherwise false.
     */
    bool	is_multicast_capable() const { return _is_multicast_capable; }

    /**
     * Test if this vif is broadcast capable.
     *
     * @return true if this vif is broadcast capable, otherwise false.
     */
    bool	is_broadcast_capable() const { return _is_broadcast_capable; }

    /**
     * Test if the underlying vif is UP.
     *
     * An example of an underlying vif is the corresponding interface
     * inside the kernel, or the MFEA interface which a PIM interface
     * matches to.
     *
     * @return true if the underlying vif is UP (when applicable), otherwise
     * false.
     */
    bool	is_underlying_vif_up() const { return _is_underlying_vif_up; }

    /* Is this thing just a dummy, and not really in the OS currently? */
    bool	is_fake() const { return _is_fake; }

    /**
     * Get the MTU of the vif.
     *
     * @return the MTU of the vif.
     */
    uint32_t	mtu() const { return _mtu; }

    /**
     * Set/reset the vif as a PIM Register interface.
     *
     * @param v if true, then set this vif as a PIM Register interface,
     * otherwise reset it.
     */
    void	set_pim_register(bool v)	{ _is_pim_register = v; }

    /**
     * Set/reset the vif as a point-to-point interface.
     *
     * @param v if true, then set this vif as a point-to-point interface,
     * otherwise reset it.
     */
    void	set_p2p(bool v)			{ _is_p2p = v; }

    /**
     * Set/reset the vif as a loopback interface.
     *
     * @param v if true, then set this vif as a loopback interface,
     * otherwise reset it.
     */
    void	set_loopback(bool v)		{ _is_loopback = v; }

    /**
     * Set/reset the vif as a discard interface.
     *
     * @param v if true, then set this vif as a discard interface,
     * otherwise reset it.
     */
    void	set_discard(bool v)		{ _is_discard = v; }

    /**
     * Set/reset the vif as an unreachable interface.
     *
     * @param v if true, then set this vif as an unreachable interface,
     * otherwise reset it.
     */
    void	set_unreachable(bool v)		{ _is_unreachable = v; }

    /**
     * Set/reset the vif as a management interface.
     *
     * @param v if true, then set this vif as a management interface,
     * otherwise reset it.
     */
    void	set_management(bool v)		{ _is_management = v; }

    /**
     * Set/reset the vif as multicast capable.
     *
     * @param v if true, then set this vif as multicast capable,
     * otherwise reset it.
     */
    void	set_multicast_capable(bool v)	{ _is_multicast_capable = v; }

    /**
     * Set/reset the vif as broadcast capable.
     *
     * @param v if true, then set this vif as broadcast capable,
     * otherwise reset it.
     */
    void	set_broadcast_capable(bool v)	{ _is_broadcast_capable = v; }

    /**
     * Set/reset the underlying vif status (when applicable).
     *
     * An example of an underlying vif is the corresponding interface
     * inside the kernel, or the MFEA interface which a PIM interface
     * matches to.
     *
     * @param v if true, then set the underlying vif status as UP,
     * otherwise the underlying vif status is set to DOWN.
     */
    void	set_underlying_vif_up(bool v)	{ _is_underlying_vif_up = v; }

    void	set_is_fake(bool v)	{ _is_fake = v; }

    /**
     * Set the MTU of the vif.
     *
     * @param v the MTU of the vif.
     */
    void	set_mtu(uint32_t v) { _mtu = v; }

    /**
     * Get the list of all addresses for this vif.
     *
     * @return the list of all addresses for this vif (@ref VifAddr).
     */
    const list<VifAddr>& addr_list() const { return _addr_list; }

    /**
     * Get the first vif address.
     *
     * @return a pointer to the first valid interface address of this vif,
     * or NULL if no addresses.
     */
    const IPvX* addr_ptr() const;

    /**
     * Add a VifAddr address to the interface.
     *
     * @param vif_addr the VifAddr (@ref VifAddr) to add to the list
     * of addresses for this vif.
     *
     * @return XORP_OK if a new address, otherwise XORP_ERROR.
     */
    int add_address(const VifAddr& vif_addr);

    /**
     * Add an IPvX address and all related information to the interface.
     *
     * @param ipvx_addr the interface address.
     * @param ipvxnet_subnet_addr the subnet address.
     * @param ipvx_broadcast_addr the broadcast address.
     * @param ipvx_peer_addr the peer address.
     * @return XORP_OK if a new address, otherwise XORP_ERROR.
     */
    int add_address(const IPvX& ipvx_addr,
		    const IPvXNet& ipvxnet_subnet_addr,
		    const IPvX& ipvx_broadcast_addr,
		    const IPvX& ipvx_peer_addr);

    /**
     * Add an IPvX address to the interface.
     *
     * @param ipvx_addr the interface address.
     * @return XORP_OK if a new address, otherwise XORP_ERROR.
     */
    int add_address(const IPvX& ipvx_addr);

    /**
     * Delete an IPvX address from the interface.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int delete_address(const IPvX& ipvx_addr);

    /**
     * Find a VifAddr that corresponds to an IPvX address.
     *
     * @param ipvx_addr the IPvX address to search for.
     * @return a pointer to the VifAddr for address @ref ipvx_addr
     * if found, otherwise NULL.
     */
    VifAddr *find_address(const IPvX& ipvx_addr);

    /**
     * Find a const VifAddr that corresponds to an IPvX address.
     *
     * @param ipvx_addr the IPvX address to search for.
     * @return a const pointer to the VifAddr for address @ref ipvx_addr
     * if found, otherwise NULL.
     */
    const VifAddr *find_address(const IPvX& ipvx_addr) const;

    /**
     * Test if an IPvX address belongs to this vif.
     *
     * @param ipvx_addr the IPvX address to test whether belongs to this vif.
     * @return true if @ref ipvx_addr belongs to this vif, otherwise false.
     */
    bool is_my_addr(const IPvX& ipvx_addr) const;

    /**
     * Test if an VifAddr is belongs to this vif.
     *
     * @param vif_addr the VifAddr address to test whether belongs to this vif.
     * @return true if @ref vif_addr belongs to this vif, otherwise false.
     */
    bool is_my_vif_addr(const VifAddr& vif_addr) const;

    /**
     * Test if a given subnet address is a subset of one of the subnet
     * addresses of this vif.
     *
     * @param ipvxnet the subnet address to test against.
     * @return true if @ref ipvxnet is a subset of one of the subnet
     * addresses of this vif, otherwise false.
     */
    bool is_same_subnet(const IPvXNet& ipvxnet) const;

    /**
     * Test if a given address belongs to one of the subnet addresses
     * of this vif.
     *
     * @param ipvx_addr the address to test against.
     * @return true if @ref ipvx_addr belongs to one of the subnet addresses
     * of this vif, otherwise false.
     */
    bool is_same_subnet(const IPvX& ipvx_addr) const;

    /**
     * Test if a given address belongs to the same point-to-point link
     * as this vif.
     *
     * @param ipvx_addr the address to test against.
     * @return true if @ref ipvx_addr belongs to the same point-to-point link
     * as this vif, otherwise false.
     */
    bool is_same_p2p(const IPvX& ipvx_addr) const;

private:
    string	_name;		// The vif name
    string	_ifname;	// The physical interface name (if applicable)
    uint32_t	_pif_index;	// Physical interface index (if applicable),
				// or 0 if invalid.
    uint32_t	_vif_index;	// Virtual interface index
    bool	_is_pim_register;	// PIM Register vif
    bool	_is_p2p;		// Point-to-point interface
    bool	_is_loopback;		// Loopback interface
    bool	_is_discard;		// Discard interface
    bool	_is_unreachable;	// Unreachable interface
    bool	_is_management;		// Management interface
    bool	_is_multicast_capable;	// Multicast-capable interface
    bool	_is_broadcast_capable;	// Broadcast-capable interface
    bool	_is_underlying_vif_up;	// True if underlying vif is up
    bool	_is_fake;;		// True if VIF is dummy (not in OS currently)
    uint32_t	_mtu;			// The MTU of the vif

    list<VifAddr> _addr_list;		// The list of addresses for this vif
};

#endif // __LIBXORP_VIF_HH__
