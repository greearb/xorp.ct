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

#ident "$XORP: xorp/pim/pim_mre.cc,v 1.1.1.1 2002/12/11 23:56:11 hodson Exp $"

//
// PIM Multicast Routing Entry handling
//


#include "pim_module.h"
#include "pim_private.hh"
#include "pim_mre.hh"
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
struct MifsetTimerArgs {
    PimMre		*_pim_mre;
    mifset_timer_func_t	_func;
    uint16_t		_vif_index;
};

//
// Local variables
//

//
// Local functions prototypes
//
static void	mifset_timer_timeout(void *data_pointer);


PimMre::PimMre(PimMrt& pim_mrt, const IPvX& source, const IPvX& group)
    : Mre<PimMre>(source, group),
    _pim_mrt(pim_mrt)
{
    _pim_rp = NULL;
    
    _mrib_rp = NULL;
    _mrib_s = NULL;
    
    _mrib_next_hop_rp = NULL;
    _mrib_next_hop_s = NULL;
    _rpfp_nbr_wc = NULL;
    _rpfp_nbr_sg = NULL;
    _rpfp_nbr_sg_rpt = NULL;
    
    _wc_entry = NULL;
    _rp_entry = NULL;
    _sg_sg_rpt_entry = NULL;
    
    for (size_t i = 0; i < MAX_VIFS; i++)
	_assert_winner_metrics[i] = NULL;
    
#if 0	// TODO
    COUNTER_RESET(_ctimers_decr_value);
#endif
    _flags = 0;
}

PimMre::~PimMre()
{
    //
    // Reset the pointers of the corresponding (S,G) and (S,G,rpt) entries
    // to this PimMre entry.
    //
    do {
	if (is_sg()) {
	    if (sg_rpt_entry() != NULL)
		sg_rpt_entry()->set_sg_entry(NULL);
	    break;
	}
	if (is_sg_rpt()) {
	    if (sg_entry() != NULL)
		sg_entry()->set_sg_rpt_entry(NULL);
	    break;
	}
    } while (false);
    
    for (size_t i = 0; i < MAX_VIFS; i++) {
	if (_assert_winner_metrics[i] != NULL) {
	    delete _assert_winner_metrics[i];
	    _assert_winner_metrics[i] = NULL;
	}
    }

    //
    // Remove this entry from various lists
    //
    remove_pim_mre_lists();
    
    //
    // Remove this entry from the PimMrt table
    //
    pim_mrt().remove_pim_mre(this);
}

