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

#ident "$XORP: xorp/pim/pim_nbr.cc,v 1.10 2003/09/30 18:27:05 pavlin Exp $"

//
// PIM neigbor routers handling
//


#include "pim_module.h"
#include "pim_private.hh"
#include "pim_nbr.hh"
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


/**
 * PimNbr::PimNbr:
 * @pim_vif: The protocol vif towards the neighbor.
 * @primary_addr: The primary address of the neighbor.
 * 
 * PIM neighbor constructor.
 **/
PimNbr::PimNbr(PimVif& pim_vif, const IPvX& primary_addr, int proto_version)
    : _pim_node(pim_vif.pim_node()),
      _pim_vif(pim_vif),
      _primary_addr(primary_addr),
      _proto_version(proto_version),
      _jp_header(pim_vif.pim_node()),
      _startup_time(TimeVal::MAXIMUM())
{
    reset_received_options();
}

/**
 * PimNbr::~PimNbr:
 * @: 
 * 
 * PIM neighbor destructor.
 **/
PimNbr::~PimNbr()
{
    // TODO: do we need to do anything special about _jp_list ??
}

/**
 * PimNbr::reset_received_options:
 * @: 
 * 
 * Reset received options from this PIM neighbor.
 **/
void
PimNbr::reset_received_options()
{
    _proto_version = _pim_vif.proto_version();
    // TODO: 0xffffffffU for _genid should be #define
    _genid = 0xffffffffU;
    _is_genid_present = false;
    set_dr_priority(PIM_HELLO_DR_PRIORITY_DEFAULT);
    set_is_dr_priority_present(false);
    _hello_holdtime = PIM_HELLO_HELLO_HOLDTIME_DEFAULT;
    _neighbor_liveness_timer.unschedule();
    _is_lan_prune_delay_present = false;
    _is_tracking_support_disabled = false;
    _lan_delay = 0;
    _override_interval = 0;
    _is_nohello_neighbor = false;
    _secondary_addr_list.clear();
}

/**
 * PimNbr::vif_index:
 * @: 
 * 
 * Return the corresponding Vif index toward this neighbor.
 * 
 * Return value: The Vif index toward this neighbor.
 **/
uint16_t
PimNbr::vif_index() const
{
    return (pim_vif().vif_index());
}

void
PimNbr::add_secondary_addr(const IPvX& v)
{
    if (find(_secondary_addr_list.begin(), _secondary_addr_list.end(), v)
	!= _secondary_addr_list.end()) {
	return;		// The address is already added
    }

    _secondary_addr_list.push_back(v);
}

void
PimNbr::delete_secondary_addr(const IPvX& v)
{
    list<IPvX>::iterator iter;

    iter = find(_secondary_addr_list.begin(), _secondary_addr_list.end(), v);
    if (iter != _secondary_addr_list.end())
	_secondary_addr_list.erase(iter);
}

bool
PimNbr::has_secondary_addr(const IPvX& secondary_addr) const
{
    return (find(_secondary_addr_list.begin(), _secondary_addr_list.end(),
		 secondary_addr)
	    != _secondary_addr_list.end());
}

bool
PimNbr::is_my_addr(const IPvX& addr) const
{
    if (addr == _primary_addr)
	return true;

    return (has_secondary_addr(addr));
}

int
PimNbr::jp_entry_add(const IPvX& source_addr, const IPvX& group_addr,
		     uint8_t group_mask_len,
		     mrt_entry_type_t mrt_entry_type,
		     action_jp_t action_jp, uint16_t holdtime,
		     bool new_group_bool)
{
    int ret_value;
    
    ret_value = _jp_header.jp_entry_add(source_addr, group_addr,
					group_mask_len, mrt_entry_type,
					action_jp, holdtime, new_group_bool);
    
    // (Re)start the timer to send the J/P message after time 0.
    // XXX: the automatic restarting will postpone the sending of
    // the message until we have no more entries to add to that message.
    _jp_send_timer = pim_node().eventloop().new_oneoff_after(
	TimeVal(0, 0),
	callback(this, &PimNbr::jp_send_timer_timeout));
    
    return (ret_value);
}

void
PimNbr::jp_send_timer_timeout()
{
    pim_vif().pim_join_prune_send(this, &_jp_header);
}

/**
 * PimNbr::neighbor_liveness_timer_timeout:
 * 
 * Timeout: expire a PIM neighbor.
 **/
