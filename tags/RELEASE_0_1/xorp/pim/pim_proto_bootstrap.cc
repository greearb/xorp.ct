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

#ident "$XORP: xorp/pim/pim_proto_bootstrap.cc,v 1.37 2002/12/09 18:29:28 hodson Exp $"


//
// PIM PIM_BOOTSTRAP messages processing.
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
 * PimVif::pim_bootstrap_recv:
 * @pim_nbr: The PIM neighbor message originator (or NULL if not a neighbor).
 * @src: The message source address.
 * @dst: The message destination address.
 * @buffer: The buffer with the message.
 * 
 * Receive PIM_BOOTSTRAP message.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimVif::pim_bootstrap_recv(PimNbr *pim_nbr, const IPvX& src,
			   const IPvX& dst, buffer_t *buffer)
{
    int		rcvd_family;
    PimBsr&	pim_bsr = pim_node().pim_bsr();
    bool	unicast_message_bool = false;
    IPvX	bsr_addr(family());
    uint8_t	hash_masklen, bsr_priority;
    uint16_t	fragment_tag;
    uint32_t	group_masklen = 0;
    
    //
    // Parse the head of the message
    //
    do {
	BUFFER_GET_HOST_16(fragment_tag, buffer);
	BUFFER_GET_OCTET(hash_masklen, buffer);
	BUFFER_GET_OCTET(bsr_priority, buffer);
	GET_ENCODED_UNICAST_ADDR(rcvd_family, bsr_addr, buffer);
	if (! bsr_addr.is_unicast()) {
	    XLOG_WARNING("RX %s from %s to %s: "
			 "invalid Bootstrap router "
			 "address: %s. "
			 "Ignoring whole message.",
			 PIMTYPE2ASCII(PIM_BOOTSTRAP),
			 cstring(src), cstring(dst),
			 cstring(bsr_addr));
	    return (XORP_ERROR);
	}
	
	//
	// Message check
	//
	// XXX: the following checks were performed earlier:
	//      - There is Hello state for BSM.src_ip_address
	//      - The sender is directly connected
	// RPF check
	if (dst == IPvX::PIM_ROUTERS(family())) {
	    PimNbr *pim_nbr_rpf = pim_node().pim_nbr_rpf_find(bsr_addr);
	    if (pim_nbr_rpf != pim_nbr)
		return (XORP_ERROR);
	} else if (pim_node().vif_find_by_addr(dst) != NULL) {
	    // BSM.dst_ip_address is one of my addresses
	    unicast_message_bool = true;
	} else {
	    // Either the destination is not the right multicast group, or
	    // somehow an unicast packet for somebody else came to me.
	    // A third possibility is the destition unicast address is
	    // one of mine, but it is not enabled for multicast routing.
	    return (XORP_ERROR);
	}
    } while (false);
    
    //
    // Process the rest of the message
    //
    do {
	size_t group_prefix_n = 0;
	string error_msg = "";
	
	//
	// Create a new BsrZone that would contain all information
	// from this message.
	//
	BsrZone bsr_zone(pim_bsr, bsr_addr, bsr_priority,
			 hash_masklen, fragment_tag);
	
	//
	// Main loop for message parsing
	//
	while (BUFFER_DATA_SIZE(buffer) > 0) {
	    IPvX	group_addr(family());
	    uint8_t	group_addr_reserved_flags;
	    uint8_t	rp_count, fragment_rp_count;
	    bool	admin_scope_zone_bool;
	    BsrGroupPrefix *bsr_group_prefix = NULL;
	    
	    //
	    // The Group (prefix) address
	    //
	    group_prefix_n++;
	    GET_ENCODED_GROUP_ADDR(rcvd_family, group_addr, group_masklen,
				   group_addr_reserved_flags, buffer);
	    IPvXNet group_prefix(group_addr, group_masklen);
	    if (! group_prefix.is_multicast()) {
		XLOG_WARNING("RX %s from %s to %s: "
			     "invalid group prefix: %s. "
			     "Ignoring whole message.",
			     PIMTYPE2ASCII(PIM_BOOTSTRAP),
			     cstring(src), cstring(dst),
			     cstring(group_prefix));
		return (XORP_ERROR);
	    }
	    if (group_addr_reserved_flags & EGADDR_Z_BIT)
		admin_scope_zone_bool = true;
	    else
		admin_scope_zone_bool = false;
	    
	    if (admin_scope_zone_bool
		&& (group_prefix_n == 1)
		&& pim_node().pim_scope_zone_table().is_scoped(
		    group_addr,
		    pim_nbr->vif_index())) {
		// if (the interface the message arrived on is an Admin Scope
		//     border for the BSM.first_group_address) {
		//   drop the BS message silently
		// }
		// TODO: XXX: PAVPAVPAV: print a warning if the scoped zone
		// is NOT the local scoped zone!!
		return (XORP_ERROR);
	    }
	    
	    if (admin_scope_zone_bool) {
		if (group_prefix_n == 1) {
		    // Set the scope zone
		    bsr_zone.set_admin_scope_zone(admin_scope_zone_bool,
						  group_prefix);
		} else {
		    // Test if the group prefix is contained by the scope zone
		    if (! bsr_zone.admin_scope_zone_id().contains(group_prefix)) {
			XLOG_WARNING("RX %s from %s to %s: "
				     "group prefix %s is not contained in "
				     "scope zone %s."
				     "Ignoring whole message.",
				     PIMTYPE2ASCII(PIM_BOOTSTRAP),
				     cstring(src), cstring(dst),
				     cstring(group_prefix),
				     cstring(bsr_zone.admin_scope_zone_id()));
			return (XORP_ERROR);
		    }
		}
	    }
	    
	    BUFFER_GET_OCTET(rp_count, buffer);
	    BUFFER_GET_OCTET(fragment_rp_count, buffer);
	    BUFFER_GET_SKIP(2, buffer);		// Reserved
	    
	    if (fragment_rp_count > rp_count) {
		XLOG_WARNING("RX %s from %s to %s: "
			     "RP count %d is larger than "
			     "fragment RP count %d. "
			     "Ignoring whole message.",
			     PIMTYPE2ASCII(PIM_BOOTSTRAP),
			     cstring(src), cstring(dst),
			     rp_count, fragment_rp_count);
		return (XORP_ERROR);
	    }
	    
	    // Add the group prefix
	    bsr_group_prefix = bsr_zone.add_bsr_group_prefix(
		admin_scope_zone_bool,
		group_prefix,
		rp_count,
		error_msg);
	    if (bsr_group_prefix == NULL) {
		XLOG_WARNING("RX %s from %s to %s: "
			     "cannot add group prefix %s to zone %s. "
			     "Ignoring whole message. Reason: %s",
			     PIMTYPE2ASCII(PIM_BOOTSTRAP),
			     cstring(src), cstring(dst),
			     cstring(group_prefix),
			     cstring(bsr_zone.admin_scope_zone_id()),
			     error_msg.c_str());
		return (XORP_ERROR);
	    }
	    
	    //
	    // The set of RPs
	    //
	    while (fragment_rp_count--) {
		IPvX		rp_addr(family());
		uint8_t		rp_priority;
		uint16_t	rp_holdtime;
		BsrRp		*bsr_rp = NULL;
		
		GET_ENCODED_UNICAST_ADDR(rcvd_family, rp_addr, buffer);
		if (! rp_addr.is_unicast()) {
		    XLOG_WARNING("RX %s from %s to %s: "
				 "invalid RP address: %s."
				 "Ignoring whole message.",
				 PIMTYPE2ASCII(PIM_BOOTSTRAP),
				 cstring(src), cstring(dst),
				 cstring(rp_addr));
		    return (XORP_ERROR);
		}
		BUFFER_GET_HOST_16(rp_holdtime, buffer);
		BUFFER_GET_OCTET(rp_priority, buffer);
		BUFFER_GET_SKIP(1, buffer);		// Reserved
		if (bsr_group_prefix != NULL) {
		    bsr_rp = bsr_group_prefix->add_rp(rp_addr, rp_priority,
						      rp_holdtime, error_msg);
		    if (bsr_rp == NULL) {
			XLOG_WARNING("RX %s from %s to %s: "
				     "cannot add RP address %s to group %s."
				     "Ignoring whole message. Reason: %s",
				     PIMTYPE2ASCII(PIM_BOOTSTRAP),
				     cstring(src), cstring(dst),
				     cstring(rp_addr),
				     cstring(bsr_group_prefix->group_prefix()),
				     error_msg.c_str());
			return (XORP_ERROR);
		    }
		}
	    }
	}
	
	//
	// Test if this is not a bogus unicast Bootstrap message.
	//
	bool is_accepted_previous_bsm = false;
	do {
	    // Find if accepted previous BSM
	    BsrZone *tmp_bsr_zone = pim_bsr.find_bsr_zone_from_list(
		pim_bsr.active_bsr_zone_list(),
		bsr_zone.is_admin_scope_zone(),
		bsr_zone.admin_scope_zone_id());
	    if (tmp_bsr_zone != NULL)
		is_accepted_previous_bsm = tmp_bsr_zone->is_accepted_previous_bsm();
	} while (false);
	if (unicast_message_bool && is_accepted_previous_bsm) {
	    // The packet was unicast, but this wasn't a quick refresh
	    // on startup. Drop the BS message silently.
	    return (XORP_ERROR);
	}
	
	//
	// Test if the received data is consistent
	//
	if (! bsr_zone.is_consistent(error_msg)) {
	    XLOG_WARNING("RX %s from %s to %s: "
			 "inconsistent Bootstrap zone %s: %s",
			 PIMTYPE2ASCII(PIM_BOOTSTRAP),
			 cstring(src), cstring(dst),
			 cstring(bsr_zone.admin_scope_zone_id()),
			 error_msg.c_str());
	    return (XORP_ERROR);
	}
	
	//
	// Test if the information in the new BSM was received already.
	//
	if (pim_bsr.contains_bsr_zone_info(pim_bsr.active_bsr_zone_list(),
					   bsr_zone)) {
	    // Redundant information. Silently ignore it.
	    continue;
	}
	
	//
	// Test if we should set the 'is_accepted_previous_bsm' flag.
	//
	// XXX: we continue accepting unicast fragments as long as
	// they carry the same fragment tag of the first fragment.
	//
	do {
	    // Find if have previous BSM.
	    BsrZone *tmp_bsr_zone = pim_bsr.find_bsr_zone_from_list(
		pim_bsr.active_bsr_zone_list(),
		bsr_zone.is_admin_scope_zone(),
		bsr_zone.admin_scope_zone_id());
	    if (tmp_bsr_zone != NULL) {
		if (tmp_bsr_zone->fragment_tag() != bsr_zone.fragment_tag()) {
		    tmp_bsr_zone->set_accepted_previous_bsm(true);
		    bsr_zone.set_accepted_previous_bsm(true);
		}
	    }
	} while (false);
	
	//
	// Try to add the received BSM
	//
	if (pim_bsr.add_active_bsr_zone(bsr_zone, error_msg) == NULL) {
	    XLOG_WARNING("RX %s from %s to %s: "
			 "cannot add Bootstrap zone %s: %s",
			 PIMTYPE2ASCII(PIM_BOOTSTRAP),
			 cstring(src), cstring(dst),
			 cstring(bsr_zone.admin_scope_zone_id()),
			 error_msg.c_str());
	    return (XORP_ERROR);
	}
	
	//
	// Forward the BSR message if needed
	//
	if (bsr_zone.is_bsm_forward()) {
	    // Forward the Bootstrap message
	    for (uint16_t vif_index = 0;
		 vif_index < pim_node().maxvifs();
		 vif_index++) {
		PimVif *pim_vif = pim_node().vif_find_by_vif_index(vif_index);
		if (pim_vif == NULL)
		    continue;
		if (pim_vif == this)
		    continue;		// Don't forward back on the iif
		
		// Don't send the Bootstrap message across scope zone boundries
		if (bsr_zone.is_admin_scope_zone()
		    && pim_node().pim_scope_zone_table().is_scoped(
			bsr_zone.admin_scope_zone_id().masked_addr(),
			vif_index))
		    continue;
		
		pim_vif->pim_bootstrap_send(IPvX::PIM_ROUTERS(family()),
					    bsr_zone);
	    }
	}
    } while (false);
    
    //
    // Originate my own BSR messages if needed
    //
    do {
	list<BsrZone *>::iterator iter_zone;
	for (iter_zone = pim_bsr.active_bsr_zone_list().begin();
	     iter_zone != pim_bsr.active_bsr_zone_list().end();
	     ++iter_zone) {
	    BsrZone& bsr_zone = *(*iter_zone);
	    if (! bsr_zone.is_bsm_originate())
		continue;
	    bsr_zone.new_fragment_tag();
	    // Send the Bootstrap message
	    for (uint16_t vif_index = 0;
		 vif_index < pim_node().maxvifs();
		 vif_index++) {
		PimVif *pim_vif = pim_node().vif_find_by_vif_index(vif_index);
		if (pim_vif == NULL)
		    continue;
		
		// Don't send the Bootstrap message across scope zone boundries
		if (bsr_zone.is_admin_scope_zone()
		    && pim_node().pim_scope_zone_table().is_scoped(
			bsr_zone.admin_scope_zone_id().masked_addr(),
			vif_index))
		    continue;
		pim_vif->pim_bootstrap_send(IPvX::PIM_ROUTERS(family()),
					    bsr_zone);
	    }
	    
	    // Housekeeping: reset the bsm_forward/bsm_originate flags.
	    bsr_zone.set_bsm_forward(false);
	    bsr_zone.set_bsm_originate(false);
	}
    } while (false);
    
    //
    // Apply the new RP info
    //
    pim_bsr.add_rps_to_rp_table();
    
    return (XORP_OK);
    
    // Various error processing
 rcvlen_error:
    XLOG_WARNING("RX %s from %s to %s: "
		 "invalid message length",
		 PIMTYPE2ASCII(PIM_BOOTSTRAP),
		 cstring(src), cstring(dst));
    return (XORP_ERROR);
    
 rcvd_masklen_error:
    XLOG_WARNING("RX %s from %s to %s: "
		 "invalid masklen = %d",
		 PIMTYPE2ASCII(PIM_BOOTSTRAP),
		 cstring(src), cstring(dst),
		 group_masklen);
    return (XORP_ERROR);
    
 rcvd_family_error:
    XLOG_WARNING("RX %s from %s to %s: "
		 "invalid address family inside = %d",
		 PIMTYPE2ASCII(PIM_BOOTSTRAP),
		 cstring(src), cstring(dst),
		 rcvd_family);
    return (XORP_ERROR);
}

