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

#ident "$XORP: xorp/pim/pim_proto_register.cc,v 1.10 2003/11/12 19:07:40 pavlin Exp $"


//
// PIM PIM_REGISTER messages processing.
//


#include "pim_module.h"
#include "pim_private.hh"
#include "mrt/inet_cksum.h"
#include "pim_mfc.hh"
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
 * PimVif::pim_register_recv:
 * @pim_nbr: The PIM neighbor message originator (or NULL if not a neighbor).
 * @src: The message source address.
 * @dst: The message destination address.
 * @buffer: The buffer with the message.
 * 
 * Receive PIM_REGISTER message.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimVif::pim_register_recv(PimNbr *pim_nbr,
			  const IPvX& src,
			  const IPvX& dst,
			  buffer_t *buffer)
{
    uint32_t pim_register_flags;
    bool null_register_bool, border_register_bool;
    IPvX inner_src(family()), inner_dst(family());
    uint32_t lookup_flags;
    PimMre *pim_mre, *pim_mre_sg;
    PimMfc *pim_mfc;
    bool is_sptbit_set = false;
    bool sent_register_stop = false;
    bool is_keepalive_timer_restarted = false;
    uint32_t keepalive_timer_sec = PIM_KEEPALIVE_PERIOD_DEFAULT;
    uint16_t register_vif_index = pim_node().pim_register_vif_index();
    
    //
    // Parse the message
    //
    BUFFER_GET_HOST_32(pim_register_flags, buffer);
    // The Border bit and the Null-Register bit
    if (pim_register_flags & PIM_BORDER_REGISTER)
	border_register_bool = true;
    else
	border_register_bool = false;
    if (pim_register_flags & PIM_NULL_REGISTER)
	null_register_bool = true;
    else
	null_register_bool = false;
    
    //
    // Get the inner source and destination addresses
    //
    switch (family()) {
    case AF_INET: {
	struct ip ip4_header;
	uint8_t *cp = (uint8_t *)&ip4_header;
	
	BUFFER_GET_DATA(cp, buffer, sizeof(ip4_header));
	inner_src.copy_in(ip4_header.ip_src);
	inner_dst.copy_in(ip4_header.ip_dst);
	if (null_register_bool) {
	    //
	    // If the inner header checksum is non-zero, then
	    // check the checksum.
	    //
	    if (ip4_header.ip_sum != 0) {
		uint16_t cksum = INET_CKSUM(&ip4_header, sizeof(ip4_header));
		if (cksum != 0) {
		    XLOG_WARNING("RX %s%s from %s to %s: "
				 "inner dummy IP header checksum error",
				 PIMTYPE2ASCII(PIM_REGISTER),
				 (null_register_bool)? "(Null)" : "",
				 cstring(src), cstring(dst));
		    ++_pimstat_bad_checksum_messages;
		    return (XORP_ERROR);
		}
	    }
	}
	break;
    }
    
#ifdef HAVE_IPV6	
    case AF_INET6: {
	struct ip6_hdr ip6_header;
	uint8_t *cp = (uint8_t *)&ip6_header;
	uint16_t inner_data_len;
	
	BUFFER_GET_DATA(cp, buffer, sizeof(ip6_header));
	inner_src.copy_in(ip6_header.ip6_src);
	inner_dst.copy_in(ip6_header.ip6_dst);
	inner_data_len = ntohs(ip6_header.ip6_plen);
	if (null_register_bool) {
	    //
	    // If the dummy PIM header is present, then
	    // check the checksum.
	    //
	    if (inner_data_len == sizeof(struct pim)) {
		struct pim pim_header;
		cp = (uint8_t *)&pim_header;
		BUFFER_GET_DATA(cp, buffer, sizeof(pim_header));
		uint16_t cksum, cksum2;
		cksum = INET_CKSUM(&pim_header, sizeof(pim_header));
		cksum2 = calculate_ipv6_pseudo_header_checksum(inner_src,
							       inner_dst,
							       sizeof(struct pim));
		cksum = INET_CKSUM_ADD(cksum, cksum2);
		if (cksum != 0) {
		    XLOG_WARNING("RX %s%s from %s to %s: "
				 "inner dummy IP header checksum error",
				 PIMTYPE2ASCII(PIM_REGISTER),
				 (null_register_bool)? "(Null)" : "",
				 cstring(src), cstring(dst));
		    ++_pimstat_bad_checksum_messages;
		    return (XORP_ERROR);
		}
	    }
	}
	break;
    }
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    
    //
    // Check the inner source and destination addresses
    //
    if (! inner_src.is_unicast()) {
	XLOG_WARNING("RX %s%s from %s to %s: "
		     "inner source address = %s must be unicast",
		     PIMTYPE2ASCII(PIM_REGISTER),
		     (null_register_bool)? "(Null)" : "",
		     cstring(src), cstring(dst),
		     cstring(inner_src));
	return (XORP_ERROR);
    }
    if (! inner_dst.is_multicast()) {
	XLOG_WARNING("RX %s%s from %s to %s: "
		     "inner destination address = %s must be multicast",
		     PIMTYPE2ASCII(PIM_REGISTER),
		     (null_register_bool)? "(Null)" : "",
		     cstring(src), cstring(dst),
		     cstring(inner_dst));
	return (XORP_ERROR);
    }
    if (inner_dst.is_linklocal_multicast()
	|| inner_dst.is_nodelocal_multicast()) {
	XLOG_WARNING("RX %s%s from %s to %s: "
		     "inner destination address = %s must not be "
		     "link or node-local multicast group",
		     PIMTYPE2ASCII(PIM_REGISTER),
		     (null_register_bool)? "(Null)" : "",
		     cstring(src), cstring(dst),
		     cstring(inner_dst));
	return (XORP_ERROR);
    }
    
    if (null_register_bool) {
	// PIM Null Register
    }

    if (border_register_bool) {
	// The Border bit is set: a Register from a PMBR
	// XXX: the new spec doesn't say how to process PIM Register
	// messages with the Border bit set.
    }
    
    //
    // XXX: the code below implements the logic in
    // packet_arrives_on_rp_tunnel( pkt ) from the spec.
    //
    
    //
    // Find if I am the RP for this group
    //
    if (register_vif_index == Vif::VIF_INDEX_INVALID) {
	// I don't have a PIM Register vif
	//
	// send Register-Stop(S,G) to outer.src
	//
	pim_register_stop_send(src, inner_src, inner_dst);
	++_pimstat_rx_register_not_rp;
	return (XORP_OK);
    }
    
    lookup_flags = PIM_MRE_RP | PIM_MRE_WC | PIM_MRE_SG | PIM_MRE_SG_RPT;
    pim_mre = pim_node().pim_mrt().pim_mre_find(inner_src, inner_dst,
						lookup_flags, 0);
    
    //
    // Test if I am the RP for the multicast group
    //
    if ((pim_mre == NULL)
	|| (pim_mre->rp_addr_ptr() == NULL)
	|| (! pim_mre->i_am_rp())
	|| (dst != *pim_mre->rp_addr_ptr())) {
	//
	// send Register-Stop(S,G) to outer.src
	//
	pim_register_stop_send(src, inner_src, inner_dst);
	++_pimstat_rx_register_not_rp;
	return (XORP_OK);
    }
    
    //
    // I am the RP for the multicast group
    //
    
    //
    // Get the (S,G) entry if exists
    //
    pim_mre_sg = NULL;
    do {
	if (pim_mre->is_sg()) {
	    pim_mre_sg = pim_mre;
	    break;
	}
	if (pim_mre->is_sg_rpt()) {
	    pim_mre_sg = pim_mre->sg_entry();
	    if (pim_mre_sg != NULL)
		break;
	}
    } while (false);
    
    is_sptbit_set = false;
    if ((pim_mre_sg != NULL) && pim_mre_sg->is_spt())
	is_sptbit_set = true;

    //
    // The code below implements the core logic inside
    // packet_arrives_on_rp_tunnel()
    // Note that we call is_switch_to_spt_desired_sg() with a time interval
    // of zero seconds and a threshold of zero bytes.
    // I.e., this check will evaluate to true if the SPT switch is enabled
    // and the threshold for the switch is 0 bytes (i.e., switch immediately).
    //
    sent_register_stop = false;
    if (is_sptbit_set
	|| (pim_mre->is_switch_to_spt_desired_sg(0, 0)
	    && pim_mre->inherited_olist_sg().none())) {
	//
	// send Register-Stop(S,G) to outer.src
	//
	pim_register_stop_send(src, inner_src, inner_dst);
	sent_register_stop = true;
    }
    if (is_sptbit_set || pim_mre->is_switch_to_spt_desired_sg(0, 0)) {
	if (sent_register_stop) {
	    // restart KeepaliveTimer(S,G) to Keepalive_Period
	    keepalive_timer_sec = PIM_KEEPALIVE_PERIOD_DEFAULT;
	} else {
	    // restart KeepaliveTimer(S,G) to RP_Keepalive_Period
	    keepalive_timer_sec = PIM_RP_KEEPALIVE_PERIOD_DEFAULT;
	}
	// Create an (S,G) entry that will keep the Keepalive Timer running
	if (pim_mre_sg == NULL) {
	    pim_mre_sg = pim_node().pim_mrt().pim_mre_find(inner_src,
							   inner_dst,
							   PIM_MRE_SG,
							   PIM_MRE_SG);
	}
	pim_mre_sg->start_keepalive_timer();
	is_keepalive_timer_restarted = true;
    }
    if ((! is_sptbit_set) && (! null_register_bool)) {
	    //
	    // decapsulate and pass the inner packet to the normal
	    // forwarding path for forwarding on the (*,G) tree.
	    // XXX: will happen at the kernel
	    //
    }
    
    pim_mfc = pim_node().pim_mrt().pim_mfc_find(inner_src, inner_dst, false);
    
    if (pim_mre_sg == NULL) {
	//
	// If necessary, add a dataflow monitor to monitor whether it is
	// time to switch to the SPT.
	//
	if (pim_mfc == NULL)
	    return (XORP_OK);
	if (pim_node().is_switch_to_spt_enabled().get()
	    && (! pim_mfc->has_spt_switch_dataflow_monitor())) {
	    uint32_t sec = pim_node().switch_to_spt_threshold_interval_sec().get();
	    uint32_t bytes = pim_node().switch_to_spt_threshold_bytes().get();
	    pim_mfc->add_dataflow_monitor(sec, 0,
					  0,		// threshold_packets
					  bytes,	// threshold_bytes
					  false, // is_threshold_in_packets
					  true,	 // is_threshold_in_bytes
					  true,		// is_geq_upcall ">="
					  false);	// is_leq_upcall "<="
	}
	
	return (XORP_OK);
    }
    
    if (pim_mfc == NULL) {
	pim_mfc = pim_node().pim_mrt().pim_mfc_find(inner_src, inner_dst, true);
	pim_mfc->set_iif_vif_index(register_vif_index);
	pim_mfc->set_olist(pim_mre->inherited_olist_sg());
	pim_mfc->add_mfc_to_kernel();
    }
    if (is_keepalive_timer_restarted
	|| (! pim_mfc->has_idle_dataflow_monitor())) {
	//
	// Add a dataflow monitor to expire idle (S,G) PimMre state
	// and/or idle PimMfc+MFC state
	//
	pim_mfc->add_dataflow_monitor(keepalive_timer_sec, 0,
				      0,	// threshold_packets
				      0,	// threshold_bytes
				      true,	// is_threshold_in_packets
				      false,	// is_threshold_in_bytes
				      false,	// is_geq_upcall ">="
				      true);	// is_leq_upcall "<="
    }

    return (XORP_OK);
    
    UNUSED(pim_nbr);
    
    // Various error processing
 rcvlen_error:
    XLOG_WARNING("RX %s from %s to %s: "
		 "invalid message length",
		 PIMTYPE2ASCII(PIM_REGISTER),
		 cstring(src), cstring(dst));
    ++_pimstat_rx_malformed_packet;
    return (XORP_ERROR);
}

int
PimVif::pim_register_send(const IPvX& rp_addr,
			  const IPvX& ,		// source_addr,
			  const IPvX& ,		// group_addr,
			  const uint8_t *rcvbuf,
			  size_t rcvlen)
{
    const struct ip *ip4 = (const struct ip *)rcvbuf;
    buffer_t *buffer;
    uint32_t flags = 0;
    IPvX from(family()), to(family());
    size_t mtu = 0;
    
    if (ip4->ip_v != from.ip_version()) {
	XLOG_WARNING("Cannot encapsulate IP packet: "
		     "inner IP version (%d) != expected IP version (%d)",
		     ip4->ip_v, from.ip_version());
	return (XORP_ERROR);
    }
    
    // TODO: XXX: PAVPAVPAV: if a border router, set the Border-bit to flags
    
    //
    // Get the inner source and destination addreses
    //
    switch (family()) {
    case AF_INET:
    {
	from.copy_in(ip4->ip_src);
	to.copy_in(ip4->ip_dst);
	// Compute the MTU
	mtu = 0xffff			// IPv4 max packet size
	    - (0xf << 2)		// IPv4 max header size
	    - sizeof(struct pim)
	    - sizeof(uint32_t);
	break;
    }
    
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	const struct ip6_hdr *ip6 = (const struct ip6_hdr *)rcvbuf;
	
	from.copy_in(ip6->ip6_src);
	to.copy_in(ip6->ip6_dst);
	// Compute the MTU
	mtu = 0xffff	      // IPv6 max payload size (jumbo payload excluded)
	    - sizeof(struct pim)
	    - sizeof(uint32_t);
	break;
    }
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    
    //
    // If the data packet is small enough, just send it
    //
    if (rcvlen <= mtu) {
	buffer = buffer_send_prepare();
	// Write all data to the buffer
	BUFFER_PUT_HOST_32(flags, buffer);
	// TODO: XXX: PAVPAVPAV: check the data length, and the inner IP pkt.
	BUFFER_PUT_DATA(rcvbuf, buffer, rcvlen);
	
	return (pim_send(rp_addr, PIM_REGISTER, buffer));
    }
    
    //
    // Fragment the inner packet, then encapsulate and send each fragment
    //
    if (family() == AF_INET) {
	uint8_t frag_buf[rcvlen];	// The buffer for the fragments
	struct ip *frag_ip4 = (struct ip *)frag_buf;
	size_t frag_optlen = 0;		// The optlen to copy into fragments
	size_t frag_ip_hl;
	
	if (ntohs(ip4->ip_off) & IP_DF) {
	    // Fragmentation is forbidded
	    XLOG_WARNING("Cannot fragment encapsulated IP packet "
			 "from %s to %s: "
			 "fragmentation not allowed",
			 cstring(from), cstring(to));
	    // XXX: we don't send ICMP "fragmentation needed" back to the
	    // sender, because in IPv4 ICMP error messages are not sent
	    // back for datagrams destinated to an multicast address.
	    return (XORP_ERROR);
	}
	if (((mtu - (ip4->ip_hl << 2)) & ~7) < 8) {
	    // Fragmentation is possible only if we can put at least
	    // 8 octets per fragment (except the last one)
	    XLOG_WARNING("Cannot fragment encapsulated IP packet "
			 "from %s to %s: "
			 "cannot send fragment with size less than 8 octets",
			 cstring(from), cstring(to));
	    return (XORP_ERROR);
	}
	
	//
	// Copy the IP header and the options that should be copied during
	// fragmentation.
	// XXX: the code below is taken from FreeBSD's ip_optcopy()
	// in netinet/ip_output.c
	//
	memcpy(frag_buf, rcvbuf, sizeof(struct ip));
	{
	    register const u_char *cp;
	    register u_char *dp;
	    int opt, optlen, cnt;
	    
	    cp = (const u_char *)(ip4 + 1);
	    dp = (u_char *)(frag_ip4 + 1);
	    cnt = (ip4->ip_hl << 2) - sizeof (struct ip);
	    for (; cnt > 0; cnt -= optlen, cp += optlen) {
                opt = cp[0];
                if (opt == IPOPT_EOL)
		    break;
                if (opt == IPOPT_NOP) {
		    // Preserve for IP mcast tunnel's LSRR alignment.
		    *dp++ = IPOPT_NOP;
		    optlen = 1;
		    continue;
                }
		
		//
		// Check for bogus lengths
		//
		if ((size_t)cnt < IPOPT_OLEN + sizeof(*cp)) {
		    XLOG_WARNING("Cannot fragment encapsulated IP packet "
				 "from %s to %s: "
				 "malformed IPv4 option",
				 cstring(from), cstring(to));
		    return (XORP_ERROR);
		}
                optlen = cp[IPOPT_OLEN];
		if (optlen < (int)(IPOPT_OLEN + sizeof(*cp)) || optlen > cnt) {
		    XLOG_WARNING("Cannot fragment encapsulated IP packet "
				 "from %s to %s: "
				 "malformed IPv4 option",
				 cstring(from), cstring(to));
		    return (XORP_ERROR);
		}
		
                if (optlen > cnt)
		    optlen = cnt;
                if (IPOPT_COPIED(opt)) {
		    memcpy(dp, cp, optlen);
		    dp += optlen;
                }
	    }
	    for (optlen = dp - (u_char *)(frag_ip4+1); optlen & 0x3; optlen++)
                *dp++ = IPOPT_EOL;
	    frag_optlen = optlen;	// return (optlen);
	}
	// The header length with the remaining IP options
	frag_ip_hl = (sizeof(struct ip) + frag_optlen);
	frag_ip4->ip_hl = frag_ip_hl >> 2;
	
	//
	// Do the fragmentation
	//
	size_t data_start = 0;
	size_t data_end = rcvlen;
	// Send the first segment with all the options
	{
	    uint8_t first_frag[mtu];
	    struct ip *first_ip = (struct ip *)first_frag;
	    size_t first_ip_hl = ip4->ip_hl << 2;
	    size_t nfb = (mtu - first_ip_hl) / 8;
	    size_t first_frag_len = first_ip_hl + nfb*8;
	    
	    memcpy(first_frag, rcvbuf, first_frag_len);
	    
	    // Correct the IP header
	    first_ip->ip_off = htons(ntohs(first_ip->ip_off) | IP_MF);
	    first_ip->ip_len = htons(first_frag_len);
	    first_ip->ip_sum = 0;
	    first_ip->ip_sum = INET_CKSUM(first_ip, first_ip_hl);
	    
	    // Encapsulate and send the fragment
	    buffer = buffer_send_prepare();
	    BUFFER_PUT_HOST_32(flags, buffer);
	    BUFFER_PUT_DATA(first_frag, buffer, first_frag_len);
	    pim_send(rp_addr, PIM_REGISTER, buffer);
	    
	    data_start += first_frag_len;
	}
	
	//
	// Create the remaining of the fragments
	//
	while (data_start < data_end) {
	    size_t nfb = (mtu - frag_ip_hl) / 8;
	    size_t frag_len = frag_ip_hl + nfb*8;
	    size_t frag_data_len = nfb*8;
	    bool is_last_fragment = false;
	    
	    // Compute the fragment length
	    if (data_end - data_start <= frag_data_len) {
		frag_data_len = data_end - data_start;
		frag_len = frag_ip_hl + frag_data_len;
		is_last_fragment = true;
	    }
	    
	    // Copy the data
	    memcpy(frag_buf + frag_ip_hl, rcvbuf + data_start, frag_data_len);
	    
	    // The IP packet total length
	    frag_ip4->ip_len = htons(frag_len);
	    
	    // The IP fragment flags and offset
	    {
		unsigned short ip_off_field = ntohs(ip4->ip_off);
		unsigned short frag_ip_off_flags = ip_off_field & ~IP_OFFMASK;
		unsigned short frag_ip_off = (ip_off_field & IP_OFFMASK);
		
		if (! is_last_fragment)
		    frag_ip_off_flags |= IP_MF;
		
		frag_ip_off += (data_start - (ip4->ip_hl << 2)) / 8;	// XXX
		frag_ip4->ip_off = htons(frag_ip_off_flags | frag_ip_off);
	    }
	    
	    frag_ip4->ip_sum = 0;
	    frag_ip4->ip_sum = INET_CKSUM(frag_ip4, frag_ip_hl);
	    
	    // Encapsulate and send the fragment
	    buffer = buffer_send_prepare();
	    BUFFER_PUT_HOST_32(flags, buffer);
	    BUFFER_PUT_DATA(frag_buf, buffer, frag_len);
	    pim_send(rp_addr, PIM_REGISTER, buffer);
	    
	    data_start += frag_data_len;
	}
    }
    
#ifdef HAVE_IPV6
    if (family() == AF_INET6) {
	// In IPv6 routers do not fragment packets that they forward.
	// Hence, just send ICMP back to the sender
	
	// TODO: XXX: PAVPAVPAV: send back ICMP "Packet Too Big"
    }
#endif // HAVE_IPV6
    
    return (XORP_OK);
    
 buflen_error:
    XLOG_UNREACHABLE();
    XLOG_ERROR("TX %s from %s to %s: "
	       "packet cannot fit into sending buffer",
	       PIMTYPE2ASCII(PIM_REGISTER),
	       cstring(domain_wide_addr()), cstring(rp_addr));
    return (XORP_ERROR);
}

int
PimVif::pim_register_null_send(const IPvX& rp_addr,
			       const IPvX& source_addr,
			       const IPvX& group_addr)
{
    buffer_t *buffer = buffer_send_prepare();
    uint32_t flags = 0;
    
    // Write all data to the buffer
    flags |= PIM_NULL_REGISTER;
    BUFFER_PUT_HOST_32(flags, buffer);
    
    // Create the dummy IP header and write it to the buffer
    switch (family()) {
    case AF_INET: {
	struct ip ip4_header;
	uint8_t *cp = (uint8_t *)&ip4_header;
	
	memset(&ip4_header, 0, sizeof(ip4_header));
	ip4_header.ip_v		= IPVERSION;
	ip4_header.ip_hl	= (sizeof(ip4_header) >> 2);
	ip4_header.ip_tos	= 0;
	ip4_header.ip_id	= 0;
	ip4_header.ip_off	= 0;
	ip4_header.ip_p		= IPPROTO_PIM;
	ip4_header.ip_len	= htons(sizeof(ip4_header));
	ip4_header.ip_ttl	= 0;
	source_addr.copy_out(ip4_header.ip_src);
	group_addr.copy_out(ip4_header.ip_dst);
	// XXX: on older Linux 'ip->ip_sum' was named 'ip->ip_csum'.
	// Later someone has realized that it is not a smart move,
	// so now Linux is in sync with the rest of the UNIX-es.
	// If you got one of those old Linux-es, you better upgrade to
	// something more decent. If you are stubborn and you don't want
	// to upgrade, then change 'ip_sum' to 'ip_csum' below, but
	// you should know that in the future you will get what you deserve :)
	ip4_header.ip_sum	= 0;
	ip4_header.ip_sum	= INET_CKSUM(&ip4_header, sizeof(ip4_header));
	
	BUFFER_PUT_DATA(cp, buffer, sizeof(ip4_header));
	break;
    }
    
#ifdef HAVE_IPV6    
    case AF_INET6: {
	//
	// First generate the dummy IPv6 header, and then the dummy PIM header
	//
	
	// Generate the dummy IPv6 header
	struct ip6_hdr ip6_header;
	uint8_t *cp = (uint8_t *)&ip6_header;
	
	// XXX: conditionally define IPv6 header related values
#ifndef IPV6_VERSION
#define IPV6_VERSION		0x60
#endif
#ifndef IPV6_VERSION_MASK
#define IPV6_VERSION_MASK	0xf0
#endif
	memset(&ip6_header, 0, sizeof(ip6_header));
	ip6_header.ip6_plen	= htons(sizeof(struct pim));
	ip6_header.ip6_flow	= 0;
	ip6_header.ip6_vfc	&= ~IPV6_VERSION_MASK;
	ip6_header.ip6_vfc	|= IPV6_VERSION;
	ip6_header.ip6_hlim	= 0;
	ip6_header.ip6_nxt	= IPPROTO_PIM;
	source_addr.copy_out(ip6_header.ip6_src);
	group_addr.copy_out(ip6_header.ip6_dst);
	
	BUFFER_PUT_DATA(cp, buffer, sizeof(ip6_header));
	
	// Generate the dummy PIM header
	uint16_t cksum, cksum2;
	struct pim pim_header;
	cp = (uint8_t *)&pim_header;
	
	memset(&pim_header, 0, sizeof(pim_header));
	cksum = INET_CKSUM(&pim_header, sizeof(pim_header));	// XXX: no-op
	cksum2 = calculate_ipv6_pseudo_header_checksum(source_addr,
						       group_addr,
						       sizeof(struct pim));
	cksum = INET_CKSUM_ADD(cksum, cksum2);
	pim_header.pim_cksum = cksum;
	
	BUFFER_PUT_DATA(cp, buffer, sizeof(pim_header));
	break;
    }
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	return (XORP_ERROR);
    }
    
    return (pim_send(rp_addr, PIM_REGISTER, buffer));
    
 buflen_error:
    XLOG_UNREACHABLE();
    XLOG_ERROR("TX %s%s from %s to %s: "
	       "packet cannot fit into sending buffer",
	       PIMTYPE2ASCII(PIM_REGISTER),
	       "(Null)",
	       cstring(domain_wide_addr()), cstring(rp_addr));
    return (XORP_ERROR);
}
