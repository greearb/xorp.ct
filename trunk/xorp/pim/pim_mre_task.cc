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

#ident "$XORP: xorp/pim/pim_mre_task.cc,v 1.5 2003/03/10 23:20:48 hodson Exp $"

//
// PIM Multicast Routing Entry task
//


#include "pim_module.h"
#include "pim_private.hh"
#include "libxorp/time_slice.hh"
#include "libxorp/utils.hh"
#include "pim_mfc.hh"
#include "pim_mre_task.hh"
#include "pim_mrt.hh"
#include "pim_nbr.hh"
#include "pim_node.hh"


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


PimMreTask::PimMreTask(PimMrt& pim_mrt,
		       PimMreTrackState::input_state_t input_state)
    : _pim_mrt(pim_mrt),
      _time_slice(100000, 20),		// 100ms, test every 20th iter
      _input_state(input_state),
      //
      _is_set_rp_addr_rp(false),
      _rp_addr_rp(IPvX::ZERO(family())),
      _is_set_rp_addr_prefix_rp(false),
      _rp_addr_prefix_rp(IPvX::ZERO(family()), 0),
      _is_processing_rp(false),
      _is_processing_rp_addr_rp(false),
      _processing_rp_addr_rp(IPvX::ZERO(family())),
      //
      _is_set_group_addr_wc(false),
      _group_addr_wc(IPvX::ZERO(family())),
      _is_set_rp_addr_wc(false),
      _rp_addr_wc(IPvX::ZERO(family())),
      _is_set_group_addr_prefix_wc(false),
      _group_addr_prefix_wc(IPvX::ZERO(family()), 0),
      _is_processing_wc(false),
      _is_processing_rp_addr_wc(false),
      _processing_rp_addr_wc(IPvX::ZERO(family())),
      _is_processing_group_addr_wc(false),
      _processing_group_addr_wc(IPvX::ZERO(family())),
      //
      _is_set_source_addr_sg_sg_rpt(false),
      _source_addr_sg_sg_rpt(IPvX::ZERO(family())),
      _is_set_group_addr_sg_sg_rpt(false),
      _group_addr_sg_sg_rpt(IPvX::ZERO(family())),
      _is_set_source_addr_prefix_sg_sg_rpt(false),
      _source_addr_prefix_sg_sg_rpt(IPvX::ZERO(family()), 0),
      _is_set_rp_addr_sg_sg_rpt(false),
      _rp_addr_sg_sg_rpt(IPvX::ZERO(family())),
      _is_processing_sg_sg_rpt(false),
      _is_processing_sg_source_addr_sg_sg_rpt(false),
      _processing_sg_source_addr_sg_sg_rpt(IPvX::ZERO(family())),
      _is_processing_sg_rpt_source_addr_sg_sg_rpt(false),
      _processing_sg_rpt_source_addr_sg_sg_rpt(IPvX::ZERO(family())),
      _is_processing_sg_group_addr_sg_sg_rpt(false),
      _processing_sg_group_addr_sg_sg_rpt(IPvX::ZERO(family())),
      _is_processing_sg_rpt_group_addr_sg_sg_rpt(false),
      _processing_sg_rpt_group_addr_sg_sg_rpt(IPvX::ZERO(family())),
      _is_processing_rp_addr_sg(false),
      _is_processing_rp_addr_sg_rpt(false),
      _processing_rp_addr_sg_sg_rpt(IPvX::ZERO(family())),
      //
      _is_set_source_addr_mfc(false),
      _source_addr_mfc(IPvX::ZERO(family())),
      _is_set_group_addr_mfc(false),
      _group_addr_mfc(IPvX::ZERO(family())),
      _is_set_source_addr_prefix_mfc(false),
      _source_addr_prefix_mfc(IPvX::ZERO(family()), 0),
      _is_set_rp_addr_mfc(false),
      _rp_addr_mfc(IPvX::ZERO(family())),
      _is_processing_mfc(false),
      _is_processing_source_addr_mfc(false),
      _processing_source_addr_mfc(IPvX::ZERO(family())),
      _is_processing_group_addr_mfc(false),
      _processing_group_addr_mfc(IPvX::ZERO(family())),
      _is_processing_rp_addr_mfc(false),
      _processing_rp_addr_mfc(IPvX::ZERO(family())),
      //
      _is_set_pim_nbr_addr_rp(false),
      _is_set_pim_nbr_addr_wc(false),
      _is_set_pim_nbr_addr_sg_sg_rpt(false),
      _pim_nbr_addr(IPvX::ZERO(family())),
      _is_processing_pim_nbr_addr_rp(false),
      _is_processing_pim_nbr_addr_wc(false),
      _is_processing_pim_nbr_addr_sg(false),
      _is_processing_pim_nbr_addr_sg_rpt(false),
      //
      _vif_index(Vif::VIF_INDEX_INVALID),
      _addr_arg(IPvX::ZERO(family()))
{
    const PimMreTrackState& pim_mre_track_state
	= _pim_mrt.pim_mre_track_state();
    
    // Get the lists of actions
    _action_list_rp = pim_mre_track_state.output_action_rp(input_state);
    _action_list_wc = pim_mre_track_state.output_action_wc(input_state);
    _action_list_sg_sg_rpt = pim_mre_track_state.output_action_sg_sg_rpt(
	input_state);
    _action_list_mfc = pim_mre_track_state.output_action_mfc(input_state);
}

PimMreTask::~PimMreTask()
{
    // Delete the (*,*,RP) entries pending deletion
    while (! _pim_mre_rp_delete_list.empty()) {
	PimMre *pim_mre = _pim_mre_rp_delete_list.front();
	_pim_mre_rp_delete_list.pop_front();
	if (pim_mre->is_task_delete_done())
	    delete pim_mre;
    }
    
    // Delete the (*,G) entries pending deletion
    while (! _pim_mre_wc_delete_list.empty()) {
	PimMre *pim_mre = _pim_mre_wc_delete_list.front();
	_pim_mre_wc_delete_list.pop_front();
	if (pim_mre->is_task_delete_done())
	    delete pim_mre;
    }
    
    // Delete the (S,G) entries pending deletion
    while (! _pim_mre_sg_delete_list.empty()) {
	PimMre *pim_mre = _pim_mre_sg_delete_list.front();
	_pim_mre_sg_delete_list.pop_front();
	if (pim_mre->is_task_delete_done())
	    delete pim_mre;
    }
    
    // Delete the (S,G,rpt) entries pending deletion
    while (! _pim_mre_sg_rpt_delete_list.empty()) {
	PimMre *pim_mre = _pim_mre_sg_rpt_delete_list.front();
	_pim_mre_sg_rpt_delete_list.pop_front();
	if (pim_mre->is_task_delete_done())
	    delete pim_mre;
    }
    
    // Delete the PimMfc entries pending deletion
    while (! _pim_mfc_delete_list.empty()) {
	PimMfc *pim_mfc = _pim_mfc_delete_list.front();
	_pim_mfc_delete_list.pop_front();
	if (pim_mfc->is_task_delete_done())
	    delete pim_mfc;
    }
    
    pim_mrt().delete_task(this);
}

PimNode&
PimMreTask::pim_node() const
{
    return (_pim_mrt.pim_node());
}

int
PimMreTask::family() const
{
    return (_pim_mrt.family());
}

//
// (Re)schedule the task to run (again).
// 
void
PimMreTask::schedule_task()
{
    _time_slice_timer =
	pim_node().event_loop().new_oneoff_after(
	    TimeVal(0, 1),
	    callback(this, &PimMreTask::time_slice_timer_timeout));
}

//
// Run the task.
//
// Return true if the processing was saved for later because it was taking too
// long time, otherwise return false.
bool
PimMreTask::run_task()
{
    _time_slice.reset();
    
    if (run_task_rp()) {
	// Time slice has expired. Schedule a new time slice.
	schedule_task();
	return (true);
    }
    
    delete this;	// XXX: should be right before the return
    
    return (false);
}

