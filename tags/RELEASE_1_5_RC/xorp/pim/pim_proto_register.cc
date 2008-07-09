// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/pim/pim_proto_register.cc,v 1.37 2007/02/16 22:46:50 pavlin Exp $"


//
// PIM PIM_REGISTER messages processing.
//


#include "pim_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/ipvx.hh"

#include "libproto/checksum.h"
#include "libproto/packet.hh"

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
    bool is_null_register, is_border_register;
    IPvX inner_src(family()), inner_dst(family());
    uint32_t lookup_flags;
    PimMre *pim_mre, *pim_mre_sg;
    PimMfc *pim_mfc = NULL;
    bool is_sptbit_set = false;
    bool sent_register_stop = false;
    bool is_keepalive_timer_restarted = false;
    uint32_t keepalive_timer_sec = PIM_KEEPALIVE_PERIOD_DEFAULT;
    uint32_t register_vif_index = pim_node().pim_register_vif_index();
    string dummy_error_msg;
    
    //
    // Parse the message
    //
    BUFFER_GET_HOST_32(pim_register_flags, buffer);
    // The Border bit and the Null-Register bit
    if (pim_register_flags & PIM_BORDER_REGISTER)
	is_border_register = true;
    else
	is_border_register = false;
    if (pim_register_flags & PIM_NULL_REGISTER)
	is_null_register = true;
    else
	is_null_register = false;
    
    //
    // Get the inner source and destination addresses
    //
    switch (family()) {
    case AF_INET: {
	uint8_t ip_header4_buffer[IpHeader4::SIZE];
	IpHeader4 ip4(ip_header4_buffer);
	
	BUFFER_GET_DATA(ip_header4_buffer, buffer, IpHeader4::SIZE);
	inner_src = IPvX(ip4.ip_src());
	inner_dst = IPvX(ip4.ip_dst());
	if (is_null_register) {
	    //
	    // If the inner header checksum is non-zero, then
	    // check the checksum.
	    //
	    if (ip4.ip_sum() != 0) {
		uint16_t cksum = inet_checksum(ip4.data(), ip4.size());
		if (cksum != 0) {
		    XLOG_WARNING("RX %s%s from %s to %s: "
				 "inner dummy IP header checksum error",
				 PIMTYPE2ASCII(PIM_REGISTER),
				 (is_null_register)? "(Null)" : "",
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
	uint8_t ip_header6_buffer[IpHeader6::SIZE];
	IpHeader6 ip6(ip_header6_buffer);
	uint16_t inner_data_len;
	
	BUFFER_GET_DATA(ip_header6_buffer, buffer, IpHeader6::SIZE);
	inner_src = IPvX(ip6.ip_src());
	inner_dst = IPvX(ip6.ip_dst());
	inner_data_len = ip6.ip_plen();
	if (is_null_register) {
	    //
	    // If the dummy PIM header is present, then
	    // check the checksum.
	    //
	    if (inner_data_len == sizeof(struct pim)) {
		struct pim pim_header;
		uint8_t *cp = (uint8_t *)&pim_header;
		BUFFER_GET_DATA(cp, buffer, sizeof(pim_header));
		uint16_t cksum, cksum2;
		cksum = inet_checksum(
		    reinterpret_cast<const uint8_t *>(&pim_header),
		    sizeof(pim_header));
		cksum2 = calculate_ipv6_pseudo_header_checksum(inner_src,
							       inner_dst,
							       sizeof(struct pim),
							       IPPROTO_PIM);
		cksum = inet_checksum_add(cksum, cksum2);
		if (cksum != 0) {
		    XLOG_WARNING("RX %s%s from %s to %s: "
				 "inner dummy IP header checksum error",
				 PIMTYPE2ASCII(PIM_REGISTER),
				 (is_null_register)? "(Null)" : "",
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
		     (is_null_register)? "(Null)" : "",
		     cstring(src), cstring(dst),
		     cstring(inner_src));
	return (XORP_ERROR);
    }
    if (! inner_dst.is_multicast()) {
	XLOG_WARNING("RX %s%s from %s to %s: "
		     "inner destination address = %s must be multicast",
		     PIMTYPE2ASCII(PIM_REGISTER),
		     (is_null_register)? "(Null)" : "",
		     cstring(src), cstring(dst),
		     cstring(inner_dst));
	return (XORP_ERROR);
    }
    if (inner_dst.is_linklocal_multicast()
	|| inner_dst.is_interfacelocal_multicast()) {
	XLOG_WARNING("RX %s%s from %s to %s: "
		     "inner destination address = %s must not be "
		     "link or interface-local multicast group",
		     PIMTYPE2ASCII(PIM_REGISTER),
		     (is_null_register)? "(Null)" : "",
		     cstring(src), cstring(dst),
		     cstring(inner_dst));
	return (XORP_ERROR);
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
	// "send Register-Stop(S,G) to outer.src"
	//
	pim_register_stop_send(src, inner_src, inner_dst, dummy_error_msg);
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
	// "send Register-Stop(S,G) to outer.src"
	//
	pim_register_stop_send(src, inner_src, inner_dst, dummy_error_msg);
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
    //
    sent_register_stop = false;
    if (is_border_register) {
	if ((pim_mre_sg != NULL)
	    && pim_mre_sg->is_pmbr_addr_set()
	    && (pim_mre_sg->pmbr_addr() != src)) {
	    //
	    // "send Register-Stop(S,G) to outer.src
	    // drop the packet silently."
	    //
	    pim_register_stop_send(src, inner_src, inner_dst, dummy_error_msg);
	    return (XORP_OK);
	}
    }
    //
    // Note that below we call is_switch_to_spt_desired_sg() with a time
    // interval of zero seconds and a threshold of zero bytes.
    // I.e., this check will evaluate to true if the SPT switch is enabled
    // and the threshold for the switch is 0 bytes (i.e., switch
    // immediately).
    //
    // If the configured threshold is larger, we are anyway going to
    // install a bandwidth monitor so that monitor will take care of
    // triggering the Register-Stop(S,G) if the bandwidth of the Register
    // messages reaches the configured threshold.
    //
    if (is_sptbit_set
	|| (pim_mre->is_switch_to_spt_desired_sg(0, 0)
	    && pim_mre->inherited_olist_sg().none())) {
	//
	// "send Register-Stop(S,G) to outer.src"
	//
	pim_register_stop_send(src, inner_src, inner_dst, dummy_error_msg);
	sent_register_stop = true;
    }
    if (is_sptbit_set || pim_mre->is_switch_to_spt_desired_sg(0, 0)) {
	// Create an (S,G) entry that will keep the Keepalive Timer running
	if (pim_mre_sg == NULL) {
	    pim_mre_sg = pim_node().pim_mrt().pim_mre_find(inner_src,
							   inner_dst,
							   PIM_MRE_SG,
							   PIM_MRE_SG);
	}
	if (sent_register_stop) {
	    // "restart KeepaliveTimer(S,G) to RP_Keepalive_Period"
	    keepalive_timer_sec = PIM_RP_KEEPALIVE_PERIOD_DEFAULT;
	    pim_mre_sg->set_is_kat_set_to_rp_keepalive_period(true);
	} else {
	    // "restart KeepaliveTimer(S,G) to Keepalive_Period"
	    keepalive_timer_sec = PIM_KEEPALIVE_PERIOD_DEFAULT;
	    pim_mre_sg->set_is_kat_set_to_rp_keepalive_period(false);
	}
	pim_mre_sg->start_keepalive_timer();
	is_keepalive_timer_restarted = true;
    }
    if (is_border_register) {
	//
	// Update the PMBR address
	//
	if (pim_mre_sg != NULL) {
	    if (! pim_mre_sg->is_pmbr_addr_set())
		pim_mre_sg->set_pmbr_addr(src);
	}
    }
    if ((! is_sptbit_set) && (! is_null_register)) {
	//
	// "decapsulate and forward the inner packet to
	// inherited_olist(S,G,rpt)"
	//
	// XXX: This will happen inside the kernel.
	// Here we only install the forwarding entry.
	//
	pim_mfc = pim_node().pim_mrt().pim_mfc_find(inner_src, inner_dst,
						    false);
	if (pim_mfc == NULL) {
	    pim_mfc = pim_node().pim_mrt().pim_mfc_find(inner_src, inner_dst,
							true);
	    pim_mfc->update_mfc(register_vif_index,
				pim_mre->inherited_olist_sg_rpt(),
				pim_mre_sg);
	}
    }

    //
    // Restart KeepaliveTimer(S,G)
    //
    if (pim_mre_sg != NULL) {
	if (is_keepalive_timer_restarted
	    || ((pim_mfc != NULL)
		&& (! pim_mfc->has_idle_dataflow_monitor()))) {
	    if (pim_mfc == NULL) {
		pim_mfc = pim_node().pim_mrt().pim_mfc_find(inner_src,
							    inner_dst,
							    true);
		if (is_sptbit_set) {
		    pim_mfc->update_mfc(pim_mre_sg->rpf_interface_s(),
					pim_mre->inherited_olist_sg(),
					pim_mre_sg);
		} else {
		    pim_mfc->update_mfc(register_vif_index,
					pim_mre->inherited_olist_sg_rpt(),
					pim_mre_sg);
		}
	    }
	    //
	    // Add a dataflow monitor to expire idle (S,G) PimMre state
	    // and/or idle PimMfc+MFC state
	    //
	    if (! pim_mfc->has_idle_dataflow_monitor()) {
		pim_mfc->add_dataflow_monitor(keepalive_timer_sec, 0,
					      0,	// threshold_packets
					      0,	// threshold_bytes
					      true,	// is_threshold_in_packets
					      false,	// is_threshold_in_bytes
					      false,	// is_geq_upcall ">="
					      true);	// is_leq_upcall "<="
	    }
	}
	return (XORP_OK);
    }

    //
    // If necessary, add a dataflow monitor to monitor whether it is
    // time to switch to the SPT.
    //
    if (pim_mfc == NULL) {
	pim_mfc = pim_node().pim_mrt().pim_mfc_find(inner_src, inner_dst,
						    false);
    }
    if (pim_mfc == NULL)
	return (XORP_OK);
    if (pim_node().is_switch_to_spt_enabled().get()
	&& (pim_mre != NULL)
	&& (pim_mre->is_monitoring_switch_to_spt_desired_sg(pim_mre_sg))
	&& (! pim_mfc->has_spt_switch_dataflow_monitor())) {
	// Install the monitor
	uint32_t sec = pim_node().switch_to_spt_threshold_interval_sec().get();
	uint32_t bytes = pim_node().switch_to_spt_threshold_bytes().get();
	pim_mfc->add_dataflow_monitor(sec, 0,
				      0,	// threshold_packets
				      bytes,	// threshold_bytes
				      false,	// is_threshold_in_packets
				      true,	// is_threshold_in_bytes
				      true,	// is_geq_upcall ">="
				      false);	// is_leq_upcall "<="
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
			  const IPvX& source_addr,
			  const IPvX& group_addr,
			  const uint8_t *rcvbuf,
			  size_t rcvlen,
			  string& error_msg)
{
    IpHeader4 ip4(rcvbuf);
    buffer_t *buffer;
    uint32_t flags = 0;
    size_t mtu = 0;
    string dummy_error_msg;
    
    UNUSED(group_addr);

    if (ip4.ip_version() != source_addr.ip_version()) {
	error_msg = c_format("Cannot encapsulate IP packet: "
			     "inner IP version (%u) != expected IP version (%u)",
			     XORP_UINT_CAST(ip4.ip_version()),
			     XORP_UINT_CAST(source_addr.ip_version()));
	XLOG_WARNING("%s", error_msg.c_str());
	return (XORP_ERROR);
    }
    
    // TODO: XXX: PAVPAVPAV: if a border router, set the Border-bit to flags
    
    //
    // Calculate the MTU.
    //
    // Note that we handle only the case when the encapsulated
    // packet size will become larger than the maximum packet size.
    //
    switch (family()) {
    case AF_INET:
    {
	mtu = 0xffff			// IPv4 max packet size
	    - (0xf << 2)		// IPv4 max header size
	    - sizeof(struct pim)
	    - sizeof(uint32_t);
	break;
    }
    
#ifdef HAVE_IPV6
    case AF_INET6:
    {
	mtu = 0xffff	      // IPv6 max payload size (jumbo payload excluded)
	    - sizeof(struct pim)
	    - sizeof(uint32_t);
	break;
    }
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	error_msg = c_format("Invalid address family: %d", family());
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
	
	return (pim_send(domain_wide_addr(), rp_addr, PIM_REGISTER, buffer,
			 error_msg));
    }
    
    //
    // Fragment the inner packet, then encapsulate and send each fragment
    //
    if (family() == AF_INET) {
	list<vector<uint8_t> > fragments;
	list<vector<uint8_t> >::iterator iter;

	if (ip4.fragment(mtu, fragments, true, error_msg) != XORP_OK) {
	    //
	    // XXX: If fragmentation is forbidded, we don't send
	    // ICMP "fragmentation needed" back to the sender, because in
	    // IPv4 ICMP error messages are not sent back for datagrams
	    // destinated to an multicast address.
	    //
	    return (XORP_ERROR);
	}

	//
	// XXX: we already checked that the data packet is larger than
	// the MTU, so the packet must have been fragmented.
	//
	XLOG_ASSERT(! fragments.empty());

	// Encapsulate and send the fragments
	for (iter = fragments.begin(); iter != fragments.end(); ++iter) {
	    vector<uint8_t>& ip_fragment = *iter;

	    buffer = buffer_send_prepare();
	    BUFFER_PUT_HOST_32(flags, buffer);
	    BUFFER_PUT_DATA(&ip_fragment[0], buffer, ip_fragment.size());
	    pim_send(domain_wide_addr(), rp_addr, PIM_REGISTER, buffer,
		     dummy_error_msg);
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
    error_msg = c_format("TX %s from %s to %s: "
			 "packet cannot fit into sending buffer",
			 PIMTYPE2ASCII(PIM_REGISTER),
			 cstring(domain_wide_addr()), cstring(rp_addr));
    XLOG_ERROR("%s", error_msg.c_str());
    return (XORP_ERROR);
}

int
PimVif::pim_register_null_send(const IPvX& rp_addr,
			       const IPvX& source_addr,
			       const IPvX& group_addr,
			       string& error_msg)
{
    buffer_t *buffer = buffer_send_prepare();
    uint32_t flags = 0;

    // Write all data to the buffer
    flags |= PIM_NULL_REGISTER;
    BUFFER_PUT_HOST_32(flags, buffer);
    
    // Create the dummy IP header and write it to the buffer
    switch (family()) {
    case AF_INET: {
	uint8_t ip_header4_buffer[IpHeader4::SIZE];
	memset(ip_header4_buffer, 0, sizeof(ip_header4_buffer));

	IpHeader4Writer ip4(ip_header4_buffer);
	
	ip4.set_ip_version(IpHeader4::IP_VERSION);
	ip4.set_ip_header_len(IpHeader4::SIZE);
	ip4.set_ip_tos(0);
	ip4.set_ip_id(0);
	ip4.set_ip_off(0);
	ip4.set_ip_p(IPPROTO_PIM);
	ip4.set_ip_len(IpHeader4::SIZE);
	ip4.set_ip_ttl(0);
	ip4.set_ip_src(source_addr.get_ipv4());
	ip4.set_ip_dst(group_addr.get_ipv4());
	ip4.set_ip_sum(0);
	ip4.set_ip_sum(ntohs(inet_checksum(ip4.data(), ip4.size())));
	
	BUFFER_PUT_DATA(ip_header4_buffer, buffer, IpHeader4::SIZE);
	break;
    }
    
#ifdef HAVE_IPV6    
    case AF_INET6: {
	//
	// First generate the dummy IPv6 header, and then the dummy PIM header
	//
	uint8_t ip_header6_buffer[IpHeader6::SIZE];
	memset(ip_header6_buffer, 0, sizeof(ip_header6_buffer));

	// Generate the dummy IPv6 header
	IpHeader6Writer ip6(ip_header6_buffer);

	ip6.set_ip_vtc_flow(0);
	ip6.set_ip_version(IpHeader6::IP_VERSION);
	ip6.set_ip_plen(sizeof(struct pim));
	ip6.set_ip_hlim(0);
	ip6.set_ip_nxt(IPPROTO_PIM);
	ip6.set_ip_src(source_addr.get_ipv6());
	ip6.set_ip_dst(group_addr.get_ipv6());
	
	BUFFER_PUT_DATA(ip_header6_buffer, buffer, IpHeader6::SIZE);
	
	// Generate the dummy PIM header
	uint16_t cksum, cksum2;
	struct pim pim_header;
	uint8_t *cp = (uint8_t *)&pim_header;
	
	memset(&pim_header, 0, sizeof(pim_header));
	cksum = inet_checksum(reinterpret_cast<const uint8_t *>(&pim_header),
			      sizeof(pim_header));	// XXX: no-op
	cksum2 = calculate_ipv6_pseudo_header_checksum(source_addr,
						       group_addr,
						       sizeof(struct pim),
						       IPPROTO_PIM);
	cksum = inet_checksum_add(cksum, cksum2);
	pim_header.pim_cksum = cksum;
	
	BUFFER_PUT_DATA(cp, buffer, sizeof(pim_header));
	break;
    }
#endif // HAVE_IPV6
    
    default:
	XLOG_UNREACHABLE();
	error_msg = c_format("Invalid address family: %d", family());
	return (XORP_ERROR);
    }
    
    return (pim_send(domain_wide_addr(), rp_addr, PIM_REGISTER, buffer,
		     error_msg));
    
 buflen_error:
    XLOG_UNREACHABLE();
    error_msg = c_format("TX %s%s from %s to %s: "
			 "packet cannot fit into sending buffer",
			 PIMTYPE2ASCII(PIM_REGISTER),
			 "(Null)",
			 cstring(domain_wide_addr()), cstring(rp_addr));
    XLOG_ERROR("%s", error_msg.c_str());
    return (XORP_ERROR);
}
