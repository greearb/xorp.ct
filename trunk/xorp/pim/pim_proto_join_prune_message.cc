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

#ident "$XORP: xorp/pim/pim_proto_join_prune_message.cc,v 1.6 2003/01/12 04:56:30 pavlin Exp $"


//
// PIM PIM_JOIN_PRUNE control messages supporting functions.
//

#include <map>

#include "pim_module.h"
#include "pim_private.hh"
#include "libxorp/utils.hh"
#include "pim_mre.hh"
#include "pim_node.hh"
#include "pim_proto.h"
#include "pim_proto_join_prune_message.hh"
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


PimJpHeader::PimJpHeader(PimNode& pim_node)
    : _pim_node(pim_node),
      _family(pim_node.family()),
      _jp_groups_n(0),
      _jp_sources_n(0),
      _holdtime(PIM_JOIN_PRUNE_HOLDTIME_DEFAULT) // XXX
{
    
}

PimJpHeader::~PimJpHeader()
{
    // Delete all 'jp_group' entries
    delete_pointers_list(_jp_groups_list);
}

void
PimJpHeader::reset()
{
    delete_pointers_list(_jp_groups_list);
    _jp_groups_n = 0;
    _jp_sources_n = 0;
    _holdtime = PIM_JOIN_PRUNE_HOLDTIME_DEFAULT; // XXX
}

PimMrt&
PimJpHeader::pim_mrt() const
{
    return (_pim_node.pim_mrt());
}

// Return true if @ipaddr found
bool
PimJpSources::j_list_found(const IPvX& ipaddr)
{
    list<IPvX>::iterator iter;
    for (iter = j_list().begin(); iter != j_list().end(); ++iter) {
	if (ipaddr == *iter)
	    return (true);
    }
    
    return (false);
}

// Return true if @ipaddr found
bool
PimJpSources::p_list_found(const IPvX& ipaddr)
{
    list<IPvX>::iterator iter;
    for (iter = p_list().begin(); iter != p_list().end(); ++iter) {
	if (ipaddr == *iter)
	    return (true);
    }
    
    return (false);
}

// Remove entry for @ipaddr, and return true if such entry was removed
bool
PimJpSources::j_list_remove(const IPvX& ipaddr)
{
    list<IPvX>::iterator iter;
    for (iter = j_list().begin(); iter != j_list().end(); ++iter) {
	if (ipaddr == *iter) {
	    j_list().erase(iter);
	    return (true);
	}
    }
    
    return (false);
}

// Remove entry for @ipaddr, and return true if such entry was removed
bool
PimJpSources::p_list_remove(const IPvX& ipaddr)
{
    list<IPvX>::iterator iter;
    for (iter = p_list().begin(); iter != p_list().end(); ++iter) {
	if (ipaddr == *iter) {
	    p_list().erase(iter);
	    return (true);
	}
    }
    
    return (false);
}

PimJpGroup::PimJpGroup(PimJpHeader& jp_header, int family)
    : _jp_header(jp_header),
      _family(family),
      _group_addr(family)
{
    _group_masklen = IPvX::addr_bitlen(family);
    _j_sources_n = 0;
    _p_sources_n = 0;
}

