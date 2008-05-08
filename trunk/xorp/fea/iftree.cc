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

#ident "$XORP: xorp/fea/iftree.cc,v 1.59 2008/03/09 00:21:16 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/c_format.hh"
#include "libxorp/vif.hh"

#include "iftree.hh"


/* ------------------------------------------------------------------------- */
/* IfTreeItem code */

string
IfTreeItem::str() const
{
    struct {
	State st;
	const char* desc;
    } t[] = { { CREATED, "CREATED" },
	      { DELETED, "DELETED" },
	      { CHANGED, "CHANGED" }
    };

    string r;
    for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); i++) {
	if ((_st & t[i].st) == 0)
	    continue;
	if (r.empty() == false)
	    r += ",";
	r += t[i].desc;
    }

    return (r);
}

/* ------------------------------------------------------------------------- */
/* IfTree code */

IfTree::IfTree()
    : IfTreeItem()
{
}

IfTree::IfTree(const IfTree& other)
    : IfTreeItem()
{
    *this = other;
}

IfTree::~IfTree()
{
    clear();
}

IfTree&
IfTree::operator=(const IfTree& other)
{
    clear();

    // Add recursively all interfaces from the other tree
    IfMap::const_iterator oi;
    for (oi = other.interfaces().begin();
	 oi != other.interfaces().end();
	 ++oi) {
	const IfTreeInterface& other_iface = *(oi->second);
	add_recursive_interface(other_iface, true);
    }
    set_state(other.state());

    return (*this);
}

void
IfTree::clear()
{
    while (! _interfaces.empty()) {
	IfTreeInterface* ifp = _interfaces.begin()->second;
	_interfaces.erase(_interfaces.begin());
	delete ifp;
    }

    XLOG_ASSERT(_ifindex_map.empty());
    XLOG_ASSERT(_vifindex_map.empty());
}

void
IfTree::add_recursive_interface(const IfTreeInterface& other_iface,
				bool mark_state)
{
    const string& ifname = other_iface.ifname();
    IfTreeInterface* ifp;

    // Add the interface
    ifp = new IfTreeInterface(*this, ifname);
    _interfaces.insert(IfMap::value_type(ifname, ifp));
    ifp->copy_state(other_iface, true);
    if (mark_state)
	ifp->set_state(other_iface.state());
    else
	ifp->mark(CREATED);

    // Add recursively all vifs from the other interface
    IfTreeInterface::VifMap::const_iterator ov;
    for (ov = other_iface.vifs().begin();
	 ov != other_iface.vifs().end();
	 ++ov) {
	const IfTreeVif& other_vif = *(ov->second);
	ifp->add_recursive_vif(other_vif, mark_state);
    }
}

int
IfTree::add_interface(const string& ifname)
{
    IfTreeInterface* ifp = find_interface(ifname);

    if (ifp != NULL) {
	ifp->mark(CREATED);
	return (XORP_OK);
    }

    ifp = new IfTreeInterface(*this, ifname);
    _interfaces.insert(IfMap::value_type(ifname, ifp));

    return (XORP_OK);
}

IfTreeInterface*
IfTree::find_interface(const string& ifname)
{
    IfTree::IfMap::iterator iter = _interfaces.find(ifname);

    if (iter == interfaces().end())
	return (NULL);

    return (iter->second);
}

const IfTreeInterface*
IfTree::find_interface(const string& ifname) const
{
    IfTree::IfMap::const_iterator iter = _interfaces.find(ifname);

    if (iter == interfaces().end())
	return (NULL);

    return (iter->second);
}

IfTreeInterface*
IfTree::find_interface(uint32_t pif_index)
{
    IfTree::IfIndexMap::iterator iter = _ifindex_map.find(pif_index);

    if (iter == _ifindex_map.end())
	return (NULL);

    return (iter->second);
}

const IfTreeInterface*
IfTree::find_interface(uint32_t pif_index) const
{
    IfTree::IfIndexMap::const_iterator iter = _ifindex_map.find(pif_index);

    if (iter == _ifindex_map.end())
	return (NULL);

    return (iter->second);
}

IfTreeVif*
IfTree::find_vif(const string& ifname, const string& vifname)
{
    IfTreeInterface* ifp = find_interface(ifname);

    if (ifp == NULL)
	return (NULL);

    return (ifp->find_vif(vifname));
}

const IfTreeVif*
IfTree::find_vif(const string& ifname, const string& vifname) const
{
    const IfTreeInterface* ifp = find_interface(ifname);

    if (ifp == NULL)
	return (NULL);

    return (ifp->find_vif(vifname));
}

IfTreeVif*
IfTree::find_vif(uint32_t pif_index)
{
    IfTree::VifIndexMap::iterator iter = _vifindex_map.find(pif_index);

    if (iter == _vifindex_map.end())
	return (NULL);

    return (iter->second);
}

const IfTreeVif*
IfTree::find_vif(uint32_t pif_index) const
{
    IfTree::VifIndexMap::const_iterator iter = _vifindex_map.find(pif_index);

    if (iter == _vifindex_map.end())
	return (NULL);

    return (iter->second);
}

IfTreeAddr4*
IfTree::find_addr(const string& ifname, const string& vifname,
		  const IPv4& addr)
{
    IfTreeVif* vifp = find_vif(ifname, vifname);

    if (vifp == NULL)
	return (NULL);

    return (vifp->find_addr(addr));
}

const IfTreeAddr4*
IfTree::find_addr(const string& ifname, const string& vifname,
		  const IPv4& addr) const
{
    const IfTreeVif* vifp = find_vif(ifname, vifname);

    if (vifp == NULL)
	return (NULL);

    return (vifp->find_addr(addr));
}

IfTreeAddr6*
IfTree::find_addr(const string& ifname, const string& vifname,
		  const IPv6& addr)
{
    IfTreeVif* vifp = find_vif(ifname, vifname);

    if (vifp == NULL)
	return (NULL);

    return (vifp->find_addr(addr));
}

const IfTreeAddr6*
IfTree::find_addr(const string& ifname, const string& vifname,
		  const IPv6& addr) const
{
    const IfTreeVif* vifp = find_vif(ifname, vifname);

    if (vifp == NULL)
	return (NULL);

    return (vifp->find_addr(addr));
}

bool
IfTree::find_interface_vif_by_addr(const IPvX& addr,
				   const IfTreeInterface*& ifp,
				   const IfTreeVif*& vifp) const
{
    IfTree::IfMap::const_iterator ii;
    IfTreeInterface::VifMap::const_iterator vi;
    IfTreeVif::IPv4Map::const_iterator ai4;
    IfTreeVif::IPv6Map::const_iterator ai6;

    ifp = NULL;
    vifp = NULL;

    //
    // Iterate through all interfaces and vifs
    //
    for (ii = interfaces().begin(); ii != interfaces().end(); ++ii) {
	const IfTreeInterface& fi = *(ii->second);
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi) {
	    const IfTreeVif& fv = *(vi->second);

	    //
	    // IPv4 address test
	    //
	    if (addr.is_ipv4()) {
		IPv4 addr4 = addr.get_ipv4();
		for (ai4 = fv.ipv4addrs().begin();
		     ai4 != fv.ipv4addrs().end();
		     ++ai4) {
		    const IfTreeAddr4& a4 = *(ai4->second);

		    if (a4.addr() == addr4) {
			// Found a match
			ifp = &fi;
			vifp = &fv;
			return (true);
		    }
		}
		continue;
	    }

	    //
	    // IPv6 address test
	    //
	    if (addr.is_ipv6()) {
		IPv6 addr6 = addr.get_ipv6();
		for (ai6 = fv.ipv6addrs().begin();
		     ai6 != fv.ipv6addrs().end();
		     ++ai6) {
		    const IfTreeAddr6& a6 = *(ai6->second);

		    if (a6.addr() == addr6) {
			// Found a match
			ifp = &fi;
			vifp = &fv;
			return (true);
		    }
		}
		continue;
	    }
	}
    }

    return (false);
}