//
// Run the (*,*,RP) related actions if any.
// In addition, call the appropriate method to run the (*,G) related actions.
//
// Return true if the processing was saved for later because it was taking too
// long time, otherwise return false.
bool
PimMreTask::run_task_rp()
{
    bool ret_value = true;
    PimMre *pim_mre;
    PimMrtRp::const_sg_iterator rp_iter, rp_iter_begin, rp_iter_end;
    
    // Test if we need to continue processing of (*,G) or (S,G)
    // or (S,G,rpt) entries from an earlier time slice.
    // XXX: run_task_wc() will take care of testing to continue
    // processing for (S,G), (S,G,rpt) or PimMfc entries.
    if (_is_processing_wc || _is_processing_sg_sg_rpt || _is_processing_mfc) {
	if (run_task_wc())
	    return (true);
    }
    
    if (_is_set_rp_addr_rp) {
	// Actions for a single RP specified by an unicast address.
	
	// XXX: for simplicity, we always perform the (*,*,RP) actions without
	// considering the time slice, simply because there is no more than
	// one (*,*,RP) entry for the particular RP.
	
	pim_mre = pim_mrt().pim_mre_find(_rp_addr_rp,
					 IPvX::ZERO(family()),
					 PIM_MRE_RP,
					 0);
	// Perform the (*,*,RP) actions
	perform_pim_mre_actions(pim_mre);
	
	// Prepare the (*,G) related state
	_rp_addr_wc = _rp_addr_rp;
	_is_set_rp_addr_wc = true;
    }
    _is_set_rp_addr_rp = false;
    
    if (_is_set_rp_addr_prefix_rp) {
	// Actions for a set of RPs specified by an unicast address prefix.
	
	//
	// Get the iterator boundaries by considering whether
	// we had to stop processing during an earlier time slice.
	//
	rp_iter_end = pim_mrt().pim_mrt_rp().source_by_prefix_end(
	    _rp_addr_prefix_rp);
	if (! _is_processing_rp_addr_rp) {
	    rp_iter_begin = pim_mrt().pim_mrt_rp().source_by_prefix_begin(
		_rp_addr_prefix_rp);
	} else {
	    rp_iter_begin = pim_mrt().pim_mrt_rp().source_by_addr_begin(
		_processing_rp_addr_rp);
	}
	
	for (rp_iter = rp_iter_begin; rp_iter != rp_iter_end; ) {
	    pim_mre = rp_iter->second;
	    ++rp_iter;
	    
	    _rp_addr_rp = *pim_mre->rp_addr_ptr();	// The RP address
	    
	    // Perform the (*,*,RP) actions
	    perform_pim_mre_actions(pim_mre);
	    
	    // Schedule and perform the (*,G), (S,G), (S,G,rpt), PimMfc
	    // processing (if any) for this RP address prefix.
	    bool time_slice_expired = false;
	    if (! (_action_list_wc.empty()
		   && _action_list_sg_sg_rpt.empty()
		   && _action_list_mfc.empty())) {
		_rp_addr_wc = _rp_addr_rp;
		_is_set_rp_addr_wc = true;
		if (run_task_wc())
		    time_slice_expired = true;
	    }
	    if (time_slice_expired || _time_slice.is_expired()) {
		// Time slice has expired. Save state if necessary and return.
		if (rp_iter != rp_iter_end) {
		    pim_mre = rp_iter->second;
		    _is_processing_rp = true;
		    _is_processing_rp_addr_rp = true;
		    _processing_rp_addr_rp = *pim_mre->rp_addr_ptr();
		}
		return (true);
	    }
	}
	_is_processing_rp_addr_rp = false;
    }
    _is_set_rp_addr_prefix_rp = false;
    
    if (_is_set_pim_nbr_addr_rp) {
	// Actions for a set of (*,*,RP) specified by a PimNbr address.
	
	// Prepare the processing of the (*,*,RP) entries for this PimNbr
	if (! _is_processing_pim_nbr_addr_rp)
	    pim_node().init_processing_pim_mre_rp(_vif_index, _pim_nbr_addr);
	
	PimNbr *pim_nbr = pim_node().find_processing_pim_mre_rp(_vif_index,
								_pim_nbr_addr);
	if (pim_nbr != NULL) {
	    
	    bool more_processing = !pim_nbr->processing_pim_mre_rp_list().empty();
	    while (more_processing) {
		// Move the PimMre entry to the non-processing list
		PimMre *pim_mre = pim_nbr->processing_pim_mre_rp_list().front();
		pim_nbr->processing_pim_mre_rp_list().pop_front();
		pim_nbr->pim_mre_rp_list().push_back(pim_mre);
		// XXX: we need to test the 'processing_pim_mre_rp_list()'
		// now in case the PimRp entry is deleted during processing.
		more_processing = !pim_nbr->processing_pim_mre_rp_list().empty();
		// Perform the (*,*,RP) actions
		perform_pim_mre_actions(pim_mre);
		
		if (_time_slice.is_expired()) {
		    // Stop processing. Save state to continue later from here.
		    _is_processing_rp = true;
		    _is_processing_pim_nbr_addr_rp = true;
		    // XXX: no need to keep state for the current entry
		    // we are processing because the
		    // PimNbr::processing_pim_mre_rp_list()
		    // keeps track of that.
		    return (true);
		}
	    }
	}
	_is_processing_pim_nbr_addr_rp = false;
	_is_set_pim_nbr_addr_wc = true;
    }
    _is_set_pim_nbr_addr_rp = false;
    
    //
    // Process the list of (*,*,RP) PimMre entries
    //
    while (! _pim_mre_rp_list.empty()) {
	PimMre *pim_mre = _pim_mre_rp_list.front();
	_pim_mre_rp_list.pop_front();
	
	// Perform the (*,*,RP) actions
	perform_pim_mre_actions(pim_mre);
	
	// Add to the list of processed entries
	_pim_mre_rp_processed_list.push_back(pim_mre);
	
	XLOG_ASSERT(pim_mre->rp_addr_ptr() != NULL);
	
	// Schedule and perform the (*,G), (S,G), (S,G,rpt), PimMfc
	// processing (if any) for this RP address.
	bool time_slice_expired = false;
	if (! (_action_list_wc.empty()
	       && _action_list_sg_sg_rpt.empty()
	       && _action_list_mfc.empty())) {
	    _rp_addr_wc = *pim_mre->rp_addr_ptr();
	    _is_set_rp_addr_wc = true;
	    if (run_task_wc())
		time_slice_expired = true;
	}
	if (time_slice_expired || _time_slice.is_expired()) {
	    // Time slice has expired. Save state if necessary and return.
	    return (true);
	}
    }
    
    //
    // We are done with the (*,*,RP) processing
    //
    _is_processing_rp = false;
    _is_processing_rp_addr_rp = false;
    _is_processing_pim_nbr_addr_rp = false;
    
    //
    // Schedule and perform the (*,G), (S,G), (S,G,rpt), PimMfc
    // processing (if any).
    //
    do {
	if (_action_list_wc.empty()
	    && _action_list_sg_sg_rpt.empty()
	    && _action_list_mfc.empty()) {
	    ret_value = false;
	    break;
	}
	if (run_task_wc()) {
	    ret_value = true;
	    break;
	}
	ret_value = false;
	break;
    } while (false);
    
    if (ret_value)
	return (ret_value);
    
    //
    // Delete the (*,*,RP) PimMre entries that are pending deletion
    //
    while (! _pim_mre_rp_delete_list.empty()) {
	PimMre *pim_mre = _pim_mre_rp_delete_list.front();
	_pim_mre_rp_delete_list.pop_front();
	if (pim_mre->is_task_delete_done())
	    delete pim_mre;
	if (_time_slice.is_expired())
	    return (true);
    }
    
    return (ret_value);
}

