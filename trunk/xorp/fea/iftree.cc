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

#ident "$XORP: xorp/fea/iftree.cc,v 1.41 2007/05/08 00:07:57 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/c_format.hh"

#include "iftree.hh"


/* ------------------------------------------------------------------------- */
/* Misc */

static inline const char* true_false(bool b)
{
    return b ? "true" : "false";
}

/* ------------------------------------------------------------------------- */
/* IfTreeItem code */

string
IfTreeItem::str() const
{
    struct {
	State st;
	const char* desc;
    } t[] = { { CREATED,  "CREATED"  },
	      { DELETED,  "DELETED"  },
	      { CHANGED, "CHANGED" }
    };

    string r;
    for (size_t i = 0; i < sizeof(t) / sizeof(t[0]); i++) {
	if ((_st & t[i].st) == 0) continue;
	if (r.empty() == false) r += ",";
	r += t[i].desc;
    }
    return r;
}

/* ------------------------------------------------------------------------- */
/* IfTree code */

void
IfTree::clear()
{
    _interfaces.clear();
}

bool
IfTree::add_interface(const string& ifname)
{
    IfTreeInterface* ifp = find_interface(ifname);
    if (ifp != NULL) {
	ifp->mark(CREATED);
	return true;
    }
    _interfaces.insert(IfMap::value_type(ifname, IfTreeInterface(ifname)));
    return true;
}

IfTreeInterface *
IfTree::find_interface(const string& ifname)
{
    IfTree::IfMap::iterator iter = _interfaces.find(ifname);

    if (iter == interfaces().end())
	return (NULL);

    return (&iter->second);
}

const IfTreeInterface *
IfTree::find_interface(const string& ifname) const
{
    IfTree::IfMap::const_iterator iter = _interfaces.find(ifname);

    if (iter == interfaces().end())
	return (NULL);

    return (&iter->second);
}

IfTreeInterface *
IfTree::find_interface(uint32_t ifindex)
{
    IfTree::IfMap::iterator iter;
    for (iter = _interfaces.begin(); iter != _interfaces.end(); ++iter) {
	if (iter->second.pif_index() == ifindex)
	    return (&iter->second);
    }

    return (NULL);
}

const IfTreeInterface *
IfTree::find_interface(uint32_t ifindex) const
{
    IfTree::IfMap::const_iterator iter;
    for (iter = _interfaces.begin(); iter != _interfaces.end(); ++iter) {
	if (iter->second.pif_index() == ifindex)
	    return (&iter->second);
    }

    return (NULL);
}

IfTreeVif *
IfTree::find_vif(const string& ifname, const string& vifname)
{
    IfTreeInterface* ifp = find_interface(ifname);

    if (ifp == NULL)
	return (NULL);

    return (ifp->find_vif(vifname));
}

const IfTreeVif *
IfTree::find_vif(const string& ifname, const string& vifname) const
{
    const IfTreeInterface* ifp = find_interface(ifname);

    if (ifp == NULL)
	return (NULL);

    return (ifp->find_vif(vifname));
}

IfTreeAddr4 *
IfTree::find_addr(const string& ifname, const string& vifname,
		  const IPv4& addr)
{
    IfTreeVif* vifp = find_vif(ifname, vifname);

    if (vifp == NULL)
	return (NULL);

    return (vifp->find_addr(addr));
}

const IfTreeAddr4 *
IfTree::find_addr(const string& ifname, const string& vifname,
		  const IPv4& addr) const
{
    const IfTreeVif* vifp = find_vif(ifname, vifname);

    if (vifp == NULL)
	return (NULL);

    return (vifp->find_addr(addr));
}

IfTreeAddr6 *
IfTree::find_addr(const string& ifname, const string& vifname,
		  const IPv6& addr)
{
    IfTreeVif* vifp = find_vif(ifname, vifname);

    if (vifp == NULL)
	return (NULL);

    return (vifp->find_addr(addr));
}