bool
IfTree::find_interface_vif_same_subnet_or_p2p(const IPvX& addr,
					      const IfTreeInterface*& ifp,
					      const IfTreeVif*& vifp) const
{
    IfTree::IfMap::const_iterator ii;
    IfTreeInterface::VifMap::const_iterator vi;
    IfTreeVif::IPv4Map::const_iterator ai4;
    IfTreeVif::IPv6Map::const_iterator ai6;

    ifp = NULL;
    vifp = NULL;

    //
    // Iterate through all interfaces and vifs
    //
    for (ii = interfaces().begin(); ii != interfaces().end(); ++ii) {
	const IfTreeInterface& fi = *(ii->second);
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi) {
	    const IfTreeVif& fv = *(vi->second);

	    //
	    // IPv4 address test
	    //
	    if (addr.is_ipv4()) {
		IPv4 addr4 = addr.get_ipv4();
		for (ai4 = fv.ipv4addrs().begin();
		     ai4 != fv.ipv4addrs().end();
		     ++ai4) {
		    const IfTreeAddr4& a4 = *(ai4->second);

		    // Test if same subnet
		    IPv4Net subnet(a4.addr(), a4.prefix_len());
		    if (subnet.contains(addr4)) {
			// Found a match
			ifp = &fi;
			vifp = &fv;
			return (true);
		    }

		    // Test if same p2p
		    if (! a4.point_to_point())
			continue;
		    if ((a4.addr() == addr4) || (a4.endpoint() == addr4)) {
			// Found a match
			ifp = &fi;
			vifp = &fv;
			return (true);
		    }
		}
		continue;
	    }

	    //
	    // IPv6 address test
	    //
	    if (addr.is_ipv6()) {
		IPv6 addr6 = addr.get_ipv6();
		for (ai6 = fv.ipv6addrs().begin();
		     ai6 != fv.ipv6addrs().end();
		     ++ai6) {
		    const IfTreeAddr6& a6 = *(ai6->second);

		    // Test if same subnet
		    IPv6Net subnet(a6.addr(), a6.prefix_len());
		    if (subnet.contains(addr6)) {
			// Found a match
			ifp = &fi;
			vifp = &fv;
			return (true);
		    }

		    // Test if same p2p
		    if (! a6.point_to_point())
			continue;
		    if ((a6.addr() == addr6) || (a6.endpoint() == addr6)) {
			// Found a match
			ifp = &fi;
			vifp = &fv;
			return (true);
		    }
		}
		continue;
	    }
	}
    }

    return (false);
}

int
IfTree::remove_interface(const string& ifname)
{
    IfTreeInterface* ifp = find_interface(ifname);

    if (ifp == NULL)
	return (XORP_ERROR);

    ifp->mark(DELETED);

    return (XORP_OK);
}

/**
 * Recursively create a new interface or update its state if it already exists.
 *
 * @param other_iface the interface with the state to copy from.
 * @return XORP_OK on success, otherwise XORP_ERROR.
 */
int
IfTree::update_interface(const IfTreeInterface& other_iface)
{
    IfTreeInterface* this_ifp;
    IfTreeInterface::VifMap::iterator vi;
    IfTreeInterface::VifMap::const_iterator ov;

    //
    // Add the interface if we don't have it already
    //
    this_ifp = find_interface(other_iface.ifname());
    if (this_ifp == NULL) {
	add_interface(other_iface.ifname());
	this_ifp = find_interface(other_iface.ifname());
	XLOG_ASSERT(this_ifp != NULL);
    }

    //
    // Update the interface state
    //
    if (! this_ifp->is_same_state(other_iface))
	this_ifp->copy_state(other_iface, false);

    //
    // Update existing vifs and addresses, and delete those that don't exist
    //
    for (vi = this_ifp->vifs().begin(); vi != this_ifp->vifs().end(); ++vi) {
	IfTreeVif* this_vifp = vi->second;
	const IfTreeVif* other_vifp;

	other_vifp = other_iface.find_vif(this_vifp->vifname());
	if ((other_vifp == NULL) || (other_vifp->is_marked(DELETED))) {
	    this_vifp->mark(DELETED);
	    continue;
	}

	if (! this_vifp->is_same_state(*other_vifp))
	    this_vifp->copy_state(*other_vifp);

	//
	// Update the IPv4 addresses
	//
	IfTreeVif::IPv4Map::iterator ai4;
	for (ai4 = this_vifp->ipv4addrs().begin();
	     ai4 != this_vifp->ipv4addrs().end();
	     ++ai4) {
	    IfTreeAddr4* this_ap = ai4->second;
	    const IfTreeAddr4* other_ap;
	    other_ap = other_vifp->find_addr(this_ap->addr());
	    if ((other_ap == NULL) || (other_ap->is_marked(DELETED))) {
		this_ap->mark(DELETED);
		continue;
	    }
	    if (! this_ap->is_same_state(*other_ap))
		this_ap->copy_state(*other_ap);
	}

	//
	// Update the IPv6 addresses
	//
	IfTreeVif::IPv6Map::iterator ai6;
	for (ai6 = this_vifp->ipv6addrs().begin();
	     ai6 != this_vifp->ipv6addrs().end();
	     ++ai6) {
	    IfTreeAddr6* this_ap = ai6->second;
	    const IfTreeAddr6* other_ap;
	    other_ap = other_vifp->find_addr(this_ap->addr());
	    if ((other_ap == NULL) || (other_ap->is_marked(DELETED))) {
		this_ap->mark(DELETED);
		continue;
	    }
	    if (! this_ap->is_same_state(*other_ap))
		this_ap->copy_state(*other_ap);
	}
    }

    //
    // Add the new vifs and addresses
    //
    for (ov = other_iface.vifs().begin();
	 ov != other_iface.vifs().end();
	 ++ov) {
	const IfTreeVif* other_vifp = ov->second;
	IfTreeVif* this_vifp;

	//
	// Add the vif
	//
	this_vifp = this_ifp->find_vif(other_vifp->vifname());
	if (this_vifp == NULL) {
	    this_ifp->add_vif(other_vifp->vifname());
	    this_vifp = this_ifp->find_vif(other_vifp->vifname());
	    XLOG_ASSERT(this_vifp != NULL);
	    this_vifp->copy_state(*other_vifp);
	}

	//
	// Add the IPv4 addresses
	//
	IfTreeVif::IPv4Map::const_iterator oa4;
	for (oa4 = other_vifp->ipv4addrs().begin();
	     oa4 != other_vifp->ipv4addrs().end();
	     ++oa4) {
	    const IfTreeAddr4* other_ap = oa4->second;
	    IfTreeAddr4* this_ap;
	    this_ap = this_vifp->find_addr(other_ap->addr());
	    if (this_ap == NULL) {
		// Add the address
		this_vifp->add_addr(other_ap->addr());
		this_ap = this_vifp->find_addr(other_ap->addr());
		XLOG_ASSERT(this_ap != NULL);
		this_ap->copy_state(*other_ap);
	    }
	}

	//
	// Add the IPv6 addresses
	//
	IfTreeVif::IPv6Map::const_iterator oa6;
	for (oa6 = other_vifp->ipv6addrs().begin();
	     oa6 != other_vifp->ipv6addrs().end();
	     ++oa6) {
	    const IfTreeAddr6* other_ap = oa6->second;
	    IfTreeAddr6* this_ap;
	    this_ap = this_vifp->find_addr(other_ap->addr());
	    if (this_ap == NULL) {
		// Add the address
		this_vifp->add_addr(other_ap->addr());
		this_ap = this_vifp->find_addr(other_ap->addr());
		XLOG_ASSERT(this_ap != NULL);
		this_ap->copy_state(*other_ap);
	    }
	}
    }

    return (XORP_OK);
}

/**
 * Delete interfaces labelled as ready for deletion, call finalize_state()
 * on remaining interfaces, and set state to NO_CHANGE.
 */
