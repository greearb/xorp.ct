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

// $XORP: xorp/fea/iftree.hh,v 1.42 2007/05/08 00:07:57 pavlin Exp $

#ifndef __FEA_IFTREE_HH__
#define __FEA_IFTREE_HH__

#include "libxorp/xorp.h"

#include <map>
#include <string>

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/mac.hh"

/**
 * Base class for Fea configurable items where the modifications need
 * to be held over and propagated later, ie changes happen during a
 * transaction but are propagated during the commit.
 */
class IfTreeItem {
public:
    IfTreeItem() : _st(CREATED), _soft(false) {}
    virtual ~IfTreeItem() {}

public:
    enum State {
	NO_CHANGE = 0x00,
	CREATED = 0x01,
	DELETED = 0x02,
	CHANGED = 0x04
    };

    inline bool set_state(State st) {
	if (bits(st) > 1) {
	    return false;
	}
	_st = st;
	return true;
    }

    inline State state() const 			{ return _st; }

    inline bool mark(State st) {
	if (bits(st) > 1) {
	    return false;
	}
	if (st & (CREATED | DELETED)) {
	    _st = st;
	    return true;
	}
	if (_st & (CREATED | DELETED)) {
	    return true;
	}
	_st = st;
	return true;
    }
    inline bool is_marked(State st) const	{ return st == _st; }

    inline void set_soft(bool en)	{ _soft = en; }

    inline bool is_soft() const		{ return _soft; }

    /**
     * Virtual method to be implemented to flush out state associated
     * objects, ie if an object is marked CREATED or CHANGED it should be
     * marked NO_CHANGE, if an object is marked DELETED, it should be
     * removed from the relevant container and destructed.
     */
    virtual void finalize_state() = 0;

    string str() const;

protected:
    inline static uint32_t bits(State st) {
	uint32_t c;
	for (c = 0; st != NO_CHANGE; c += st & 0x01)
	    st = State(st >> 1);
	return c;
    }

    State _st;
    bool  _soft;
};


// Classes derived from IfTreeItem
class IfTree;
class IfTreeInterface;
class IfTreeVif;
class IfTreeAddr4;
class IfTreeAddr6;

/**
 * Container class for Fea Interface objects in a system.
 */
class IfTree : public IfTreeItem {
public:
    typedef map<const string, IfTreeInterface> IfMap;

    /**
     * Remove all interface state from the interface tree.
     */
    void clear();

    /**
     * Create a new interface.
     *
     * @param ifname interface name.
     *
     * @return true on success, false if an error.
     */
    bool add_interface(const string& ifname);

    /**
     * Label interface as ready for deletion.  Deletion does not occur
     * until finalize_state() is called.
     *
     * @param ifname name of interface to be labelled.
     *
     * @return true on success, false if ifname is invalid.
     */
    bool remove_interface(const string& ifname);

    /**
     * Create a new interface or update its state if it already exists.
     *
     * @param other_iface the interface with the state to copy from.
     *
     * @return true on success, false if an error.
     */
    bool update_interface(const IfTreeInterface& other_iface);

    /**
     * Find an interface.
     *
     * @param ifname the interface name to search for.
     * @return a pointer to the interface (@see IfTreeInterface) or NULL
     * if not found.
     */
    IfTreeInterface* find_interface(const string& ifname);

    /**
     * Find a const interface.
     *
     * @param ifname the interface name to search for.
     * @return a const pointer to the interface (@see IfTreeInterface) or NULL
     * if not found.
     */
    const IfTreeInterface* find_interface(const string& ifname) const;

    /**
     * Find an interface for a given physical index.
     *
     * @param ifindex the interface index to search for.
     * @return a pointer to the interface (@see IfTreeInterface) or NULL
     * if not found.
     */
    IfTreeInterface* find_interface(uint32_t ifindex);

