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

#ident "$XORP: xorp/pim/pim_proto_join_prune.cc,v 1.4 2003/06/16 22:48:03 pavlin Exp $"


//
// PIM PIM_JOIN_PRUNE messages processing.
//


#include "pim_module.h"
#include "pim_private.hh"
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
 * PimVif::pim_join_prune_recv:
 * @pim_nbr: The PIM neighbor message originator (or NULL if not a neighbor).
 * @src: The message source address.
 * @dst: The message destination address.
 * @buffer: The buffer with the message.
 * @message_type: The message type (should be either %PIM_JOIN_PRUNE
 * or %PIM_GRAFT.
 * 
 * Receive PIM_JOIN_PRUNE message.
 * XXX: PIM_GRAFT message has the same format as the
 * PIM_JOIN_PRUNE message, hence we use the PIM_JOIN_PRUNE processing
 * function to process PIM_GRAFT as well.
 * TODO: for now ignore PIM_GRAFT messages logic; will need to come
 * back to it later.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimVif::pim_join_prune_recv(PimNbr *pim_nbr, const IPvX& src,
			    const IPvX& dst, buffer_t *buffer,
			    uint8_t message_type)
{
    int		holdtime;
    int		rcvd_family;
    IPvX	target_nbr_addr(family());
    IPvX	source_addr(family()), group_addr(family());
    uint8_t	source_masklen, group_masklen;
    uint8_t	group_addr_reserved_flags;
    int		groups_n, sources_n, sources_j_n, sources_p_n;
    uint8_t	source_flags;
    bool	ignore_group_bool, rp_entry_bool, new_group_bool;
    action_jp_t	action_jp;
    mrt_entry_type_t mrt_entry_type;
    PimJpHeader jp_header(pim_node());
    
    //
    // Parse the message
    //
    GET_ENCODED_UNICAST_ADDR(rcvd_family, target_nbr_addr, buffer);
    // Check the target neighbor address.
    // XXX: on point-to-point links we accept target_nbr_addr of all zeros
    if (! (target_nbr_addr.is_unicast()
	   || (is_p2p() && (target_nbr_addr == IPvX::ZERO(family()))))) {
	XLOG_WARNING("RX %s from %s to %s: "
		     "invalid 'upstream neighbor' "
		     "address: %s",
		     PIMTYPE2ASCII(message_type),
		     cstring(src), cstring(dst),
		     cstring(target_nbr_addr));
	return (XORP_ERROR);
    }
    
    BUFFER_GET_SKIP(1, buffer);		// Reserved
    BUFFER_GET_OCTET(groups_n, buffer);
    BUFFER_GET_HOST_16(holdtime, buffer);
    
    //
    // XXX: here we should test that (groups_n > 0), but
    // we should be liberal about such errors.
    //
    while (groups_n--) {
	GET_ENCODED_GROUP_ADDR(rcvd_family, group_addr, group_masklen,
			       group_addr_reserved_flags, buffer);
	BUFFER_GET_HOST_16(sources_j_n, buffer);
	BUFFER_GET_HOST_16(sources_p_n, buffer);
	// Check the group address
	ignore_group_bool = true;
	rp_entry_bool = false;
	new_group_bool = true;
	
	//
	// Check the group address and masklen
	//
	do {
	    if (group_masklen == IPvX::ip_multicast_base_address_masklen(family())
		&& (group_addr == IPvX::MULTICAST_BASE(family()))) {
		// XXX: (*,*,RP) Join/Prune
		ignore_group_bool = false;
		rp_entry_bool = true;
		break;
	    }
	    if (! group_addr.is_multicast()) {
		XLOG_WARNING("RX %s from %s to %s: "
			     "invalid group address: %s",
			     PIMTYPE2ASCII(message_type),
			     cstring(src), cstring(dst),
			     cstring(group_addr));
		ignore_group_bool = true;
		break;
	    }
	    if (group_addr.is_linklocal_multicast()
		|| group_addr.is_nodelocal_multicast()) {
		XLOG_WARNING("RX %s from %s to %s: "
			     "non-routable link or node-local "
			     "group address: %s",
			     PIMTYPE2ASCII(message_type),
			     cstring(src), cstring(dst),
			     cstring(group_addr));
		ignore_group_bool = true;
		break;
	    }
	    if (group_masklen != group_addr.addr_bitlen()) {
		ignore_group_bool = true;
		XLOG_WARNING("RX %s from %s to %s: "
			     "invalid group masklen for group %s: %d",
			     PIMTYPE2ASCII(message_type),
			     cstring(src), cstring(dst),
			     cstring(group_addr),
			     group_masklen);
		break;
	    }
	    ignore_group_bool = false;
	    break;
	} while (false);
	
	sources_n = sources_j_n + sources_p_n;
	action_jp = ACTION_JOIN;
	while (sources_n--) {
	    if (sources_j_n > 0) {
		action_jp = ACTION_JOIN;
		sources_j_n--;
	    } else {
		action_jp = ACTION_PRUNE;
		sources_p_n--;
	    }
	    
	    GET_ENCODED_SOURCE_ADDR(rcvd_family, source_addr,
				    source_masklen, source_flags, buffer);
	    if (ignore_group_bool)
		continue;
	    
	    // Check the source address and masklen
	    if (! source_addr.is_unicast()) {
		XLOG_WARNING("RX %s from %s to %s: "
			     "invalid source/RP address: %s",
			     PIMTYPE2ASCII(message_type),
			     cstring(src), cstring(dst),
			     cstring(source_addr));
		continue;
	    }
	    if (source_masklen != source_addr.addr_bitlen()) {
		XLOG_WARNING("RX %s from %s to %s: "
			     "invalid source masklen "
			     "for group %s source/RP %s: %d",
			     PIMTYPE2ASCII(message_type),
			     cstring(src), cstring(dst),
			     cstring(group_addr),
			     cstring(source_addr),
			     source_masklen);
		continue;
	    }
	    
	    //
	    // Check the source flags if they are valid.
	    //
	    if (proto_is_pimdm()) {
		if (source_flags & (ESADDR_RPT_BIT | ESADDR_WC_BIT | ESADDR_S_BIT)) {
		    // XXX: PIM-DM does not use those flags.
		    XLOG_WARNING("RX %s from %s to %s: "
				 "invalid source/RP flags "
				 "while running in PIM-DM mode "
				 "for (%s, %s): %#x",
				 PIMTYPE2ASCII(message_type),
				 cstring(src), cstring(dst),
				 cstring(source_addr),
				 cstring(group_addr),
				 source_flags);
		    continue;
		}
	    }
	    if (proto_is_pimsm()) {
		if ((source_flags & ESADDR_S_BIT) != ESADDR_S_BIT) {
		    // Missing "Sparse bit"
		    XLOG_WARNING("RX %s from %s to %s: "
				 "missing Sparse-bit flag "
				 "while running in PIM-SM mode "
				 "for (%s, %s)",
				 PIMTYPE2ASCII(message_type),
				 cstring(src), cstring(dst),
				 cstring(source_addr),
				 cstring(group_addr));
		    continue;
		}
	    }
	    //
	    // Check the source flags to find the entry type.
	    //
	    mrt_entry_type = MRT_ENTRY_UNKNOWN;
	    // (*,*,RP) entry
	    if (rp_entry_bool) {
		if ((source_flags & (ESADDR_RPT_BIT | ESADDR_WC_BIT))
		    != (ESADDR_RPT_BIT | ESADDR_WC_BIT)) {
		    XLOG_WARNING("RX %s from %s to %s: "
				 "invalid source/RP flags "
				 "for (*,*,RP) RP = %s: %#x",
				 PIMTYPE2ASCII(message_type),
				 cstring(src), cstring(dst),
				 cstring(source_addr), source_flags);
		    continue;
		}
		mrt_entry_type = MRT_ENTRY_RP;
		goto jp_entry_add_label;
	    }
	    
	    // (*,G) entry
	    if ((source_flags & (ESADDR_RPT_BIT | ESADDR_WC_BIT))
		== (ESADDR_RPT_BIT | ESADDR_WC_BIT)) {
		// Check if the RP address matches. If not, silently ignore
		PimRp *pim_rp = pim_node().rp_table().rp_find(group_addr);
		if ((pim_rp == NULL) || (pim_rp->rp_addr() != source_addr)) {
		    XLOG_TRACE(pim_node().is_log_trace(),
			       "RX %s from %s to %s: "
			       "(*,G) Join/Prune entry for group %s "
			       "RP address does not match: "
			       "router RP = %s, message RP = %s",
			       PIMTYPE2ASCII(message_type),
			       cstring(src), cstring(dst),
			       cstring(group_addr),
			       pim_rp? cstring(pim_rp->rp_addr()) : "NULL",
			       cstring(source_addr));
		    continue;
		}
		mrt_entry_type = MRT_ENTRY_WC;
		goto jp_entry_add_label;
	    }
	    
	    // (S,G,rpt) entry
	    if ((source_flags & (ESADDR_RPT_BIT | ESADDR_WC_BIT))
		== ESADDR_RPT_BIT) {
		mrt_entry_type = MRT_ENTRY_SG_RPT;
		goto jp_entry_add_label;
	    }
	    
	    // (S,G) entry
	    if ((source_flags & (ESADDR_RPT_BIT | ESADDR_WC_BIT)) == 0) {
		mrt_entry_type = MRT_ENTRY_SG;
		goto jp_entry_add_label;
	    }
	    
	    // Unknown entry
	    XLOG_WARNING("RX %s from %s to %s: "
			 "invalid source/RP flags for (%s, %s): %#x",
			 PIMTYPE2ASCII(message_type),
			 cstring(src), cstring(dst),
			 cstring(source_addr),
			 cstring(group_addr),
			 source_flags);
	    continue;
	    
	jp_entry_add_label:
	    // An the entry to the pool of entries pending commit.
	    // TODO: if error, then ignore the whole message??
	    jp_header.jp_entry_add(source_addr, group_addr,
				   group_masklen, mrt_entry_type,
				   action_jp, holdtime, new_group_bool);
	    new_group_bool = false;
	}
    }
    
    // Commit all Join/Prune requests to the MRT
    jp_header.mrt_commit(this, target_nbr_addr);
    jp_header.reset();
    
    UNUSED(pim_nbr);
    
    return (XORP_OK);
    
    // Various error processing
 rcvlen_error:
    XLOG_WARNING("RX %s from %s to %s: "
		 "invalid message length",
		 PIMTYPE2ASCII(PIM_JOIN_PRUNE),
		 cstring(src), cstring(dst));
    return (XORP_ERROR);
    
 rcvd_masklen_error:
    XLOG_WARNING("RX %s from %s to %s: "
		 "invalid masklen = %d",
		 PIMTYPE2ASCII(PIM_JOIN_PRUNE),
		 cstring(src), cstring(dst),
		 group_masklen);
    return (XORP_ERROR);
    
 rcvd_family_error:
    XLOG_WARNING("RX %s from %s to %s: "
		 "invalid address family inside = %d",
		 PIMTYPE2ASCII(PIM_JOIN_PRUNE),
		 cstring(src), cstring(dst),
		 rcvd_family);
    return (XORP_ERROR);
}

//
// Send Join/Prune message(s) to a PIM neighbor.
//
int
PimVif::pim_join_prune_send(PimNbr *pim_nbr, PimJpHeader *jp_header)
{
    int ret_value = jp_header->network_commit(this, pim_nbr->addr());
    
    jp_header->reset();
    
    return (ret_value);
}
