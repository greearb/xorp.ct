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

#ident "$XORP: xorp/pim/pim_proto_cand_rp_adv.cc,v 1.11 2003/08/12 15:11:37 pavlin Exp $"


//
// PIM PIM_CAND_RP_ADV messages processing.
//


#include "pim_module.h"
#include "pim_private.hh"
#include "pim_bsr.hh"
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
 * PimVif::pim_cand_rp_adv_recv:
 * @pim_nbr: The PIM neighbor message originator (or NULL if not a neighbor).
 * @src: The message source address.
 * @dst: The message destination address.
 * @buffer: The buffer with the message.
 * 
 * Receive PIM_CAND_RP_ADV message.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimVif::pim_cand_rp_adv_recv(PimNbr *pim_nbr,
			     const IPvX& src,
			     const IPvX& dst,
			     buffer_t *buffer)
{
    uint8_t	prefix_count, rp_priority;
    uint16_t	rp_holdtime;
    IPvX	rp_addr(family());
    int		rcvd_family;
    IPvX	group_addr(family());
    uint8_t	group_mask_len;
    IPvXNet	group_prefix(family());
    uint8_t	group_addr_reserved_flags;
    PimBsr&	pim_bsr = pim_node().pim_bsr();
    bool	is_scope_zone = false;
    BsrZone	*active_bsr_zone = NULL;
    BsrRp	*bsr_rp;
    string	error_msg = "";
    bool	bool_add_rps_to_rp_table = false;
    
    //
    // Parse the message
    //
    BUFFER_GET_OCTET(prefix_count, buffer);
    BUFFER_GET_OCTET(rp_priority, buffer);
    BUFFER_GET_HOST_16(rp_holdtime, buffer);
    GET_ENCODED_UNICAST_ADDR(rcvd_family, rp_addr, buffer);
    
    if (! rp_addr.is_unicast()) {
	XLOG_WARNING("RX %s from %s to %s: "
		     "invalid RP address: %s ",
		     PIMTYPE2ASCII(PIM_CAND_RP_ADV),
		     cstring(src), cstring(dst),
		     cstring(rp_addr));
	return (XORP_ERROR);
    }
    
    if (prefix_count == 0) {
	// Prefix count of 0 implies all multicast groups.
	is_scope_zone = false;
	group_prefix = IPvXNet::ip_multicast_base_prefix(family());
	// Try to find a non-scoped zone for this prefix
	active_bsr_zone = pim_bsr.find_active_bsr_zone_by_prefix(group_prefix,
								 false);
	if (active_bsr_zone == NULL) {
	    // XXX: don't know anything about this zone yet
	    ++_pimstat_rx_candidate_rp_not_bsr;
	    return (XORP_ERROR);
	}
	if (active_bsr_zone->bsr_zone_state() != BsrZone::STATE_ELECTED_BSR) {
	    // Silently drop the message
	    ++_pimstat_rx_candidate_rp_not_bsr;
	    return (XORP_ERROR);
	}
	
	//
	// Check if the Cand-RP info has changed
	//
	bool is_rp_info_changed = false;
	do {
	    BsrRp *active_bsr_rp
		= active_bsr_zone->find_rp(group_prefix, rp_addr);
	    if (active_bsr_rp == NULL) {
		is_rp_info_changed = true;
		break;
	    }
	    if (rp_priority != active_bsr_rp->rp_priority()) {
		is_rp_info_changed = true;
		break;
	    }
	    if (rp_holdtime != active_bsr_rp->rp_holdtime()) {
		is_rp_info_changed = true;
		break;
	    }
	} while (false);
	
	bsr_rp = active_bsr_zone->add_rp(group_prefix,
					 is_scope_zone,
					 rp_addr,
					 rp_priority,
					 rp_holdtime,
					 error_msg);
	if (bsr_rp == NULL) {
	    XLOG_WARNING("RX %s from %s to %s: "
			 "Cannot add RP %s for group prefix %s "
			 "to BSR zone %s: %s",
			 PIMTYPE2ASCII(PIM_CAND_RP_ADV),
			 cstring(src), cstring(dst),
			 cstring(rp_addr),
			 cstring(group_prefix),
			 cstring(active_bsr_zone->zone_id()),
			 error_msg.c_str());
	    return (XORP_ERROR);
	}
	
	bsr_rp->start_candidate_rp_expiry_timer();
	
	// XXX: schedule to send immediately a Bootstrap message if needed
	// and add to the RP table.
	if (is_rp_info_changed) {
	    active_bsr_zone->expire_bsr_timer();
	    pim_bsr.add_rps_to_rp_table();
	}
	
	return (XORP_OK);
    }
    
    while (prefix_count--) {
	GET_ENCODED_GROUP_ADDR(rcvd_family, group_addr, group_mask_len,
			       group_addr_reserved_flags, buffer);
	if (group_addr_reserved_flags & EGADDR_Z_BIT)
	    is_scope_zone = true;
	else
	    is_scope_zone = false;
	group_prefix = IPvXNet(group_addr, group_mask_len);
	if (! group_prefix.masked_addr().is_multicast()) {
	    XLOG_WARNING("RX %s from %s to %s: "
			 "invalid group address: %s ",
			 PIMTYPE2ASCII(PIM_CAND_RP_ADV),
			 cstring(src), cstring(dst),
			 cstring(group_prefix));
	    continue;
	}
	//
	// Try to find a scoped zone for this group prefix.
	// Only if no scoped zone, then try non-scoped zone.
	active_bsr_zone = pim_bsr.find_active_bsr_zone_by_prefix(group_prefix,
								 true);
	if (active_bsr_zone == NULL) {
	    active_bsr_zone = pim_bsr.find_active_bsr_zone_by_prefix(group_prefix,
								     false);
	}
	if (active_bsr_zone == NULL)
	    continue;		// XXX: don't know anything about this zone yet
	if (active_bsr_zone->bsr_zone_state() != BsrZone::STATE_ELECTED_BSR) {
	    // Silently ignore the prefix
	    continue;
	}
	
	//
	// Check if the Cand-RP info has changed
	//
	bool is_rp_info_changed = false;
	do {
	    BsrRp *active_bsr_rp
		= active_bsr_zone->find_rp(group_prefix, rp_addr);
	    if (active_bsr_rp == NULL) {
		is_rp_info_changed = true;
		break;
	    }
	    if (rp_priority != active_bsr_rp->rp_priority()) {
		is_rp_info_changed = true;
		break;
	    }
	    if (rp_holdtime != active_bsr_rp->rp_holdtime()) {
		is_rp_info_changed = true;
		break;
	    }
	} while (false);
	
	bsr_rp = active_bsr_zone->add_rp(group_prefix,
					 is_scope_zone,
					 rp_addr,
					 rp_priority,
					 rp_holdtime,
					 error_msg);
	if (bsr_rp == NULL) {
	    XLOG_WARNING("RX %s from %s to %s: "
			 "Cannot add RP %s for group prefix %s "
			 "to BSR zone %s: %s",
			 PIMTYPE2ASCII(PIM_CAND_RP_ADV),
			 cstring(src), cstring(dst),
			 cstring(rp_addr),
			 cstring(group_prefix),
			 cstring(active_bsr_zone->zone_id()),
			 error_msg.c_str());
	    continue;
	}
	bsr_rp->start_candidate_rp_expiry_timer();
	
	// XXX: schedule to send immediately a Bootstrap message if needed
	if (is_rp_info_changed) {
	    active_bsr_zone->expire_bsr_timer();
	    bool_add_rps_to_rp_table = true;
	}
    }
    // If needed, add RPs to the RP table.
    if (bool_add_rps_to_rp_table)
	pim_bsr.add_rps_to_rp_table();
    
    
    UNUSED(pim_nbr);
    
    return (XORP_OK);
    
    // Various error processing
 rcvlen_error:
    XLOG_WARNING("RX %s from %s to %s: "
		 "invalid message length",
		 PIMTYPE2ASCII(PIM_CAND_RP_ADV),
		 cstring(src), cstring(dst));
    ++_pimstat_rx_malformed_packet;
    return (XORP_ERROR);

 rcvd_mask_len_error:
    XLOG_WARNING("RX %s from %s to %s: "
		 "invalid group mask length = %d",
		 PIMTYPE2ASCII(PIM_CAND_RP_ADV),
		 cstring(src), cstring(dst),
		 group_mask_len);
    return (XORP_ERROR);
    
 rcvd_family_error:
    XLOG_WARNING("RX %s from %s to %s: "
		 "invalid address family inside = %d",
		 PIMTYPE2ASCII(PIM_CAND_RP_ADV),
		 cstring(src), cstring(dst),
		 rcvd_family);
    return (XORP_ERROR);
}