void
PimNbr::neighbor_liveness_timer_timeout()
{
    pim_vif().delete_pim_nbr_from_nbr_list(this);
    
    if (pim_vif().dr_addr() == primary_addr()) {
	// The neighbor to expire is the DR. Select a new DR.
	pim_vif().pim_dr_elect();
    }
    
    if (pim_vif().pim_nbrs_number() <= 1) {
	// XXX: the last neighbor on this vif: take any actions if necessary
    }
    
    pim_vif().delete_pim_nbr(this);
}

void
PimNbr::init_processing_pim_mre_rp()
{
    _processing_pim_mre_rp_list.splice(
	_processing_pim_mre_rp_list.end(),
	_pim_mre_rp_list);
}

void
PimNbr::init_processing_pim_mre_wc()
{
    _processing_pim_mre_wc_list.splice(
	_processing_pim_mre_wc_list.end(),
	_pim_mre_wc_list);
}

void
PimNbr::init_processing_pim_mre_sg()
{
    _processing_pim_mre_sg_list.splice(
	_processing_pim_mre_sg_list.end(),
	_pim_mre_sg_list);
}

void
PimNbr::init_processing_pim_mre_sg_rpt()
{
    _processing_pim_mre_sg_rpt_list.splice(
	_processing_pim_mre_sg_rpt_list.end(),
	_pim_mre_sg_rpt_list);
}

//
// XXX: A PimMre entry may reference to the same PimNbr more than once,
// therefore we may call this method more than once for the same PimMre.
// Hence, to avoid this, the method that calls add_pim_mre() must make
// sure that it wasn't called before:
// E.g., we can use PimMre::is_pim_nbr_in_use() to check that.
// The alternative is to use find() for each of the list before appending
// the entry there, but this adds some unpleasant overhead.
// Nevertheless, things can easily become intractable, hence we
// use find() to make sure everything is consistent.
// Note that (S,G) and (S,G,rpt) entries are always added, regardless whether
// they have (*,G) entry. This is needed to take care of source-related RPF
// neighbors.
void
PimNbr::add_pim_mre(PimMre *pim_mre)
{
    // TODO: completely remove?
    // if (pim_mre->is_sg() || pim_mre->is_sg_rpt()) {
    //	if ((pim_mre->wc_entry() != NULL) && (! ignore_wc_entry))
    //	    return;	// XXX: we don't add (S,G) or (S,G,rpt) that have (*,G)
    // }
    
    if (pim_mre->is_rp()) {
	if (find(_pim_mre_rp_list.begin(),
		 _pim_mre_rp_list.end(),
		 pim_mre)
	    != _pim_mre_rp_list.end()) {
	    return;	// Entry is already on the list
	}
	_pim_mre_rp_list.push_back(pim_mre);
	return;
    }
    if (pim_mre->is_wc()) {
	if (find(_pim_mre_wc_list.begin(),
		 _pim_mre_wc_list.end(),
		 pim_mre)
	    != _pim_mre_wc_list.end()) {
	    return;	// Entry is already on the list
	}
	_pim_mre_wc_list.push_back(pim_mre);
	return;
    }
    if (pim_mre->is_sg()) {
	if (find(_pim_mre_sg_list.begin(),
		 _pim_mre_sg_list.end(),
		 pim_mre)
	    != _pim_mre_sg_list.end()) {
	    return;	// Entry is already on the list
	}
	_pim_mre_sg_list.push_back(pim_mre);
	return;
    }
    if (pim_mre->is_sg_rpt()) {
	if (find(_pim_mre_sg_rpt_list.begin(),
		 _pim_mre_sg_rpt_list.end(),
		 pim_mre)
	    != _pim_mre_sg_rpt_list.end()) {
	    return;	// Entry is already on the list
	}
	_pim_mre_sg_rpt_list.push_back(pim_mre);
	return;
    }
}

