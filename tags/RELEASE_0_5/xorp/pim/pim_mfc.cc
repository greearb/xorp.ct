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

#ident "$XORP: xorp/pim/pim_mfc.cc,v 1.13 2003/09/25 02:19:43 pavlin Exp $"

//
// PIM Multicast Forwarding Cache handling
//


#include "pim_module.h"
#include "pim_private.hh"
#include "pim_mfc.hh"
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


PimMfc::PimMfc(PimMrt& pim_mrt, const IPvX& source, const IPvX& group)
    : Mre<PimMfc>(source, group),
    _pim_mrt(pim_mrt),
    _rp_addr(IPvX::ZERO(family())),
    _iif_vif_index(Vif::VIF_INDEX_INVALID),
    _flags(0)
{
    
}

PimMfc::~PimMfc()
{
    // TODO: should we always call delete_mfc_from_kernel()?
    delete_mfc_from_kernel();
    
    //
    // Remove this entry from the RP table.
    //
    pim_node().rp_table().delete_pim_mfc(this);
    
    //
    // Remove this entry from the PimMrt table.
    //
    pim_mrt().remove_pim_mfc(this);
    
    //
    // Cancel the (S,G) Keepalive Timer
    //
    PimMre *pim_mre_sg = pim_mrt().pim_mre_find(source_addr(), group_addr(),
						PIM_MRE_SG, 0);
    if ((pim_mre_sg != NULL) && pim_mre_sg->is_keepalive_timer_running()) {
	pim_mre_sg->cancel_keepalive_timer();
	pim_mre_sg->entry_try_remove();
    }
}

PimNode&
PimMfc::pim_node() const
{
    return (_pim_mrt.pim_node());
}

int
PimMfc::family() const
{
    return (_pim_mrt.family());
}

void
PimMfc::set_rp_addr(const IPvX& v)
{
    if (v == _rp_addr)
	return;			// Nothing changed
    
    uncond_set_rp_addr(v);
}

void
PimMfc::uncond_set_rp_addr(const IPvX& v)
{
    pim_node().rp_table().delete_pim_mfc(this);
    _rp_addr = v;
    pim_node().rp_table().add_pim_mfc(this);
}

void
PimMfc::recompute_rp_mfc()
{
    IPvX new_rp_addr(IPvX::ZERO(family()));
    
    PimRp *new_pim_rp = pim_node().rp_table().rp_find(group_addr());
    
    if (new_pim_rp != NULL)
	new_rp_addr = new_pim_rp->rp_addr();
    
    if (new_rp_addr == rp_addr())
	return;			// Nothing changed
    
    set_rp_addr(new_rp_addr);
    add_mfc_to_kernel();
    // XXX: we just recompute the state, hence no need to add
    // a dataflow monitor.
}

