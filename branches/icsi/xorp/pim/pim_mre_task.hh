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

// $XORP: xorp/pim/pim_mre_task.hh,v 1.26 2002/12/09 18:29:26 hodson Exp $


#ifndef __PIM_PIM_MRE_TASK_HH__
#define __PIM_PIM_MRE_TASK_HH__


//
// PIM Multicast Routing Entry task definitions.
//


#include <list>

#include "libxorp/time_slice.hh"
#include "pim_mre.hh"
#include "pim_mre_track_state.hh"


//
// Constants definitions
//


//
// Structures/classes, typedefs and macros
//

class PimMrt;

// Task for PIM-specific Multicast Routing Entry
class PimMreTask {
public:
    PimMreTask(PimMrt& pim_mrt, PimMreTrackState::input_state_t input_state);
    ~PimMreTask();
    
    void cleanup_pim_mre_list(list<PimMre *>& pim_mre_list);
    void cleanup_pim_mfc_list(list<PimMfc *>& pim_mfc_list);
    
    // General info: PimNode, PimMrt, family, etc.
    PimNode&	pim_node()	const;
    PimMrt&	pim_mrt()	const	{ return (_pim_mrt);		}
    int		family()	const;
    
    void	schedule_task();
    bool	run_task();
    bool	run_task_rp();
    bool	run_task_wc();
    bool	run_task_sg_sg_rpt();
    bool	run_task_mfc();
    void	perform_pim_mre_actions(PimMre *pim_mre);
    void	perform_pim_mre_sg_sg_rpt_actions(PimMre *pim_mre_sg,
						  PimMre *pim_mre_sg_rpt);
    void	perform_pim_mfc_actions(PimMfc *pim_mfc);
    void	perform_pim_mfc_actions(const IPvX& source_addr,
					const IPvX& group_addr);
    void	add_pim_mre(PimMre *pim_mre);
    void	add_pim_mfc(PimMfc *pim_mfc);
    
    //
    // (*,*,RP) state setup
    //
    void	set_rp_addr_rp(const IPvX& rp_addr) {
	_rp_addr_rp = rp_addr;
	_is_set_rp_addr_rp = true;
    }
    void	set_rp_addr_prefix_rp(const IPvXNet& rp_addr_prefix) {
	_rp_addr_prefix_rp = rp_addr_prefix;
	_is_set_rp_addr_prefix_rp = true;
    }
    
    //
    // (*,G) state setup
    //
    void	set_group_addr_wc(const IPvX& group_addr) {
	_group_addr_wc = group_addr;
	_is_set_group_addr_wc = true;
    }
    void	set_rp_addr_wc(const IPvX& rp_addr) {
	_rp_addr_wc = rp_addr;
	_is_set_rp_addr_wc = true;
    }
    void	set_group_addr_prefix_wc(const IPvXNet& group_addr_prefix) {
	_group_addr_prefix_wc = group_addr_prefix;
	_is_set_group_addr_prefix_wc = true;
    }
    
    //
    // (S,G) and (S,G,rpt) state setup
    //
    void	set_source_addr_sg_sg_rpt(const IPvX& source_addr) {
	_source_addr_sg_sg_rpt = source_addr;
	_is_set_source_addr_sg_sg_rpt = true;
    }
    void	set_group_addr_sg_sg_rpt(const IPvX& group_addr) {
	_group_addr_sg_sg_rpt = group_addr;
	_is_set_group_addr_sg_sg_rpt = true;
    }
    void	set_source_addr_prefix_sg_sg_rpt(const IPvXNet& addr_prefix) {
	_source_addr_prefix_sg_sg_rpt = addr_prefix;
	_is_set_source_addr_prefix_sg_sg_rpt = true;
    }
    void	set_rp_addr_sg_sg_rpt(const IPvX& rp_addr) {
	_rp_addr_sg_sg_rpt = rp_addr;
	_is_set_rp_addr_sg_sg_rpt = true;
    }
    
    //
    // PimMfc state setup
    //
    void	set_source_addr_mfc(const IPvX& source_addr) {
	_source_addr_mfc = source_addr;
	_is_set_source_addr_mfc = true;
    }
    void	set_group_addr_mfc(const IPvX& group_addr) {
	_group_addr_mfc = group_addr;
	_is_set_group_addr_mfc = true;
    }
    void	set_source_addr_prefix_mfc(const IPvXNet& addr_prefix) {
	_source_addr_prefix_mfc = addr_prefix;
	_is_set_source_addr_prefix_mfc = true;
    }
    void	set_rp_addr_mfc(const IPvX& rp_addr) {
	_rp_addr_mfc = rp_addr;
	_is_set_rp_addr_mfc = true;
    }
    
    //
    // Other related setup
    //
    void	set_pim_nbr_addr_rp(const IPvX& v) {
	_pim_nbr_addr = v;
	_is_set_pim_nbr_addr_rp = true;
    }
    void	set_pim_nbr_addr_wc(const IPvX& v) {
	_pim_nbr_addr = v;
	_is_set_pim_nbr_addr_wc = true;
    }
    void	set_pim_nbr_addr_sg_sg_rpt(const IPvX& v) {
	_pim_nbr_addr = v;
	_is_set_pim_nbr_addr_sg_sg_rpt = true;
    }
    void	set_vif_index(uint16_t v) { _vif_index = v; }
    void	set_addr_arg(const IPvX& v) { _addr_arg = v; }
    
    //
    // Misc. information
    //
    uint16_t	vif_index() const { return (_vif_index); }
    const IPvX&	addr_arg() const { return (_addr_arg); }
    PimMreTrackState::input_state_t input_state() const { return (_input_state); }
    
private:
    // Private state
    PimMrt&	_pim_mrt;		// The PIM MRT
    