void
IfTree::finalize_state()
{
    IfMap::iterator ii = _interfaces.begin();
    while (ii != _interfaces.end()) {
	IfTreeInterface* ifp = ii->second;

	// If interface is marked as deleted, delete it.
	if (ifp->is_marked(DELETED)) {
	    _interfaces.erase(ii++);
	    delete ifp;
	    continue;
	}
	// Call finalize_state on interfaces that remain
	ifp->finalize_state();
	++ii;
    }
    set_state(NO_CHANGE);
}

string
IfTree::str() const
{
    string r;
    IfMap::const_iterator ii;

    //
    // Print the interfaces
    //
    for (ii = interfaces().begin(); ii != interfaces().end(); ++ii) {
	const IfTreeInterface& fi = *(ii->second);
	r += fi.str() + string("\n");

	//
	// Print the vifs
	//
	for (IfTreeInterface::VifMap::const_iterator vi = fi.vifs().begin();
	     vi != fi.vifs().end();
	     ++vi) {
	    const IfTreeVif& fv = *(vi->second);
	    r += string("  ") + fv.str() + string("\n");

	    //
	    // Print the IPv4 addresses
	    //
	    for (IfTreeVif::IPv4Map::const_iterator ai = fv.ipv4addrs().begin();
		 ai != fv.ipv4addrs().end();
		 ++ai) {
		const IfTreeAddr4& a = *(ai->second);
		r += string("    ") + a.str() + string("\n");
	    }

	    //
	    // Print the IPv6 addresses
	    //
	    for (IfTreeVif::IPv6Map::const_iterator ai = fv.ipv6addrs().begin();
		 ai != fv.ipv6addrs().end();
		 ++ai) {
		const IfTreeAddr6& a = *(ai->second);
		r += string("    ") + a.str() + string("\n");
	    }
	}
    }

    return (r);
}

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
IfTree&
IfTree::align_with_pulled_changes(const IfTree& other,
				  const IfTree& user_config)
{
    IfTree::IfMap::iterator ii;

    //
    // Iterate through all interfaces
    //
    for (ii = interfaces().begin(); ii != interfaces().end(); ++ii) {
	IfTreeInterface* this_ifp = ii->second;
	const string& ifname = this_ifp->ifname();
	const IfTreeInterface* other_ifp = other.find_interface(ifname);
	const IfTreeInterface* user_ifp = user_config.find_interface(ifname);

	//
	// Ignore "soft" interfaces
	//
	if (this_ifp->is_soft())
	    continue;

	//
	// Special processing for "default_system_config" interfaces
	//
	if (this_ifp->default_system_config()) {
	    if (other_ifp != NULL) {
		update_interface(*other_ifp);
	    } else {
		this_ifp->set_enabled(false);
		IfTreeInterface::VifMap::iterator vi;
		for (vi = this_ifp->vifs().begin();
		     vi != this_ifp->vifs().end();
		     ++vi) {
		    IfTreeVif* this_vifp = vi->second;
		    this_vifp->mark(DELETED);
		}
	    }
	    continue;
	}

	//
	// Disable if not in the other tree
	//
	if (other_ifp == NULL) {
	    this_ifp->set_enabled(false);
	    continue;
	}

	//
	// Copy state from the other interface
	//
	if (! this_ifp->is_same_state(*other_ifp)) {
	    bool enabled = false;
	    if ((user_ifp != NULL) && user_ifp->enabled())
		enabled = true;
	    this_ifp->copy_state(*other_ifp, false);
	    if (! enabled)
		this_ifp->set_enabled(enabled);
	}

	//
	// Align the vif state
	//
	IfTreeInterface::VifMap::iterator vi;
	for (vi = this_ifp->vifs().begin();
	     vi != this_ifp->vifs().end();
	     ++vi) {
	    IfTreeVif* this_vifp = vi->second;
	    const string& vifname = this_vifp->vifname();
	    const IfTreeVif* other_vifp = other_ifp->find_vif(vifname);
	    const IfTreeVif* user_vifp = NULL;

	    if (user_ifp != NULL)
		user_vifp = user_ifp->find_vif(vifname);

	    //
	    // Disable if not in the other tree
	    //
	    if (other_vifp == NULL) {
		this_vifp->set_enabled(false);
		continue;
	    }

	    //
	    // Copy state from the other vif
	    //
	    if (! this_vifp->is_same_state(*other_vifp)) {
		bool enabled = false;
		if ((user_vifp != NULL) && user_vifp->enabled())
		    enabled = true;
		this_vifp->copy_state(*other_vifp);
		if (! enabled)
		    this_vifp->set_enabled(enabled);
	    }

	    //
	    // Align the IPv4 address state
	    //
	    IfTreeVif::IPv4Map::iterator ai4;
	    for (ai4 = this_vifp->ipv4addrs().begin();
		 ai4 != this_vifp->ipv4addrs().end();
		 ++ai4) {
		IfTreeAddr4* this_ap = ai4->second;
		const IPv4& addr = this_ap->addr();
		const IfTreeAddr4* other_ap = other_vifp->find_addr(addr);
		const IfTreeAddr4* user_ap = NULL;

		if (user_vifp != NULL)
		    user_ap = user_vifp->find_addr(addr);

		//
		// Disable if not in the other tree
		//
		if (other_ap == NULL) {
		    this_ap->set_enabled(false);
		    continue;
		}

		//
		// Copy state from the other address
		//
		if (! this_ap->is_same_state(*other_ap)) {
		    bool enabled = false;
		    if ((user_ap != NULL) && user_ap->enabled())
			enabled = true;
		    this_ap->copy_state(*other_ap);
		    if (! enabled)
			this_ap->set_enabled(enabled);
		}
	    }

	    //
	    // Align the IPv6 address state
	    //
	    IfTreeVif::IPv6Map::iterator ai6;
	    for (ai6 = this_vifp->ipv6addrs().begin();
		 ai6 != this_vifp->ipv6addrs().end();
		 ++ai6) {
		IfTreeAddr6* this_ap = ai6->second;
		const IPv6& addr = this_ap->addr();
		const IfTreeAddr6* other_ap = other_vifp->find_addr(addr);
		const IfTreeAddr6* user_ap = NULL;

		if (user_vifp != NULL)
		    user_ap = user_vifp->find_addr(addr);

		//
		// Disable if not in the other tree
		//
		if (other_ap == NULL) {
		    this_ap->set_enabled(false);
		    continue;
		}

		//
		// Copy state from the other address
		//
		if (! this_ap->is_same_state(*other_ap)) {
		    bool enabled = false;
		    if ((user_ap != NULL) && user_ap->enabled())
			enabled = true;
		    this_ap->copy_state(*other_ap);
		    if (! enabled)
			this_ap->set_enabled(enabled);
		}
	    }
	}
    }

    return (*this);
}

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
 * The alignment works as follows:
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
 *        Also, if the item is disabled in the user config tree, it is
 *        marked as "disabled" in the local tree.
 *
 * @param other the configuration tree to align state with.
 * @param user_config the user configuration tree to reference during
 * the alignment.
 * @return modified configuration structure.
 */