void
PimMfc::recompute_iif_olist_mfc()
{
    uint16_t new_iif_vif_index = Vif::VIF_INDEX_INVALID;
    Mifset new_olist;
    uint16_t old_iif_vif_index = iif_vif_index();
    Mifset old_olist = olist();
    uint32_t lookup_flags;
    PimMre *pim_mre, *pim_mre_sg;
    
    lookup_flags = PIM_MRE_RP | PIM_MRE_WC | PIM_MRE_SG | PIM_MRE_SG_RPT;
    pim_mre = pim_mrt().pim_mre_find(source_addr(), group_addr(),
				     lookup_flags, 0);
    if (pim_mre == NULL) {
	// No matching multicast routing entry. Remove the PimMfc entry.
	// TODO: XXX: PAVPAVPAV: do we really want to remove the entry?
	// E.g., just reset the olist, and leave the entry itself to timeout?
	
	set_has_forced_deletion(true);
	entry_try_remove();
	return;
    }
    
    // Get the (S,G) PimMre entry (if exists)
    pim_mre_sg = NULL;
    do {
	if (pim_mre->is_sg()) {
	    pim_mre_sg = pim_mre;
	    break;
	}
	if (pim_mre->is_sg_rpt()) {
	    pim_mre_sg = pim_mre->sg_entry();
	    break;
	}
	break;
    } while (false);
    
    // Compute the new iif and the olist
    if ((pim_mre_sg != NULL)
	&& ((pim_mre_sg->is_spt() && pim_mre_sg->is_joined_state())
	    || (pim_mre_sg->is_directly_connected_s()))) {
	new_iif_vif_index = pim_mre_sg->rpf_interface_s();
    } else {
	new_iif_vif_index = pim_mre->rpf_interface_rp();
    }
    new_olist = pim_mre->inherited_olist_sg();
    
    // XXX: this check should be even if the iif and oifs didn't change
    // TODO: track the reason and explain it.
    if (new_iif_vif_index == Vif::VIF_INDEX_INVALID) {
	// TODO: XXX: PAVPAVPAV: completely remove the olist check and comments
	// || new_olist.none()) {
	// No incoming interface or outgoing interfaces.
	// Remove the PimMfc entry.
	// TODO: XXX: PAVPAVPAV: do we really want to remove the entry?
	// E.g., just reset the olist, and leave the entry itself to timeout?
	
	set_has_forced_deletion(true);
	entry_try_remove();
	return;
    }
    
    if ((new_iif_vif_index == old_iif_vif_index) && (new_olist == old_olist)) {
	return;			// Nothing changed
    }
    
    if ((old_iif_vif_index != Vif::VIF_INDEX_INVALID)
	&& (old_olist.none())) {
	// XXX: probably an entry that was installed to stop NOCACHE upcalls,
	// or that was left around until the (S,G) NotJoined routing state
	// expires. Just delete the PimMfc entry, and then later when we are
	// forced to install a new PimMfc entry because of a NOCACHE upcall,
	// we will set appropriately the SPT bit, etc.
	
	set_has_forced_deletion(true);
	entry_try_remove();
	return;
    }
    
    // Set the new iif and the olist
    set_iif_vif_index(new_iif_vif_index);
    set_olist(new_olist);
    
    add_mfc_to_kernel();
    // XXX: we just recompute the state, hence no need to add
    // a dataflow monitor.
}

//
// The SPT-switch threshold has changed.
// XXX: we will unconditionally install/deinstall SPT-switch dataflow monitor,
// because we don't keep state about the previous value of the threshold.
//
void
PimMfc::recompute_spt_switch_threshold_changed_mfc()
{
    install_spt_switch_dataflow_monitor_mfc(NULL);
}

//
// Conditionally install/deinstall SPT-switch dataflow monitor
// (i.e., only if the conditions whether we need dataflow monitor have changed)
//
void
PimMfc::recompute_monitoring_switch_to_spt_desired_mfc()
{
    PimMre *pim_mre;
    PimMre *pim_mre_sg;
    uint32_t lookup_flags;
    bool has_spt_switch = has_spt_switch_dataflow_monitor();
    bool is_spt_switch_desired;
    
    lookup_flags = PIM_MRE_RP | PIM_MRE_WC | PIM_MRE_SG | PIM_MRE_SG_RPT;
    pim_mre = pim_mrt().pim_mre_find(source_addr(), group_addr(),
				     lookup_flags, 0);
    
    if (pim_mre == NULL)
	return;		// Nothing to install
    
    //
    // Get the (S,G) entry (if exists)
    //
    pim_mre_sg = NULL;
    do {
	if (pim_mre->is_sg()) {
	    pim_mre_sg = pim_mre;
	    break;
	}
	if (pim_mre->is_sg_rpt()) {
	    pim_mre_sg = pim_mre->sg_entry();
	    break;
	}
	break;
    } while (false);
    
    is_spt_switch_desired = pim_mre->is_monitoring_switch_to_spt_desired_sg(pim_mre_sg);
    if ((pim_mre_sg != NULL) && (pim_mre_sg->is_keepalive_timer_running()))
	is_spt_switch_desired = false;
    
    if (has_spt_switch == is_spt_switch_desired)
	return;		// Nothing changed
    
    install_spt_switch_dataflow_monitor_mfc(pim_mre);
}