const IfTreeAddr6 *
IfTree::find_addr(const string& ifname, const string& vifname,
		  const IPv6& addr) const
{
    const IfTreeVif* vifp = find_vif(ifname, vifname);

    if (vifp == NULL)
	return (NULL);

    return (vifp->find_addr(addr));
}

bool
IfTree::remove_interface(const string& ifname)
{
    IfTreeInterface* ifp = find_interface(ifname);
    if (ifp == NULL)
	return false;

    ifp->mark(DELETED);
    return true;
}

/**
 * Create a new interface or update its state if it already exists.
 *
 * @param other_iface the interface with the state to copy from.
 *
 * @return true on success, false if an error.
 */
bool
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
	this_ifp->copy_state(other_iface);

    //
    // Update existing vifs and addresses
    //
    for (vi = this_ifp->vifs().begin(); vi != this_ifp->vifs().end(); ++vi) {
	IfTreeVif& this_vif = vi->second;
	const IfTreeVif* other_vifp;

	other_vifp = other_iface.find_vif(this_vif.vifname());
	if (other_vifp == NULL) {
	    this_vif.mark(DELETED);
	    continue;
	}
	const IfTreeVif& other_vif = *other_vifp;

	if (! this_vif.is_same_state(this_vif))
	    this_vif.copy_state(other_vif);

	IfTreeVif::IPv4Map::iterator ai4;
	for (ai4 = this_vif.ipv4addrs().begin();
	     ai4 != this_vif.ipv4addrs().end();
	     ++ai4) {
	    const IfTreeAddr4* other_ap;
	    other_ap = other_vif.find_addr(ai4->second.addr());
	    if (other_ap == NULL) {
		ai4->second.mark(DELETED);
		continue;
	    }
	    if (! ai4->second.is_same_state(*other_ap))
		ai4->second.copy_state(*other_ap);
	}

	IfTreeVif::IPv6Map::iterator ai6;
	for (ai6 = this_vif.ipv6addrs().begin();
	     ai6 != this_vif.ipv6addrs().end();
	     ++ai6) {
	    const IfTreeAddr6* other_ap;
	    other_ap = other_vif.find_addr(ai6->second.addr());
	    if (other_ap == NULL) {
		ai6->second.mark(DELETED);
		continue;
	    }
	    if (! ai6->second.is_same_state(*other_ap))
		ai6->second.copy_state(*other_ap);
	}
    }

    //
    // Add the new vifs and addresses
    //
    for (ov = other_iface.vifs().begin();
	 ov != other_iface.vifs().end();
	 ++ov) {
	const IfTreeVif& other_vif = ov->second;
	IfTreeVif* this_vifp;

	if (this_ifp->find_vif(other_vif.vifname()) != NULL)
	    continue;		// The vif was already updated

	// Add the vif
	this_ifp->add_vif(other_vif.vifname());
	this_vifp = this_ifp->find_vif(other_vif.vifname());
	XLOG_ASSERT(this_vifp != NULL);
	IfTreeVif& this_vif = *this_vifp;
	this_vif.copy_state(other_vif);

	// Add the IPv4 addresses
	IfTreeVif::IPv4Map::const_iterator oa4;
	for (oa4 = other_vif.ipv4addrs().begin();
	     oa4 != other_vif.ipv4addrs().end();
	     ++oa4) {
	    IfTreeAddr4* this_ap;
	    this_ap = this_vif.find_addr(oa4->second.addr());
	    if (this_ap != NULL)
		continue;	// The address was already updated

	    // Add the address
	    this_vif.add_addr(oa4->second.addr());
	    this_ap = this_vif.find_addr(oa4->second.addr());
	    XLOG_ASSERT(this_ap != NULL);
	    this_ap->copy_state(oa4->second);
	}

	// Add the IPv6 addresses
	IfTreeVif::IPv6Map::const_iterator oa6;
	for (oa6 = other_vif.ipv6addrs().begin();
	     oa6 != other_vif.ipv6addrs().end();
	     ++oa6) {
	    IfTreeAddr6* this_ap;
	    this_ap = this_vif.find_addr(oa6->second.addr());
	    if (this_ap != NULL)
		continue;	// The address was already updated

	    // Add the address
	    this_vif.add_addr(oa6->second.addr());
	    this_ap = this_vif.find_addr(oa6->second.addr());
	    XLOG_ASSERT(this_ap != NULL);
	    this_ap->copy_state(oa6->second);
	}
    }

    return true;
}