IfTree&
IfTree::align_with_observed_changes(const IfTree& other,
				    const IfTree& user_config)
{
    IfTree::IfMap::const_iterator oi;

    //
    // Iterate through all interfaces in the other tree
    //
    for (oi = other.interfaces().begin();
	 oi != other.interfaces().end(); ++oi) {
	const IfTreeInterface* other_ifp = oi->second;
	const string& ifname = other_ifp->ifname();
	IfTreeInterface* this_ifp = find_interface(ifname);
	const IfTreeInterface* user_ifp = user_config.find_interface(ifname);

	//
	// Ignore interfaces that are not in the local or user config tree
	//
	if (this_ifp == NULL) {
	    if (user_ifp == NULL)
		continue;
	    // Create the interface
	    add_interface(ifname);
	    this_ifp = find_interface(ifname);
	    XLOG_ASSERT(this_ifp != NULL);
	    this_ifp->copy_state(*user_ifp, true);
	    this_ifp->copy_state(*other_ifp, false);
	    this_ifp->mark(CREATED);
	}

	//
	// Ignore "soft" interfaces
	//
	if (this_ifp->is_soft())
	    continue;

	//
	// Special processing for "default_system_config" interfaces
	//
	if (this_ifp->default_system_config()) {
	    update_interface(*other_ifp);
	    continue;
	}

	//
	// Test for "DELETED" entries
	//
	if (other_ifp->is_marked(DELETED)) {
	    this_ifp->set_enabled(false);
	    continue;
	}

	//
	// Test for "CREATED" or "CHANGED" entries
	//
	if (other_ifp->is_marked(CREATED) || other_ifp->is_marked(CHANGED)) {
	    bool enabled = false;
	    if ((user_ifp != NULL) && user_ifp->enabled())
		enabled = true;
	    //
	    // Copy state from the other entry
	    //
	    if (! this_ifp->is_same_state(*other_ifp)) {
		this_ifp->copy_state(*other_ifp, false);
		if (! enabled)
		    this_ifp->set_enabled(enabled);
		this_ifp->mark(CHANGED);	// XXX: no-op if it was CREATED
	    }
	}

	//
	// Align the vif state
	//
	IfTreeInterface::VifMap::const_iterator ov;
	for (ov = other_ifp->vifs().begin();
	     ov != other_ifp->vifs().end();
	     ++ov) {
	    const IfTreeVif* other_vifp = ov->second;
	    const string& vifname = other_vifp->vifname();
	    IfTreeVif* this_vifp = this_ifp->find_vif(vifname);
	    const IfTreeVif* user_vifp = NULL;

	    if (user_ifp != NULL)
		user_vifp = user_ifp->find_vif(vifname);

	    //
	    // Ignore entries that are not in the local or user config tree
	    //
	    if (this_vifp == NULL) {
		if (user_vifp == NULL)
		    continue;
		// Create the vif
		this_ifp->add_vif(vifname);
		this_vifp = this_ifp->find_vif(vifname);
		XLOG_ASSERT(this_vifp != NULL);
		this_vifp->copy_state(*other_vifp);
		this_vifp->mark(CREATED);
	    }

	    //
	    // Test for "DELETED" entries
	    //
	    if (other_vifp->is_marked(DELETED)) {
		this_vifp->set_enabled(false);
		continue;
	    }

	    //
	    // Test for "CREATED" or "CHANGED" entries
	    //
	    if (other_vifp->is_marked(CREATED)
		|| other_vifp->is_marked(CHANGED)) {
		bool enabled = false;
		if ((user_vifp != NULL) && user_vifp->enabled())
		    enabled = true;
		//
		// Copy state from the other entry
		//
		if (! this_vifp->is_same_state(*other_vifp)) {
		    this_vifp->copy_state(*other_vifp);
		    if (! enabled)
			this_vifp->set_enabled(enabled);
		    this_vifp->mark(CHANGED);	// XXX: no-op if it was CREATED
		}
	    }

	    //
	    // Align the IPv4 address state
	    //
	    IfTreeVif::IPv4Map::const_iterator oa4;
	    for (oa4 = other_vifp->ipv4addrs().begin();
		 oa4 != other_vifp->ipv4addrs().end();
		 ++oa4) {
		const IfTreeAddr4* other_ap = oa4->second;
		const IPv4& addr = other_ap->addr();
		IfTreeAddr4* this_ap = this_vifp->find_addr(addr);
		const IfTreeAddr4* user_ap = NULL;

		if (user_vifp != NULL)
		    user_ap = user_vifp->find_addr(addr);

		//
		// Ignore entries that are not in the local or user config tree
		//
		if (this_ap == NULL) {
		    if (user_ap == NULL)
			continue;
		    // Create the address
		    this_vifp->add_addr(addr);
		    this_ap = this_vifp->find_addr(addr);
		    XLOG_ASSERT(this_ap != NULL);
		    this_ap->copy_state(*other_ap);
		    this_ap->mark(CREATED);
		}

		//
		// Test for "DELETED" entries
		//
		if (other_ap->is_marked(DELETED)) {
		    this_ap->set_enabled(false);
		    continue;
		}

		//
		// Test for "CREATED" or "CHANGED" entries
		//
		if (other_ap->is_marked(CREATED)
		    || other_ap->is_marked(CHANGED)) {
		    bool enabled = false;
		    if ((user_ap != NULL) && user_ap->enabled())
			enabled = true;
		    //
		    // Copy state from the other entry
		    //
		    if (! this_ap->is_same_state(*other_ap)) {
			this_ap->copy_state(*other_ap);
			if (! enabled)
			    this_ap->set_enabled(enabled);
			this_ap->mark(CHANGED);	// XXX: no-op if it was CREATED
		    }
		}
	    }

	    //
	    // Align the IPv6 address state
	    //
	    IfTreeVif::IPv6Map::const_iterator oa6;
	    for (oa6 = other_vifp->ipv6addrs().begin();
		 oa6 != other_vifp->ipv6addrs().end();
		 ++oa6) {
		const IfTreeAddr6* other_ap = oa6->second;
		const IPv6& addr = other_ap->addr();
		IfTreeAddr6* this_ap = this_vifp->find_addr(addr);
		const IfTreeAddr6* user_ap = NULL;

		if (user_vifp != NULL)
		    user_ap = user_vifp->find_addr(addr);

		//
		// Ignore entries that are not in the local or user config tree
		//
		if (this_ap == NULL) {
		    if (user_ap == NULL)
			continue;
		    // Create the address
		    this_vifp->add_addr(addr);
		    this_ap = this_vifp->find_addr(addr);
		    XLOG_ASSERT(this_ap != NULL);
		    this_ap->copy_state(*other_ap);
		    this_ap->mark(CREATED);
		}

		//
		// Test for "DELETED" entries
		//
		if (other_ap->is_marked(DELETED)) {
		    this_ap->set_enabled(false);
		    continue;
		}

		//
		// Test for "CREATED" or "CHANGED" entries
		//
		if (other_ap->is_marked(CREATED)
		    || other_ap->is_marked(CHANGED)) {
		    bool enabled = false;
		    if ((user_ap != NULL) && user_ap->enabled())
			enabled = true;
		    //
		    // Copy state from the other entry
		    //
		    if (! this_ap->is_same_state(*other_ap)) {
			if (! enabled)
			    this_ap->set_enabled(enabled);
			this_ap->mark(CHANGED);	// XXX: no-op if it was CREATED
		    }
		}
	    }
	}
    }

    return (*this);
}

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
 *    (a) "NO_CHANGE": The state of the entry in the other tree is not
 *        propagated to the local tree, but its subtree entries are
 *        processed.
 *    (b) "DELETED": The item in the local tree is marked as "DELETED",
 *        and the subtree entries are ignored.
 *    (c) "CREATED" or "CHANGED": If the state of the entry is different
 *        in the other and the local tree, it is copied to the local tree,
 *        and the item in the local tree is marked as "CHANGED".
 *
 * @param other the configuration tree to align state with.
 * @return modified configuration structure.
 */