//
// Add the PimMre entry to various lists
//
void
PimMre::add_pim_mre_lists()
{
    //
    // Add this entry to various lists towards neighbors
    //
    do {
	if (is_rp()) {
	    // (*,*,RP)
	    if (_mrib_next_hop_rp != NULL) {
		_mrib_next_hop_rp->add_pim_mre(this);
	    } else {
		pim_node().add_pim_mre_no_pim_nbr(this);
	    }
	    break;
	}
	
	if (is_wc()) {
	    // (*,G)
	    if (_mrib_next_hop_rp != NULL) {
		_mrib_next_hop_rp->add_pim_mre(this);
	    } else {
		pim_node().add_pim_mre_no_pim_nbr(this);
	    }
	    if (_rpfp_nbr_wc != _mrib_next_hop_rp) {
		if (_rpfp_nbr_wc != NULL) {
		    _rpfp_nbr_wc->add_pim_mre(this);
		} else {
		    pim_node().add_pim_mre_no_pim_nbr(this);
		}
	    }
	    break;
	}
	
	if (is_sg()) {
	    // (S,G)
	    if (_mrib_next_hop_s != NULL) {
		_mrib_next_hop_s->add_pim_mre(this);
	    } else {
		pim_node().add_pim_mre_no_pim_nbr(this);
	    }
	    if (_rpfp_nbr_sg != _mrib_next_hop_s) {
		if (_rpfp_nbr_sg != NULL) {
		    _rpfp_nbr_sg->add_pim_mre(this);
		} else {
		    pim_node().add_pim_mre_no_pim_nbr(this);
		}
	    }
	    break;
	}
	
	if (is_sg_rpt()) {
	    // (S,G,rpt)
	    if (_rpfp_nbr_sg_rpt != NULL) {
		_rpfp_nbr_sg_rpt->add_pim_mre(this);
	    } else {
		pim_node().add_pim_mre_no_pim_nbr(this);
	    }
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while (false);
    
    //
    // Add this entry to the RP table
    //
    pim_node().rp_table().add_pim_mre(this);
}

//
// Remove the PimMre entry from various lists
//
void
PimMre::remove_pim_mre_lists()
{
    //
    // Remove this entry from various lists towards neighbors
    //
    do {
	if (is_rp()) {
	    // (*,*,RP)
	    if (_mrib_next_hop_rp != NULL) {
		_mrib_next_hop_rp->delete_pim_mre(this);
	    } else {
		pim_node().delete_pim_mre_no_pim_nbr(this);
	    }
	    _mrib_next_hop_rp = NULL;
	    break;
	}
	
	if (is_wc()) {
	    // (*,G)
	    if (_mrib_next_hop_rp != NULL) {
		_mrib_next_hop_rp->delete_pim_mre(this);
	    } else {
		pim_node().delete_pim_mre_no_pim_nbr(this);
	    }
	    if (_rpfp_nbr_wc != _mrib_next_hop_rp) {
		if (_rpfp_nbr_wc != NULL) {
		    _rpfp_nbr_wc->delete_pim_mre(this);
		} else {
		    pim_node().delete_pim_mre_no_pim_nbr(this);
		}
	    }
	    _mrib_next_hop_rp = NULL;
	    _rpfp_nbr_wc = NULL;
	    break;
	}
	
	if (is_sg()) {
	    // (S,G)
	    if (_mrib_next_hop_s != NULL) {
		_mrib_next_hop_s->delete_pim_mre(this);
	    } else {
		pim_node().delete_pim_mre_no_pim_nbr(this);
	    }
	    if (_rpfp_nbr_sg != _mrib_next_hop_s) {
		if (_rpfp_nbr_sg != NULL) {
		    _rpfp_nbr_sg->delete_pim_mre(this);
		} else {
		    pim_node().delete_pim_mre_no_pim_nbr(this);
		}
	    }
	    _mrib_next_hop_s = NULL;
	    _mrib_next_hop_rp = NULL;
	    break;
	}
	
	if (is_sg_rpt()) {
	    // (S,G,rpt)
	    if (_rpfp_nbr_sg_rpt != NULL) {
		_rpfp_nbr_sg_rpt->delete_pim_mre(this);
	    } else {
		pim_node().delete_pim_mre_no_pim_nbr(this);
	    }
	    _rpfp_nbr_sg_rpt = NULL;
	    break;
	}
	
	XLOG_ASSERT(false);
	break;
    } while (false);
    
    //
    // Remove this entry from the RP table
    //
    pim_node().rp_table().delete_pim_mre(this);
}

PimNode&
PimMre::pim_node() const
{
    return (_pim_mrt.pim_node());
}

int
PimMre::family() const
{
    return (_pim_mrt.family());
}

uint16_t
PimMre::pim_register_vif_index() const
{
    return (_pim_mrt.pim_register_vif_index());
}

//
// Mifset functions
//
void
PimMre::set_sg(bool v)
{
    if (v) {
	set_sg_rpt(false);
	set_wc(false);
	set_rp(false);
	_flags |= PIM_MRE_SG;
    } else {
	_flags &= ~PIM_MRE_SG;
    }
}

void
PimMre::set_sg_rpt(bool v)
{
    if (v) {
	set_sg(false);
	set_wc(false);
	set_rp(false);
	_flags |= PIM_MRE_SG_RPT;
    } else {
	_flags &= ~PIM_MRE_SG_RPT;
    }
}

void
PimMre::set_wc(bool v)
{
    if (v) {
	set_sg(false);
	set_sg_rpt(false);
	set_rp(false);
	_flags |= PIM_MRE_WC;
    } else {
	_flags &= ~PIM_MRE_WC;
    }
}

void
PimMre::set_rp(bool v)
{
    if (v) {
	set_sg(false);
	set_sg_rpt(false);
	set_wc(false);
	_flags |= PIM_MRE_RP;
    } else {
	_flags &= ~PIM_MRE_RP;
    }
}

// Note: applies only for (S,G)
void
PimMre::set_spt(bool v)
{
    if (! is_sg())
	return;
    
    if (is_spt() == v)
	return;		// Nothing changed
    
    if (v) {
	_flags |= PIM_MRE_SPT;
    } else {
	_flags &= ~PIM_MRE_SPT;
    }
    
    pim_mrt().add_task_sptbit_sg(source_addr(), group_addr());
}

const IPvX *
PimMre::rp_addr_ptr() const
{
    if (is_rp())
	return (&source_addr());	// XXX: (*,*,RP) entry
    
    if (pim_rp() != NULL)
	return (&pim_rp()->rp_addr());
    
    return (NULL);
}

const string
PimMre::rp_addr_string() const
{
    const IPvX *addr_ptr = rp_addr_ptr();
    
    if (addr_ptr != NULL)
	return (cstring(*addr_ptr));
    else
	return ("RP_ADDR_UNKNOWN");
}

//
// Set of state interface functions (See Section 4.1.6 in I-D ver. 03)
//
// Note: works for all entries
const Mifset&
PimMre::joins_rp() const
{
    static Mifset mifs;
    const PimMre *pim_mre_rp;
    
    if (is_rp()) {
	pim_mre_rp = this;
    } else {
	pim_mre_rp = rp_entry();
	if (pim_mre_rp == NULL) {
	    mifs.reset();
	    return (mifs);
	}
    }
    
    mifs = pim_mre_rp->downstream_join_state();
    mifs |= pim_mre_rp->downstream_prune_pending_state();
    return (mifs);
}

// Note: works for (*,G), (S,G), (S,G,rpt)
const Mifset&
PimMre::joins_wc() const
{
    static Mifset mifs;
    const PimMre *pim_mre_wc;
    
    if (is_wc()) {
	pim_mre_wc = this;
    } else {
	pim_mre_wc = wc_entry();
	if (pim_mre_wc == NULL) {
	    mifs.reset();
	    return (mifs);
	}
    }
    
    mifs = pim_mre_wc->downstream_join_state();
    mifs |= pim_mre_wc->downstream_prune_pending_state();
    return (mifs);
}

// Note: applies only for (S,G)
const Mifset&
PimMre::joins_sg() const
{
    static Mifset mifs;
    
    if (! is_sg()) {
	mifs.reset();
	return (mifs);
    }
    
    mifs = downstream_join_state();
    mifs |= downstream_prune_pending_state();
    return (mifs);
}

// Note: applies only for (S,G,rpt)
const Mifset&
PimMre::prunes_sg_rpt() const
{
    static Mifset mifs;
    
    if (! is_sg_rpt()) {
	mifs.reset();
	return (mifs);
    }
    
    mifs = downstream_prune_state();
    mifs |= downstream_prune_tmp_state();
    return(mifs);    
}

// Note: works for (*,*,RP), (*,G), (S,G), (S,G,rpt)
bool
PimMre::is_join_desired_rp() const
{
    Mifset m;
    
    m = immediate_olist_rp();
    if (m.any())
	return (true);
    else
	return (false);
}

// Note: works for (*,G), (S,G), (S,G,rpt)
bool
PimMre::is_join_desired_wc() const
{
    Mifset m;
    uint16_t vif_index;
    
    m = immediate_olist_wc();
    if (m.any())
	return (true);
    
    vif_index = rpf_interface_rp();
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    if (is_join_desired_rp()
	&& (assert_winner_metric_wc(vif_index) != NULL))
	return (true);
    else
	return (false);
}

// Note: works for (*,G), (S,G), (S,G,rpt)
bool
PimMre::is_rpt_join_desired_g() const
{
    return (is_join_desired_wc() || is_join_desired_rp());
}

// Note: works only for (S,G)
bool
PimMre::is_join_desired_sg() const
{
    Mifset m;
    
    if (! is_sg())
	return (false);
    
    m = immediate_olist_sg();
    if (m.any())
	return (true);
    
    m = inherited_olist_sg();
    if (is_keepalive_timer_running()
	&& m.any())
	return (true);
    else
	return (false);
}

// Note: works only for (S,G,rpt)
bool
PimMre::is_prune_desired_sg_rpt() const
{
    Mifset m;
    PimMre *pim_mre_wc;
    PimMre *pim_mre_sg;
    
    if (! is_sg_rpt())
	return (false);
    
    pim_mre_wc = wc_entry();
    if (pim_mre_wc == NULL)
	return (false);
    
    if (! is_rpt_join_desired_g())
	return (false);
    
    m = inherited_olist_sg_rpt();
    if (m.none())
	return (true);
    
    pim_mre_sg = sg_entry();
    if (pim_mre_sg != NULL) {
	if (pim_mre_sg->is_spt()
	    && (pim_mre_wc->rpfp_nbr_wc() != pim_mre_sg->rpfp_nbr_sg()))
	    return (true);
    }
    
    return (false);
}

// Note: works for (*,*,RP), (*,G), (S,G), (S,G,rpt)
const Mifset&
PimMre::immediate_olist_rp() const
{
    return (joins_rp());
}

// Note: works for (*,G), (S,G), (S,G,rpt)
const Mifset&
PimMre::immediate_olist_wc() const
{
    static Mifset mifs;
    
    mifs = joins_wc();
    mifs |= pim_include_wc();
    mifs &= ~lost_assert_wc();
    
    return (mifs);
}

// Note: works for (S,G)
const Mifset&
PimMre::immediate_olist_sg() const
{
    static Mifset mifs;
    
    if (! is_sg()) {
	mifs.reset();
	return (mifs);
    }
    
    mifs = joins_sg();
    mifs |= pim_include_sg();
    mifs &= ~lost_assert_sg();
    
    return (mifs);
}

// Note: works for (S,G)
const Mifset&
PimMre::inherited_olist_sg() const
{
    static Mifset mifs;
    PimMre *pim_mre_sg_rpt;
    
    if (! is_sg()) {
	mifs.reset();
	return (mifs);
    }
    
    mifs.reset();
    
    pim_mre_sg_rpt = sg_rpt_entry();
    if (pim_mre_sg_rpt != NULL)
	mifs = pim_mre_sg_rpt->inherited_olist_sg_rpt();
    else
	mifs = inherited_olist_sg_rpt_forward();	// XXX
    mifs |= immediate_olist_sg();
    
    return (mifs);
}

// Note: works for (S,G,rpt)
const Mifset&
PimMre::inherited_olist_sg_rpt() const
{
    static Mifset mifs;
    Mifset mifs2;
    PimMre *pim_mre_sg;
    
    if (! is_sg_rpt()) {
	mifs.reset();
	return (mifs);
    }
    
    mifs = joins_rp();
    mifs |= joins_wc();
    mifs &= ~prunes_sg_rpt();
    
    mifs2 = pim_include_wc();
    pim_mre_sg = sg_entry();
    if (pim_mre_sg != NULL)
	mifs2 &= ~(pim_mre_sg->pim_exclude_sg());
    mifs |= mifs2;
    
    mifs2 = lost_assert_wc();
    mifs2 |= lost_assert_sg_rpt();
    mifs &= ~mifs2;
    
    return (mifs);
}

//
// Note: applies for (*,*,RP), (*,G), (S,G), (S,G,rpt)
// XXX: internals MUST be syncronized with PimMre::inherited_olist_sg()
//
const Mifset&
PimMre::inherited_olist_sg_forward() const
{
    static Mifset mifs;
    
    mifs.reset();
    
    do {
	if (is_sg_rpt()) {
	    //
	    // (S,G,rpt)
	    //
	    PimMre *pim_mre_sg = sg_entry();
	    if (pim_mre_sg != NULL) {
		mifs = pim_mre_sg->inherited_olist_sg();
		break;
	    }
	    mifs = inherited_olist_sg_rpt();
	    
	    break;
	}
	if (is_sg()) {
	    //
	    // (S,G)
	    //
	    mifs = inherited_olist_sg();
	    
	    break;
	}
	if (is_wc()) {
	    //
	    // (*,G)
	    //
	    mifs = inherited_olist_sg_rpt_forward();
	    
	    break;
	}
	if (is_rp()) {
	    //
	    // (*,*,RP)
	    //
	    mifs = inherited_olist_sg_rpt_forward();
	    
	    break;
	}
    } while (false);
    
    return (mifs);
}

//
// Note: applies for (*,*,RP), (*,G), (S,G), (S,G,rpt)
// XXX: internals MUST be syncronized with PimMre::inherited_olist_sg_rpt()
//
const Mifset&
PimMre::inherited_olist_sg_rpt_forward() const
{
    static Mifset mifs;
    Mifset mifs2;

    mifs.reset();
    
    do {
	if (is_sg_rpt()) {
	    //
	    // (S,G,rpt)
	    //
	    mifs = inherited_olist_sg_rpt();
	    
	    break;
	}
	if (is_sg()) {
	    //
	    // (S,G)
	    //
	    PimMre *pim_mre_sg_rpt = sg_rpt_entry();
	    if (pim_mre_sg_rpt != NULL) {
		mifs = pim_mre_sg_rpt->inherited_olist_sg_rpt_forward();
		break;
	    }
	    mifs = joins_rp();
	    mifs |= joins_wc();
	    
	    mifs2 = pim_include_wc();
	    mifs2 &= ~pim_exclude_sg();
	    mifs |= mifs2;
	    
	    mifs2 = lost_assert_wc();
	    mifs &= ~mifs2;
	    
	    break;
	}
	if (is_wc()) {
	    //
	    // (*,G)
	    //
	    mifs = joins_rp();
	    mifs |= joins_wc();
	    
	    mifs2 = pim_include_wc();
	    mifs |= mifs2;
	    
	    mifs2 = lost_assert_wc();
	    mifs &= ~mifs2;
	    
	    break;
	}
	if (is_rp()) {
	    //
	    // (*,*,RP)
	    //
	    mifs = joins_rp();
	    
	    break;
	}
    } while (false);
    
    return (mifs);
}

// Note: works for (*,G), (S,G), (S,G,rpt)
const Mifset&
PimMre::pim_include_wc() const
{
    static Mifset mifs;
    
    mifs = i_am_dr();
    mifs &= ~lost_assert_wc();
    mifs |= i_am_assert_winner_wc();
    mifs &= local_receiver_include_wc();
    
    return (mifs);
}

// Note: works only for (S,G)
const Mifset&
PimMre::pim_include_sg() const
{
    static Mifset mifs;
    
    mifs = i_am_dr();
    mifs &= ~lost_assert_sg();
    mifs |= i_am_assert_winner_sg();
    mifs &= local_receiver_include_sg();
    
    return (mifs);
}

// Note: works only for (S,G)
const Mifset&
PimMre::pim_exclude_sg() const
{
    static Mifset mifs;
    
    mifs = i_am_dr();
    mifs &= ~lost_assert_sg();
    mifs |= i_am_assert_winner_sg();
    mifs &= local_receiver_exclude_sg();
    
    return (mifs);
}

// Note: works only for (S,G)
const Mifset&
PimMre::local_receiver_include_sg() const
{
    static Mifset mifs;
    
    if (! is_sg()) {
	mifs.reset();
	return (mifs);
    }
    
    return (local_receiver_include());
}

// Note: works for (*,G), (S,G), (S,G,rpt)
const Mifset&
PimMre::local_receiver_include_wc() const
{
    static Mifset mifs;
    const PimMre *pim_mre_wc;
    
    if (is_wc()) {
	pim_mre_wc = this;
    } else {
	pim_mre_wc = wc_entry();
	if (pim_mre_wc == NULL) {
	    mifs.reset();
	    return (mifs);
	}
    }
    
    return (pim_mre_wc->local_receiver_include());
}

const Mifset&
PimMre::local_receiver_exclude_sg() const
{
    static Mifset mifs;
    
    if (! is_sg()) {
	mifs.reset();
	return (mifs);
    }
    
    mifs = local_receiver_include_wc();
    mifs &= local_receiver_exclude();
    
    return (mifs);
}

void
PimMre::set_local_receiver_include(uint16_t vif_index, bool v)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    if (_local_receiver_include.test(vif_index) == v)
	return;			// Nothing changed
    
    if (v)
	_local_receiver_include.set(vif_index);
    else
	_local_receiver_include.reset(vif_index);
    
    // Add the task to recompute the effect of this and take actions
    do {
	if (is_wc()) {
	    pim_mrt().add_task_local_receiver_include_wc(vif_index,
							 group_addr());
	    break;
	}
	if (is_sg()) {
	    pim_mrt().add_task_local_receiver_include_sg(vif_index,
							 source_addr(),
							 group_addr());
	    break;
	}
    } while (false);
}

void
PimMre::set_local_receiver_exclude(uint16_t vif_index, bool v)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    if (_local_receiver_exclude.test(vif_index) == v)
	return;			// Nothing changed
    
    if (v)
	_local_receiver_exclude.set(vif_index);
    else
	_local_receiver_exclude.reset(vif_index);

    // Add the task to recompute the effect of this and take actions
    do {
	if (is_sg()) {
	    pim_mrt().add_task_local_receiver_exclude_sg(vif_index,
							 source_addr(),
							 group_addr());
	    break;
	}
    } while (false);
}

