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

#ident "$XORP: xorp/pim/pim_mrt_mfc.cc,v 1.4 2003/02/07 00:39:31 pavlin Exp $"

//
// PIM Multicast Routing Table MFC-related implementation.
//


#include "pim_module.h"
#include "pim_private.hh"
#include "pim_mfc.hh"
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

int
PimMrt::signal_message_nocache_recv(const string& src_module_instance_name,
				    x_module_id , // src_module_id,
				    uint16_t vif_index,
				    const IPvX& src,
				    const IPvX& dst)
{
    XLOG_TRACE(pim_node().is_log_trace(),
	       "RX NOCACHE signal from %s: vif_index = %d src = %s dst = %s",
	       src_module_instance_name.c_str(),
	       vif_index,
	       cstring(src), cstring(dst));
    
    receive_data(vif_index, src, dst);
    
    return (XORP_OK);
}

int
PimMrt::signal_message_wrongvif_recv(const string& src_module_instance_name,
				     x_module_id , // src_module_id,
				     uint16_t vif_index,
				     const IPvX& src,
				     const IPvX& dst)
{
    XLOG_TRACE(pim_node().is_log_trace(),
	       "RX WRONGVIF signal from %s: vif_index = %d src = %s dst = %s",
	       src_module_instance_name.c_str(),
	       vif_index,
	       cstring(src), cstring(dst));
    
    receive_data(vif_index, src, dst);
    
    return (XORP_OK);
}

int
PimMrt::signal_message_wholepkt_recv(const string& src_module_instance_name,
				     x_module_id , // src_module_id,
				     uint16_t vif_index,
				     const IPvX& src,
				     const IPvX& dst,
				     const uint8_t *rcvbuf,
				     size_t rcvlen)
{
    PimMre *pim_mre_sg;
    const IPvX *rp_addr_ptr;
    PimVif *pim_vif = NULL;
    
    XLOG_TRACE(pim_node().is_log_trace(),
	       "RX WHOLEPKT signal from %s: vif_index = %d src = %s dst = %s",
	       src_module_instance_name.c_str(),
	       vif_index,
	       cstring(src), cstring(dst));
    
    //
    // Find the corresponding (S,G) multicast routing entry
    //
    pim_mre_sg = pim_mre_find(src, dst, PIM_MRE_SG, 0);
    if (pim_mre_sg == NULL) {
	// This shoudn't happen, because to receive WHOLEPKT signal,
	// we must have installed first the appropriate (S,G) MFC entry
	// that matches a (S,G) PimMre entry.
	XLOG_ERROR("RX WHOLEPKT signal from %s: vif_index = %d src = %s dst = %s: "
		   "no matching (S,G) multicast routing entry",
		   src_module_instance_name.c_str(),
		   vif_index,
		   cstring(src), cstring(dst));
	return (XORP_ERROR);
    }
    
    //
    // Find the RP address
    //
    rp_addr_ptr = pim_mre_sg->rp_addr_ptr();
    if (rp_addr_ptr == NULL) {
	XLOG_WARNING("RX WHOLEPKT signal from %s: src = %s dst = %s "
		     "vif_index = %d: no RP address for this group",
		     src_module_instance_name.c_str(),
		     cstring(src), cstring(dst),
		     vif_index);
	return (XORP_ERROR);
    }
    
    // XXX: If we want to be pedantic, here we might want to lookup
    // the PimMfc entries and check whether the PIM Register vif is
    // in the list of outgoing interfaces.
    // However, this is probably an overkill, because the olist of
    // the kernel MFC should be exactly same, hence we don't do it.
    
    //
    // Send a PIM Register to the RP
    //
    pim_vif = pim_node().vif_find_direct(src);
    if (pim_vif == NULL) {
	XLOG_WARNING("RX WHOLEPKT signal from %s: src = %s dst = %s: "
		     "no interface directly connected to source",
		     src_module_instance_name.c_str(),
		     cstring(src), cstring(dst));
	return (XORP_ERROR);
    }
    
    pim_vif->pim_register_send(*rp_addr_ptr, src, dst, rcvbuf, rcvlen);
    
    return (XORP_OK);
}