IfTree&
IfTree::align_with_user_config(const IfTree& other)
{
    IfTree::IfMap::const_iterator oi;

    //
    // Iterate through all interfaces in the other tree
    //
    for (oi = other.interfaces().begin();
	 oi != other.interfaces().end(); ++oi) {
	const IfTreeInterface* other_ifp = oi->second;
	IfTreeInterface* this_ifp = find_interface(other_ifp->ifname());

	//
	// Recursively add entries that are not in the local tree
	//
	if (this_ifp == NULL) {
	    add_recursive_interface(*other_ifp, false);
	    continue;
	}

	//
	// Test for "DELETED" entries
	//
	if (other_ifp->is_marked(DELETED)) {
	    this_ifp->mark(DELETED);
	    continue;
	}

	//
	// Test for "CREATED" or "CHANGED" entries
	//
	if (other_ifp->is_marked(CREATED) || other_ifp->is_marked(CHANGED)) {
	    //
	    // Copy state from the other entry
	    //
	    if (! this_ifp->is_same_state(*other_ifp)) {
		this_ifp->copy_state(*other_ifp, false);
		this_ifp->mark(CHANGED);
	    }
	}

	//
	// Align the vif state
	//
	IfTreeInterface::VifMap::const_iterator ov;
	for (ov = other_ifp->vifs().begin();
	     ov != other_ifp->vifs().end();
	     ++ov) {
	    const IfTreeVif* other_vifp = ov->second;
	    IfTreeVif* this_vifp = this_ifp->find_vif(other_vifp->vifname());

	    //
	    // Recursively add entries that are not in the local tree
	    //
	    if (this_vifp == NULL) {
		this_ifp->add_recursive_vif(*other_vifp, false);
		continue;
	    }

	    //
	    // Test for "DELETED" entries
	    //
	    if (other_vifp->is_marked(DELETED)) {
		this_vifp->mark(DELETED);
		continue;
	    }

	    //
	    // Test for "CREATED" or "CHANGED" entries
	    //
	    if (other_vifp->is_marked(CREATED)
		|| other_vifp->is_marked(CHANGED)) {
		//
		// Copy state from the other entry
		//
		if (! this_vifp->is_same_state(*other_vifp)) {
		    this_vifp->copy_state(*other_vifp);
		    this_vifp->mark(CHANGED);
		}
	    }

	    //
	    // Align the IPv4 address state
	    //
	    IfTreeVif::IPv4Map::const_iterator oa4;
	    for (oa4 = other_vifp->ipv4addrs().begin();
		 oa4 != other_vifp->ipv4addrs().end();
		 ++oa4) {
		const IfTreeAddr4* other_ap = oa4->second;
		IfTreeAddr4* this_ap = this_vifp->find_addr(other_ap->addr());

		//
		// Recursively add entries that are not in the local tree
		//
		if (this_ap == NULL) {
		    this_vifp->add_recursive_addr(*other_ap, false);
		    continue;
		}

		//
		// Test for "DELETED" entries
		//
		if (other_ap->is_marked(DELETED)) {
		    this_ap->mark(DELETED);
		    continue;
		}

		//
		// Test for "CREATED" or "CHANGED" entries
		//
		if (other_ap->is_marked(CREATED)
		    || other_ap->is_marked(CHANGED)) {
		    //
		    // Copy state from the other entry
		    //
		    if (! this_ap->is_same_state(*other_ap)) {
			this_ap->copy_state(*other_ap);
			this_ap->mark(CHANGED);
		    }
		}
	    }

	    //
	    // Align the IPv6 address state
	    //
	    IfTreeVif::IPv6Map::const_iterator oa6;
	    for (oa6 = other_vifp->ipv6addrs().begin();
		 oa6 != other_vifp->ipv6addrs().end();
		 ++oa6) {
		const IfTreeAddr6* other_ap = oa6->second;
		IfTreeAddr6* this_ap = this_vifp->find_addr(other_ap->addr());

		//
		// Recursively add entries that are not in the local tree
		//
		if (this_ap == NULL) {
		    this_vifp->add_recursive_addr(*other_ap, false);
		    continue;
		}

		//
		// Test for "DELETED" entries
		//
		if (other_ap->is_marked(DELETED)) {
		    this_ap->mark(DELETED);
		    continue;
		}

		//
		// Test for "CREATED" or "CHANGED" entries
		//
		if (other_ap->is_marked(CREATED)
		    || other_ap->is_marked(CHANGED)) {
		    //
		    // Copy state from the other entry
		    //
		    if (! this_ap->is_same_state(*other_ap)) {
			this_ap->copy_state(*other_ap);
			this_ap->mark(CHANGED);
		    }
		}
	    }
	}
    }

    return (*this);
}

/**
 * Prepare configuration for pushing and replacing previous configuration.
 *
 * If the previous configuration is to be replaced with new configuration,
 * we need to prepare the state that will delete, update, and add the
 * new state as appropriate.
 * The preparation works as follows:
 * - All items in the local tree are preserved and are marked as created.
 * - All items in the other tree that are not in the local tree are
 *   added to the local tree and are marked as deleted.
 *
 * @param other the configuration tree to be used to prepare the
 * replacement state.
 * @return modified configuration structure.
 */
IfTree&
IfTree::prepare_replacement_state(const IfTree& other)
{
    IfTree::IfMap::iterator ii;
    IfTree::IfMap::const_iterator oi;

    //
    // Mark all entries in the local tree as created
    //
    for (ii = interfaces().begin(); ii != interfaces().end(); ++ii) {
	IfTreeInterface& fi = *(ii->second);
	fi.mark(CREATED);

	// Mark all vifs
	IfTreeInterface::VifMap::iterator vi;
	for (vi = fi.vifs().begin(); vi != fi.vifs().end(); ++vi) {
	    IfTreeVif& fv = *(vi->second);
	    fv.mark(CREATED);

	    // Mark all IPv4 addresses
	    IfTreeVif::IPv4Map::iterator ai4;
	    for (ai4 = fv.ipv4addrs().begin();
		 ai4 != fv.ipv4addrs().end();
		 ++ai4) {
		IfTreeAddr4& fa = *(ai4->second);
		fa.mark(CREATED);
	    }

	    // Mark all IPv6 addresses
	    IfTreeVif::IPv6Map::iterator ai6;
	    for (ai6 = fv.ipv6addrs().begin();
		 ai6 != fv.ipv6addrs().end();
		 ++ai6) {
		IfTreeAddr6& fa = *(ai6->second);
		fa.mark(CREATED);
	    }
	}
    }

    //
    // Iterate through all interfaces in the other tree
    //
    for (oi = other.interfaces().begin();
	 oi != other.interfaces().end();
	 ++oi) {
	const IfTreeInterface& other_iface = *(oi->second);
	const string& ifname = other_iface.ifname();
	IfTreeInterface* ifp = find_interface(ifname);
	if (ifp == NULL) {
	    //
	    // Add local interface and mark it for deletion.
	    //
	    // Mark local interface for deletion if not present in other.
	    //
	    add_interface(ifname);
	    ifp = find_interface(ifname);
	    XLOG_ASSERT(ifp != NULL);
	    ifp->copy_state(other_iface, false);
	    ifp->mark(DELETED);
	}

	//
	// Iterate through all vifs in the other tree
	//
	IfTreeInterface::VifMap::const_iterator ov;
	for (ov = other_iface.vifs().begin();
	     ov != other_iface.vifs().end();
	     ++ov) {
	    const IfTreeVif& other_vif = *(ov->second);
	    const string& vifname = other_vif.vifname();
	    IfTreeVif* vifp = ifp->find_vif(vifname);
	    if (vifp == NULL) {
		//
		// Add local vif and mark it for deletion.
		//
		ifp->add_vif(vifname);
		vifp = ifp->find_vif(vifname);
		XLOG_ASSERT(vifp != NULL);
		vifp->copy_state(other_vif);
		vifp->mark(DELETED);
	    }

	    //
	    // Iterate through all IPv4 addresses in the other tree
	    //
	    IfTreeVif::IPv4Map::const_iterator oa4;
	    for (oa4 = other_vif.ipv4addrs().begin();
		 oa4 != other_vif.ipv4addrs().end();
		 ++oa4) {
		const IfTreeAddr4& other_addr = *(oa4->second);
		IfTreeAddr4* ap = vifp->find_addr(other_addr.addr());
		if (ap == NULL) {
		    //
		    // Add local IPv4 address and mark it for deletion.
		    //
		    vifp->add_addr(other_addr.addr());
		    ap = vifp->find_addr(other_addr.addr());
		    XLOG_ASSERT(ap != NULL);
		    ap->copy_state(other_addr);
		    ap->mark(DELETED);
		}
	    }

	    //
	    // Iterate through all IPv6 addresses in the other tree
	    //
	    IfTreeVif::IPv6Map::const_iterator oa6;
	    for (oa6 = other_vif.ipv6addrs().begin();
		 oa6 != other_vif.ipv6addrs().end();
		 ++oa6) {
		const IfTreeAddr6& other_addr = *(oa6->second);
		IfTreeAddr6* ap = vifp->find_addr(other_addr.addr());
		if (ap == NULL) {
		    //
		    // Add local IPv6 address and mark it for deletion.
		    //
		    vifp->add_addr(other_addr.addr());
		    ap = vifp->find_addr(other_addr.addr());
		    XLOG_ASSERT(ap != NULL);
		    ap->copy_state(other_addr);
		    ap->mark(DELETED);
		}
	    }
	}
    }

    return (*this);
}