// Return: %XORP_OK on success, otherwise %XORP_ERROR
int
PimVif::pim_bootstrap_send(const IPvX& dst_addr,
			   const BsrZone& bsr_zone)
{
    size_t avail_buffer_size = 0;
    
    if (bsr_zone.bsr_addr() == dst_addr)
	return (XORP_ERROR);	// Never send-back to the BSR
    
    if (bsr_zone.bsr_addr() == IPvX::ZERO(family()))
	return (XORP_ERROR);	// There is no BSR yet
    
    if (pim_nbrs_number() == 0)
	return (XORP_ERROR);	// No PIM neighbors yet, hence don't send it
    
    buffer_t *buffer = pim_bootstrap_send_prepare(dst_addr, bsr_zone, true);
    if (buffer == NULL)
	return (XORP_ERROR);
    
    list<BsrGroupPrefix *>::const_iterator iter_prefix;
    for (iter_prefix = bsr_zone.bsr_group_prefix_list().begin();
	 iter_prefix != bsr_zone.bsr_group_prefix_list().end();
	 ++iter_prefix) {
	BsrGroupPrefix *bsr_group_prefix = *iter_prefix;
	size_t needed_size = 0;
	size_t received_rp_count = bsr_group_prefix->received_rp_count();
	size_t max_fragment_rp_count = 0;
	size_t send_fragment_rp_count = 0;
	size_t send_remain_rp_count = 0;
	uint8_t group_addr_reserved_flags = 0;
	
	if (bsr_group_prefix->is_admin_scope_zone())
	    group_addr_reserved_flags |= EGADDR_Z_BIT;
	
	needed_size =
	    ENCODED_GROUP_ADDR_SIZE(family())
	    + sizeof(uint8_t)
	    + sizeof(uint8_t)
	    + sizeof(uint16_t);
	needed_size +=
	    received_rp_count * (
		ENCODED_UNICAST_ADDR_SIZE(family())
		+ sizeof(uint16_t)
		+ sizeof(uint8_t)
		+ sizeof(uint8_t));
	if (needed_size > BUFFER_AVAIL_TAIL(buffer)) {
	    //
	    // Try to do fragmentation among group prefixes for this zone
	    //
	    if (iter_prefix != bsr_zone.bsr_group_prefix_list().begin()) {
		// Send the accumulated prefixes so far
		if (pim_send(dst_addr, PIM_BOOTSTRAP, buffer) < 0)
		    return (XORP_ERROR);
		buffer = pim_bootstrap_send_prepare(dst_addr, bsr_zone, false);
		if (buffer == NULL)
		    return (XORP_ERROR);
	    }
	}
	
	//
	// Compute the max. number of RPs we can add to the message
	//
	avail_buffer_size = BUFFER_AVAIL_TAIL(buffer);
	if (avail_buffer_size <
	    ENCODED_GROUP_ADDR_SIZE(family())
	    + sizeof(uint8_t)
	    + sizeof(uint8_t)
	    + sizeof(uint16_t)) {
	    goto buflen_error;
	}
	avail_buffer_size -=
	    ENCODED_GROUP_ADDR_SIZE(family())
	    + sizeof(uint8_t)
	    + sizeof(uint8_t)
	    + sizeof(uint16_t);
	
	max_fragment_rp_count = avail_buffer_size /
	    (ENCODED_UNICAST_ADDR_SIZE(family())
	     + sizeof(uint16_t)
	     + sizeof(uint8_t)
	     + sizeof(uint8_t));
	if (max_fragment_rp_count == 0)
	    goto buflen_error;
	send_fragment_rp_count = max_fragment_rp_count;
	if (send_fragment_rp_count > received_rp_count)
	    send_fragment_rp_count = received_rp_count;
	send_remain_rp_count = received_rp_count;
	
	PUT_ENCODED_GROUP_ADDR(family(),
			       bsr_group_prefix->group_prefix().masked_addr(),
			       bsr_group_prefix->group_prefix().prefix_len(),
			       group_addr_reserved_flags, buffer);
	// RP count and Fragment RP count
	BUFFER_PUT_OCTET(bsr_group_prefix->expected_rp_count(), buffer);
	BUFFER_PUT_OCTET(send_fragment_rp_count, buffer);
	BUFFER_PUT_HOST_16(0, buffer);			// Reserved
	// The list of RPs
	list<BsrRp *>::const_iterator iter_rp;
	for (iter_rp = bsr_group_prefix->rp_list().begin();
	     iter_rp != bsr_group_prefix->rp_list().end();
	     ++iter_rp) {
	    BsrRp *bsr_rp = *iter_rp;
	    
	    if (send_fragment_rp_count == 0) {
		//
		// Send the accumulated RPs so far
		//
		if (pim_send(dst_addr, PIM_BOOTSTRAP, buffer) < 0)
		    return (XORP_ERROR);
		buffer = pim_bootstrap_send_prepare(dst_addr, bsr_zone, false);
		if (buffer == NULL)
		    return (XORP_ERROR);
		
		avail_buffer_size = BUFFER_AVAIL_TAIL(buffer);
		if (avail_buffer_size <
		    ENCODED_GROUP_ADDR_SIZE(family())
		    + sizeof(uint8_t)
		    + sizeof(uint8_t)
		    + sizeof(uint16_t)) {
		    goto buflen_error;
		}
		avail_buffer_size -=
		    ENCODED_GROUP_ADDR_SIZE(family())
		    + sizeof(uint8_t)
		    + sizeof(uint8_t)
		    + sizeof(uint16_t);
		
		max_fragment_rp_count = avail_buffer_size /
		    (ENCODED_UNICAST_ADDR_SIZE(family())
		     + sizeof(uint16_t)
		     + sizeof(uint8_t)
		     + sizeof(uint8_t));
		if (max_fragment_rp_count == 0)
		    goto buflen_error;
		send_fragment_rp_count = max_fragment_rp_count;
		if (send_fragment_rp_count > send_remain_rp_count)
		    send_fragment_rp_count = send_remain_rp_count;
		
		PUT_ENCODED_GROUP_ADDR(family(),
				       bsr_group_prefix->group_prefix().masked_addr(),
				       bsr_group_prefix->group_prefix().prefix_len(),
				       group_addr_reserved_flags, buffer);
		// RP count and Fragment RP count
		BUFFER_PUT_OCTET(bsr_group_prefix->expected_rp_count(),
				 buffer);
		BUFFER_PUT_OCTET(send_fragment_rp_count, buffer);
		BUFFER_PUT_HOST_16(0, buffer);		// Reserved

	    }
	    send_fragment_rp_count--;
	    send_remain_rp_count--;
	    
	    //
	    // Put the RP information
	    //
	    PUT_ENCODED_UNICAST_ADDR(family(), bsr_rp->rp_addr(), buffer);
	    BUFFER_PUT_HOST_16(bsr_rp->rp_holdtime(), buffer);
	    BUFFER_PUT_OCTET(bsr_rp->rp_priority(), buffer);
	    BUFFER_PUT_OCTET(0, buffer);		// Reserved
	}
    }
    
    //
    // Send the lastest fragment
    //
    if (pim_send(dst_addr, PIM_BOOTSTRAP, buffer) < 0)
	return (XORP_ERROR);
    
    return (XORP_OK);
    
 invalid_addr_family_error:
    XLOG_ASSERT(false);
    XLOG_ERROR("TX %s from %s to %s: "
	       "invalid address family error = %d",
	       PIMTYPE2ASCII(PIM_BOOTSTRAP),
	       cstring(addr()), cstring(dst_addr),
	       family());
    return (XORP_ERROR);
    
 buflen_error:
    XLOG_ASSERT(false);
    XLOG_ERROR("TX %s from %s to %s: "
	       "packet cannot fit into sending buffer",
	       PIMTYPE2ASCII(PIM_BOOTSTRAP),
	       cstring(addr()), cstring(dst_addr));
    return (XORP_ERROR);
}