//
// Run the (*,G) related actions if any.
// In addition, call the appropriate method to run the (S,G), (S,G,rpt), PimMfc
// related actions.
//
// Return true if the processing was saved for later because it was taking too
// long time, otherwise return false.
bool
PimMreTask::run_task_wc()
{
    bool ret_value = true;
    PimMre *pim_mre;
    PimMrtG::const_gs_iterator g_iter, g_iter_begin, g_iter_end;
    
    // Test if we need to continue processing of (S,G) or (S,G,rpt)
    // entries from an earlier time slice.
    if (_is_processing_sg_sg_rpt || _is_processing_mfc) {
	if (run_task_sg_sg_rpt())
	    return (true);
    }
    
    if (_is_set_group_addr_wc) {
	// Actions for a single group specified by a multicast address.
	
	// XXX: for simplicity, we always perform the (*,G) actions without
	// considering the time slice, simply because in this case there is
	// only one (*,G) entry.
	
	pim_mre = pim_mrt().pim_mre_find(IPvX::ZERO(family()),
					 _group_addr_wc,
					 PIM_MRE_WC,
					 0);
	// Perform the (*,G) actions
	perform_pim_mre_actions(pim_mre);
	
	// Prepare the (S,G) and (S,G,rpt) related state
	_group_addr_sg_sg_rpt = _group_addr_wc;
	_is_set_group_addr_sg_sg_rpt = true;
    }
    _is_set_group_addr_wc = false;
    
    if (_is_set_rp_addr_wc) {
	// Actions for a set of (*,G) entries specified by the RP address
	// they match to.
	
	PimRp *pim_rp;
	RpTable& rp_table = pim_node().rp_table();
	
	// Prepare the processing of the (*,G) entries for this RP address
	if (! _is_processing_rp_addr_wc) {
	    rp_table.init_processing_pim_mre_wc(_rp_addr_wc);
	} else {
	    _rp_addr_wc = _processing_rp_addr_wc;
	}
	
	while ((pim_rp = rp_table.find_processing_pim_mre_wc(_rp_addr_wc))
		!= NULL) {
	    
	    bool more_processing = !pim_rp->processing_pim_mre_wc_list().empty();
	    while (more_processing) {
		// Move the PimMre entry to the non-processing list
		PimMre *pim_mre = pim_rp->processing_pim_mre_wc_list().front();
		pim_rp->processing_pim_mre_wc_list().pop_front();
		pim_rp->pim_mre_wc_list().push_back(pim_mre);
		// XXX: we need to test the 'processing_pim_mre_wc_list()'
		// now in case the PimRp entry is deleted during processing.
		more_processing = !pim_rp->processing_pim_mre_wc_list().empty();
		
		// Perform the (*,G) actions
		perform_pim_mre_actions(pim_mre);
		
		// Schedule and perform the (*,G), (S,G), (S,G,rpt), PimMfc
		// processing (if any) for this multicast group.
		bool time_slice_expired = false;
		if (! (_action_list_sg_sg_rpt.empty()
		       && _action_list_mfc.empty())) {
		    _is_set_source_addr_sg_sg_rpt = false;
		    _group_addr_sg_sg_rpt = pim_mre->group_addr();
		    _is_set_group_addr_sg_sg_rpt = true;
		    _is_set_source_addr_prefix_sg_sg_rpt = false;
		    if (run_task_sg_sg_rpt())
			time_slice_expired = true;
		}
		if (time_slice_expired || _time_slice.is_expired()) {
		    // Stop processing. Save state to continue later from here.
		    if (more_processing) {
			_is_processing_wc = true;
			_is_processing_rp_addr_wc = true;
			_processing_rp_addr_wc = _rp_addr_wc;
			// XXX: no need to keep state for the current group
			// we are processing because the
			// PimRp::processing_pim_mre_wc_list()
			// keeps track of that.
		    }
		    return (true);
		}
	    }
	}
	_is_processing_rp_addr_wc = false;
	
	// Prepare the (S,G) and (S,G,rpt) related state
	_rp_addr_sg_sg_rpt = _rp_addr_wc;
	_is_set_rp_addr_sg_sg_rpt = true;
    }
    _is_set_rp_addr_wc = false;
    
    if (_is_set_group_addr_prefix_wc) {
	// Actions for a set of (*,G) entries specified by a multicast
	// address prefix.
	
	//
	// Get the iterator boundaries by considering whether
	// we had to stop processing during an earlier time slice.
	//
	g_iter_end = pim_mrt().pim_mrt_g().group_by_prefix_end(
	    _group_addr_prefix_wc);
	if (! _is_processing_group_addr_wc) {
	    g_iter_begin = pim_mrt().pim_mrt_g().group_by_prefix_begin(
		_group_addr_prefix_wc);
	} else {
	    g_iter_begin = pim_mrt().pim_mrt_g().group_by_addr_begin(
		_processing_group_addr_wc);
	}
	
	for (g_iter = g_iter_begin; g_iter != g_iter_end; ) {
	    pim_mre = g_iter->second;
	    ++g_iter;
	    
	    // Perform the (*,G) actions
	    perform_pim_mre_actions(pim_mre);
	    
	    // Schedule and perform the (S,G), (S,G,rpt), PimMfc
	    // processing (if any) for this multicast group.
	    bool time_slice_expired = false;
	    if (! (_action_list_sg_sg_rpt.empty()
		   && _action_list_mfc.empty())) {
		_is_set_source_addr_sg_sg_rpt = false;
		_group_addr_sg_sg_rpt = pim_mre->group_addr();
		_is_set_group_addr_sg_sg_rpt = true;
		_is_set_source_addr_prefix_sg_sg_rpt = false;
		if (run_task_sg_sg_rpt())
		    time_slice_expired = true;
	    }
	    if (time_slice_expired || _time_slice.is_expired()) {
		// Time slice has expired. Save state if necessary and return.
		if (g_iter != g_iter_end) {
		    pim_mre = g_iter->second;
		    _processing_group_addr_wc = pim_mre->group_addr();
		    _is_processing_wc = true;
		    _is_processing_group_addr_wc = true;
		}
		return (true);
	    }
	}
	_is_processing_group_addr_wc = false;
    }
    _is_set_group_addr_prefix_wc = false;
    
    if (_is_set_pim_nbr_addr_wc) {
	// Actions for a set of (*,G) specified by a PimNbr address.
	
	// Prepare the processing of the (*,G) entries for this PimNbr
	if (! _is_processing_pim_nbr_addr_wc)
	    pim_node().init_processing_pim_mre_wc(_vif_index, _pim_nbr_addr);
	
	PimNbr *pim_nbr = pim_node().find_processing_pim_mre_wc(_vif_index,
								_pim_nbr_addr);
	if (pim_nbr != NULL) {
	    
	    bool more_processing = !pim_nbr->processing_pim_mre_wc_list().empty();
	    while (more_processing) {
		// Move the PimMre entry to the non-processing list
		PimMre *pim_mre = pim_nbr->processing_pim_mre_wc_list().front();
		pim_nbr->processing_pim_mre_wc_list().pop_front();
		pim_nbr->pim_mre_wc_list().push_back(pim_mre);
		// we need to test the 'processing_pim_mre_wc_list()'
		// now in case the PimRp entry is deleted during processing.
		more_processing = !pim_nbr->processing_pim_mre_wc_list().empty();
		// Perform the (*,G) actions
		perform_pim_mre_actions(pim_mre);
		
		// Schedule and perform the (S,G), (S,G,rpt), PimMfc
		// processing (if any) for this multicast group.
		// XXX: note that this may result in processing
		// those entries twice. The reason is because the (S,G)
		// and (S,G,rpt) entries are added to the appropriate list
		// for this PimNbr even if they may have a (*,G) entry.
		// If this is not desirable, then remove the code below.
		// However, if we want to process the (S,G) and (S,G,rpt)
		// entries right after the corresponding (*,G) entry,
		// we need the processing below.
		bool time_slice_expired = false;
		if (! (_action_list_sg_sg_rpt.empty()
		       && _action_list_mfc.empty())) {
		    _is_set_source_addr_sg_sg_rpt = false;
		    _group_addr_sg_sg_rpt = pim_mre->group_addr();
		    _is_set_group_addr_sg_sg_rpt = true;
		    _is_set_source_addr_prefix_sg_sg_rpt = false;
		    if (run_task_sg_sg_rpt())
			time_slice_expired = true;
		}
		if (time_slice_expired || _time_slice.is_expired()) {
		    // Time slice has expired. Save state to continue late
		    // from here.
		    _is_processing_wc = true;
		    _is_processing_pim_nbr_addr_wc = true;
		    // XXX: no need to keep state for the current entry
		    // we are processing because the
		    // PimNbr::processing_pim_mre_wc_list()
		    // keeps track of that.
		    return (true);
		}
	    }
	}
	_is_processing_pim_nbr_addr_wc = false;
	_is_set_pim_nbr_addr_sg_sg_rpt = true;
    }
    _is_set_pim_nbr_addr_wc = false;
    
    //
    // Process the list of (*,G) PimMre entries
    //
    while (! _pim_mre_wc_list.empty()) {
	PimMre *pim_mre = _pim_mre_wc_list.front();
	_pim_mre_wc_list.pop_front();
	
	// Perform the (*,G) actions
	perform_pim_mre_actions(pim_mre);
	
	// Add to the list of processed entries
	_pim_mre_wc_processed_list.push_back(pim_mre);
	
	// Schedule and perform the (S,G), (S,G,rpt), PimMfc
	// processing (if any) for this multicast group.
	bool time_slice_expired = false;
	if (! (_action_list_sg_sg_rpt.empty()
	       && _action_list_mfc.empty())) {
	    _is_set_source_addr_sg_sg_rpt = false;
	    _group_addr_sg_sg_rpt = pim_mre->group_addr();
	    _is_set_group_addr_sg_sg_rpt = true;
	    _is_set_source_addr_prefix_sg_sg_rpt = false;
	    if (run_task_sg_sg_rpt())
		time_slice_expired = true;
	}
	if (time_slice_expired || _time_slice.is_expired()) {
	    // Time slice has expired. Save state if necessary and return.
	    return (true);
	}
    }
    
    //
    // We are done with the (*,G) processing
    //
    _is_processing_wc = false;
    _is_processing_rp_addr_wc = false;
    _is_processing_group_addr_wc = false;
    _is_processing_pim_nbr_addr_wc = false;
    
    //
    // Schedule and perform the (S,G), (S,G,rpt), PimMfc processing (if any)
    //
    do {
	if (_action_list_sg_sg_rpt.empty()
	    && _action_list_mfc.empty()) {
	    ret_value = false;
	    break;
	}
	if (run_task_sg_sg_rpt()) {
	    ret_value = true;
	    break;
	}
	ret_value = false;
	break;
    } while (false);
    
    if (ret_value)
	return (ret_value);
    
    //
    // Delete the (*,G) PimMre entries that are pending deletion
    //
    while (! _pim_mre_wc_delete_list.empty()) {
	PimMre *pim_mre = _pim_mre_wc_delete_list.front();
	_pim_mre_wc_delete_list.pop_front();
	if (pim_mre->is_task_delete_done())
	    delete pim_mre;
	if (_time_slice.is_expired())
	    return (true);
    }
    
    return (ret_value);
}

