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

#ident "$XORP: xorp/pim/pim_mre_data.cc,v 1.10 2004/02/22 03:27:24 pavlin Exp $"

//
// PIM Multicast Routing Entry data handling
//


#include "pim_module.h"
#include "pim_private.hh"
#include "pim_proto.h"
#include "pim_mfc.hh"
#include "pim_mre.hh"
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


// Note: applies only for (S,G)
void
PimMre::update_sptbit_sg(uint16_t iif_vif_index)
{
    PimMre *pim_mre_wc = wc_entry();
    PimNbr *pim_nbr_rpfp_nbr_wc = NULL;
    
    if (iif_vif_index == Vif::VIF_INDEX_INVALID)
	return;

    if (! is_sg())
	return;
    
    if (mrib_s() == NULL)
	return;
    
    if (pim_mre_wc != NULL)
	pim_nbr_rpfp_nbr_wc = pim_mre_wc->rpfp_nbr_wc();
    
    if ((iif_vif_index == rpf_interface_s())
	&& is_join_desired_sg()
	&& (is_directly_connected_s()
	    || (rpf_interface_s() != rpf_interface_rp())
	    || (inherited_olist_sg_rpt().none())
	    || ((rpfp_nbr_sg() == pim_nbr_rpfp_nbr_wc)
		&& (rpfp_nbr_sg() != NULL)))) {
	set_spt(true);
    }
}

//
// Return true if monitoring switch to SPT is desired, otherwise false.
// Note: this implements part of the CheckSwitchToSpt(S,G) macro
// (i.e., check_switch_to_spt_sg()).
// Note: applies for all entries
// Note: by spec definition it should apply only for (*,G), (S,G), (S,G,rpt),
// but the RP-based SPT switch extends it for (*,*,RP) entries as well.
//
bool
PimMre::is_monitoring_switch_to_spt_desired_sg(const PimMre *pim_mre_sg) const
{
    Mifset mifs;
    
    //
    // The RP-based SPT switch
    //
    if (i_am_rp())
	return (true);
    
    //
    // The last-hop router SPT switch (as described in the spec)
    //
    if (! (is_wc() || is_sg() || is_sg_rpt()))
	return (false);
    
    mifs = pim_include_wc();
    if (pim_mre_sg != NULL) {
	mifs &= ~pim_mre_sg->pim_exclude_sg();
	mifs |= pim_mre_sg->pim_include_sg();
    }
    
    return (mifs.any());
}

// Policy-defined SPT switch.
// Note: applies for all entries
bool
PimMre::is_switch_to_spt_desired_sg(uint32_t measured_interval_sec,
				    uint32_t measured_bytes) const
{
    if (! pim_node().is_switch_to_spt_enabled().get())
	return (false);		// SPT-switch disabled
    
    //
    // SPT-switch enabled
    //
    
    // Test if the switch was desired already
    if (_flags & PIM_MRE_SWITCH_TO_SPT_DESIRED)
	return (true);
    
    //
    // Test whether the number of forwarded bytes is equal or above
    // the threshold value, and this is within the boundaries of the
    // pre-defined interval.
    //
    if ((measured_bytes >= pim_node().switch_to_spt_threshold_bytes().get())
	&& (measured_interval_sec
	    <= pim_node().switch_to_spt_threshold_interval_sec().get())) {
	return (true);
    }
    
    return (false);
}

// Return true if switch to SPT is desired, otherwise false.
// Note: if switch to SPT is desired, then the (S,G) Keepalive Timer is
// always restarted.
// Note: applies for all entries
// Note: by spec definition it should apply only for (*,G), (S,G), (S,G,rpt),
// but the RP-based SPT switch extends it for (*,*,RP) entries as well.
bool
PimMre::check_switch_to_spt_sg(const IPvX& src, const IPvX& dst,
			       PimMre*& pim_mre_sg,
			       uint32_t measured_interval_sec,
			       uint32_t measured_bytes)
{
    //
    // XXX: Part of the CheckSwitchToSpt(S,G) macro
    // is implemented by is_monitoring_switch_to_spt_desired_sg()
    //
    if (is_monitoring_switch_to_spt_desired_sg(pim_mre_sg)
	&& is_switch_to_spt_desired_sg(measured_interval_sec,
				       measured_bytes)) {
	// restart KeepaliveTimer(S,G);
	if (pim_mre_sg == NULL) {
	    // XXX: create the (S,G) entry
	    pim_mre_sg = pim_node().pim_mrt().pim_mre_find(src, dst,
							   PIM_MRE_SG,
							   PIM_MRE_SG);
	}
	pim_mre_sg->start_keepalive_timer();
	pim_mre_sg->set_switch_to_spt_desired_sg(true);
	return (true);
    }
    
    return (false);
}

// Note: applies only for (S,G)
void
PimMre::set_switch_to_spt_desired_sg(bool v)
{
    if (! is_sg())
	return;

    if (v)
	_flags |= PIM_MRE_SWITCH_TO_SPT_DESIRED;
    else
	_flags &= ~PIM_MRE_SWITCH_TO_SPT_DESIRED;
}