//
// Prepare the zone-specific data and add it to the buffer to send.
//
buffer_t *
PimVif::pim_bootstrap_send_prepare(const IPvX& dst_addr,
				   const BsrZone& bsr_zone,
				   bool is_first_fragment)
{
    buffer_t *buffer = buffer_send_prepare(_buffer_send_bootstrap);
    uint8_t hash_masklen = bsr_zone.hash_masklen();
    uint8_t group_addr_reserved_flags = 0;
    
    //
    // Write zone-related data to the buffer
    //
    BUFFER_PUT_HOST_16(bsr_zone.fragment_tag(), buffer);
    BUFFER_PUT_OCTET(hash_masklen, buffer);
    BUFFER_PUT_OCTET(bsr_zone.bsr_priority(), buffer);
    PUT_ENCODED_UNICAST_ADDR(family(), bsr_zone.bsr_addr(), buffer);
    
    //
    // Test whether we need a prefix for the entire admin scope
    // range with no RPs.
    //
    do {
	if (! bsr_zone.is_admin_scope_zone())
	    break;
	
	list<BsrGroupPrefix *>::const_iterator iter_prefix
	    = bsr_zone.bsr_group_prefix_list().begin();
	if (iter_prefix == bsr_zone.bsr_group_prefix_list().end())
	    break;
	BsrGroupPrefix *bsr_group_prefix = *iter_prefix;
	if (is_first_fragment
	    && (bsr_group_prefix->group_prefix()
		== bsr_zone.admin_scope_zone_id()))
	    break;	// XXX: the admin scope range will be added later
	
	//
	// Add the entire admin scope range with no RPs.
	//
	group_addr_reserved_flags = 0;
	group_addr_reserved_flags |= EGADDR_Z_BIT;
	PUT_ENCODED_GROUP_ADDR(family(),
			       bsr_zone.admin_scope_zone_id().masked_addr(),
			       bsr_zone.admin_scope_zone_id().prefix_len(),
			       group_addr_reserved_flags, buffer);
	BUFFER_PUT_OCTET(0, buffer);		// RP count
	BUFFER_PUT_OCTET(0, buffer);		// Fragment RP count
	BUFFER_PUT_HOST_16(0, buffer);		// Reserved
    } while (false);
    
    return (buffer);

 invalid_addr_family_error:
    XLOG_ASSERT(false);
    XLOG_ERROR("TX %s from %s to %s: "
	       "invalid address family error = %d",
	       PIMTYPE2ASCII(PIM_BOOTSTRAP),
	       cstring(addr()), cstring(dst_addr),
	       family());
    return (NULL);
    
 buflen_error:
    XLOG_ASSERT(false);
    XLOG_ERROR("TX %s from %s to %s: "
	       "packet cannot fit into sending buffer",
	       PIMTYPE2ASCII(PIM_BOOTSTRAP),
	       cstring(addr()), cstring(dst_addr));
    return (NULL);
}