//
// Unconditionally install/deinstall SPT-switch dataflow monitor
//
void
PimMfc::install_spt_switch_dataflow_monitor_mfc(PimMre *pim_mre)
{
    PimMre *pim_mre_sg;
    bool has_idle = has_idle_dataflow_monitor();
    bool has_spt_switch = has_spt_switch_dataflow_monitor();
    
    // If necessary, perform the PimMre table lookup
    if (pim_mre == NULL) {
	uint32_t lookup_flags;
	
	lookup_flags = PIM_MRE_RP | PIM_MRE_WC | PIM_MRE_SG | PIM_MRE_SG_RPT;
	pim_mre = pim_mrt().pim_mre_find(source_addr(), group_addr(),
					 lookup_flags, 0);
	if (pim_mre == NULL)
	    return;		// Nothing to install
    }
    
    //
    // Get the (S,G) entry (if exists)
    //
    pim_mre_sg = NULL;
    do {
	if (pim_mre->is_sg()) {
	    pim_mre_sg = pim_mre;
	    break;
	}
	if (pim_mre->is_sg_rpt()) {
	    pim_mre_sg = pim_mre->sg_entry();
	    break;
	}
	break;
    } while (false);
    
    //
    // Delete the existing SPT-switch dataflow monitor (if such)
    //
    // XXX: because we don't keep state about the original value of
    // dataflow_monitor_sec, therefore we remove all dataflows for
    // this (S,G), and then if necessary we install back the
    // idle dataflow monitor.
    //
    if (has_spt_switch) {
	delete_all_dataflow_monitor();
	if (has_idle) {
	    // Restore the idle dataflow monitor
	    uint32_t expected_dataflow_monitor_sec = PIM_KEEPALIVE_PERIOD_DEFAULT;
	    if ((iif_vif_index() == pim_node().pim_register_vif_index())
		&& (pim_mre->i_am_rp())) {
		if (expected_dataflow_monitor_sec
		    < PIM_RP_KEEPALIVE_PERIOD_DEFAULT) {
		    expected_dataflow_monitor_sec
			= PIM_RP_KEEPALIVE_PERIOD_DEFAULT;
		}
	    }
	    add_dataflow_monitor(expected_dataflow_monitor_sec, 0,
				 0,		// threshold_packets
				 0,		// threshold_bytes
				 true,		// is_threshold_in_packets
				 false,		// is_threshold_in_bytes
				 false,		// is_geq_upcall ">="
				 true);		// is_leq_upcall "<="
				 
	}
    }
    
    //
    // If necessary, install a new SPT-switch dataflow monitor
    //
    if (pim_node().is_switch_to_spt_enabled().get()
	&& (! ((pim_mre_sg != NULL)
	       && (pim_mre_sg->is_keepalive_timer_running())))) {
	uint32_t sec = pim_node().switch_to_spt_threshold_interval_sec().get();
	uint32_t bytes = pim_node().switch_to_spt_threshold_bytes().get();
	
	if (pim_mre->is_monitoring_switch_to_spt_desired_sg(pim_mre_sg)) {
	    add_dataflow_monitor(sec, 0,
				 0,		// threshold_packets
				 bytes,		// threshold_bytes
				 false,		// is_threshold_in_packets
				 true,		// is_threshold_in_bytes
				 true,		// is_geq_upcall ">="
				 false);	// is_leq_upcall "<="
	}
    }
}