//
// Run the (S,G) and (S,G,rpt) related actions if any.
//
// Return true if the processing was saved for later because it was taking too
// long time, otherwise return false.
bool
PimMreTask::run_task_sg_sg_rpt()
{
    bool ret_value = true;
    PimMre *pim_mre_sg, *pim_mre_sg_rpt;
    
    // Test if we need to continue processing of PimMfc
    // entries from an earlier time slice.
    if (_is_processing_mfc) {
	if (run_task_mfc())
	    return (true);
    }
    
    if (_is_set_source_addr_sg_sg_rpt && _is_set_group_addr_sg_sg_rpt) {
	// Actions for a single (S,G) and/or (S,G,rpt) entry
	
	// XXX: for simplicity, we always perform the (S,G) and/or (S,G,rpt)
	// actions without considering the time slice, simply because in this
	// case there are no more than two entries.
	pim_mre_sg = pim_mrt().pim_mre_find(_source_addr_sg_sg_rpt,
					    _group_addr_sg_sg_rpt,
					    PIM_MRE_SG,
					    0);
	if (pim_mre_sg != NULL)
	    pim_mre_sg_rpt = pim_mre_sg->sg_rpt_entry();
	else
	    pim_mre_sg_rpt = pim_mrt().pim_mre_find(_source_addr_sg_sg_rpt,
						    _group_addr_sg_sg_rpt,
						    PIM_MRE_SG_RPT,
						    0);
	// Perform the (S,G) and (S,G,rpt) actions
	perform_pim_mre_sg_sg_rpt_actions(pim_mre_sg, pim_mre_sg_rpt);
	
	// Prepare the PimMfc related state
	_source_addr_mfc = _source_addr_sg_sg_rpt;
	_is_set_source_addr_mfc = true;
	_group_addr_mfc = _group_addr_sg_sg_rpt;
	_is_set_group_addr_mfc = true;
    }
    
    if ( (! _is_set_source_addr_sg_sg_rpt) && _is_set_group_addr_sg_sg_rpt) {
	// Actions for a set of (S,G) and/or (S,G,rpt) entries for a
	// particular multicast group address.
	
	//
	// Perform the actions for all (S,G) and (S,G,rpt) entries
	//
	PimMrtSg::const_gs_iterator gs_iter, gs_iter_begin, gs_iter_end;
	
	gs_iter_end = pim_mrt().pim_mrt_sg().group_by_addr_end(
	    _group_addr_sg_sg_rpt);
	
	if (! (_is_processing_sg_source_addr_sg_sg_rpt
	       || _is_processing_sg_rpt_source_addr_sg_sg_rpt)) {
	    gs_iter_begin = pim_mrt().pim_mrt_sg().group_by_addr_begin(
		_group_addr_sg_sg_rpt);
	} else {
	    if (_is_processing_sg_source_addr_sg_sg_rpt) {
		gs_iter_begin = pim_mrt().pim_mrt_sg().group_source_by_addr_begin(
		    _processing_sg_source_addr_sg_sg_rpt,
		    _group_addr_sg_sg_rpt);
	    } else {
		// XXX: continue with the (S,G,rpt) processing later
		gs_iter_begin = gs_iter_end;
	    }
	}
	
	for (gs_iter = gs_iter_begin; gs_iter != gs_iter_end; ) {
	    pim_mre_sg = gs_iter->second;
	    ++gs_iter;
	    
	    pim_mre_sg_rpt = pim_mre_sg->sg_rpt_entry();
	    
	    // Perform the (S,G) and (S,G,rpt) actions
	    perform_pim_mre_sg_sg_rpt_actions(pim_mre_sg, pim_mre_sg_rpt);
	    
	    if (_time_slice.is_expired()) {
		// Stop processing. Save state to continue later from here.
		if (gs_iter != gs_iter_end) {
		    pim_mre_sg = gs_iter->second;
		    _processing_sg_source_addr_sg_sg_rpt
			= pim_mre_sg->source_addr();
		    _is_processing_sg_sg_rpt = true;
		    _is_processing_sg_source_addr_sg_sg_rpt = true;
		}
		return (true);
	    }
	}
	_is_processing_sg_source_addr_sg_sg_rpt = false;
	
	//
	// Perform the actions for only those (S,G,rpt) entries that do not
	// have a corresponding (S,G) entry.
	//
	gs_iter_end = pim_mrt().pim_mrt_sg_rpt().group_by_addr_end(
	    _group_addr_sg_sg_rpt);
	
	if (! _is_processing_sg_rpt_source_addr_sg_sg_rpt) {
	    gs_iter_begin = pim_mrt().pim_mrt_sg_rpt().group_by_addr_begin(
		_group_addr_sg_sg_rpt);
	} else {
	    gs_iter_begin = pim_mrt().pim_mrt_sg_rpt().group_source_by_addr_begin(
		_processing_sg_rpt_source_addr_sg_sg_rpt,
		_group_addr_sg_sg_rpt);
	}
	
	for (gs_iter = gs_iter_begin; gs_iter != gs_iter_end; ) {
	    pim_mre_sg_rpt = gs_iter->second;
	    ++gs_iter;
	    
	    pim_mre_sg = pim_mre_sg_rpt->sg_entry();
	    
	    if (pim_mre_sg != NULL)
		continue;  // XXX: The (S,G,rpt) entry was already processed
	    
	    // Perform the (S,G,rpt) actions
	    perform_pim_mre_actions(pim_mre_sg_rpt);
	    
	    if (_time_slice.is_expired()) {
		// Stop processing. Save state to continue later from here.
		if (gs_iter != gs_iter_end) {
		    pim_mre_sg_rpt = gs_iter->second;
		    _processing_sg_rpt_source_addr_sg_sg_rpt
			= pim_mre_sg_rpt->source_addr();
		    _is_processing_sg_sg_rpt = true;
		    _is_processing_sg_rpt_source_addr_sg_sg_rpt = true;
		}
		return (true);
	    }
	}
	_is_processing_sg_rpt_source_addr_sg_sg_rpt = false;
	
	// Prepare the PimMfc related state
	_group_addr_mfc = _group_addr_sg_sg_rpt;
	_is_set_group_addr_mfc = true;
    }
    _is_set_source_addr_sg_sg_rpt = false;
    _is_set_group_addr_sg_sg_rpt = false;
    
    if (_is_set_source_addr_prefix_sg_sg_rpt) {
	// Actions for a set of (S,G) and/or (S,G,rpt) entries specified
	// by an unicast source address prefix.
	
	//
	// Perform the actions for all (S,G) and (S,G,rpt) entries
	//
	PimMrtSg::const_sg_iterator sg_iter, sg_iter_begin, sg_iter_end;
	
	sg_iter_end = pim_mrt().pim_mrt_sg().source_by_prefix_end(
	    _source_addr_prefix_sg_sg_rpt);
	
	if (! (_is_processing_sg_group_addr_sg_sg_rpt
	       || _is_processing_sg_rpt_group_addr_sg_sg_rpt)) {
	    sg_iter_begin = pim_mrt().pim_mrt_sg().source_by_prefix_begin(
		_source_addr_prefix_sg_sg_rpt);
	} else {
	    if (_is_processing_sg_group_addr_sg_sg_rpt) {
		sg_iter_begin = pim_mrt().pim_mrt_sg().source_group_by_addr_begin(
		    _processing_sg_source_addr_sg_sg_rpt,
		    _processing_sg_group_addr_sg_sg_rpt);
	    } else {
		// XXX: continue with the (S,G,rpt) processing late
		sg_iter_begin = sg_iter_end;
	    }
	}
	
	for (sg_iter = sg_iter_begin; sg_iter != sg_iter_end; ) {
	    pim_mre_sg = sg_iter->second;
	    ++sg_iter;
	    
	    pim_mre_sg_rpt = pim_mre_sg->sg_rpt_entry();
	    
	    // Perform the (S,G) and (S,G,rpt) actions
	    perform_pim_mre_sg_sg_rpt_actions(pim_mre_sg, pim_mre_sg_rpt);
	    
	    if (_time_slice.is_expired()) {
		// Stop processing. Save state to continue later from here.
		if (sg_iter != sg_iter_end) {
		    pim_mre_sg = sg_iter->second;
		    _processing_sg_source_addr_sg_sg_rpt
			= pim_mre_sg->source_addr();
		    _processing_sg_group_addr_sg_sg_rpt
			= pim_mre_sg->group_addr();
		    _is_processing_sg_sg_rpt = true;
		    _is_processing_sg_source_addr_sg_sg_rpt = true;
		    _is_processing_sg_group_addr_sg_sg_rpt = true;
		}
		return (true);
	    }
	}
	_is_processing_sg_source_addr_sg_sg_rpt = false;
	_is_processing_sg_group_addr_sg_sg_rpt = false;
	
	//
	// Perform the actions for only those (S,G,rpt) entries that do not
	// have a corresponding (S,G) entry.
	//
	sg_iter_end = pim_mrt().pim_mrt_sg_rpt().source_by_prefix_end(
	    _source_addr_prefix_sg_sg_rpt);
	
	if (! _is_processing_sg_rpt_group_addr_sg_sg_rpt) {
	    sg_iter_begin = pim_mrt().pim_mrt_sg_rpt().source_by_prefix_begin(
		_source_addr_prefix_sg_sg_rpt);
	} else {
	    sg_iter_begin = pim_mrt().pim_mrt_sg_rpt().source_group_by_addr_begin(
		_processing_sg_rpt_source_addr_sg_sg_rpt,
		_processing_sg_rpt_group_addr_sg_sg_rpt);
	}
	
	for (sg_iter = sg_iter_begin; sg_iter != sg_iter_end; ) {
	    pim_mre_sg_rpt = sg_iter->second;
	    ++sg_iter;
	    
	    pim_mre_sg = pim_mre_sg_rpt->sg_entry();
	    
	    if (pim_mre_sg != NULL)
		continue;    // XXX: The (S,G,rpt) entry was already processed
	    
	    // Perform the (S,G,rpt) actions
	    perform_pim_mre_actions(pim_mre_sg_rpt);
	    
	    if (_time_slice.is_expired()) {
		// Stop processing. Save state to continue later from here.
		if (sg_iter != sg_iter_end) {
		    pim_mre_sg_rpt = sg_iter->second;
		    _processing_sg_rpt_source_addr_sg_sg_rpt
			= pim_mre_sg_rpt->source_addr();
		    _processing_sg_rpt_group_addr_sg_sg_rpt
			= pim_mre_sg_rpt->group_addr();
		    _is_processing_sg_sg_rpt = true;
		    _is_processing_sg_rpt_source_addr_sg_sg_rpt = true;
		    _is_processing_sg_rpt_group_addr_sg_sg_rpt = true;
		}
		return (true);
	    }
	}
	_is_processing_sg_rpt_source_addr_sg_sg_rpt = false;
	_is_processing_sg_rpt_group_addr_sg_sg_rpt = false;
	
	// Prepare the PimMfc related state
	_source_addr_prefix_mfc = _source_addr_prefix_sg_sg_rpt;
	_is_set_source_addr_prefix_mfc = true;
    }
    _is_set_source_addr_prefix_sg_sg_rpt = false;
    
    if (_is_set_rp_addr_sg_sg_rpt) {
	// Actions for a set of (S,G) and (S,G,rpt) entries specified
	// by the RP address they match to.
	// XXX: only those entries that have no (*,G) entry are on the
	// processing list.
	
	PimRp *pim_rp;
	RpTable& rp_table = pim_node().rp_table();
	
	// Prepare the processing of the (S,G) entries for this RP address
	if (! _is_processing_rp_addr_sg) {
	    rp_table.init_processing_pim_mre_sg(_rp_addr_sg_sg_rpt);
	} else {
	    _rp_addr_sg_sg_rpt = _processing_rp_addr_sg_sg_rpt;
	}
	
	while ((pim_rp = rp_table.find_processing_pim_mre_sg(_rp_addr_sg_sg_rpt))
		!= NULL) {
	    
	    bool more_processing = !pim_rp->processing_pim_mre_sg_list().empty();
	    while (more_processing) {
		// Move the PimMre entry to the non-processing list
		PimMre *pim_mre_sg = pim_rp->processing_pim_mre_sg_list().front();
		pim_rp->processing_pim_mre_sg_list().pop_front();
		pim_rp->pim_mre_sg_list().push_back(pim_mre_sg);
		// XXX: we need to test the 'processing_pim_mre_sg_list()'
		// now in case the PimRp entry is deleted during processing.
		more_processing = !pim_rp->processing_pim_mre_sg_list().empty();
		
		_source_addr_sg_sg_rpt = pim_mre_sg->source_addr();
		
		pim_mre_sg_rpt = pim_mre_sg->sg_rpt_entry();
		
		// Perform the (S,G) and (S,G,rpt) actions
		perform_pim_mre_sg_sg_rpt_actions(pim_mre_sg, pim_mre_sg_rpt);
		
		if (_time_slice.is_expired()) {
		    // Stop processing. Save state to continue later from here.
		    if (more_processing) {
			_is_processing_sg_sg_rpt = true;
			_is_processing_rp_addr_sg = true;
			_processing_rp_addr_sg_sg_rpt = _rp_addr_sg_sg_rpt;
			// XXX: no need to keep state for the current entry
			// we are processing because the
			// PimRp::processing_pim_mre_sg_list()
			// keeps track of that.
		    }
		    return (true);
		}
	    }
	}
	_is_processing_rp_addr_sg = false;
	
	// Prepare the processing of the (S,G,rpt) entries for this RP address
	if (! _is_processing_rp_addr_sg_rpt) {
	    rp_table.init_processing_pim_mre_sg_rpt(_rp_addr_sg_sg_rpt);
	} else {
	    _rp_addr_sg_sg_rpt = _processing_rp_addr_sg_sg_rpt;
	}
	
	while ((pim_rp = rp_table.find_processing_pim_mre_sg_rpt(_rp_addr_sg_sg_rpt))
		!= NULL) {
	    
	    bool more_processing = !pim_rp->processing_pim_mre_sg_rpt_list().empty();
	    while (more_processing) {
		// Move the PimMre entry to the non-processing list
		PimMre *pim_mre_sg_rpt = pim_rp->processing_pim_mre_sg_rpt_list().front();
		pim_rp->processing_pim_mre_sg_rpt_list().pop_front();
		pim_rp->pim_mre_sg_rpt_list().push_back(pim_mre_sg_rpt);
		// XXX: we need to test the 'processing_pim_mre_sg_rpt_list()'
		// now in case the PimRp entry is deleted during processing.
		more_processing = !pim_rp->processing_pim_mre_sg_rpt_list().empty();
		
		_source_addr_sg_sg_rpt = pim_mre_sg_rpt->source_addr();
		
		// Perform the (S,G,rpt) actions only for those entries
		// that don't have (S,G) entry
		if (pim_mre_sg_rpt->sg_entry() == NULL)
		    perform_pim_mre_actions(pim_mre_sg_rpt);
		
		if (_time_slice.is_expired()) {
		    // Stop processing. Save state to continue later from here.
		    if (more_processing) {
			_is_processing_sg_sg_rpt = true;
			_is_processing_rp_addr_sg_rpt = true;
			_processing_rp_addr_sg_sg_rpt = _rp_addr_sg_sg_rpt;
			// XXX: no need to keep state for the current entry
			// we are processing because the
			// PimRp::processing_pim_mre_sg_list()
			// keeps track of that.
		    }
		    return (true);
		}
	    }
	}
	_is_processing_rp_addr_sg_rpt = false;
	
	// Prepare the PimMfc related state
	_rp_addr_mfc = _rp_addr_sg_sg_rpt;
	_is_set_rp_addr_mfc = true;
    }
    _is_set_rp_addr_sg_sg_rpt = false;
    
    if (_is_set_pim_nbr_addr_sg_sg_rpt) {
	// Actions for a set of (S,G) and (S,G,rpt) entries specified
	// by the PimNbr address they match to.
	// XXX: only those entries that have no (*,G) entry are on the
	// processing list.
	
	// Prepare the processing of the (S,G) entries for this PimNbr
	if (! _is_processing_pim_nbr_addr_sg)
	    pim_node().init_processing_pim_mre_sg(_vif_index, _pim_nbr_addr);
	
	PimNbr *pim_nbr = pim_node().find_processing_pim_mre_sg(_vif_index,
								_pim_nbr_addr);
	if (pim_nbr != NULL) {
	    
	    bool more_processing = !pim_nbr->processing_pim_mre_sg_list().empty();
	    while (more_processing) {
		// Move the PimMre entry to the non-processing list
		PimMre *pim_mre_sg = pim_nbr->processing_pim_mre_sg_list().front();
		pim_nbr->processing_pim_mre_sg_list().pop_front();
		pim_nbr->pim_mre_sg_list().push_back(pim_mre_sg);
		// XXX: we need to test the 'processing_pim_mre_sg_list()'
		// now in case the PimRp entry is deleted during processing.
		more_processing = !pim_nbr->processing_pim_mre_sg_list().empty();
		_source_addr_sg_sg_rpt = pim_mre_sg->source_addr();
		
		pim_mre_sg_rpt = pim_mre_sg->sg_rpt_entry();
		
		// Perform the (S,G) and (S,G,rpt) actions
		perform_pim_mre_sg_sg_rpt_actions(pim_mre_sg, pim_mre_sg_rpt);
		
		if (_time_slice.is_expired()) {
		    // Stop processing. Save state to continue later from here.
		    if (more_processing) {
			_is_processing_sg_sg_rpt = true;
			_is_processing_pim_nbr_addr_sg = true;
			// XXX: no need to keep state for the current entry
			// we are processing because the
			// PimNbr::processing_pim_mre_sg_list()
			// keeps track of that.
		    }
		    return (true);
		}
	    }
	}
	_is_processing_pim_nbr_addr_sg = false;

	// Prepare the processing of the (S,G,rpt) entries for this PimNbr
	if (! _is_processing_pim_nbr_addr_sg_rpt)
	    pim_node().init_processing_pim_mre_sg_rpt(_vif_index, _pim_nbr_addr);
	
	pim_nbr = pim_node().find_processing_pim_mre_sg_rpt(_vif_index,
							    _pim_nbr_addr);
	if (pim_nbr != NULL) {
	    
	    bool more_processing = !pim_nbr->processing_pim_mre_sg_rpt_list().empty();
	    while (more_processing) {
		// Move the PimMre entry to the non-processing list
		PimMre *pim_mre_sg_rpt = pim_nbr->processing_pim_mre_sg_rpt_list().front();
		pim_nbr->processing_pim_mre_sg_rpt_list().pop_front();
		pim_nbr->pim_mre_sg_rpt_list().push_back(pim_mre_sg_rpt);
		// XXX: we need to test the 'processing_pim_mre_sg_rpt_list()'
		// now in case the PimRp entry is deleted during processing.
		more_processing = !pim_nbr->processing_pim_mre_sg_rpt_list().empty();
		_source_addr_sg_sg_rpt = pim_mre_sg_rpt->source_addr();
		
		// Perform the (S,G,rpt) actions only for those entries
		// that don't have (S,G) entry
		if (pim_mre_sg_rpt->sg_entry() == NULL)
		    perform_pim_mre_actions(pim_mre_sg_rpt);
		
		if (_time_slice.is_expired()) {
		    // Stop processing. Save state to continue later from here.
		    if (more_processing) {
			_is_processing_sg_sg_rpt = true;
			_is_processing_pim_nbr_addr_sg_rpt = true;
			// XXX: no need to keep state for the current entry
			// we are processing because the
			// PimRp::processing_pim_mre_sg_list()
			// keeps track of that.
		    }
		    return (true);
		}
	    }
	}
	_is_processing_pim_nbr_addr_sg_rpt = false;
    }
    _is_set_pim_nbr_addr_sg_sg_rpt = false;

    //
    // Process the list of (S,G) PimMre entries
    //
    // TODO: is it OK always to process the (S,G) entries first, and then
    // the (S,G,rpt) entries?
    while (! _pim_mre_sg_list.empty()) {
	PimMre *pim_mre = _pim_mre_sg_list.front();
	_pim_mre_sg_list.pop_front();
	
	// Perform the (S,G) actions
	perform_pim_mre_actions(pim_mre);
	
	// Add to the list of processed entries
	_pim_mre_sg_processed_list.push_back(pim_mre);
	
	// Schedule and perform the PimMfc
	// processing (if any) for this source and multicast group.
	bool time_slice_expired = false;
	if (! (_action_list_mfc.empty())) {
	    _source_addr_mfc = pim_mre->source_addr();
	    _is_set_source_addr_mfc = true;
	    _group_addr_mfc = pim_mre->group_addr();
	    _is_set_group_addr_mfc = true;
	    if (run_task_mfc())
		time_slice_expired = true;
	}
	if (time_slice_expired || _time_slice.is_expired()) {
	    // Time slice has expired. Save state if necessary and return.
	    return (true);
	}
    }
    
    //
    // Process the list of (S,G,rpt) PimMre entries
    //
    while (! _pim_mre_sg_rpt_list.empty()) {
	PimMre *pim_mre = _pim_mre_sg_rpt_list.front();
	_pim_mre_sg_rpt_list.pop_front();
	
	// Perform the (S,G,rpt) actions
	perform_pim_mre_actions(pim_mre);
	
	// Add to the list of processed entries
	_pim_mre_sg_rpt_processed_list.push_back(pim_mre);
	
	// Schedule and perform the PimMfc
	// processing (if any) for this source and multicast group.
	bool time_slice_expired = false;
	if (! (_action_list_mfc.empty())) {
	    _source_addr_mfc = pim_mre->source_addr();
	    _is_set_source_addr_mfc = true;
	    _group_addr_mfc = pim_mre->group_addr();
	    _is_set_group_addr_mfc = true;
	    if (run_task_mfc())
		time_slice_expired = true;
	}
	if (time_slice_expired || _time_slice.is_expired()) {
	    // Time slice has expired. Save state if necessary and return.
	    return (true);
	}
    }
    
    //
    // We are done with the (S,G) and (S,G,rpt) processing
    //
    _is_processing_sg_sg_rpt = false;
    _is_processing_sg_source_addr_sg_sg_rpt = false;
    _is_processing_sg_rpt_source_addr_sg_sg_rpt = false;
    _is_processing_sg_group_addr_sg_sg_rpt = false;
    _is_processing_sg_rpt_group_addr_sg_sg_rpt = false;
    _is_processing_rp_addr_sg = false;
    _is_processing_rp_addr_sg_rpt = false;
    _is_processing_pim_nbr_addr_sg = false;
    _is_processing_pim_nbr_addr_sg_rpt = false;
    
    //
    // Schedule and perform the PimMfc processing (if any)
    //
    do {
	if (_action_list_mfc.empty()) {
	    ret_value = false;
	    break;
	}
	if (run_task_mfc()) {
	    ret_value = true;
	    break;
	}
	ret_value = false;
	break;
    } while (false);
    
    if (ret_value)
	return (ret_value);
    
    //
    // Delete the (S,G) PimMre entries that are pending deletion
    //
    while (! _pim_mre_sg_delete_list.empty()) {
	PimMre *pim_mre = _pim_mre_sg_delete_list.front();
	_pim_mre_sg_delete_list.pop_front();
	if (pim_mre->is_task_delete_done())
	    delete pim_mre;
	if (_time_slice.is_expired())
	    return (true);
    }
    
    //
    // Delete the (S,G,rpt) PimMre entries that are pending deletion
    //
    while (! _pim_mre_sg_rpt_delete_list.empty()) {
	PimMre *pim_mre = _pim_mre_sg_rpt_delete_list.front();
	_pim_mre_sg_rpt_delete_list.pop_front();
	if (pim_mre->is_task_delete_done())
	    delete pim_mre;
	if (_time_slice.is_expired())
	    return (true);
    }
    
    return (ret_value);
}