    /**
     * Find a const interface for a given physical index.
     *
     * @param ifindex the interface index to search for.
     * @return a const pointer to the interface (@see IfTreeInterface) or NULL
     * if not found.
     */
    const IfTreeInterface* find_interface(uint32_t ifindex) const;

    /**
     * Find a vif.
     *
     * @param ifname the interface name to search for.
     * @param vifname the vif name to search for.
     * @return a pointer to the vif (@see IfTreeVif) or NULL if not found.
     */
    IfTreeVif* find_vif(const string& ifname, const string& vifname);

    /**
     * Find a const vif.
     *
     * @param ifname the interface name to search for.
     * @param vifname the vif name to search for.
     * @return a const pointer to the vif (@see IfTreeVif) or NULL
     * if not found.
     */
    const IfTreeVif* find_vif(const string& ifname,
			      const string& vifname) const;

    /**
     * Find an IPv4 address.
     *
     * @param ifname the interface name to search for.
     * @param vifname the vif name to search for.
     * @param addr the address to search for.
     * @return a pointer to the vif (@see IfTreeAddr4) or NULL if not found.
     */
    IfTreeAddr4* find_addr(const string& ifname, const string& vifname,
			   const IPv4& addr);

    /**
     * Find a const IPv4 address.
     *
     * @param ifname the interface name to search for.
     * @param vifname the vif name to search for.
     * @param addr the address to search for.
     * @return a const pointer to the vif (@see IfTreeAddr4) or NULL
     * if not found.
     */
    const IfTreeAddr4* find_addr(const string& ifname, const string& vifname,
				 const IPv4& addr) const;

    /**
     * Find an IPv6 address.
     *
     * @param ifname the interface name to search for.
     * @param vifname the vif name to search for.
     * @param addr the address to search for.
     * @return a pointer to the vif (@see IfTreeAddr6) or NULL if not found.
     */
    IfTreeAddr6* find_addr(const string& ifname, const string& vifname,
			   const IPv6& addr);

    /**
     * Find a const IPv6 address.
     *
     * @param ifname the interface name to search for.
     * @param vifname the vif name to search for.
     * @param addr the address to search for.
     * @return a pointer to the vif (@see IfTreeAddr6) or NULL if not found.
     */
    const IfTreeAddr6* find_addr(const string& ifname, const string& vifname,
				 const IPv6& addr) const;

    /**
     * Get the map with the stored interfaces.
     *
     * @return the map with the stored interfaces.
     */
    inline IfMap& interfaces() { return _interfaces; }

    /**
     * Get the const map with the stored interfaces.
     *
     * @return the const map with the stored interfaces.
     */
    inline const IfMap& interfaces() const { return _interfaces; }

    /**
     * Align user supplied configuration with the device configuration.
     *
     * Inside the FEA there may be multiple configuration representations,
     * typically one the user modifies and one that mirrors the hardware.
     * Errors may occur pushing the user config down onto the hardware and
     * we need a method to update the user config from the h/w config that
     * exists after the config push.  We can't just copy the h/w config since
     * the user config is restricted to configuration set by the user.
     * The alignment works as follows:
     * - If the item in the local tree is "disabled", then the state is copied
     *   but the item is still marked as "disabled". Otherwise, the rules
     *   below are applied.
     * - If an item from the local tree is not in the other tree,
     *   it is marked as deleted in the local tree.
     *   However, if an interface from the local tree is marked as "soft"
     *   or "discard_emulated", and is not in the other tree, the interface
     *   is not marked as deleted in the local tree.
     * - If an item from the local tree is in the other tree,
     *   its state is copied from the other tree to the local tree.
     *   However, if an item from the local tree is marked as "flipped",
     *   it will be set in the local tree even if it is not set in the other
     *   tree.
     * - If an item from the other tree is not in the local tree, we do NOT
     *   copy it to the local tree.
     *
     * @param other the configuration tree to align state with.
     * @return modified configuration structure.
     */
    IfTree& align_with(const IfTree& other);

