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

#ident "$XORP: xorp/pim/pim_mrib_table.cc,v 1.6 2004/06/10 22:41:31 hodson Exp $"

//
// PIM Multicast Routing Information Base Table implementation.
//


#include "pim_module.h"
#include "pim_private.hh"

#include "libxorp/time_slice.hh"

#include "pim_mre.hh"
#include "pim_mrt.hh"
#include "pim_node.hh"
#include "pim_mrib_table.hh"
#include "pim_vif.hh"


PimMribTable::PimMribTable(PimNode& pim_node)
    : MribTable(pim_node.family()),
      _pim_node(pim_node)
{
    
}

PimMribTable::~PimMribTable()
{
    
}

int
PimMribTable::family()
{
    return (pim_node().family());
}

PimMrt&
PimMribTable::pim_mrt()
{
    return (pim_node().pim_mrt());
}

void
PimMribTable::clear()
{
    MribTable::clear();
    
    add_modified_prefix(IPvXNet(IPvX::ZERO(family()), 0));
    apply_mrib_changes();
}

Mrib *
PimMribTable::find(const IPvX& address) const
{
    Mrib *mrib;
    
    mrib = MribTable::find(address);
    
    if (mrib != NULL) {
	//
	// Test if the RPF vif is valid and UP
	//
	PimVif *pim_vif = pim_node().vif_find_by_vif_index(mrib->next_hop_vif_index());
	if ((pim_vif == NULL) || (! pim_vif->is_up()))
	    return (NULL);
    }
    
    return (mrib);
}

void
PimMribTable::add_pending_insert(uint32_t tid, const Mrib& mrib,
				 const string& next_hop_vif_name)
{
    add_modified_prefix(mrib.dest_prefix());

    //
    // XXX: if the next-hop interface points toward the loopback interface
    // (e.g., in case of KAME IPv6 stack), or points toward an invalid
    // interface (e.g., in case the loopback interface is not configured),
    // and if the MRIB entry is for one of my own addresses, then we
    // overwrite it with the network interface this address belongs to.
    //
    uint16_t vif_index = mrib.next_hop_vif_index();
    PimVif *pim_vif = pim_node().vif_find_by_vif_index(vif_index);
    if ((vif_index == Vif::VIF_INDEX_INVALID)
	|| ((pim_vif != NULL) && pim_vif->is_loopback())) {
	const IPvXNet& dest_prefix = mrib.dest_prefix();
	if (dest_prefix.prefix_len() == IPvX::addr_bitlen(family())) {
	    pim_vif = pim_node().vif_find_by_addr(dest_prefix.masked_addr());
	    if (pim_vif != NULL) {
		Mrib modified_mrib(mrib);
		modified_mrib.set_next_hop_vif_index(pim_vif->vif_index());
		MribTable::add_pending_insert(tid, modified_mrib);
		return;
	    }
	}
    }
    MribTable::add_pending_insert(tid, mrib);

    if (pim_vif == NULL)
	add_unresolved_prefix(mrib.dest_prefix(), next_hop_vif_name);
}

void
PimMribTable::add_pending_remove(uint32_t tid, const Mrib& mrib)
{
    add_modified_prefix(mrib.dest_prefix());
    MribTable::add_pending_remove(tid, mrib);

    delete_unresolved_prefix(mrib.dest_prefix());
}

void
PimMribTable::commit_pending_transactions(uint32_t tid)
{
    MribTable::commit_pending_transactions(tid);
    
    apply_mrib_changes();
}

//
// Apply changes to the PIM MRT due to MRIB changes.
// XXX: a number of tasks (one per modified prefix) are scheduled
// to perform the appropriate actions.
//
void
PimMribTable::apply_mrib_changes()
{
    while (! _modified_prefix_list.empty()) {
	// Get the modified prefix address
	IPvXNet modified_prefix_addr = _modified_prefix_list.front();
	_modified_prefix_list.pop_front();
	
	pim_node().pim_mrt().add_task_mrib_changed(modified_prefix_addr);
    }
}

//
// Search the list of modified address prefixes and remove
// all smaller prefixes. If there is a prefix that is larger
// than the modified_prefix, then don't do anything.
//
void
PimMribTable::add_modified_prefix(const IPvXNet& modified_prefix)
{
    //
    // Search the list for overlapping address prefixes.
    //
    list<IPvXNet>::iterator iter1;
    for (iter1 = _modified_prefix_list.begin();
	 iter1 != _modified_prefix_list.end(); ) {
	list<IPvXNet>::iterator iter2 = iter1;
	++iter1;
	IPvXNet& old_addr_prefix = *iter2;
	
	if (old_addr_prefix.contains(modified_prefix))
	    return;		// Nothing to merge/enlarge
	if (modified_prefix.contains(old_addr_prefix)) {
	    // Remove the old address prefix, because the new one is larger
	    _modified_prefix_list.erase(iter2);
	}
    }
    
    //
    // Add the new address prefix to the list of modifed prefixes
    //
    _modified_prefix_list.push_back(modified_prefix);
}

void
PimMribTable::add_unresolved_prefix(const IPvXNet& dest_prefix,
				    const string& next_hop_vif_name)
{
    map<IPvXNet, string>::iterator iter;

    // Erase old unresolved entry (if exists)
    iter = _unresolved_prefixes.find(dest_prefix);
    if (iter != _unresolved_prefixes.end())
	_unresolved_prefixes.erase(iter);

    _unresolved_prefixes.insert(make_pair(dest_prefix, next_hop_vif_name));
}

void
PimMribTable::delete_unresolved_prefix(const IPvXNet& dest_prefix)
{
    map<IPvXNet, string>::iterator iter;

    // Erase unresolved entry (if exists)
    iter = _unresolved_prefixes.find(dest_prefix);
    if (iter != _unresolved_prefixes.end())
	_unresolved_prefixes.erase(iter);
}

void
PimMribTable::resolve_prefixes_by_vif_name(const string& next_hop_vif_name,
					   uint16_t next_hop_vif_index)
{
    map<IPvXNet, string>::iterator iter, iter2;

    //
    // Find all unresolved prefixes for the given vif name
    // and update their vif index.
    //
    for (iter = _unresolved_prefixes.begin();
	 iter != _unresolved_prefixes.end();
	 ) {
	iter2 = iter;
	++iter;
	if (iter2->second != next_hop_vif_name)
	    continue;
	// Update the mrib entry
	MribTable::update_entry_vif_index(iter2->first,
					  next_hop_vif_index);
	_modified_prefix_list.push_back(iter2->first);
	_unresolved_prefixes.erase(iter2);
    }

    apply_mrib_changes();
}