void
IfTree::finalize_state()
{
    IfMap::iterator ii = _interfaces.begin();
    while (ii != _interfaces.end()) {
	//
	// If interface is marked as deleted, delete it.
	//
	if (ii->second.is_marked(DELETED)) {
	    _interfaces.erase(ii++);
	    continue;
	}
	//
	// Call finalize_state on interfaces that remain
	//
	ii->second.finalize_state();
	++ii;
    }
    set_state(NO_CHANGE);
}

string
IfTree::str() const
{
    string r;
    IfMap::const_iterator ii;

    for (ii = interfaces().begin(); ii != interfaces().end(); ++ii) {
	const IfTreeInterface& fi = ii->second;
	r += fi.str() + string("\n");
	for (IfTreeInterface::VifMap::const_iterator vi = fi.vifs().begin();
	     vi != fi.vifs().end(); ++vi) {
	    const IfTreeVif& fv = vi->second;
	    r += string("  ") + fv.str() + string("\n");
	    for (IfTreeVif::IPv4Map::const_iterator ai = fv.ipv4addrs().begin();
		 ai != fv.ipv4addrs().end(); ++ai) {
		const IfTreeAddr4& a = ai->second;
		r += string("    ") + a.str() + string("\n");
	    }
	    for (IfTreeVif::IPv6Map::const_iterator ai = fv.ipv6addrs().begin();
		 ai != fv.ipv6addrs().end(); ++ai) {
		const IfTreeAddr6& a = ai->second;
		r += string("    ") + a.str() + string("\n");
	    }
	}
    }

    return r;
}

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
IfTree&
IfTree::align_with(const IfTree& other)
{
    IfTree::IfMap::iterator ii;

    for (ii = interfaces().begin(); ii != interfaces().end(); ++ii) {
	const string& ifname = ii->second.ifname();
	const IfTreeInterface* other_ifp = other.find_interface(ifname);
	bool flipped = ii->second.flipped();

	if (! ii->second.enabled()) {
	    if ((other_ifp != NULL)
		&& (! ii->second.is_same_state(*other_ifp))) {
		ii->second.copy_state(*other_ifp);
		ii->second.set_enabled(false);
		ii->second.set_flipped(flipped);
	    }
	    continue;
	}

	if (other_ifp == NULL) {
	    //
	    // Mark local interface for deletion, not present in other,
	    // unless the local interface is marked as "soft" or
	    // "discard_emulated".
	    //
	    if (! (ii->second.is_soft() || ii->second.is_discard_emulated()))
		ii->second.mark(DELETED);
	    continue;
	} else {
	    if (! ii->second.is_same_state(*other_ifp)) {
		ii->second.copy_state(*other_ifp);
		ii->second.set_flipped(flipped);
	    }
	}

	IfTreeInterface::VifMap::iterator vi;
	for (vi = ii->second.vifs().begin();
	     vi != ii->second.vifs().end(); ++vi) {
	    const string& vifname = vi->second.vifname();
	    const IfTreeVif* other_vifp = other_ifp->find_vif(vifname);

	    if (! vi->second.enabled()) {
		if ((other_vifp != NULL)
		    && (! vi->second.is_same_state(*other_vifp))) {
		    vi->second.copy_state(*other_vifp);
		    vi->second.set_enabled(false);
		}
		continue;
	    }

	    if (other_vifp == NULL) {
		vi->second.mark(DELETED);
		continue;
	    } else {
		if (! vi->second.is_same_state(*other_vifp))
		    vi->second.copy_state(*other_vifp);
	    }

	    IfTreeVif::IPv4Map::iterator ai4;
	    for (ai4 = vi->second.ipv4addrs().begin();
		 ai4 != vi->second.ipv4addrs().end(); ++ai4) {
		const IfTreeAddr4* other_ap;
		other_ap = other_vifp->find_addr(ai4->second.addr());
		if (! ai4->second.enabled()) {
		    if ((other_ap != NULL)
			&& (! ai4->second.is_same_state(*other_ap))) {
			ai4->second.copy_state(*other_ap);
			ai4->second.set_enabled(false);
		    }
		    continue;
		}

		if (other_ap == NULL) {
		    ai4->second.mark(DELETED);
		} else {
		    if (! ai4->second.is_same_state(*other_ap))
			ai4->second.copy_state(*other_ap);
		}
	    }

	    IfTreeVif::IPv6Map::iterator ai6;
	    for (ai6 = vi->second.ipv6addrs().begin();
		 ai6 != vi->second.ipv6addrs().end(); ++ai6) {
		const IfTreeAddr6* other_ap;
		other_ap = other_vifp->find_addr(ai6->second.addr());
		if (! ai6->second.enabled()) {
		    if ((other_ap != NULL)
			&& (! ai6->second.is_same_state(*other_ap))) {
			ai6->second.copy_state(*other_ap);
			ai6->second.set_enabled(false);
		    }
		    continue;
		}

		if (other_ap == NULL) {
		    ai6->second.mark(DELETED);
		} else {
		    if (! ai6->second.is_same_state(*other_ap))
			ai6->second.copy_state(*other_ap);
		}
	    }
	}
    }

    return *this;
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
 *   Only if the interface is marked as "soft" or "discard_emulated",
 *   or if the item in the other state is marked as disabled, then it
 *   is not added.
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
	ii->second.mark(CREATED);
	IfTreeInterface::VifMap::iterator vi;
	for (vi = ii->second.vifs().begin();
	     vi != ii->second.vifs().end(); ++vi) {
	    vi->second.mark(CREATED);
	    IfTreeVif::IPv4Map::iterator ai4;
	    for (ai4 = vi->second.ipv4addrs().begin();
		 ai4 != vi->second.ipv4addrs().end(); ++ai4) {
		ai4->second.mark(CREATED);
	    }
	    IfTreeVif::IPv6Map::iterator ai6;
	    for (ai6 = vi->second.ipv6addrs().begin();
		 ai6 != vi->second.ipv6addrs().end(); ++ai6) {
		ai6->second.mark(CREATED);
	    }
	}
    }

    for (oi = other.interfaces().begin(); oi != other.interfaces().end(); ++oi) {
	if (! oi->second.enabled())
	    continue;		// XXX: ignore disabled state
	const string& ifname = oi->second.ifname();
	IfTreeInterface* ifp = find_interface(ifname);
	if (ifp == NULL) {
	    //
	    // Add local interface and mark it for deletion.
	    //
	    // Mark local interface for deletion, not present in other,
	    // unless the local interface is marked as "soft" or
	    // "discard_emulated".
	    //
	    if (! (oi->second.is_soft() || oi->second.is_discard_emulated())) {
		add_interface(ifname);
		ifp = find_interface(ifname);
		XLOG_ASSERT(ifp != NULL);
		ifp->copy_state(oi->second);
		ifp->mark(DELETED);
	    }
	}

	IfTreeInterface::VifMap::const_iterator ov;
	for (ov = oi->second.vifs().begin();
	     ov != oi->second.vifs().end(); ++ov) {
	    if (! ov->second.enabled())
		continue;	// XXX: ignore disabled state
	    const string& vifname = ov->second.vifname();
	    IfTreeVif* vifp = ifp->find_vif(vifname);
	    if (vifp == NULL) {
		//
		// Add local vif and mark it for deletion.
		//
		ifp->add_vif(vifname);
		vifp = ifp->find_vif(vifname);
		XLOG_ASSERT(vifp != NULL);
		vifp->copy_state(ov->second);
		vifp->mark(DELETED);
	    }

	    IfTreeVif::IPv4Map::const_iterator oa4;
	    for (oa4 = ov->second.ipv4addrs().begin();
		 oa4 != ov->second.ipv4addrs().end(); ++oa4) {
		if (! oa4->second.enabled())
		    continue;	// XXX: ignore disabled state
		IfTreeAddr4* ap = vifp->find_addr(oa4->second.addr());
		if (ap == NULL) {
		    //
		    // Add local IPv4 address and mark it for deletion.
		    //
		    vifp->add_addr(oa4->second.addr());
		    ap = vifp->find_addr(oa4->second.addr());
		    XLOG_ASSERT(ap != NULL);
		    ap->copy_state(oa4->second);
		    ap->mark(DELETED);
		}
	    }

	    IfTreeVif::IPv6Map::const_iterator oa6;
	    for (oa6 = ov->second.ipv6addrs().begin();
		 oa6 != ov->second.ipv6addrs().end(); ++oa6) {
		if (! oa6->second.enabled())
		    continue;	// XXX: ignore disabled state
		IfTreeAddr6* ap = vifp->find_addr(oa6->second.addr());
		if (ap == NULL) {
		    //
		    // Add local IPv6 address and mark it for deletion.
		    //
		    vifp->add_addr(oa6->second.addr());
		    ap = vifp->find_addr(oa6->second.addr());
		    XLOG_ASSERT(ap != NULL);
		    ap->copy_state(oa6->second);
		    ap->mark(DELETED);
		}
	    }
	}
    }

    return *this;
}

