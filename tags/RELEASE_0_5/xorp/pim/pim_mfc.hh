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

// $XORP: xorp/pim/pim_mfc.hh,v 1.4 2003/07/12 01:14:37 pavlin Exp $


#ifndef __PIM_PIM_MFC_HH__
#define __PIM_PIM_MFC_HH__


//
// PIM Multicast Forwarding Cache definitions.
//


#include "mrt/mifset.hh"
#include "mrt/mrt.hh"


//
// Constants definitions
//


//
// Structures/classes, typedefs and macros
//

class IPvX;
class PimMre;
class PimMrt;
class PimNode;


// PIM-specific Multicast Forwarding Cache
class PimMfc : public Mre<PimMfc> {
public:
    PimMfc(PimMrt& pim_mrt, const IPvX& source, const IPvX& group);
    ~PimMfc();

    // General info: PimNode, PimMrt, family, etc.
    PimNode&	pim_node()	const;
    PimMrt&	pim_mrt()	const	{ return (_pim_mrt);		}
    int		family()	const;

    const IPvX& rp_addr() const { return (_rp_addr); }
    void	set_rp_addr(const IPvX& v);
    void	uncond_set_rp_addr(const IPvX& v);
    uint16_t	iif_vif_index() const	{ return (_iif_vif_index);	}
    void	set_iif_vif_index(uint16_t v) { _iif_vif_index = v;	}
    const Mifset& olist() const	{ return (_olist);	}
    const Mifset& olist_disable_wrongvif() const {
	return (_olist_disable_wrongvif);
    }
    bool	is_set_oif(uint16_t vif_index) const {
	return (_olist.test(vif_index));
    }
    void	set_olist(const Mifset& v) { _olist = v;	}
    void	set_olist_disable_wrongvif(const Mifset& v) {
	_olist_disable_wrongvif = v;
    }
    void	set_oif(uint16_t vif_index, bool v) {
	if (v)
	    _olist.set(vif_index);
	else
	    _olist.reset(vif_index);
    }
    
    void	recompute_rp_mfc();
    void	recompute_iif_olist_mfc();
    void	recompute_spt_switch_threshold_changed_mfc();
    void	recompute_monitoring_switch_to_spt_desired_mfc();
    void	install_spt_switch_dataflow_monitor_mfc(PimMre *pim_mre);
    
    int		add_mfc_to_kernel();
    int		delete_mfc_from_kernel();
    
    int		add_dataflow_monitor(uint32_t threshold_interval_sec,
				     uint32_t threshold_interval_usec,
				     uint32_t threshold_packets,
				     uint32_t threshold_bytes,
				     bool is_threshold_in_packets,
				     bool is_threshold_in_bytes,
				     bool is_geq_upcall,
				     bool is_leq_upcall);
    int		delete_dataflow_monitor(uint32_t threshold_interval_sec,
					uint32_t threshold_interval_usec,
					uint32_t threshold_packets,
					uint32_t threshold_bytes,
					bool is_threshold_in_packets,
					bool is_threshold_in_bytes,
					bool is_geq_upcall,
					bool is_leq_upcall);
    int		delete_all_dataflow_monitor();
    
    bool	entry_try_remove();
    bool	entry_can_remove() const;
    void	remove_pim_mfc_entry_mfc();
    
    bool	is_task_delete_pending() const { return (_flags & PIM_MFC_TASK_DELETE_PENDING); }
    void	set_is_task_delete_pending(bool v) {
	if (v)
	    _flags |= PIM_MFC_TASK_DELETE_PENDING;
	else
	    _flags &= ~PIM_MFC_TASK_DELETE_PENDING;
    }
    bool	is_task_delete_done() const { return (_flags & PIM_MFC_TASK_DELETE_DONE); }
    void	set_is_task_delete_done(bool v) {
	if (v)
	    _flags |= PIM_MFC_TASK_DELETE_DONE;
	else
	    _flags &= ~PIM_MFC_TASK_DELETE_DONE;
    }
    
    bool	has_idle_dataflow_monitor() const { return (_flags & PIM_MFC_HAS_IDLE_DATAFLOW_MONITOR); }
    void	set_has_idle_dataflow_monitor(bool v) {
	if (v)
	    _flags |= PIM_MFC_HAS_IDLE_DATAFLOW_MONITOR;
	else
	    _flags &= ~PIM_MFC_HAS_IDLE_DATAFLOW_MONITOR;
    }

    bool	has_spt_switch_dataflow_monitor() const { return (_flags & PIM_MFC_HAS_SPT_SWITCH_DATAFLOW_MONITOR); }
    void	set_has_spt_switch_dataflow_monitor(bool v) {
	if (v)
	    _flags |= PIM_MFC_HAS_SPT_SWITCH_DATAFLOW_MONITOR;
	else
	    _flags &= ~PIM_MFC_HAS_SPT_SWITCH_DATAFLOW_MONITOR;
    }
    
    bool	has_forced_deletion() const { return (_flags & PIM_MFC_HAS_FORCED_DELETION); }
    void	set_has_forced_deletion(bool v) {
	if (v)
	    _flags |= PIM_MFC_HAS_FORCED_DELETION;
	else
	    _flags &= ~PIM_MFC_HAS_FORCED_DELETION;
    }
    
private:
    PimMrt&	_pim_mrt;		// The PIM MRT (yuck!)
    IPvX	_rp_addr;		// The RP address
    uint16_t	_iif_vif_index;		// The incoming interface
    Mifset	_olist;			// The outgoing interfaces
    Mifset	_olist_disable_wrongvif;// The outgoing interfaces for which
					// the WRONGVIF kernel signal is
					// disabled.
    
    // PimMfc _flags
    enum {
	PIM_MFC_TASK_DELETE_PENDING = 1 << 0,	// Entry is pending deletion
	PIM_MFC_TASK_DELETE_DONE    = 1 << 1,	// Entry is ready to be deleted
	PIM_MFC_HAS_IDLE_DATAFLOW_MONITOR = 1 << 2, // Entry has an idle dataflow monitor
	PIM_MFC_HAS_SPT_SWITCH_DATAFLOW_MONITOR = 1 << 3, // Entry has a SPT-switch dataflow monitor
	PIM_MFC_HAS_FORCED_DELETION = 1 << 4	// Entry is forced to be deleted
    };
    
    uint32_t	_flags;			// Various flags (see PIM_MFC_* above)
};

//
// Global variables
//

//
// Global functions prototypes
//

#endif // __PIM_PIM_MFC_HH__