    /**
     * Prepare configuration for pushing and replacing previous configuration.
     *
     * If the previous configuration is to be replaced with new configuration,
     * we need to prepare the state that will delete, update, and add the
     * new state as appropriate.
     * The preparation works as follows:
     * - All items in the local tree are preserved and marked as created.
     * - All items in the other tree that are not in the local tree are
     *   added to the local tree and are marked as deleted.
     *   Only if the interface is marked as "soft" or "discard_emulated",
     *   or if the item in the other state is marked as disabled, then it
     *   is not added.
     *
     * @param other the configuration tree to be used to prepare the
     * replacement state.
     * @return modified configuration structure.
     */
    IfTree& prepare_replacement_state(const IfTree& other);

    /**
     * Prune bogus deleted state.
     * 
     * If an item from the local tree is marked as deleted, but is not
     * in the other tree, then it is removed.
     * 
     * @param old_iftree the old tree with the state that is used as reference.
     * @return the modified configuration tree.
     */
    IfTree& prune_bogus_deleted_state(const IfTree& old_iftree);

    /**
     * Track modifications from the live config state as read from the kernel.
     *
     * All interface-related modifications as received by the observer
     * mechanism are recorded in a local copy of the interface tree
     * (the live configuration tree). Some of those modifications however
     * should be propagated to the XORP local configuration tree.
     * This method updates a local configuration tree with only the relevant
     * modifications of the live configuration tree:
     * - Only if an item is in the local configuration tree, its status
     *   may be modified.
     * - If the "no_carrier" flag of an interface is changed in the live
     *   configuration tree, the corresponding flag in the local configuration
     *   tree is updated.
     * 
     * @param other the live configuration tree whose modifications are
     * tracked.
     * @return modified configuration structure.
     */
    IfTree& track_live_config_state(const IfTree& other);

    /**
     * Delete interfaces labelled as ready for deletion, call finalize_state()
     * on remaining interfaces, and set state to NO_CHANGE.
     */
    void finalize_state();

    /**
     * @return string representation of IfTree.
     */
    string str() const;

protected:
    IfMap	_interfaces;
};


/**
 * Fea class for holding physical interface state.
 */
class IfTreeInterface : public IfTreeItem {
public:
    typedef map<const string, IfTreeVif> VifMap;

    IfTreeInterface(const string& ifname);

    inline const string& name() const	{ return _ifname; }

    inline const string& ifname() const	{ return _ifname; }

    inline uint32_t pif_index() const	{ return _pif_index; }
    inline void set_pif_index(uint32_t v) { _pif_index = v; mark(CHANGED); }

    inline bool enabled() const		{ return _enabled; }

    inline void set_enabled(bool en)	{ _enabled = en; mark(CHANGED); }

    inline uint32_t mtu() const		{ return _mtu; }

    inline void set_mtu(uint32_t mtu)	{ _mtu = mtu; mark(CHANGED); }

    inline const Mac& mac() const	{ return _mac; }

    inline void set_mac(const Mac& mac)	{ _mac = mac; mark(CHANGED); }

    inline bool no_carrier() const	{ return _no_carrier; }

    inline void set_no_carrier(bool v)	{ _no_carrier = v; mark(CHANGED); }

    inline bool discard() const		{ return _discard; }

    inline void set_discard(bool discard) {
	_discard = discard;
	mark(CHANGED);
    }

    inline bool is_discard_emulated() const { return _is_discard_emulated; }

    inline void set_discard_emulated(bool v) {
	_is_discard_emulated = v;
	mark(CHANGED);
    }

    /**
     * Get the flipped flag.
     *
     * This flag indicates the interface's enable/disable status
     * has been flipped (i.e., first disabled, and then enabled).
     *
     * @return true if the interface has been flipped, otherwise false.
     */
    inline bool flipped() const		{ return _flipped; }

    /**
     * Set the value of the flipped flag.
     *
     * @param v if true, then the flipped flag is enabled, otherwise is
     * disabled.
     */
    inline void set_flipped(bool v)	{ _flipped = v; mark(CHANGED); }

