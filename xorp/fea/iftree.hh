// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2012 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
//
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net


#ifndef __FEA_IFTREE_HH__
#define __FEA_IFTREE_HH__

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"
#include "libxorp/mac.hh"

class IPvX;

/**
 * Base class for FEA configurable items where the modifications need
 * to be held over and propagated later, ie changes happen during a
 * transaction but are propagated during the commit.
 */
class IfTreeItem :
    public NONCOPYABLE
{
public:
    IfTreeItem() : _st(CREATED), _soft(false) {}
    virtual ~IfTreeItem() {}

public:
    enum State {
	NO_CHANGE	= 0x00,
	CREATED		= 0x01,
	DELETED		= 0x02,
	CHANGED		= 0x04
    };

    int set_state(State st) {
	if (bits(st) > 1) {
	    return (XORP_ERROR);
	}
	_st = st;
	return (XORP_OK);
    }

    State state() const 		{ return _st; }

    virtual int mark(State st) {
	if (bits(st) > 1) {
	    return (XORP_ERROR);
	}
	if (st & (CREATED | DELETED)) {
	    _st = st;
	    return (XORP_OK);
	}
	if (_st & (CREATED | DELETED)) {
	    return (XORP_OK);
	}
	_st = st;
	return (XORP_OK);
    }
    bool is_marked(State st) const	{ return st == _st; }

    void set_soft(bool en)		{ _soft = en; }

    bool is_soft() const		{ return _soft; }

    /**
     * Virtual method to be implemented to flush out state associated
     * objects, ie if an object is marked CREATED or CHANGED it should be
     * marked NO_CHANGE, if an object is marked DELETED, it should be
     * removed from the relevant container and destructed.
     */
    virtual void finalize_state() = 0;

    string str() const;

protected:
    static uint32_t bits(State st) {
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


enum IfTreeIfaceEventE {
    IFTREE_DELETE_IFACE, // marked deleted
    IFTREE_ERASE_IFACE //erased entirely
};

enum IfTreeVifEventE {
    IFTREE_DELETE_VIF, // marked deleted
    IFTREE_ERASE_VIF //erased entirely
};

/** IfTree will make these callbacks to listeners when certain actions
 * occur.
 */
class IfTreeListener {
public:
    virtual ~IfTreeListener() { }

    virtual void notifyDeletingIface(const string& ifname) = 0; // marking iface deleted
    virtual void notifyErasingIface(const string& ifname) = 0; // erasing iface memory

    // These may not be called if the parrent iface is being deleted, so
    // alwaysclean up all VIFs in the iface handler.
    virtual void notifyDeletingVif(const string& ifname, const string& vifname) = 0; // marking vif deleted
    virtual void notifyErasingVif(const string& ifname, const string& vifname) = 0; // erasing vif memory

    // Add more as are needed.
};

/**
 * Container class for FEA Interface objects in a system.
 */
class IfTree : public IfTreeItem {
public:
    typedef map<string, IfTreeInterface*> IfMap;
    typedef map<uint32_t, IfTreeInterface*> IfIndexMap;
    //
    // XXX: We use a multimap for the index->vif mapping, because a VLAN
    // vif could be listed in two places: with its parent physical interface,
    // or along as its own parent interface.
    //
    typedef multimap<uint32_t, IfTreeVif*> VifIndexMap;

    /**
     * Default constructor.
     */
    IfTree(const char* tree_name);

    /**
     * Constructor from another IfTree.
     *
     * @param other the other IfTree.
     */
    IfTree(const IfTree& other);

    /**
     * Destructor.
     */
    virtual ~IfTree();

    /**
     * Assignment operator.
     *
     * @param other the other IfTree.
     */
    IfTree& operator=(const IfTree& other);

    /**
     * Remove all interface state from the interface tree.
     */
    void clear();

    /** Not really const, but better than having to pass non-const
     * references to other classes.
     */
    void registerListener(IfTreeListener* l) const;

    /** Not really const, but better than having to pass non-const
     * references to other classes.
     */
    void unregisterListener(IfTreeListener* l) const;


    /**
     * Add recursively a new interface.
     *
     * @param other_iface the interface to add recursively.
     * @param mark_state if true, then mark the state same as the state
     * from the other interface, otherwise the state will be CREATED.
     */
    void add_recursive_interface(const IfTreeInterface& other_iface,
				 bool mark_state);

    /**
     * Create a new interface.
     *
     * @param ifname the interface name.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_interface(const string& ifname);

    /**
     * Label interface as ready for deletion. Deletion does not occur
     * until finalize_state() is called.
     *
     * @param ifname the name of the interface to be labelled.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int remove_interface(const string& ifname);

    /**
     * Recursively create a new interface or update its state if it already
     * exists.
     *
     * @param other_iface the interface with the state to copy from.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int update_interface(const IfTreeInterface& other_iface);

    /**
     * Find an interface.
     *
     * @param ifname the interface name to search for.
     * @return a pointer to the interface (@ref IfTreeInterface) or NULL
     * if not found.
     */
    IfTreeInterface* find_interface(const string& ifname);

    /**
     * Find a const interface.
     *
     * @param ifname the interface name to search for.
     * @return a const pointer to the interface (@ref IfTreeInterface) or NULL
     * if not found.
     */
    const IfTreeInterface* find_interface(const string& ifname) const;

    /**
     * Find an interface for a given physical index.
     *
     * @param pif_index the physical interface index to search for.
     * @return a pointer to the interface (@ref IfTreeInterface) or NULL
     * if not found.
     */
    IfTreeInterface* find_interface(uint32_t pif_index);

    /**
     * Find a const interface for a given physical index.
     *
     * @param pif_index the physical interface index to search for.
     * @return a const pointer to the interface (@ref IfTreeInterface) or NULL
     * if not found.
     */
    const IfTreeInterface* find_interface(uint32_t pif_index) const;

    /**
     * Find a vif.
     *
     * @param ifname the interface name to search for.
     * @param vifname the vif name to search for.
     * @return a pointer to the vif (@ref IfTreeVif) or NULL if not found.
     */
    IfTreeVif* find_vif(const string& ifname, const string& vifname);

    /**
     * Find a const vif.
     *
     * @param ifname the interface name to search for.
     * @param vifname the vif name to search for.
     * @return a const pointer to the vif (@ref IfTreeVif) or NULL
     * if not found.
     */
    const IfTreeVif* find_vif(const string& ifname,
			      const string& vifname) const;

    /**
     * Find a vif for a given physical index.
     *
     * @param pif_index the physical interface index to search for.
     * @return a pointer to the vif (@ref IfTreeVif) or NULL if not found.
     */
    IfTreeVif* find_vif(uint32_t pif_index);

    /**
     * Find a const vif for a given physical index.
     *
     * @param pif_index the physical interface index to search for.
     * @return a const pointer to the vif (@ref IfTreeVif) or NULL
     * if not found.
     */
    const IfTreeVif* find_vif(uint32_t pif_index) const;

    /**
     * Find an IPv4 address.
     *
     * @param ifname the interface name to search for.
     * @param vifname the vif name to search for.
     * @param addr the address to search for.
     * @return a pointer to the vif (@ref IfTreeAddr4) or NULL if not found.
     */
    IfTreeAddr4* find_addr(const string& ifname, const string& vifname,
			   const IPv4& addr);

    /**
     * Find a const IPv4 address.
     *
     * @param ifname the interface name to search for.
     * @param vifname the vif name to search for.
     * @param addr the address to search for.
     * @return a const pointer to the vif (@ref IfTreeAddr4) or NULL
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
     * @return a pointer to the vif (@ref IfTreeAddr6) or NULL if not found.
     */
    IfTreeAddr6* find_addr(const string& ifname, const string& vifname,
			   const IPv6& addr);

    /**
     * Find a const IPv6 address.
     *
     * @param ifname the interface name to search for.
     * @param vifname the vif name to search for.
     * @param addr the address to search for.
     * @return a pointer to the vif (@ref IfTreeAddr6) or NULL if not found.
     */
    const IfTreeAddr6* find_addr(const string& ifname, const string& vifname,
				 const IPv6& addr) const;

    /**
     * Find an interface and a vif by an address that belongs to that
     * interface and vif.
     *
     * @param addr the address.
     * @param ifp return-by-reference a pointer to the interface.
     * @param vifp return-by-reference a pointer to the vif.
     * @return true if a match is found, otherwise false.
     */
    bool find_interface_vif_by_addr(const IPvX& addr,
				    const IfTreeInterface*& ifp,
				    const IfTreeVif*& vifp) const;

    /**
     * Find an interface and a vif by an address that shares the same subnet
     * or p2p address.
     *
     * @param addr the address.
     * @param ifp return-by-reference a pointer to the interface.
     * @param vifp return-by-reference a pointer to the vif.
     * @return true if a match is found, otherwise false.
     */
    bool find_interface_vif_same_subnet_or_p2p(const IPvX& addr,
					       const IfTreeInterface*& ifp,
					       const IfTreeVif*& vifp) const;

    /**
     * Get the map with the stored interfaces.
     *
     * @return the map with the stored interfaces.
     */
    IfMap& interfaces() { return _interfaces; }

    /**
     * Get the const map with the stored interfaces.
     *
     * @return the const map with the stored interfaces.
     */
    const IfMap& interfaces() const { return _interfaces; }

    /**
     * Align system-user merged configuration with the pulled changes
     * in the system configuration.
     *
     * Inside the FEA there may be multiple configuration representations,
     * typically one the user modifies, one that mirrors the hardware,
     * and one that merges those two (e.g., some of the merged information
     * comes from the user configuration while other might come
     * from the underlying system).
     * Errors may occur pushing the user config down onto the hardware and
     * we need a method to update the merged config from the h/w config that
     * exists after the config push. We can't just copy the h/w config since
     * the user config is restricted to configuration set by the user.
     *
     * The alignment works as follows:
     * 1. If an interface in the local tree is marked as "soft", its
     *    state is not modified and the rest of the processing is ignored.
     * 2. If an interface in the local tree is marked as
     *    "default_system_config", the rest of the processing is not
     *    applied, and the following rules are used instead:
     *    (a) If the interface is not in the other tree, it is marked
     *        as "disabled" and its vifs are marked for deletion.
     *    (b) Otherwise, its state (and the subtree below it) is copied
     *        as-is from the other tree.
     * 3. If an item in the local tree is not in the other tree, it is
     *    marked as "disabled" in the local tree.
     * 4. If an item in the local tree is in the other tree, and its state
     *    is different in the local and the other tree, the state
     *    is copied from the other tree to the local tree.
     *    Also, if the item is disabled in the user config tree, it is
     *    marked as "disabled" in the local tree.
     *
     * @param other the configuration tree to align state with.
     * @param user_config the user configuration tree to reference during
     * the alignment.
     * @return modified configuration structure.
     */
    IfTree& align_with_pulled_changes(const IfTree& other,
				      const IfTree& user_config);

    /**
     * Align system-user merged configuration with the observed changes
     * in the system configuration.
     *
     * Inside the FEA there may be multiple configuration representations,
     * typically one the user modifies, one that mirrors the hardware,
     * and one that merges those two (e.g., some of the merged information
     * comes from the user configuration while other might come
     * from the underlying system).
     * On certain systems there could be asynchronous updates originated
     * by the system that are captured by the FEA interface observer
     * (e.g., a cable is unplugged, a tunnel interface is added/deleted, etc).
     *
     * This method is used to align those updates with the merged
     * configuration.
     *
     * 1. If an interface in the other tree is not in the local tree, it is
     *    tested whether is in the user configuration tree. If not, the
     *    rest of the processing is not applied and the interface is ignored.
     *    Otherwise it is created, its state is merged from the user config
     *    and other tree, and is marked as "CREATED".
     * 2. If an interface in the local tree is marked as "soft", its
     *    state is not modified and the rest of the processing is ignored.
     * 3. If an interface in the local tree is marked as
     *    "default_system_config", the rest of the processing is not
     *    applied, and its state (and the subtree below it) is copied
     *    as-is from the other tree.
     * 4. If an item in the other tree is not in the local tree, it is
     *    tested whether is in the user configuration tree. If not, the
     *    rest of the processing is not applied and the item is ignored.
     *    Otherwise it is created, its state is merged from the user config
     *    and the other tree, and is marked as "CREATED".
     * 5. If an item in the other tree is marked as:
     *    (a) "NO_CHANGE": The state of the entry in the other tree is not
     *        propagated to the local tree, but its subtree entries are
     *        processed.
     *    (b) "DELETED": The item in the local tree is disabled, and the
     *        subtree entries are ignored.
     *    (c) "CREATED" or "CHANGED": If the state of the entry is different
     *        in the other and the local tree, it is copied to the local tree,
     *        and the item in the local tree is marked as "CREATED" or
     *        "CHANGED".
     *        unless it was marked earlier as "CREATED".
     *        Also, if the item is disabled in the user config tree, it is
     *        marked as "disabled" in the local tree.
     *
     * @param other the configuration tree to align state with.
     * @param user_config the user configuration tree to reference during
     * the alignment.
     * @return modified configuration structure.
     */
    IfTree& align_with_observed_changes(const IfTree& other,
					const IfTree& user_config);

    /**
     * Align system-user merged configuration with the user configuration
     * changes.
     *
     * Inside the FEA there may be multiple configuration representations,
     * typically one the user modifies, one that mirrors the hardware,
     * and one that merges those two (e.g., some of the merged information
     * comes from the user configuration while other might come
     * from the underlying system).
     *
     * This method is used to align the user configuration changes with the
     * merged configuration.
     *
     * The alignment works as follows:
     * 1. If an item in the other tree is not in the local tree, it is
     *    created in the local tree and its state (and the subtree below it)
     *    is copied as-is from the other tree, and the rest of the processing
     *    is ignored.
     * 2. If an item in the other tree is marked as:
     *    (a) "DELETED": The item in the local tree is marked as "DELETED",
     *        and the subtree entries are ignored.
     *    (b) All other: If the state of the item is different in the other
     *        and the local tree, it is copied to the local tree.
     *        Note that we compare the state even for "NO_CHANGE" items in
     *        case a previous change to a parent item in the merged tree
     *        has affected the entry (e.g., disabled interface would
     *        disable the vifs and addresses as well).
     *
     * @param other the configuration tree to align state with.
     * @return modified configuration structure.
     */
    IfTree& align_with_user_config(const IfTree& other);

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
     *
     * @param other the configuration tree to be used to prepare the
     * replacement state.
     * @return modified configuration structure.
     */
    IfTree& prepare_replacement_state(const IfTree& other);

    /**
     * Prune bogus deleted state.
     *
     * If an item in the local tree is marked as deleted, but is not
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

    const string& getName() const { return name; }

    void markVifDeleted(IfTreeVif* ifp);
    void markIfaceDeleted(IfTreeInterface* ifp);

protected:
    friend class IfTreeInterface;
    friend class IfTreeVif;

    void insert_ifindex(IfTreeInterface* ifp);
    void erase_ifindex(IfTreeInterface* ifp);
    void insert_vifindex(IfTreeVif* vifp);
    void erase_vifindex(IfTreeVif* vifp);

    void sendEvent(IfTreeVifEventE e, IfTreeVif* vifp);
    void sendEvent(IfTreeIfaceEventE e, IfTreeInterface* ifp);

private:
    string name; // identifier for this tree
    IfMap	_interfaces;
    IfIndexMap	_ifindex_map;		// Map of pif_index to interface
    VifIndexMap	_vifindex_map;		// Map of pif_index to vif

    // Make this mutable so that we can past const references to other classes,
    // but still let them be listeners.
    mutable list<IfTreeListener*> listeners;
};


/**
 * FEA class for holding physical interface state.
 */
class IfTreeInterface :
    public IfTreeItem
{
public:
    typedef map<string, IfTreeVif*> VifMap;
    typedef set<Mac>			  MacSet;

    IfTreeInterface(IfTree& iftree, const string& ifname);
    virtual ~IfTreeInterface();

    IfTree& iftree()			{ return _iftree; }
    const string& ifname() const	{ return _ifname; }

    uint32_t pif_index() const		{ return _pif_index; }
    void set_pif_index(uint32_t v)	{
	iftree().erase_ifindex(this);
	_pif_index = v;
	mark(CHANGED);
	iftree().insert_ifindex(this);
    }

    virtual int mark(State st) {
	int rv = IfTreeItem::mark(st);
	if (st == DELETED) {
	    set_probed_vlan(false); // need to reprobe if this ever goes un-deleted
	}
	return rv;
    }

    // This relates to vlans.
    bool cr_by_xorp() const { return _created_by_xorp; }
    void set_cr_by_xorp(bool b) { _created_by_xorp = b; }

    bool probed_vlan() const { return _probed_vlan; }
    void set_probed_vlan(bool b) { _probed_vlan = b; }

    bool enabled() const		{ return _enabled; }
    void set_enabled(bool en)		{ _enabled = en; mark(CHANGED); }

    uint32_t mtu() const		{ return _mtu; }
    void set_mtu(uint32_t mtu)		{ _mtu = mtu; mark(CHANGED); }

    const Mac& mac() const		{ return _mac; }
    void set_mac(const Mac& mac)	{ _mac = mac; mark(CHANGED); }

    MacSet& macs()			{ return _macs; }
    const MacSet& macs() const		{ return _macs; }

    bool no_carrier() const		{ return _no_carrier; }
    void set_no_carrier(bool v)		{ _no_carrier = v; mark(CHANGED); }

    uint64_t baudrate() const		{ return _baudrate; }
    void set_baudrate(uint64_t v)	{ _baudrate = v; mark(CHANGED); }

    const string& parent_ifname() const { return _parent_ifname; }
    void set_parent_ifname(string& v) { if (v != _parent_ifname) { _parent_ifname = v; mark(CHANGED); }}

    bool is_vlan() const;
    const string& iface_type() const { return _iface_type; }
    void set_iface_type(string& v) { if (v != _iface_type) { _iface_type = v; mark(CHANGED); }}

    const string& vid() const { return _vid; }
    void set_vid(string& v) { if (v != _vid) { _vid = v; mark(CHANGED); }}

    bool discard() const		{ return _discard; }
    void set_discard(bool discard)	{ _discard = discard; mark(CHANGED); }

    bool unreachable() const		{ return _unreachable; }
    void set_unreachable(bool v)	{ _unreachable = v; mark(CHANGED); }

    bool management() const		{ return _management; }
    void set_management(bool v)		{ _management = v; mark(CHANGED); }

    bool default_system_config() const	{ return _default_system_config; }
    void set_default_system_config(bool v) { _default_system_config = v; mark(CHANGED); }

    /**
     * Get the system-specific interface flags.
     *
     * Typically, this value is read from the underlying system, and is
     * used only for internal purpose.
     *
     * @return the system-specific interface flags.
     */
    uint32_t interface_flags() const	{ return _interface_flags; }

    /**
     * Store the system-specific interface flags.
     *
     * Typically, this value is read from the underlying system, and is
     * used only for internal purpose.
     *
     * @param v the value of the system-specific interface flags to store.
     */
    void set_interface_flags(uint32_t v)	{
	_interface_flags = v;
	mark(CHANGED);
    }


    const VifMap& vifs() const		{ return _vifs; }
    VifMap& vifs()			{ return _vifs; }

    /**
     * Add recursively a new vif.
     *
     * @param other_vif the vif to add recursively.
     * @param mark_state if true, then mark the state same as the state
     * from the other vif, otherwise the state will be CREATED.
     */
    void add_recursive_vif(const IfTreeVif& other_vif, bool mark_state);

    /**
     * Create a new vif.
     *
     * @param vifname the vif name.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_vif(const string& vifname);

    /**
     * Label vif as ready for deletion. Deletion does not occur
     * until finalize_state() is called.
     *
     * @param vifname the name of the vif to be labelled.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int remove_vif(const string& vifname);

    /**
     * Find a vif.
     *
     * @param vifname the vif name to search for.
     * @return a pointer to the vif (@ref IfTreeVif) or NULL if not found.
     */
    IfTreeVif* find_vif(const string& vifname);

    /**
     * Find a const vif.
     *
     * @param vifname the vif name to search for.
     * @return a const pointer to the vif (@ref IfTreeVif) or NULL
     * if not found.
     */
    const IfTreeVif* find_vif(const string& vifname) const;

    /**
     * Find a vif for a given physical index.
     *
     * @param pif_index the physical interface index to search for.
     * @return a pointer to the interface (@ref IfTreeVif) or NULL
     * if not found.
     */
    IfTreeVif* find_vif(uint32_t pif_index);

    /**
     * Find a const vif for a given physical index.
     *
     * @param pif_index the physical interface index to search for.
     * @return a const pointer to the interface (@ref IfTreeVif) or NULL
     * if not found.
     */
    const IfTreeVif* find_vif(uint32_t pif_index) const;

    /**
     * Find an IPv4 address.
     *
     * @param vifname the vif name to search for.
     * @param addr the address to search for.
     * @return a pointer to the vif (@ref IfTreeAddr4) or NULL if not found.
     */
    IfTreeAddr4* find_addr(const string& vifname, const IPv4& addr);

    /**
     * Find a const IPv4 address.
     *
     * @param vifname the vif name to search for.
     * @param addr the address to search for.
     * @return a const pointer to the vif (@ref IfTreeAddr4) or NULL
     * if not found.
     */
    const IfTreeAddr4* find_addr(const string& vifname,
				 const IPv4& addr) const;

    /**
     * Find an IPv6 address.
     *
     * @param vifname the vif name to search for.
     * @param addr the address to search for.
     * @return a pointer to the vif (@ref IfTreeAddr6) or NULL if not found.
     */
    IfTreeAddr6* find_addr(const string& vifname, const IPv6& addr);

    /**
     * Find a const IPv6 address.
     *
     * @param vifname the vif name to search for.
     * @param addr the address to search for.
     * @return a pointer to the vif (@ref IfTreeAddr6) or NULL if not found.
     */
    const IfTreeAddr6* find_addr(const string& vifname,
				 const IPv6& addr) const;

    /**
     * Copy state of internal variables from another IfTreeInterface.
     *
     * @param o the interface to copy from.
     * @param copy_user_config if true then copy the flags from the
     * user's configuration.
     */
    void copy_state(const IfTreeInterface& o, bool copy_user_config);

    /**
     * Test if the interface-specific internal state is same.
     *
     * @param o the IfTreeInterface to compare against.
     * @return true if the interface-specific internal state is same.
     */
    bool is_same_state(const IfTreeInterface& o);

    void finalize_state();

    string str() const;

private:
    IfTree&	_iftree;
    const string _ifname;

    /* virtual interface info */
    string _parent_ifname;
    string _iface_type;
    string _vid;

    uint32_t	_pif_index;
    bool        _created_by_xorp; // for vlans
    bool        _probed_vlan;
    bool 	_enabled;
    bool	_discard;
    bool	_unreachable;
    bool	_management;
    bool	_default_system_config;
    uint32_t 	_mtu;
    Mac 	_mac;
    bool	_no_carrier;
    uint64_t	_baudrate;		// The link baudrate
    uint32_t	_interface_flags;	// The system-specific interface flags
    VifMap	_vifs;
    MacSet	_macs; // XXX: not part of user config, but used by processes
};


/**
 * FEA class for virtual (logical) interface state.
 */
class IfTreeVif :
    public IfTreeItem
{
public:
    typedef map<IPv4, IfTreeAddr4*> IPv4Map;
    typedef map<IPv6, IfTreeAddr6*> IPv6Map;

    IfTreeVif(IfTreeInterface& iface, const string& vifname);
    virtual ~IfTreeVif();

    IfTree& iftree()			{ return _iface.iftree(); }
    const string& ifname() const	{ return _iface.ifname(); }
    const string& vifname() const	{ return _vifname; }

    uint32_t pif_index() const		{ return _pif_index; }
    void set_pif_index(uint32_t v)	{
	iftree().erase_vifindex(this);
	_pif_index = v;
	mark(CHANGED);
	iftree().insert_vifindex(this);
    }

    uint32_t vif_index() const		{ return _vif_index; }
    void set_vif_index(uint32_t v)	{ _vif_index = v; mark(CHANGED); }

    bool enabled() const		{ return _enabled; }
    bool broadcast() const		{ return _broadcast; }
    bool loopback() const		{ return _loopback; }
    bool point_to_point() const		{ return _point_to_point; }
    bool multicast() const		{ return _multicast; }
    bool pim_register() const		{ return _pim_register; }

    void set_enabled(bool en)		{ _enabled = en; mark(CHANGED); }
    void set_broadcast(bool v)		{ _broadcast = v; mark(CHANGED); }
    void set_loopback(bool v)		{ _loopback = v; mark(CHANGED); }
    void set_point_to_point(bool v)	{ _point_to_point = v; mark(CHANGED); }
    void set_multicast(bool v)		{ _multicast = v; mark(CHANGED); }
    void set_pim_register(bool v)	{ _pim_register = v; mark(CHANGED); }

    /**
     * Get the system-specific vif flags.
     *
     * Typically, this value is read from the underlying system, and is
     * used only for internal purpose.
     *
     * @return the system-specific vif flags.
     */
    uint32_t vif_flags() const		{ return _vif_flags; }

    /**
     * Store the system-specific vif flags.
     *
     * Typically, this value is read from the underlying system, and is
     * used only for internal purpose.
     *
     * @param v the value of the system-specific vif flags to store.
     */
    void set_vif_flags(uint32_t v)	{ _vif_flags = v; mark(CHANGED); }

    const IPv4Map& ipv4addrs() const	{ return _ipv4addrs; }
    IPv4Map& ipv4addrs()		{ return _ipv4addrs; }

    const IPv6Map& ipv6addrs() const	{ return _ipv6addrs; }
    IPv6Map& ipv6addrs()		{ return _ipv6addrs; }

    /**
     * Copy recursively from another vif.
     *
     * @param other_vif the vif to copy recursively from.
     */
    void copy_recursive_vif(const IfTreeVif& other_vif);

    /**
     * Add recursively a new IPv4 address.
     *
     * @param other_addr the address to add recursively.
     * @param mark_state if true, then mark the state same as the state
     * from the other address, otherwise the state will be CREATED.
     */
    void add_recursive_addr(const IfTreeAddr4& other_addr, bool mark_state);

    /**
     * Add recursively a new IPv6 address.
     *
     * @param other_addr the address to add recursively.
     * @param mark_state if true, then mark the state same as the state
     * from the other address, otherwise the state will be CREATED.
     */
    void add_recursive_addr(const IfTreeAddr6& other_addr, bool mark_state);

    /**
     * Add IPv4 address.
     *
     * @param addr address to be added.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_addr(const IPv4& addr);

    /**
     * Mark IPv4 address as DELETED.
     *
     * Deletion occurs when finalize_state is called.
     *
     * @param addr address to labelled.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int remove_addr(const IPv4& addr);

    /**
     * Add IPv6 address.
     *
     * @param addr address to be added.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int add_addr(const IPv6& addr);

    /**
     * Mark IPv6 address as DELETED.
     *
     * Deletion occurs when finalize_state is called.
     *
     * @param addr address to labelled.
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int remove_addr(const IPv6& addr);

    /**
     * Find an IPv4 address.
     *
     * @param addr the address to search for.
     * @return a pointer to the vif (@ref IfTreeAddr4) or NULL if not found.
     */
    IfTreeAddr4* find_addr(const IPv4& addr);

    /**
     * Find a const IPv4 address.
     *
     * @param addr the address to search for.
     * @return a const pointer to the vif (@ref IfTreeAddr4) or NULL
     * if not found.
     */
    const IfTreeAddr4* find_addr(const IPv4& addr) const;

    /**
     * Find an IPv6 address.
     *
     * @param addr the address to search for.
     * @return a pointer to the vif (@ref IfTreeAddr6) or NULL if not found.
     */
    IfTreeAddr6* find_addr(const IPv6& addr);

    /**
     * Find a const IPv6 address.
     *
     * @param addr the address to search for.
     * @return a pointer to the vif (@ref IfTreeAddr6) or NULL if not found.
     */
    const IfTreeAddr6* find_addr(const IPv6& addr) const;

    /**
     * Propagate vif flags to the addresses.
     */
    void propagate_flags_to_addresses();

    /**
     * Copy state of internal variables from another IfTreeVif.
     */
    void copy_state(const IfTreeVif& o) {
	set_pif_index(o.pif_index());
	set_vif_index(o.vif_index());
	set_enabled(o.enabled());
	set_broadcast(o.broadcast());
	set_loopback(o.loopback());
	set_point_to_point(o.point_to_point());
	set_multicast(o.multicast());
	set_pim_register(o.pim_register());
	set_vif_flags(o.vif_flags());
    }

    /**
     * Test if the vif-specific internal state is same.
     *
     * @param o the IfTreeVif to compare against.
     * @return true if the vif-specific internal state is same.
     */
    bool is_same_state(const IfTreeVif& o) {
	return ((pif_index() == o.pif_index())
		&& (vif_index() == o.vif_index())
		&& (enabled() == o.enabled())
		&& (broadcast() == o.broadcast())
		&& (loopback() == o.loopback())
		&& (point_to_point() == o.point_to_point())
		&& (multicast() == o.multicast())
		&& (pim_register() == o.pim_register())
		&& (vif_flags() == o.vif_flags()));
    }

    void finalize_state();

    string str() const;

private:
    IfTreeInterface& _iface;
    const string _vifname;

    uint32_t	_pif_index;
    uint32_t	_vif_index;
    bool 	_enabled;
    bool	_broadcast;
    bool	_loopback;
    bool	_point_to_point;
    bool	_multicast;
    bool	_pim_register;
    uint32_t	_vif_flags;		// The system-specific vif flags

    IPv4Map	 _ipv4addrs;
    IPv6Map	 _ipv6addrs;
};


/**
 * Class for holding an IPv4 interface address and address related items.
 */
class IfTreeAddr4 :
    public IfTreeItem
{
public:
    IfTreeAddr4(const IPv4& addr)
	: IfTreeItem(),
	  _addr(addr),
	  _enabled(false),
	  _broadcast(false),
	  _loopback(false),
	  _point_to_point(false),
	  _multicast(false),
	  _prefix_len(0)
    {}

    const IPv4& addr() const		{ return _addr; }

    bool enabled() const		{ return _enabled; }
    bool broadcast() const		{ return _broadcast; }
    bool loopback() const		{ return _loopback; }
    bool point_to_point() const		{ return _point_to_point; }
    bool multicast() const		{ return _multicast; }

    void set_enabled(bool en)		{ _enabled = en; mark(CHANGED); }
    void set_broadcast(bool v)		{ _broadcast = v; mark(CHANGED); }
    void set_loopback(bool v)		{ _loopback = v; mark(CHANGED); }
    void set_point_to_point(bool v)	{ _point_to_point = v; mark(CHANGED); }
    void set_multicast(bool v)		{ _multicast = v; mark(CHANGED); }

    /**
     * Get prefix length associates with address.
     */
    uint32_t prefix_len() const		{ return _prefix_len; }

    /**
     * Set prefix length associate with address.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_prefix_len(uint32_t prefix_len);

    /**
     * Get the broadcast address.
     *
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
    void copy_state(const IfTreeAddr4& o) {
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
    bool is_same_state(const IfTreeAddr4& o) {
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

private:
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
class IfTreeAddr6 :
    public IfTreeItem
{
public:
    IfTreeAddr6(const IPv6& addr)
	: IfTreeItem(),
	  _addr(addr),
	  _enabled(false),
	  _loopback(false),
	  _point_to_point(false),
	  _multicast(false),
	  _prefix_len(0)
    {}

    const IPv6& addr() const		{ return _addr; }

    bool enabled() const		{ return _enabled; }
    bool loopback() const		{ return _loopback; }
    bool point_to_point() const		{ return _point_to_point; }
    bool multicast() const		{ return _multicast; }

    void set_enabled(bool en)		{ _enabled = en; mark(CHANGED); }
    void set_loopback(bool v)		{ _loopback = v; mark(CHANGED); }
    void set_point_to_point(bool v)	{ _point_to_point = v; mark(CHANGED); }
    void set_multicast(bool v)		{ _multicast = v; mark(CHANGED); }

    /**
     * Get prefix length associated with address.
     */
    uint32_t prefix_len() const		{ return _prefix_len; }

    /**
     * Set prefix length associate with address.
     *
     * @return XORP_OK on success, otherwise XORP_ERROR.
     */
    int set_prefix_len(uint32_t prefix_len);

    IPv6 endpoint() const;

    void set_endpoint(const IPv6& oaddr);

    /**
     * Copy state of internal variables from another IfTreeAddr6.
     */
    void copy_state(const IfTreeAddr6& o) {
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
    bool is_same_state(const IfTreeAddr6& o) {
	return ((enabled() == o.enabled())
		&& (loopback() == o.loopback())
		&& (point_to_point() == o.point_to_point())
		&& (multicast() == o.multicast())
		&& (endpoint() == o.endpoint())
		&& (prefix_len() == o.prefix_len()));
    }

    void finalize_state();

    string str() const;

private:
    IPv6	_addr;

    bool 	_enabled;
    bool	_loopback;
    bool	_point_to_point;
    bool	_multicast;

    IPv6	_oaddr;		// Other address - p2p endpoint
    uint32_t	_prefix_len;	// The prefix length
};

#endif // __FEA_IFTREE_HH__