/**
 * Prune bogus deleted state.
 * 
 * If an item from the local tree is marked as deleted, but is not
 * in the other tree, then it is removed.
 * 
 * @param old_iftree the old tree with the state that is used as reference.
 * @return the modified configuration tree.
 */
IfTree&
IfTree::prune_bogus_deleted_state(const IfTree& old_iftree)
{
    IfTree::IfMap::iterator ii;

    ii = _interfaces.begin();
    while (ii != _interfaces.end()) {
	const string& ifname = ii->second.ifname();
	if (! ii->second.is_marked(DELETED)) {
	    ++ii;
	    continue;
	}
	const IfTreeInterface* old_ifp;
	old_ifp = old_iftree.find_interface(ifname);
	if (old_ifp == NULL) {
	    // Remove this item from the local tree
	    _interfaces.erase(ii++);
	    continue;
	}

	IfTreeInterface::VifMap::iterator vi;
	vi = ii->second.vifs().begin();
	while (vi != ii->second.vifs().end()) {
	    const string& vifname = vi->second.vifname();
	    if (! vi->second.is_marked(DELETED)) {
		++vi;
		continue;
	    }
	    const IfTreeVif* old_vifp;
	    old_vifp = old_ifp->find_vif(vifname);
	    if (old_vifp == NULL) {
		// Remove this item from the local tree
		ii->second.vifs().erase(vi++);
		continue;
	    }

	    IfTreeVif::IPv4Map::iterator ai4;
	    ai4 = vi->second.ipv4addrs().begin();
	    while (ai4 != vi->second.ipv4addrs().end()) {
		if (! ai4->second.is_marked(DELETED)) {
		    ++ai4;
		    continue;
		}
		const IfTreeAddr4* old_ap;
		old_ap = old_vifp->find_addr(ai4->second.addr());
		if (old_ap == NULL) {
		    // Remove this item from the local tree
		    vi->second.ipv4addrs().erase(ai4++);
		    continue;
		}
		++ai4;
	    }

	    IfTreeVif::IPv6Map::iterator ai6;
	    ai6 = vi->second.ipv6addrs().begin();
	    while (ai6 != vi->second.ipv6addrs().end()) {
		if (! ai6->second.is_marked(DELETED)) {
		    ++ai6;
		    continue;
		}
		const IfTreeAddr6* old_ap;
		old_ap = old_vifp->find_addr(ai6->second.addr());
		if (old_ap == NULL) {
		    // Remove this item from the local tree
		    vi->second.ipv6addrs().erase(ai6++);
		    continue;
		}
		++ai6;
	    }
	    ++vi;
	}
	++ii;
    }

    return *this;
}

