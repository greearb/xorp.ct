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

#ident "$XORP: xorp/pim/pim_mre_data.cc,v 1.5 2003/03/27 00:19:03 pavlin Exp $"

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

// TODO: policy-defined SPT switch.
// Here we should implement checking whether the bw is above a threshold
bool
PimMre::is_switch_to_spt_desired_sg() const
{
    if (! pim_node().is_switch_to_spt_enabled().get())
	return (false);		// SPT-switch disabled
    
    if (pim_node().is_switch_to_spt_enabled().get()
	&& (pim_node().switch_to_spt_threshold_bytes().get() == 0)) {
	return (true);		// SPT-switch enabled on first packet
    }
    
    return (false);	// TODO: XXX: PAVPAVPAV: temp. no SPT-switch
}

// Return true if switch to SPT is desired, otherwise false.
// XXX: PimMre::check_switch_to_spt_sg() does NOT restart the
// KeepAliveTimer(S,G). Hence, this timer must be restarted by the function
// that calls PimMre::check_switch_to_spt_sg()
// XXX: can work for any entry, but should apply only
// for (*,G), (S,G), (S,G,rpt)
bool
PimMre::check_switch_to_spt_sg()
{
    Mifset mifs;
    
    mifs = pim_include_wc();
    mifs &= ~pim_exclude_sg();
    mifs |= pim_include_sg();
    
    if (mifs.any() && is_switch_to_spt_desired_sg()) {
	// XXX: need to "restart KeepaliveTimer(S,G);" after return.
	return (true);
    }
    
    return (false);
}

void
PimMre::recompute_check_switch_to_spt_sg()
{
    // TODO: XXX: PAVPAVPAV: take any other actions? E.g., what about
    // "restart KeepaliveTimer(S,G)" comment
    // in PimMre::check_switch_to_spt_sg();?
    check_switch_to_spt_sg();
}
