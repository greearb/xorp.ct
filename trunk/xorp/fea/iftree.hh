// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/fea/iftree.hh,v 1.24 2004/11/05 04:11:04 bms Exp $

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
    bool add_if(const string& ifname);

    /**
     * Label interface as ready for deletion.  Deletion does not occur
     * until finalize_state() is called.
     *
     * @param ifname name of interface to be labelled.
     *
     * @return true on success, false if ifname is invalid.
     */
    bool remove_if(const string& ifname);

    /**
     * Create a new interface or update its state if it already exists.
     *
     * @param other_iface the interface with the state to copy from.
     *
     * @return true on success, false if an error.
     */
    bool update_if(const IfTreeInterface& other_iface);

    /**
     * Get iterator of corresponding to named interface.
     *
     * @param ifname interface name to find iterator for.
     *
     * @return iterator, will be equal to ifs().end() if invalid.
     */
    inline IfMap::iterator get_if(const string& ifname);

    /**
     * Get iterator of corresponding to named interface.
     *
     * @param ifname interface name to find iterator for.
     *
     * @return iterator, will be equal to ifs().end() if invalid.
     */
    inline IfMap::const_iterator get_if(const string& ifname) const;

    inline const IfMap& ifs() const { return _ifs; }

    inline IfMap& ifs() { return _ifs; }

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
     * - If the item in the local tree is "disabled", then the state is not
     *   modified. Otherwise, the rules below are applied.
     * - If an item from the local tree is not in the other tree,
     *   it is marked as deleted in the local tree.
     * - If an item from the local tree is in the other tree,
     *   its state is copied from the other tree to the local tree.
     * - If an item from the other tree is not in the local tree, we do NOT
     *   copy it to the local tree.
     *
     * @param other the configuration tree to align state with.
     * @return modified configuration structure.
     */
    IfTree& align_with(const IfTree& other);

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
     * Delete interfaces labelled as ready for deletion, call finalize_state()
     * on remaining interfaces, and set state to NO_CHANGE.
     */
    void finalize_state();

    /**
     * @return string representation of IfTree.
     */
    string str() const;

protected:
    IfMap	_ifs;
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

    inline uint16_t pif_index() const	{ return _pif_index; }
    inline void set_pif_index(uint16_t v) { _pif_index = v; mark(CHANGED); }

    inline bool enabled() const		{ return _enabled; }

    inline void set_enabled(bool en)	{ _enabled = en; mark(CHANGED); }

    inline uint32_t mtu() const		{ return _mtu; }

    inline void set_mtu(uint32_t mtu)	{ _mtu = mtu; mark(CHANGED); }

    inline const Mac& mac() const	{ return _mac; }

    inline void set_mac(const Mac& mac)	{ _mac = mac; mark(CHANGED); }

    inline bool discard() const		{ return _discard; }

    inline void set_discard(bool discard) {
	if (discard)
	    _soft = true;
	_discard = discard;
	mark(CHANGED);
    }

    /**
     * Get the system-specific interface flags.
     *
     * Typically, this value is read from the underlying system, and is
     * used only for internal purpose.
     *
     * @return the system-specific interface flags.
     */
    inline uint32_t if_flags() const	{ return _if_flags; }

    /**
     * Store the system-specific interface flags.
     *
     * Typically, this value is read from the underlying system, and is
     * used only for internal purpose.
     *
     * @param if_flags the value of the system-specific interface flags to
     * store.
     */
    inline void set_if_flags(uint32_t if_flags) { _if_flags = if_flags; mark(CHANGED); }

    inline const VifMap& vifs() const	{ return _vifs; }
    inline VifMap& vifs()		{ return _vifs; }

    bool add_vif(const string& vifname);

    bool remove_vif(const string& vifname);

    inline VifMap::iterator get_vif(const string& vifname);

    inline VifMap::const_iterator get_vif(const string& vifname) const;

    /**
     * Copy state of internal variables from another IfTreeInterface.
     */
    inline void copy_state(const IfTreeInterface& o)
    {
	set_pif_index(o.pif_index());
	set_enabled(o.enabled());
	set_mtu(o.mtu());
	set_mac(o.mac());
	set_if_flags(o.if_flags());
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
		&& (if_flags() == o.if_flags()));
    }

    void finalize_state();

    string str() const;