// Note: works for (*,G), (S,G), (S,G,rpt)
const Mifset&
PimMre::i_am_assert_winner_wc() const
{
    static Mifset mifs;
    const PimMre *pim_mre_wc;
    
    if (is_wc()) {
	pim_mre_wc = this;
    } else {
	pim_mre_wc = wc_entry();
	if (pim_mre_wc == NULL) {
	    mifs.reset();
	    return (mifs);
	}
    }
    
    mifs = pim_mre_wc->i_am_assert_winner_state();
    
    return (mifs);
}

const Mifset&
PimMre::i_am_assert_winner_sg() const
{
    static Mifset mifs;
    
    if (! is_sg()) {
	mifs.reset();
	return (mifs);
    }
    
    mifs = i_am_assert_winner_state();
    
    return (mifs);
}

// Note: applies for (*,G), (S,G), (S,G,rpt)
const Mifset&
PimMre::i_am_assert_loser_wc() const
{
    static Mifset mifs;
    const PimMre *pim_mre_wc;
    
    if (is_wc()) {
	pim_mre_wc = this;
    } else {
	pim_mre_wc = wc_entry();
	if (pim_mre_wc == NULL) {
	    mifs.reset();
	    return (mifs);
	}
    }
    
    mifs = pim_mre_wc->i_am_assert_loser_state();
    
    return (mifs);
}

