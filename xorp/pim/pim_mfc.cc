// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2011 XORP, Inc and Others-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

//
// PIM Multicast Forwarding Cache handling
//


#include "pim_module.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "pim_mfc.hh"
#include "pim_node.hh"
#include "pim_vif.hh"


PimMfc::PimMfc(PimMrt* pim_mrt, const IPvX& source, const IPvX& group)
    : Mre<PimMfc>(source, group),
    _pim_mrt(pim_mrt),
    _rp_addr(IPvX::ZERO(family())),
    _iif_vif_index(Vif::VIF_INDEX_INVALID),
    _flags(0)
{
    
}

PimMfc::~PimMfc()
{
    //
    // XXX: don't trigger any communications with the kernel
    // if PimNode's destructor is invoked.
    //
    if (pim_node()->node_status() != PROC_NULL)
	delete_mfc_from_kernel();
    
    //
    // Remove this entry from the RP table.
    //
    pim_node()->rp_table().delete_pim_mfc(this);
    
    //
    // Remove this entry from the PimMrt table.
    //
    pim_mrt()->remove_pim_mfc(this);
    
    //
    // Cancel the (S,G) Keepalive Timer
    //
    PimMre *pim_mre_sg = pim_mrt()->pim_mre_find(source_addr(), group_addr(),
						PIM_MRE_SG, 0);
    if ((pim_mre_sg != NULL) && pim_mre_sg->is_keepalive_timer_running()) {
	pim_mre_sg->cancel_keepalive_timer();
	pim_mre_sg->entry_try_remove();
    }
}

PimNode*
PimMfc::pim_node() const
{
    if (_pim_mrt)
	return _pim_mrt->pim_node();
    return NULL;
}

int
PimMfc::family() const
{
    return (_pim_mrt->family());
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
    pim_node()->rp_table().delete_pim_mfc(this);
    _rp_addr = v;
    pim_node()->rp_table().add_pim_mfc(this);
}

void
PimMfc::recompute_rp_mfc()
{
    IPvX new_rp_addr(IPvX::ZERO(family()));
    
    PimRp *new_pim_rp = pim_node()->rp_table().rp_find(group_addr());
    
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
    uint32_t new_iif_vif_index = Vif::VIF_INDEX_INVALID;
    Mifset new_olist;
    uint32_t lookup_flags;
    PimMre *pim_mre, *pim_mre_sg, *pim_mre_sg_rpt;
    
    lookup_flags = PIM_MRE_RP | PIM_MRE_WC | PIM_MRE_SG | PIM_MRE_SG_RPT;
    pim_mre = pim_mrt()->pim_mre_find(source_addr(), group_addr(),
				     lookup_flags, 0);
    if (pim_mre == NULL) {
	//
	// No matching multicast routing entry. Remove the PimMfc entry.
	// TODO: XXX: PAVPAVPAV: do we really want to remove the entry?
	// E.g., just reset the olist, and leave the entry itself to timeout?
	//
	
	set_has_forced_deletion(true);
	entry_try_remove();
	return;
    }
    
    // Get the (S,G) and the (S,G,rpt) PimMre entries (if exist)
    pim_mre_sg = NULL;
    pim_mre_sg_rpt = NULL;
    do {
	if (pim_mre->is_sg()) {
	    pim_mre_sg = pim_mre;
	    pim_mre_sg_rpt = pim_mre->sg_rpt_entry();
	    break;
	}
	if (pim_mre->is_sg_rpt()) {
	    pim_mre_sg = pim_mre->sg_entry();
	    pim_mre_sg_rpt = pim_mre;
	    break;
	}
	break;
    } while (false);
    
    //
    // Compute the new iif and the olist
    //
    // XXX: The computation is loosely based on the
    // "On receipt of data from S to G on interface iif:" procedure.
    //
    // Note that the is_directly_connected_s() check is not in the
    // spec, but for all practical purpose we want it here.
    //
    if ((pim_mre_sg != NULL)
	&& (pim_mre_sg->is_spt() || pim_mre_sg->is_directly_connected_s())) {
	new_iif_vif_index = pim_mre_sg->rpf_interface_s();
	new_olist = pim_mre->inherited_olist_sg();
    } else {
	new_iif_vif_index = pim_mre->rpf_interface_rp();
	new_olist = pim_mre->inherited_olist_sg_rpt();

	//
	// XXX: Test a special case when we are not forwarding multicast
	// traffic, but we need the forwarding entry to prevent unnecessary
	// "no cache" or "wrong iif" upcalls.
	//
	if (new_olist.none()) {
	    uint32_t iif_vif_index_s = Vif::VIF_INDEX_INVALID;
	    if (pim_mre_sg != NULL) {
		iif_vif_index_s = pim_mre_sg->rpf_interface_s();
	    } else if (pim_mre_sg_rpt != NULL) {
		iif_vif_index_s = pim_mre_sg_rpt->rpf_interface_s();
	    }
	    if ((iif_vif_index_s != Vif::VIF_INDEX_INVALID)
		&& (iif_vif_index_s == iif_vif_index())) {
		new_iif_vif_index = iif_vif_index_s;
	    }
	}
    }
    if (new_iif_vif_index != Vif::VIF_INDEX_INVALID)
	new_olist.reset(new_iif_vif_index);
    
    // XXX: this check should be used even if the iif and oifs didn't change
    // TODO: track the reason and explain it.
    if (new_iif_vif_index == Vif::VIF_INDEX_INVALID) {
	//
	// TODO: XXX: PAVPAVPAV: completely remove the olist check and comments
	// || new_olist.none()) {
	// No incoming interface or outgoing interfaces.
	// Remove the PimMfc entry.
	// TODO: XXX: PAVPAVPAV: do we really want to remove the entry?
	// E.g., just reset the olist, and leave the entry itself to timeout?
	//
	
	set_has_forced_deletion(true);
	entry_try_remove();
	return;
    }

    //
    // XXX: we just recompute the state, hence no need to add
    // a dataflow monitor.
    //
    update_mfc(new_iif_vif_index, new_olist, pim_mre_sg);
}