/**
 * Prune bogus deleted state.
 * 
 * If an item in the local tree is marked as deleted, but is not
 * in the other tree, then it is removed.
 * 
 * @param old_iftree the old tree with the state that is used as reference.
 * @return the modified configuration tree.
 */
IfTree&
IfTree::prune_bogus_deleted_state(const IfTree& old_iftree)
{
    IfTree::IfMap::iterator ii;

    //
    // Iterate through all interfaces
    //
    ii = _interfaces.begin();
    while (ii != _interfaces.end()) {
	IfTreeInterface* ifp = ii->second;
	const string& ifname = ifp->ifname();
	if (! ifp->is_marked(DELETED)) {
	    ++ii;
	    continue;
	}
	const IfTreeInterface* old_ifp;
	old_ifp = old_iftree.find_interface(ifname);
	if (old_ifp == NULL) {
	    // Remove this item from the local tree
	    _interfaces.erase(ii++);
	    delete ifp;
	    continue;
	}

	//
	// Iterate through all vifs
	//
	IfTreeInterface::VifMap::iterator vi;
	vi = ifp->vifs().begin();
	while (vi != ifp->vifs().end()) {
	    IfTreeVif* vifp = vi->second;
	    const string& vifname = vifp->vifname();
	    if (! vifp->is_marked(DELETED)) {
		++vi;
		continue;
	    }
	    const IfTreeVif* old_vifp;
	    old_vifp = old_ifp->find_vif(vifname);
	    if (old_vifp == NULL) {
		// Remove this item from the local tree
		ifp->vifs().erase(vi++);
		delete vifp;
		continue;
	    }

	    //
	    // Iterate through all IPv4 addresses
	    //
	    IfTreeVif::IPv4Map::iterator ai4;
	    ai4 = vifp->ipv4addrs().begin();
	    while (ai4 != vifp->ipv4addrs().end()) {
		IfTreeAddr4* ap = ai4->second;
		if (! ap->is_marked(DELETED)) {
		    ++ai4;
		    continue;
		}
		const IfTreeAddr4* old_ap;
		old_ap = old_vifp->find_addr(ap->addr());
		if (old_ap == NULL) {
		    // Remove this item from the local tree
		    vifp->ipv4addrs().erase(ai4++);
		    delete ap;
		    continue;
		}
		++ai4;
	    }

	    //
	    // Iterate through all IPv6 addresses
	    //
	    IfTreeVif::IPv6Map::iterator ai6;
	    ai6 = vifp->ipv6addrs().begin();
	    while (ai6 != vifp->ipv6addrs().end()) {
		IfTreeAddr6* ap = ai6->second;
		if (! ap->is_marked(DELETED)) {
		    ++ai6;
		    continue;
		}
		const IfTreeAddr6* old_ap;
		old_ap = old_vifp->find_addr(ap->addr());
		if (old_ap == NULL) {
		    // Remove this item from the local tree
		    vifp->ipv6addrs().erase(ai6++);
		    delete ap;
		    continue;
		}
		++ai6;
	    }
	    ++vi;
	}
	++ii;
    }

    return (*this);
}

void
IfTree::insert_ifindex(IfTreeInterface* ifp)
{
    IfIndexMap::iterator iter;

    XLOG_ASSERT(ifp != NULL);

    if (ifp->pif_index() == 0)
	return;		// Ignore: invalid pif_index

    iter = _ifindex_map.find(ifp->pif_index());
    if (iter != _ifindex_map.end()) {
	XLOG_ASSERT(iter->second == ifp);
	iter->second = ifp;
	return;
    }

    _ifindex_map[ifp->pif_index()] = ifp;
}

void
IfTree::erase_ifindex(IfTreeInterface* ifp)
{
    IfIndexMap::iterator iter;

    XLOG_ASSERT(ifp != NULL);

    if (ifp->pif_index() == 0)
	return;		// Ignore: invalid pif_index

    iter = _ifindex_map.find(ifp->pif_index());
    XLOG_ASSERT(iter != _ifindex_map.end());
    XLOG_ASSERT(iter->second == ifp);

    _ifindex_map.erase(iter);
}

void
IfTree::insert_vifindex(IfTreeVif* vifp)
{
    VifIndexMap::iterator iter;

    XLOG_ASSERT(vifp != NULL);

    if (vifp->pif_index() == 0)
	return;		// Ignore: invalid pif_index

    // XXX: Check all multimap entries that match to the same pif_index
    iter = _vifindex_map.find(vifp->pif_index());
    while (iter != _vifindex_map.end()) {
	if (iter->first != vifp->pif_index())
	    break;
	if (iter->second == vifp)
	    return;		// Entry has been added previously
	++iter;
    }

    _vifindex_map.insert(make_pair(vifp->pif_index(), vifp));
}

void
IfTree::erase_vifindex(IfTreeVif* vifp)
{
    VifIndexMap::iterator iter;

    XLOG_ASSERT(vifp != NULL);

    if (vifp->pif_index() == 0)
	return;		// Ignore: invalid pif_index

    iter = _vifindex_map.find(vifp->pif_index());
    XLOG_ASSERT(iter != _vifindex_map.end());

    // XXX: Check all multimap entries that match to the same pif_index
    while (iter != _vifindex_map.end()) {
	if (iter->first != vifp->pif_index())
	    break;
	if (iter->second == vifp) {
	    // Entry found
	    _vifindex_map.erase(iter);
	    return;
	}
	++iter;
    }

    XLOG_UNREACHABLE();
}

/* ------------------------------------------------------------------------- */
/* IfTreeInterface code */

IfTreeInterface::IfTreeInterface(IfTree& iftree, const string& ifname)
    : IfTreeItem(),
      _iftree(iftree),
      _ifname(ifname),
      _pif_index(0),
      _enabled(false),
      _discard(false),
      _unreachable(false),
      _management(false),
      _default_system_config(false),
      _mtu(0),
      _no_carrier(false),
      _interface_flags(0)
{}

IfTreeInterface::~IfTreeInterface()
{
    while (! _vifs.empty()) {
	IfTreeVif* vifp = _vifs.begin()->second;
	_vifs.erase(_vifs.begin());
	delete vifp;
    }

    iftree().erase_ifindex(this);
}

void
IfTreeInterface::add_recursive_vif(const IfTreeVif& other_vif, bool mark_state)
{
    const string& vifname = other_vif.vifname();
    IfTreeVif* vifp;

    // Add the vif
    vifp = new IfTreeVif(*this, vifname);
    _vifs.insert(IfTreeInterface::VifMap::value_type(vifname, vifp));
    vifp->copy_state(other_vif);
    if (mark_state)
	vifp->set_state(other_vif.state());
    else
	vifp->mark(CREATED);

    // Add recursively all the IPv4 addresses from the other vif
    IfTreeVif::IPv4Map::const_iterator oa4;
    for (oa4 = other_vif.ipv4addrs().begin();
	 oa4 != other_vif.ipv4addrs().end();
	 ++oa4) {
	const IfTreeAddr4& other_addr = *(oa4->second);
	vifp->add_recursive_addr(other_addr, mark_state);
    }

    // Add recursively all the IPv6 addresses from the other vif
    IfTreeVif::IPv6Map::const_iterator oa6;
    for (oa6 = other_vif.ipv6addrs().begin();
	 oa6 != other_vif.ipv6addrs().end();
	 ++oa6) {
	const IfTreeAddr6& other_addr = *(oa6->second);
	vifp->add_recursive_addr(other_addr, mark_state);
    }
}