// Note: applies only for (S,G)
const Mifset&
PimMre::i_am_assert_loser_sg() const
{
    static Mifset mifs;
    
    if (! is_sg()) {
	mifs.reset();
	return (mifs);
    }
    
    mifs = i_am_assert_loser_state();
    
    return (mifs);
}

// Note: applies for (*,G), (S,G), (S,G,rpt)
const Mifset&
PimMre::lost_assert_wc() const
{
    static Mifset mifs;
    uint16_t vif_index;
    
    mifs = i_am_assert_loser_wc();
    vif_index = rpf_interface_rp();
    if (vif_index != Vif::VIF_INDEX_INVALID)
	mifs.reset(vif_index);
    
    return (mifs);
}

// Note: applies only for (S,G)
const Mifset&
PimMre::lost_assert_sg() const
{
    static Mifset mifs;
    uint16_t vif_index;
    
    if (! is_sg()) {
	mifs.reset();
	return (mifs);
    }
    
    mifs = i_am_assert_loser_sg();
    mifs &= assert_winner_metric_is_better_than_spt_assert_metric_sg();	// XXX
    vif_index = rpf_interface_s();
    if (vif_index != Vif::VIF_INDEX_INVALID)
	mifs.reset(vif_index);
    
    return (mifs);
}

// Note: applies only for (S,G,rpt)
const Mifset&
PimMre::lost_assert_sg_rpt() const
{
    static Mifset mifs;
    PimMre *pim_mre_sg;
    uint16_t vif_index;
    
    if (! is_sg_rpt()) {
	mifs.reset();
	return (mifs);
    }
    
    mifs.reset();
    
    pim_mre_sg = sg_entry();
    if (pim_mre_sg != NULL)
	mifs = pim_mre_sg->i_am_assert_loser_sg();
    
    vif_index = rpf_interface_rp();
    if (vif_index != Vif::VIF_INDEX_INVALID)
	mifs.reset(vif_index);
    
    if (pim_mre_sg != NULL) {
	if (pim_mre_sg->is_spt()) {
	    vif_index = pim_mre_sg->rpf_interface_s();
	    if (vif_index != Vif::VIF_INDEX_INVALID)
		mifs.reset(vif_index);
	}
    }
    
    return (mifs);
}

// Note: applies only for (S,G)
const Mifset&
PimMre::could_assert_sg() const
{
    static Mifset mifs;
    Mifset mifs2;
    PimMre *pim_mre_sg_rpt;
    uint16_t vif_index;
    
    if (! is_sg()) {
	mifs.reset();
	return (mifs);
    }
    
    if (! is_spt()) {		// XXX: SPTbit(S,G) must be true
	mifs.reset();
	return (mifs);
    }
    
    mifs = joins_rp();
    mifs |= joins_wc();
    pim_mre_sg_rpt = sg_rpt_entry();
    if (pim_mre_sg_rpt != NULL)
	mifs &= ~(pim_mre_sg_rpt->prunes_sg_rpt());
    
    mifs2 = pim_include_wc();
    mifs2 &= ~pim_exclude_sg();
    mifs |= mifs2;
    
    mifs &= ~lost_assert_wc();
    mifs |= joins_sg();
    mifs |= pim_include_sg();
    
    vif_index = rpf_interface_s();
    if (vif_index != Vif::VIF_INDEX_INVALID)
	mifs.reset(vif_index);
    
    return (mifs);
}

// TODO: XXX: PAVPAVPAV: make sure that this is not called for (*,*,RP)
// TODO: XXX: PAVPAVPAV: clarify for which entries this can be called!
const Mifset&
PimMre::could_assert_wc() const
{
    static Mifset mifs;
    uint16_t vif_index;
    
    mifs = joins_rp();
    mifs |= joins_wc();
    mifs |= pim_include_wc();
    
    vif_index = rpf_interface_rp();
    if (vif_index != Vif::VIF_INDEX_INVALID)
	mifs.reset(vif_index);
    
    return (mifs);
}

//
// ASSERT state (per interface)
//
void
PimMre::set_could_assert_state(uint16_t vif_index)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    if (is_could_assert_state(vif_index))
	return;			// Nothing changed
    
    _could_assert_state.set(vif_index);
}

void
PimMre::reset_could_assert_state(uint16_t vif_index)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    if (! is_could_assert_state(vif_index))
	return;			// Nothing changed
    
    _could_assert_state.reset(vif_index);
}

bool
PimMre::is_could_assert_state(uint16_t vif_index) const
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    return (_could_assert_state.test(vif_index));
}