void
PimMrt::receive_data(uint16_t iif_vif_index, const IPvX& src, const IPvX& dst)
{
    PimVif *pim_vif;
    PimMre *pim_mre;
    PimMre *pim_mre_sg = NULL;
    PimMfc *pim_mfc = NULL;
    Mifset olist;
    uint32_t lookup_flags;
    bool is_sptbit_set = false;
    bool is_directly_connected_s = false;
    bool is_keepalive_timer_restarted = false;
    bool is_wrong_iif = true;
    
    if (iif_vif_index == Vif::VIF_INDEX_INVALID) {
	return;		// Invalid vif
    }
    
    pim_vif = vif_find_by_vif_index(iif_vif_index);
    if ((pim_vif == NULL) || (! pim_vif->is_up())) {
	return;		// No such vif, or the vif is not in operation
    }
    
    //
    // Lookup the PimMrt for the longest-match
    //
    lookup_flags = PIM_MRE_RP | PIM_MRE_WC | PIM_MRE_SG | PIM_MRE_SG_RPT;
    pim_mre = pim_mre_find(src, dst, lookup_flags, 0);
    
    // Test if source is directly connected
    do {
	if (pim_mre != NULL) {
	    if (pim_mre->is_sg() || pim_mre->is_sg_rpt()) {
		if (pim_mre->is_directly_connected_s()) {
		    is_directly_connected_s = true;
		    break;
		}
	    }
	}
	if (pim_node().is_directly_connected(src)) {
	    is_directly_connected_s = true;
	    break;
	}
	break;
    } while (false);
    
    if ((pim_mre != NULL) && pim_mre->is_sg())
	pim_mre_sg = pim_mre;
    
    //
    // Take action if directly-connected source
    //
    if (is_directly_connected_s) {
	// Create a (S,G) entry if necessary
	if ((pim_mre == NULL) || (! pim_mre->is_sg())) {
	    if (pim_mre_sg == NULL) {
		pim_mre = pim_mre_find(src, dst, PIM_MRE_SG, PIM_MRE_SG);
		pim_mre_sg = pim_mre;
	    }
	    //
	    // Take action if directly-connected source
	    //
	    // set KeepaliveTimer(S,G) to Keepalive_Period
	    // # Note: register state transition may happen as a result
	    // # of restarting KeepaliveTimer, and must be dealt with here.
	    pim_mre_sg->start_keepalive_timer();
	    is_keepalive_timer_restarted = true;
	    
	    // TODO: XXX: PAVPAVPAV: OK TO COMMENT?
	    pim_mre_sg->recompute_is_could_register_sg();
	    pim_mre_sg->recompute_is_join_desired_sg();
	}
    }
    
    if (pim_mre == NULL) {
	// XXX: install a MFC in the kernel with NULL oifs
	pim_mfc = pim_mfc_find(src, dst, true);
	XLOG_ASSERT(pim_mfc != NULL);
	
	pim_mfc->set_iif_vif_index(iif_vif_index);
	pim_mfc->add_mfc_to_kernel();
	if (! pim_mfc->has_dataflow_monitor()) {
	    // Add a dataflow monitor to expire idle (S,G) MFC state
	    // XXX: strictly speaking, the period doesn't have
	    // to be PIM_KEEPALIVE_PERIOD_DEFAULT, because it has nothing
	    // to do with PIM, but for simplicity we use it here.
	    pim_mfc->add_dataflow_monitor(PIM_KEEPALIVE_PERIOD_DEFAULT, 0,
					  0,	// threshold_packets
					  0,	// threshold_bytes
					  true,	// is_threshold_in_packets
					  false,// is_threshold_in_bytes
					  false,// is_geq_upcall ">="
					  true);// is_leq_upcall "<="
	}
	return;
    }
    
    //
    // Get the (S,G) entry (if exists).
    //
    if ((pim_mre_sg == NULL) && pim_mre->is_sg_rpt())
	pim_mre_sg = pim_mre->sg_entry();
    
    //
    // Update the SPT-bit
    //
    if (pim_mre_sg != NULL) {
	// Update_SPTbit(S,G,iif)
	pim_mre_sg->update_sptbit_sg(iif_vif_index);
	is_sptbit_set = pim_mre_sg->is_spt();
    }
    
    //
    // Perform the rest of the processing
    //
    if ((pim_mre_sg != NULL)
	&& (iif_vif_index == pim_mre_sg->rpf_interface_s())
	&& (pim_mre_sg->is_joined_state()
	    || is_directly_connected_s)) {
	is_wrong_iif = false;
	olist = pim_mre_sg->inherited_olist_sg_forward();
	if (olist.any() && (! is_keepalive_timer_restarted)) {
	    // restart KeepaliveTimer(S,G)
	    pim_mre_sg->start_keepalive_timer();
	    is_keepalive_timer_restarted = true;
	}
    } else if ((iif_vif_index == pim_mre->rpf_interface_rp())
	       && (is_sptbit_set == false)) {
	is_wrong_iif = false;
	olist = pim_mre->inherited_olist_sg_rpt_forward();
	if (pim_mre->check_switch_to_spt_sg()) {
	    if (pim_mre_sg == NULL) {
		// XXX: create the (S,G) entry to initiate (S,G) Join
		pim_mre_sg = pim_mre_find(src,
					  dst,
					  PIM_MRE_SG,
					  PIM_MRE_SG);
		pim_mre_sg->start_keepalive_timer();
		is_keepalive_timer_restarted = true;
		
		// TODO: XXX: PAVPAVPAV: OK TO COMMENT?
		// pim_mre_sg->recompute_is_could_register_sg();
		// pim_mre_sg->recompute_is_join_desired_sg();
	    }
	    XLOG_ASSERT(pim_mre_sg != NULL);
	    if (! is_keepalive_timer_restarted) {
		pim_mre_sg->start_keepalive_timer();
		is_keepalive_timer_restarted = true;
	    }
	}
    } else {
	// # Note: RPF check failed
	if ((is_sptbit_set == true)
	    && pim_mre->inherited_olist_sg_forward().test(iif_vif_index)) {
	    // send Assert(S,G) on iif_vif_index
	    XLOG_ASSERT(pim_mre_sg != NULL);
	    pim_mre_sg->wrong_iif_data_arrived_sg(pim_vif, src);
	} else if ((is_sptbit_set == false)
		   && (pim_mre->inherited_olist_sg_rpt_forward().test(
		       iif_vif_index))) {
	    // send Assert(*,G) on iif_vif_index
	    PimMre *pim_mre_wc = NULL;
	    bool is_new_entry = false;
	    
	    do {
		if (pim_mre->is_wc()) {
		    pim_mre_wc = pim_mre;
		    break;
		}
		pim_mre_wc = pim_mre->wc_entry();
		break;
	    } while (false);
	    if (pim_mre_wc == NULL) {
		pim_mre_wc = pim_mre_find(src, dst,  PIM_MRE_WC,  PIM_MRE_WC);
		is_new_entry = true;
	    }
	    XLOG_ASSERT(pim_mre_wc != NULL);
	    pim_mre_wc->wrong_iif_data_arrived_wc(pim_vif, src);
	    if (is_new_entry)
		pim_mre_wc->entry_try_remove();
	}
	return;		// TODO: XXX: PAVPAVPAV: not in the spec (yet)
    }
    
    olist.reset(iif_vif_index);
    
    // Lookup/create a PimMfc entry
    pim_mfc = pim_mfc_find(src, dst, true);
    XLOG_ASSERT(pim_mfc != NULL);
    
    if ((! is_wrong_iif)
	|| (pim_mfc->iif_vif_index() == Vif::VIF_INDEX_INVALID)) {
	pim_mfc->set_iif_vif_index(iif_vif_index);
    }
    pim_mfc->set_olist(olist);
    pim_mfc->add_mfc_to_kernel();
    if (is_keepalive_timer_restarted || (! pim_mfc->has_dataflow_monitor())) {
	//
	// Add a dataflow monitor to expire idle (S,G) PimMre state
	// and/or idle PimMfc+MFC state
	//
	// First, compute the dataflow monitor value, which may be different
	// for (S,G) entry in the RP that is used for Register decapsulation.
	uint32_t expected_dataflow_monitor_sec = PIM_KEEPALIVE_PERIOD_DEFAULT;
	if (is_keepalive_timer_restarted
	    && (iif_vif_index == pim_register_vif_index())) {
	    bool i_am_rp = false;
	    do {
		if (pim_mre_sg != NULL) {
		    i_am_rp = pim_mre_sg->i_am_rp();
		    break;
		}
		if (pim_mre != NULL) {
		    i_am_rp = pim_mre->i_am_rp();
		    break;
		}
	    } while (false);
	    if (i_am_rp) {
		if (expected_dataflow_monitor_sec
		    < PIM_RP_KEEPALIVE_PERIOD_DEFAULT) {
		    expected_dataflow_monitor_sec
			= PIM_RP_KEEPALIVE_PERIOD_DEFAULT;
		}
	    }
	}
	
	pim_mfc->add_dataflow_monitor(expected_dataflow_monitor_sec, 0,
				      0,	// threshold_packets
				      0,	// threshold_bytes
				      true,	// is_threshold_in_packets
				      false,	// is_threshold_in_bytes
				      false,	// is_geq_upcall ">="
				      true);	// is_leq_upcall "<="
    }
    
    // TODO: XXX: PAVPAVPAV: if a (*,G) entry, add a dataflow monitor
    // to monitor whether it is time to switch to the SPT.
}