int
IfTreeInterface::add_vif(const string& vifname)
{
    IfTreeVif* vifp = find_vif(vifname);

    if (vifp != NULL) {
	vifp->mark(CREATED);
	return (XORP_OK);
    }

    vifp = new IfTreeVif(*this, vifname);
    _vifs.insert(IfTreeInterface::VifMap::value_type(vifname, vifp));

    return (XORP_OK);
}

int
IfTreeInterface::remove_vif(const string& vifname)
{
    IfTreeVif* vifp = find_vif(vifname);

    if (vifp == NULL)
	return (XORP_ERROR);

    vifp->mark(DELETED);

    return (XORP_OK);
}

IfTreeVif*
IfTreeInterface::find_vif(const string& vifname)
{
    IfTreeInterface::VifMap::iterator iter = _vifs.find(vifname);

    if (iter == _vifs.end())
	return (NULL);

    return (iter->second);
}

const IfTreeVif*
IfTreeInterface::find_vif(const string& vifname) const
{
    IfTreeInterface::VifMap::const_iterator iter = _vifs.find(vifname);

    if (iter == _vifs.end())
	return (NULL);

    return (iter->second);
}

IfTreeVif*
IfTreeInterface::find_vif(uint32_t pif_index)
{
    IfTreeInterface::VifMap::iterator iter;

    for (iter = _vifs.begin(); iter != _vifs.end(); ++iter) {
	if (iter->second->pif_index() == pif_index)
	    return (iter->second);
    }

    return (NULL);
}

const IfTreeVif*
IfTreeInterface::find_vif(uint32_t pif_index) const
{
    IfTreeInterface::VifMap::const_iterator iter;

    for (iter = _vifs.begin(); iter != _vifs.end(); ++iter) {
	if (iter->second->pif_index() == pif_index)
	    return (iter->second);
    }

    return (NULL);
}

IfTreeAddr4*
IfTreeInterface::find_addr(const string& vifname, const IPv4& addr)
{
    IfTreeVif* vif = find_vif(vifname);

    if (vif == NULL)
	return (NULL);

    return (vif->find_addr(addr));
}

const IfTreeAddr4*
IfTreeInterface::find_addr(const string& vifname, const IPv4& addr) const
{
    const IfTreeVif* vif = find_vif(vifname);

    if (vif == NULL)
	return (NULL);

    return (vif->find_addr(addr));
}

IfTreeAddr6*
IfTreeInterface::find_addr(const string& vifname, const IPv6& addr)
{
    IfTreeVif* vif = find_vif(vifname);

    if (vif == NULL)
	return (NULL);

    return (vif->find_addr(addr));
}

const IfTreeAddr6*
IfTreeInterface::find_addr(const string& vifname, const IPv6& addr) const
{
    const IfTreeVif* vif = find_vif(vifname);

    if (vif == NULL)
	return (NULL);

    return (vif->find_addr(addr));
}

void
IfTreeInterface::finalize_state()
{
    //
    // Iterate through all vifs
    //
    VifMap::iterator vi = _vifs.begin();
    while (vi != _vifs.end()) {
	IfTreeVif* vifp = vi->second;

	// If interface is marked as deleted, delete it.
	if (vifp->is_marked(DELETED)) {
	    _vifs.erase(vi++);
	    delete vifp;
	    continue;
	}
	// Call finalize_state on vifs that remain
	vifp->finalize_state();
	++vi;
    }
    set_state(NO_CHANGE);
}

string
IfTreeInterface::str() const
{
    string r = c_format("Interface %s { pif_index = %u } { enabled := %s } "
			"{ discard := %s } { unreachable := %s } "
			"{ management = %s } { default_system_config = %s }"
			"{ mtu := %u } { mac := %s } { no_carrier = %s } "
			"{ flags := %u }",
			_ifname.c_str(),
			XORP_UINT_CAST(_pif_index),
			bool_c_str(_enabled),
			bool_c_str(_discard),
			bool_c_str(_unreachable),
			bool_c_str(_management),
			bool_c_str(_default_system_config),
			XORP_UINT_CAST(_mtu),
			_mac.str().c_str(),
			bool_c_str(_no_carrier),
			XORP_UINT_CAST(_interface_flags));
    r += string(" ") + IfTreeItem::str();

    return (r);
}

/* ------------------------------------------------------------------------- */
/* IfTreeVif code */

IfTreeVif::IfTreeVif(IfTreeInterface& iface, const string& vifname)
    : IfTreeItem(),
      _iface(iface),
      _vifname(vifname),
      _pif_index(0),
      _vif_index(Vif::VIF_INDEX_INVALID),
      _enabled(false),
      _broadcast(false),
      _loopback(false),
      _point_to_point(false),
      _multicast(false),
      _pim_register(false),
      _vif_flags(0),
      _is_vlan(false),
      _vlan_id(0)
{}

IfTreeVif::~IfTreeVif()
{
    while (! _ipv4addrs.empty()) {
	IfTreeAddr4* ap = _ipv4addrs.begin()->second;
	_ipv4addrs.erase(_ipv4addrs.begin());
	delete ap;
    }

    while (! _ipv6addrs.empty()) {
	IfTreeAddr6* ap = _ipv6addrs.begin()->second;
	_ipv6addrs.erase(_ipv6addrs.begin());
	delete ap;
    }

    iftree().erase_vifindex(this);
}

void
IfTreeVif::copy_recursive_vif(const IfTreeVif& other_vif)
{
    // Remove the old IPv4 addresses
    while (! _ipv4addrs.empty()) {
	IfTreeAddr4* ap = _ipv4addrs.begin()->second;
	_ipv4addrs.erase(_ipv4addrs.begin());
	delete ap;
    }

    // Remove the old IPv4 addresses
    while (! _ipv6addrs.empty()) {
	IfTreeAddr6* ap = _ipv6addrs.begin()->second;
	_ipv6addrs.erase(_ipv6addrs.begin());
	delete ap;
    }

    copy_state(other_vif);

    // Add all the IPv4 addresses from the other vif
    IfTreeVif::IPv4Map::const_iterator oa4;
    for (oa4 = other_vif.ipv4addrs().begin();
	 oa4 != other_vif.ipv4addrs().end();
	 ++oa4) {
	const IfTreeAddr4& other_addr = *(oa4->second);
	const IPv4& addr = other_addr.addr();
	IfTreeAddr4* ap = new IfTreeAddr4(addr);
	_ipv4addrs.insert(IfTreeVif::IPv4Map::value_type(addr, ap));
	ap->copy_state(other_addr);
    }

    // Add all the IPv6 addresses from the other vif
    IfTreeVif::IPv6Map::const_iterator oa6;
    for (oa6 = other_vif.ipv6addrs().begin();
	 oa6 != other_vif.ipv6addrs().end();
	 ++oa6) {
	const IfTreeAddr6& other_addr = *(oa6->second);
	const IPv6& addr = other_addr.addr();
	IfTreeAddr6* ap = new IfTreeAddr6(addr);
	_ipv6addrs.insert(IfTreeVif::IPv6Map::value_type(addr, ap));
	ap->copy_state(other_addr);
    }
}

void
IfTreeVif::add_recursive_addr(const IfTreeAddr4& other_addr, bool mark_state)
{
    const IPv4& addr = other_addr.addr();
    IfTreeAddr4* ap;

    // Add the address
    ap = new IfTreeAddr4(addr);
    _ipv4addrs.insert(IfTreeVif::IPv4Map::value_type(addr, ap));
    ap->copy_state(other_addr);
    if (mark_state)
	ap->set_state(other_addr.state());
    else
	ap->mark(CREATED);
}

void
IfTreeVif::add_recursive_addr(const IfTreeAddr6& other_addr, bool mark_state)
{
    const IPv6& addr = other_addr.addr();
    IfTreeAddr6* ap;

    // Add the address
    ap = new IfTreeAddr6(addr);
    _ipv6addrs.insert(IfTreeVif::IPv6Map::value_type(addr, ap));
    ap->copy_state(other_addr);
    if (mark_state)
	ap->set_state(other_addr.state());
    else
	ap->mark(CREATED);
}