    /**
     * Get the system-specific interface flags.
     *
     * Typically, this value is read from the underlying system, and is
     * used only for internal purpose.
     *
     * @return the system-specific interface flags.
     */
    inline uint32_t interface_flags() const	{ return _interface_flags; }

    /**
     * Store the system-specific interface flags.
     *
     * Typically, this value is read from the underlying system, and is
     * used only for internal purpose.
     *
     * @param v the value of the system-specific interface flags to store.
     */
    inline void set_interface_flags(uint32_t v) { _interface_flags = v; mark(CHANGED); }

    inline const VifMap& vifs() const	{ return _vifs; }
    inline VifMap& vifs()		{ return _vifs; }

    bool add_vif(const string& vifname);

    bool remove_vif(const string& vifname);

    /**
     * Find a vif.
     *
     * @param vifname the vif name to search for.
     * @return a pointer to the vif (@see IfTreeVif) or NULL if not found.
     */
    IfTreeVif* find_vif(const string& vifname);

    /**
     * Find a const vif.
     *
     * @param vifname the vif name to search for.
     * @return a const pointer to the vif (@see IfTreeVif) or NULL
     * if not found.
     */
    const IfTreeVif* find_vif(const string& vifname) const;

    /**
     * Find a vif for a given physical index.
     *
     * @param ifindex the interface index to search for.
     * @return a pointer to the interface (@see IfTreeInterface) or NULL
     * if not found.
     */
    IfTreeVif* find_vif(uint32_t ifindex);

    /**
     * Find a const vif for a given physical index.
     *
     * @param ifindex the interface index to search for.
     * @return a const pointer to the interface (@see IfTreeInterface) or NULL
     * if not found.
     */
    const IfTreeVif* find_vif(uint32_t ifindex) const;

    /**
     * Find an IPv4 address.
     *
     * @param vifname the vif name to search for.
     * @param addr the address to search for.
     * @return a pointer to the vif (@see IfTreeAddr4) or NULL if not found.
     */
    IfTreeAddr4* find_addr(const string& vifname, const IPv4& addr);

    /**
     * Find a const IPv4 address.
     *
     * @param vifname the vif name to search for.
     * @param addr the address to search for.
     * @return a const pointer to the vif (@see IfTreeAddr4) or NULL
     * if not found.
     */
    const IfTreeAddr4* find_addr(const string& vifname,
				 const IPv4& addr) const;

    /**
     * Find an IPv6 address.
     *
     * @param vifname the vif name to search for.
     * @param addr the address to search for.
     * @return a pointer to the vif (@see IfTreeAddr6) or NULL if not found.
     */
    IfTreeAddr6* find_addr(const string& vifname, const IPv6& addr);

    /**
     * Find a const IPv6 address.
     *
     * @param vifname the vif name to search for.
     * @param addr the address to search for.
     * @return a pointer to the vif (@see IfTreeAddr6) or NULL if not found.
     */
    const IfTreeAddr6* find_addr(const string& vifname,
				 const IPv6& addr) const;

    /**
     * Copy state of internal variables from another IfTreeInterface.
     */
    inline void copy_state(const IfTreeInterface& o)
    {
	set_pif_index(o.pif_index());
	set_enabled(o.enabled());
	set_mtu(o.mtu());
	set_mac(o.mac());
	set_no_carrier(o.no_carrier());
	set_flipped(o.flipped());
	set_interface_flags(o.interface_flags());
    }

    /**
     * Test if the interface-specific internal state is same.
     *
     * @param o the IfTreeInterface to compare against.
     * @return true if the interface-specific internal state is same.
     */
    inline bool is_same_state(const IfTreeInterface& o)
    {
	return ((pif_index() == o.pif_index())
		&& (enabled() == o.enabled())
		&& (mtu() == o.mtu())
		&& (mac() == o.mac())
		&& (no_carrier() == o.no_carrier())
		&& (flipped() == o.flipped())
		&& (interface_flags() == o.interface_flags()));
    }