int
PimMfc::add_mfc_to_kernel()
{
    if (pim_node().is_log_trace()) {
	string res;
	for (uint16_t i = 0; i < pim_node().maxvifs(); i++) {
	    if (olist().test(i))
		res += "O";
	    else
		res += ".";
	}
	XLOG_TRACE(pim_node().is_log_trace(),
		   "Add MFC entry: (%s,%s) iif = %d olist = %s",
		   cstring(source_addr()),
		   cstring(group_addr()),
		   iif_vif_index(),
		   res.c_str());
    }
    
    if (pim_node().add_mfc_to_kernel(*this) < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimMfc::delete_mfc_from_kernel()
{
    if (pim_node().is_log_trace()) {
	string res;
	for (uint16_t i = 0; i < pim_node().maxvifs(); i++) {
	    if (olist().test(i))
		res += "O";
	    else
		res += ".";
	}
	XLOG_TRACE(pim_node().is_log_trace(),
		   "Delete MFC entry: (%s,%s) iif = %d olist = %s",
		   cstring(source_addr()),
		   cstring(group_addr()),
		   iif_vif_index(),
		   res.c_str());
    }
    
    delete_all_dataflow_monitor();
    if (pim_node().delete_mfc_from_kernel(*this) < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimMfc::add_dataflow_monitor(uint32_t threshold_interval_sec,
			     uint32_t threshold_interval_usec,
			     uint32_t threshold_packets,
			     uint32_t threshold_bytes,
			     bool is_threshold_in_packets,
			     bool is_threshold_in_bytes,
			     bool is_geq_upcall,
			     bool is_leq_upcall)
{
    if (pim_node().add_dataflow_monitor(source_addr(),
					group_addr(),
					threshold_interval_sec,
					threshold_interval_usec,
					threshold_packets,
					threshold_bytes,
					is_threshold_in_packets,
					is_threshold_in_bytes,
					is_geq_upcall,
					is_leq_upcall) < 0) {
	return (XORP_ERROR);
    }
    
    if (is_leq_upcall
	&& ((is_threshold_in_packets && (threshold_packets == 0))
	    || (is_threshold_in_bytes && (threshold_bytes == 0)))) {
	set_has_idle_dataflow_monitor(true);
    }
    
    if (is_geq_upcall) {
	set_has_spt_switch_dataflow_monitor(true);
    }
    
    return (XORP_OK);
}

int
PimMfc::delete_dataflow_monitor(uint32_t threshold_interval_sec,
				uint32_t threshold_interval_usec,
				uint32_t threshold_packets,
				uint32_t threshold_bytes,
				bool is_threshold_in_packets,
				bool is_threshold_in_bytes,
				bool is_geq_upcall,
				bool is_leq_upcall)
{
    if (pim_node().delete_dataflow_monitor(source_addr(),
					   group_addr(),
					   threshold_interval_sec,
					   threshold_interval_usec,
					   threshold_packets,
					   threshold_bytes,
					   is_threshold_in_packets,
					   is_threshold_in_bytes,
					   is_geq_upcall,
					   is_leq_upcall) < 0) {
	return (XORP_ERROR);
    }

    if (is_leq_upcall
	&& ((is_threshold_in_packets && (threshold_packets == 0))
	    || (is_threshold_in_bytes && (threshold_bytes == 0)))) {
	set_has_idle_dataflow_monitor(false);
    }
    
    if (is_geq_upcall) {
	set_has_spt_switch_dataflow_monitor(false);
    }
    
    return (XORP_OK);
}

int
PimMfc::delete_all_dataflow_monitor()
{
    set_has_idle_dataflow_monitor(false);
    set_has_spt_switch_dataflow_monitor(false);
    
    if (pim_node().delete_all_dataflow_monitor(source_addr(),
					       group_addr()) < 0) {
	return (XORP_ERROR);
    }
    
    return (XORP_OK);
}

// Try to remove PimMfc entry if not needed anymore.
// Return true if entry is scheduled to be removed, othewise false.
bool
PimMfc::entry_try_remove()
{
    bool ret_value;
    
    if (is_task_delete_pending())
	return (true);		// The entry is already pending deletion
    
    ret_value = entry_can_remove();
    if (ret_value)
	pim_mrt().add_task_delete_pim_mfc(this);
    
    return (ret_value);
}

bool
PimMfc::entry_can_remove() const
{
    uint32_t lookup_flags;
    PimMre *pim_mre;
    
    if (has_forced_deletion())
	return (true);
    
    if (iif_vif_index() == Vif::VIF_INDEX_INVALID)
	return (true);
    
    lookup_flags = PIM_MRE_RP | PIM_MRE_WC | PIM_MRE_SG | PIM_MRE_SG_RPT;
    
    pim_mre = pim_mrt().pim_mre_find(source_addr(), group_addr(),
				     lookup_flags, 0);
    if (pim_mre == NULL)
	return (true);
    
    return (false);
}

void
PimMfc::remove_pim_mfc_entry_mfc()
{
    if (is_task_delete_pending() && entry_can_remove()) {
	//
	// Remove the entry from the PimMrt, and mark it as deletion done
	//
	pim_mrt().remove_pim_mfc(this);
	set_is_task_delete_done(true);
    } else {
	set_is_task_delete_pending(false);
	set_is_task_delete_done(false);
	return;
    }
}