//
// A method for recomputing and setting the SPTbit that
// is triggered by the dependency-tracking machinery.
//
// Note that the spec says that the recomputation
// by calling Update_SPTbit(S,G,iif) should happen when a packet
// arrives. However, given that we do not forward the data packets
// in user space, we need to emulate that by triggering the
// recomputation whenever some of the input statements have changed.
// Also, note that this recomputation is not needed if there is not
// underlying PimMfc entry; the lack of such entry will anyway trigger
// a system uncall once the first multicast packet is received for
// forwarding.
//
// Note: the SPTbit setting applies only if there is an (S,G) PimMfc entry.
// Return true if state has changed, otherwise return false.
bool
PimMfc::recompute_update_sptbit_mfc()
{
    PimMre *pim_mre_sg = pim_mrt()->pim_mre_find(source_addr(), group_addr(),
						PIM_MRE_SG, 0);

    // XXX: no (S,G) entry, hence nothing to update
    if (pim_mre_sg == NULL)
	return false;

    if (pim_mre_sg->is_spt()) {
	// XXX: nothing to do, because the SPTbit is already set
	return false;
    }

    pim_mre_sg->update_sptbit_sg(iif_vif_index());

    if (pim_mre_sg->is_spt())
	return true;	// XXX: the SPTbit has been set

    return false;
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
    pim_mre = pim_mrt()->pim_mre_find(source_addr(), group_addr(),
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
	pim_mre = pim_mrt()->pim_mre_find(source_addr(), group_addr(),
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
	    if ((pim_mre_sg != NULL)
		&& pim_mre_sg->is_kat_set_to_rp_keepalive_period()) {
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
    if (pim_node()->is_switch_to_spt_enabled().get()
	&& (! ((pim_mre_sg != NULL)
	       && (pim_mre_sg->is_keepalive_timer_running())))) {
	uint32_t sec = pim_node()->switch_to_spt_threshold_interval_sec().get();
	uint32_t bytes = pim_node()->switch_to_spt_threshold_bytes().get();
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

void
PimMfc::update_mfc(uint32_t new_iif_vif_index, const Mifset& new_olist,
		   const PimMre* pim_mre_sg)
{
    bool is_changed = false;

    if (new_iif_vif_index == Vif::VIF_INDEX_INVALID) {
	//
	// XXX: if the new iif is invalid, then we always unconditionally
	// add the entry to the kernel. This is a work-around in case
	// we just created a new PimMfc entry with invalid iif and empty
	// olist and we want to install that entry.
	//
	is_changed = true;
    }

    if (new_iif_vif_index != iif_vif_index()) {
	set_iif_vif_index(new_iif_vif_index);
	is_changed = true;
    }

    if (new_olist != olist()) {
	set_olist(new_olist);
	is_changed = true;
    }

    //
    // Calculate the set of outgoing interfaces for which the WRONGVIF signal
    // is disabled.
    //
    Mifset new_olist_disable_wrongvif;
    new_olist_disable_wrongvif.set();	// XXX: by default disable on all vifs
    new_olist_disable_wrongvif ^= new_olist;	// Enable on all outgoing vifs
    //
    // If we are in process of switching to the SPT, then enable the WRONGVIF
    // signal on the expected new incoming interface.
    //
    if ((pim_mre_sg != NULL)
	&& (! pim_mre_sg->is_spt())
	&& (pim_mre_sg->rpf_interface_s() != pim_mre_sg->rpf_interface_rp())
	&& (pim_mre_sg->was_switch_to_spt_desired_sg()
	    || pim_mre_sg->is_join_desired_sg())) {
	if (pim_mre_sg->rpf_interface_s() != Vif::VIF_INDEX_INVALID)
	    new_olist_disable_wrongvif.reset(pim_mre_sg->rpf_interface_s());
    }
    if (new_olist_disable_wrongvif != olist_disable_wrongvif()) {
	set_olist_disable_wrongvif(new_olist_disable_wrongvif);
	is_changed = true;
    }

    if (is_changed)
	add_mfc_to_kernel();
}

int
PimMfc::add_mfc_to_kernel()
{
    if (pim_node()->is_log_trace()) {
	string res, res2;
	for (uint32_t i = 0; i < pim_node()->maxvifs(); i++) {
	    if (olist().test(i))
		res += "O";
	    else
		res += ".";
	    if (olist_disable_wrongvif().test(i))
		res2 += "O";
	    else
		res2 += ".";
	}
	XLOG_TRACE(pim_node()->is_log_trace(),
		   "Add MFC entry: (%s, %s) iif = %d olist = %s "
		   "olist_disable_wrongvif = %s",
		   cstring(source_addr()),
		   cstring(group_addr()),
		   iif_vif_index(),
		   res.c_str(),
		   res2.c_str());
    }
    
    if (pim_node()->add_mfc_to_kernel(*this) != XORP_OK)
	return (XORP_ERROR);
    
    return (XORP_OK);
}

int
PimMfc::delete_mfc_from_kernel()
{
    if (!pim_node()) {
	return XORP_OK;
    }

    if (pim_node()->is_log_trace()) {
	string res;
	for (uint32_t i = 0; i < pim_node()->maxvifs(); i++) {
	    if (olist().test(i))
		res += "O";
	    else
		res += ".";
	}
	XLOG_TRACE(pim_node()->is_log_trace(),
		   "Delete MFC entry: (%s, %s) iif = %d olist = %s",
		   cstring(source_addr()),
		   cstring(group_addr()),
		   iif_vif_index(),
		   res.c_str());
    }

    //
    // XXX: we don't call delete_all_dataflow_monitor(), because
    // the deletion of the MFC entry itself will remove all associated
    // dataflow monitors.
    //
    if (pim_node()->delete_mfc_from_kernel(*this) != XORP_OK)
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
    XLOG_TRACE(pim_node()->is_log_trace(),
	       "Add dataflow monitor: "
	       "source = %s group = %s "
	       "threshold_interval_sec = %d threshold_interval_usec = %d "
	       "threshold_packets = %d threshold_bytes = %d "
	       "is_threshold_in_packets = %d is_threshold_in_bytes = %d "
	       "is_geq_upcall = %d is_leq_upcall = %d",
	       cstring(source_addr()), cstring(group_addr()),
	       threshold_interval_sec, threshold_interval_usec,
	       threshold_packets, threshold_bytes,
	       is_threshold_in_packets, is_threshold_in_bytes,
	       is_geq_upcall, is_leq_upcall);

    if (pim_node()->add_dataflow_monitor(source_addr(),
					group_addr(),
					threshold_interval_sec,
					threshold_interval_usec,
					threshold_packets,
					threshold_bytes,
					is_threshold_in_packets,
					is_threshold_in_bytes,
					is_geq_upcall,
					is_leq_upcall)
	!= XORP_OK) {
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
    XLOG_TRACE(pim_node()->is_log_trace(),
	       "Delete dataflow monitor: "
	       "source = %s group = %s "
	       "threshold_interval_sec = %d threshold_interval_usec = %d "
	       "threshold_packets = %d threshold_bytes = %d "
	       "is_threshold_in_packets = %d is_threshold_in_bytes = %d "
	       "is_geq_upcall = %d is_leq_upcall = %d",
	       cstring(source_addr()), cstring(group_addr()),
	       threshold_interval_sec, threshold_interval_usec,
	       threshold_packets, threshold_bytes,
	       is_threshold_in_packets, is_threshold_in_bytes,
	       is_geq_upcall, is_leq_upcall);

    if (pim_node()->delete_dataflow_monitor(source_addr(),
					   group_addr(),
					   threshold_interval_sec,
					   threshold_interval_usec,
					   threshold_packets,
					   threshold_bytes,
					   is_threshold_in_packets,
					   is_threshold_in_bytes,
					   is_geq_upcall,
					   is_leq_upcall)
	!= XORP_OK) {
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
    XLOG_TRACE(pim_node()->is_log_trace(),
	       "Delete all dataflow monitors: "
	       "source = %s group = %s",
	       cstring(source_addr()), cstring(group_addr()));

    set_has_idle_dataflow_monitor(false);
    set_has_spt_switch_dataflow_monitor(false);
    
    if (pim_node()->delete_all_dataflow_monitor(source_addr(),
						group_addr())
	!= XORP_OK) {
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
	pim_mrt()->add_task_delete_pim_mfc(this);
    
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
    
    pim_mre = pim_mrt()->pim_mre_find(source_addr(), group_addr(),
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
	pim_mrt()->remove_pim_mfc(this);
	set_is_task_delete_done(true);
    } else {
	set_is_task_delete_pending(false);
	set_is_task_delete_done(false);
	return;
    }
}