    void finalize_state();

    string str() const;

protected:
    const string _ifname;
    uint32_t	 _pif_index;
    bool 	 _enabled;
    bool	 _discard;
    bool	 _is_discard_emulated;
    uint32_t 	 _mtu;
    Mac 	 _mac;
    bool	 _no_carrier;
    bool	 _flipped;	// If true, interface -> down, then -> up
    uint32_t	 _interface_flags;	// The system-specific interface flags
    VifMap	 _vifs;
};


/**
 * Fea class for virtual (logical) interface state.
 */
class IfTreeVif : public IfTreeItem {
public:
    typedef map<const IPv4, IfTreeAddr4> IPv4Map;
    typedef map<const IPv6, IfTreeAddr6> IPv6Map;

    IfTreeVif(const string& ifname, const string& vifname);

    const string& ifname() const	{ return _ifname; }

    const string& vifname() const	{ return _vifname; }

    inline uint32_t pif_index() const	{ return _pif_index; }
    inline void set_pif_index(uint32_t v) { _pif_index = v; mark(CHANGED); }

    inline bool enabled() const		{ return _enabled; }
    inline bool broadcast() const	{ return _broadcast; }
    inline bool loopback() const	{ return _loopback; }
    inline bool point_to_point() const	{ return _point_to_point; }
    inline bool multicast() const	{ return _multicast; }
    inline bool pim_register() const	{ return _pim_register; }

    inline void set_enabled(bool en)	{ _enabled = en; mark(CHANGED); }
    inline void set_broadcast(bool v)	{ _broadcast = v; mark(CHANGED); }
    inline void set_loopback(bool v)	{ _loopback = v; mark(CHANGED); }
    inline void set_point_to_point(bool v) { _point_to_point = v; mark(CHANGED); }
    inline void set_multicast(bool v)	{ _multicast = v; mark(CHANGED); }
    inline void set_pim_register(bool v) { _pim_register = v; mark(CHANGED); }

    inline const IPv4Map& ipv4addrs() const { return _ipv4addrs; }
    inline IPv4Map& ipv4addrs()		{ return _ipv4addrs; }

    inline const IPv6Map& ipv6addrs() const { return _ipv6addrs; }
    inline IPv6Map& ipv6addrs()		{ return _ipv6addrs; }

    /**
     * Add IPv4 address.
     *
     * @param addr address to be added.
     *
     * @return true on success, false if an error.
     */
    bool add_addr(const IPv4& addr);

    /**
     * Mark IPv4 address as DELETED.
     *
     * Deletion occurs when finalize_state is called.
     *
     * @param addr address to labelled.
     *
     * @return true on success, false if address does not exist.
     */
    bool remove_addr(const IPv4& addr);

    /**
     * Add IPv6 address.
     *
     * @param addr address to be added.
     *
     * @return true on success, false if an error.
     */
    bool add_addr(const IPv6& addr);

    /**
     * Mark IPv6 address as DELETED.
     *
     * Deletion occurs when finalize_state is called.
     *
     * @param addr address to labelled.
     *
     * @return true on success, false if address does not exist.
     */
    bool remove_addr(const IPv6& addr);

    /**
     * Find an IPv4 address.
     *
     * @param addr the address to search for.
     * @return a pointer to the vif (@see IfTreeAddr4) or NULL if not found.
     */
    IfTreeAddr4* find_addr(const IPv4& addr);

    /**
     * Find a const IPv4 address.
     *
     * @param addr the address to search for.
     * @return a const pointer to the vif (@see IfTreeAddr4) or NULL
     * if not found.
     */
    const IfTreeAddr4* find_addr(const IPv4& addr) const;

    /**
     * Find an IPv6 address.
     *
     * @param addr the address to search for.
     * @return a pointer to the vif (@see IfTreeAddr6) or NULL if not found.
     */
    IfTreeAddr6* find_addr(const IPv6& addr);