/**
 * Track modifications from the live config state as read from the kernel.
 *
 * All interface-related modifications as received by the observer
 * mechanism are recorded in a local copy of the interface tree
 * (the live configuration tree). Some of those modifications however
 * should be propagated to the XORP local configuration tree.
 * This method updates a local configuration tree with only the relevant
 * modifications of the live configuration tree:
 * - Only if an item is in the local configuration tree, its state
 *   may be modified.
 * - If the "no_carrier" flag of an interface is changed in the live
 *   configuration tree, the corresponding flag in the local configuration
 *   tree is updated.
 * 
 * @param other the live configuration tree whose modifications are
 * tracked.
 * @return modified configuration structure.
 */
IfTree&
IfTree::track_live_config_state(const IfTree& other)
{
    IfTreeInterface* ifp;
    IfTree::IfMap::const_iterator oi;

    for (oi = other.interfaces().begin(); oi != other.interfaces().end(); ++oi) {
	const string& ifname = oi->second.ifname();
	ifp = find_interface(ifname);
	if (ifp == NULL)
	    continue;

	//
	// Update the "no_carrier" flag
	//
	if (ifp->no_carrier() != oi->second.no_carrier())
	    ifp->set_no_carrier(oi->second.no_carrier());
    }

    return *this;
}

