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

#ident "$XORP: xorp/pim/pim_mrt.cc,v 1.57 2002/12/09 18:29:27 hodson Exp $"

//
// PIM Multicast Routing Table implementation.
//


#include "pim_module.h"
#include "pim_private.hh"
#include "libxorp/utils.hh"
#include "pim_mfc.hh"
#include "pim_mre.hh"
#include "pim_mrt.hh"
#include "pim_node.hh"
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


PimMrt::PimMrt(PimNode& pim_node)
    : _pim_node(pim_node),
      _pim_mrt_sg(*this),
      _pim_mrt_sg_rpt(*this),
      _pim_mrt_g(*this),
      _pim_mrt_rp(*this),
      _pim_mrt_mfc(*this),
      _pim_mre_track_state(*this)
{
    
}

PimMrt::~PimMrt()
{
    
}

PimMrtSg::PimMrtSg(PimMrt& pim_mrt)
    : _pim_mrt(pim_mrt)
{
    
}

PimMrtSg::~PimMrtSg()
{
    
}

PimMrtG::PimMrtG(PimMrt& pim_mrt)
    : _pim_mrt(pim_mrt)
{
    
}

PimMrtG::~PimMrtG()
{
    
}

PimMrtRp::PimMrtRp(PimMrt& pim_mrt)
    : _pim_mrt(pim_mrt)
{
    
}

PimMrtRp::~PimMrtRp()
{
    
}

PimMrtMfc::PimMrtMfc(PimMrt& pim_mrt)
    : _pim_mrt(pim_mrt)
{
    
}

PimMrtMfc::~PimMrtMfc()
{
    
}

int
PimMrt::family() const
{
    return (pim_node().family());
}

PimMribTable&
PimMrt::pim_mrib_table()
{
    return (pim_node().pim_mrib_table());
}

Mifset&
PimMrt::i_am_dr()
{
    return pim_node().pim_vifs_dr();
}

PimVif *
PimMrt::vif_find_by_vif_index(uint16_t vif_index)
{
    return (pim_node().vif_find_by_vif_index(vif_index));
}

PimVif *
PimMrt::vif_find_pim_register()
{
    return (pim_node().vif_find_pim_register());
}

uint16_t
PimMrt::pim_register_vif_index() const
{
    return (pim_node().pim_register_vif_index());
}