const Mifset&
PimMre::assert_tracking_desired_sg() const
{
    static Mifset mifs;
    Mifset mifs2;
    PimMre *pim_mre_sg_rpt;
    uint16_t vif_index;
    
    if (! is_sg()) {
	mifs.reset();
	return (mifs);
    }
    
    mifs = joins_rp();
    mifs |= joins_wc();
    
    pim_mre_sg_rpt = sg_rpt_entry();
    if (pim_mre_sg_rpt != NULL)
	mifs &= ~(pim_mre_sg_rpt->prunes_sg_rpt());
    
    mifs2 = pim_include_wc();
    mifs2 &= ~pim_exclude_sg();
    mifs |= mifs2;

    mifs2 &= ~lost_assert_wc();
    mifs |= joins_sg();

    mifs2 = i_am_dr();
    mifs2 |= i_am_assert_winner_sg();
    mifs2 &= local_receiver_include_sg();
    mifs |= mifs2;
    
    if (is_join_desired_sg()) {
	vif_index = rpf_interface_s();
	if (vif_index != Vif::VIF_INDEX_INVALID)
	    mifs.set(vif_index);
    }
    if (is_join_desired_wc() && (! is_spt())) {
	vif_index = rpf_interface_rp();
	if (vif_index != Vif::VIF_INDEX_INVALID)
	    mifs.set(vif_index);
    }
    
    return (mifs);
}

const Mifset&
PimMre::assert_tracking_desired_wc() const
{
    static Mifset mifs;
    Mifset mifs2;
    uint16_t vif_index;
    
    mifs = could_assert_wc();
    
    mifs2 = i_am_dr();
    mifs2 |= i_am_assert_winner_wc();
    mifs2 &= local_receiver_include_wc();
    mifs |= mifs2;
    
    if (is_rpt_join_desired_g()) {
	vif_index = rpf_interface_rp();
	if (vif_index != Vif::VIF_INDEX_INVALID)
	    mifs.set(vif_index);
    }
    
    return (mifs);
}

void
PimMre::set_assert_tracking_desired_state(uint16_t vif_index)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    if (is_assert_tracking_desired_state(vif_index))
	return;			// Nothing changed
    
    _assert_tracking_desired_state.set(vif_index);
}

void
PimMre::reset_assert_tracking_desired_state(uint16_t vif_index)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    if (! is_assert_tracking_desired_state(vif_index))
	return;			// Nothing changed
    
    _assert_tracking_desired_state.reset(vif_index);
}

bool
PimMre::is_assert_tracking_desired_state(uint16_t vif_index) const
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    return (_assert_tracking_desired_state.test(vif_index));
}


// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_assert_tracking_desired_sg(uint16_t vif_index)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    if (! is_sg())
	return (false);
    
    if (is_i_am_assert_loser_state(vif_index))
	goto assert_loser_state_label;
    return (false);
    
 assert_loser_state_label:
    // IamAssertLoser state
    if (! is_assert_tracking_desired_state(vif_index))
	return (false);		// The old AssertTrackingDesired(S,G,I)
				// was already false
    // The old AssertTrackingDesired(S,G,I) is true
    if (assert_tracking_desired_sg().test(vif_index))
	return (false);		// No change in CouldAssert(S,G,I)
    // AssertTrackingDesired(S,G,I) -> FALSE
    reset_assert_tracking_desired_state(vif_index);
    set_assert_noinfo_state(vif_index);
    goto a5;
    
 a5:
    //  * Delete assert info (AssertWinner(S,G,I),
    //	  and AssertWinnerMetric(S,G,I) assume default values).
    delete_assert_winner_metric_sg(vif_index);
    // TODO: anything else to remove?
    set_assert_noinfo_state(vif_index);
    return (true);
}

// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_assert_tracking_desired_wc(uint16_t vif_index)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    if (! is_wc())
	return (false);

    if (is_i_am_assert_loser_state(vif_index))
	goto assert_loser_state_label;
    return (false);
    
 assert_loser_state_label:
    // IamAssertLoser state
    if (! is_assert_tracking_desired_state(vif_index))
	return (false);		// The old AssertTrackingDesired(*,G,I)
				// was already false
    // The old AssertTrackingDesired(*,G,I) is true
    if (assert_tracking_desired_wc().test(vif_index))
	return (false);		// No change in CouldAssert(*,G,I)
    // AssertTrackingDesired(*,G,I) -> FALSE
    reset_assert_tracking_desired_state(vif_index);
    set_assert_noinfo_state(vif_index);
    goto a5;
    
 a5:
    //  * Delete assert info (AssertWinner(*,G,I),
    //	  and AssertWinnerMetric(*,G,I) assume default values).
    delete_assert_winner_metric_wc(vif_index);
    // TODO: anything else to remove?
    set_assert_noinfo_state(vif_index);
    return (true);
}

// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_my_assert_metric_sg(uint16_t vif_index)
{
    AssertMetric *my_assert_metric, *winner_metric;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    if (! is_sg())
	return (false);
    
    if (is_i_am_assert_loser_state(vif_index))
	goto assert_loser_state_label;
    return (false);
    
 assert_loser_state_label:
    // IamAssertLoser state
    my_assert_metric = my_assert_metric_sg(vif_index);
    winner_metric = assert_winner_metric_sg(vif_index);
    XLOG_ASSERT(winner_metric != NULL);
    XLOG_ASSERT(my_assert_metric->addr() != winner_metric->addr());
    // Test if my metric has become better
    if (! my_assert_metric->is_better(winner_metric))
	return (false);
    goto a5;
    
 a5:
    //  * Delete assert info (AssertWinner(S,G,I),
    //	  and AssertWinnerMetric(S,G,I) assume default values).
    delete_assert_winner_metric_sg(vif_index);
    // TODO: anything else to remove?
    set_assert_noinfo_state(vif_index);
    return (true);
}

// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_my_assert_metric_wc(uint16_t vif_index)
{
    AssertMetric *my_assert_metric, *winner_metric;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    if (! is_wc())
	return (false);
    
    if (is_i_am_assert_loser_state(vif_index))
	goto assert_loser_state_label;
    return (false);
    
 assert_loser_state_label:
    // IamAssertLoser state
    my_assert_metric = rpt_assert_metric(vif_index);
    winner_metric = assert_winner_metric_wc(vif_index);
    XLOG_ASSERT(winner_metric != NULL);	// TODO: is this assert OK? E.g, what about if loser to (S,G) Winner?
    XLOG_ASSERT(my_assert_metric->addr() != winner_metric->addr());
    // Test if my metric has become better
    if (! my_assert_metric->is_better(winner_metric))
	return (false);
    goto a5;
    
 a5:
    //  * Delete assert info (AssertWinner(*,G,I),
    //	  and AssertWinnerMetric(*,G,I) assume default values).
    delete_assert_winner_metric_wc(vif_index);
    // TODO: anything else to remove?
    set_assert_noinfo_state(vif_index);
    return (true);
}