//
// Run the PimMfc related actions if any.
//
// Return true if the processing was saved for later because it was taking too
// long time, otherwise return false.
bool
PimMreTask::run_task_mfc()
{
    PimMfc *pim_mfc;
    
    if (_is_set_source_addr_mfc && _is_set_group_addr_mfc) {
	// Actions for a single PimMfc entry
	
	// XXX: for simplicity, we always perform the PimMfc
	// actions without considering the time slice, simply because in this
	// case there is no more than one entry.
	pim_mfc = pim_mrt().pim_mfc_find(_source_addr_mfc,
					 _group_addr_mfc,
					 false);
	
	// Perform the PimMfc actions
	perform_pim_mfc_actions(pim_mfc);
    }
    
    if ( (! _is_set_source_addr_mfc) && _is_set_group_addr_mfc) {
	// Actions for a set of PimMfc entries for a
	// particular multicast group address.
	
	//
	// Perform the actions for all PimMfc entries
	//
	PimMrtMfc::const_gs_iterator gs_iter, gs_iter_begin, gs_iter_end;
	
	gs_iter_end = pim_mrt().pim_mrt_mfc().group_by_addr_end(
	    _group_addr_mfc);
	
	if (! _is_processing_source_addr_mfc) {
	    gs_iter_begin = pim_mrt().pim_mrt_mfc().group_by_addr_begin(
		_group_addr_mfc);
	} else {
	    gs_iter_begin = pim_mrt().pim_mrt_mfc().group_source_by_addr_begin(
		_processing_source_addr_mfc,
		_group_addr_mfc);
	}
	
	for (gs_iter = gs_iter_begin; gs_iter != gs_iter_end; ) {
	    pim_mfc = gs_iter->second;
	    ++gs_iter;
	    
	    // Perform the PimMfc actions
	    perform_pim_mfc_actions(pim_mfc);
	    
	    if (_time_slice.is_expired()) {
		// Stop processing. Save state to continue later from here.
		if (gs_iter != gs_iter_end) {
		    pim_mfc = gs_iter->second;
		    _processing_source_addr_mfc
			= pim_mfc->source_addr();
		    _is_processing_mfc = true;
		    _is_processing_source_addr_mfc = true;
		}
		return (true);
	    }
	}
	_is_processing_source_addr_mfc = false;
    }
    _is_set_source_addr_mfc = false;
    _is_set_group_addr_mfc = false;
    
    if (_is_set_source_addr_prefix_mfc) {
	// Actions for a set of PimMfc entries specified
	// by an unicast source address prefix.
	
	//
	// Perform the actions for all PimMfc entries
	//
	PimMrtMfc::const_sg_iterator sg_iter, sg_iter_begin, sg_iter_end;
	
	sg_iter_end = pim_mrt().pim_mrt_mfc().source_by_prefix_end(
	    _source_addr_prefix_mfc);
	
	if (! _is_processing_group_addr_mfc) {
	    sg_iter_begin = pim_mrt().pim_mrt_mfc().source_by_prefix_begin(
		_source_addr_prefix_mfc);
	} else {
	    sg_iter_begin = pim_mrt().pim_mrt_mfc().source_group_by_addr_begin(
		    _processing_source_addr_mfc,
		    _processing_group_addr_mfc);
	}
	
	for (sg_iter = sg_iter_begin; sg_iter != sg_iter_end; ) {
	    pim_mfc = sg_iter->second;
	    ++sg_iter;
	    
	    // Perform the PimMfc actions
	    perform_pim_mfc_actions(pim_mfc);
	    
	    if (_time_slice.is_expired()) {
		// Stop processing. Save state to continue later from here.
		if (sg_iter != sg_iter_end) {
		    pim_mfc = sg_iter->second;
		    _processing_source_addr_mfc
			= pim_mfc->source_addr();
		    _processing_group_addr_mfc
			= pim_mfc->group_addr();
		    _is_processing_mfc = true;
		    _is_processing_source_addr_mfc = true;
		    _is_processing_group_addr_mfc = true;
		}
		return (true);
	    }
	}
	_is_processing_source_addr_mfc = false;
	_is_processing_group_addr_mfc = false;
    }
    _is_set_source_addr_prefix_mfc = false;

    if (_is_set_rp_addr_mfc) {
	// Actions for a set of PimMfc entries specified
	// by the RP address they match to.
	// XXX: all entries are on the processing list.
	
	PimRp *pim_rp;
	RpTable& rp_table = pim_node().rp_table();
	
	// Prepare the processing of the PimMfc entries for this RP address
	if (! _is_processing_rp_addr_mfc) {
	    rp_table.init_processing_pim_mfc(_rp_addr_mfc);
	} else {
	    _rp_addr_mfc = _processing_rp_addr_mfc;
	}
	
	while ((pim_rp = rp_table.find_processing_pim_mfc(_rp_addr_mfc))
		!= NULL) {
	    
	    bool more_processing = !pim_rp->processing_pim_mfc_list().empty();
	    while (more_processing) {
		// Move the PimMfc entry to the non-processing list
		PimMfc *pim_mfc = pim_rp->processing_pim_mfc_list().front();
		pim_rp->processing_pim_mfc_list().pop_front();
		pim_rp->pim_mfc_list().push_back(pim_mfc);
		// XXX: we need to test the 'processing_pim_mfc_list()'
		// now in case the PimRp entry is deleted during processing.
		more_processing = !pim_rp->processing_pim_mfc_list().empty();
		
		_source_addr_mfc = pim_mfc->source_addr();
		
		// Perform the PimMfc actions
		perform_pim_mfc_actions(pim_mfc);
		
		if (_time_slice.is_expired()) {
		    // Stop processing. Save state to continue later from here.
		    if (more_processing) {
			_is_processing_mfc = true;
			_is_processing_rp_addr_mfc = true;
			_processing_rp_addr_mfc = _rp_addr_mfc;
			// XXX: no need to keep state for the current entry
			// we are processing because the
			// PimRp::processing_pim_mfc_list()
			// keeps track of that.
		    }
		    return (true);
		}
	    }
	}
	_is_processing_rp_addr_mfc = false;
    }
    _is_set_rp_addr_mfc = false;
    
    //
    // Process the list of PimMfc entries
    //
    while (! _pim_mfc_list.empty()) {
	PimMfc *pim_mfc = _pim_mfc_list.front();
	_pim_mfc_list.pop_front();
	
	// Perform the PimMfc actions
	perform_pim_mfc_actions(pim_mfc);
	
	// Add to the list of processed entries
	_pim_mfc_processed_list.push_back(pim_mfc);
	
	if (_time_slice.is_expired()) {
	    // Time slice has expired. Save state if necessary and return.
	    return (true);
	}
    }
    
    //
    // We are done with the PimMfc processing
    //
    _is_processing_mfc = false;
    _is_processing_source_addr_mfc = false;
    _is_processing_group_addr_mfc = false;
    _is_processing_rp_addr_mfc = false;
    
    //
    // Delete the PimMfc entries that are pending deletion
    //
    while (! _pim_mfc_delete_list.empty()) {
	PimMfc *pim_mfc = _pim_mfc_delete_list.front();
	_pim_mfc_delete_list.pop_front();
	if (pim_mfc->is_task_delete_done())
	    delete pim_mfc;
	if (_time_slice.is_expired())
	    return (true);
    }
    
    return (false);
}

