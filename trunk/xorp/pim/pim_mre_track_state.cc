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

#ident "$XORP: xorp/pim/pim_mre_track_state.cc,v 1.11 2003/02/11 08:13:18 pavlin Exp $"

//
// PIM Multicast Routing Entry state tracking
//


#include "pim_module.h"
#include "pim_private.hh"

#include <algorithm>

#include "pim_mfc.hh"
#include "pim_mre_track_state.hh"
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


PimMreTrackState::PimMreTrackState(PimMrt& pim_mrt)
    : _pim_mrt(pim_mrt)
{
    list<PimMreAction> action_list;

    //
    // Init the state dependencies
    //
    output_state_rp_wc(action_list);
    output_state_rp_sg(action_list);
    output_state_rp_sg_rpt(action_list);
    output_state_rp_mfc(action_list);
    output_state_mrib_rp_rp(action_list);
    output_state_mrib_rp_wc(action_list);
    output_state_mrib_rp_sg(action_list);
    output_state_mrib_rp_sg_rpt(action_list);
    output_state_mrib_s_sg(action_list);
    output_state_mrib_s_sg_rpt(action_list);
    output_state_is_join_desired_rp(action_list);
    output_state_is_join_desired_wc(action_list);
    output_state_is_join_desired_sg(action_list);
    output_state_is_prune_desired_sg_rpt(action_list);
    output_state_is_prune_desired_sg_rpt_sg(action_list);
    output_state_is_rpt_join_desired_g(action_list);
    output_state_inherited_olist_sg_rpt(action_list);
    output_state_iif_olist_mfc(action_list);
    output_state_is_could_register_sg(action_list);
    output_state_assert_tracking_desired_sg(action_list);
    output_state_assert_tracking_desired_wc(action_list);
    output_state_could_assert_sg(action_list);
    output_state_could_assert_wc(action_list);
    output_state_my_assert_metric_sg(action_list);
    output_state_my_assert_metric_wc(action_list);
    output_state_assert_rpf_interface_sg(action_list);
    output_state_assert_rpf_interface_wc(action_list);
    output_state_assert_receive_join_sg(action_list);
    output_state_assert_receive_join_wc(action_list);
    output_state_assert_winner_nbr_sg_gen_id(action_list);
    output_state_assert_winner_nbr_wc_gen_id(action_list);
    output_state_receive_join_wc_by_sg_rpt(action_list);
    output_state_receive_end_of_message_sg_rpt(action_list);
    output_state_sg_see_prune_wc(action_list);
    output_state_check_switch_to_spt_sg(action_list);
    output_state_rpfp_nbr_wc(action_list);
    output_state_rpfp_nbr_wc_gen_id(action_list);
    output_state_rpfp_nbr_sg(action_list);
    output_state_rpfp_nbr_sg_gen_id(action_list);
    output_state_rpfp_nbr_sg_rpt(action_list);
    output_state_rpfp_nbr_sg_rpt_sg(action_list);
    output_state_mrib_next_hop_rp(action_list);
    output_state_mrib_next_hop_rp_gen_id(action_list);
    output_state_mrib_next_hop_rp_g(action_list);
    output_state_mrib_next_hop_s(action_list);
    output_state_out_start_vif_rp(action_list);
    output_state_out_start_vif_wc(action_list);
    output_state_out_start_vif_sg(action_list);
    output_state_out_start_vif_sg_rpt(action_list);
    output_state_out_stop_vif_rp(action_list);
    output_state_out_stop_vif_wc(action_list);
    output_state_out_stop_vif_sg(action_list);
    output_state_out_stop_vif_sg_rpt(action_list);
    output_state_out_add_pim_mre_rp_entry_rp(action_list);
    output_state_out_add_pim_mre_rp_entry_wc(action_list);
    output_state_out_add_pim_mre_rp_entry_sg(action_list);
    output_state_out_add_pim_mre_rp_entry_sg_rpt(action_list);
    output_state_out_add_pim_mre_wc_entry_wc(action_list);
    output_state_out_add_pim_mre_wc_entry_sg(action_list);
    output_state_out_add_pim_mre_wc_entry_sg_rpt(action_list);
    output_state_out_add_pim_mre_sg_entry_sg(action_list);
    output_state_out_add_pim_mre_sg_entry_sg_rpt(action_list);
    output_state_out_add_pim_mre_sg_rpt_entry_sg(action_list);
    output_state_out_add_pim_mre_sg_rpt_entry_sg_rpt(action_list);
    output_state_out_remove_pim_mre_rp_entry_rp(action_list);
    output_state_out_remove_pim_mre_rp_entry_wc(action_list);
    output_state_out_remove_pim_mre_rp_entry_sg(action_list);
    output_state_out_remove_pim_mre_rp_entry_sg_rpt(action_list);
    output_state_out_remove_pim_mre_wc_entry_wc(action_list);
    output_state_out_remove_pim_mre_wc_entry_sg(action_list);
    output_state_out_remove_pim_mre_wc_entry_sg_rpt(action_list);
    output_state_out_remove_pim_mre_sg_entry_sg(action_list);
    output_state_out_remove_pim_mre_sg_entry_sg_rpt(action_list);
    output_state_out_remove_pim_mre_sg_rpt_entry_sg(action_list);
    output_state_out_remove_pim_mre_sg_rpt_entry_sg_rpt(action_list);
    
    //
    // Create the final result
    //
    for (int i = 0; i < INPUT_STATE_MAX; i++) {
	input_state_t input_state = static_cast<input_state_t>(i);
	list<PimMreAction> action_list;
	action_list = _action_lists[input_state].compute_action_list();
	action_list = remove_state(action_list);
	_action_lists[input_state].clear();
	list<PimMreAction>::iterator iter;
	for (iter = action_list.begin(); iter != action_list.end(); ++iter) {
	    PimMreAction action = *iter;
	    add_action(input_state, action);
	}
    }
}

PimNode&
PimMreTrackState::pim_node() const
{
    return (_pim_mrt.pim_node());
}

int
PimMreTrackState::family() const
{
    return (_pim_mrt.family());
}

// Return %XORP_OK if the action list has been added, otherwise return %XORP_ERROR.
int
PimMreTrackState::add_action_list(input_state_t input_state,
				  list<PimMreAction> action_list)
{
    int ret_value = XORP_ERROR;
    
    if (input_state >= INPUT_STATE_MAX)
	return (XORP_ERROR);
    
    _action_lists[input_state].add_action_list(action_list);
    ret_value = XORP_OK;
    
    return (ret_value);
}

void
PimMreTrackState::ActionLists::clear()
{
    _action_list_vector.clear();
}

void
PimMreTrackState::ActionLists::add_action_list(list<PimMreAction> action_list)
{
    _action_list_vector.push_back(action_list);
}

list<PimMreAction>
PimMreTrackState::ActionLists::compute_action_list()
{
    list<PimMreAction> action_list;
    
    //
    // Remove the duplicates (that follow one-after another), and reverse
    // the action ordering
    //
    for (size_t i = 0; i < _action_list_vector.size(); i++) {
	list<PimMreAction>& l = _action_list_vector[i];
	l.erase(unique(l.begin(), l.end()), l.end());
	l.reverse();		// XXX
    }
    
    //
    // Get the actions
    //
    do {
	PimMreAction action = pop_next_action();
	if (action.output_state() == OUTPUT_STATE_MAX)
	    break;
	action_list.push_back(action);
    } while (true);
    
    //
    // Check if all actions were used
    //
    for (size_t i = 0; i < _action_list_vector.size(); i++) {
	list<PimMreAction>& l = _action_list_vector[i];
	if (! l.empty()) {
	    XLOG_FATAL("PimMreTrackState machinery: incomplete action set");
	    exit (1);
	}
    }
    
    return (action_list);
}

//
// Return an action that is only in the head of one or more lists
//
PimMreAction
PimMreTrackState::ActionLists::pop_next_action()
{
    PimMreAction invalid_action(OUTPUT_STATE_MAX, PIM_MRE_RP);
    
    for (size_t i = 0; i < _action_list_vector.size(); i++) {
	list<PimMreAction>& l = _action_list_vector[i];
	if (l.empty())
	    continue;
	PimMreAction action = l.front();
	if (! is_head_only_action(action))
	    continue;
	//
	// Found an action. Remove it from the head of all lists
	//
	for (size_t j = 0; j < _action_list_vector.size(); j++) {
	    list<PimMreAction>& l2 = _action_list_vector[j];
	    if (l2.empty())
		continue;
	    PimMreAction action2 = l2.front();
	    if (action == action2)
		l2.pop_front();
	}
	return (action);
    }
    
    return (invalid_action);
}

//
// Test if this action is only in the head of some lists
//
bool
PimMreTrackState::ActionLists::is_head_only_action(const PimMreAction& action) const
{
    for (size_t i = 0; i < _action_list_vector.size(); i++) {
	const list<PimMreAction>& l = _action_list_vector[i];
	if (l.size() <= 1)
	    continue;
	list<PimMreAction>::const_iterator iter;
	iter = l.begin();
	++iter;
	if (find(iter, l.end(), action) != l.end())
	    return (false);
    }
    
    return (true);
}

// Return %XORP_OK if the action has been added, otherwise return %XORP_ERROR.
int
PimMreTrackState::add_action(input_state_t input_state,
			     const PimMreAction& action)
{
    int ret_value = XORP_ERROR;
    
    if (input_state >= INPUT_STATE_MAX)
	return (XORP_ERROR);
    
    if (action.is_sg() || action.is_sg_rpt()) {
	// (S,G) or (S,G,rpt) action
	list<PimMreAction>& action_list = _output_action_sg_sg_rpt[input_state];
	if (! can_add_action_to_list(action_list, action)) {
	    return (XORP_ERROR);
	}
	action_list.push_back(action);
	ret_value = XORP_OK;
    }
    
    if (action.is_wc()) {
	// (*,G) action
	list<PimMreAction>& action_list = _output_action_wc[input_state];
	if (! can_add_action_to_list(action_list, action)) {
	    return (XORP_ERROR);
	}
	action_list.push_back(action);
	ret_value = XORP_OK;
    }

    if (action.is_rp()) {
	// (*,*,RP) action
	list<PimMreAction>& action_list = _output_action_rp[input_state];
	if (! can_add_action_to_list(action_list, action)) {
	    return (XORP_ERROR);
	}
	action_list.push_back(action);
	ret_value = XORP_OK;
    }
    
    if (action.is_mfc()) {
	// MFC action
	list<PimMreAction>& action_list = _output_action_mfc[input_state];
	if (! can_add_action_to_list(action_list, action)) {
	    return (XORP_ERROR);
	}
	action_list.push_back(action);
	ret_value = XORP_OK;
    }
    
    if (ret_value == XORP_OK) {
	// Rebuild the list of all actions:
	// - The (*,*,RP) actions are first.
	// - The (*,G) actions follow the (*,*,RP) actions.
	// - The (S,G) and (S,G,rpt) actions (in order of appearance)
	//   follow the (*,G) actions.
	
	_output_action[input_state].clear();
	
	_output_action[input_state].insert(
	    _output_action[input_state].end(),
	    _output_action_rp[input_state].begin(),
	    _output_action_rp[input_state].end());

	_output_action[input_state].insert(
	    _output_action[input_state].end(),
	    _output_action_wc[input_state].begin(),
	    _output_action_wc[input_state].end());

	_output_action[input_state].insert(
	    _output_action[input_state].end(),
	    _output_action_sg_sg_rpt[input_state].begin(),
	    _output_action_sg_sg_rpt[input_state].end());

	_output_action[input_state].insert(
	    _output_action[input_state].end(),
	    _output_action_mfc[input_state].begin(),
	    _output_action_mfc[input_state].end());
    }
    
    return (ret_value);
}

// Return true if the action can be added to the list, otherwise return false
bool
PimMreTrackState::can_add_action_to_list(const list<PimMreAction>& action_list,
					 const PimMreAction& action) const
{
    if (action_list.empty())
	return (true);
    
    const PimMreAction& last_action = action_list.back();
    if (last_action != action)
	return (true);
    
    return (false);
}

