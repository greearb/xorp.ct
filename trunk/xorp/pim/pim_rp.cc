// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/pim/pim_rp.cc,v 1.2 2003/03/10 23:20:52 hodson Exp $"


//
// PIM RP information implementation.
// PIM-SMv2 (draft-ietf-pim-sm-new-* and draft-ietf-pim-sm-bsr-*)
//


#include "pim_module.h"
#include "pim_private.hh"
#include "libxorp/time_slice.hh"
#include "libxorp/utils.hh"
#include "pim_mfc.hh"
#include "pim_mre.hh"
#include "pim_node.hh"
#include "pim_rp.hh"
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
// The RP hash function. Stolen from Eddy Rusty's (eddy@isi.edu)
// implementation (for compatibility ;)
//
#define SEED1   1103515245
#define SEED2   12345
#define RP_HASH_VALUE(G, M, C) (((SEED1) * (((SEED1) * ((G) & (M)) + (SEED2)) ^ (C)) + (SEED2)) % 0x80000000U)
#define RP_HASH_VALUE_DERIVED(Gderived, C) (((SEED1) * (((SEED1) * ((Gderived)) + (SEED2)) ^ (C)) + (SEED2)) % 0x80000000U)

//
// Local variables
//

//
// Local functions prototypes
//


/**
 * RpTable::RpTable:
 * @pim_node: The associated PIM node.
 * 
 * RpTable constructor.
 **/
RpTable::RpTable(PimNode& pim_node)
    : ProtoUnit(pim_node.family(), pim_node.module_id()),
      _pim_node(pim_node)
{
    
}

/**
 * RpTable::~RpTable:
 * @void: 
 * 
 * RpTable destructor.
 * 
 **/
RpTable::~RpTable(void)
{
    stop();
}

/**
 * RpTable::start:
 * @void: 
 * 
 * Start the RP table.
 * 
 * Return value: %XORP_OK on success, otherwize %XORP_ERROR.
 **/
int
RpTable::start(void)
{
    if (ProtoUnit::start() < 0)
	return (XORP_ERROR);
    
    // Perform misc. RP table-specific start operations
    // TODO: IMPLEMENT IT!
    
    return (XORP_OK);
}