//
// "RPF_interface(S) stops being I"
// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_assert_rpf_interface_sg(uint16_t vif_index)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    if (! is_sg())
	return (false);
    
    if (is_i_am_assert_loser_state(vif_index))
	goto assert_loser_state_label;
    return (false);
    
 assert_loser_state_label:
    // IamAssertLoser state
    if (rpf_interface_s() == vif_index)
	return (false);			// Nothing has changed
    goto a5;
    
 a5:
    //  * Delete assert info (AssertWinner(S,G,I),
    //	  and AssertWinnerMetric(S,G,I) assume default values).
    delete_assert_winner_metric_sg(vif_index);
    // TODO: anything else to remove?
    set_assert_noinfo_state(vif_index);
    return (true);
}

//
// "RPF_interface(RP(G)) stops being I"
// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_assert_rpf_interface_wc(uint16_t vif_index)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    if (! is_wc())
	return (false);
    
    if (is_i_am_assert_loser_state(vif_index))
	goto assert_loser_state_label;
    return (false);
    
 assert_loser_state_label:
    // IamAssertLoser state
    if (rpf_interface_rp() == vif_index)
	return (false);			// Nothing has changed
    goto a5;
    
 a5:
    //  * Delete assert info (AssertWinner(*,G,I),
    //	  and AssertWinnerMetric(*,G,I) assume default values).
    delete_assert_winner_metric_wc(vif_index);
    // TODO: anything else to remove?
    set_assert_noinfo_state(vif_index);
    return (true);
}

// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_assert_receive_join_sg(uint16_t vif_index)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    if (! is_sg())
	return (false);
    
    if (is_i_am_assert_loser_state(vif_index))
	goto assert_loser_state_label;
    return (false);
    
 assert_loser_state_label:
    // IamAssertLoser state
    goto a5;
    
 a5:
    //  * Delete assert info (AssertWinner(S,G,I),
    //	  and AssertWinnerMetric(S,G,I) assume default values).
    delete_assert_winner_metric_sg(vif_index);
    // TODO: anything else to remove?
    set_assert_noinfo_state(vif_index);
    return (true);
}

// Return true if state has changed, otherwise return false.
bool
PimMre::recompute_assert_receive_join_wc(uint16_t vif_index)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (false);
    
    if (! is_wc())
	return (false);
    
    if (is_i_am_assert_loser_state(vif_index))
	goto assert_loser_state_label;
    return (false);
    
 assert_loser_state_label:
    // IamAssertLoser state
    goto a5;
    
 a5:
    //  * Delete assert info (AssertWinner(*,G,I),
    //	  and AssertWinnerMetric(*,G,I) assume default values).
    delete_assert_winner_metric_wc(vif_index);
    // TODO: anything else to remove?
    set_assert_noinfo_state(vif_index);
    return (true);
}

AssertMetric *
PimMre::my_assert_metric_sg(uint16_t vif_index) const
{
    Mifset mifs;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (NULL);
    
    if (! is_sg())
	return (NULL);
    
    mifs = could_assert_sg();
    if (mifs.test(vif_index))
	return (spt_assert_metric(vif_index));
    
    mifs = could_assert_wc();
    if (mifs.test(vif_index))
	return (rpt_assert_metric(vif_index));
    
    return (infinite_assert_metric());
}

AssertMetric *
PimMre::spt_assert_metric(uint16_t vif_index) const
{
    static AssertMetric assert_metric(IPvX::ZERO(family()));
    PimVif *pim_vif;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (NULL);
    
    if (! is_sg())
	return (NULL);
    
    pim_vif = pim_mrt().vif_find_by_vif_index(vif_index);
    if (pim_vif == NULL)
	return (NULL);
    
    assert_metric.set_addr(pim_vif->addr());
    assert_metric.set_rpt_bit_flag(false);
    assert_metric.set_metric_preference(metric_preference_s());
    assert_metric.set_route_metric(route_metric_s());
    
    return (&assert_metric);
}

AssertMetric *
PimMre::rpt_assert_metric(uint16_t vif_index) const
{
    static AssertMetric assert_metric(IPvX::ZERO(family()));
    PimVif *pim_vif;
    const PimMre *pim_mre_wc;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return (NULL);
    
    if (is_wc()) {
	pim_mre_wc = this;
    } else {
	pim_mre_wc = wc_entry();
	if (pim_mre_wc == NULL)
	    return (NULL);
    }
    
    pim_vif = pim_mrt().vif_find_by_vif_index(vif_index);
    if (pim_vif == NULL)
	return (NULL);
    
    assert_metric.set_addr(pim_vif->addr());
    assert_metric.set_rpt_bit_flag(true);
    assert_metric.set_metric_preference(pim_mre_wc->metric_preference_rp());
    assert_metric.set_route_metric(pim_mre_wc->route_metric_rp());
    
    return (&assert_metric);
}

AssertMetric *
PimMre::infinite_assert_metric() const
{
    static AssertMetric assert_metric(IPvX::ZERO(family()));
    
    // XXX: in case of Assert, the IP address with minimum value is the loser
    assert_metric.set_addr(IPvX::ZERO(family()));
    assert_metric.set_rpt_bit_flag(true);
    assert_metric.set_metric_preference(PIM_ASSERT_MAX_METRIC_PREFERENCE);
    assert_metric.set_route_metric(PIM_ASSERT_MAX_METRIC);
    
    return (&assert_metric);
}

// Note: applies only for (S,G) and (S,G,rpt)
uint32_t
PimMre::route_metric_s() const
{
    Mrib *mrib = mrib_s();
    
    if (mrib != NULL)
	return (mrib->metric());
    
    return (PIM_ASSERT_MAX_METRIC);
}

// Note: applies for all entries
uint32_t
PimMre::route_metric_rp() const
{
    Mrib *mrib = mrib_rp();
    
    if (mrib != NULL)
	return (mrib->metric());
    
    return (PIM_ASSERT_MAX_METRIC);
}

// Note: applies only for (S,G) and (S,G,rpt)
uint32_t
PimMre::metric_preference_s() const
{
    Mrib *mrib = mrib_s();
    
    if (mrib != NULL)
	return (mrib->metric_preference());
    
    return (PIM_ASSERT_MAX_METRIC_PREFERENCE);
}

// Note: applies for all entries
uint32_t
PimMre::metric_preference_rp() const
{
    Mrib *mrib = mrib_rp();
    
    if (mrib != NULL)
	return (mrib->metric_preference());
    
    return (PIM_ASSERT_MAX_METRIC_PREFERENCE);
}

// Try to remove PimMre entry if not needed anymore.
// The function is generic and can be applied to any type of PimMre entry.
// Return true if entry is scheduled to be removed, othewise false.
bool
PimMre::entry_try_remove()
{
    bool ret_value;
    
    // TODO: XXX: PAVPAVPAV: make sure it is called
    // from all appropriate places!!
    
    ret_value = entry_can_remove();
    
    if (ret_value) {
	pim_mrt().add_task_delete_pim_mre(this);
    }
    
    return (ret_value);
}