protected:
    const string _ifname;
    uint16_t	 _pif_index;
    bool 	 _enabled;
    bool	 _discard;
    uint32_t 	 _mtu;
    Mac 	 _mac;
    uint32_t	 _if_flags;	// The system-specific interface flags
    VifMap	 _vifs;
};


/**
 * Fea class for virtual (logical) interface state.
 */
class IfTreeVif : public IfTreeItem {
public:
    typedef map<const IPv4, IfTreeAddr4> V4Map;
    typedef map<const IPv6, IfTreeAddr6> V6Map;

    IfTreeVif(const string& ifname, const string& vifname);

    const string& ifname() const	{ return _ifname; }

    const string& vifname() const	{ return _vifname; }

    inline uint16_t pif_index() const	{ return _pif_index; }
    inline void set_pif_index(uint16_t v) { _pif_index = v; mark(CHANGED); }

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

    inline const V4Map& v4addrs() const	{ return _v4addrs; }
    inline V4Map& v4addrs()		{ return _v4addrs; }

    inline const V6Map& v6addrs() const	{ return _v6addrs; }
    inline V6Map& v6addrs()		{ return _v6addrs; }

    inline V4Map::iterator get_addr(const IPv4& a);

    inline V6Map::iterator get_addr(const IPv6& a);

    inline V4Map::const_iterator get_addr(const IPv4& a) const;

    inline V6Map::const_iterator get_addr(const IPv6& a) const;

    /**
     * Add address.
     *
     * @param v4addr address to be added.
     *
     * @return true on success, false if an error.
     */
    bool add_addr(const IPv4& v4addr);

    /**
     * Mark address as DELETED.  Deletion occurs when finalize_state is called.
     *
     * @param v4addr address to labelled.
     *
     * @return true on success, false if address does not exist.
     */
    bool remove_addr(const IPv4& v4addr);

    /**
     * Add address.
     *
     * @param v6addr address to be added.
     *
     * @return true on success, false if an error.
     */
    bool add_addr(const IPv6& v6addr);

    /**
     * Mark address as DELETED.  Deletion occurs when finalize_state is called.
     *
     * @param v6addr address to labelled.
     *
     * @return true on success, false if address does not exist.
     */
    bool remove_addr(const IPv6& v6addr);

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
		&& (multicast() == o.multicast()));
    }

    void finalize_state();

    string str() const;

protected:
    const string _ifname;
    const string _vifname;

    uint16_t	 _pif_index;
    bool 	 _enabled;
    bool	 _broadcast;
    bool	 _loopback;
    bool	 _point_to_point;
    bool	 _multicast;

    V4Map	 _v4addrs;
    V6Map	 _v6addrs;
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


//
// Inline IfTree methods
//

inline IfTree::IfMap::iterator
IfTree::get_if(const string& ifname)
{
    return _ifs.find(ifname);
}

inline IfTree::IfMap::const_iterator
IfTree::get_if(const string& ifname) const
{
    return _ifs.find(ifname);
}

//
// Inline IfTreeInterface methods
//

inline IfTreeInterface::VifMap::iterator
IfTreeInterface::get_vif(const string& vifname)
{
    return _vifs.find(vifname);
}

inline
IfTreeInterface::VifMap::const_iterator
IfTreeInterface::get_vif(const string& vifname) const
{
    return _vifs.find(vifname);
}

//
// Inline IfTreeVif methods
//

inline IfTreeVif::V4Map::iterator
IfTreeVif::get_addr(const IPv4& a)
{
    return _v4addrs.find(a);
}

inline IfTreeVif::V6Map::iterator
IfTreeVif::get_addr(const IPv6& a)
{
    return _v6addrs.find(a);
}

inline IfTreeVif::V4Map::const_iterator
IfTreeVif::get_addr(const IPv4& a) const
{
    return _v4addrs.find(a);
}

inline IfTreeVif::V6Map::const_iterator
IfTreeVif::get_addr(const IPv6& a) const
{
    return _v6addrs.find(a);
}

#endif // __FEA_IFTREE_HH__