/* ------------------------------------------------------------------------- */
/* IfTreeInterface code */

IfTreeInterface::IfTreeInterface(const string& ifname)
    : IfTreeItem(), _ifname(ifname), _pif_index(0),
      _enabled(false), _discard(false), _is_discard_emulated(false),
      _mtu(0), _no_carrier(false), _flipped(false), _interface_flags(0)
{}

bool
IfTreeInterface::add_vif(const string& vifname)
{
    IfTreeVif* vifp;

    vifp = find_vif(vifname);
    if (vifp != NULL) {
	vifp->mark(CREATED);
	return true;
    }

    _vifs.insert(VifMap::value_type(vifname, IfTreeVif(name(), vifname)));
    return true;
}

bool
IfTreeInterface::remove_vif(const string& vifname)
{
    IfTreeVif* vifp;

    vifp = find_vif(vifname);
    if (vifp == NULL)
	return false;

    vifp->mark(DELETED);
    return true;
}

IfTreeVif *
IfTreeInterface::find_vif(const string& vifname)
{
    IfTreeInterface::VifMap::iterator iter = _vifs.find(vifname);

    if (iter == _vifs.end())
	return (NULL);

    return (&iter->second);
}

const IfTreeVif *
IfTreeInterface::find_vif(const string& vifname) const
{
    IfTreeInterface::VifMap::const_iterator iter = _vifs.find(vifname);

    if (iter == _vifs.end())
	return (NULL);

    return (&iter->second);
}

IfTreeVif *
IfTreeInterface::find_vif(uint32_t ifindex)
{
    IfTreeInterface::VifMap::iterator iter;
    for (iter = _vifs.begin(); iter != _vifs.end(); ++iter) {
	if (iter->second.pif_index() == ifindex)
	    return (&iter->second);
    }

    return (NULL);
}

const IfTreeVif *
IfTreeInterface::find_vif(uint32_t ifindex) const
{
    IfTreeInterface::VifMap::const_iterator iter;
    for (iter = _vifs.begin(); iter != _vifs.end(); ++iter) {
	if (iter->second.pif_index() == ifindex)
	    return (&iter->second);
    }

    return (NULL);
}