// Return: XORP_ERROR if the addition of this entry is inconsistent
// XXX: the (*,*,RP) entries are first in the chain.
// @new_group_bool: if true, create a new PimJpGroup().
int
PimJpHeader::jp_entry_add(const IPvX& source_addr, const IPvX& group_addr,
			  uint32_t group_masklen,
			  mrt_entry_type_t mrt_entry_type,
			  action_jp_t action_jp, uint16_t holdtime,
			  bool new_group_bool)
{
    bool jp_group_found_bool = false;
    PimJpGroup *jp_group = NULL;
    PimJpSources *jp_sources = NULL;
    
    if (! new_group_bool) {
	// Allow to merge togeter all entries for the same group.
	// Find the group entry
	list<PimJpGroup *>::iterator iter;
	for (iter = _jp_groups_list.begin(); iter != _jp_groups_list.end();
	     ++iter) {
	    jp_group = *iter;
	    if ( (group_addr != jp_group->group_addr())
		 || (group_masklen != jp_group->group_masklen()))
		continue;
	    jp_group_found_bool = true;
	    break;
	}
    }
    
    if ( ! jp_group_found_bool) {
	// Create a new entry
	jp_group = new PimJpGroup(*this, family());
#if 0	// TODO: if we want the (*,*,RP) messages to be in front the
	// outgoing J/P messages, this is the place to take care of it.
	// XXX: disable it for now
	// XXX: the (*,*,RP) entries are first in the chain
	if (mrt_entry_type == MRT_ENTRY_RP)
	    _jp_groups_list.push_front(jp_group);
	else
#endif // 0
	    _jp_groups_list.push_back(jp_group);
	jp_group->set_group_addr(group_addr);
	jp_group->set_group_masklen(group_masklen);
	incr_jp_groups_n();
    }
    _holdtime = holdtime;	// XXX: older holdtime may be modified
    XLOG_ASSERT(jp_group != NULL);
    
    // Perform sanity check for conflicting entries,
    // as well as find the type of entry.
    // XXX: the '?' entries in the J/P rules table are accepted
    // (see the J/P messages format section in the spec).
    switch(mrt_entry_type) {
    case MRT_ENTRY_RP:
	if (action_jp == ACTION_JOIN) {
	    // (*,*,RP) Join
	    if (jp_group->rp()->j_list_found(source_addr))
		return (XORP_OK);		// Already added; ignore.
	} else {
	    // (*,*,RP) Prune
	    if (jp_group->rp()->p_list_found(source_addr))
		return (XORP_OK);		// Already added; ignore.
	}
	jp_sources = jp_group->rp();
	break;
	
    case MRT_ENTRY_WC:
	if (action_jp == ACTION_JOIN) {
	    // (*,G) Join
	    if (jp_group->wc()->j_list_found(source_addr))
		return (XORP_OK);		// Already added; ignore.
	    if (jp_group->wc()->p_list_found(source_addr))
		return (XORP_ERROR);	// Combination not allowed
	    // Remove redundant entries: (S,G,rpt)J
	    jp_group->sg_rpt()->j_list_remove(source_addr);
	} else {
	    // (*,G) Prune
	    if (jp_group->wc()->j_list_found(source_addr))
		return (XORP_ERROR);	// Combination not allowed
	    if (jp_group->wc()->p_list_found(source_addr))
		return (XORP_OK);		// Already added; ignore.
	    // Remove redundant entries: (S,G,rpt)J and (S,G,rpt)P
	    jp_group->sg_rpt()->j_list_remove(source_addr);
	    jp_group->sg_rpt()->p_list_remove(source_addr);
	}
	jp_sources = jp_group->wc();
	break;
	
    case MRT_ENTRY_SG_RPT:
	if (action_jp == ACTION_JOIN) {
	    if (jp_group->wc()->j_list_found(source_addr))
		return (XORP_OK);		// Redundant; ignore.
	    if (jp_group->wc()->p_list_found(source_addr))
		return (XORP_OK);		// Redundant; ignore.
	    // (S,G,rpt) Join
	    if (jp_group->sg_rpt()->j_list_found(source_addr))
		return (XORP_OK);		// Already added; ignore.
	    if (jp_group->sg_rpt()->p_list_found(source_addr))
		return (XORP_ERROR);	// Combination not allowed
	} else {
	    // (S,G,rpt) Prune
	    if (jp_group->wc()->p_list_found(source_addr))
		return (XORP_OK);		// Redundant; ignore.
	    if (jp_group->sg_rpt()->j_list_found(source_addr))
		return (XORP_ERROR);	// Combination not allowed
	    if (jp_group->sg_rpt()->p_list_found(source_addr))
		return (XORP_OK);		// Already added; ignore.
	    if (jp_group->sg()->j_list_found(source_addr))
		return (XORP_OK);		// Redundant; ignore.
	    if (jp_group->sg()->p_list_found(source_addr))
		return (XORP_OK);		// Redundant; ignore. (TODO: why?)
	}
	jp_sources = jp_group->sg_rpt();
	break;
	
    case MRT_ENTRY_SG:
	if (action_jp == ACTION_JOIN) {
	    // (S,G) Join
	    if (jp_group->sg()->j_list_found(source_addr))
		return (XORP_OK);		// Already added; ignore.
	    if (jp_group->sg()->p_list_found(source_addr))
		return (XORP_ERROR);	// Combination not allowed
	    // Remove redundant entries: (S,G,rpt)P
	    jp_group->sg_rpt()->p_list_remove(source_addr);
	} else {
	    // (S,G) Prune
	    if (jp_group->sg()->j_list_found(source_addr))
		return (XORP_ERROR);	// Combination not allowed
	    if (jp_group->sg()->p_list_found(source_addr))
		return (XORP_OK);		// Already added; ignore.
	    // Remove redundant entries: (S,G,rpt)P  (TODO: why?)
	    jp_group->sg_rpt()->p_list_remove(source_addr);
	}
	jp_sources = jp_group->sg();
	break;
	
    default:
	XLOG_ASSERT(false);
	return (XORP_ERROR);
    }
    XLOG_ASSERT(jp_sources != NULL);
    
    // Add the new entry and record the fact.
    if (action_jp == ACTION_JOIN) {
	jp_sources->j_list().push_back(source_addr);
	jp_sources->incr_j_n();
	jp_group->incr_j_sources_n();
    } else {
	jp_sources->p_list().push_back(source_addr);
	jp_sources->incr_p_n();
	jp_group->incr_p_sources_n();
    }
    
    return (XORP_OK);
}