//
// XXX: @create_flags must be a subset of @lookup_flags
// XXX: if no creation allowed, the entry that it returns may be the
// next one in the map.
//
// XXX: if the entry to lookup and/or create is (*,*,RP), then:
//  - If group is IPvX::ZERO(family()), then we lookup/create the
//    entry by using 'source' which is the RP address
//  - If group is NOT IPvX::ZERO(family()), then we lookup/create the
//    entry by lookup first which is the RP for 'group'.
//  - Regardless of the group address, the created (*,*,RP) entry
//    always has a group address of IPvX::MULTICAST_BASE(family())
PimMre *
PimMrt::pim_mre_find(const IPvX& source, const IPvX& group,
		     uint32_t lookup_flags, uint32_t create_flags)
{
    PimMre *pim_mre = NULL;
    
    create_flags &= lookup_flags;
    
    //
    // Try to lookup if entry was installed already.
    // XXX: the order is important, because we want the longest-match.
    //
    do {
	if (lookup_flags & PIM_MRE_SG) {
	    //
	    // (S,G) entry
	    //
	    pim_mre = _pim_mrt_sg.find(source, group);
	    if (pim_mre != NULL)
		break;
	}
	if (lookup_flags & PIM_MRE_SG_RPT) {
	    //
	    // (S,G,rpt) entry
	    //
	    pim_mre = _pim_mrt_sg_rpt.find(source, group);
	    if (pim_mre != NULL)
		break;
	}
	if (lookup_flags & PIM_MRE_WC) {
	    //
	    // (*,G) entry
	    //
	    pim_mre = _pim_mrt_g.find(IPvX::ZERO(family()), group);
	    if (pim_mre != NULL)
		break;
	}
	if (lookup_flags & PIM_MRE_RP) {
	    //
	    // (*,*,RP) entry
	    //
	    if (group == IPvX::ZERO(family())) {
		// XXX: the entry is specified by the RP address ('source')
		pim_mre = _pim_mrt_rp.find(source,
					   IPvX::MULTICAST_BASE(family()));
		if (pim_mre != NULL)
		    break;
	    } else {
		// XXX: the entry is specified by the group address
		PimRp *pim_rp = pim_node().rp_table().rp_find(group);
		if (pim_rp != NULL)
		    pim_mre = _pim_mrt_rp.find(pim_rp->rp_addr(),
					       IPvX::MULTICAST_BASE(family()));
	    }
	    if (pim_mre != NULL)
		break;
	}
    } while (false);
    
    if (pim_mre != NULL)
	return (pim_mre);
    
    //
    // Lookup failed. Create the entry if creation is allowed.
    //
    // If creation allowed, create the entry, insert it and return it.
    // XXX: the order is important, because we want the longest-match.
    do {
	if (create_flags & (PIM_MRE_SG)) {
	    //
	    // (S,G) entry
	    //
	    
	    // Create and insert the entry
	    pim_mre = new PimMre(*this, source, group);
	    pim_mre->set_sg(true);
	    pim_mre = _pim_mrt_sg.insert(pim_mre);
	    
	    // Set source-related state
	    if (pim_node().is_directly_connected(source))
		pim_mre->set_directly_connected_s(true);
	    else
		pim_mre->set_directly_connected_s(false);
	    
	    // Set the pointer to the corresponding (*,G) entry (if exists);
	    pim_mre->set_wc_entry(pim_mre_find(source, group, PIM_MRE_WC, 0));

	    // Set the pointer to the corresponding (S,G,rpt) entry
	    // (if exists), and vice-versa
	    PimMre *pim_mre_sg_rpt = pim_mre_find(source, group,
						  PIM_MRE_SG_RPT, 0);
	    pim_mre->set_sg_rpt_entry(pim_mre_sg_rpt);
	    if (pim_mre_sg_rpt != NULL)
		pim_mre_sg_rpt->set_sg_entry(pim_mre);
	    
	    // Compute and set the RP-related state
	    if (pim_mre->wc_entry() != NULL)
		pim_mre->uncond_set_pim_rp(pim_mre->wc_entry()->pim_rp());
	    else
		pim_mre->uncond_set_pim_rp(pim_mre->compute_rp());
	    
	    // Compute and set the MRIB and RPF-related state
	    pim_mre->set_mrib_rp(pim_mre->compute_mrib_rp());
	    pim_mre->set_mrib_s(pim_mre->compute_mrib_s());
	    pim_mre->set_mrib_next_hop_s(pim_mre->compute_mrib_next_hop_s());
	    pim_mre->set_rpfp_nbr_sg(pim_mre->compute_rpfp_nbr_sg());
	    if ((pim_mre->mrib_next_hop_s() == NULL)
		|| (pim_mre->rpfp_nbr_sg() == NULL)) {
		pim_node().add_pim_mre_no_pim_nbr(pim_mre);
	    }
	    
	    // Add a task to handle any CPU-intensive operations
	    // XXX: not needed for this entry.
	    // add_task_add_pim_mre(pim_mre);
	    
	    break;
	}
	
	if (create_flags & PIM_MRE_SG_RPT) {
	    //
	    // (S,G,rpt) entry
	    //
	    
	    // Create and insert the entry
	    pim_mre = new PimMre(*this, source, group);
	    pim_mre->set_sg_rpt(true);
	    pim_mre = _pim_mrt_sg_rpt.insert(pim_mre);
	    
	    // Set source-related state
	    if (pim_node().is_directly_connected(source))
		pim_mre->set_directly_connected_s(true);
	    else
		pim_mre->set_directly_connected_s(false);
	    
	    // Set the pointer to the corresponding (*,G) entry (if exists);
	    pim_mre->set_wc_entry(pim_mre_find(source, group, PIM_MRE_WC, 0));

	    // Set the pointer to the corresponding (S,G) entry
	    // (if exists), and vice-versa
	    PimMre *pim_mre_sg = pim_mre_find(source, group,
					      PIM_MRE_SG, 0);
	    pim_mre->set_sg_entry(pim_mre_sg);
	    if (pim_mre_sg != NULL)
		pim_mre_sg->set_sg_rpt_entry(pim_mre);
	    
	    // Compute and set the RP-related state
	    if (pim_mre->wc_entry() != NULL)
		pim_mre->uncond_set_pim_rp(pim_mre->wc_entry()->pim_rp());
	    else
		pim_mre->uncond_set_pim_rp(pim_mre->compute_rp());
	    
	    // Compute and set the MRIB and RPF-related state
	    pim_mre->set_mrib_rp(pim_mre->compute_mrib_rp());
	    pim_mre->set_mrib_s(pim_mre->compute_mrib_s());
	    pim_mre->set_rpfp_nbr_sg_rpt(pim_mre->compute_rpfp_nbr_sg_rpt());
	    if (pim_mre->rpfp_nbr_sg_rpt() == NULL) {
		pim_node().add_pim_mre_no_pim_nbr(pim_mre);
	    }
	    
	    // Add a task to handle any CPU-intensive operations
	    // XXX: not needed for this entry.
	    // add_task_add_pim_mre(pim_mre);
	    
	    break;
	}
	
	if (create_flags & PIM_MRE_WC) {
	    //
	    // (*,G) entry
	    //
	    
	    // Create and insert the entry
	    pim_mre = new PimMre(*this, IPvX::ZERO(family()), group);
	    pim_mre->set_wc(true);
	    pim_mre = _pim_mrt_g.insert(pim_mre);
	    
	    // Compute and set the RP-related state
	    pim_mre->uncond_set_pim_rp(pim_mre->compute_rp());
	    
	    // Compute and set the MRIB and RPF-related state
	    pim_mre->set_mrib_rp(pim_mre->compute_mrib_rp());
	    pim_mre->set_mrib_next_hop_rp(pim_mre->compute_mrib_next_hop_rp());
	    pim_mre->set_rpfp_nbr_wc(pim_mre->compute_rpfp_nbr_wc());
	    if ((pim_mre->mrib_next_hop_rp() == NULL)
		|| (pim_mre->rpfp_nbr_wc() == NULL)) {
		pim_node().add_pim_mre_no_pim_nbr(pim_mre);
	    }
	    
	    // Add a task to handle any CPU-intensive operations
	    // This task will assign the wc_entry() pointer for
	    // all (S,G) and (S,G,rpt) entries.
	    add_task_add_pim_mre(pim_mre);
	    
	    break;
	}
	
	if (create_flags & PIM_MRE_RP) {
	    //
	    // (*,*,RP) entry
	    //
	    
	    // Create and insert the entry
	    if (group == IPvX::ZERO(family())) {
		// XXX: the entry is specified by the RP address
		pim_mre = new PimMre(*this, source,
				     IPvX::MULTICAST_BASE(family()));
	    } else {
		// XXX: the entry is specified by the group address
		PimRp *pim_rp = pim_node().rp_table().rp_find(group);
		if (pim_rp != NULL)
		    pim_mre = new PimMre(*this, pim_rp->rp_addr(),
					 IPvX::MULTICAST_BASE(family()));
	    }
	    if (pim_mre == NULL)
		break;
	    pim_mre->set_rp(true);
	    pim_mre = _pim_mrt_rp.insert(pim_mre);
	    
	    // Compute and set the RP-related state
	    if (pim_node().is_my_addr(*pim_mre->rp_addr_ptr()))
		pim_mre->set_i_am_rp(true);
	    else
		pim_mre->set_i_am_rp(false);
	    
	    // Compute and set the MRIB and RPF-related state
	    pim_mre->set_mrib_rp(pim_mre->compute_mrib_rp());
	    pim_mre->set_mrib_next_hop_rp(pim_mre->compute_mrib_next_hop_rp());
	    if (pim_mre->mrib_next_hop_rp() == NULL) {
		pim_node().add_pim_mre_no_pim_nbr(pim_mre);
	    }
	    
	    // Add a task to handle any CPU-intensive operations
	    // XXX: not needed for this entry.
	    // add_task_add_pim_mre(pim_mre);
	    
	    // XXX: the rp_entry() pointer for all related (*,G) entries
	    // will be setup by PimMre::set_pim_rp(), which itself
	    // will be called indirectly when a task updating the related (*,G)
	    // entries has been scheduled. This task will be scheduled
	    // whenever the RP-Set has been changed. Note that this will
	    // work because (*,*,RP) entries are always created whenever
	    // a new RP is added to the RP-Set. Thus, all (*,G) entries
	    // for that RP are "assigned" to that RP _after_ the (*,*,RP)
	    // entry is created.
	    
	    break;
	}
    } while (false);
    
    return (pim_mre);
}

