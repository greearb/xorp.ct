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

#ident "$XORP: xorp/pim/pim_mrt_task.cc,v 1.29 2002/12/09 18:29:27 hodson Exp $"

//
// PIM Multicast Routing Table task-related implementation.
//


#include "pim_module.h"
#include "pim_private.hh"
#include "pim_mre_task.hh"
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


void
PimMrt::add_task(PimMreTask *pim_mre_task)
{
    PimVif *pim_vif;
    
    _pim_mre_task_list.push_back(pim_mre_task);
    
    //
    // Record if a PimVif is in use by this task
    //
    pim_vif = pim_node().vif_find_by_vif_index(pim_mre_task->vif_index());
    if (pim_vif == NULL)
	return;
    
    pim_vif->incr_usage_by_pim_mre_task();
}

void
PimMrt::delete_task(PimMreTask *pim_mre_task)
{
    PimVif *pim_vif;
    list<PimMreTask *>::iterator iter;
    list<PimMreTask *>& task_list = pim_mre_task_list();
    
    //
    // Remove from the list of tasks
    //
    iter = find(task_list.begin(), task_list.end(), pim_mre_task);
    if (iter == task_list.end())
	return;		// Not found
    
    task_list.erase(iter);
    
    //
    // Record if a PimVif is not in use anymore by this task
    //
    pim_vif = pim_node().vif_find_by_vif_index(pim_mre_task->vif_index());
    if (pim_vif == NULL)
	return;
    
    pim_vif->decr_usage_by_pim_mre_task();
}

void
PimMrt::schedule_task(PimMreTask *pim_mre_task)
{
    pim_mre_task->schedule_task();
}