int
PimJpHeader::mrt_commit(PimVif *pim_vif, const IPvX& target_nbr_addr)
{
    bool	i_am_target_router_bool = true;
    uint32_t	lookup_flags = 0, create_flags = 0;
    uint16_t	vif_index;
    uint16_t	holdtime;
    uint32_t	source_masklen, group_masklen;
    IPvX	source_addr(family()), group_addr(family());
    list<PimJpGroup *>::iterator iter;
    PimMre	*pim_mre;
    map<IPvX, IPvX> groups_map, join_wc_map;
    
    vif_index = pim_vif->vif_index();
    
    //
    // Test if I am the target router; e.g., in case I need to
    // perform Join/Prune suppression.
    // XXX: on p2p interfaces, target_nbr_addr of all zeros is also accepted
    if (pim_vif->is_my_addr(target_nbr_addr)
	|| (pim_vif->is_p2p() && target_nbr_addr == IPvX::ZERO(family()))) {
	i_am_target_router_bool = true;
    } else {
	i_am_target_router_bool = false;
    }
    
    //
    // Create the map with all group addresses
    //
    if (i_am_target_router_bool) {
	for (iter = _jp_groups_list.begin(); iter != _jp_groups_list.end();
	     ++iter) {
	    PimJpGroup *jp_group = *iter;
	    group_addr = jp_group->group_addr();
	    group_masklen = jp_group->group_masklen();
	    if (group_masklen != group_addr.addr_bitlen())
		continue;	// XXX: exclude (e.g., probably (*,*,RP) entry)
	    groups_map.insert(pair<IPvX, IPvX>(group_addr, group_addr));
	}
    }
    
    //
    // The main loop
    //
    for (iter = _jp_groups_list.begin(); iter != _jp_groups_list.end();
	 ++iter) {
	list<IPvX>::iterator iter2;
	PimJpGroup *jp_group = *iter;
	group_addr = jp_group->group_addr();
	group_masklen = jp_group->group_masklen();
	holdtime = _holdtime;
	
	//
	// Build the map for all (*,G) Joins so far
	//
	if (i_am_target_router_bool) {
	    if (! jp_group->wc()->j_list().empty())
		join_wc_map.insert(pair<IPvX, IPvX>(group_addr, group_addr));
	}
	
	// (*,*,RP) Join
	for (iter2 = jp_group->rp()->j_list().begin();
	     iter2 != jp_group->rp()->j_list().end();
	     ++iter2) {
	    source_addr		= *iter2;
	    source_masklen	= IPvX::addr_bitlen(family());
	    
	    if (i_am_target_router_bool)
		pim_mrt().add_task_receive_join_rp(vif_index, source_addr);
	    
	    lookup_flags	= PIM_MRE_RP;
	    if (i_am_target_router_bool)
		create_flags = lookup_flags;
	    else
		create_flags = 0;
	    pim_mre = pim_mrt().pim_mre_find(source_addr, IPvX::ZERO(family()),
					     lookup_flags, create_flags);
	    if (pim_mre == NULL) {
		if (create_flags)
		    goto pim_mre_find_error;
	    } else {
		if (i_am_target_router_bool) {
		    pim_mre->receive_join_rp(vif_index, holdtime);
		} else {
		    pim_mre->rp_see_join_rp(vif_index, holdtime,
					    target_nbr_addr);
		}
	    }
	}
	// (*,*,RP) Prune
	for (iter2 = jp_group->rp()->p_list().begin();
	     iter2 != jp_group->rp()->p_list().end();
	     ++iter2) {
	    source_addr		= *iter2;
	    source_masklen	= IPvX::addr_bitlen(family());
	    
	    if (i_am_target_router_bool)
		pim_mrt().add_task_receive_prune_rp(vif_index, source_addr);
	    
	    lookup_flags	= PIM_MRE_RP;
	    // XXX: if no entry, the (*,*,RP) Prune should not create one
	    create_flags = 0;
	    
	    pim_mre = pim_mrt().pim_mre_find(source_addr, IPvX::ZERO(family()),
					     lookup_flags, create_flags);
	    if (pim_mre == NULL) {
		if (create_flags)
		    goto pim_mre_find_error;
	    } else {
		if (i_am_target_router_bool) {
		    pim_mre->receive_prune_rp(vif_index, holdtime);
		} else {
		    pim_mre->rp_see_prune_rp(vif_index, holdtime,
					     target_nbr_addr);
		}
	    }
	}
	// (*,G) Join
	for (iter2 = jp_group->wc()->j_list().begin();
	     iter2 != jp_group->wc()->j_list().end();
	     ++iter2) {
	    source_addr		= *iter2;
	    source_masklen	= IPvX::addr_bitlen(family());
	    
	    if (i_am_target_router_bool)
		pim_mrt().add_task_receive_join_wc(vif_index, group_addr);
	    
	    lookup_flags	= PIM_MRE_WC;
	    if (i_am_target_router_bool)
		create_flags = lookup_flags;
	    else
		create_flags = 0;
	    pim_mre = pim_mrt().pim_mre_find(source_addr, group_addr,
					     lookup_flags, create_flags);
	    if (pim_mre == NULL) {
		if (create_flags)
		    goto pim_mre_find_error;
	    } else {
		if (i_am_target_router_bool) {
		    pim_mre->receive_join_wc(vif_index, holdtime);
		} else {
		    pim_mre->wc_see_join_wc(vif_index, holdtime,
					    target_nbr_addr);
		}
	    }
	}
	// (*,G) Prune
	for (iter2 = jp_group->wc()->p_list().begin();
	     iter2 != jp_group->wc()->p_list().end();
	     ++iter2) {
	    source_addr		= *iter2;
	    source_masklen	= IPvX::addr_bitlen(family());
	    
	    if (i_am_target_router_bool) {
		pim_mrt().add_task_receive_prune_wc(vif_index, group_addr);
	    } else {
		pim_mrt().add_task_see_prune_wc(vif_index, group_addr,
						target_nbr_addr);
	    }
	    
	    lookup_flags	= PIM_MRE_WC;
	    // XXX: if no entry, the (*,G) Prune should not create one
	    create_flags = 0;
	    pim_mre = pim_mrt().pim_mre_find(source_addr, group_addr,
					     lookup_flags, create_flags);
	    if (pim_mre == NULL) {
		if (create_flags)
		    goto pim_mre_find_error;
	    } else {
		if (i_am_target_router_bool) {
		    pim_mre->receive_prune_wc(vif_index, holdtime);
		} else {
		    pim_mre->wc_see_prune_wc(vif_index, holdtime,
					     target_nbr_addr);
		}
	    }
	}
	// (S,G,rpt) Join
	for (iter2 = jp_group->sg_rpt()->j_list().begin();
	     iter2 != jp_group->sg_rpt()->j_list().end();
	     ++iter2) {
	    source_addr		= *iter2;
	    source_masklen	= IPvX::addr_bitlen(family());
	    
	    if (i_am_target_router_bool) {
		pim_mrt().add_task_receive_join_sg_rpt(vif_index, source_addr,
						       group_addr);
	    }
	    
	    lookup_flags	= PIM_MRE_SG_RPT;
	    // XXX: if no entry, the (S,G,rpt) Join should not create one
	    create_flags = 0;
	    pim_mre = pim_mrt().pim_mre_find(source_addr, group_addr,
					     lookup_flags, create_flags);
	    if (pim_mre == NULL) {
		if (create_flags)
		    goto pim_mre_find_error;
	    } else {
		if (i_am_target_router_bool) {
		    pim_mre->receive_join_sg_rpt(vif_index, holdtime);
		} else {
		    pim_mre->sg_rpt_see_join_sg_rpt(vif_index, holdtime,
						    target_nbr_addr);
		}
	    }
	}
	// (S,G,rpt) Prune
	for (iter2 = jp_group->sg_rpt()->p_list().begin();
	     iter2 != jp_group->sg_rpt()->p_list().end();
	     ++iter2) {
	    source_addr		= *iter2;
	    source_masklen	= IPvX::addr_bitlen(family());
	    
	    if (i_am_target_router_bool) {
		pim_mrt().add_task_receive_prune_sg_rpt(vif_index, source_addr,
							group_addr);
	    }
	    
	    lookup_flags	= PIM_MRE_SG_RPT;
	    // XXX: even if no entry, the (S,G,rpt) Prune should create one
	    // XXX: even if the (S,G,rpt) Prune is not for me, try to create
	    // an entry.
	    if (i_am_target_router_bool)
		create_flags = lookup_flags;
	    else
		create_flags = lookup_flags;
	    pim_mre = pim_mrt().pim_mre_find(source_addr, group_addr,
					     lookup_flags, create_flags);
	    if (pim_mre == NULL) {
		if (create_flags)
		    goto pim_mre_find_error;
	    } else {
		bool join_wc_received_bool;
		if (jp_group->wc()->j_list().empty())
		    join_wc_received_bool = false;
		else
		    join_wc_received_bool = true;
		if (! join_wc_received_bool) {
		    join_wc_received_bool
			= (join_wc_map.find(group_addr) != join_wc_map.end());
		}
		if (i_am_target_router_bool) {
		    pim_mre->receive_prune_sg_rpt(vif_index, holdtime,
						  join_wc_received_bool);
		} else {
		    pim_mre->sg_rpt_see_prune_sg_rpt(vif_index, holdtime,
						     target_nbr_addr);
		    pim_mre->entry_try_remove();
		}
	    }
	    
	    //
	    // Take care of the (S,G) entry that
	    // "See Prune (S,G,rpt) to RPF'(S,G)"
	    //
	    if (! i_am_target_router_bool) {
		PimMre *pim_mre_sg = NULL;
		if (pim_mre != NULL) {
		    pim_mre_sg = pim_mre->sg_entry();
		} else {
		    pim_mre_sg = pim_mrt().pim_mre_find(source_addr,
							group_addr,
							PIM_MRE_SG, 0);
		}
		if (pim_mre_sg != NULL)
		    pim_mre_sg->sg_see_prune_sg_rpt(vif_index, holdtime,
						    target_nbr_addr);
	    }
	}
	// (S,G) Join
	for (iter2 = jp_group->sg()->j_list().begin();
	     iter2 != jp_group->sg()->j_list().end();
	     ++iter2) {
	    source_addr		= *iter2;
	    source_masklen	= IPvX::addr_bitlen(family());
	    
	    if (i_am_target_router_bool) {
		pim_mrt().add_task_receive_join_sg(vif_index, source_addr,
						   group_addr);
	    }
	    
	    lookup_flags	= PIM_MRE_SG;
	    if (i_am_target_router_bool)
		create_flags = lookup_flags;
	    else
		create_flags = 0;
	    pim_mre = pim_mrt().pim_mre_find(source_addr, group_addr,
					     lookup_flags, create_flags);
	    if (pim_mre == NULL) {
		if (create_flags)
		    goto pim_mre_find_error;
	    } else {
		if (i_am_target_router_bool) {
		    pim_mre->receive_join_sg(vif_index, holdtime);
		} else {
		    pim_mre->sg_see_join_sg(vif_index, holdtime,
					    target_nbr_addr);
		}
	    }
	}
	// (S,G) Prune
	for (iter2 = jp_group->sg()->p_list().begin();
	     iter2 != jp_group->sg()->p_list().end();
	     ++iter2) {
	    source_addr		= *iter2;
	    source_masklen	= IPvX::addr_bitlen(family());
	    
	    if (i_am_target_router_bool) {
		pim_mrt().add_task_receive_prune_sg(vif_index, source_addr,
						    group_addr);
	    }
    	    
	    lookup_flags	= PIM_MRE_SG;
	    // XXX: if no entry, the (S,G) Prune should not create one
	    create_flags = 0;
	    pim_mre = pim_mrt().pim_mre_find(source_addr, group_addr,
					     lookup_flags, create_flags);
	    if (pim_mre == NULL) {
		if (create_flags)
		    goto pim_mre_find_error;
	    } else {
		if (i_am_target_router_bool) {
		    pim_mre->receive_prune_sg(vif_index, holdtime);
		} else {
		    pim_mre->sg_see_prune_sg(vif_index, holdtime,
					     target_nbr_addr);
		}
	    }
	    
	    //
	    // Take care of the (S,G,rpt) entry that
	    // "See Prune (S,G) to RPF'(S,G,rpt)"
	    //
	    if (! i_am_target_router_bool) {
		PimMre *pim_mre_sg_rpt = NULL;
		if (pim_mre != NULL)
		    pim_mre_sg_rpt = pim_mre->sg_rpt_entry();
		if (pim_mre_sg_rpt == NULL) {
		    // XXX: always create the (S,G,rpt) entry even
		    // if the (S,G) Prune is not for me
		    lookup_flags = PIM_MRE_SG_RPT;
		    create_flags = lookup_flags;
		    pim_mre_sg_rpt = pim_mrt().pim_mre_find(source_addr,
							    group_addr,
							    lookup_flags,
							    create_flags);
		    if (pim_mre_sg_rpt == NULL) {
			if (create_flags)
			    goto pim_mre_find_error;
		    }
		}
		if (pim_mre_sg_rpt != NULL) {
		    pim_mre_sg_rpt->sg_rpt_see_prune_sg(vif_index, holdtime,
							target_nbr_addr);
		    pim_mre_sg_rpt->entry_try_remove();
		}
	    }
	}
    }
    
    //
    // Take care of the (S,G,rpt) entries that see
    // "End of Message"
    //
    if (i_am_target_router_bool) {
	map<IPvX, IPvX>::iterator map_iter;
	for (map_iter = groups_map.begin(); map_iter != groups_map.end();
	     ++map_iter) {
	    group_addr = map_iter->second;
	    pim_mrt().add_task_receive_end_of_message_sg_rpt(vif_index,
							     group_addr);
	}
    }
    
    return (XORP_OK);
    
 pim_mre_find_error:
    XLOG_ASSERT(false);
    XLOG_ERROR("INTERNAL PimMrt ERROR: "
	       "cannot create entry for (%s,%s) create_flags = %#x",
	       cstring(source_addr), cstring(group_addr), create_flags);
    return (XORP_ERROR);
}