/**
 * PimMreTrackState::remove_action_from_list:
 * @action_list: The list of actions.
 * @keep_action: The action that defines whether @remove_action should be
 * removed.
 * @remove_action: The action to remove if it occurs after @keep_action.
 * 
 * Remove action @remove action from the list of actions @action_list,
 * but only if action @keep_action is placed earlier on that list.
 * If necessary, each occurance of @remove_action is removed.
 * 
 * Return value: The modified list after/if @remove_action is removed.
 **/
list<PimMreAction>
PimMreTrackState::remove_action_from_list(list<PimMreAction> action_list,
					  PimMreAction keep_action,
					  PimMreAction remove_action)
{
    list<PimMreAction>::iterator iter;
    
    iter = find(action_list.begin(), action_list.end(), keep_action);
    if (iter == action_list.end())
	return (action_list);
    
    do {
	list<PimMreAction>::iterator iter2;
	
	iter2 = find(iter, action_list.end(), remove_action);
	if (iter2 == action_list.end())
	    break;
	action_list.erase(iter2);
    } while (true);
    
    return (action_list);
}

void
PimMreTrackState::print_actions_name() const
{
    vector<string> input_state_names(INPUT_STATE_MAX);
    vector<string> output_state_names(OUTPUT_STATE_MAX);

#define INPUT_NAME(enum_name)						\
do {									\
	input_state_names[enum_name] = #enum_name;			\
} while (false)
#define OUTPUT_NAME(enum_name)						\
do {									\
	output_state_names[enum_name] = #enum_name;			\
} while (false)
    
    INPUT_NAME(INPUT_STATE_RP_CHANGED);				// 0
    INPUT_NAME(INPUT_STATE_MRIB_RP_CHANGED);			// 1
    INPUT_NAME(INPUT_STATE_MRIB_S_CHANGED);			// 2
    INPUT_NAME(INPUT_STATE_MRIB_NEXT_HOP_RP_CHANGED);		// 3
    INPUT_NAME(INPUT_STATE_MRIB_NEXT_HOP_RP_GEN_ID_CHANGED);	// 4
    INPUT_NAME(INPUT_STATE_MRIB_NEXT_HOP_RP_G_CHANGED);		// 5
    INPUT_NAME(INPUT_STATE_MRIB_NEXT_HOP_S_CHANGED);		// 6
    INPUT_NAME(INPUT_STATE_RPFP_NBR_WC_CHANGED);		// 7
    INPUT_NAME(INPUT_STATE_RPFP_NBR_WC_GEN_ID_CHANGED);		// 8
    INPUT_NAME(INPUT_STATE_RPFP_NBR_SG_CHANGED);		// 9
    INPUT_NAME(INPUT_STATE_RPFP_NBR_SG_GEN_ID_CHANGED);		// 10
    INPUT_NAME(INPUT_STATE_RPFP_NBR_SG_RPT_CHANGED);		// 11
    INPUT_NAME(INPUT_STATE_RECEIVE_JOIN_RP);			// 12
    INPUT_NAME(INPUT_STATE_RECEIVE_JOIN_WC);			// 13
    INPUT_NAME(INPUT_STATE_RECEIVE_JOIN_SG);			// 14
    INPUT_NAME(INPUT_STATE_RECEIVE_JOIN_SG_RPT);		// 15
    INPUT_NAME(INPUT_STATE_RECEIVE_PRUNE_RP);			// 16
    INPUT_NAME(INPUT_STATE_RECEIVE_PRUNE_WC);			// 17
    INPUT_NAME(INPUT_STATE_RECEIVE_PRUNE_SG);			// 18
    INPUT_NAME(INPUT_STATE_RECEIVE_PRUNE_SG_RPT);		// 19
    INPUT_NAME(INPUT_STATE_RECEIVE_END_OF_MESSAGE_SG_RPT);	// 20
    INPUT_NAME(INPUT_STATE_SEE_PRUNE_WC);			// 21
    INPUT_NAME(INPUT_STATE_DOWNSTREAM_JP_STATE_RP);		// 22
    INPUT_NAME(INPUT_STATE_DOWNSTREAM_JP_STATE_WC);		// 23
    INPUT_NAME(INPUT_STATE_DOWNSTREAM_JP_STATE_SG);		// 24
    INPUT_NAME(INPUT_STATE_DOWNSTREAM_JP_STATE_SG_RPT);		// 25
    INPUT_NAME(INPUT_STATE_UPSTREAM_JP_STATE_SG);		// 26
    INPUT_NAME(INPUT_STATE_LOCAL_RECEIVER_INCLUDE_WC);		// 27
    INPUT_NAME(INPUT_STATE_LOCAL_RECEIVER_INCLUDE_SG);		// 28
    INPUT_NAME(INPUT_STATE_LOCAL_RECEIVER_EXCLUDE_SG);		// 29
    INPUT_NAME(INPUT_STATE_ASSERT_STATE_WC);			// 30
    INPUT_NAME(INPUT_STATE_ASSERT_STATE_SG);			// 31
    INPUT_NAME(INPUT_STATE_ASSERT_WINNER_NBR_WC_GEN_ID_CHANGED);// 32
    INPUT_NAME(INPUT_STATE_ASSERT_WINNER_NBR_SG_GEN_ID_CHANGED);// 33
    INPUT_NAME(INPUT_STATE_ASSERT_RPF_INTERFACE_WC_CHANGED);	// 34
    INPUT_NAME(INPUT_STATE_ASSERT_RPF_INTERFACE_SG_CHANGED);	// 35
    INPUT_NAME(INPUT_STATE_I_AM_DR);				// 36
    INPUT_NAME(INPUT_STATE_MY_IP_ADDRESS);			// 37
    INPUT_NAME(INPUT_STATE_MY_IP_SUBNET_ADDRESS);		// 38
    INPUT_NAME(INPUT_STATE_IS_SWITCH_TO_SPT_DESIRED_SG);	// 39
    INPUT_NAME(INPUT_STATE_KEEPALIVE_TIMER_SG);			// 40
    INPUT_NAME(INPUT_STATE_SPTBIT_SG);				// 41
    INPUT_NAME(INPUT_STATE_IN_START_VIF);			// 42
    INPUT_NAME(INPUT_STATE_IN_STOP_VIF);			// 43
    INPUT_NAME(INPUT_STATE_IN_ADD_PIM_MRE_RP);			// 44
    INPUT_NAME(INPUT_STATE_IN_ADD_PIM_MRE_WC);			// 45
    INPUT_NAME(INPUT_STATE_IN_ADD_PIM_MRE_SG);			// 46
    INPUT_NAME(INPUT_STATE_IN_ADD_PIM_MRE_SG_RPT);		// 47
    INPUT_NAME(INPUT_STATE_IN_REMOVE_PIM_MRE_RP);		// 48
    INPUT_NAME(INPUT_STATE_IN_REMOVE_PIM_MRE_WC);		// 49
    INPUT_NAME(INPUT_STATE_IN_REMOVE_PIM_MRE_SG);		// 50
    INPUT_NAME(INPUT_STATE_IN_REMOVE_PIM_MRE_SG_RPT);		// 51
    
    OUTPUT_NAME(OUTPUT_STATE_RP_WC);				// 0
    OUTPUT_NAME(OUTPUT_STATE_RP_SG);				// 1
    OUTPUT_NAME(OUTPUT_STATE_RP_SG_RPT);			// 2
    OUTPUT_NAME(OUTPUT_STATE_RP_MFC);				// 3
    OUTPUT_NAME(OUTPUT_STATE_MRIB_RP_RP);			// 4
    OUTPUT_NAME(OUTPUT_STATE_MRIB_RP_WC);			// 5
    OUTPUT_NAME(OUTPUT_STATE_MRIB_RP_SG);			// 6
    OUTPUT_NAME(OUTPUT_STATE_MRIB_RP_SG_RPT);			// 7
    OUTPUT_NAME(OUTPUT_STATE_MRIB_S_SG);			// 8
    OUTPUT_NAME(OUTPUT_STATE_MRIB_S_SG_RPT);			// 9
    OUTPUT_NAME(OUTPUT_STATE_IS_JOIN_DESIRED_RP);		// 10
    OUTPUT_NAME(OUTPUT_STATE_IS_JOIN_DESIRED_WC);		// 11
    OUTPUT_NAME(OUTPUT_STATE_IS_JOIN_DESIRED_SG);		// 12
    OUTPUT_NAME(OUTPUT_STATE_IS_PRUNE_DESIRED_SG_RPT);		// 13
    OUTPUT_NAME(OUTPUT_STATE_IS_PRUNE_DESIRED_SG_RPT_SG);	// 14
    OUTPUT_NAME(OUTPUT_STATE_IS_RPT_JOIN_DESIRED_G);		// 15
    OUTPUT_NAME(OUTPUT_STATE_INHERITED_OLIST_SG_RPT);		// 16
    OUTPUT_NAME(OUTPUT_STATE_IIF_OLIST_MFC);			// 17
    OUTPUT_NAME(OUTPUT_STATE_IS_COULD_REGISTER_SG);		// 18
    OUTPUT_NAME(OUTPUT_STATE_ASSERT_TRACKING_DESIRED_SG);	// 19
    OUTPUT_NAME(OUTPUT_STATE_ASSERT_TRACKING_DESIRED_WC);	// 20
    OUTPUT_NAME(OUTPUT_STATE_COULD_ASSERT_SG);			// 21
    OUTPUT_NAME(OUTPUT_STATE_COULD_ASSERT_WC);			// 22
    OUTPUT_NAME(OUTPUT_STATE_MY_ASSERT_METRIC_SG);		// 23
    OUTPUT_NAME(OUTPUT_STATE_MY_ASSERT_METRIC_WC);		// 24
    OUTPUT_NAME(OUTPUT_STATE_ASSERT_RPF_INTERFACE_SG);		// 25
    OUTPUT_NAME(OUTPUT_STATE_ASSERT_RPF_INTERFACE_WC);		// 26
    OUTPUT_NAME(OUTPUT_STATE_ASSERT_RECEIVE_JOIN_SG);		// 27
    OUTPUT_NAME(OUTPUT_STATE_ASSERT_RECEIVE_JOIN_WC);		// 28
    OUTPUT_NAME(OUTPUT_STATE_ASSERT_WINNER_NBR_SG_GEN_ID);	// 29
    OUTPUT_NAME(OUTPUT_STATE_ASSERT_WINNER_NBR_WC_GEN_ID);	// 30
    OUTPUT_NAME(OUTPUT_STATE_RECEIVE_JOIN_WC_BY_SG_RPT);	// 31
    OUTPUT_NAME(OUTPUT_STATE_RECEIVE_END_OF_MESSAGE_SG_RPT);	// 32
    OUTPUT_NAME(OUTPUT_STATE_SG_SEE_PRUNE_WC);			// 33
    OUTPUT_NAME(OUTPUT_STATE_CHECK_SWITCH_TO_SPT_SG);		// 34
    OUTPUT_NAME(OUTPUT_STATE_RPFP_NBR_WC);			// 35
    OUTPUT_NAME(OUTPUT_STATE_RPFP_NBR_WC_GEN_ID);		// 36
    OUTPUT_NAME(OUTPUT_STATE_RPFP_NBR_SG);			// 37
    OUTPUT_NAME(OUTPUT_STATE_RPFP_NBR_SG_GEN_ID);		// 38
    OUTPUT_NAME(OUTPUT_STATE_RPFP_NBR_SG_RPT);			// 39
    OUTPUT_NAME(OUTPUT_STATE_RPFP_NBR_SG_RPT_SG);		// 40
    OUTPUT_NAME(OUTPUT_STATE_MRIB_NEXT_HOP_RP);			// 41
    OUTPUT_NAME(OUTPUT_STATE_MRIB_NEXT_HOP_RP_GEN_ID);		// 42
    OUTPUT_NAME(OUTPUT_STATE_MRIB_NEXT_HOP_RP_G);		// 43
    OUTPUT_NAME(OUTPUT_STATE_MRIB_NEXT_HOP_S);			// 44
    OUTPUT_NAME(OUTPUT_STATE_OUT_START_VIF_RP);			// 45
    OUTPUT_NAME(OUTPUT_STATE_OUT_START_VIF_WC);			// 46
    OUTPUT_NAME(OUTPUT_STATE_OUT_START_VIF_SG);			// 47
    OUTPUT_NAME(OUTPUT_STATE_OUT_START_VIF_SG_RPT);		// 48
    OUTPUT_NAME(OUTPUT_STATE_OUT_STOP_VIF_RP);			// 49
    OUTPUT_NAME(OUTPUT_STATE_OUT_STOP_VIF_WC);			// 50
    OUTPUT_NAME(OUTPUT_STATE_OUT_STOP_VIF_SG);			// 51
    OUTPUT_NAME(OUTPUT_STATE_OUT_STOP_VIF_SG_RPT);		// 52
    OUTPUT_NAME(OUTPUT_STATE_OUT_ADD_PIM_MRE_RP_ENTRY_RP);	// 53
    OUTPUT_NAME(OUTPUT_STATE_OUT_ADD_PIM_MRE_RP_ENTRY_WC);	// 54
    OUTPUT_NAME(OUTPUT_STATE_OUT_ADD_PIM_MRE_RP_ENTRY_SG);	// 55
    OUTPUT_NAME(OUTPUT_STATE_OUT_ADD_PIM_MRE_RP_ENTRY_SG_RPT);	// 56
    OUTPUT_NAME(OUTPUT_STATE_OUT_ADD_PIM_MRE_WC_ENTRY_WC);	// 57
    OUTPUT_NAME(OUTPUT_STATE_OUT_ADD_PIM_MRE_WC_ENTRY_SG);	// 58
    OUTPUT_NAME(OUTPUT_STATE_OUT_ADD_PIM_MRE_WC_ENTRY_SG_RPT);	// 59
    OUTPUT_NAME(OUTPUT_STATE_OUT_ADD_PIM_MRE_SG_ENTRY_SG);	// 60
    OUTPUT_NAME(OUTPUT_STATE_OUT_ADD_PIM_MRE_SG_ENTRY_SG_RPT);	// 61
    OUTPUT_NAME(OUTPUT_STATE_OUT_ADD_PIM_MRE_SG_RPT_ENTRY_SG);	// 62
    OUTPUT_NAME(OUTPUT_STATE_OUT_ADD_PIM_MRE_SG_RPT_ENTRY_SG_RPT);// 63
    OUTPUT_NAME(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_RP_ENTRY_RP);	// 64
    OUTPUT_NAME(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_RP_ENTRY_WC);	// 65
    OUTPUT_NAME(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_RP_ENTRY_SG);	// 66
    OUTPUT_NAME(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_RP_ENTRY_SG_RPT);// 67
    OUTPUT_NAME(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_WC_ENTRY_WC);	// 68
    OUTPUT_NAME(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_WC_ENTRY_SG);	// 69
    OUTPUT_NAME(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_WC_ENTRY_SG_RPT);// 70
    OUTPUT_NAME(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_SG_ENTRY_SG);	// 71
    OUTPUT_NAME(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_SG_ENTRY_SG_RPT);// 72
    OUTPUT_NAME(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_SG_RPT_ENTRY_SG);// 73
    OUTPUT_NAME(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_SG_RPT_ENTRY_SG_RPT);// 74
    
#undef INPUT_NAME
#undef OUTPUT_NAME
    
    for (size_t i = 0; i < INPUT_STATE_MAX; i++) {
	list<PimMreAction>::const_iterator iter;
	printf("Input = %s\n",
	       input_state_names[i].c_str());
	for (iter = _output_action[i].begin();
	     iter != _output_action[i].end();
	     ++iter) {
	    const PimMreAction& action = *iter;
	    string entry_type_str = "UnknownEntryType";
	    do {
		if (action.is_sg()) {
		    entry_type_str = "(S,G)";
		    break;
		}
		if (action.is_sg_rpt()) {
		    entry_type_str = "(S,G,rpt)";
		    break;
		}
		if (action.is_wc()) {
		    entry_type_str = "(*,G)";
		    break;
		}
		if (action.is_rp()) {
		    entry_type_str = "(*,*,RP)";
		    break;
		}
		if (action.is_mfc()) {
		    entry_type_str = "(MFC)";
		    break;
		}
	    } while (false);
	    printf("%8s%s%*s\n",
		   "",
		   output_state_names[action.output_state()].c_str(),
		   (int)(80 - 8 - output_state_names[action.output_state()].size() - 5),
		   entry_type_str.c_str());
	}
	printf("\n");
    }
}

