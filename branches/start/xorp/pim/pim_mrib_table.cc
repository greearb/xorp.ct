// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

#ident "$XORP: xorp/pim/pim_mrib_table.cc,v 1.20 2002/12/09 18:29:27 hodson Exp $"

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


//
// Exported variables
//

//
// Local constants definitions
//

//
// Local structures/classes, typedefs and macros
//

//
// Local variables
//

//
// Local functions prototypes
//


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
PimMribTable::add_pending_insert(const Mrib& mrib)
{
    add_modified_prefix(mrib.dest_prefix());
    MribTable::add_pending_insert(mrib);
}

void
PimMribTable::add_pending_remove(const Mrib& mrib)
{
    add_modified_prefix(mrib.dest_prefix());
    MribTable::add_pending_remove(mrib);
}

void
PimMribTable::commit_pending_transactions()
{
    MribTable::commit_pending_transactions();
    
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

// Search the list of modified address prefixes and remove
// all smaller prefixes. If there is a prefix that is larger
// than the new_addr_prefix, then don't do anything.
void
PimMribTable::add_modified_prefix(const IPvXNet& new_addr_prefix)
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
	
	if (old_addr_prefix.contains(new_addr_prefix))
	    return;		// Nothing to merge/enlarge
	if (new_addr_prefix.contains(old_addr_prefix)) {
	    // Remove the old address prefix, because the new one is larger
	    _modified_prefix_list.erase(iter2);
	}
    }
    
    //
    // Add the new address prefix to the list of modifed prefixes
    //
    _modified_prefix_list.push_back(new_addr_prefix);
}