/**
 * RpTable::stop:
 * @void: 
 * 
 * Stop the RP table.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
RpTable::stop(void)
{
    if (ProtoUnit::stop() < 0)
	return (XORP_ERROR);
    
    // Perform misc. RP table-specific stop operations
    // TODO: IMPLEMENT IT!
    
    // Remove all RP entries
    // TODO: any other actions?
    delete_pointers_list(_rp_list);
    delete_pointers_list(_processing_rp_list);
    
    return (XORP_OK);
}

//
// Lookup the RP for @group_addr
//
PimRp *
RpTable::rp_find(const IPvX& group_addr)
{
    list<PimRp *>::iterator iter;
    PimRp *best_rp = NULL;
    
    for (iter = _rp_list.begin(); iter != _rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	
	if (! pim_rp->group_prefix().contains(group_addr))
	    continue;
	
	switch (pim_rp->rp_learned_method()) {
	case PimRp::RP_LEARNED_METHOD_AUTORP:
	case PimRp::RP_LEARNED_METHOD_BOOTSTRAP:
	case PimRp::RP_LEARNED_METHOD_STATIC:
	    break;
	default:
	    // Unknown learned method. Ignore the RP.
	    continue;
	}
	
	//
	// Found a matching RP
	//
	if (best_rp == NULL) {
	    best_rp = pim_rp;
	    continue;
	}
	
	best_rp = compare_rp(group_addr, best_rp, pim_rp);
    }
    
    return (best_rp);
}

PimRp *
RpTable::compare_rp(const IPvX& group_addr, PimRp *rp1, PimRp *rp2) const
{
    //
    // Choose the longer match (if exist)
    //
    if (rp1->group_prefix().prefix_len() > rp2->group_prefix().prefix_len())
	return (rp1);
    
    if (rp1->group_prefix().prefix_len() < rp2->group_prefix().prefix_len())
	return (rp2);
    
    //
    // Compare priorities
    //
    //
    // XXX: Only the Bootstrap-derived RPs priority is defined as
    // 'smaller is better'. The priority of the RPs derived by alternative
    // methods is not defined. Typically, a domain would use only one
    // method to derive the Cand-RP set. If more than one method is
    // used, extra care is needed that the priority comparison among
    // all RPs is unambiguous.
    //
    switch (rp1->rp_learned_method()) {
    case PimRp::RP_LEARNED_METHOD_AUTORP:
    case PimRp::RP_LEARNED_METHOD_BOOTSTRAP:
    case PimRp::RP_LEARNED_METHOD_STATIC:
	switch (rp2->rp_learned_method()) {
	case PimRp::RP_LEARNED_METHOD_AUTORP:
	case PimRp::RP_LEARNED_METHOD_BOOTSTRAP:
	case PimRp::RP_LEARNED_METHOD_STATIC:
	    // Smaller is better (XXX: true for BOOTSTRAP)
	    if (rp1->rp_priority() < rp2->rp_priority())
		return (rp1);
	    if (rp1->rp_priority() > rp2->rp_priority())
		return (rp2);
	    break;
	default:
	    XLOG_UNREACHABLE();
	    // Unknown learned method. Ignore the RP.
	    return (rp1);
	}
	break;
    default:
	XLOG_UNREACHABLE();
	// Unknown learned method. Ignore the RP.
	return (rp2);
    }
    //
    // Compute the hash function values for both RPs
    //
    // First compute the group masked address, and then its derived
    // form (if it is longer than 32 bits)
    //
    uint32_t derived_masked_group1 = 0;
    uint32_t derived_masked_group2 = 0;
    uint32_t derived_rp1 = 0;
    uint32_t derived_rp2 = 0;
    do {
	IPvXNet group_masked_addr1(group_addr, rp1->hash_masklen());
	IPvXNet group_masked_addr2(group_addr, rp2->hash_masklen());
	
	derived_masked_group1 = derived_addr(group_masked_addr1.masked_addr());
	derived_masked_group2 = derived_addr(group_masked_addr2.masked_addr());
	derived_rp1 = derived_addr(rp1->rp_addr());
	derived_rp2 = derived_addr(rp2->rp_addr());
    } while (false);
    
    uint32_t hash_value1 = RP_HASH_VALUE_DERIVED(derived_masked_group1,
						 derived_rp1);
    uint32_t hash_value2 = RP_HASH_VALUE_DERIVED(derived_masked_group2,
						 derived_rp2);
    if (hash_value1 > hash_value2)
	return (rp1);
    if (hash_value1 < hash_value2)
	return (rp2);
    if (rp1->rp_addr() > rp2->rp_addr())
	return (rp1);
    if (rp1->rp_addr() < rp2->rp_addr())
	return (rp2);
    
    return (rp1);	// XXX: both RPs seem identical
}  
  
//
// Compute and return the XOR value of an address as an 32-bit integer value.
//
uint32_t
RpTable::derived_addr(const IPvX& addr) const
{
    size_t addr_size = addr.addr_size()/sizeof(uint32_t);
    uint32_t addr_array[addr_size];
    uint32_t result = 0;
	    
    addr.copy_out((uint8_t *)addr_array);
    for (size_t i = 0; i < addr_size; i++)
	result ^= addr_array[i];
    
    return (result);
}

//
// Return: a pointer to the RP entry if it can be added, otherwise NULL.
//
PimRp *
RpTable::add_rp(const IPvX& rp_addr,
		uint8_t rp_priority,
		const IPvXNet& group_prefix,
		uint8_t hash_masklen,
		PimRp::rp_learned_method_t rp_learned_method)
{
    //
    // Check if we have already an entry for that RP 
    //
    list<PimRp *>::iterator iter;
    for (iter = _rp_list.begin(); iter != _rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	if (pim_rp->rp_addr() != rp_addr)
	    continue;
	if (pim_rp->group_prefix() != group_prefix)
	    continue;
	if (pim_rp->rp_learned_method() != rp_learned_method) {
	    // TODO: XXX: do we allow same RP is installed by different
	    // methods? Maybe no, so here we should print a warning instead
	    // and refuse to add that RP.
	    XLOG_WARNING("Cannot add RP %s for group prefix %s "
			 "and learned method %s: already have same RP with "
			 "learned method %s",
			 cstring(rp_addr),
			 cstring(group_prefix),
			 PimRp::rp_learned_method_str(rp_learned_method).c_str(),
			 pim_rp->rp_learned_method_str().c_str());
	    continue;
	}
	if ((pim_rp->rp_priority() == rp_priority)
	    && (pim_rp->hash_masklen() == hash_masklen)) {
	    // Found exactly same entry. Return it.
	    return (pim_rp);
	}
	// Found an entry for same RP and group prefix.
	// Update its priority and hash masklen.
	pim_rp->set_rp_priority(rp_priority);
	pim_rp->set_hash_masklen(hash_masklen);
	pim_rp->set_is_updated(true);
	return (pim_rp);
    }
    
    //
    // Add a new RP entry
    //
    PimRp *new_pim_rp = new PimRp(*this, rp_addr, rp_priority,
				  group_prefix, hash_masklen,
				  rp_learned_method);
    _rp_list.push_back(new_pim_rp);
    new_pim_rp->set_is_updated(true);
    return (new_pim_rp);
}

//
// Delete an RP entry
// Return %XORP_OK on success, otherwise %XORP_ERROR.
//
int
RpTable::delete_rp(const IPvX& rp_addr,
		   const IPvXNet& group_prefix,
		   PimRp::rp_learned_method_t rp_learned_method)
{
    //
    // Find the RP entry
    //
    list<PimRp *>::iterator iter;
    for (iter = _rp_list.begin(); iter != _rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	if (pim_rp->rp_addr() != rp_addr)
	    continue;
	if (pim_rp->group_prefix() != group_prefix)
	    continue;
	if (pim_rp->rp_learned_method() != rp_learned_method) {
	    // TODO: XXX: do we allow same RP is installed by different
	    // methods? Maybe no, so here we should print a warning instead
	    // and refuse to delete that RP.
	    XLOG_WARNING("Cannot delete RP %s for group prefix %s "
			 "and learned method %s: already have same RP with "
			 "learned method %s",
			 cstring(rp_addr),
			 cstring(group_prefix),
			 PimRp::rp_learned_method_str(rp_learned_method).c_str(),
			 pim_rp->rp_learned_method_str().c_str());
	    continue;
	}
	
	//
	// Entry found. Delete it from the list of RPs and, if needed,
	// put it on the list to process its PimMre and PimMfc entries.
	//
	_rp_list.erase(iter);
	if (pim_rp->pim_mre_wc_list().empty()
	    && pim_rp->pim_mre_sg_list().empty()
	    && pim_rp->pim_mre_sg_rpt_list().empty()
	    && pim_rp->pim_mfc_list().empty()
	    && pim_rp->processing_pim_mre_wc_list().empty()
	    && pim_rp->processing_pim_mre_sg_list().empty()
	    && pim_rp->processing_pim_mre_sg_rpt_list().empty()
	    && pim_rp->processing_pim_mfc_list().empty()) {
	    delete pim_rp;
	} else {
	    _processing_rp_list.push_back(pim_rp);
	    pim_rp->set_is_updated(true);
	}
	
	return (XORP_OK);
    }
    
    return (XORP_ERROR);
}

//
// Apply the RP changes to the PIM multicast routing table
// Return true if the processing was saved for later because it was taking too
// long time, otherwise return false.
//
bool
RpTable::apply_rp_changes()
{
    bool ret_value = false;
    list<PimRp *>::iterator rp_iter1;
    
    //
    // Mark all RP entries that may have been affected.
    // E.g., if a shorter-match entry has been updated,
    // all longer-match entries need update too.
    //
    for (rp_iter1 = _rp_list.begin(); rp_iter1 != _rp_list.end(); ++rp_iter1) {
	list<PimRp *>::iterator rp_iter2 = rp_iter1;
	PimRp *pim_rp1 = *rp_iter1;
	if (! pim_rp1->is_updated())
	    continue;
	for (rp_iter2 = _rp_list.begin();
	     rp_iter2 != _rp_list.end();
	     ++rp_iter2) {
	    PimRp *pim_rp2 = *rp_iter2;
	    if (pim_rp2->group_prefix().contains(pim_rp1->group_prefix()))
		pim_rp2->set_is_updated(true);
	}
    }
    
    //
    // Schedule the tasks to take care of the RP change
    //
    // First process the entries that may have to change to a new RP
    for (rp_iter1 = _rp_list.begin(); rp_iter1 != _rp_list.end(); ++rp_iter1) {
	PimRp *pim_rp = *rp_iter1;
	PimMre *pim_mre;
	if (! pim_rp->is_updated())
	    continue;
	pim_rp->set_is_updated(false);
	// XXX: add an (*,*,RP) entry even if it is idle.
	pim_mre = pim_node().pim_mrt().pim_mre_find(pim_rp->rp_addr(),
						    IPvX::ZERO(family()),
						    PIM_MRE_RP,
						    PIM_MRE_RP);
	XLOG_ASSERT(pim_mre != NULL);
	pim_node().pim_mrt().add_task_rp_changed(pim_rp->rp_addr());
	ret_value = true;
    }
    // Next process the entries that have obsolete, or no RP entry
    for (rp_iter1 = _processing_rp_list.begin();
	 rp_iter1 != _processing_rp_list.end(); ++rp_iter1) {
	PimRp *pim_rp = *rp_iter1;
	// XXX: note that here we don't consider the pim_rp->is_updated() flag
	// because all PimRp entries on the processing_rp_list need
	// consideration.
	pim_node().pim_mrt().add_task_rp_changed(pim_rp->rp_addr());
	ret_value = true;
    }
    
    return (ret_value);
}

// Used by (*,G), (S,G), (S,G,rpt)
void
RpTable::add_pim_mre(PimMre *pim_mre)
{
    PimRp *new_pim_rp = pim_mre->pim_rp();
    
    if (! (pim_mre->is_wc() || pim_mre->is_sg() || pim_mre->is_sg_rpt()))
	return;
    
    if (pim_mre->is_sg() || pim_mre->is_sg_rpt()) {
	if (pim_mre->wc_entry() != NULL)
	    return;	// XXX: we don't add (S,G) or (S,G,rpt) that have (*,G)
    }
    
    if (new_pim_rp == NULL) {
	//
	// Find the special PimRp entry that contains all (*,G) or (S,G)
	// or (S,G,rpt) or PimMfc entries that have no RP yet. If not found,
	// create it.
	// XXX: that special PimRp entry has RP address of IPvX::ZERO()
	// XXX: there could be one special PimRp entry for (*,G), (S,G),
	// (S,G,rpt) and PimMfc entries.
	//
	new_pim_rp = find_processing_rp_by_addr(IPvX::ZERO(family()));
	
	if (new_pim_rp == NULL) {
	    new_pim_rp = new PimRp(*this,
				   IPvX::ZERO(family()),
				   0,
				   IPvXNet(IPvX::ZERO(family()), 0),
				   0,
				   PimRp::RP_LEARNED_METHOD_UNKNOWN);
	    _processing_rp_list.push_back(new_pim_rp);
	}
    }
    
    XLOG_ASSERT(new_pim_rp != NULL);
    
    // Add the PimMre entry to the appropriate list
    do {
	if (pim_mre->is_wc()) {
	    new_pim_rp->pim_mre_wc_list().push_back(pim_mre);
	    break;
	}
	if (pim_mre->is_sg()) {
	    new_pim_rp->pim_mre_sg_list().push_back(pim_mre);
	    break;
	}
	if (pim_mre->is_sg_rpt()) {
	    new_pim_rp->pim_mre_sg_rpt_list().push_back(pim_mre);
	    break;
	}
    } while (false);
}

// Used by PimMfc entries
void
RpTable::add_pim_mfc(PimMfc *pim_mfc)
{
    PimRp *new_pim_rp = rp_find(pim_mfc->group_addr());
    
    if (new_pim_rp == NULL) {
	//
	// Find the special PimRp entry that contains all (*,G) or (S,G)
	// or (S,G,rpt) or PimMfc entries that have no RP yet. If not found,
	// create it.
	// XXX: that special PimRp entry has RP address of IPvX::ZERO()
	// XXX: there could be one special PimRp entry for (*,G), (S,G),
	// (S,G,rpt) and PimMfc entries.
	//
	new_pim_rp = find_processing_rp_by_addr(IPvX::ZERO(family()));
	
	if (new_pim_rp == NULL) {
	    new_pim_rp = new PimRp(*this,
				   IPvX::ZERO(family()),
				   0,
				   IPvXNet(IPvX::ZERO(family()), 0),
				   0,
				   PimRp::RP_LEARNED_METHOD_UNKNOWN);
	    _processing_rp_list.push_back(new_pim_rp);
	}
    }
    
    XLOG_ASSERT(new_pim_rp != NULL);
    
    // Add the PimMre entry to the appropriate list
    do {
	new_pim_rp->pim_mfc_list().push_back(pim_mfc);
	break;
    } while (false);
}

//
// XXX: this method SHOULD be called ONLY by PimMre::set_pim_rp().
// If it is called by another method, then that method MUST
// take care of PimMre::set_pim_rp() to set appropriately the "PimRp *_pim_rp"
// entry.
// Used by (*,G), (S,G), (S,G,rpt)
void
RpTable::delete_pim_mre(PimMre *pim_mre)
{
    PimRp *old_pim_rp = pim_mre->pim_rp();
    
    if (! (pim_mre->is_wc() || pim_mre->is_sg() || pim_mre->is_sg_rpt()))
	return;
    
    //
    // Remove pim_mre from the list of its RP (or from the no-RP list)
    //
    list<PimMre *>::iterator pim_mre_iter;
    if (old_pim_rp == NULL) {
	// XXX: find the special PimRp entry that contains
	// the (*,G) or (S,G) or (S,G,rpt) entries that have no RP.
	old_pim_rp = find_processing_rp_by_addr(IPvX::ZERO(family()));
    }
    
    if (old_pim_rp != NULL) {
	do {
	    if (pim_mre->is_wc()) {
		//
		// (*,G) entry
		//
		
		//
		// Try the pim_mre_wc_list()
		//
		pim_mre_iter = find(old_pim_rp->pim_mre_wc_list().begin(),
				    old_pim_rp->pim_mre_wc_list().end(),
				    pim_mre);
		if (pim_mre_iter != old_pim_rp->pim_mre_wc_list().end()) {
		    old_pim_rp->pim_mre_wc_list().erase(pim_mre_iter);
		    break;
		}
		//
		// Try the processing_pim_mre_wc_list()
		//
		pim_mre_iter = find(
		    old_pim_rp->processing_pim_mre_wc_list().begin(),
		    old_pim_rp->processing_pim_mre_wc_list().end(),
		    pim_mre);
		if (pim_mre_iter
		    != old_pim_rp->processing_pim_mre_wc_list().end()) {
		    old_pim_rp->processing_pim_mre_wc_list().erase(pim_mre_iter);
		    break;
		}
		break;
	    }
	    if (pim_mre->is_sg()) {
		//
		// (S,G) entry
		//
		
		//
		// Try the pim_mre_sg_list()
		//
		pim_mre_iter = find(old_pim_rp->pim_mre_sg_list().begin(),
				    old_pim_rp->pim_mre_sg_list().end(),
				    pim_mre);
		if (pim_mre_iter != old_pim_rp->pim_mre_sg_list().end()) {
		    old_pim_rp->pim_mre_sg_list().erase(pim_mre_iter);
		    break;
		}
		//
		// Try the processing_pim_mre_sg_list()
		//
		pim_mre_iter = find(
		    old_pim_rp->processing_pim_mre_sg_list().begin(),
		    old_pim_rp->processing_pim_mre_sg_list().end(),
		    pim_mre);
		if (pim_mre_iter
		    != old_pim_rp->processing_pim_mre_sg_list().end()) {
		    old_pim_rp->processing_pim_mre_sg_list().erase(pim_mre_iter);
		    break;
		}
		break;
	    }
	    if (pim_mre->is_sg_rpt()) {
		//
		// (S,G,rpt) entry
		//
		
		//
		// Try the pim_mre_sg_rpt_list()
		//
		pim_mre_iter = find(old_pim_rp->pim_mre_sg_rpt_list().begin(),
				    old_pim_rp->pim_mre_sg_rpt_list().end(),
				    pim_mre);
		if (pim_mre_iter != old_pim_rp->pim_mre_sg_rpt_list().end()) {
		    old_pim_rp->pim_mre_sg_rpt_list().erase(pim_mre_iter);
		    break;
		}
		//
		// Try the processing_pim_mre_sg_rpt_list()
		//
		pim_mre_iter = find(
		    old_pim_rp->processing_pim_mre_sg_rpt_list().begin(),
		    old_pim_rp->processing_pim_mre_sg_rpt_list().end(),
		    pim_mre);
		if (pim_mre_iter
		    != old_pim_rp->processing_pim_mre_sg_rpt_list().end()) {
		    old_pim_rp->processing_pim_mre_sg_rpt_list().erase(pim_mre_iter);
		    break;
		}
		break;
	    }
	} while (false);
    }

    // XXX: we should not call pim_mre->set_pim_rp(NULL) here,
    // because it will result in a calling loop.
    // pim_mre->set_pim_rp(NULL);
    
    // If the PimRp entry was pending deletion, and if this was the
    // lastest PimMre entry, delete the PimRp entry.
    do {
	if (old_pim_rp == NULL)
	    break;
	if ( ! (old_pim_rp->pim_mre_wc_list().empty()
		&& old_pim_rp->pim_mre_sg_list().empty()
		&& old_pim_rp->pim_mre_sg_rpt_list().empty()
		&& old_pim_rp->pim_mfc_list().empty()
		&& old_pim_rp->processing_pim_mre_wc_list().empty()
		&& old_pim_rp->processing_pim_mre_sg_list().empty()
		&& old_pim_rp->processing_pim_mre_sg_rpt_list().empty()
		&& old_pim_rp->processing_pim_mfc_list().empty())) {
	    break;	// There are still more entries
	}
	
	list<PimRp *>::iterator pim_rp_iter;
	pim_rp_iter = find(_processing_rp_list.begin(),
			   _processing_rp_list.end(),
			   old_pim_rp);
	if (pim_rp_iter != _processing_rp_list.end()) {
	    _processing_rp_list.erase(pim_rp_iter);
	    delete old_pim_rp;
	    old_pim_rp = NULL;
	}
    } while (false);
}

// Used by PimMfc
void
RpTable::delete_pim_mfc(PimMfc *pim_mfc)
{
    const IPvX& rp_addr = pim_mfc->rp_addr();
    list<PimRp *>::iterator pim_rp_iter;
    list<PimMfc *>::iterator pim_mfc_iter;
    PimRp *old_pim_rp = NULL;
    
    //
    // Remove pim_mfc from the list of its RP (or from the no-RP list)
    // XXX: because we don't know which PimRp entry has the pim_mfc
    // on its list, we try all PimRp entries that match the RP address.
    //
    do {
	//
	// Try to remove from the rp_list()
	//
	for (pim_rp_iter = _rp_list.begin();
	     pim_rp_iter != _rp_list.end();
	     ++pim_rp_iter) {
	    PimRp *pim_rp = *pim_rp_iter;
	    
	    if (pim_rp->rp_addr() != rp_addr)
		continue;
	    
	    // Try the pim_mfc_list()
	    pim_mfc_iter = find(pim_rp->pim_mfc_list().begin(),
				pim_rp->pim_mfc_list().end(),
				pim_mfc);
	    if (pim_mfc_iter != pim_rp->pim_mfc_list().end()) {
		pim_rp->pim_mfc_list().erase(pim_mfc_iter);
		old_pim_rp = pim_rp;
		break;
	    }
	    // Try the processing_pim_mfc_list()
	    pim_mfc_iter = find(pim_rp->processing_pim_mfc_list().begin(),
				pim_rp->processing_pim_mfc_list().end(),
				pim_mfc);
	    if (pim_mfc_iter != pim_rp->processing_pim_mfc_list().end()) {
		pim_rp->processing_pim_mfc_list().erase(pim_mfc_iter);
		old_pim_rp = pim_rp;
		break;
	    }
	}
	if (old_pim_rp != NULL)
	    break;
	
	//
	// Try to remove from the processing_rp_list()
	//
	for (pim_rp_iter = _processing_rp_list.begin();
	     pim_rp_iter != _processing_rp_list.end();
	     ++pim_rp_iter) {
	    PimRp *pim_rp = *pim_rp_iter;
	    
	    if (pim_rp->rp_addr() != rp_addr)
		continue;
	    
	    pim_mfc_iter = find(pim_rp->pim_mfc_list().begin(),
				pim_rp->pim_mfc_list().end(),
				pim_mfc);
	    if (pim_mfc_iter != pim_rp->pim_mfc_list().end()) {
		pim_rp->pim_mfc_list().erase(pim_mfc_iter);
		old_pim_rp = pim_rp;
		break;
	    }
	    // Try the processing_pim_mfc_list()
	    pim_mfc_iter = find(pim_rp->processing_pim_mfc_list().begin(),
				pim_rp->processing_pim_mfc_list().end(),
				pim_mfc);
	    if (pim_mfc_iter != pim_rp->processing_pim_mfc_list().end()) {
		pim_rp->processing_pim_mfc_list().erase(pim_mfc_iter);
		old_pim_rp = pim_rp;
		break;
	    }
	}
	if (old_pim_rp != NULL)
	    break;
    } while (false);
    
    // If the PimRp entry was pending deletion, and if this was the
    // lastest PimMfc entry, delete the PimRp entry.
    do {
	if (old_pim_rp == NULL)
	    break;
	if ( ! (old_pim_rp->pim_mre_wc_list().empty()
		&& old_pim_rp->pim_mre_sg_list().empty()
		&& old_pim_rp->pim_mre_sg_rpt_list().empty()
		&& old_pim_rp->pim_mfc_list().empty()
		&& old_pim_rp->processing_pim_mre_wc_list().empty()
		&& old_pim_rp->processing_pim_mre_sg_list().empty()
		&& old_pim_rp->processing_pim_mre_sg_rpt_list().empty()
		&& old_pim_rp->processing_pim_mfc_list().empty())) {
	    break;	// There are still more entries
	}
	
	list<PimRp *>::iterator pim_rp_iter;
	pim_rp_iter = find(_processing_rp_list.begin(),
			   _processing_rp_list.end(),
			   old_pim_rp);
	if (pim_rp_iter != _processing_rp_list.end()) {
	    _processing_rp_list.erase(pim_rp_iter);
	    delete old_pim_rp;
	    old_pim_rp = NULL;
	}
    } while (false);
}

//
// Prepare all PimRp entries with RP address of @rp_add to
// process their (*,G) PimMre entries.
//
void
RpTable::init_processing_pim_mre_wc(const IPvX& rp_addr)
{
    list<PimRp *>::iterator iter;
    
    for (iter = _rp_list.begin(); iter != _rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	if (pim_rp->rp_addr() == rp_addr)
	    pim_rp->init_processing_pim_mre_wc();
    }
    for (iter = _processing_rp_list.begin();
	 iter != _processing_rp_list.end();
	 ++iter) {
	PimRp *pim_rp = *iter;
	if (pim_rp->rp_addr() == rp_addr)
	    pim_rp->init_processing_pim_mre_wc();
    }
}

//
// Prepare all PimRp entries with RP address of @rp_add to
// process their (S,G) PimMre entries.
//
void
RpTable::init_processing_pim_mre_sg(const IPvX& rp_addr)
{
    list<PimRp *>::iterator iter;
    
    for (iter = _rp_list.begin(); iter != _rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	if (pim_rp->rp_addr() == rp_addr)
	    pim_rp->init_processing_pim_mre_sg();
    }
    for (iter = _processing_rp_list.begin();
	 iter != _processing_rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	if (pim_rp->rp_addr() == rp_addr)
	    pim_rp->init_processing_pim_mre_sg();
    }
}

//
// Prepare all PimRp entries with RP address of @rp_add to
// process their (S,G,rpt) PimMre entries.
//
void
RpTable::init_processing_pim_mre_sg_rpt(const IPvX& rp_addr)
{
    list<PimRp *>::iterator iter;
    
    for (iter = _rp_list.begin(); iter != _rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	if (pim_rp->rp_addr() == rp_addr)
	    pim_rp->init_processing_pim_mre_sg_rpt();
    }
    for (iter = _processing_rp_list.begin();
	 iter != _processing_rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	if (pim_rp->rp_addr() == rp_addr)
	    pim_rp->init_processing_pim_mre_sg_rpt();
    }
}

//
// Prepare all PimRp entries with RP address of @rp_add to
// process their PimMfc entries.
//
void
RpTable::init_processing_pim_mfc(const IPvX& rp_addr)
{
    list<PimRp *>::iterator iter;
    
    for (iter = _rp_list.begin(); iter != _rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	if (pim_rp->rp_addr() == rp_addr)
	    pim_rp->init_processing_pim_mfc();
    }
    for (iter = _processing_rp_list.begin();
	 iter != _processing_rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	if (pim_rp->rp_addr() == rp_addr)
	    pim_rp->init_processing_pim_mfc();
    }
}

//
// Return the first PimRp entry with RP address of @rp_addr
// that has pending (*,G) entries to be processed.
//
PimRp *
RpTable::find_processing_pim_mre_wc(const IPvX& rp_addr)
{
    list<PimRp *>::iterator iter;
    
    for (iter = _rp_list.begin(); iter != _rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	if ((pim_rp->rp_addr() == rp_addr)
	    && (! pim_rp->processing_pim_mre_wc_list().empty()))
	    return (pim_rp);
    }
    for (iter = _processing_rp_list.begin();
	 iter != _processing_rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	if ((pim_rp->rp_addr() == rp_addr)
	    && (! pim_rp->processing_pim_mre_wc_list().empty()))
	    return (pim_rp);
    }
    
    return (NULL);
}

//
// Return the first PimRp entry with RP address of @rp_addr
// that has pending (S,G) entries to be processed.
//
PimRp *
RpTable::find_processing_pim_mre_sg(const IPvX& rp_addr)
{
    list<PimRp *>::iterator iter;
    
    for (iter = _rp_list.begin(); iter != _rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	if ((pim_rp->rp_addr() == rp_addr)
	    && (! pim_rp->processing_pim_mre_sg_list().empty()))
	    return (pim_rp);
    }
    for (iter = _processing_rp_list.begin();
	 iter != _processing_rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	if ((pim_rp->rp_addr() == rp_addr)
	    && (! pim_rp->processing_pim_mre_sg_list().empty()))
	    return (pim_rp);
    }
    
    return (NULL);
}

//
// Return the first PimRp entry with RP address of @rp_addr
// that has pending (S,G,rpt) entries to be processed.
//
PimRp *
RpTable::find_processing_pim_mre_sg_rpt(const IPvX& rp_addr)
{
    list<PimRp *>::iterator iter;
    
    for (iter = _rp_list.begin(); iter != _rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	if ((pim_rp->rp_addr() == rp_addr)
	    && (! pim_rp->processing_pim_mre_sg_rpt_list().empty()))
	    return (pim_rp);
    }
    for (iter = _processing_rp_list.begin();
	 iter != _processing_rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	if ((pim_rp->rp_addr() == rp_addr)
	    && (! pim_rp->processing_pim_mre_sg_rpt_list().empty()))
	    return (pim_rp);
    }
    
    return (NULL);
}

//
// Return the first PimRp entry with RP address of @rp_addr
// that has pending PimMfc entries to be processed.
//
PimRp *
RpTable::find_processing_pim_mfc(const IPvX& rp_addr)
{
    list<PimRp *>::iterator iter;
    
    for (iter = _rp_list.begin(); iter != _rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	if ((pim_rp->rp_addr() == rp_addr)
	    && (! pim_rp->processing_pim_mfc_list().empty()))
	    return (pim_rp);
    }
    for (iter = _processing_rp_list.begin();
	 iter != _processing_rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	if ((pim_rp->rp_addr() == rp_addr)
	    && (! pim_rp->processing_pim_mfc_list().empty()))
	    return (pim_rp);
    }
    
    return (NULL);
}

//
// Return the first PimRp entry with RP address of @rp_addr
// that is on the processing_rp_list 
//
PimRp *
RpTable::find_processing_rp_by_addr(const IPvX& rp_addr)
{
    list<PimRp *>::iterator iter;
    
    for (iter = _processing_rp_list.begin();
	 iter != _processing_rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	if (pim_rp->rp_addr() == rp_addr)
	    return (pim_rp);
    }
    
    return (NULL);
}

//
// Return true if address @rp_addr is still contained in the RpTable,
// otherwise return false.
//
bool
RpTable::has_rp_addr(const IPvX& rp_addr)
{
    list<PimRp *>::iterator iter;
    
    for (iter = _rp_list.begin(); iter != _rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	if (pim_rp->rp_addr() == rp_addr)
	    return (true);
    }
    for (iter = _processing_rp_list.begin();
	 iter != _processing_rp_list.end(); ++iter) {
	PimRp *pim_rp = *iter;
	if (pim_rp->rp_addr() == rp_addr)
	    return (true);
    }
    
    return (false);
}

PimRp::PimRp(RpTable& rp_table, const IPvX& rp_addr, uint8_t rp_priority,
	     const IPvXNet& group_prefix, uint8_t hash_masklen,
	     rp_learned_method_t rp_learned_method)
    : _rp_table(rp_table),
      _rp_addr(rp_addr),
      _rp_priority(rp_priority),
      _group_prefix(group_prefix),
      _hash_masklen(hash_masklen),
      _rp_learned_method(rp_learned_method),
      _is_updated(true),
      _i_am_rp(_rp_table.pim_node().is_my_addr(rp_addr))
{
    
}

PimRp::PimRp(RpTable& rp_table, const PimRp& pim_rp)
    : _rp_table(rp_table),
      _rp_addr(pim_rp.rp_addr()),
      _rp_priority(pim_rp.rp_priority()),
      _group_prefix(pim_rp.group_prefix()),
      _hash_masklen(pim_rp.hash_masklen()),
      _rp_learned_method(pim_rp.rp_learned_method()),
      _is_updated(pim_rp.is_updated()),
      _i_am_rp(pim_rp.i_am_rp())
{
    
}

PimRp::~PimRp()
{
    // Try to remove the (*,*,RP) entry if no such RP anymore,
    // and the entry is not needed.
    if (! rp_table().has_rp_addr(rp_addr())) {
	PimMre *pim_mre;
	pim_mre = rp_table().pim_node().pim_mrt().pim_mre_find(
	    rp_addr(),
	    IPvX::ZERO(rp_table().family()),
	    PIM_MRE_RP,
	    0);
	if (pim_mre != NULL)
	    pim_mre->entry_try_remove();
    }
}

const string
PimRp::rp_learned_method_str(rp_learned_method_t rp_learned_method)
{
    switch (rp_learned_method) {
    case RP_LEARNED_METHOD_AUTORP:
	return ("AUTORP");
    case RP_LEARNED_METHOD_BOOTSTRAP:
	return ("BOOTSTRAP");
    case RP_LEARNED_METHOD_STATIC:
	return ("STATIC");
    default:
	// Unknown learned method.
	return ("UNKNOWN");
    }
    
    return ("UNKNOWN");
}

const string
PimRp::rp_learned_method_str() const
{
    return (rp_learned_method_str(rp_learned_method()));
}

void
PimRp::init_processing_pim_mre_wc()
{
    _processing_pim_mre_wc_list.splice(
	_processing_pim_mre_wc_list.end(),
	_pim_mre_wc_list);
}

void
PimRp::init_processing_pim_mre_sg()
{
    _processing_pim_mre_sg_list.splice(
	_processing_pim_mre_sg_list.end(),
	_pim_mre_sg_list);
}

void
PimRp::init_processing_pim_mre_sg_rpt()
{
    _processing_pim_mre_sg_rpt_list.splice(
	_processing_pim_mre_sg_rpt_list.end(),
	_pim_mre_sg_rpt_list);
}

void
PimRp::init_processing_pim_mfc()
{
    _processing_pim_mfc_list.splice(
	_processing_pim_mfc_list.end(),
	_pim_mfc_list);
}
