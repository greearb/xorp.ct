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

// $XORP: xorp/pim/pim_mre_track_state.hh,v 1.3 2003/01/23 05:01:27 pavlin Exp $


#ifndef __PIM_PIM_MRE_TRACK_STATE_HH__
#define __PIM_PIM_MRE_TRACK_STATE_HH__


//
// PIM Multicast Routing Entry state tracking definitions.
//


#include <list>
#include <vector>

#include "pim_mre.hh"


//
// Constants definitions
//


//
// Structures/classes, typedefs and macros
//

class PimMfc;
class PimMreAction;
class PimMrt;
class PimNbr;

// State tracking for PIM-specific Multicast Routing Entry
class PimMreTrackState {
public:
    PimMreTrackState(PimMrt& pim_mrt);

    // General info: PimNode, PimMrt, family, etc.
    PimNode&	pim_node()	const;
    PimMrt&	pim_mrt()	const	{ return (_pim_mrt);		}
    int		family()	const;
    
    void	print_actions_name() const;
    void	print_actions_num() const;
    
    //
    // The input state
    //
    enum input_state_t {
	INPUT_STATE_RP_CHANGED = 0,			// 0
	INPUT_STATE_MRIB_RP_CHANGED,			// 1
	INPUT_STATE_MRIB_S_CHANGED,			// 2
	INPUT_STATE_MRIB_NEXT_HOP_RP_CHANGED,		// 3
	INPUT_STATE_MRIB_NEXT_HOP_RP_GEN_ID_CHANGED,	// 4
	INPUT_STATE_MRIB_NEXT_HOP_RP_G_CHANGED,		// 5
	INPUT_STATE_MRIB_NEXT_HOP_S_CHANGED,		// 6
	INPUT_STATE_RPFP_NBR_WC_CHANGED,		// 7
	INPUT_STATE_RPFP_NBR_WC_GEN_ID_CHANGED,		// 8
	INPUT_STATE_RPFP_NBR_SG_CHANGED,		// 9
	INPUT_STATE_RPFP_NBR_SG_GEN_ID_CHANGED,		// 10
	INPUT_STATE_RPFP_NBR_SG_RPT_CHANGED,		// 11
	INPUT_STATE_RECEIVE_JOIN_RP,			// 12
	INPUT_STATE_RECEIVE_JOIN_WC,			// 13
	INPUT_STATE_RECEIVE_JOIN_SG,			// 14
	INPUT_STATE_RECEIVE_JOIN_SG_RPT,		// 15
	INPUT_STATE_RECEIVE_PRUNE_RP,			// 16
	INPUT_STATE_RECEIVE_PRUNE_WC,			// 17
	INPUT_STATE_RECEIVE_PRUNE_SG,			// 18
	INPUT_STATE_RECEIVE_PRUNE_SG_RPT,		// 19
	INPUT_STATE_RECEIVE_END_OF_MESSAGE_SG_RPT,	// 20
	INPUT_STATE_SEE_PRUNE_WC,			// 21
	INPUT_STATE_DOWNSTREAM_JP_STATE_RP,		// 22
	INPUT_STATE_DOWNSTREAM_JP_STATE_WC,		// 23
	INPUT_STATE_DOWNSTREAM_JP_STATE_SG,		// 24
	INPUT_STATE_DOWNSTREAM_JP_STATE_SG_RPT,		// 25
	INPUT_STATE_UPSTREAM_JP_STATE_SG,		// 26
	INPUT_STATE_LOCAL_RECEIVER_INCLUDE_WC,		// 27
	INPUT_STATE_LOCAL_RECEIVER_INCLUDE_SG,		// 28
	INPUT_STATE_LOCAL_RECEIVER_EXCLUDE_SG,		// 29
	INPUT_STATE_ASSERT_STATE_WC,			// 30
	INPUT_STATE_ASSERT_STATE_SG,			// 31
	INPUT_STATE_I_AM_DR,				// 32
	INPUT_STATE_MY_IP_ADDRESS,			// 33
	INPUT_STATE_MY_IP_SUBNET_ADDRESS,		// 34
	INPUT_STATE_IS_SWITCH_TO_SPT_DESIRED_SG,	// 35
	INPUT_STATE_KEEPALIVE_TIMER_SG,			// 36
	INPUT_STATE_SPTBIT_SG,				// 37
	INPUT_STATE_IN_START_VIF,			// 38
	INPUT_STATE_IN_STOP_VIF,			// 39
	INPUT_STATE_IN_ADD_PIM_MRE_RP,			// 40
	INPUT_STATE_IN_ADD_PIM_MRE_WC,			// 41
	INPUT_STATE_IN_ADD_PIM_MRE_SG,			// 42
	INPUT_STATE_IN_ADD_PIM_MRE_SG_RPT,		// 43
	INPUT_STATE_IN_REMOVE_PIM_MRE_RP,		// 44
	INPUT_STATE_IN_REMOVE_PIM_MRE_WC,		// 45
	INPUT_STATE_IN_REMOVE_PIM_MRE_SG,		// 46
	INPUT_STATE_IN_REMOVE_PIM_MRE_SG_RPT,		// 47
	INPUT_STATE_MAX
    };
    //
    // The output state
    //
    enum output_state_t {
	OUTPUT_STATE_RP_WC = 0,				// 0
	OUTPUT_STATE_RP_SG,				// 1
	OUTPUT_STATE_RP_SG_RPT,				// 2
	OUTPUT_STATE_RP_MFC,				// 3
	OUTPUT_STATE_MRIB_RP_RP,			// 4
	OUTPUT_STATE_MRIB_RP_WC,			// 5
	OUTPUT_STATE_MRIB_RP_SG,			// 6
	OUTPUT_STATE_MRIB_RP_SG_RPT,			// 7
	OUTPUT_STATE_MRIB_S_SG,				// 8
	OUTPUT_STATE_MRIB_S_SG_RPT,			// 9
	OUTPUT_STATE_IS_JOIN_DESIRED_RP,		// 10
	OUTPUT_STATE_IS_JOIN_DESIRED_WC,		// 11
	OUTPUT_STATE_IS_JOIN_DESIRED_SG,		// 12
	OUTPUT_STATE_IS_PRUNE_DESIRED_SG_RPT,		// 13
	OUTPUT_STATE_IS_PRUNE_DESIRED_SG_RPT_SG,	// 14
	OUTPUT_STATE_IS_RPT_JOIN_DESIRED_G,		// 15
	OUTPUT_STATE_INHERITED_OLIST_SG_RPT,		// 16
	OUTPUT_STATE_IIF_OLIST_MFC,			// 17
	OUTPUT_STATE_RP_REGISTER_SG_CHANGED,		// 18
	OUTPUT_STATE_IS_COULD_REGISTER_SG,		// 19
	OUTPUT_STATE_ASSERT_TRACKING_DESIRED_SG,	// 20
	OUTPUT_STATE_ASSERT_TRACKING_DESIRED_WC,	// 21
	OUTPUT_STATE_COULD_ASSERT_SG,			// 22
	OUTPUT_STATE_COULD_ASSERT_WC,			// 23
	OUTPUT_STATE_MY_ASSERT_METRIC_SG,		// 24
	OUTPUT_STATE_MY_ASSERT_METRIC_WC,		// 25
	OUTPUT_STATE_ASSERT_RPF_INTERFACE_SG,		// 26
	OUTPUT_STATE_ASSERT_RPF_INTERFACE_WC,		// 27
	OUTPUT_STATE_ASSERT_RECEIVE_JOIN_SG,		// 28
	OUTPUT_STATE_ASSERT_RECEIVE_JOIN_WC,		// 29
	OUTPUT_STATE_RECEIVE_JOIN_WC_BY_SG_RPT,		// 30
	OUTPUT_STATE_RECEIVE_END_OF_MESSAGE_SG_RPT,	// 31
	OUTPUT_STATE_SG_SEE_PRUNE_WC,			// 32
	OUTPUT_STATE_CHECK_SWITCH_TO_SPT_SG,		// 33
	OUTPUT_STATE_RPFP_NBR_WC,			// 34
	OUTPUT_STATE_RPFP_NBR_WC_GEN_ID,		// 35
	OUTPUT_STATE_RPFP_NBR_SG,			// 36
	OUTPUT_STATE_RPFP_NBR_SG_GEN_ID,		// 37
	OUTPUT_STATE_RPFP_NBR_SG_RPT,			// 38
	OUTPUT_STATE_RPFP_NBR_SG_RPT_SG,		// 39
	OUTPUT_STATE_MRIB_NEXT_HOP_RP,			// 40
	OUTPUT_STATE_MRIB_NEXT_HOP_RP_GEN_ID,		// 41
	OUTPUT_STATE_MRIB_NEXT_HOP_RP_G,		// 42
	OUTPUT_STATE_MRIB_NEXT_HOP_S,			// 43
	OUTPUT_STATE_OUT_START_VIF_RP,			// 44
	OUTPUT_STATE_OUT_START_VIF_WC,			// 45
	OUTPUT_STATE_OUT_START_VIF_SG,			// 46
	OUTPUT_STATE_OUT_START_VIF_SG_RPT,		// 47
	OUTPUT_STATE_OUT_STOP_VIF_RP,			// 48
	OUTPUT_STATE_OUT_STOP_VIF_WC,			// 49
	OUTPUT_STATE_OUT_STOP_VIF_SG,			// 50
	OUTPUT_STATE_OUT_STOP_VIF_SG_RPT,		// 51
	OUTPUT_STATE_OUT_ADD_PIM_MRE_RP_ENTRY_RP,	// 52
	OUTPUT_STATE_OUT_ADD_PIM_MRE_RP_ENTRY_WC,	// 53
	OUTPUT_STATE_OUT_ADD_PIM_MRE_RP_ENTRY_SG,	// 54
	OUTPUT_STATE_OUT_ADD_PIM_MRE_RP_ENTRY_SG_RPT,	// 55
	OUTPUT_STATE_OUT_ADD_PIM_MRE_WC_ENTRY_WC,	// 56
	OUTPUT_STATE_OUT_ADD_PIM_MRE_WC_ENTRY_SG,	// 57
	OUTPUT_STATE_OUT_ADD_PIM_MRE_WC_ENTRY_SG_RPT,	// 58
	OUTPUT_STATE_OUT_ADD_PIM_MRE_SG_ENTRY_SG,	// 59
	OUTPUT_STATE_OUT_ADD_PIM_MRE_SG_ENTRY_SG_RPT,	// 60
	OUTPUT_STATE_OUT_ADD_PIM_MRE_SG_RPT_ENTRY_SG,	// 61
	OUTPUT_STATE_OUT_ADD_PIM_MRE_SG_RPT_ENTRY_SG_RPT,// 62
	OUTPUT_STATE_OUT_REMOVE_PIM_MRE_RP_ENTRY_RP,	// 63
	OUTPUT_STATE_OUT_REMOVE_PIM_MRE_RP_ENTRY_WC,	// 64
	OUTPUT_STATE_OUT_REMOVE_PIM_MRE_RP_ENTRY_SG,	// 65
	OUTPUT_STATE_OUT_REMOVE_PIM_MRE_RP_ENTRY_SG_RPT,// 66
	OUTPUT_STATE_OUT_REMOVE_PIM_MRE_WC_ENTRY_WC,	// 67
	OUTPUT_STATE_OUT_REMOVE_PIM_MRE_WC_ENTRY_SG,	// 68
	OUTPUT_STATE_OUT_REMOVE_PIM_MRE_WC_ENTRY_SG_RPT,// 69
	OUTPUT_STATE_OUT_REMOVE_PIM_MRE_SG_ENTRY_SG,	// 70
	OUTPUT_STATE_OUT_REMOVE_PIM_MRE_SG_ENTRY_SG_RPT,// 71
	OUTPUT_STATE_OUT_REMOVE_PIM_MRE_SG_RPT_ENTRY_SG,// 72
	OUTPUT_STATE_OUT_REMOVE_PIM_MRE_SG_RPT_ENTRY_SG_RPT,// 73
	OUTPUT_STATE_MAX
    };
    