void
PimMreTrackState::print_actions_num() const
{
    for (size_t i = 0; i < INPUT_STATE_MAX; i++) {
	list<PimMreAction>::const_iterator iter;
	printf("Input action = %u Output actions =", (uint32_t)i);
	for (iter = _output_action[i].begin();
	     iter != _output_action[i].end();
	     ++iter) {
	    const PimMreAction& action = *iter;
	    string entry_type_str = "UnknownEntryType";
	    do {
		if (action.is_sg()) {
		    entry_type_str = "(S,G)";
		    break;
		}
		if (action.is_sg_rpt()) {
		    entry_type_str = "(S,G,rpt)";
		    break;
		}
		if (action.is_wc()) {
		    entry_type_str = "(*,G)";
		    break;
		}
		if (action.is_rp()) {
		    entry_type_str = "(*,*,RP)";
		    break;
		}
		if (action.is_mfc()) {
		    entry_type_str = "(MFC)";
		    break;
		}
	    } while (false);
	    printf(" %d/%s", action.output_state(), entry_type_str.c_str());
	}
	printf("\n");
    }
}

//
// Remove state methods
//

//
// The parent method that removes all extra state
//
list<PimMreAction>
PimMreTrackState::remove_state(list<PimMreAction> action_list)
{
    //
    // Remove all extra states
    //
    action_list = remove_state_mrib_next_hop_rp_g_changed(action_list);
    action_list = remove_state_mrib_next_hop_s_changed(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::remove_state_mrib_next_hop_rp_g_changed(list<PimMreAction> action_list)
{
    PimMreAction keep_action(OUTPUT_STATE_MRIB_NEXT_HOP_RP_G, PIM_MRE_WC);
    PimMreAction remove_action(OUTPUT_STATE_RPFP_NBR_WC, PIM_MRE_WC);
    
    return (remove_action_from_list(action_list, keep_action, remove_action));
}

list<PimMreAction>
PimMreTrackState::remove_state_mrib_next_hop_s_changed(list<PimMreAction> action_list)
{
    PimMreAction keep_action(OUTPUT_STATE_MRIB_NEXT_HOP_S, PIM_MRE_SG);
    PimMreAction remove_action(OUTPUT_STATE_RPFP_NBR_SG, PIM_MRE_SG);
    
    return (remove_action_from_list(action_list, keep_action, remove_action));
}

//
// Input state methods
//
void
PimMreTrackState::input_state_rp_changed(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_RP_CHANGED, action_list);
}

void
PimMreTrackState::input_state_mrib_rp_changed(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_MRIB_RP_CHANGED, action_list);
}

void
PimMreTrackState::input_state_mrib_s_changed(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_MRIB_S_CHANGED, action_list);
}

void
PimMreTrackState::input_state_mrib_next_hop_rp_changed(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_MRIB_NEXT_HOP_RP_CHANGED, action_list);
}

void
PimMreTrackState::input_state_mrib_next_hop_rp_gen_id_changed(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_MRIB_NEXT_HOP_RP_GEN_ID_CHANGED, action_list);
}

void
PimMreTrackState::input_state_mrib_next_hop_rp_g_changed(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_MRIB_NEXT_HOP_RP_G_CHANGED, action_list);
}

void
PimMreTrackState::input_state_mrib_next_hop_s_changed(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_MRIB_NEXT_HOP_S_CHANGED, action_list);
}

void
PimMreTrackState::input_state_rpfp_nbr_wc_changed(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_RPFP_NBR_WC_CHANGED, action_list);
}

void
PimMreTrackState::input_state_rpfp_nbr_wc_gen_id_changed(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_RPFP_NBR_WC_GEN_ID_CHANGED, action_list);
}

void
PimMreTrackState::input_state_rpfp_nbr_sg_changed(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_RPFP_NBR_SG_CHANGED, action_list);
}

void
PimMreTrackState::input_state_rpfp_nbr_sg_gen_id_changed(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_RPFP_NBR_SG_GEN_ID_CHANGED, action_list);
}

void
PimMreTrackState::input_state_rpfp_nbr_sg_rpt_changed(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_RPFP_NBR_SG_RPT_CHANGED, action_list);
}

void
PimMreTrackState::input_state_receive_join_rp(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_RECEIVE_JOIN_RP, action_list);
}

void
PimMreTrackState::input_state_receive_join_wc(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_RECEIVE_JOIN_WC, action_list);
}

void
PimMreTrackState::input_state_receive_join_sg(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_RECEIVE_JOIN_SG, action_list);
}

void
PimMreTrackState::input_state_receive_join_sg_rpt(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_RECEIVE_JOIN_SG_RPT, action_list);
}

void
PimMreTrackState::input_state_receive_prune_rp(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_RECEIVE_PRUNE_RP, action_list);
}

void
PimMreTrackState::input_state_receive_prune_wc(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_RECEIVE_PRUNE_WC, action_list);
}

void
PimMreTrackState::input_state_receive_prune_sg(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_RECEIVE_PRUNE_SG, action_list);
}

void
PimMreTrackState::input_state_receive_prune_sg_rpt(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_RECEIVE_PRUNE_SG_RPT, action_list);
}

void
PimMreTrackState::input_state_receive_end_of_message_sg_rpt(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_RECEIVE_END_OF_MESSAGE_SG_RPT, action_list);
}

void
PimMreTrackState::input_state_see_prune_wc(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_SEE_PRUNE_WC, action_list);
}

void
PimMreTrackState::input_state_downstream_jp_state_rp(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_DOWNSTREAM_JP_STATE_RP, action_list);
}

void
PimMreTrackState::input_state_downstream_jp_state_wc(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_DOWNSTREAM_JP_STATE_WC, action_list);
}

void
PimMreTrackState::input_state_downstream_jp_state_sg(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_DOWNSTREAM_JP_STATE_SG, action_list);
}

void
PimMreTrackState::input_state_downstream_jp_state_sg_rpt(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_DOWNSTREAM_JP_STATE_SG_RPT, action_list);
}

void
PimMreTrackState::input_state_upstream_jp_state_sg(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_UPSTREAM_JP_STATE_SG, action_list);
}

void
PimMreTrackState::input_state_local_receiver_include_wc(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_LOCAL_RECEIVER_INCLUDE_WC, action_list);
}

void
PimMreTrackState::input_state_local_receiver_include_sg(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_LOCAL_RECEIVER_INCLUDE_SG, action_list);
}

void
PimMreTrackState::input_state_local_receiver_exclude_sg(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_LOCAL_RECEIVER_EXCLUDE_SG, action_list);
}

void
PimMreTrackState::input_state_assert_state_wc(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_ASSERT_STATE_WC, action_list);
}

void
PimMreTrackState::input_state_assert_state_sg(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_ASSERT_STATE_SG, action_list);
}

void
PimMreTrackState::input_state_assert_winner_nbr_wc_gen_id_changed(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_ASSERT_WINNER_NBR_WC_GEN_ID_CHANGED, action_list);
}

void
PimMreTrackState::input_state_assert_rpf_interface_wc_changed(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_ASSERT_RPF_INTERFACE_WC_CHANGED, action_list);
}

void
PimMreTrackState::input_state_assert_rpf_interface_sg_changed(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_ASSERT_RPF_INTERFACE_SG_CHANGED, action_list);
}

void
PimMreTrackState::input_state_assert_winner_nbr_sg_gen_id_changed(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_ASSERT_WINNER_NBR_SG_GEN_ID_CHANGED, action_list);
}

void
PimMreTrackState::input_state_i_am_dr(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_I_AM_DR, action_list);
}

void
PimMreTrackState::input_state_my_ip_address(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_MY_IP_ADDRESS, action_list);
}

void
PimMreTrackState::input_state_my_ip_subnet_address(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_MY_IP_SUBNET_ADDRESS, action_list);
}

void
PimMreTrackState::input_state_is_switch_to_spt_desired_sg(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_IS_SWITCH_TO_SPT_DESIRED_SG, action_list);
}

void
PimMreTrackState::input_state_keepalive_timer_sg(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_KEEPALIVE_TIMER_SG, action_list);
}