    /**
     * Find a const IPv6 address.
     *
     * @param addr the address to search for.
     * @return a pointer to the vif (@see IfTreeAddr6) or NULL if not found.
     */
    const IfTreeAddr6* find_addr(const IPv6& addr) const;

    /**
     * Copy state of internal variables from another IfTreeVif.
     */
    inline void copy_state(const IfTreeVif& o)
    {
	set_pif_index(o.pif_index());
	set_enabled(o.enabled());
	set_broadcast(o.broadcast());
	set_loopback(o.loopback());
	set_point_to_point(o.point_to_point());
	set_multicast(o.multicast());
	set_pim_register(o.pim_register());
    }

    /**
     * Test if the vif-specific internal state is same.
     *
     * @param o the IfTreeVif to compare against.
     * @return true if the vif-specific internal state is same.
     */
    inline bool is_same_state(const IfTreeVif& o)
    {
	return ((pif_index() == o.pif_index())
		&& (enabled() == o.enabled())
		&& (broadcast() == o.broadcast())
		&& (loopback() == o.loopback())
		&& (point_to_point() == o.point_to_point())
		&& (multicast() == o.multicast())
		&& (pim_register() == o.pim_register()));
    }

    void finalize_state();

    string str() const;

protected:
    const string _ifname;
    const string _vifname;

    uint32_t	_pif_index;
    bool 	_enabled;
    bool	_broadcast;
    bool	_loopback;
    bool	_point_to_point;
    bool	_multicast;
    bool	_pim_register;

    IPv4Map	 _ipv4addrs;
    IPv6Map	 _ipv6addrs;
};


/**
 * Class for holding an IPv4 interface address and address related items.
 */
class IfTreeAddr4 : public IfTreeItem {
public:
    IfTreeAddr4(const IPv4& addr)
	: IfTreeItem(), _addr(addr), _enabled(false), _broadcast(false),
	  _loopback(false), _point_to_point(false), _multicast(false),
	  _prefix_len(0)
    {}

    inline const IPv4& addr() const	{ return _addr; }

    inline bool enabled() const		{ return _enabled; }
    inline bool broadcast() const	{ return _broadcast; }
    inline bool loopback() const	{ return _loopback; }
    inline bool point_to_point() const	{ return _point_to_point; }
    inline bool multicast() const	{ return _multicast; }

    inline void set_enabled(bool en)	{ _enabled = en; mark(CHANGED); }
    inline void set_broadcast(bool v)	{ _broadcast = v; mark(CHANGED); }
    inline void set_loopback(bool v)	{ _loopback = v; mark(CHANGED); }
    inline void set_point_to_point(bool v) { _point_to_point = v; mark(CHANGED); }
    inline void set_multicast(bool v)	{ _multicast = v; mark(CHANGED); }

    /**
     * Get prefix length associates with address.
     */
    inline uint32_t prefix_len() const	{ return _prefix_len; }

    /**
     * Set prefix length associate with address.
     * @return true on success, false if prefix length is invalid.
     */
    bool set_prefix_len(uint32_t prefix_len);

    /**
     * Get the broadcast address.
     * @return the broadcast address or IPv4::ZERO() if there is no
     * broadcast address set.
     */
    IPv4 bcast() const;

    /**
     * Set the broadcast address.
     * @param baddr the broadcast address.
     */
    void set_bcast(const IPv4& baddr);

    /**
     * Get the endpoint address of a point-to-point link.
     * @return the broadcast address or IPv4::ZERO() if there is no
     * broadcast address set.
     */
    IPv4 endpoint() const;

    /**
     * Set the endpoint address of a point-to-point link.
     * @param oaddr the endpoint address.
     */
    void set_endpoint(const IPv4& oaddr);