IfTreeAddr4 *
IfTreeInterface::find_addr(const string& vifname, const IPv4& addr)
{
    IfTreeVif* vif = find_vif(vifname);

    if (vif == NULL)
	return (NULL);

    return (vif->find_addr(addr));
}

const IfTreeAddr4 *
IfTreeInterface::find_addr(const string& vifname, const IPv4& addr) const
{
    const IfTreeVif* vif = find_vif(vifname);

    if (vif == NULL)
	return (NULL);

    return (vif->find_addr(addr));
}

IfTreeAddr6 *
IfTreeInterface::find_addr(const string& vifname, const IPv6& addr)
{
    IfTreeVif* vif = find_vif(vifname);

    if (vif == NULL)
	return (NULL);

    return (vif->find_addr(addr));
}

const IfTreeAddr6 *
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
    VifMap::iterator vi = _vifs.begin();
    while (vi != _vifs.end()) {
	//
	// If interface is marked as deleted, delete it.
	//
	if (vi->second.is_marked(DELETED)) {
	    _vifs.erase(vi++);
	    continue;
	}
	//
	// Call finalize_state on vifs that remain
	//
	vi->second.finalize_state();
	++vi;
    }
    set_state(NO_CHANGE);
}

string
IfTreeInterface::str() const
{
    return c_format("Interface %s { enabled := %s } "
		    "{ mtu := %u } { mac := %s } {no_carrier = %s} "
		    "{ flipped := %s } { flags := %u }",
		    _ifname.c_str(),
		    true_false(_enabled),
		    XORP_UINT_CAST(_mtu),
		    _mac.str().c_str(),
		    true_false(_no_carrier),
		    true_false(_flipped),
		    XORP_UINT_CAST(_interface_flags))
	+ string(" ")
	+ IfTreeItem::str();
}

/* ------------------------------------------------------------------------- */
/* IfTreeVif code */

IfTreeVif::IfTreeVif(const string& ifname, const string& vifname)
    : IfTreeItem(), _ifname(ifname), _vifname(vifname),
      _pif_index(0), _enabled(false),
      _broadcast(false), _loopback(false), _point_to_point(false),
      _multicast(false), _pim_register(false)
{}

bool
IfTreeVif::add_addr(const IPv4& addr)
{
    IfTreeAddr4* ap = find_addr(addr);

    if (ap != NULL) {
	ap->mark(CREATED);
	return true;
    }
    _ipv4addrs.insert(IPv4Map::value_type(addr, IfTreeAddr4(addr)));
    return true;
}

bool
IfTreeVif::remove_addr(const IPv4& addr)
{
    IfTreeAddr4* ap = find_addr(addr);

    if (ap == NULL)
	return false;
    ap->mark(DELETED);
    return true;
}

bool
IfTreeVif::add_addr(const IPv6& addr)
{
    IfTreeAddr6* ap = find_addr(addr);

    if (ap != NULL) {
	ap->mark(CREATED);
	return true;
    }
    _ipv6addrs.insert(IPv6Map::value_type(addr, IfTreeAddr6(addr)));
    return true;
}

bool
IfTreeVif::remove_addr(const IPv6& addr)
{
    IfTreeAddr6* ap = find_addr(addr);

    if (ap == NULL)
	return false;
    ap->mark(DELETED);
    return true;
}

IfTreeAddr4*
IfTreeVif::find_addr(const IPv4& addr)
{
    IfTreeVif::IPv4Map::iterator iter = _ipv4addrs.find(addr);

    if (iter == _ipv4addrs.end())
	return (NULL);

    return (&iter->second);
}

const IfTreeAddr4*
IfTreeVif::find_addr(const IPv4& addr) const
{
    IfTreeVif::IPv4Map::const_iterator iter = _ipv4addrs.find(addr);

    if (iter == _ipv4addrs.end())
	return (NULL);

    return (&iter->second);
}