void
PimMrt::add_task_rp_changed(const IPvX& affected_rp_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the RP-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_RP_CHANGED);
	pim_mre_task->set_rp_addr_rp(affected_rp_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_mrib_changed(const IPvXNet& modified_prefix_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the RP-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_MRIB_RP_CHANGED);
	pim_mre_task->set_rp_addr_prefix_rp(modified_prefix_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the S-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_MRIB_S_CHANGED);
	pim_mre_task->set_source_addr_prefix_sg_sg_rpt(modified_prefix_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

// TODO: not used (not needed?)
void
PimMrt::add_task_mrib_next_hop_changed(const IPvXNet& modified_prefix_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the RP-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_MRIB_NEXT_HOP_RP_CHANGED);
	pim_mre_task->set_rp_addr_prefix_rp(modified_prefix_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the G-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_MRIB_NEXT_HOP_RP_G_CHANGED);
	pim_mre_task->set_rp_addr_prefix_rp(modified_prefix_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the S-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_MRIB_NEXT_HOP_S_CHANGED);
	pim_mre_task->set_source_addr_prefix_sg_sg_rpt(modified_prefix_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

// TODO: not used (not needed?)
void
PimMrt::add_task_mrib_next_hop_rp_gen_id_changed(const IPvX& rp_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the RP-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_MRIB_NEXT_HOP_RP_GEN_ID_CHANGED);
	pim_mre_task->set_rp_addr_rp(rp_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_pim_nbr_changed(uint16_t vif_index, const IPvX& pim_nbr_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (*,*,RP)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_MRIB_NEXT_HOP_RP_CHANGED);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_pim_nbr_addr_rp(pim_nbr_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the (*,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_MRIB_NEXT_HOP_RP_G_CHANGED);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_pim_nbr_addr_wc(pim_nbr_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the (*,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_RPFP_NBR_WC_CHANGED);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_pim_nbr_addr_wc(pim_nbr_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the (S,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_MRIB_NEXT_HOP_S_CHANGED);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_pim_nbr_addr_sg_sg_rpt(pim_nbr_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the (S,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_RPFP_NBR_SG_CHANGED);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_pim_nbr_addr_sg_sg_rpt(pim_nbr_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the (S,G,rpt)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_RPFP_NBR_SG_RPT_CHANGED);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_pim_nbr_addr_sg_sg_rpt(pim_nbr_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_pim_nbr_gen_id_changed(uint16_t vif_index,
					const IPvX& pim_nbr_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the RP-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_MRIB_NEXT_HOP_RP_GEN_ID_CHANGED);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_pim_nbr_addr_rp(pim_nbr_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the (*,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_RPFP_NBR_WC_GEN_ID_CHANGED);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_pim_nbr_addr_wc(pim_nbr_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);

    do {
	// Schedule the (S,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_RPFP_NBR_SG_GEN_ID_CHANGED);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_pim_nbr_addr_sg_sg_rpt(pim_nbr_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

// TODO: not used (not needed?)
void
PimMrt::add_task_receive_join_rp(uint16_t vif_index, const IPvX& rp_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (*,*,RP)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_RECEIVE_JOIN_RP);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_rp_addr_rp(rp_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_receive_join_wc(uint16_t vif_index, const IPvX& group_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (*,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_RECEIVE_JOIN_WC);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_group_addr_wc(group_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_receive_join_sg(uint16_t vif_index,
				 const IPvX& source_addr,
				 const IPvX& group_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (S,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_RECEIVE_JOIN_SG);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_source_addr_sg_sg_rpt(source_addr);
	pim_mre_task->set_group_addr_sg_sg_rpt(group_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_receive_join_sg_rpt(uint16_t vif_index,
				     const IPvX& source_addr,
				     const IPvX& group_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (S,G,rpt)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_RECEIVE_JOIN_SG_RPT);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_source_addr_sg_sg_rpt(source_addr);
	pim_mre_task->set_group_addr_sg_sg_rpt(group_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_receive_prune_rp(uint16_t vif_index, const IPvX& rp_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (*,*,RP)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_RECEIVE_PRUNE_RP);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_rp_addr_rp(rp_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_receive_prune_wc(uint16_t vif_index, const IPvX& group_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (*,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_RECEIVE_PRUNE_WC);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_group_addr_wc(group_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_see_prune_wc(uint16_t vif_index, const IPvX& group_addr,
			      const IPvX& target_nbr_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (*,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_SEE_PRUNE_WC);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_group_addr_wc(group_addr);
	pim_mre_task->set_addr_arg(target_nbr_addr);	// XXX
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_receive_prune_sg(uint16_t vif_index,
				  const IPvX& source_addr,
				  const IPvX& group_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (S,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_RECEIVE_PRUNE_SG);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_source_addr_sg_sg_rpt(source_addr);
	pim_mre_task->set_group_addr_sg_sg_rpt(group_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_receive_prune_sg_rpt(uint16_t vif_index,
				      const IPvX& source_addr,
				      const IPvX& group_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (S,G,rpt)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_RECEIVE_PRUNE_SG_RPT);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_source_addr_sg_sg_rpt(source_addr);
	pim_mre_task->set_group_addr_sg_sg_rpt(group_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_receive_no_prune_sg_rpt(uint16_t vif_index,
					 const IPvX& group_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (S,G,rpt)-related changes (for a given group)
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_RECEIVE_NO_PRUNE_SG_RPT);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_group_addr_sg_sg_rpt(group_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_downstream_jp_state_rp(uint16_t vif_index,
					const IPvX& rp_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (*,*,RP)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_DOWNSTREAM_JP_STATE_RP);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_rp_addr_rp(rp_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_downstream_jp_state_wc(uint16_t vif_index,
					const IPvX& group_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (*,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_DOWNSTREAM_JP_STATE_WC);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_group_addr_wc(group_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_downstream_jp_state_sg(uint16_t vif_index,
					const IPvX& source_addr,
					const IPvX& group_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (S,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_DOWNSTREAM_JP_STATE_SG);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_source_addr_sg_sg_rpt(source_addr);
	pim_mre_task->set_group_addr_sg_sg_rpt(group_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_downstream_jp_state_sg_rpt(uint16_t vif_index,
					    const IPvX& source_addr,
					    const IPvX& group_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (S,G,rpt)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_DOWNSTREAM_JP_STATE_SG_RPT);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_source_addr_sg_sg_rpt(source_addr);
	pim_mre_task->set_group_addr_sg_sg_rpt(group_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_upstream_jp_state_sg(const IPvX& source_addr,
				      const IPvX& group_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (S,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_UPSTREAM_JP_STATE_SG);
	pim_mre_task->set_source_addr_sg_sg_rpt(source_addr);
	pim_mre_task->set_group_addr_sg_sg_rpt(group_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_local_receiver_include_wc(uint16_t vif_index,
					   const IPvX& group_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (*,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_LOCAL_RECEIVER_INCLUDE_WC);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_group_addr_wc(group_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_local_receiver_include_sg(uint16_t vif_index,
					   const IPvX& source_addr,
					   const IPvX& group_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (S,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_LOCAL_RECEIVER_INCLUDE_SG);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_source_addr_sg_sg_rpt(source_addr);
	pim_mre_task->set_group_addr_sg_sg_rpt(group_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_local_receiver_exclude_sg(uint16_t vif_index,
					   const IPvX& source_addr,
					   const IPvX& group_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (S,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_LOCAL_RECEIVER_EXCLUDE_SG);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_source_addr_sg_sg_rpt(source_addr);
	pim_mre_task->set_group_addr_sg_sg_rpt(group_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_assert_state_wc(uint16_t vif_index, const IPvX& group_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (*,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_ASSERT_STATE_WC);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_group_addr_wc(group_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_assert_state_sg(uint16_t vif_index,
				 const IPvX& source_addr,
				 const IPvX& group_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (S,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_ASSERT_STATE_SG);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_source_addr_sg_sg_rpt(source_addr);
	pim_mre_task->set_group_addr_sg_sg_rpt(group_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_i_am_dr(uint16_t vif_index)
{
    PimMreTask *pim_mre_task;

    do {
	// Schedule the (*,*,RP)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_I_AM_DR);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_rp_addr_prefix_rp(IPvXNet(family()));
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the (*,G)-related changes
	// XXX: most of processing will be redundant
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_I_AM_DR);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_group_addr_prefix_wc(IPvXNet::ip_multicast_base_prefix(family()));
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the (S,G) and (S,G,rpt)-related changes
	// XXX: most of processing will be redundant
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_I_AM_DR);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_source_addr_prefix_sg_sg_rpt(IPvXNet(family()));
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

//
// TODO: not used
//
void
PimMrt::add_task_my_ip_address(uint16_t vif_index)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (*,*,RP)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_MY_IP_ADDRESS);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_rp_addr_prefix_rp(IPvXNet(family()));
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the (*,G)-related changes
	// XXX: most of processing will be redundant
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_MY_IP_ADDRESS);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_group_addr_prefix_wc(IPvXNet::ip_multicast_base_prefix(family()));
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the (S,G) and (S,G,rpt)-related changes
	// XXX: most of processing will be redundant
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_MY_IP_ADDRESS);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_source_addr_prefix_sg_sg_rpt(IPvXNet(family()));
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

//
// TODO: not used
//
void
PimMrt::add_task_my_ip_subnet_address(uint16_t vif_index)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (*,*,RP)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_MY_IP_SUBNET_ADDRESS);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_rp_addr_prefix_rp(IPvXNet(family()));
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the (*,G)-related changes
	// XXX: most of processing will be redundant
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_MY_IP_SUBNET_ADDRESS);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_group_addr_prefix_wc(IPvXNet::ip_multicast_base_prefix(family()));
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the (S,G) and (S,G,rpt)-related changes
	// XXX: most of processing will be redundant
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_MY_IP_SUBNET_ADDRESS);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_source_addr_prefix_sg_sg_rpt(IPvXNet(family()));
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

//
// TODO: XXX: PAVPAVPAV: USE IT!!
//
void
PimMrt::add_task_is_switch_to_spt_desired_sg(const IPvX& source_addr,
					     const IPvX& group_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (S,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_IS_SWITCH_TO_SPT_DESIRED_SG);
	pim_mre_task->set_source_addr_sg_sg_rpt(source_addr);
	pim_mre_task->set_group_addr_sg_sg_rpt(group_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_keepalive_timer_sg(const IPvX& source_addr,
				    const IPvX& group_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (S,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_KEEPALIVE_TIMER_SG);
	pim_mre_task->set_source_addr_sg_sg_rpt(source_addr);
	pim_mre_task->set_group_addr_sg_sg_rpt(group_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_sptbit_sg(const IPvX& source_addr, const IPvX& group_addr)
{
    PimMreTask *pim_mre_task;
    
    do {
	// Schedule the (S,G)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_SPTBIT_SG);
	pim_mre_task->set_source_addr_sg_sg_rpt(source_addr);
	pim_mre_task->set_group_addr_sg_sg_rpt(group_addr);
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_start_vif(uint16_t vif_index)
{
    PimMreTask *pim_mre_task;
    
    //
    // The incoming interfaces
    //
    do {
	// Schedule the RP-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_MRIB_RP_CHANGED);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_rp_addr_prefix_rp(IPvXNet(family()));
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the S-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_MRIB_S_CHANGED);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_source_addr_prefix_sg_sg_rpt(IPvXNet(family()));
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);

    //
    // The outgoing interfaces
    //
    do {
	// Schedule the (*,*,RP)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_IN_START_VIF);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_rp_addr_prefix_rp(IPvXNet(family()));
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the (*,G)-related changes
	// XXX: most of processing will be redundant
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_IN_START_VIF);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_group_addr_prefix_wc(IPvXNet::ip_multicast_base_prefix(family()));
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the (S,G) and (S,G,rpt)-related changes
	// XXX: most of processing will be redundant
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_IN_START_VIF);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_source_addr_prefix_sg_sg_rpt(IPvXNet(family()));
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_stop_vif(uint16_t vif_index)
{
    PimMreTask *pim_mre_task;
    
    //
    // The incoming interfaces
    //
    do {
	// Schedule the RP-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_MRIB_RP_CHANGED);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_rp_addr_prefix_rp(IPvXNet(family()));
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the S-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_MRIB_S_CHANGED);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_source_addr_prefix_sg_sg_rpt(IPvXNet(family()));
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);

    //
    // The outgoing interfaces
    //
    do {
	// Schedule the (*,*,RP)-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_IN_STOP_VIF);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_rp_addr_prefix_rp(IPvXNet(family()));
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the (*,G)-related changes
	// XXX: most of processing will be redundant
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_IN_STOP_VIF);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_group_addr_prefix_wc(IPvXNet::ip_multicast_base_prefix(family()));
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
    
    do {
	// Schedule the (S,G) and (S,G,rpt)-related changes
	// XXX: most of processing will be redundant
	pim_mre_task
	    = new PimMreTask(*this,
			     PimMreTrackState::INPUT_STATE_IN_STOP_VIF);
	pim_mre_task->set_vif_index(vif_index);
	pim_mre_task->set_source_addr_prefix_sg_sg_rpt(IPvXNet(family()));
	
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_add_pim_mre(PimMre *pim_mre)
{
    PimMreTask *pim_mre_task = NULL;
    PimMreTrackState::input_state_t input_state = PimMreTrackState::INPUT_STATE_MAX;
    
    if (pim_mre->is_task_delete_pending()) {
	XLOG_ASSERT(false);
	return;
    }
    
    //
    // Compute the proper input state
    //
    do {
	if (pim_mre->is_rp()) {
	    input_state = PimMreTrackState::INPUT_STATE_IN_ADD_PIM_MRE_RP;
	    break;
	}
	if (pim_mre->is_wc()) {
	    input_state = PimMreTrackState::INPUT_STATE_IN_ADD_PIM_MRE_WC;
	    break;
	}
	if (pim_mre->is_sg()) {
	    input_state = PimMreTrackState::INPUT_STATE_IN_ADD_PIM_MRE_SG;
	    break;
	}
	if (pim_mre->is_sg_rpt()) {
	    input_state = PimMreTrackState::INPUT_STATE_IN_ADD_PIM_MRE_SG_RPT;
	    break;
	}
	XLOG_ASSERT(false);
	return;
    } while (false);
    
    //
    // If the lastest task is same, just
    // reuse that task. Otherwise, allocate a new task.
    //
    list<PimMreTask *>::reverse_iterator iter;
    iter = pim_mre_task_list().rbegin();
    if (iter != pim_mre_task_list().rend()) {
	pim_mre_task = *iter;
	if (pim_mre_task->input_state() == input_state) {
	    pim_mre_task->add_pim_mre(pim_mre);
	    return;
	}
    }
    
    do {
	// Schedule the PimMre-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     input_state);
	pim_mre_task->add_pim_mre(pim_mre);		// XXX
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

void
PimMrt::add_task_delete_pim_mre(PimMre *pim_mre)
{
    PimMreTask *pim_mre_task = NULL;
    PimMreTrackState::input_state_t input_state = PimMreTrackState::INPUT_STATE_MAX;
    
    if (pim_mre->is_task_delete_pending()) {
	// The entry is already pending deletion.
	// Shoudn't happen, but just in case...
	return;
    }
    
    //
    // Compute the proper input state
    //
    do {
	if (pim_mre->is_rp()) {
	    input_state = PimMreTrackState::INPUT_STATE_IN_REMOVE_PIM_MRE_RP;
	    break;
	}
	if (pim_mre->is_wc()) {
	    input_state = PimMreTrackState::INPUT_STATE_IN_REMOVE_PIM_MRE_WC;
	    break;
	}
	if (pim_mre->is_sg()) {
	    input_state = PimMreTrackState::INPUT_STATE_IN_REMOVE_PIM_MRE_SG;
	    break;
	}
	if (pim_mre->is_sg_rpt()) {
	    input_state = PimMreTrackState::INPUT_STATE_IN_REMOVE_PIM_MRE_SG_RPT;
	    break;
	}
	XLOG_ASSERT(false);
	return;
    } while (false);
    
    //
    // Remove the entry from the PimMrt only, and mark it as pending deletion
    //
    remove_pim_mre(pim_mre);
    pim_mre->set_is_task_delete_pending(true);
    
    //
    // If the lastest task is same, just
    // reuse that task. Otherwise, allocate a new task.
    //
    list<PimMreTask *>::reverse_iterator iter;
    iter = pim_mre_task_list().rbegin();
    if (iter != pim_mre_task_list().rend()) {
	pim_mre_task = *iter;
	if (pim_mre_task->input_state() == input_state) {
	    pim_mre_task->add_pim_mre(pim_mre);
	    return;
	}
    }
    
    do {
	// Schedule the PimMre-related changes
	pim_mre_task
	    = new PimMreTask(*this,
			     input_state);
	pim_mre_task->add_pim_mre(pim_mre);		// XXX
	add_task(pim_mre_task);
	schedule_task(pim_mre_task);
    } while (false);
}