// XXX: if @is_create_bool is true, the entry will be created if it did
// not exist before.
PimMfc *
PimMrt::pim_mfc_find(const IPvX& source, const IPvX& group,
		     bool is_create_bool)
{
    PimMfc *pim_mfc = NULL;
    
    //
    // Try to lookup if entry was installed already.
    //
    pim_mfc = _pim_mrt_mfc.find(source, group);
    if (pim_mfc != NULL)
	return (pim_mfc);
    
    //
    // Lookup failed. Create the entry if creation is allowed.
    //
    if (is_create_bool) {
	// Create and insert the entry
	pim_mfc = new PimMfc(*this, source, group);
	pim_mfc = _pim_mrt_mfc.insert(pim_mfc);
	
	// Compute and set the RP-related state
	PimRp *pim_rp = pim_node().rp_table().rp_find(group);
	if (pim_rp != NULL)
	    pim_mfc->uncond_set_rp_addr(pim_rp->rp_addr());
	else
	    pim_mfc->uncond_set_rp_addr(IPvX::ZERO(family()));
    }
    
    return (pim_mfc);
}

int
PimMrt::remove_pim_mre(PimMre *pim_mre)
{
    int ret_value = XORP_ERROR;
    
    if (pim_mre->is_sg()) {
	ret_value = _pim_mrt_sg.remove(pim_mre);
	return (ret_value);
    }
    if (pim_mre->is_sg_rpt()) {
	ret_value = _pim_mrt_sg_rpt.remove(pim_mre);
	return (ret_value);
    }
    if (pim_mre->is_wc()) {
	ret_value = _pim_mrt_g.remove(pim_mre);
	return (ret_value);
    }
    if (pim_mre->is_rp()) {
	ret_value = _pim_mrt_rp.remove(pim_mre);
	return (ret_value);
    }
    
    return (ret_value);
}

int
PimMrt::remove_pim_mfc(PimMfc *pim_mfc)
{
    int ret_value = _pim_mrt_mfc.remove(pim_mfc);
    
    return (ret_value);
}