IfTreeAddr6*
IfTreeVif::find_addr(const IPv6& addr)
{
    IfTreeVif::IPv6Map::iterator iter = _ipv6addrs.find(addr);

    if (iter == _ipv6addrs.end())
	return (NULL);

    return (&iter->second);
}

const IfTreeAddr6*
IfTreeVif::find_addr(const IPv6& addr) const
{
    IfTreeVif::IPv6Map::const_iterator iter = _ipv6addrs.find(addr);

    if (iter == _ipv6addrs.end())
	return (NULL);

    return (&iter->second);
}

void
IfTreeVif::finalize_state()
{
    for (IPv4Map::iterator ai = _ipv4addrs.begin(); ai != _ipv4addrs.end(); ) {
	//
	// If address is marked as deleted, delete it.
	//
	if (ai->second.is_marked(DELETED)) {
	    _ipv4addrs.erase(ai++);
	    continue;
	}
	//
	// Call finalize_state on addresses that remain
	//
	ai->second.finalize_state();
	++ai;
    }

    for (IPv6Map::iterator ai = _ipv6addrs.begin(); ai != _ipv6addrs.end(); ) {
	//
	// If address is marked as deleted, delete it.
	//
	if (ai->second.is_marked(DELETED)) {
	    _ipv6addrs.erase(ai++);
	    continue;
	}
	//
	// Call finalize_state on interfaces that remain
	//
	ai->second.finalize_state();
	++ai;
    }
    set_state(NO_CHANGE);
}

string
IfTreeVif::str() const
{
    string pim_register_str;

    //
    // XXX: Conditionally print the pim_register flag, because it is
    // used only by the MFEA.
    //
    if (_pim_register)
	pim_register_str = c_format("{ pim_register := %s } ",
				    true_false(_pim_register));
    pim_register_str += string(" ");	// XXX: unconditional extra space

    return c_format("VIF %s { enabled := %s } { broadcast := %s } "
		    "{ loopback := %s } { point_to_point := %s } "
		    "{ multicast := %s } ",
		    _vifname.c_str(), true_false(_enabled),
		    true_false(_broadcast), true_false(_loopback),
		    true_false(_point_to_point), true_false(_multicast))
	+ pim_register_str + IfTreeItem::str();
}

/* ------------------------------------------------------------------------- */
/* IfTreeAddr4 code */

bool
IfTreeAddr4::set_prefix_len(uint32_t prefix_len)
{
    if (prefix_len > IPv4::addr_bitlen())
	return false;

    _prefix_len = prefix_len;
    mark(CHANGED);
    return true;
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
	return _oaddr;
    return IPv4::ZERO();
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
	return _oaddr;
    return IPv4::ZERO();
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
			_addr.str().c_str(), true_false(_enabled),
			true_false(_broadcast), true_false(_loopback),
			true_false(_point_to_point), true_false(_multicast),
			XORP_UINT_CAST(_prefix_len));
    if (_point_to_point)
	r += c_format(" { endpoint := %s }", _oaddr.str().c_str());
    if (_broadcast)
	r += c_format(" { broadcast := %s }", _oaddr.str().c_str());
    r += string(" ") + IfTreeItem::str();
    return r;
}

/* ------------------------------------------------------------------------- */
/* IfTreeAddr6 code */

bool
IfTreeAddr6::set_prefix_len(uint32_t prefix_len)
{
    if (prefix_len > IPv6::addr_bitlen())
	return false;

    _prefix_len = prefix_len;
    mark(CHANGED);
    return true;
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
	return _oaddr;
    return IPv6::ZERO();
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
			_addr.str().c_str(), true_false(_enabled),
			true_false(_loopback),
			true_false(_point_to_point), true_false(_multicast),
			XORP_UINT_CAST(_prefix_len));
    if (_point_to_point)
	r += c_format(" { endpoint := %s }", _oaddr.str().c_str());
    r += string(" ") + IfTreeItem::str();
    return r;
}