int
PimMrt::signal_dataflow_recv(const IPvX& source_addr,
			     const IPvX& group_addr,
			     uint32_t threshold_interval_sec,
			     uint32_t threshold_interval_usec,
			     uint32_t measured_interval_sec,
			     uint32_t measured_interval_usec,
			     uint32_t threshold_packets,
			     uint32_t threshold_bytes,
			     uint32_t measured_packets,
			     uint32_t measured_bytes,
			     bool is_threshold_in_packets,
			     bool is_threshold_in_bytes,
			     bool is_geq_upcall,
			     bool is_leq_upcall)
{
    PimMre *pim_mre;
    PimMre *pim_mre_sg;
    PimMfc *pim_mfc;
    uint32_t lookup_flags
	= PIM_MRE_RP | PIM_MRE_WC | PIM_MRE_SG | PIM_MRE_SG_RPT;
    uint32_t expected_dataflow_monitor_sec = PIM_KEEPALIVE_PERIOD_DEFAULT;
    bool i_am_rp = false;
    
    XLOG_TRACE(pim_node().is_log_trace(),
	       "RX DATAFLOW signal: "
	       "source = %s group = %s "
	       "threshold_interval_sec = %d threshold_interval_usec = %d "
	       "measured_interval_sec = %d measured_interval_usec = %d "
	       "threshold_packets = %d threshold_bytes = %d "
	       "measured_packets = %d measured_bytes = %d "
	       "is_threshold_in_packets = %d is_threshold_in_bytes = %d "
	       "is_geq_upcall = %d is_leq_upcall = %d",
	       cstring(source_addr), cstring(group_addr),
	       threshold_interval_sec, threshold_interval_usec,
	       measured_interval_sec, measured_interval_usec,
	       threshold_packets, threshold_bytes,
	       measured_packets, measured_bytes,
	       is_threshold_in_packets, is_threshold_in_bytes,
	       is_geq_upcall, is_leq_upcall);
    
    if (is_geq_upcall) {
	// TODO: XXX: PAVPAVPAV: take care of this!!
	return (XORP_OK);
    }
    
    pim_mfc = pim_mfc_find(source_addr, group_addr, false);
    
    if (pim_mfc == NULL) {
	pim_node().delete_all_dataflow_monitor(source_addr, group_addr);
	return (XORP_ERROR);	// No such PimMfc entry
    }
    
    pim_mre = pim_mre_find(source_addr, group_addr, lookup_flags, 0);
    
    //
    // Get the (S,G) entry
    //
    pim_mre_sg = NULL;
    do {
	if (pim_mre == NULL)
	    break;
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
    // Compute what is the expected value for the Keepalive Timer
    //
    if (pim_mfc->iif_vif_index() == pim_register_vif_index()) {
	do {
	    i_am_rp = false;
	    if (pim_mre_sg != NULL) {
		i_am_rp = pim_mre_sg->i_am_rp();
		break;
	    }
	    if (pim_mre != NULL) {
		i_am_rp = pim_mre->i_am_rp();
		break;
	    }
	} while (false);
	if (i_am_rp) {
	    if (expected_dataflow_monitor_sec
		< PIM_RP_KEEPALIVE_PERIOD_DEFAULT) {
		expected_dataflow_monitor_sec
		    = PIM_RP_KEEPALIVE_PERIOD_DEFAULT;
	    }
	}
    }
    
    //
    // Test if time to remove a MFC entry and/or if the Keepalive timer
    // has expired.
    //
    if ((measured_packets == 0)
	&& (threshold_interval_sec >= expected_dataflow_monitor_sec)) {
	// Idle source: delete the MFC entry
	delete pim_mfc;
	pim_mfc = NULL;
	if (pim_mre_sg != NULL) {
	    // Idle source: the Keepalive(S,G) timer has expired
	    pim_mre_sg->keepalive_timer_timeout();
	    return (XORP_OK);
	}
    }
    
    //
    // Restart the Keepalive timer if it has expired prematurely
    //
    if ((measured_packets == 0)
	&& (threshold_interval_sec < expected_dataflow_monitor_sec)) {
	XLOG_ASSERT(pim_mfc != NULL);
	if (pim_mre_sg != NULL) {
	    // First delete the old dataflow
	    pim_mfc->delete_dataflow_monitor(threshold_interval_sec,
					     threshold_interval_usec,
					     threshold_packets,
					     threshold_bytes,
					     is_threshold_in_packets,
					     is_threshold_in_bytes,
					     is_geq_upcall,
					     is_leq_upcall);
	    pim_mfc->add_dataflow_monitor(expected_dataflow_monitor_sec, 0,
					  0,	// threshold_packets
					  0,	// threshold_bytes
					  true,	// is_threshold_in_packets
					  false,// is_threshold_in_bytes
					  false,// is_geq_upcall ">="
					  true);// is_leq_upcall "<="
	}
    }
    
    if (pim_mre == NULL) {
	// No such PimMre entry. Silently delete the PimMfc entry.
	delete pim_mfc;
	return (XORP_ERROR);	// No such PimMre entry
    }
    
    if (pim_mre->is_wc()) {
	// TODO: XXX: PAVPAVPAV: check if switch to SPT is desired.
	// TODO: XXX: PAVPAVPAV: if true, remove the dataflow monitor
    }
    
    return (XORP_OK);
    
    UNUSED(threshold_interval_usec);
    UNUSED(measured_interval_sec);
    UNUSED(measured_interval_usec);
    UNUSED(threshold_packets);
    UNUSED(threshold_bytes);
    UNUSED(measured_bytes);
    UNUSED(is_threshold_in_packets);
    UNUSED(is_threshold_in_bytes);
    UNUSED(is_geq_upcall);
    UNUSED(is_leq_upcall);
}