//
// Perform the scheduled actions for a PimMre entry.
//
void
PimMreTask::perform_pim_mre_actions(PimMre *pim_mre)
{
    list<PimMreAction>::iterator action_iter;
    
    if (pim_mre == NULL)
	return;
    
    if (pim_mre->is_rp()) {
	for (action_iter = _action_list_rp.begin();
	     action_iter != _action_list_rp.end();
	     ++action_iter) {
	    PimMreAction action = *action_iter;
	    action.perform_action(*pim_mre, _vif_index, _addr_arg);
	}
	return;
    }
    
    if (pim_mre->is_wc()) {
	for (action_iter = _action_list_wc.begin();
	     action_iter != _action_list_wc.end();
	     ++action_iter) {
	    PimMreAction action = *action_iter;
	    action.perform_action(*pim_mre, _vif_index, _addr_arg);
	}
	return;
    }
    
    if (pim_mre->is_sg()) {
	for (action_iter = _action_list_sg_sg_rpt.begin();
	     action_iter != _action_list_sg_sg_rpt.end();
	     ++action_iter) {
	    PimMreAction action = *action_iter;
	    if (action.is_sg())
		action.perform_action(*pim_mre, _vif_index, _addr_arg);
	}
	return;
    }
    
    if (pim_mre->is_sg_rpt()) {
	for (action_iter = _action_list_sg_sg_rpt.begin();
	     action_iter != _action_list_sg_sg_rpt.end();
	     ++action_iter) {
	    PimMreAction action = *action_iter;
	    if (action.is_sg_rpt())
		action.perform_action(*pim_mre, _vif_index, _addr_arg);
	}
	return;
    }
}