void
PimMreTrackState::input_state_sptbit_sg(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_SPTBIT_SG, action_list);
}

void
PimMreTrackState::input_state_in_start_vif(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_IN_START_VIF, action_list);
}

void
PimMreTrackState::input_state_in_stop_vif(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_IN_STOP_VIF, action_list);
}

void
PimMreTrackState::input_state_in_add_pim_mre_rp(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_IN_ADD_PIM_MRE_RP, action_list);
}

void
PimMreTrackState::input_state_in_add_pim_mre_wc(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_IN_ADD_PIM_MRE_WC, action_list);
}

void
PimMreTrackState::input_state_in_add_pim_mre_sg(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_IN_ADD_PIM_MRE_SG, action_list);
}

void
PimMreTrackState::input_state_in_add_pim_mre_sg_rpt(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_IN_ADD_PIM_MRE_SG_RPT, action_list);
}

void
PimMreTrackState::input_state_in_remove_pim_mre_rp(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_IN_REMOVE_PIM_MRE_RP, action_list);
}

void
PimMreTrackState::input_state_in_remove_pim_mre_wc(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_IN_REMOVE_PIM_MRE_WC, action_list);
}

void
PimMreTrackState::input_state_in_remove_pim_mre_sg(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_IN_REMOVE_PIM_MRE_SG, action_list);
}

void
PimMreTrackState::input_state_in_remove_pim_mre_sg_rpt(list<PimMreAction> action_list)
{
    add_action_list(INPUT_STATE_IN_REMOVE_PIM_MRE_SG_RPT, action_list);
}