// Test if it is OK to remove PimMre entry.
// It tests whether all downstream and upstream
// states are NoInfo, as well as whether no relevant timers are running.
// The function is generic and can be applied to any type of PimMre entry.
// Return true if entry can be removed, othewise false.
bool
PimMre::entry_can_remove() const
{
    // TODO: XXX: PAVPAVPAV: add more checks if needed
    
    if (_local_receiver_include.any())
	return (false);
    if (_local_receiver_exclude.any())
	return (false);
    if (_downstream_join_state.any())
	return (false);
    if (_downstream_prune_state.any())
	return (false);
    if (_downstream_prune_pending_state.any())
	return (false);
    // XXX: we don't really need to test _downstream_tmp_state, because
    // it is used only in combination with other state, but anyway...
    if (_downstream_tmp_state.any())
	return (false);
    if (is_rp() || is_wc() || is_sg()) {
	if (is_joined_state())
	    return (false);
    }
    if (is_rp()) {
	if (immediate_olist_rp().any())
	    return (false);
	if ((rp_addr_ptr() != NULL)
	    && pim_node().rp_table().has_rp_addr(*rp_addr_ptr())) {
	    return (false);
	}
    }
    if (is_wc()) {
	if (immediate_olist_wc().any())
	    return (false);
	if (pim_include_wc().any())
	    return (false);
    }
    if (is_sg()) {
	if (immediate_olist_sg().any())
	    return (false);
	if (pim_include_sg().any())
	    return (false);
	if (pim_exclude_sg().any())
	    return (false);
    }
#if 0		// TODO: XXX: PAVPAVPAV: not needed?
    // XXX: (S,G,rpt) entry can be removed if all downstream state is NoInfo
    if (is_sg_rpt()) {
	if (is_pruned_state() || is_not_pruned_state())
	    return (false);
    }
#endif // 0
#if 0		// TODO: XXX: PAVPAVPAV: not needed?
    if (inherited_olist_sg_forward().any())
	return (false);
    if (inherited_olist_sg_rpt_forward().any())
	return (false);
#endif // 0
#if 1		// TODO: XXX: PAVPAVPAV: not needed?
    if (is_sg()) {
	if (is_register_join_state()
	    || is_register_prune_state()
	    || is_register_join_pending_state())
	    return (false);
	if (is_could_register_sg())
	    return (false);
    }
#endif // 1
    
    if (is_wc() || is_sg() || is_sg_rpt()) {	// TODO: apply for (S,G,rpt)?
	if (i_am_assert_winner_state().any()
	    || i_am_assert_loser_state().any())
	    return (false);
    }
    
    if (is_sg()) {
	// TODO: OK NOT to remove if the KeepaliveTimer(S,G) is running?
	if (is_keepalive_timer_running())
	    return (false);
    }
    
    return (true);
}

// The KeepaliveTimer(S,G)
// XXX: applies only for (S,G)
void
PimMre::start_keepalive_timer()
{
    if (! is_sg())
	return;
    
    if (is_keepalive_timer_running())
	return;		// Nothing changed
    
    _flags |= PIM_MRE_KEEPALIVE_TIMER_IS_SET;
    
    pim_mrt().add_task_keepalive_timer_sg(source_addr(), group_addr());
}

// The KeepaliveTimer(S,G)
// XXX: applies only for (S,G)
void
PimMre::cancel_keepalive_timer()
{
    if (! is_sg())
	return;
    
    if (! is_keepalive_timer_running())
	return;		// Nothing changed
    
    _flags &= ~PIM_MRE_KEEPALIVE_TIMER_IS_SET;
    
    pim_mrt().add_task_keepalive_timer_sg(source_addr(), group_addr());
}

// The KeepaliveTimer(S,G)
// XXX: applies only for (S,G)
bool
PimMre::is_keepalive_timer_running() const
{
    if (! is_sg())
	return (false);
    
    return (_flags & PIM_MRE_KEEPALIVE_TIMER_IS_SET);
}

// The KeepaliveTimer(S,G)
// XXX: applies only for (S,G)
void
PimMre::keepalive_timer_timeout()
{
    if (! is_sg())
	return;
    
    if (! is_keepalive_timer_running())
	return;
    cancel_keepalive_timer();
    
    // TODO: XXX: PAVPAVPAV: is it OK to unconditionally remove the entry?
    // entry_try_remove();
    pim_mrt().add_task_delete_pim_mre(this);
}

//
// MISC. info
//
// Note: applies for all entries
const Mifset&
PimMre::i_am_dr() const
{
    return pim_mrt().i_am_dr();
}

//
// MifsetTimer related functions
//
void
PimMre::mifset_timer_start(MifsetTimers& mifset_timers, uint16_t vif_index,
			   uint32_t delay_sec, uint32_t delay_usec,
			   mifset_timer_func_t func)
{
    MifsetTimerArgs *mifset_timer_args = new MifsetTimerArgs;
    
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    mifset_timer_args->_pim_mre = this;
    mifset_timer_args->_func = func;
    mifset_timer_args->_vif_index = vif_index;
    
    mifset_timers.set_mif_timer(vif_index, delay_sec, delay_usec,
				mifset_timer_timeout, mifset_timer_args);
}

static void
mifset_timer_timeout(void *data_pointer)
{
    MifsetTimerArgs *m = (MifsetTimerArgs *)data_pointer;
    
    (m->_pim_mre->*m->_func)(m->_vif_index);
    
    delete m;
}

uint16_t
PimMre::mifset_timer_remain(MifsetTimers& mifset_timers, uint16_t vif_index)
{
    struct timeval timeval;
    
    // TODO: the return value should be <= 65535
    return (mifset_timers.mif_timer_remain(vif_index, timeval));
}

void
PimMre::mifset_timer_cancel(MifsetTimers& mifset_timers, uint16_t vif_index)
{
    if (vif_index == Vif::VIF_INDEX_INVALID)
	return;
    
    return (mifset_timers.cancel_mif_timer(vif_index));
}

//
// PimVif-related methods
//
void
PimMre::recompute_start_vif_rp(uint16_t vif_index)
{
    // XXX: nothing to do
    
    UNUSED(vif_index);
}

void
PimMre::recompute_start_vif_wc(uint16_t vif_index)
{
    // XXX: nothing to do
    
    UNUSED(vif_index);
}

void
PimMre::recompute_start_vif_sg(uint16_t vif_index)
{
    // XXX: nothing to do
    
    UNUSED(vif_index);
}

void
PimMre::recompute_start_vif_sg_rpt(uint16_t vif_index)
{
    // XXX: nothing to do
    
    UNUSED(vif_index);
}

void
PimMre::recompute_stop_vif_rp(uint16_t vif_index)
{
    //
    // Reset all associated state with 'vif_index'
    //
    
    downstream_prune_pending_timer_timeout_rp(vif_index);
    mifset_timer_cancel(_downstream_prune_pending_timers, vif_index);
    downstream_expiry_timer_timeout_rp(vif_index);
    mifset_timer_cancel(_downstream_expiry_timers, vif_index);
    
    mifset_timer_cancel(assert_timers, vif_index);
    reset_assert_tracking_desired_state(vif_index);
    reset_could_assert_state(vif_index);
    // TODO: reset '_asserts_rate_limit'
    set_assert_noinfo_state(vif_index);
    
    set_local_receiver_include(vif_index, false);
    set_local_receiver_exclude(vif_index, false);
    set_downstream_noinfo_state(vif_index);
}