    list<PimMreAction>	_action_list_rp; // The list of (*,*,RP) actions
    list<PimMreAction>	_action_list_wc; // The list of (*,G) actions
    list<PimMreAction>	_action_list_sg_sg_rpt; // The list of (S,G) and
						// (S,G,rpt) actions
    list<PimMreAction>	_action_list_mfc; // The list of MFC actions
    
    TimeSlice		_time_slice;	// The time slice
    
    const PimMreTrackState::input_state_t _input_state;	// The input state
    
    // Timer to schedule the processing for the next time slice.
    Timer	_time_slice_timer;
    
    //
    // (*,*,RP) related state
    //
    bool	_is_set_rp_addr_rp;
    IPvX	_rp_addr_rp;
    bool	_is_set_rp_addr_prefix_rp;
    IPvXNet	_rp_addr_prefix_rp;
    list<PimMre *> _pim_mre_rp_list;		// The (*,*,RP) PimMre list
    list<PimMre *> _pim_mre_rp_processed_list;	// The (*,*,RP) processed list
    // State to continue the processing with the next time slice
    bool	_is_processing_rp;
    bool	_is_processing_rp_addr_rp;
    IPvX	_processing_rp_addr_rp;
    
    //
    // (*,G) related state
    //
    bool	_is_set_group_addr_wc;
    IPvX	_group_addr_wc;
    bool	_is_set_rp_addr_wc;
    IPvX	_rp_addr_wc;
    bool	_is_set_group_addr_prefix_wc;
    IPvXNet	_group_addr_prefix_wc;
    list<PimMre *> _pim_mre_wc_list;		// The (*,G) PimMre list
    list<PimMre *> _pim_mre_wc_processed_list;	// The (*,G) processed list
    // State to continue the processing with the next time slice
    bool	_is_processing_wc;
    bool	_is_processing_rp_addr_wc;
    IPvX	_processing_rp_addr_wc;
    bool	_is_processing_group_addr_wc;
    IPvX	_processing_group_addr_wc;
    
    //
    // (S,G) and (S,G,rpt) related state
    //
    bool	_is_set_source_addr_sg_sg_rpt;
    IPvX	_source_addr_sg_sg_rpt;
    bool	_is_set_group_addr_sg_sg_rpt;
    IPvX	_group_addr_sg_sg_rpt;
    bool	_is_set_source_addr_prefix_sg_sg_rpt;
    IPvXNet	_source_addr_prefix_sg_sg_rpt;
    bool	_is_set_rp_addr_sg_sg_rpt;
    IPvX	_rp_addr_sg_sg_rpt;
    list<PimMre *> _pim_mre_sg_list;		// The (S,G) PimMre list
    list<PimMre *> _pim_mre_sg_processed_list;	// The (S,G) processed list
    list<PimMre *> _pim_mre_sg_rpt_list;	// The (S,G,rpt) PimMre list
    list<PimMre *> _pim_mre_sg_rpt_processed_list;// The (S,G,rpt) processed list
    // State to continue the processing with the next time slice
    bool	_is_processing_sg_sg_rpt;
    bool	_is_processing_sg_source_addr_sg_sg_rpt;
    IPvX	_processing_sg_source_addr_sg_sg_rpt;
    bool	_is_processing_sg_rpt_source_addr_sg_sg_rpt;
    IPvX	_processing_sg_rpt_source_addr_sg_sg_rpt;
    bool	_is_processing_sg_group_addr_sg_sg_rpt;
    IPvX	_processing_sg_group_addr_sg_sg_rpt;
    bool	_is_processing_sg_rpt_group_addr_sg_sg_rpt;
    IPvX	_processing_sg_rpt_group_addr_sg_sg_rpt;
    bool	_is_processing_rp_addr_sg;
    bool	_is_processing_rp_addr_sg_rpt;
    IPvX	_processing_rp_addr_sg_sg_rpt;
    
    //
    // PimMfc related state
    //
    bool	_is_set_source_addr_mfc;
    IPvX	_source_addr_mfc;
    bool	_is_set_group_addr_mfc;
    IPvX	_group_addr_mfc;
    bool	_is_set_source_addr_prefix_mfc;
    IPvXNet	_source_addr_prefix_mfc;
    bool	_is_set_rp_addr_mfc;
    IPvX	_rp_addr_mfc;
    list<PimMfc *> _pim_mfc_list;		// The PimMfc list
    list<PimMfc *> _pim_mfc_processed_list;	// The PimMfc processed list
    // State to continue the processing with the next time slice
    bool	_is_processing_mfc;
    bool	_is_processing_source_addr_mfc;
    IPvX	_processing_source_addr_mfc;
    bool	_is_processing_group_addr_mfc;
    IPvX	_processing_group_addr_mfc;
    bool	_is_processing_rp_addr_mfc;
    IPvX	_processing_rp_addr_mfc;
    
    //
    // (*,*,RP), (*,G), (S,G), (S,G,rpt) PimNbr related state
    //
    bool	_is_set_pim_nbr_addr_rp;
    bool	_is_set_pim_nbr_addr_wc;
    bool	_is_set_pim_nbr_addr_sg_sg_rpt;
    IPvX	_pim_nbr_addr;
    bool	_is_processing_pim_nbr_addr_rp;
    bool	_is_processing_pim_nbr_addr_wc;
    bool	_is_processing_pim_nbr_addr_sg;
    bool	_is_processing_pim_nbr_addr_sg_rpt;
    
    // The 'occasionally-used' argument(s).
    uint16_t	_vif_index;
    IPvX	_addr_arg;
};

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __PIM_PIM_MRE_TASK_HH__
