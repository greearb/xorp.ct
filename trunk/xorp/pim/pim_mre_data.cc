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

#ident "$XORP: xorp/pim/pim_mre_data.cc,v 1.7 2003/07/07 23:13:01 pavlin Exp $"

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
    
    // TODO: XXX: PAVPAVPAV: what about if both rpfp_nbr_sg() and
    // pim_nbr_rpfp_nbr_wc are NULL??
    
    if ((iif_vif_index == rpf_interface_s())
	&& is_join_desired_sg()
	&& (is_directly_connected_s()
	    || (rpf_interface_s() != rpf_interface_rp())
	    || (inherited_olist_sg_rpt().none())
	    || (rpfp_nbr_sg() == pim_nbr_rpfp_nbr_wc))) {
	set_spt(true);
    }
}

// Policy-defined SPT switch.
// Note: applies only for (*,G), (S,G), (S,G,rpt)
bool
PimMre::is_switch_to_spt_desired_sg() const
{
    if (! (is_wc() || is_sg() || is_sg_rpt()))
	return (false);
    
    if (! pim_node().is_switch_to_spt_enabled().get())
	return (false);		// SPT-switch disabled
    
    if (pim_node().is_switch_to_spt_enabled().get()
	&& (pim_node().switch_to_spt_threshold_bytes().get() == 0)) {
	return (true);		// SPT-switch enabled on first packet
    }
    
    return (false);	// TODO: XXX: PAVPAVPAV: temp. no SPT-switch
}

// Return true if switch to SPT is desired, and the Keepalive Timer was
// restarted, otherwise false.
// Note: applies only for (*,G), (S,G), (S,G,rpt)
bool
PimMre::check_switch_to_spt_sg(const IPvX& src, const IPvX& dst,
			       PimMre*& pim_mre_sg)
{
    Mifset mifs;
    
    if (! (is_wc() || is_sg() || is_sg_rpt()))
	return (false);
    
    mifs = pim_include_wc();
    mifs &= ~pim_exclude_sg();
    mifs |= pim_include_sg();
    
    if (mifs.any() && is_switch_to_spt_desired_sg()) {
	// restart KeepAliveTimer(S,G);
	if (pim_mre_sg == NULL) {
	    // XXX: create the (S,G) entry
	    pim_mre_sg = pim_node().pim_mrt().pim_mre_find(src, dst,
							   PIM_MRE_SG,
							   PIM_MRE_SG);
	}
	pim_mre_sg->start_keepalive_timer();
	return (true);
    }
    
    return (false);
}