//
// Perform the (S,G) and (S,G,rpt) actions
//
void
PimMreTask::perform_pim_mre_sg_sg_rpt_actions(PimMre *pim_mre_sg,
					      PimMre *pim_mre_sg_rpt)
{
    list<PimMreAction>::iterator action_iter;
    for (action_iter = _action_list_sg_sg_rpt.begin();
	 action_iter != _action_list_sg_sg_rpt.end();
	 ++action_iter) {
	PimMreAction action = *action_iter;
	if (action.is_sg()) {
	    if (pim_mre_sg != NULL)
		action.perform_action(*pim_mre_sg, _vif_index, _addr_arg);
	} else if (action.is_sg_rpt()) {
	    if (pim_mre_sg_rpt != NULL)
		action.perform_action(*pim_mre_sg_rpt, _vif_index, _addr_arg);
	}
    }
}

//
// Perform the scheduled actions for a PimMfc entry.
//
void
PimMreTask::perform_pim_mfc_actions(PimMfc *pim_mfc)
{
    list<PimMreAction>::iterator action_iter;
    
    if (pim_mfc == NULL)
	return;
    
    for (action_iter = _action_list_mfc.begin();
	 action_iter != _action_list_mfc.end();
	 ++action_iter) {
	PimMreAction action = *action_iter;
	action.perform_action(*pim_mfc);
    }
}