int
IfTreeVif::add_addr(const IPv4& addr)
{
    IfTreeAddr4* ap = find_addr(addr);

    if (ap != NULL) {
	ap->mark(CREATED);
	return (XORP_OK);
    }

    ap = new IfTreeAddr4(addr);
    _ipv4addrs.insert(IPv4Map::value_type(addr, ap));

    return (XORP_OK);
}

int
IfTreeVif::remove_addr(const IPv4& addr)
{
    IfTreeAddr4* ap = find_addr(addr);

    if (ap == NULL)
	return (XORP_ERROR);

    ap->mark(DELETED);

    return (XORP_OK);
}

int
IfTreeVif::add_addr(const IPv6& addr)
{
    IfTreeAddr6* ap = find_addr(addr);

    if (ap != NULL) {
	ap->mark(CREATED);
	return (XORP_OK);
    }

    ap = new IfTreeAddr6(addr);
    _ipv6addrs.insert(IPv6Map::value_type(addr, ap));

    return (XORP_OK);
}

int
IfTreeVif::remove_addr(const IPv6& addr)
{
    IfTreeAddr6* ap = find_addr(addr);

    if (ap == NULL)
	return (XORP_ERROR);

    ap->mark(DELETED);

    return (XORP_OK);
}

IfTreeAddr4*
IfTreeVif::find_addr(const IPv4& addr)
{
    IfTreeVif::IPv4Map::iterator iter = _ipv4addrs.find(addr);

    if (iter == _ipv4addrs.end())
	return (NULL);

    return (iter->second);
}

const IfTreeAddr4*
IfTreeVif::find_addr(const IPv4& addr) const
{
    IfTreeVif::IPv4Map::const_iterator iter = _ipv4addrs.find(addr);

    if (iter == _ipv4addrs.end())
	return (NULL);

    return (iter->second);
}

IfTreeAddr6*
IfTreeVif::find_addr(const IPv6& addr)
{
    IfTreeVif::IPv6Map::iterator iter = _ipv6addrs.find(addr);

    if (iter == _ipv6addrs.end())
	return (NULL);

    return (iter->second);
}

const IfTreeAddr6*
IfTreeVif::find_addr(const IPv6& addr) const
{
    IfTreeVif::IPv6Map::const_iterator iter = _ipv6addrs.find(addr);

    if (iter == _ipv6addrs.end())
	return (NULL);

    return (iter->second);
}

void
IfTreeVif::finalize_state()
{
    //
    // Iterate through all IPv4 addresses
    //
    for (IPv4Map::iterator ai = _ipv4addrs.begin(); ai != _ipv4addrs.end(); ) {
	IfTreeAddr4* ap = ai->second;

	// If address is marked as deleted, delete it.
	if (ap->is_marked(DELETED)) {
	    _ipv4addrs.erase(ai++);
	    delete ap;
	    continue;
	}
	// Call finalize_state on addresses that remain
	ap->finalize_state();
	++ai;
    }

    //
    // Iterate through all IPv6 addresses
    //
    for (IPv6Map::iterator ai = _ipv6addrs.begin(); ai != _ipv6addrs.end(); ) {
	IfTreeAddr6* ap = ai->second;

	// If address is marked as deleted, delete it.
	if (ap->is_marked(DELETED)) {
	    _ipv6addrs.erase(ai++);
	    delete ap;
	    continue;
	}
	// Call finalize_state on interfaces that remain
	ap->finalize_state();
	++ai;
    }
    set_state(NO_CHANGE);
}

string
IfTreeVif::str() const
{
    string pim_register_str;
    string vif_index_str;
    string vlan_str;

    //
    // XXX: Conditionally print the pim_register flag, because it is
    // used only by the MFEA.
    //
    if (_pim_register) {
	pim_register_str = c_format("{ pim_register := %s } ",
				    bool_c_str(_pim_register));
    }
    // XXX: Conditionally print the vif_index, because it is rarely used
    if (_vif_index != Vif::VIF_INDEX_INVALID) {
	vif_index_str = c_format("{ vif_index := %u } ",
				 XORP_UINT_CAST(_vif_index));
    }
    // XXX: Conditionally print the VLAN ID
    if (_is_vlan) {
	vlan_str = c_format("{ vlan_id = %u } ", _vlan_id);
    }
    vif_index_str += pim_register_str;
    vif_index_str += vlan_str;

    string r = c_format("VIF %s { pif_index = %u } { enabled := %s } "
			"{ broadcast := %s } { loopback := %s } "
			"{ point_to_point := %s } { multicast := %s } "
			"{ flags := %u }",
			_vifname.c_str(), XORP_UINT_CAST(_pif_index),
			bool_c_str(_enabled), bool_c_str(_broadcast),
			bool_c_str(_loopback), bool_c_str(_point_to_point),
			bool_c_str(_multicast), XORP_UINT_CAST(_vif_flags));
    r += vif_index_str + string(" ") + IfTreeItem::str();

    return (r);
}

/* ------------------------------------------------------------------------- */
/* IfTreeAddr4 code */

int
IfTreeAddr4::set_prefix_len(uint32_t prefix_len)
{
    if (prefix_len > IPv4::addr_bitlen())
	return (XORP_ERROR);

    _prefix_len = prefix_len;
    mark(CHANGED);

    return (XORP_OK);
}

void
IfTreeAddr4::set_bcast(const IPv4& baddr)
{
    _oaddr = baddr;
    mark(CHANGED);
}

IPv4
IfTreeAddr4::bcast() const
{
    if (broadcast())
	return (_oaddr);

    return (IPv4::ZERO());
}

void
IfTreeAddr4::set_endpoint(const IPv4& oaddr)
{
    _oaddr = oaddr;
    mark(CHANGED);
}

IPv4
IfTreeAddr4::endpoint() const
{
    if (point_to_point())
	return (_oaddr);

    return (IPv4::ZERO());
}

void
IfTreeAddr4::finalize_state()
{
    set_state(NO_CHANGE);
}

string
IfTreeAddr4::str() const
{
    string r = c_format("IPv4Addr %s { enabled := %s } { broadcast := %s } "
			"{ loopback := %s } { point_to_point := %s } "
			"{ multicast := %s } "
			"{ prefix_len := %u }",
			_addr.str().c_str(), bool_c_str(_enabled),
			bool_c_str(_broadcast), bool_c_str(_loopback),
			bool_c_str(_point_to_point), bool_c_str(_multicast),
			XORP_UINT_CAST(_prefix_len));
    if (_point_to_point)
	r += c_format(" { endpoint := %s }", _oaddr.str().c_str());
    if (_broadcast)
	r += c_format(" { broadcast := %s }", _oaddr.str().c_str());
    r += string(" ") + IfTreeItem::str();

    return (r);
}

/* ------------------------------------------------------------------------- */
/* IfTreeAddr6 code */

int
IfTreeAddr6::set_prefix_len(uint32_t prefix_len)
{
    if (prefix_len > IPv6::addr_bitlen())
	return (XORP_ERROR);

    _prefix_len = prefix_len;
    mark(CHANGED);

    return (XORP_OK);
}

void
IfTreeAddr6::set_endpoint(const IPv6& oaddr)
{
    _oaddr = oaddr;
    mark(CHANGED);
}

IPv6
IfTreeAddr6::endpoint() const
{
    if (point_to_point())
	return (_oaddr);

    return (IPv6::ZERO());
}

void
IfTreeAddr6::finalize_state()
{
    set_state(NO_CHANGE);
}

string
IfTreeAddr6::str() const
{
    string r = c_format("IPv6Addr %s { enabled := %s } "
			"{ loopback := %s } { point_to_point := %s } "
			"{ multicast := %s } "
			"{ prefix_len := %u }",
			_addr.str().c_str(), bool_c_str(_enabled),
			bool_c_str(_loopback),
			bool_c_str(_point_to_point), bool_c_str(_multicast),
			XORP_UINT_CAST(_prefix_len));
    if (_point_to_point)
	r += c_format(" { endpoint := %s }", _oaddr.str().c_str());
    r += string(" ") + IfTreeItem::str();

    return (r);
}