// XXX: @buffer must have been prepared in advance
int
PimJpHeader::network_commit(PimNbr *pim_nbr, buffer_t *buffer)
{
    uint32_t	source_masklen;
    uint32_t	flags;
    IPvX	source_addr(family());
    list<PimJpGroup *>::iterator iter;
    uint8_t sparse_bit = pim_node().proto_is_pimsm() ? ESADDR_S_BIT : 0;
    
    // Add first all (S,G,rpt) entries that need to be included
    // with the (*,G) Join messages
    // TODO: we must check message size, and eventually split the message
    // when necessary.
    for (iter = _jp_groups_list.begin(); iter != _jp_groups_list.end();
	 ++iter) {
	list<IPvX>::iterator iter2;
	PimJpGroup *jp_group = *iter;
	
	// (*,G) Join
	for (iter2 = jp_group->wc()->j_list().begin();
	     iter2 != jp_group->wc()->j_list().end();
	     ++iter2) {
	    PimMre *pim_mre_wc = pim_mrt().pim_mre_find(IPvX::ZERO(family()),
							jp_group->group_addr(),
							PIM_MRE_WC, 0);
	    if (pim_mre_wc == NULL)
		continue;
	    //
	    // Add all necessary (S,G,rpt) entries for this group
	    // First we go through all (S,G) entries, and then through
	    // the (S,G,rpt) entries.
	    //
	    
	    //
	    // Go through the (S,G) entries and add (S,G,rpt) Prune
	    // if needed
	    //
	    PimMrtSg::const_gs_iterator iter3_begin, iter3_end, iter3;
	    iter3_begin = pim_mrt().pim_mrt_sg().group_by_addr_begin(pim_mre_wc->group_addr());
	    iter3_end = pim_mrt().pim_mrt_sg().group_by_addr_end(pim_mre_wc->group_addr());
	    for (iter3 = iter3_begin; iter3 != iter3_end; ++iter3) {
		PimMre *pim_mre_sg = iter3->second;
		
		if (pim_mre_sg->is_spt()) {
		    // Note: If receiving (S,G) on the SPT, we only prune off
		    // the shared tree if the rpf neighbors differ, i.e.
		    // if ( RPF'(*,G) != RPF'(S,G) )
		    if (pim_mre_wc->rpfp_nbr_wc() != pim_mre_sg->rpfp_nbr_sg())
			goto add_prune_sg_rpt_label1;
		}
		continue;
		
	    add_prune_sg_rpt_label1:
		bool new_group_bool = false;
		jp_entry_add(pim_mre_sg->source_addr(),
			     pim_mre_sg->group_addr(),
			     IPvX::addr_bitlen(family()),
			     MRT_ENTRY_SG_RPT,
			     ACTION_PRUNE, _holdtime, new_group_bool);
	    }
	    //
	    // Go through the (S,G,rpt) entries and add (S,G,rpt) Prune
	    // if needed.
	    //
	    iter3_begin = pim_mrt().pim_mrt_sg_rpt().group_by_addr_begin(pim_mre_wc->group_addr());
	    iter3_end = pim_mrt().pim_mrt_sg_rpt().group_by_addr_end(pim_mre_wc->group_addr());
	    for (iter3 = iter3_begin; iter3 != iter3_end; ++iter3) {
		PimMre *pim_mre_sg_rpt = iter3->second;
		if (pim_mre_sg_rpt->inherited_olist_sg_rpt().none()) {
		    // Note: all (*,G) olist interfaces sent rpt prunes
		    // for (S,G).
		    goto add_prune_sg_rpt_label2;
		} else if (pim_mre_wc->rpfp_nbr_wc()
			   != pim_mre_sg_rpt->rpfp_nbr_sg_rpt()) {
		    // Note: we joined the shared tree, but there was
		    // an (S,G) assert and the source tree RPF neighbor
		    // is different.
		    goto add_prune_sg_rpt_label2;
		}
		continue;
		
	    add_prune_sg_rpt_label2:
		do {
		    //
		    // XXX: check if we added already the entry because
		    // of an (S,G) entry.
		    //
		    PimMre *pim_mre_sg = pim_mre_sg_rpt->sg_rpt_entry();
		    if ((pim_mre_sg != NULL) && (pim_mre_sg->is_spt())) {
			// Note: If receiving (S,G) on the SPT, we only prune
			// off the shared tree if the rpf neighbors differ,
			// i.e. if ( RPF'(*,G) != RPF'(S,G) )
			if (pim_mre_wc->rpfp_nbr_wc()
			    != pim_mre_sg->rpfp_nbr_sg())
			    continue;		// XXX: already added
		    }
		} while (false);
		
		bool new_group_bool = false;
		jp_entry_add(pim_mre_sg_rpt->source_addr(),
			     pim_mre_sg_rpt->group_addr(),
			     IPvX::addr_bitlen(family()),
			     MRT_ENTRY_SG_RPT,
			     ACTION_PRUNE, _holdtime, new_group_bool);
	    }
	}
    }
    
    // Create the message
    PUT_ENCODED_UNICAST_ADDR(family(), pim_nbr->addr(), buffer);
    BUFFER_PUT_OCTET(0, buffer);			// Reserved
    BUFFER_PUT_OCTET(_jp_groups_n, buffer);		// Number of groups
    BUFFER_PUT_HOST_16(_holdtime, buffer);		// Holdtime
    
    // TODO: we must check message size, and eventually split the message
    // when necessary.
    for (iter = _jp_groups_list.begin(); iter != _jp_groups_list.end();
	 ++iter) {
	list<IPvX>::iterator iter2;
	PimJpGroup *jp_group = *iter;
	uint8_t group_addr_reserved_flags = 0;
	
	PUT_ENCODED_GROUP_ADDR(family(), jp_group->group_addr(),
			       jp_group->group_masklen(),
			       group_addr_reserved_flags, buffer);
	// The number of joined sources
	BUFFER_PUT_HOST_16(jp_group->j_sources_n(), buffer);
	// The number of pruned sources
	BUFFER_PUT_HOST_16(jp_group->p_sources_n(), buffer);
	
	// (*,*,RP) Join
	for (iter2 = jp_group->rp()->j_list().begin();
	     iter2 != jp_group->rp()->j_list().end();
	     ++iter2) {
	    source_addr		= *iter2;
	    source_masklen	= IPvX::addr_bitlen(family());
	    flags		= ESADDR_RPT_BIT | ESADDR_WC_BIT | sparse_bit;
	    PUT_ENCODED_SOURCE_ADDR(family(), source_addr, source_masklen,
				    flags, buffer);
	}
	// (*,*,RP) Prune
	for (iter2 = jp_group->rp()->p_list().begin();
	     iter2 != jp_group->rp()->p_list().end();
	     ++iter2) {
	    source_addr		= *iter2;
	    source_masklen	= IPvX::addr_bitlen(family());
	    flags		= ESADDR_RPT_BIT | ESADDR_WC_BIT | sparse_bit;
	    PUT_ENCODED_SOURCE_ADDR(family(), source_addr, source_masklen,
				    flags, buffer);
	}
	// (*,G) Join
	for (iter2 = jp_group->wc()->j_list().begin();
	     iter2 != jp_group->wc()->j_list().end();
	     ++iter2) {
	    source_addr		= *iter2;
	    source_masklen	= IPvX::addr_bitlen(family());
	    flags		= ESADDR_RPT_BIT | ESADDR_WC_BIT | sparse_bit;
	    PUT_ENCODED_SOURCE_ADDR(family(), source_addr, source_masklen,
				    flags, buffer);
	}
	// (S,G,rpt) Join
	for (iter2 = jp_group->sg_rpt()->j_list().begin();
	     iter2 != jp_group->sg_rpt()->j_list().end();
	     ++iter2) {
	    source_addr		= *iter2;
	    source_masklen	= IPvX::addr_bitlen(family());
	    flags		= ESADDR_RPT_BIT | sparse_bit;
	    PUT_ENCODED_SOURCE_ADDR(family(), source_addr, source_masklen,
				    flags, buffer);
	}
	// (S,G) Join
	for (iter2 = jp_group->sg()->j_list().begin();
	     iter2 != jp_group->sg()->j_list().end();
	     ++iter2) {
	    source_addr		= *iter2;
	    source_masklen	= IPvX::addr_bitlen(family());
	    flags		= sparse_bit;
	    PUT_ENCODED_SOURCE_ADDR(family(), source_addr, source_masklen,
				    flags, buffer);
	}
	// (*,G) Prune
	for (iter2 = jp_group->wc()->p_list().begin();
	     iter2 != jp_group->wc()->p_list().end();
	     ++iter2) {
	    source_addr		= *iter2;
	    source_masklen	= IPvX::addr_bitlen(family());
	    flags		= ESADDR_RPT_BIT | ESADDR_WC_BIT | sparse_bit;
	    PUT_ENCODED_SOURCE_ADDR(family(), source_addr, source_masklen,
				    flags, buffer);
	}
	// (S,G,rpt) Prune
	for (iter2 = jp_group->sg_rpt()->p_list().begin();
	     iter2 != jp_group->sg_rpt()->p_list().end();
	     ++iter2) {
	    source_addr		= *iter2;
	    source_masklen	= IPvX::addr_bitlen(family());
	    flags		= ESADDR_RPT_BIT | sparse_bit;
	    PUT_ENCODED_SOURCE_ADDR(family(), source_addr, source_masklen,
				    flags, buffer);
	}
	// (S,G) Prune
	for (iter2 = jp_group->sg()->p_list().begin();
	     iter2 != jp_group->sg()->p_list().end();
	     ++iter2) {
	    source_addr		= *iter2;
	    source_masklen	= IPvX::addr_bitlen(family());
	    flags		= sparse_bit;
	    PUT_ENCODED_SOURCE_ADDR(family(), source_addr, source_masklen,
				    flags, buffer);
	}
    }
    
    return (XORP_OK);
    
 invalid_addr_family_error:
    XLOG_ASSERT(false);
    XLOG_ERROR("INTERNAL %s ERROR: "
	       "invalid address family error = %d",
	       PIMTYPE2ASCII(PIM_JOIN_PRUNE),
	       family());
    return (XORP_ERROR);
    
 buflen_error:
    XLOG_ERROR("INTERNAL %s ERROR: packet cannot fit into sending buffer",
	       PIMTYPE2ASCII(PIM_JOIN_PRUNE));
    
    return (XORP_ERROR);
}