int
PimVif::pim_cand_rp_adv_send(const IPvX& bsr_addr, const BsrZone& bsr_zone)
{
    // TODO: add a check whether I am a Cand-RP for that zone.
    // XXX: for now there is no simple check for that, so add a flag to BsrZone
    
    if (! bsr_addr.is_unicast())
	return (XORP_ERROR);	// Invalid BSR address
    
    if (bsr_addr == bsr_zone.my_bsr_addr())
	return (XORP_ERROR);	// Don't send the message to my BSR address
    
    //
    // XXX: for some reason, the 'Priority' and 'Holdtime' fields
    // are per Cand-RP-Advertise message, instead of per group prefix.
    // Well, hmmm, the Cand-RP state actually is that RP priority
    // is per RP per group prefix, so if the priority for each
    // group prefix is different, we have to send more than one
    // Cand-RP-Advertise messages.
    // Here we could try to be smart and combine Cand-RP-Advertise messages
    // that have same priority and holdtime, but this makes things
    // more complicated (TODO: add that complexity!)
    // Hence, we send only one group prefix per Cand-RP-Advertise message.
    //
    list<BsrGroupPrefix *>::const_iterator iter_prefix;
    for (iter_prefix = bsr_zone.bsr_group_prefix_list().begin();
	 iter_prefix != bsr_zone.bsr_group_prefix_list().end();
	 ++iter_prefix) {
	BsrGroupPrefix *bsr_group_prefix = *iter_prefix;
	const IPvXNet& group_prefix = bsr_group_prefix->group_prefix();
	bool all_multicast_groups_bool, is_zbr;
    	
	if (group_prefix == IPvXNet::ip_multicast_base_prefix(family())) {
	    all_multicast_groups_bool = true;
	} else {
	    all_multicast_groups_bool = false;
	}
	
	//
	// Test if I am Zone Border Router (ZBR) for this prefix 
	//
	if (pim_node().pim_scope_zone_table().is_zone_border_router(group_prefix))
	    is_zbr = true;
	else
	    is_zbr = false;
	
	list<BsrRp *>::const_iterator iter_rp;
	for (iter_rp = bsr_group_prefix->rp_list().begin();
	     iter_rp != bsr_group_prefix->rp_list().end();
	     ++iter_rp) {
	    BsrRp *bsr_rp = *iter_rp;
	    buffer_t *buffer;
	    
	    if (! pim_node().is_my_addr(bsr_rp->rp_addr()))
		continue;	// Ignore addresses that aren't mine
	    
	    buffer = buffer_send_prepare();
	    // Write all data to the buffer
	    if (all_multicast_groups_bool)
		BUFFER_PUT_OCTET(0, buffer);	// XXX: default to all groups
	    else
		BUFFER_PUT_OCTET(1, buffer);	// XXX: send one group prefix
	    BUFFER_PUT_OCTET(bsr_rp->rp_priority(), buffer);
	    if (bsr_zone.is_cancel())
		BUFFER_PUT_HOST_16(0, buffer);
	    else
		BUFFER_PUT_HOST_16(bsr_rp->rp_holdtime(), buffer);
	    PUT_ENCODED_UNICAST_ADDR(family(), bsr_rp->rp_addr(), buffer);
	    if (! all_multicast_groups_bool) {
		uint8_t group_addr_reserved_flags = 0;
		if (is_zbr)
		    group_addr_reserved_flags |= EGADDR_Z_BIT;
		PUT_ENCODED_GROUP_ADDR(family(),
				       group_prefix.masked_addr(),
				       group_prefix.prefix_len(),
				       group_addr_reserved_flags, buffer);
	    }
	    
	    pim_send(bsr_addr, PIM_CAND_RP_ADV, buffer);
	}
    }
    
    return (XORP_OK);
    
 invalid_addr_family_error:
    XLOG_UNREACHABLE();
    XLOG_ERROR("TX %s from %s to %s: "
	       "invalid address family error = %d",
	       PIMTYPE2ASCII(PIM_CAND_RP_ADV),
	       cstring(addr()), cstring(bsr_addr),
	       family());
    return (XORP_ERROR);
    
 buflen_error:
    XLOG_UNREACHABLE();
    XLOG_ERROR("TX %s from %s to %s: "
	       "packet cannot fit into sending buffer",
	       PIMTYPE2ASCII(PIM_CAND_RP_ADV),
	       cstring(addr()), cstring(bsr_addr));
    return (XORP_ERROR);
}