void
PimMre::recompute_stop_vif_wc(uint16_t vif_index)
{
    //
    // Reset all associated state with 'vif_index'
    //
    
    downstream_prune_pending_timer_timeout_wc(vif_index);
    mifset_timer_cancel(_downstream_prune_pending_timers, vif_index);
    downstream_expiry_timer_timeout_wc(vif_index);
    mifset_timer_cancel(_downstream_expiry_timers, vif_index);
    
    assert_timer_timeout_wc(vif_index);
    delete_assert_winner_metric_wc(vif_index);
    
    mifset_timer_cancel(assert_timers, vif_index);
    reset_assert_tracking_desired_state(vif_index);
    reset_could_assert_state(vif_index);
    // TODO: reset '_asserts_rate_limit'
    set_assert_noinfo_state(vif_index);
    
    set_local_receiver_include(vif_index, false);
    set_local_receiver_exclude(vif_index, false);
    set_downstream_noinfo_state(vif_index);
}

void
PimMre::recompute_stop_vif_sg(uint16_t vif_index)
{
    //
    // Reset all associated state with 'vif_index'
    //
    
    downstream_prune_pending_timer_timeout_sg(vif_index);
    mifset_timer_cancel(_downstream_prune_pending_timers, vif_index);
    downstream_expiry_timer_timeout_sg(vif_index);
    mifset_timer_cancel(_downstream_expiry_timers, vif_index);
    
    assert_timer_timeout_sg(vif_index);
    delete_assert_winner_metric_sg(vif_index);
    set_assert_winner_metric_is_better_than_spt_assert_metric_sg(vif_index, false);
    
    mifset_timer_cancel(assert_timers, vif_index);
    reset_assert_tracking_desired_state(vif_index);
    reset_could_assert_state(vif_index);
    // TODO: reset '_asserts_rate_limit'
    set_assert_noinfo_state(vif_index);
    
    set_local_receiver_include(vif_index, false);
    set_local_receiver_exclude(vif_index, false);
    set_downstream_noinfo_state(vif_index);
}

void
PimMre::recompute_stop_vif_sg_rpt(uint16_t vif_index)
{
    //
    // Reset all associated state with 'vif_index'
    //
    
    downstream_prune_pending_timer_timeout_sg_rpt(vif_index);
    mifset_timer_cancel(_downstream_prune_pending_timers, vif_index);
    downstream_expiry_timer_timeout_sg_rpt(vif_index);
    mifset_timer_cancel(_downstream_expiry_timers, vif_index);
    
    mifset_timer_cancel(assert_timers, vif_index);
    reset_assert_tracking_desired_state(vif_index);
    reset_could_assert_state(vif_index);
    // TODO: reset '_asserts_rate_limit'
    set_assert_noinfo_state(vif_index);
    
    set_local_receiver_include(vif_index, false);
    set_local_receiver_exclude(vif_index, false);
    set_downstream_noinfo_state(vif_index);
}

// XXX: applies for (*,*,RP), (*,G), (S,G), (S,G,rpt)
void
PimMre::add_pim_mre_rp_entry()
{
    if (is_wc() || is_sg() || is_sg_rpt()) {
	// XXX: the RP-related state is set by a special task
	// uncond_set_pim_rp(compute_rp());
    }
}

// XXX: applies for (*,G), (S,G), (S,G,rpt)
void
PimMre::add_pim_mre_wc_entry()
{
    if (is_sg() || is_sg_rpt()) {
	PimMre *pim_mre_wc = pim_mrt().pim_mre_find(source_addr(),
						    group_addr(),
						    PIM_MRE_WC,
						    0);
	if (pim_mre_wc == wc_entry())
	    return;		// Nothing changed
	// Remove from the PimNbr and PimRp lists.
	//
	// Remove this entry from the RP table lists.
	// XXX: We don't delete this entry from the PimNbr lists,
	// because an (S,G) or (S,G,rpt) is on those lists regardless
	// whether it has a matching (*,G) entry.
	XLOG_ASSERT(pim_mre_wc != NULL);
	pim_node().rp_table().delete_pim_mre(this);
	set_wc_entry(pim_mre_wc);
    }
}

// XXX: applies for (S,G), (S,G,rpt)
void
PimMre::add_pim_mre_sg_entry()
{
    // XXX: the cross-pointers between (S,G) and and (S,G,rpt) are set
    // when the PimMre entry was created
}

// XXX: applies for (S,G), (S,G,rpt)
void
PimMre::add_pim_mre_sg_rpt_entry()
{
    // XXX: the cross-pointers between (S,G) and and (S,G,rpt) are set
    // when the PimMre entry was created
}

// XXX: applies for (*,*,RP), (*,G), (S,G), (S,G,rpt)
void
PimMre::remove_pim_mre_rp_entry()
{
    if (is_wc() || is_sg() || is_sg_rpt()) {
	// XXX: the RP-related state is set by a special task
	// uncond_set_pim_rp(compute_rp());
    }
    if (is_rp())
	set_is_task_delete_done(true);
}

// XXX: applies for (*,G), (S,G), (S,G,rpt)
void
PimMre::remove_pim_mre_wc_entry()
{
    if (is_sg() || is_sg_rpt()) {
	PimMre *pim_mre_wc = pim_mrt().pim_mre_find(source_addr(),
						    group_addr(),
						    PIM_MRE_WC,
						    0);
	if (pim_mre_wc == wc_entry())
	    return;		// Nothing changed
	set_wc_entry(pim_mre_wc);
	// Add to the PimNbr and PimRp lists.
	add_pim_mre_lists();
    }
    if (is_wc())
	set_is_task_delete_done(true);
}

// XXX: applies for (S,G), (S,G,rpt)
void
PimMre::remove_pim_mre_sg_entry()
{
    if (is_sg_rpt()) {
	PimMre *pim_mre_sg = pim_mrt().pim_mre_find(source_addr(),
						    group_addr(),
						    PIM_MRE_SG,
						    0);
	if (pim_mre_sg == sg_entry())
	    return;		// Nothing changed
	set_sg(pim_mre_sg);
    }
    if (is_sg())
	set_is_task_delete_done(true);
}

// XXX: applies for (S,G), (S,G,rpt)
void
PimMre::remove_pim_mre_sg_rpt_entry()
{
    if (is_sg()) {
	PimMre *pim_mre_sg_rpt = pim_mrt().pim_mre_find(source_addr(),
							group_addr(),
							PIM_MRE_SG_RPT,
							0);
	if (pim_mre_sg_rpt == sg_rpt_entry())
	    return;		// Nothing changed
	set_sg_rpt(pim_mre_sg_rpt);
    }
    if (is_sg_rpt())
	set_is_task_delete_done(true);
}