//
// Perform the scheduled actions for a PimMfc entry specified by
// source and group address.
//
void
PimMreTask::perform_pim_mfc_actions(const IPvX& source_addr,
				    const IPvX& group_addr)
{
    PimMfc *pim_mfc;
    
    pim_mfc = pim_mrt().pim_mfc_find(source_addr, group_addr, false);
    
    perform_pim_mfc_actions(pim_mfc);
}

//
// Add a PimMre entry to the appropriate list of entries to process
//
void
PimMreTask::add_pim_mre(PimMre *pim_mre)
{
    if (pim_mre->is_rp()) {
	_pim_mre_rp_list.push_back(pim_mre);
	return;
    }
    if (pim_mre->is_wc()) {
	_pim_mre_wc_list.push_back(pim_mre);
	return;
    }
    if (pim_mre->is_sg()) {
	_pim_mre_sg_list.push_back(pim_mre);
	return;
    }
    if (pim_mre->is_sg_rpt()) {
	_pim_mre_sg_rpt_list.push_back(pim_mre);
	return;
    }
}

//
// Add a PimMre entry to the list of entries to delete
//
void
PimMreTask::add_pim_mre_delete(PimMre *pim_mre)
{
    if (pim_mre->is_rp()) {
	_pim_mre_rp_delete_list.push_back(pim_mre);
	return;
    }
    if (pim_mre->is_wc()) {
	_pim_mre_wc_delete_list.push_back(pim_mre);
	return;
    }
    if (pim_mre->is_sg()) {
	_pim_mre_sg_delete_list.push_back(pim_mre);
	return;
    }
    if (pim_mre->is_sg_rpt()) {
	_pim_mre_sg_rpt_delete_list.push_back(pim_mre);
	return;
    }
}

//
// Add a PimMfc entry to the appropriate list of entries to process
//
void
PimMreTask::add_pim_mfc(PimMfc *pim_mfc)
{
    _pim_mfc_list.push_back(pim_mfc);
}

//
// Add a PimMfc entry to the list of entries to delete
//
void
PimMreTask::add_pim_mfc_delete(PimMfc *pim_mfc)
{
    _pim_mfc_delete_list.push_back(pim_mfc);
}

//
// A timeout handler to continue processing a task.
//
void
PimMreTask::time_slice_timer_timeout()
{
    run_task();
}