void
PimNbr::delete_pim_mre(PimMre *pim_mre)
{
    list<PimMre *>::iterator pim_mre_iter;
    
    do {
	if (pim_mre->is_rp()) {
	    //
	    // (*,*,RP) entry
	    //
	    
	    //
	    // Try the pim_mre_rp_list
	    //
	    pim_mre_iter = find(_pim_mre_rp_list.begin(),
				_pim_mre_rp_list.end(),
				pim_mre);
	    if (pim_mre_iter != _pim_mre_rp_list.end()) {
		_pim_mre_rp_list.erase(pim_mre_iter);
		break;
	    }
	    //
	    // Try the processing_pim_mre_rp_list
	    //
	    pim_mre_iter = find(_processing_pim_mre_rp_list.begin(),
				_processing_pim_mre_rp_list.end(),
				pim_mre);
	    if (pim_mre_iter != _processing_pim_mre_rp_list.end()) {
		_processing_pim_mre_rp_list.erase(pim_mre_iter);
		break;
	    }
	}
	if (pim_mre->is_wc()) {
	    //
	    // (*,G) entry
	    //
	    
	    //
	    // Try the pim_mre_wc_list
	    //
	    pim_mre_iter = find(_pim_mre_wc_list.begin(),
				_pim_mre_wc_list.end(),
				pim_mre);
	    if (pim_mre_iter != _pim_mre_wc_list.end()) {
		_pim_mre_wc_list.erase(pim_mre_iter);
		break;
	    }
	    //
	    // Try the processing_pim_mre_wc_list
	    //
	    pim_mre_iter = find(_processing_pim_mre_wc_list.begin(),
				_processing_pim_mre_wc_list.end(),
				pim_mre);
	    if (pim_mre_iter != _processing_pim_mre_wc_list.end()) {
		_processing_pim_mre_wc_list.erase(pim_mre_iter);
		break;
	    }
	}
	if (pim_mre->is_sg()) {
	    //
	    // (S,G) entry
	    //
	    
	    //
	    // Try the pim_mre_sg_list
	    //
	    pim_mre_iter = find(_pim_mre_sg_list.begin(),
				_pim_mre_sg_list.end(),
				pim_mre);
	    if (pim_mre_iter != _pim_mre_sg_list.end()) {
		_pim_mre_sg_list.erase(pim_mre_iter);
		break;
	    }
	    //
	    // Try the processing_pim_mre_sg_list
	    //
	    pim_mre_iter = find(_processing_pim_mre_sg_list.begin(),
				_processing_pim_mre_sg_list.end(),
				pim_mre);
	    if (pim_mre_iter != _processing_pim_mre_sg_list.end()) {
		_processing_pim_mre_sg_list.erase(pim_mre_iter);
		break;
	    }
	}
	if (pim_mre->is_sg_rpt()) {
	    //
	    // (S,G,rpt) entry
	    //
	    
	    //
	    // Try the pim_mre_sg_rpt_list
	    //
	    pim_mre_iter = find(_pim_mre_sg_rpt_list.begin(),
				_pim_mre_sg_rpt_list.end(),
				pim_mre);
	    if (pim_mre_iter != _pim_mre_sg_rpt_list.end()) {
		_pim_mre_sg_rpt_list.erase(pim_mre_iter);
		break;
	    }
	    //
	    // Try the processing_pim_mre_sg_rpt_list
	    //
	    pim_mre_iter = find(_processing_pim_mre_sg_rpt_list.begin(),
				_processing_pim_mre_sg_rpt_list.end(),
				pim_mre);
	    if (pim_mre_iter != _processing_pim_mre_sg_rpt_list.end()) {
		_processing_pim_mre_sg_rpt_list.erase(pim_mre_iter);
		break;
	    }
	}
    } while (false);
    
    // If the PimNbr was pending deletion and if this was the
    // lastest PimMre entry, delete the PimNbr entry.
    do {
	if ( ! (_pim_mre_rp_list.empty()
		&& _processing_pim_mre_rp_list.empty()
		&& _pim_mre_wc_list.empty()
		&& _processing_pim_mre_wc_list.empty()
		&& _pim_mre_sg_list.empty()
		&& _processing_pim_mre_sg_list.empty()
		&& _pim_mre_sg_rpt_list.empty()
		&& _processing_pim_mre_sg_rpt_list.empty())) {
	    break;	// There are still more entries
	}
	
	list<PimNbr *>::iterator pim_nbr_iter;
	pim_nbr_iter = find(pim_node().processing_pim_nbr_list().begin(),
			    pim_node().processing_pim_nbr_list().end(),
			    this);
	if (pim_nbr_iter != pim_node().processing_pim_nbr_list().end()) {
	    pim_node().processing_pim_nbr_list().erase(pim_nbr_iter);
	    delete this;
	    return;
	}
    } while (false);
}