    /**
     * Copy state of internal variables from another IfTreeAddr4.
     */
    inline void copy_state(const IfTreeAddr4& o)
    {
	set_enabled(o.enabled());
	set_broadcast(o.broadcast());
	set_loopback(o.loopback());
	set_point_to_point(o.point_to_point());
	set_multicast(o.multicast());
	if (o.broadcast())
	    set_bcast(o.bcast());
	if (o.point_to_point())
	    set_endpoint(o.endpoint());
	set_prefix_len(o.prefix_len());
    }

    /**
     * Test if the address-specific internal state is same.
     *
     * @param o the IfTreeAddr4 to compare against.
     * @return true if the address-specific internal state is same.
     */
    inline bool is_same_state(const IfTreeAddr4& o)
    {
	return ((enabled() == o.enabled())
		&& (broadcast() == o.broadcast())
		&& (loopback() == o.loopback())
		&& (point_to_point() == o.point_to_point())
		&& (multicast() == o.multicast())
		&& (bcast() == o.bcast())
		&& (endpoint() == o.endpoint())
		&& (prefix_len() == o.prefix_len()));
    }

    void finalize_state();

    string str() const;

protected:
    IPv4	_addr;

    bool 	_enabled;
    bool	_broadcast;
    bool	_loopback;
    bool	_point_to_point;
    bool	_multicast;

    IPv4	_oaddr;		// Other address - p2p endpoint or bcast addr
    uint32_t	_prefix_len;	// The prefix length
};


/**
 * Class for holding an IPv6 interface address and address related items.
 */
class IfTreeAddr6 : public IfTreeItem
{
public:
    IfTreeAddr6(const IPv6& addr)
	: IfTreeItem(), _addr(addr), _enabled(false),
	  _loopback(false), _point_to_point(false), _multicast(false),
	  _prefix_len(0)
    {}

    const IPv6& addr() const		{ return _addr; }

    inline bool enabled() const		{ return _enabled; }
    inline bool loopback() const	{ return _loopback; }
    inline bool point_to_point() const	{ return _point_to_point; }
    inline bool multicast() const	{ return _multicast; }

    inline void set_enabled(bool en)	{ _enabled = en; mark(CHANGED); }
    inline void set_loopback(bool v)	{ _loopback = v; mark(CHANGED); }
    inline void set_point_to_point(bool v) { _point_to_point = v; mark(CHANGED); }
    inline void set_multicast(bool v)	{ _multicast = v; mark(CHANGED); }

    /**
     * Get prefix length associated with address.
     */
    inline uint32_t prefix_len() const	{ return _prefix_len; }

    /**
     * Set prefix length associate with address.
     * @return true on success, false if prefix length is invalid.
     */
    bool set_prefix_len(uint32_t prefix_len);

    IPv6 endpoint() const;

    void set_endpoint(const IPv6& oaddr);

    /**
     * Copy state of internal variables from another IfTreeAddr6.
     */
    inline void copy_state(const IfTreeAddr6& o)
    {
	set_enabled(o.enabled());
	set_loopback(o.loopback());
	set_point_to_point(o.point_to_point());
	set_multicast(o.multicast());
	if (o.point_to_point())
	    set_endpoint(o.endpoint());
	set_prefix_len(o.prefix_len());
    }

    /**
     * Test if the address-specific internal state is same.
     *
     * @param o the IfTreeAddr6 to compare against.
     * @return true if the address-specific internal state is same.
     */
    inline bool is_same_state(const IfTreeAddr6& o)
    {
	return ((enabled() == o.enabled())
		&& (loopback() == o.loopback())
		&& (point_to_point() == o.point_to_point())
		&& (multicast() == o.multicast())
		&& (endpoint() == o.endpoint())
		&& (prefix_len() == o.prefix_len()));
    }

    void finalize_state();

    string str() const;

protected:
    IPv6	_addr;

    bool 	_enabled;
    bool	_loopback;
    bool	_point_to_point;
    bool	_multicast;

    IPv6	_oaddr;		// Other address - p2p endpoint
    uint32_t	_prefix_len;	// The prefix length
};

#endif // __FEA_IFTREE_HH__