//
// Output state methods
//
list<PimMreAction>
PimMreTrackState::output_state_rp_wc(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_RP_WC, PIM_MRE_WC);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_rp_wc(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_rp_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_RP_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_rp_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_rp_sg_rpt(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_RP_SG_RPT, PIM_MRE_SG_RPT);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_rp_sg_rpt(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_rp_mfc(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_RP_MFC, PIM_MFC);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_rp_mfc(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_mrib_rp_rp(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_MRIB_RP_RP, PIM_MRE_RP);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_mrib_rp_rp(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_mrib_rp_wc(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_MRIB_RP_WC, PIM_MRE_WC);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_mrib_rp_wc(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_mrib_rp_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_MRIB_RP_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_mrib_rp_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_mrib_rp_sg_rpt(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_MRIB_RP_SG_RPT, PIM_MRE_SG_RPT);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_mrib_rp_sg_rpt(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_mrib_s_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_MRIB_S_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_mrib_s_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_mrib_s_sg_rpt(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_MRIB_S_SG_RPT, PIM_MRE_SG_RPT);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_mrib_s_sg_rpt(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_is_join_desired_rp(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_IS_JOIN_DESIRED_RP, PIM_MRE_RP);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_is_join_desired_rp(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_is_join_desired_wc(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_IS_JOIN_DESIRED_WC, PIM_MRE_WC);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_is_join_desired_wc(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_is_join_desired_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_IS_JOIN_DESIRED_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_is_join_desired_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_is_prune_desired_sg_rpt(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_IS_PRUNE_DESIRED_SG_RPT, PIM_MRE_SG_RPT);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_is_prune_desired_sg_rpt(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_is_prune_desired_sg_rpt_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_IS_PRUNE_DESIRED_SG_RPT_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_is_prune_desired_sg_rpt_sg(action_list);
    
    return (action_list);
}

// XXX: the action is performed on (S,G,rpt) states
list<PimMreAction>
PimMreTrackState::output_state_is_rpt_join_desired_g(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_IS_RPT_JOIN_DESIRED_G, PIM_MRE_SG_RPT);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_is_rpt_join_desired_g(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_inherited_olist_sg_rpt(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_INHERITED_OLIST_SG_RPT, PIM_MRE_SG_RPT);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_inherited_olist_sg_rpt(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_iif_olist_mfc(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_IIF_OLIST_MFC, PIM_MFC);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_iif_olist_mfc(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_is_could_register_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_IS_COULD_REGISTER_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_is_could_register_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_assert_tracking_desired_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_ASSERT_TRACKING_DESIRED_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_assert_tracking_desired_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_assert_tracking_desired_wc(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_ASSERT_TRACKING_DESIRED_WC, PIM_MRE_WC);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_assert_tracking_desired_wc(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_could_assert_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_COULD_ASSERT_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_could_assert_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_could_assert_wc(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_COULD_ASSERT_WC, PIM_MRE_WC);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_could_assert_wc(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_my_assert_metric_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_MY_ASSERT_METRIC_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_my_assert_metric_sg(action_list);
    
    return (action_list);
}

// XXX: not in the spec; a wrapper for rpt_assert_metric(G,I)
list<PimMreAction>
PimMreTrackState::output_state_my_assert_metric_wc(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_MY_ASSERT_METRIC_WC, PIM_MRE_WC);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_my_assert_metric_wc(action_list);
    
    return (action_list);
}

// XXX: Assert-state (S,G) machine for "RPF_interface (S) stops being I"
list<PimMreAction>
PimMreTrackState::output_state_assert_rpf_interface_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_ASSERT_RPF_INTERFACE_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_assert_rpf_interface_sg(action_list);
    
    return (action_list);
}

// XXX: Assert-state (*,G) machine for "RPF_interface (RP(G)) stops being I"
list<PimMreAction>
PimMreTrackState::output_state_assert_rpf_interface_wc(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_ASSERT_RPF_INTERFACE_WC, PIM_MRE_WC);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_assert_rpf_interface_wc(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_assert_receive_join_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_ASSERT_RECEIVE_JOIN_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_assert_receive_join_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_assert_receive_join_wc(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_ASSERT_RECEIVE_JOIN_WC, PIM_MRE_WC);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_assert_receive_join_wc(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_assert_winner_nbr_sg_gen_id(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_ASSERT_WINNER_NBR_SG_GEN_ID, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_assert_winner_nbr_sg_gen_id(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_assert_winner_nbr_wc_gen_id(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_ASSERT_WINNER_NBR_WC_GEN_ID, PIM_MRE_WC);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_assert_winner_nbr_wc_gen_id(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_receive_join_wc_by_sg_rpt(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_RECEIVE_JOIN_WC_BY_SG_RPT, PIM_MRE_SG_RPT);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_receive_join_wc_by_sg_rpt(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_receive_end_of_message_sg_rpt(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_RECEIVE_END_OF_MESSAGE_SG_RPT, PIM_MRE_SG_RPT);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_receive_end_of_message_sg_rpt(action_list);
    
    return (action_list);
}

// XXX: the action is performed on (S,G) states
list<PimMreAction>
PimMreTrackState::output_state_sg_see_prune_wc(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_SG_SEE_PRUNE_WC, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_sg_see_prune_wc(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_check_switch_to_spt_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_CHECK_SWITCH_TO_SPT_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_check_switch_to_spt_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_rpfp_nbr_wc(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_RPFP_NBR_WC, PIM_MRE_WC);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_rpfp_nbr_wc(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_rpfp_nbr_wc_gen_id(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_RPFP_NBR_WC_GEN_ID, PIM_MRE_WC);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_rpfp_nbr_wc_gen_id(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_rpfp_nbr_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_RPFP_NBR_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_rpfp_nbr_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_rpfp_nbr_sg_gen_id(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_RPFP_NBR_SG_GEN_ID, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_rpfp_nbr_sg_gen_id(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_rpfp_nbr_sg_rpt(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_RPFP_NBR_SG_RPT, PIM_MRE_SG_RPT);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_rpfp_nbr_sg_rpt(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_rpfp_nbr_sg_rpt_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_RPFP_NBR_SG_RPT_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_rpfp_nbr_sg_rpt_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_mrib_next_hop_rp(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_MRIB_NEXT_HOP_RP, PIM_MRE_RP);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_mrib_next_hop_rp(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_mrib_next_hop_rp_gen_id(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_MRIB_NEXT_HOP_RP_GEN_ID, PIM_MRE_RP);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_mrib_next_hop_rp_gen_id(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_mrib_next_hop_rp_g(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_MRIB_NEXT_HOP_RP_G, PIM_MRE_WC);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_mrib_next_hop_rp_g(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_mrib_next_hop_s(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_MRIB_NEXT_HOP_S, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_mrib_next_hop_s(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_start_vif_rp(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_START_VIF_RP, PIM_MRE_RP);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_start_vif_rp(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_start_vif_wc(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_START_VIF_WC, PIM_MRE_WC);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_start_vif_wc(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_start_vif_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_START_VIF_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_start_vif_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_start_vif_sg_rpt(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_START_VIF_SG_RPT, PIM_MRE_SG_RPT);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_start_vif_sg_rpt(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_stop_vif_rp(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_STOP_VIF_RP, PIM_MRE_RP);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_stop_vif_rp(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_stop_vif_wc(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_STOP_VIF_WC, PIM_MRE_WC);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_stop_vif_wc(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_stop_vif_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_STOP_VIF_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_stop_vif_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_stop_vif_sg_rpt(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_STOP_VIF_SG_RPT, PIM_MRE_SG_RPT);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_stop_vif_sg_rpt(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_add_pim_mre_rp_entry_rp(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_ADD_PIM_MRE_RP_ENTRY_RP, PIM_MRE_RP);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_add_pim_mre_rp_entry_rp(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_add_pim_mre_rp_entry_wc(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_ADD_PIM_MRE_RP_ENTRY_WC, PIM_MRE_WC);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_add_pim_mre_rp_entry_wc(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_add_pim_mre_rp_entry_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_ADD_PIM_MRE_RP_ENTRY_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_add_pim_mre_rp_entry_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_add_pim_mre_rp_entry_sg_rpt(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_ADD_PIM_MRE_RP_ENTRY_SG_RPT, PIM_MRE_SG_RPT);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_add_pim_mre_rp_entry_sg_rpt(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_add_pim_mre_wc_entry_wc(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_ADD_PIM_MRE_WC_ENTRY_WC, PIM_MRE_WC);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_add_pim_mre_wc_entry_wc(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_add_pim_mre_wc_entry_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_ADD_PIM_MRE_WC_ENTRY_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_add_pim_mre_wc_entry_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_add_pim_mre_wc_entry_sg_rpt(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_ADD_PIM_MRE_WC_ENTRY_SG_RPT, PIM_MRE_SG_RPT);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_add_pim_mre_wc_entry_sg_rpt(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_add_pim_mre_sg_entry_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_ADD_PIM_MRE_SG_ENTRY_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_add_pim_mre_sg_entry_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_add_pim_mre_sg_entry_sg_rpt(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_ADD_PIM_MRE_SG_ENTRY_SG_RPT, PIM_MRE_SG_RPT);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_add_pim_mre_sg_entry_sg_rpt(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_add_pim_mre_sg_rpt_entry_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_ADD_PIM_MRE_SG_RPT_ENTRY_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_add_pim_mre_sg_rpt_entry_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_add_pim_mre_sg_rpt_entry_sg_rpt(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_ADD_PIM_MRE_SG_RPT_ENTRY_SG_RPT, PIM_MRE_SG_RPT);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_add_pim_mre_sg_rpt_entry_sg_rpt(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_remove_pim_mre_rp_entry_rp(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_RP_ENTRY_RP, PIM_MRE_RP);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_remove_pim_mre_rp_entry_rp(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_remove_pim_mre_rp_entry_wc(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_RP_ENTRY_WC, PIM_MRE_WC);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_remove_pim_mre_rp_entry_wc(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_remove_pim_mre_rp_entry_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_RP_ENTRY_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_remove_pim_mre_rp_entry_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_remove_pim_mre_rp_entry_sg_rpt(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_RP_ENTRY_SG_RPT, PIM_MRE_SG_RPT);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_remove_pim_mre_rp_entry_sg_rpt(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_remove_pim_mre_wc_entry_wc(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_WC_ENTRY_WC, PIM_MRE_WC);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_remove_pim_mre_wc_entry_wc(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_remove_pim_mre_wc_entry_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_WC_ENTRY_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_remove_pim_mre_wc_entry_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_remove_pim_mre_wc_entry_sg_rpt(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_WC_ENTRY_SG_RPT, PIM_MRE_SG_RPT);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_remove_pim_mre_wc_entry_sg_rpt(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_remove_pim_mre_sg_entry_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_SG_ENTRY_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_remove_pim_mre_sg_entry_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_remove_pim_mre_sg_entry_sg_rpt(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_SG_ENTRY_SG_RPT, PIM_MRE_SG_RPT);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_remove_pim_mre_sg_entry_sg_rpt(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_remove_pim_mre_sg_rpt_entry_sg(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_SG_RPT_ENTRY_SG, PIM_MRE_SG);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_remove_pim_mre_sg_rpt_entry_sg(action_list);
    
    return (action_list);
}

list<PimMreAction>
PimMreTrackState::output_state_out_remove_pim_mre_sg_rpt_entry_sg_rpt(list<PimMreAction> action_list)
{
    bool init_flag = action_list.empty();
    PimMreAction action(OUTPUT_STATE_OUT_REMOVE_PIM_MRE_SG_RPT_ENTRY_SG_RPT, PIM_MRE_SG_RPT);
    
    if (can_add_action_to_list(action_list, action))
	action_list.push_back(action);
    
    if (init_flag)
	track_state_out_remove_pim_mre_sg_rpt_entry_sg_rpt(action_list);
    
    return (action_list);
}

//
// Track state methods
//
void
PimMreTrackState::track_state_rp_wc(list<PimMreAction> action_list)
{
    action_list = output_state_rp_wc(action_list);
    
    track_state_rp(action_list);
}

void
PimMreTrackState::track_state_rp_sg(list<PimMreAction> action_list)
{
    action_list = output_state_rp_sg(action_list);
    
    track_state_rp(action_list);
}

void
PimMreTrackState::track_state_rp_sg_rpt(list<PimMreAction> action_list)
{
    action_list = output_state_rp_sg_rpt(action_list);
    
    track_state_rp(action_list);
}

void
PimMreTrackState::track_state_rp_mfc(list<PimMreAction> action_list)
{
    action_list = output_state_rp_mfc(action_list);
    
    track_state_rp(action_list);
}

void
PimMreTrackState::track_state_mrib_rp_rp(list<PimMreAction> action_list)
{
    action_list = output_state_mrib_rp_rp(action_list);
    
    track_state_mrib_rp(action_list);
}

void
PimMreTrackState::track_state_mrib_rp_wc(list<PimMreAction> action_list)
{
    action_list = output_state_mrib_rp_wc(action_list);
    
    track_state_mrib_rp(action_list);
}

void
PimMreTrackState::track_state_mrib_rp_sg(list<PimMreAction> action_list)
{
    action_list = output_state_mrib_rp_sg(action_list);
    
    track_state_mrib_rp(action_list);
}

void
PimMreTrackState::track_state_mrib_rp_sg_rpt(list<PimMreAction> action_list)
{
    action_list = output_state_mrib_rp_sg_rpt(action_list);
    
    track_state_mrib_rp(action_list);
}

void
PimMreTrackState::track_state_mrib_s_sg(list<PimMreAction> action_list)
{
    action_list = output_state_mrib_s_sg(action_list);
    
    track_state_mrib_s(action_list);
}

void
PimMreTrackState::track_state_mrib_s_sg_rpt(list<PimMreAction> action_list)
{
    action_list = output_state_mrib_s_sg_rpt(action_list);
    
    track_state_mrib_s(action_list);
}

void
PimMreTrackState::track_state_rp(list<PimMreAction> action_list)
{
    input_state_rp_changed(action_list);
}

void
PimMreTrackState::track_state_mrib_rp(list<PimMreAction> action_list)
{
    track_state_rp(action_list);
    
    input_state_mrib_rp_changed(action_list);
}

void
PimMreTrackState::track_state_mrib_s(list<PimMreAction> action_list)
{
    input_state_mrib_s_changed(action_list);
}

void
PimMreTrackState::track_state_rpf_interface_rp(list<PimMreAction> action_list)
{
    track_state_rp(action_list);
    track_state_mrib_rp(action_list);
}

void
PimMreTrackState::track_state_rpf_interface_s(list<PimMreAction> action_list)
{
    track_state_mrib_s(action_list);
}

void
PimMreTrackState::track_state_mrib_next_hop_rp(list<PimMreAction> action_list)
{
    action_list = output_state_mrib_next_hop_rp(action_list);
    
    track_state_rp(action_list);
    track_state_mrib_rp(action_list);
    
    input_state_mrib_next_hop_rp_changed(action_list);
}

void
PimMreTrackState::track_state_mrib_next_hop_rp_gen_id(list<PimMreAction> action_list)
{
    action_list = output_state_mrib_next_hop_rp_gen_id(action_list);
    
    input_state_mrib_next_hop_rp_gen_id_changed(action_list);
}

void
PimMreTrackState::track_state_mrib_next_hop_rp_g(list<PimMreAction> action_list)
{
    action_list = output_state_mrib_next_hop_rp_g(action_list);
    
    track_state_mrib_next_hop_rp(action_list);
    
    input_state_mrib_next_hop_rp_g_changed(action_list);
}

void
PimMreTrackState::track_state_mrib_next_hop_s(list<PimMreAction> action_list)
{
    action_list = output_state_mrib_next_hop_s(action_list);
    
    track_state_mrib_s(action_list);
    
    input_state_mrib_next_hop_s_changed(action_list);
}

void
PimMreTrackState::track_state_mrib_pref_metric_s(list<PimMreAction> action_list)
{
    track_state_mrib_s(action_list);
}

void
PimMreTrackState::track_state_mrib_pref_metric_rp(list<PimMreAction> action_list)
{
    track_state_rp(action_list);
    track_state_mrib_rp(action_list);
}

void
PimMreTrackState::track_state_receive_join_rp(list<PimMreAction> action_list)
{
    input_state_receive_join_rp(action_list);
}

void
PimMreTrackState::track_state_receive_join_wc(list<PimMreAction> action_list)
{
    input_state_receive_join_wc(action_list);
}

void
PimMreTrackState::track_state_receive_join_sg(list<PimMreAction> action_list)
{
    input_state_receive_join_sg(action_list);
}

void
PimMreTrackState::track_state_receive_join_sg_rpt(list<PimMreAction> action_list)
{
    input_state_receive_join_sg_rpt(action_list);
}

// XXX: unused
void
PimMreTrackState::track_state_receive_prune_rp(list<PimMreAction> action_list)
{
    input_state_receive_prune_rp(action_list);
}

void
PimMreTrackState::track_state_receive_prune_wc(list<PimMreAction> action_list)
{
    input_state_receive_prune_wc(action_list);
}

void
PimMreTrackState::track_state_receive_prune_sg(list<PimMreAction> action_list)
{
    input_state_receive_prune_sg(action_list);
}

// XXX: unused
void
PimMreTrackState::track_state_receive_prune_sg_rpt(list<PimMreAction> action_list)
{
    input_state_receive_prune_sg_rpt(action_list);
}

void
PimMreTrackState::track_state_receive_end_of_message_sg_rpt(list<PimMreAction> action_list)
{
    action_list = output_state_receive_end_of_message_sg_rpt(action_list);
    
    input_state_receive_end_of_message_sg_rpt(action_list);
}

void
PimMreTrackState::track_state_sg_see_prune_wc(list<PimMreAction> action_list)
{
    action_list = output_state_sg_see_prune_wc(action_list);
    
    input_state_see_prune_wc(action_list);
}
						      
void
PimMreTrackState::track_state_downstream_jp_state_rp(list<PimMreAction> action_list)
{
    input_state_downstream_jp_state_rp(action_list);
}

void
PimMreTrackState::track_state_downstream_jp_state_wc(list<PimMreAction> action_list)
{
    track_state_receive_prune_wc(action_list);
    
    input_state_downstream_jp_state_wc(action_list);
}

void
PimMreTrackState::track_state_downstream_jp_state_sg(list<PimMreAction> action_list)
{
    track_state_receive_prune_sg(action_list);
    
    input_state_downstream_jp_state_sg(action_list);
}

void
PimMreTrackState::track_state_downstream_jp_state_sg_rpt(list<PimMreAction> action_list)
{
    input_state_downstream_jp_state_sg_rpt(action_list);
}

void
PimMreTrackState::track_state_upstream_jp_state_sg(list<PimMreAction> action_list)
{
    input_state_upstream_jp_state_sg(action_list);
}

void
PimMreTrackState::track_state_local_receiver_include_wc(list<PimMreAction> action_list)
{
    input_state_local_receiver_include_wc(action_list);
}

void
PimMreTrackState::track_state_local_receiver_include_sg(list<PimMreAction> action_list)
{
    input_state_local_receiver_include_sg(action_list);
}

void
PimMreTrackState::track_state_local_receiver_exclude_sg(list<PimMreAction> action_list)
{
    input_state_local_receiver_exclude_sg(action_list);
}

void
PimMreTrackState::track_state_assert_state_wc(list<PimMreAction> action_list)
{
    input_state_assert_state_wc(action_list);
}

void
PimMreTrackState::track_state_assert_state_sg(list<PimMreAction> action_list)
{
    input_state_assert_state_sg(action_list);
}

void
PimMreTrackState::track_state_i_am_dr(list<PimMreAction> action_list)
{
    input_state_i_am_dr(action_list);
}

void
PimMreTrackState::track_state_my_ip_address(list<PimMreAction> action_list)
{
    input_state_my_ip_address(action_list);
}

void
PimMreTrackState::track_state_my_ip_subnet_address(list<PimMreAction> action_list)
{
    input_state_my_ip_subnet_address(action_list);
}

void
PimMreTrackState::track_state_is_switch_to_spt_desired_sg(list<PimMreAction> action_list)
{
    input_state_is_switch_to_spt_desired_sg(action_list);
}

void
PimMreTrackState::track_state_keepalive_timer_sg(list<PimMreAction> action_list)
{
    input_state_keepalive_timer_sg(action_list);
}

//
//
//
void
PimMreTrackState::track_state_immediate_olist_rp(list<PimMreAction> action_list)
{
    track_state_joins_rp(action_list);
}

void
PimMreTrackState::track_state_immediate_olist_wc(list<PimMreAction> action_list)
{
    track_state_joins_wc(action_list);
    track_state_pim_include_wc(action_list);
    track_state_lost_assert_wc(action_list);
}

void
PimMreTrackState::track_state_immediate_olist_sg(list<PimMreAction> action_list)
{
    track_state_joins_sg(action_list);
    track_state_pim_include_sg(action_list);
    track_state_lost_assert_sg(action_list);
}

void
PimMreTrackState::track_state_inherited_olist_sg_rpt(list<PimMreAction> action_list)
{
    action_list = output_state_inherited_olist_sg_rpt(action_list);
    
    track_state_joins_rp(action_list);
    track_state_joins_wc(action_list);
    track_state_prunes_sg_rpt(action_list);
    track_state_pim_include_wc(action_list);
    track_state_pim_exclude_sg(action_list);
    track_state_lost_assert_wc(action_list);
    track_state_lost_assert_sg_rpt(action_list);
}

void
PimMreTrackState::track_state_inherited_olist_sg(list<PimMreAction> action_list)
{
    track_state_inherited_olist_sg_rpt(action_list);
    track_state_immediate_olist_sg(action_list);
}

void
PimMreTrackState::track_state_iif_olist_mfc(list<PimMreAction> action_list)
{
    action_list = output_state_iif_olist_mfc(action_list);
    
    track_state_sptbit_sg(action_list);
    track_state_rpf_interface_s(action_list);
    track_state_upstream_jp_state_sg(action_list);
    // TODO: XXX: PAVPAVPAV: do we need to track the upstream state for
    // other entries?
    track_state_inherited_olist_sg(action_list);
    track_state_rpf_interface_rp(action_list);
    track_state_inherited_olist_sg_rpt(action_list);
    track_state_is_switch_to_spt_desired_sg(action_list);
}

void
PimMreTrackState::track_state_pim_include_wc(list<PimMreAction> action_list)
{
    track_state_i_am_dr(action_list);
    track_state_lost_assert_wc(action_list);
    track_state_assert_winner_wc(action_list);
    track_state_local_receiver_include_wc(action_list);
}

void
PimMreTrackState::track_state_pim_include_sg(list<PimMreAction> action_list)
{
    track_state_i_am_dr(action_list);
    track_state_lost_assert_sg(action_list);
    track_state_assert_winner_sg(action_list);
    track_state_local_receiver_include_sg(action_list);
}

void
PimMreTrackState::track_state_pim_exclude_sg(list<PimMreAction> action_list)
{
    track_state_i_am_dr(action_list);
    track_state_lost_assert_sg(action_list);
    track_state_assert_winner_sg(action_list);
    track_state_local_receiver_exclude_sg(action_list);
}

void
PimMreTrackState::track_state_joins_rp(list<PimMreAction> action_list)
{
    track_state_downstream_jp_state_rp(action_list);
}

void
PimMreTrackState::track_state_joins_wc(list<PimMreAction> action_list)
{
    track_state_downstream_jp_state_wc(action_list);
}

void
PimMreTrackState::track_state_joins_sg(list<PimMreAction> action_list)
{
    track_state_downstream_jp_state_sg(action_list);
}

void
PimMreTrackState::track_state_prunes_sg_rpt(list<PimMreAction> action_list)
{
    track_state_downstream_jp_state_sg_rpt(action_list);
}

void
PimMreTrackState::track_state_rpfp_nbr_wc(list<PimMreAction> action_list)
{
    action_list = output_state_rpfp_nbr_wc(action_list);
    
    track_state_rpf_interface_rp(action_list);
    track_state_i_am_assert_loser_wc(action_list);
    track_state_assert_winner_wc(action_list);
    track_state_mrib_next_hop_rp_g(action_list);
    
    input_state_rpfp_nbr_wc_changed(action_list);
}

void
PimMreTrackState::track_state_rpfp_nbr_wc_gen_id(list<PimMreAction> action_list)
{
    action_list = output_state_rpfp_nbr_wc_gen_id(action_list);
    
    input_state_rpfp_nbr_wc_gen_id_changed(action_list);
}

void
PimMreTrackState::track_state_rpfp_nbr_sg_rpt(list<PimMreAction> action_list)
{
    action_list = output_state_rpfp_nbr_sg_rpt(action_list);
    
    track_state_rpf_interface_rp(action_list);
    track_state_i_am_assert_loser_sg(action_list);
    track_state_assert_winner_sg(action_list);
    track_state_rpfp_nbr_wc(action_list);
    
    input_state_rpfp_nbr_sg_rpt_changed(action_list);
}

void
PimMreTrackState::track_state_rpfp_nbr_sg_rpt_sg(list<PimMreAction> action_list)
{
    action_list = output_state_rpfp_nbr_sg_rpt_sg(action_list);
    
    track_state_rpfp_nbr_sg_rpt(action_list);
}

void
PimMreTrackState::track_state_rpfp_nbr_sg(list<PimMreAction> action_list)
{
    action_list = output_state_rpfp_nbr_sg(action_list);
    
    track_state_rpf_interface_s(action_list);
    track_state_i_am_assert_loser_sg(action_list);
    track_state_assert_winner_sg(action_list);
    track_state_mrib_next_hop_s(action_list);
    
    input_state_rpfp_nbr_sg_changed(action_list);
}

void
PimMreTrackState::track_state_rpfp_nbr_sg_gen_id(list<PimMreAction> action_list)
{
    action_list = output_state_rpfp_nbr_sg_gen_id(action_list);
    
    input_state_rpfp_nbr_sg_gen_id_changed(action_list);
}

void
PimMreTrackState::track_state_check_switch_to_spt_sg(list<PimMreAction> action_list)
{
    action_list = output_state_check_switch_to_spt_sg(action_list);
    
    track_state_pim_include_wc(action_list);
    track_state_pim_exclude_sg(action_list);
    track_state_pim_include_sg(action_list);
    track_state_is_switch_to_spt_desired_sg(action_list);
}

void
PimMreTrackState::track_state_sptbit_sg(list<PimMreAction> action_list)
{
    // XXX: the SPTbit(S,G) does not depend on other
    // state for recomputation and is updated only when a data packet
    // or an Assert(S,G) are received.
    
    input_state_sptbit_sg(action_list);
}

void
PimMreTrackState::track_state_is_could_register_sg(list<PimMreAction> action_list)
{
    action_list = output_state_is_could_register_sg(action_list);
    
    track_state_rpf_interface_s(action_list);
    track_state_i_am_dr(action_list);
    track_state_keepalive_timer_sg(action_list);
    track_state_is_directly_connected_s(action_list);
    track_state_rp(action_list); // TODO: XXX: PAVPAVPAV: not in the spec (yet)
}

void
PimMreTrackState::track_state_is_join_desired_rp(list<PimMreAction> action_list)
{
    action_list = output_state_is_join_desired_rp(action_list);
    
    track_state_immediate_olist_rp(action_list);
}

void
PimMreTrackState::track_state_is_join_desired_wc(list<PimMreAction> action_list)
{
    action_list = output_state_is_join_desired_wc(action_list);
    
    track_state_immediate_olist_wc(action_list);
    track_state_is_join_desired_rp(action_list);
    track_state_rpf_interface_rp(action_list);
    track_state_assert_winner_wc(action_list);
}

void
PimMreTrackState::track_state_is_join_desired_sg(list<PimMreAction> action_list)
{
    action_list = output_state_is_join_desired_sg(action_list);
    
    track_state_immediate_olist_sg(action_list);
    track_state_keepalive_timer_sg(action_list);
    track_state_inherited_olist_sg(action_list);
}

void
PimMreTrackState::track_state_is_rpt_join_desired_g(list<PimMreAction> action_list)
{
    action_list = output_state_is_rpt_join_desired_g(action_list);
    
    track_state_is_join_desired_wc(action_list);
    track_state_is_join_desired_rp(action_list);
}

void
PimMreTrackState::track_state_is_prune_desired_sg_rpt(list<PimMreAction> action_list)
{
    action_list = output_state_is_prune_desired_sg_rpt(action_list);
    
    track_state_is_rpt_join_desired_g(action_list);
    track_state_inherited_olist_sg_rpt(action_list);
    track_state_sptbit_sg(action_list);
    track_state_rpfp_nbr_wc(action_list);
    track_state_rpfp_nbr_sg(action_list);
}

void
PimMreTrackState::track_state_is_prune_desired_sg_rpt_sg(list<PimMreAction> action_list)
{
    action_list = output_state_is_prune_desired_sg_rpt_sg(action_list);
    
    track_state_is_prune_desired_sg_rpt(action_list);
}

void
PimMreTrackState::track_state_could_assert_sg(list<PimMreAction> action_list)
{
    action_list = output_state_could_assert_sg(action_list);
    
    track_state_sptbit_sg(action_list);
    track_state_rpf_interface_s(action_list);
    track_state_joins_rp(action_list);
    track_state_joins_wc(action_list);
    track_state_prunes_sg_rpt(action_list);
    track_state_pim_include_wc(action_list);
    track_state_pim_exclude_sg(action_list);
    track_state_lost_assert_wc(action_list);
    track_state_joins_sg(action_list);
    track_state_pim_include_sg(action_list);
}

void
PimMreTrackState::track_state_assert_tracking_desired_sg(list<PimMreAction> action_list)
{
    action_list = output_state_assert_tracking_desired_sg(action_list);
    
    track_state_joins_rp(action_list);
    track_state_joins_wc(action_list);
    track_state_prunes_sg_rpt(action_list);
    track_state_pim_include_wc(action_list);
    track_state_pim_exclude_sg(action_list);
    track_state_lost_assert_wc(action_list);
    track_state_joins_sg(action_list);
    track_state_local_receiver_include_sg(action_list);
    track_state_i_am_dr(action_list);
    track_state_assert_winner_sg(action_list);
    track_state_rpf_interface_s(action_list);
    track_state_is_join_desired_sg(action_list);
    track_state_rpf_interface_rp(action_list);
    track_state_is_join_desired_wc(action_list);
    track_state_sptbit_sg(action_list);
}

void
PimMreTrackState::track_state_could_assert_wc(list<PimMreAction> action_list)
{
    action_list = output_state_could_assert_wc(action_list);
    
    track_state_joins_rp(action_list);
    track_state_joins_wc(action_list);
    track_state_pim_include_wc(action_list);
    track_state_rpf_interface_rp(action_list);
}

void
PimMreTrackState::track_state_assert_tracking_desired_wc(list<PimMreAction> action_list)
{
    action_list = output_state_assert_tracking_desired_wc(action_list);
    
    track_state_could_assert_wc(action_list);
    track_state_local_receiver_include_wc(action_list);
    track_state_i_am_dr(action_list);
    track_state_assert_winner_wc(action_list);
    track_state_rpf_interface_rp(action_list);
    track_state_is_rpt_join_desired_g(action_list);
}

void
PimMreTrackState::track_state_my_assert_metric_sg(list<PimMreAction> action_list)
{
    action_list = output_state_my_assert_metric_sg(action_list);
    
    track_state_could_assert_sg(action_list);
    track_state_spt_assert_metric(action_list);
    track_state_could_assert_wc(action_list);
    track_state_rpt_assert_metric(action_list);
}

void
PimMreTrackState::track_state_my_assert_metric_wc(list<PimMreAction> action_list)
{
    action_list = output_state_my_assert_metric_wc(action_list);
    
    track_state_rpt_assert_metric(action_list);
}

void
PimMreTrackState::track_state_spt_assert_metric(list<PimMreAction> action_list)
{
    track_state_mrib_pref_metric_s(action_list);
    track_state_my_ip_address(action_list);
}

void
PimMreTrackState::track_state_rpt_assert_metric(list<PimMreAction> action_list)
{
    track_state_mrib_pref_metric_rp(action_list);
    track_state_my_ip_address(action_list);
}

void
PimMreTrackState::track_state_lost_assert_sg_rpt(list<PimMreAction> action_list)
{
    track_state_rpf_interface_rp(action_list);
    track_state_rpf_interface_s(action_list);
    track_state_sptbit_sg(action_list);
    track_state_assert_winner_sg(action_list);
}

void
PimMreTrackState::track_state_lost_assert_sg(list<PimMreAction> action_list)
{
    track_state_rpf_interface_s(action_list);
    track_state_assert_winner_sg(action_list);
    track_state_assert_winner_metric_is_better_than_spt_assert_metric_sg(action_list);
}

void
PimMreTrackState::track_state_lost_assert_wc(list<PimMreAction> action_list)
{
    track_state_rpf_interface_rp(action_list);
    track_state_assert_winner_wc(action_list);
}

void
PimMreTrackState::track_state_assert_rpf_interface_sg(list<PimMreAction> action_list)
{
    action_list = output_state_assert_rpf_interface_sg(action_list);
    
    input_state_assert_rpf_interface_sg_changed(action_list);
}

void
PimMreTrackState::track_state_assert_rpf_interface_wc(list<PimMreAction> action_list)
{
    action_list = output_state_assert_rpf_interface_wc(action_list);
    
    input_state_assert_rpf_interface_wc_changed(action_list);
}

void
PimMreTrackState::track_state_assert_receive_join_sg(list<PimMreAction> action_list)
{
    action_list = output_state_assert_receive_join_sg(action_list);
    
    track_state_receive_join_sg(action_list);
}

void
PimMreTrackState::track_state_assert_receive_join_wc(list<PimMreAction> action_list)
{
    action_list = output_state_assert_receive_join_wc(action_list);
    
    track_state_receive_join_wc(action_list);
    track_state_receive_join_rp(action_list);
}

void
PimMreTrackState::track_state_assert_winner_nbr_sg_gen_id(list<PimMreAction> action_list)
{
    action_list = output_state_assert_winner_nbr_sg_gen_id(action_list);
    
    input_state_assert_winner_nbr_sg_gen_id_changed(action_list);
}

void
PimMreTrackState::track_state_assert_winner_nbr_wc_gen_id(list<PimMreAction> action_list)
{
    action_list = output_state_assert_winner_nbr_wc_gen_id(action_list);
    
    input_state_assert_winner_nbr_wc_gen_id_changed(action_list);
}

void
PimMreTrackState::track_state_receive_join_wc_by_sg_rpt(list<PimMreAction> action_list)
{
    action_list = output_state_receive_join_wc_by_sg_rpt(action_list);
    
    track_state_receive_join_wc(action_list);
}

// XXX: unused
void
PimMreTrackState::track_state_i_am_assert_winner_sg(list<PimMreAction> action_list)
{
    track_state_assert_state_sg(action_list);
}

// XXX: unused
void
PimMreTrackState::track_state_i_am_assert_winner_wc(list<PimMreAction> action_list)
{
    track_state_assert_state_wc(action_list);
}

void
PimMreTrackState::track_state_i_am_assert_loser_sg(list<PimMreAction> action_list)
{
    track_state_assert_state_sg(action_list);
}

void
PimMreTrackState::track_state_i_am_assert_loser_wc(list<PimMreAction> action_list)
{
    track_state_assert_state_wc(action_list);
}

void
PimMreTrackState::track_state_assert_winner_sg(list<PimMreAction> action_list)
{
    track_state_assert_state_sg(action_list);
}

void
PimMreTrackState::track_state_assert_winner_wc(list<PimMreAction> action_list)
{
    track_state_assert_state_wc(action_list);
}

void
PimMreTrackState::track_state_assert_winner_metric_sg(list<PimMreAction> action_list)
{
    track_state_assert_state_sg(action_list);
}

// XXX: unused
void
PimMreTrackState::track_state_assert_winner_metric_wc(list<PimMreAction> action_list)
{
    track_state_assert_state_wc(action_list);
}

void
PimMreTrackState::track_state_assert_winner_metric_is_better_than_spt_assert_metric_sg(list<PimMreAction> action_list)
{
    track_state_assert_state_sg(action_list);
    track_state_assert_winner_metric_sg(action_list);
    track_state_spt_assert_metric(action_list);
}

void
PimMreTrackState::track_state_is_directly_connected_s(list<PimMreAction> action_list)
{
    track_state_my_ip_address(action_list);
    track_state_my_ip_subnet_address(action_list);
}

void
PimMreTrackState::track_state_in_start_vif(list<PimMreAction> action_list)
{
    input_state_in_start_vif(action_list);
}

void
PimMreTrackState::track_state_in_stop_vif(list<PimMreAction> action_list)
{
    input_state_in_stop_vif(action_list);
}

void
PimMreTrackState::track_state_in_add_pim_mre_rp(list<PimMreAction> action_list)
{
    input_state_in_add_pim_mre_rp(action_list);
}

void
PimMreTrackState::track_state_in_add_pim_mre_wc(list<PimMreAction> action_list)
{
    input_state_in_add_pim_mre_wc(action_list);
}

void
PimMreTrackState::track_state_in_add_pim_mre_sg(list<PimMreAction> action_list)
{
    input_state_in_add_pim_mre_sg(action_list);
}

void
PimMreTrackState::track_state_in_add_pim_mre_sg_rpt(list<PimMreAction> action_list)
{
    input_state_in_add_pim_mre_sg_rpt(action_list);
}

void
PimMreTrackState::track_state_in_remove_pim_mre_rp(list<PimMreAction> action_list)
{
    input_state_in_remove_pim_mre_rp(action_list);
}

void
PimMreTrackState::track_state_in_remove_pim_mre_wc(list<PimMreAction> action_list)
{
    input_state_in_remove_pim_mre_wc(action_list);
}

void
PimMreTrackState::track_state_in_remove_pim_mre_sg(list<PimMreAction> action_list)
{
    input_state_in_remove_pim_mre_sg(action_list);
}

void
PimMreTrackState::track_state_in_remove_pim_mre_sg_rpt(list<PimMreAction> action_list)
{
    input_state_in_remove_pim_mre_sg_rpt(action_list);
}

void
PimMreTrackState::track_state_out_start_vif_rp(list<PimMreAction> action_list)
{
    action_list = output_state_out_start_vif_rp(action_list);
    
    track_state_in_start_vif(action_list);
}

void
PimMreTrackState::track_state_out_start_vif_wc(list<PimMreAction> action_list)
{
    action_list = output_state_out_start_vif_wc(action_list);
    
    track_state_in_start_vif(action_list);
}

void
PimMreTrackState::track_state_out_start_vif_sg(list<PimMreAction> action_list)
{
    action_list = output_state_out_start_vif_sg(action_list);
    
    track_state_in_start_vif(action_list);
}

void
PimMreTrackState::track_state_out_start_vif_sg_rpt(list<PimMreAction> action_list)
{
    action_list = output_state_out_start_vif_sg_rpt(action_list);
    
    track_state_in_start_vif(action_list);
}

void
PimMreTrackState::track_state_out_stop_vif_rp(list<PimMreAction> action_list)
{
    action_list = output_state_out_stop_vif_rp(action_list);
    
    track_state_in_stop_vif(action_list);
}

void
PimMreTrackState::track_state_out_stop_vif_wc(list<PimMreAction> action_list)
{
    action_list = output_state_out_stop_vif_wc(action_list);
    
    track_state_in_stop_vif(action_list);
}

void
PimMreTrackState::track_state_out_stop_vif_sg(list<PimMreAction> action_list)
{
    action_list = output_state_out_stop_vif_sg(action_list);
    
    track_state_in_stop_vif(action_list);
}

void
PimMreTrackState::track_state_out_stop_vif_sg_rpt(list<PimMreAction> action_list)
{
    action_list = output_state_out_stop_vif_sg_rpt(action_list);
    
    track_state_in_stop_vif(action_list);
}

void
PimMreTrackState::track_state_out_add_pim_mre_rp_entry_rp(list<PimMreAction> action_list)
{
    action_list = output_state_out_add_pim_mre_rp_entry_rp(action_list);
    
    track_state_in_add_pim_mre_rp(action_list);
}

void
PimMreTrackState::track_state_out_add_pim_mre_rp_entry_wc(list<PimMreAction> action_list)
{
    action_list = output_state_out_add_pim_mre_rp_entry_wc(action_list);
    
    track_state_in_add_pim_mre_rp(action_list);
}

void
PimMreTrackState::track_state_out_add_pim_mre_rp_entry_sg(list<PimMreAction> action_list)
{
    action_list = output_state_out_add_pim_mre_rp_entry_sg(action_list);
    
    track_state_in_add_pim_mre_rp(action_list);
}

void
PimMreTrackState::track_state_out_add_pim_mre_rp_entry_sg_rpt(list<PimMreAction> action_list)
{
    action_list = output_state_out_add_pim_mre_rp_entry_sg_rpt(action_list);
    
    track_state_in_add_pim_mre_rp(action_list);
}

void
PimMreTrackState::track_state_out_add_pim_mre_wc_entry_wc(list<PimMreAction> action_list)
{
    action_list = output_state_out_add_pim_mre_wc_entry_wc(action_list);
    
    track_state_in_add_pim_mre_wc(action_list);
}

void
PimMreTrackState::track_state_out_add_pim_mre_wc_entry_sg(list<PimMreAction> action_list)
{
    action_list = output_state_out_add_pim_mre_wc_entry_sg(action_list);
    
    track_state_in_add_pim_mre_wc(action_list);
}

void
PimMreTrackState::track_state_out_add_pim_mre_wc_entry_sg_rpt(list<PimMreAction> action_list)
{
    action_list = output_state_out_add_pim_mre_wc_entry_sg_rpt(action_list);
    
    track_state_in_add_pim_mre_wc(action_list);
}

void
PimMreTrackState::track_state_out_add_pim_mre_sg_entry_sg(list<PimMreAction> action_list)
{
    action_list = output_state_out_add_pim_mre_sg_entry_sg(action_list);
    
    track_state_in_add_pim_mre_sg(action_list);
}

void
PimMreTrackState::track_state_out_add_pim_mre_sg_entry_sg_rpt(list<PimMreAction> action_list)
{
    action_list = output_state_out_add_pim_mre_sg_entry_sg_rpt(action_list);
    
    track_state_in_add_pim_mre_sg(action_list);
}

void
PimMreTrackState::track_state_out_add_pim_mre_sg_rpt_entry_sg(list<PimMreAction> action_list)
{
    action_list = output_state_out_add_pim_mre_sg_rpt_entry_sg(action_list);
    
    track_state_in_add_pim_mre_sg_rpt(action_list);
}

void
PimMreTrackState::track_state_out_add_pim_mre_sg_rpt_entry_sg_rpt(list<PimMreAction> action_list)
{
    action_list = output_state_out_add_pim_mre_sg_rpt_entry_sg_rpt(action_list);
    
    track_state_in_add_pim_mre_sg_rpt(action_list);
}

void
PimMreTrackState::track_state_out_remove_pim_mre_rp_entry_rp(list<PimMreAction> action_list)
{
    action_list = output_state_out_remove_pim_mre_rp_entry_rp(action_list);
    
    track_state_in_remove_pim_mre_rp(action_list);
}

void
PimMreTrackState::track_state_out_remove_pim_mre_rp_entry_wc(list<PimMreAction> action_list)
{
    action_list = output_state_out_remove_pim_mre_rp_entry_wc(action_list);
    
    track_state_in_remove_pim_mre_rp(action_list);
}

void
PimMreTrackState::track_state_out_remove_pim_mre_rp_entry_sg(list<PimMreAction> action_list)
{
    action_list = output_state_out_remove_pim_mre_rp_entry_sg(action_list);
    
    track_state_in_remove_pim_mre_rp(action_list);
}

void
PimMreTrackState::track_state_out_remove_pim_mre_rp_entry_sg_rpt(list<PimMreAction> action_list)
{
    action_list = output_state_out_remove_pim_mre_rp_entry_sg_rpt(action_list);
    
    track_state_in_remove_pim_mre_rp(action_list);
}

void
PimMreTrackState::track_state_out_remove_pim_mre_wc_entry_wc(list<PimMreAction> action_list)
{
    action_list = output_state_out_remove_pim_mre_wc_entry_wc(action_list);
    
    track_state_in_remove_pim_mre_wc(action_list);
}

void
PimMreTrackState::track_state_out_remove_pim_mre_wc_entry_sg(list<PimMreAction> action_list)
{
    action_list = output_state_out_remove_pim_mre_wc_entry_sg(action_list);
    
    track_state_in_remove_pim_mre_wc(action_list);
}

void
PimMreTrackState::track_state_out_remove_pim_mre_wc_entry_sg_rpt(list<PimMreAction> action_list)
{
    action_list = output_state_out_remove_pim_mre_wc_entry_sg_rpt(action_list);
    
    track_state_in_remove_pim_mre_wc(action_list);
}

void
PimMreTrackState::track_state_out_remove_pim_mre_sg_entry_sg(list<PimMreAction> action_list)
{
    action_list = output_state_out_remove_pim_mre_sg_entry_sg(action_list);
    
    track_state_in_remove_pim_mre_sg(action_list);
}

void
PimMreTrackState::track_state_out_remove_pim_mre_sg_entry_sg_rpt(list<PimMreAction> action_list)
{
    action_list = output_state_out_remove_pim_mre_sg_entry_sg_rpt(action_list);
    
    track_state_in_remove_pim_mre_sg(action_list);
}

void
PimMreTrackState::track_state_out_remove_pim_mre_sg_rpt_entry_sg(list<PimMreAction> action_list)
{
    action_list = output_state_out_remove_pim_mre_sg_rpt_entry_sg(action_list);
    
    track_state_in_remove_pim_mre_sg_rpt(action_list);
}

void
PimMreTrackState::track_state_out_remove_pim_mre_sg_rpt_entry_sg_rpt(list<PimMreAction> action_list)
{
    action_list = output_state_out_remove_pim_mre_sg_rpt_entry_sg_rpt(action_list);
    
    track_state_in_remove_pim_mre_sg_rpt(action_list);
}


void
PimMreAction::perform_action(PimMre& pim_mre, uint16_t vif_index,
			     const IPvX& addr_arg)
{
    uint16_t i, maxvifs;
    
    switch (output_state()) {
	
    case PimMreTrackState::OUTPUT_STATE_RP_WC:				// 0
	pim_mre.recompute_rp_wc();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_RP_SG:				// 1
	pim_mre.recompute_rp_sg();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_RP_SG_RPT:			// 2
	pim_mre.recompute_rp_sg_rpt();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_RP_MFC:				// 3
	XLOG_ASSERT(false);
	// pim_mfc.recompute_rp_mfc();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_MRIB_RP_RP:			// 4
	pim_mre.recompute_mrib_rp_rp();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_MRIB_RP_WC:			// 5
	pim_mre.recompute_mrib_rp_wc();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_MRIB_RP_SG:			// 6
	pim_mre.recompute_mrib_rp_sg();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_MRIB_RP_SG_RPT:			// 7
	pim_mre.recompute_mrib_rp_sg_rpt();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_MRIB_S_SG:			// 8
	pim_mre.recompute_mrib_s_sg();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_MRIB_S_SG_RPT:			// 9
	pim_mre.recompute_mrib_s_sg_rpt();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_IS_JOIN_DESIRED_RP:		// 10
	pim_mre.recompute_is_join_desired_rp();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_IS_JOIN_DESIRED_WC:		// 11
	pim_mre.recompute_is_join_desired_wc();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_IS_JOIN_DESIRED_SG:		// 12
	pim_mre.recompute_is_join_desired_sg();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_IS_PRUNE_DESIRED_SG_RPT:	// 13
	pim_mre.recompute_is_prune_desired_sg_rpt();
	break;

    case PimMreTrackState::OUTPUT_STATE_IS_PRUNE_DESIRED_SG_RPT_SG:	// 14
	pim_mre.recompute_is_prune_desired_sg_rpt_sg();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_IS_RPT_JOIN_DESIRED_G:		// 15
	pim_mre.recompute_is_rpt_join_desired_g();
	break;

    case PimMreTrackState::OUTPUT_STATE_INHERITED_OLIST_SG_RPT:		// 16
	pim_mre.recompute_inherited_olist_sg_rpt();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_IIF_OLIST_MFC:			// 17
	XLOG_ASSERT(false);
	// pim_mfc.recompute_iif_olist_mfc();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_IS_COULD_REGISTER_SG:		// 18
	pim_mre.recompute_is_could_register_sg();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_ASSERT_TRACKING_DESIRED_SG:	// 19
	pim_mre.recompute_assert_tracking_desired_sg();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_ASSERT_TRACKING_DESIRED_WC:	// 20
	pim_mre.recompute_assert_tracking_desired_wc();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_COULD_ASSERT_SG:		// 21
	pim_mre.recompute_could_assert_sg();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_COULD_ASSERT_WC:		// 22
	pim_mre.recompute_could_assert_wc();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_MY_ASSERT_METRIC_SG:		// 23
	if (vif_index != Vif::VIF_INDEX_INVALID) {
	    pim_mre.recompute_my_assert_metric_sg(vif_index);
	} else {
	    maxvifs = pim_mre.pim_node().maxvifs();
    	    for (i = 0; i < maxvifs; i++)
		pim_mre.recompute_my_assert_metric_sg(i);
	}
	break;
	
    case PimMreTrackState::OUTPUT_STATE_MY_ASSERT_METRIC_WC:		// 24
	if (vif_index != Vif::VIF_INDEX_INVALID) {
	    pim_mre.recompute_my_assert_metric_wc(vif_index);
	} else {
	    maxvifs = pim_mre.pim_node().maxvifs();
    	    for (i = 0; i < maxvifs; i++)
		pim_mre.recompute_my_assert_metric_wc(i);
	}
	break;
	
    case PimMreTrackState::OUTPUT_STATE_ASSERT_RPF_INTERFACE_SG:	// 25
	XLOG_ASSERT(vif_index != Vif::VIF_INDEX_INVALID);
	pim_mre.recompute_assert_rpf_interface_sg(vif_index);
	break;
	
    case PimMreTrackState::OUTPUT_STATE_ASSERT_RPF_INTERFACE_WC:	// 26
	XLOG_ASSERT(vif_index != Vif::VIF_INDEX_INVALID);
	pim_mre.recompute_assert_rpf_interface_wc(vif_index);
	break;
	break;
	
    case PimMreTrackState::OUTPUT_STATE_ASSERT_RECEIVE_JOIN_SG:		// 27
	XLOG_ASSERT(vif_index != Vif::VIF_INDEX_INVALID);
	pim_mre.recompute_assert_receive_join_sg(vif_index);
	break;
	
    case PimMreTrackState::OUTPUT_STATE_ASSERT_RECEIVE_JOIN_WC:		// 28
	XLOG_ASSERT(vif_index != Vif::VIF_INDEX_INVALID);
	pim_mre.recompute_assert_receive_join_wc(vif_index);
	break;

    case PimMreTrackState::OUTPUT_STATE_ASSERT_WINNER_NBR_SG_GEN_ID:	// 29
	XLOG_ASSERT(vif_index != Vif::VIF_INDEX_INVALID);
	pim_mre.recompute_assert_winner_nbr_sg_gen_id_changed(vif_index,
							      addr_arg);
	break;
	
    case PimMreTrackState::OUTPUT_STATE_ASSERT_WINNER_NBR_WC_GEN_ID:	// 30
	XLOG_ASSERT(vif_index != Vif::VIF_INDEX_INVALID);
	pim_mre.recompute_assert_winner_nbr_wc_gen_id_changed(vif_index,
							      addr_arg);
	break;
	
    case PimMreTrackState::OUTPUT_STATE_RECEIVE_JOIN_WC_BY_SG_RPT:	// 31
	XLOG_ASSERT(vif_index != Vif::VIF_INDEX_INVALID);
	pim_mre.receive_join_wc_by_sg_rpt(vif_index);
	break;
	
    case PimMreTrackState::OUTPUT_STATE_RECEIVE_END_OF_MESSAGE_SG_RPT:	// 32
	pim_mre.receive_end_of_message_sg_rpt(vif_index);
	break;
	
    case PimMreTrackState::OUTPUT_STATE_SG_SEE_PRUNE_WC:		// 33
	pim_mre.sg_see_prune_wc(vif_index, addr_arg);
	break;
	
    case PimMreTrackState::OUTPUT_STATE_CHECK_SWITCH_TO_SPT_SG:		// 34
	pim_mre.recompute_check_switch_to_spt_sg();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_RPFP_NBR_WC:			// 35
	pim_mre.recompute_rpfp_nbr_wc_changed();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_RPFP_NBR_WC_GEN_ID:		// 36
	pim_mre.recompute_rpfp_nbr_wc_gen_id_changed();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_RPFP_NBR_SG:			// 37
	pim_mre.recompute_rpfp_nbr_sg_changed();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_RPFP_NBR_SG_GEN_ID:		// 38
	pim_mre.recompute_rpfp_nbr_sg_gen_id_changed();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_RPFP_NBR_SG_RPT:		// 39
	pim_mre.recompute_rpfp_nbr_sg_rpt_changed();
	break;

    case PimMreTrackState::OUTPUT_STATE_RPFP_NBR_SG_RPT_SG:		// 40
	pim_mre.recompute_rpfp_nbr_sg_rpt_sg_changed();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_MRIB_NEXT_HOP_RP:		// 41
	pim_mre.recompute_mrib_next_hop_rp_changed();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_MRIB_NEXT_HOP_RP_GEN_ID:	// 42
	pim_mre.recompute_mrib_next_hop_rp_gen_id_changed();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_MRIB_NEXT_HOP_RP_G:		// 43
	pim_mre.recompute_mrib_next_hop_rp_g_changed();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_MRIB_NEXT_HOP_S:		// 44
	pim_mre.recompute_mrib_next_hop_s_changed();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_START_VIF_RP:		// 45
	pim_mre.recompute_start_vif_rp(vif_index);
	break;

    case PimMreTrackState::OUTPUT_STATE_OUT_START_VIF_WC:		// 46
	pim_mre.recompute_start_vif_wc(vif_index);
	break;

    case PimMreTrackState::OUTPUT_STATE_OUT_START_VIF_SG:		// 47
	pim_mre.recompute_start_vif_sg(vif_index);
	break;

    case PimMreTrackState::OUTPUT_STATE_OUT_START_VIF_SG_RPT:		// 48
	pim_mre.recompute_start_vif_sg_rpt(vif_index);
	break;

    case PimMreTrackState::OUTPUT_STATE_OUT_STOP_VIF_RP:		// 49
	pim_mre.recompute_stop_vif_rp(vif_index);
	break;

    case PimMreTrackState::OUTPUT_STATE_OUT_STOP_VIF_WC:		// 50
	pim_mre.recompute_stop_vif_wc(vif_index);
	break;

    case PimMreTrackState::OUTPUT_STATE_OUT_STOP_VIF_SG:		// 51
	pim_mre.recompute_stop_vif_sg(vif_index);
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_STOP_VIF_SG_RPT:		// 52
	pim_mre.recompute_stop_vif_sg_rpt(vif_index);
	break;

    case PimMreTrackState::OUTPUT_STATE_OUT_ADD_PIM_MRE_RP_ENTRY_RP:	// 53
	pim_mre.add_pim_mre_rp_entry();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_ADD_PIM_MRE_RP_ENTRY_WC:	// 54
	pim_mre.add_pim_mre_rp_entry();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_ADD_PIM_MRE_RP_ENTRY_SG:	// 55
	pim_mre.add_pim_mre_rp_entry();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_ADD_PIM_MRE_RP_ENTRY_SG_RPT: // 56
	pim_mre.add_pim_mre_rp_entry();
	break;

    case PimMreTrackState::OUTPUT_STATE_OUT_ADD_PIM_MRE_WC_ENTRY_WC:	// 57
	pim_mre.add_pim_mre_wc_entry();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_ADD_PIM_MRE_WC_ENTRY_SG:	// 58
	pim_mre.add_pim_mre_wc_entry();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_ADD_PIM_MRE_WC_ENTRY_SG_RPT: // 59
	pim_mre.add_pim_mre_wc_entry();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_ADD_PIM_MRE_SG_ENTRY_SG:	// 60
	pim_mre.add_pim_mre_sg_entry();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_ADD_PIM_MRE_SG_ENTRY_SG_RPT: // 61
	pim_mre.add_pim_mre_sg_entry();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_ADD_PIM_MRE_SG_RPT_ENTRY_SG: // 62
	pim_mre.add_pim_mre_sg_rpt_entry();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_ADD_PIM_MRE_SG_RPT_ENTRY_SG_RPT: // 63
	pim_mre.add_pim_mre_sg_rpt_entry();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_REMOVE_PIM_MRE_RP_ENTRY_RP:	// 64
	pim_mre.remove_pim_mre_rp_entry();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_REMOVE_PIM_MRE_RP_ENTRY_WC:	// 65
	pim_mre.remove_pim_mre_rp_entry();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_REMOVE_PIM_MRE_RP_ENTRY_SG:	// 66
	pim_mre.remove_pim_mre_rp_entry();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_REMOVE_PIM_MRE_RP_ENTRY_SG_RPT: // 67
	pim_mre.remove_pim_mre_rp_entry();
	break;

    case PimMreTrackState::OUTPUT_STATE_OUT_REMOVE_PIM_MRE_WC_ENTRY_WC:	// 68
	pim_mre.remove_pim_mre_wc_entry();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_REMOVE_PIM_MRE_WC_ENTRY_SG:	// 69
	pim_mre.remove_pim_mre_wc_entry();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_REMOVE_PIM_MRE_WC_ENTRY_SG_RPT: // 70
	pim_mre.remove_pim_mre_wc_entry();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_REMOVE_PIM_MRE_SG_ENTRY_SG:	// 71
	pim_mre.remove_pim_mre_sg_entry();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_REMOVE_PIM_MRE_SG_ENTRY_SG_RPT: // 72
	pim_mre.remove_pim_mre_sg_entry();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_REMOVE_PIM_MRE_SG_RPT_ENTRY_SG: // 73
	pim_mre.remove_pim_mre_sg_rpt_entry();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_OUT_REMOVE_PIM_MRE_SG_RPT_ENTRY_SG_RPT: // 74
	pim_mre.remove_pim_mre_sg_rpt_entry();
	break;
	
    default:
	XLOG_ASSERT(false);
	break;
    }
}

void
PimMreAction::perform_action(PimMfc& pim_mfc)
{
    switch (output_state()) {
	
    case PimMreTrackState::OUTPUT_STATE_RP_MFC:				// 3
	pim_mfc.recompute_rp_mfc();
	break;
	
    case PimMreTrackState::OUTPUT_STATE_IIF_OLIST_MFC:			// 17
	pim_mfc.recompute_iif_olist_mfc();
	break;
	
    default:
	XLOG_ASSERT(false);
	break;
    }
}