    //
    // The input state methods
    //
    void	input_state_rp_changed(list<PimMreAction> action_list);
    void	input_state_mrib_rp_changed(list<PimMreAction> action_list);
    void	input_state_mrib_s_changed(list<PimMreAction> action_list);
    void	input_state_mrib_next_hop_rp_changed(list<PimMreAction> action_list);
    void	input_state_mrib_next_hop_rp_gen_id_changed(list<PimMreAction> action_list);
    void	input_state_mrib_next_hop_rp_g_changed(list<PimMreAction> action_list);
    void	input_state_mrib_next_hop_s_changed(list<PimMreAction> action_list);
    void	input_state_rpfp_nbr_wc_changed(list<PimMreAction> action_list);
    void	input_state_rpfp_nbr_wc_gen_id_changed(list<PimMreAction> action_list);
    void	input_state_rpfp_nbr_sg_changed(list<PimMreAction> action_list);
    void	input_state_rpfp_nbr_sg_gen_id_changed(list<PimMreAction> action_list);
    void	input_state_rpfp_nbr_sg_rpt_changed(list<PimMreAction> action_list);
    void	input_state_receive_join_rp(list<PimMreAction> action_list);
    void	input_state_receive_join_wc(list<PimMreAction> action_list);
    void	input_state_receive_join_sg(list<PimMreAction> action_list);
    void	input_state_receive_join_sg_rpt(list<PimMreAction> action_list);
    void	input_state_receive_prune_rp(list<PimMreAction> action_list);
    void	input_state_receive_prune_wc(list<PimMreAction> action_list);
    void	input_state_receive_prune_sg(list<PimMreAction> action_list);
    void	input_state_receive_prune_sg_rpt(list<PimMreAction> action_list);
    void	input_state_receive_end_of_message_sg_rpt(list<PimMreAction> action_list);
    void	input_state_see_prune_wc(list<PimMreAction> action_list);
    void	input_state_downstream_jp_state_rp(list<PimMreAction> action_list);
    void	input_state_downstream_jp_state_wc(list<PimMreAction> action_list);
    void	input_state_downstream_jp_state_sg(list<PimMreAction> action_list);
    void	input_state_downstream_jp_state_sg_rpt(list<PimMreAction> action_list);
    void	input_state_upstream_jp_state_sg(list<PimMreAction> action_list);
    void	input_state_local_receiver_include_wc(list<PimMreAction> action_list);
    void	input_state_local_receiver_include_sg(list<PimMreAction> action_list);
    void	input_state_local_receiver_exclude_sg(list<PimMreAction> action_list);
    void	input_state_assert_state_wc(list<PimMreAction> action_list);
    void	input_state_assert_state_sg(list<PimMreAction> action_list);
    void	input_state_i_am_dr(list<PimMreAction> action_list);
    void	input_state_my_ip_address(list<PimMreAction> action_list);
    void	input_state_my_ip_subnet_address(list<PimMreAction> action_list);
    void	input_state_is_switch_to_spt_desired_sg(list<PimMreAction> action_list);
    void	input_state_keepalive_timer_sg(list<PimMreAction> action_list);
    void	input_state_sptbit_sg(list<PimMreAction> action_list);
    void	input_state_in_start_vif(list<PimMreAction> action_list);
    void	input_state_in_stop_vif(list<PimMreAction> action_list);
    void	input_state_in_add_pim_mre_rp(list<PimMreAction> action_list);
    void	input_state_in_add_pim_mre_wc(list<PimMreAction> action_list);
    void	input_state_in_add_pim_mre_sg(list<PimMreAction> action_list);
    void	input_state_in_add_pim_mre_sg_rpt(list<PimMreAction> action_list);
    void	input_state_in_remove_pim_mre_rp(list<PimMreAction> action_list);
    void	input_state_in_remove_pim_mre_wc(list<PimMreAction> action_list);
    void	input_state_in_remove_pim_mre_sg(list<PimMreAction> action_list);
    void	input_state_in_remove_pim_mre_sg_rpt(list<PimMreAction> action_list);
    
    
    //
    // The output state methods
    //
    list<PimMreAction>	output_state_rp_wc(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_rp_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_rp_sg_rpt(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_rp_mfc(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_mrib_rp_rp(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_mrib_rp_wc(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_mrib_rp_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_mrib_rp_sg_rpt(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_mrib_s_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_mrib_s_sg_rpt(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_is_join_desired_rp(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_is_join_desired_wc(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_is_join_desired_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_is_prune_desired_sg_rpt(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_is_prune_desired_sg_rpt_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_is_rpt_join_desired_g(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_inherited_olist_sg_rpt(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_iif_olist_mfc(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_rp_register_sg_changed(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_is_could_register_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_assert_tracking_desired_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_assert_tracking_desired_wc(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_could_assert_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_could_assert_wc(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_my_assert_metric_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_my_assert_metric_wc(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_assert_rpf_interface_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_assert_rpf_interface_wc(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_assert_receive_join_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_assert_receive_join_wc(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_receive_join_wc_by_sg_rpt(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_receive_end_of_message_sg_rpt(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_sg_see_prune_wc(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_check_switch_to_spt_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_rpfp_nbr_wc(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_rpfp_nbr_wc_gen_id(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_rpfp_nbr_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_rpfp_nbr_sg_gen_id(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_rpfp_nbr_sg_rpt(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_rpfp_nbr_sg_rpt_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_mrib_next_hop_rp(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_mrib_next_hop_rp_gen_id(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_mrib_next_hop_rp_g(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_mrib_next_hop_s(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_start_vif_rp(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_start_vif_wc(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_start_vif_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_start_vif_sg_rpt(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_stop_vif_rp(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_stop_vif_wc(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_stop_vif_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_stop_vif_sg_rpt(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_add_pim_mre_rp_entry_rp(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_add_pim_mre_rp_entry_wc(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_add_pim_mre_rp_entry_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_add_pim_mre_rp_entry_sg_rpt(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_add_pim_mre_wc_entry_wc(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_add_pim_mre_wc_entry_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_add_pim_mre_wc_entry_sg_rpt(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_add_pim_mre_sg_entry_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_add_pim_mre_sg_entry_sg_rpt(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_add_pim_mre_sg_rpt_entry_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_add_pim_mre_sg_rpt_entry_sg_rpt(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_remove_pim_mre_rp_entry_rp(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_remove_pim_mre_rp_entry_wc(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_remove_pim_mre_rp_entry_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_remove_pim_mre_rp_entry_sg_rpt(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_remove_pim_mre_wc_entry_wc(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_remove_pim_mre_wc_entry_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_remove_pim_mre_wc_entry_sg_rpt(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_remove_pim_mre_sg_entry_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_remove_pim_mre_sg_entry_sg_rpt(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_remove_pim_mre_sg_rpt_entry_sg(list<PimMreAction> action_list);
    list<PimMreAction>	output_state_out_remove_pim_mre_sg_rpt_entry_sg_rpt(list<PimMreAction> action_list);
    
    //
    // The output actions
    //
    const list<PimMreAction>& output_action_rp(input_state_t input_state) const {
	return (_output_action_rp[input_state]);
    }
    const list<PimMreAction>& output_action_wc(input_state_t input_state) const {
	return (_output_action_wc[input_state]);
    }
    const list<PimMreAction>& output_action_sg_sg_rpt(input_state_t input_state) const {
	return (_output_action_sg_sg_rpt[input_state]);
    }
    const list<PimMreAction>& output_action_mfc(input_state_t input_state) const {
	return (_output_action_mfc[input_state]);
    }
    
    //
    // The remove state methods
    //
    list<PimMreAction>	remove_state(list<PimMreAction> action_list);
    list<PimMreAction>	remove_state_mrib_next_hop_rp_g_changed(list<PimMreAction> action_list);
    list<PimMreAction>	remove_state_mrib_next_hop_s_changed(list<PimMreAction> action_list);
    
    
private:
    //
    // Methods related to actions
    // 
    int		add_action_list(input_state_t input_state,
				list<PimMreAction> action_list);
    int		add_action(input_state_t input_state,
			   const PimMreAction& action);
    bool	can_add_action_to_list(const list<PimMreAction>& action_list,
				       const PimMreAction& action) const;
    list<PimMreAction>	remove_action_from_list(list<PimMreAction> action_list,
						PimMreAction keep_action,
						PimMreAction remove_action);
    
    // The list of the (*,*,RP) actions
    list<PimMreAction>	_output_action_rp[INPUT_STATE_MAX];
    // The list of the (*,G) actions
    list<PimMreAction>	_output_action_wc[INPUT_STATE_MAX];
    // The list of the (S,G) and (S,G,rpt) actions (in order of appearance)
    list<PimMreAction>	_output_action_sg_sg_rpt[INPUT_STATE_MAX];
    // The list of the MFC actions
    list<PimMreAction>	_output_action_mfc[INPUT_STATE_MAX];
    // The list of all actions:
    // - The (*,*,RP) actions are first.
    // - The (*,G) actions follow the (*,*,RP) actions.
    // - The (S,G) and (S,G,rpt) actions (in order of appearance)
    //   follow the (*,G) actions.
    list<PimMreAction>	_output_action[INPUT_STATE_MAX];
    
    class ActionLists {
    public:
	void clear();
	void add_action_list(list<PimMreAction> action_list);
	list<PimMreAction> compute_action_list();
	
    private:
	PimMreAction	pop_next_action();
	bool		is_head_only_action(const PimMreAction& action) const;
	
	vector<list<PimMreAction> > _action_list_vector;
    };
    
    ActionLists		_action_lists[INPUT_STATE_MAX];
    
    //
    // The track state methods
    //
    // The RP entry
    void	track_state_rp(list<PimMreAction> action_list);
    void	track_state_rp_wc(list<PimMreAction> action_list);
    void	track_state_rp_sg(list<PimMreAction> action_list);
    void	track_state_rp_sg_rpt(list<PimMreAction> action_list);
    void	track_state_rp_mfc(list<PimMreAction> action_list);
    void	track_state_mrib_rp_rp(list<PimMreAction> action_list);
    void	track_state_mrib_rp_wc(list<PimMreAction> action_list);
    void	track_state_mrib_rp_sg(list<PimMreAction> action_list);
    void	track_state_mrib_rp_sg_rpt(list<PimMreAction> action_list);
    void	track_state_mrib_s_sg(list<PimMreAction> action_list);
    void	track_state_mrib_s_sg_rpt(list<PimMreAction> action_list);
    // MRIB info
    void	track_state_mrib_rp(list<PimMreAction> action_list);
    void	track_state_mrib_s(list<PimMreAction> action_list);
    void	track_state_rpf_interface_rp(list<PimMreAction> action_list);
    void	track_state_rpf_interface_s(list<PimMreAction> action_list);
    // RPF neighbor info
    void	track_state_mrib_next_hop_rp(list<PimMreAction> action_list);
    void	track_state_mrib_next_hop_rp_gen_id(list<PimMreAction> action_list);
    void	track_state_mrib_next_hop_rp_g(list<PimMreAction> action_list);
    void	track_state_mrib_next_hop_s(list<PimMreAction> action_list);
    void	track_state_mrib_pref_metric_s(list<PimMreAction> action_list);
    void	track_state_mrib_pref_metric_rp(list<PimMreAction> action_list);
    // JOIN/PRUNE info
    void	track_state_receive_join_rp(list<PimMreAction> action_list);
    void	track_state_receive_join_wc(list<PimMreAction> action_list);
    void	track_state_receive_join_sg(list<PimMreAction> action_list);
    void	track_state_receive_join_sg_rpt(list<PimMreAction> action_list);
    void	track_state_receive_prune_rp(list<PimMreAction> action_list);
    void	track_state_receive_prune_wc(list<PimMreAction> action_list);
    void	track_state_receive_prune_sg(list<PimMreAction> action_list);
    void	track_state_receive_prune_sg_rpt(list<PimMreAction> action_list);
    void	track_state_receive_end_of_message_sg_rpt(list<PimMreAction> action_list);
    void	track_state_sg_see_prune_wc(list<PimMreAction> action_list);
    // J/P (downstream) state (per interface)
    void	track_state_downstream_jp_state_rp(list<PimMreAction> action_list);
    void	track_state_downstream_jp_state_wc(list<PimMreAction> action_list);
    void	track_state_downstream_jp_state_sg(list<PimMreAction> action_list);
    void	track_state_downstream_jp_state_sg_rpt(list<PimMreAction> action_list);
    // J/P (upstream) state
    void	track_state_upstream_jp_state_sg(list<PimMreAction> action_list);
    // Local receivers info
    void	track_state_local_receiver_include_wc(list<PimMreAction> action_list);
    void	track_state_local_receiver_include_sg(list<PimMreAction> action_list);
    void	track_state_local_receiver_exclude_sg(list<PimMreAction> action_list);
    void	track_state_assert_state_sg(list<PimMreAction> action_list);
    void	track_state_assert_state_wc(list<PimMreAction> action_list);
    // MISC. info
    void	track_state_i_am_dr(list<PimMreAction> action_list);
    void	track_state_my_ip_address(list<PimMreAction> action_list);
    void	track_state_my_ip_subnet_address(list<PimMreAction> action_list);
    void	track_state_is_switch_to_spt_desired_sg(list<PimMreAction> action_list);
    // MISC. timers
    void	track_state_keepalive_timer_sg(list<PimMreAction> action_list);
    // J/P state recomputation
    void	track_state_immediate_olist_rp(list<PimMreAction> action_list);
    void	track_state_immediate_olist_wc(list<PimMreAction> action_list);
    void	track_state_immediate_olist_sg(list<PimMreAction> action_list);
    void	track_state_inherited_olist_sg_rpt(list<PimMreAction> action_list);
    void	track_state_inherited_olist_sg(list<PimMreAction> action_list);
    void	track_state_iif_olist_mfc(list<PimMreAction> action_list);
    void	track_state_pim_include_wc(list<PimMreAction> action_list);
    void	track_state_pim_include_sg(list<PimMreAction> action_list);
    void	track_state_pim_exclude_sg(list<PimMreAction> action_list);
    void	track_state_joins_rp(list<PimMreAction> action_list);
    void	track_state_joins_wc(list<PimMreAction> action_list);
    void	track_state_joins_sg(list<PimMreAction> action_list);
    void	track_state_prunes_sg_rpt(list<PimMreAction> action_list);
    void	track_state_rpfp_nbr_wc(list<PimMreAction> action_list);
    void	track_state_rpfp_nbr_wc_gen_id(list<PimMreAction> action_list);
    void	track_state_rpfp_nbr_sg_rpt(list<PimMreAction> action_list);
    void	track_state_rpfp_nbr_sg_rpt_sg(list<PimMreAction> action_list);
    void	track_state_rpfp_nbr_sg(list<PimMreAction> action_list);
    void	track_state_rpfp_nbr_sg_gen_id(list<PimMreAction> action_list);
    void	track_state_check_switch_to_spt_sg(list<PimMreAction> action_list);
    // Data
    void	track_state_sptbit_sg(list<PimMreAction> action_list);
    void	track_state_is_could_register_sg(list<PimMreAction> action_list);
    void	track_state_is_join_desired_rp(list<PimMreAction> action_list);
    void	track_state_is_join_desired_wc(list<PimMreAction> action_list);
    void	track_state_is_join_desired_sg(list<PimMreAction> action_list);
    void	track_state_is_prune_desired_sg_rpt(list<PimMreAction> action_list);
    void	track_state_is_prune_desired_sg_rpt_sg(list<PimMreAction> action_list);
    void	track_state_is_rpt_join_desired_g(list<PimMreAction> action_list);
    void	track_state_could_assert_sg(list<PimMreAction> action_list);
    void	track_state_assert_tracking_desired_sg(list<PimMreAction> action_list);
    void	track_state_could_assert_wc(list<PimMreAction> action_list);
    void	track_state_assert_tracking_desired_wc(list<PimMreAction> action_list);
    void	track_state_my_assert_metric_sg(list<PimMreAction> action_list);
    void	track_state_my_assert_metric_wc(list<PimMreAction> action_list);
    void	track_state_spt_assert_metric(list<PimMreAction> action_list);
    void	track_state_rpt_assert_metric(list<PimMreAction> action_list);
    void	track_state_lost_assert_sg_rpt(list<PimMreAction> action_list);
    void	track_state_lost_assert_sg(list<PimMreAction> action_list);
    void	track_state_lost_assert_wc(list<PimMreAction> action_list);
    void	track_state_assert_rpf_interface_sg(list<PimMreAction> action_list);
    void	track_state_assert_rpf_interface_wc(list<PimMreAction> action_list);
    void	track_state_assert_receive_join_sg(list<PimMreAction> action_list);
    void	track_state_assert_receive_join_wc(list<PimMreAction> action_list);
    void	track_state_receive_join_wc_by_sg_rpt(list<PimMreAction> action_list);
    void	track_state_i_am_assert_winner_sg(list<PimMreAction> action_list);
    void	track_state_i_am_assert_winner_wc(list<PimMreAction> action_list);
    void	track_state_i_am_assert_loser_sg(list<PimMreAction> action_list);
    void	track_state_i_am_assert_loser_wc(list<PimMreAction> action_list);
    void	track_state_assert_winner_sg(list<PimMreAction> action_list);
    void	track_state_assert_winner_wc(list<PimMreAction> action_list);
    void	track_state_assert_winner_metric_sg(list<PimMreAction> action_list);
    void	track_state_assert_winner_metric_wc(list<PimMreAction> action_list);
    void	track_state_assert_winner_metric_is_better_than_spt_assert_metric_sg(list<PimMreAction> action_list);
    void	track_state_rp_register_sg_changed(list<PimMreAction> action_list);
    // MISC. other stuff
    void	track_state_is_directly_connected_s(list<PimMreAction> action_list);
    void	track_state_in_start_vif(list<PimMreAction> action_list);
    void	track_state_in_stop_vif(list<PimMreAction> action_list);
    void	track_state_out_start_vif_rp(list<PimMreAction> action_list);
    void	track_state_out_start_vif_wc(list<PimMreAction> action_list);
    void	track_state_out_start_vif_sg(list<PimMreAction> action_list);
    void	track_state_out_start_vif_sg_rpt(list<PimMreAction> action_list);
    void	track_state_out_stop_vif_rp(list<PimMreAction> action_list);
    void	track_state_out_stop_vif_wc(list<PimMreAction> action_list);
    void	track_state_out_stop_vif_sg(list<PimMreAction> action_list);
    void	track_state_out_stop_vif_sg_rpt(list<PimMreAction> action_list);
    void	track_state_in_add_pim_mre_rp(list<PimMreAction> action_list);
    void	track_state_in_add_pim_mre_wc(list<PimMreAction> action_list);
    void	track_state_in_add_pim_mre_sg(list<PimMreAction> action_list);
    void	track_state_in_add_pim_mre_sg_rpt(list<PimMreAction> action_list);
    void	track_state_in_remove_pim_mre_rp(list<PimMreAction> action_list);
    void	track_state_in_remove_pim_mre_wc(list<PimMreAction> action_list);
    void	track_state_in_remove_pim_mre_sg(list<PimMreAction> action_list);
    void	track_state_in_remove_pim_mre_sg_rpt(list<PimMreAction> action_list);
    void	track_state_out_add_pim_mre_rp_entry_rp(list<PimMreAction> action_list);
    void	track_state_out_add_pim_mre_rp_entry_wc(list<PimMreAction> action_list);
    void	track_state_out_add_pim_mre_rp_entry_sg(list<PimMreAction> action_list);
    void	track_state_out_add_pim_mre_rp_entry_sg_rpt(list<PimMreAction> action_list);
    void	track_state_out_add_pim_mre_wc_entry_wc(list<PimMreAction> action_list);
    void	track_state_out_add_pim_mre_wc_entry_sg(list<PimMreAction> action_list);
    void	track_state_out_add_pim_mre_wc_entry_sg_rpt(list<PimMreAction> action_list);
    void	track_state_out_add_pim_mre_sg_entry_sg(list<PimMreAction> action_list);
    void	track_state_out_add_pim_mre_sg_entry_sg_rpt(list<PimMreAction> action_list);
    void	track_state_out_add_pim_mre_sg_rpt_entry_sg(list<PimMreAction> action_list);
    void	track_state_out_add_pim_mre_sg_rpt_entry_sg_rpt(list<PimMreAction> action_list);
    void	track_state_out_remove_pim_mre_rp_entry_rp(list<PimMreAction> action_list);
    void	track_state_out_remove_pim_mre_rp_entry_wc(list<PimMreAction> action_list);
    void	track_state_out_remove_pim_mre_rp_entry_sg(list<PimMreAction> action_list);
    void	track_state_out_remove_pim_mre_rp_entry_sg_rpt(list<PimMreAction> action_list);
    void	track_state_out_remove_pim_mre_wc_entry_wc(list<PimMreAction> action_list);
    void	track_state_out_remove_pim_mre_wc_entry_sg(list<PimMreAction> action_list);
    void	track_state_out_remove_pim_mre_wc_entry_sg_rpt(list<PimMreAction> action_list);
    void	track_state_out_remove_pim_mre_sg_entry_sg(list<PimMreAction> action_list);
    void	track_state_out_remove_pim_mre_sg_entry_sg_rpt(list<PimMreAction> action_list);
    void	track_state_out_remove_pim_mre_sg_rpt_entry_sg(list<PimMreAction> action_list);
    void	track_state_out_remove_pim_mre_sg_rpt_entry_sg_rpt(list<PimMreAction> action_list);
    
    // Private state
    PimMrt&	_pim_mrt;		// The PIM MRT
};

// Class to keep state for taking actions
class PimMreAction {
public:
    // XXX: the @param entry type is one of the following:
    // PIM_MRE_SG, PIM_MRE_SG_RPT, PIM_MRE_WC, PIM_MRE_RP, PIM_MFC
    PimMreAction(PimMreTrackState::output_state_t output_state,
		 uint32_t entry_type)
	: _output_state(output_state), _entry_type (entry_type) {}
    PimMreAction(const PimMreAction& action)
	: _output_state(action.output_state()),
	  _entry_type(action.entry_type()) {}
    PimMreTrackState::output_state_t output_state() const {
	return (_output_state);
    }
    bool is_sg() const { return (_entry_type & PIM_MRE_SG); }
    bool is_sg_rpt() const { return (_entry_type & PIM_MRE_SG_RPT); }
    bool is_wc() const { return (_entry_type & PIM_MRE_WC); }
    bool is_rp() const { return (_entry_type & PIM_MRE_RP); }
    bool is_mfc() const { return (_entry_type & PIM_MFC); }
    uint32_t entry_type() const { return (_entry_type); }
    
    bool operator==(const PimMreAction& action) const {
	return ((output_state() == action.output_state())
		&& (entry_type() == action.entry_type()));
    }
    
    void perform_action(PimMre& pim_mre, uint16_t vif_index,
			const IPvX& addr_arg);
    void perform_action(PimMfc& pim_mfc);
    
    
private:
    PimMreTrackState::output_state_t	_output_state;
    uint32_t	_entry_type;
};


//
// Global variables
//

//
// Global functions prototypes
//

#endif // __PIM_PIM_MRE_TRACK_STATE_HH__
