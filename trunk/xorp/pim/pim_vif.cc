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

#ident "$XORP: xorp/pim/pim_vif.cc,v 1.3 2003/01/07 01:43:03 pavlin Exp $"


//
// PIM virtual interfaces implementation.
//


#include "pim_module.h"
#include "pim_private.hh"
#include "mrt/inet_cksum.h"
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
 * PimVif::PimVif:
 * @pim_node: The PIM node this interface belongs to.
 * @vif: The generic Vif interface that contains various information.
 * 
 * PIM protocol vif constructor.
 **/
PimVif::PimVif(PimNode& pim_node, const Vif& vif)
    : ProtoUnit(pim_node.family(), pim_node.module_id()),
      Vif(vif),
      _pim_node(pim_node),
      _dr_addr(pim_node.family()),
      _pim_nbr_me(*this, IPvX::ZERO(pim_node.family()), PIM_VERSION_DEFAULT),
      _hello_triggered_delay(PIM_HELLO_HELLO_TRIGGERED_DELAY_DEFAULT),
      _hello_period(PIM_HELLO_HELLO_PERIOD_DEFAULT,
		    callback(this, &PimVif::set_hello_period_callback)),
      _hello_holdtime(PIM_HELLO_HELLO_HOLDTIME_DEFAULT,
		      callback(this, &PimVif::set_hello_holdtime_callback)),
      _genid(RANDOM(0xffffffffU),
	     callback(this, &PimVif::set_genid_callback)),
      _dr_priority(PIM_HELLO_DR_ELECTION_PRIORITY_DEFAULT,
		   callback(this, &PimVif::set_dr_priority_callback)),
      _lan_delay(LAN_DELAY_MSEC_DEFAULT,
		 callback(this, &PimVif::set_lan_delay_callback)),
      _override_interval(LAN_OVERRIDE_INTERVAL_MSEC_DEFAULT,
			 callback(this,
				  &PimVif::set_override_interval_callback)),
      _is_tracking_support_disabled(false,
				    callback(this,
					     &PimVif::set_is_tracking_support_disabled_callback)),
      _accept_nohello_neighbors(false),
      _join_prune_period(PIM_JOIN_PRUNE_PERIOD_DEFAULT,
			 callback(this,
				  &PimVif::set_join_prune_period_callback)),
      _join_prune_holdtime(PIM_JOIN_PRUNE_HOLDTIME_DEFAULT),
      _assert_time(PIM_ASSERT_ASSERT_TIME_DEFAULT),
      _assert_override_interval(PIM_ASSERT_ASSERT_OVERRIDE_INTERVAL_DEFAULT),
      _usage_by_pim_mre_task(0)
{
    _buffer_send = BUFFER_MALLOC(BUF_SIZE_DEFAULT);
    _buffer_send_hello = BUFFER_MALLOC(BUF_SIZE_DEFAULT);
    _buffer_send_bootstrap = BUFFER_MALLOC(BUF_SIZE_DEFAULT);
    _proto_flags = 0;
    
    set_proto_version_default(PIM_VERSION_DEFAULT);
    
    set_default_config();
    
    set_should_send_pim_hello(true);
}

/**
 * PimVif::~PimVif:
 * @void: 
 * 
 * PIM protocol vif destructor.
 * 
 **/
PimVif::~PimVif(void)
{
    stop();
    
    BUFFER_FREE(_buffer_send);
    BUFFER_FREE(_buffer_send_hello);
    BUFFER_FREE(_buffer_send_bootstrap);
    
    // Remove all PIM neighbor entries
    while (! _pim_nbrs.empty()) {
	PimNbr *pim_nbr = _pim_nbrs.front();
	_pim_nbrs.pop_front();
	// TODO: perform the appropriate actions
	delete_pim_nbr(pim_nbr);
    }
}

/**
 * PimVif::pim_mrt:
 * @: 
 * 
 * Get the PIM Multicast Routing Table.
 * 
 * Return value: A reference to the PIM Multicast Routing Table.
 **/
PimMrt&
PimVif::pim_mrt() const
{
    return (_pim_node.pim_mrt());
}

/**
 * PimVif::set_default_config:
 * @: 
 * 
 * Set configuration to default values.
 **/
void
PimVif::set_default_config()
{
    // Protocol version
    set_proto_version(proto_version_default());
    
    // Hello-related parameters
    hello_triggered_delay().reset();
    hello_period().reset();
    hello_holdtime().reset();
    genid().set(RANDOM(0xffffffffU));
    dr_priority().reset();
    lan_delay().reset();
    override_interval().reset();
    is_tracking_support_disabled().reset();
    accept_nohello_neighbors().reset();
    
    // Join/Prune-related parameters
    _join_prune_period.reset();
    _join_prune_holdtime.reset();
}

/**
 * PimVif::set_proto_version:
 * @proto_version: The protocol version to set.
 * 
 * Set protocol version.
 * 
 * Return value: %XORP_OK is @proto_version is valid, otherwise %XORP_ERROR.
 **/
int
PimVif::set_proto_version(int proto_version)
{
    if ((proto_version < PIM_VERSION_MIN) || (proto_version > PIM_VERSION_MAX))
	return (XORP_ERROR);
    
    ProtoUnit::set_proto_version(proto_version);
    
    return (XORP_OK);
}

/**
 * PimVif::start:
 * @void: 
 * 
 * Start PIM on a single virtual interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimVif::start(void)
{
    if (! is_underlying_vif_up())
	return (XORP_ERROR);
    if (is_loopback())
	return (XORP_ERROR);
    if (! (is_multicast_capable() || is_pim_register()))
	return (XORP_ERROR);
    if (addr_ptr() == NULL)
	return (XORP_ERROR);
    if (! addr_ptr()->is_unicast())
	return (XORP_ERROR);
    
    if (ProtoUnit::start() < 0)
	return (XORP_ERROR);
    
    pim_nbr_me().set_addr(*addr_ptr());		// Set my PimNbr address
    
    //
    // Start the vif with the kernel
    //
    if (pim_node().start_protocol_kernel_vif(vif_index()) != XORP_OK) {
	XLOG_ERROR("Error starting protocol vif %s with the kernel",
		   name().c_str());
	return (XORP_ERROR);
    }
    
    XLOG_INFO("STARTING %s%s",
	      this->str().c_str(), flags_string().c_str());
    
    if (! is_pim_register()) {    
	//
	// Join the appropriate multicast groups: ALL-PIM-ROUTERS
	//
	const IPvX group1 = IPvX::PIM_ROUTERS(family());
	if (pim_node().join_multicast_group(vif_index(), group1) != XORP_OK) {
	    XLOG_ERROR("Error joining group %s on vif %s",
		       cstring(group1), name().c_str());
	    return (XORP_ERROR);
	}
	
	pim_hello_start();
	
	//
	// Add MLD6/IGMP membership tracking
	//
	pim_node().add_protocol_mld6igmp(vif_index());
    }
    
    //
    // Add the task to take care of the PimMre processing
    //
    pim_node().pim_mrt().add_task_start_vif(vif_index());
    
    return (XORP_OK);
}

/**
 * PimVif::stop:
 * @void: 
 * 
 * Gracefully stop PIM on a single virtual interface.
 * XXX: The graceful stop will attempt to send Join/Prune, Assert, etc.
 * messages for all multicast routing entries to gracefully clean-up
 * state with neighbors.
 * XXX: After the multicast routing entries cleanup is completed,
 * PimVif::final_stop() is called to complete the job.
 * XXX: If this method is called one-after-another, the second one
 * will force calling immediately PimVif::final_stop() to quickly
 * finish the job. TODO: is this a desired semantic?
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimVif::stop(void)
{
    int ret_value = XORP_OK;
    
    if (! (is_up() || is_pending_up() || is_pending_down()))
	return (XORP_ERROR);
    
    //
    // Add the task to take care of the PimMre processing
    //
    if (is_up()) {
	pim_node().pim_mrt().add_task_stop_vif(vif_index());
    }
    if ((_usage_by_pim_mre_task == 0)
	|| is_pending_up()
	|| is_pending_down()) {
	ret_value = final_stop();
	return (ret_value);
    }
    
    if (! is_pim_register()) {
	//
	// Delete MLD6/IGMP membership tracking
	//
	pim_node().delete_protocol_mld6igmp(vif_index());
	
	// XXX: if the DR-priority Hello option is not in use, then we cannot
	// stop ourselves being the DR. In that case, there could be a
	// period of "missing-DR" until we completely stop this
	// vif (if it was the DR).
	pim_hello_stop_dr();
	
	set_i_am_dr(false);
    }
    
    if (ProtoUnit::pending_stop() < 0)
	ret_value = XORP_ERROR;
    
    _dr_addr = IPvX::ZERO(family());
    
    XLOG_INFO("PENDING STOP %s%s",
	      this->str().c_str(), flags_string().c_str());
    
    return (ret_value);
}

/**
 * PimVif::final_stop:
 * @void: 
 * 
 * Completely stop PIM on a single virtual interface.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimVif::final_stop(void)
{
    int ret_value = XORP_OK;
    
    if (! (is_up() || is_pending_up() || is_pending_down()))
	return (XORP_ERROR);
    
    if (! is_pim_register()) {
	//
	// Delete MLD6/IGMP membership tracking
	//
	if (is_up() || is_pending_up())
	    pim_node().delete_protocol_mld6igmp(vif_index());
	
	pim_hello_stop();
	
	set_i_am_dr(false);
    }
    
    if (ProtoUnit::stop() < 0)
	ret_value = XORP_ERROR;
    
    _dr_addr = IPvX::ZERO(family());
    _hello_timer.cancel();
    _hello_once_timer.cancel();
    
    // Remove all PIM neighbor entries
    while (! _pim_nbrs.empty()) {
	PimNbr *pim_nbr = _pim_nbrs.front();
	_pim_nbrs.pop_front();
	// TODO: perform the appropriate actions
	delete_pim_nbr(pim_nbr);
    }
    
    //
    // Stop the vif with the kernel
    //
    if (pim_node().stop_protocol_kernel_vif(vif_index()) != XORP_OK) {
	XLOG_ERROR("Error stopping protocol vif %s with the kernel",
		   name().c_str());
	return (XORP_ERROR);
    }
    
    XLOG_INFO("STOPPED %s%s",
	      this->str().c_str(), flags_string().c_str());
    
    //
    // Test if time to completely stop the PimNode itself because of this vif
    //
    if (pim_node().is_pending_down()
	&& (! pim_node().has_pending_down_units())) {
	pim_node().final_stop();
    }
    
    return (ret_value);
}

/**
 * PimVif::pim_send:
 * @dst: The message destination address.
 * @message_type: The PIM type of the message.
 * @buffer: The buffer with the rest of the message.
 * 
 * Send PIM message.
 * XXX: The beginning of the @buffer must have been reserved
 * for the PIM header.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimVif::pim_send(const IPvX& dst,
		 uint8_t message_type,
		 buffer_t *buffer)
{
    uint8_t pim_vt;
    uint16_t cksum;
    int ip_tos = -1;
    int ret_value;
    size_t datalen;
    
    if (! (is_up() || is_pending_down()))
	return (XORP_ERROR);
    
    //
    // If necessary, send first a Hello message
    //
    if (should_send_pim_hello()) {
	switch (message_type) {
	case PIM_JOIN_PRUNE:
	case PIM_BOOTSTRAP:		// XXX: not in the spec yet
	case PIM_ASSERT:
	    pim_hello_first_send();
	    break;
	default:
	    break;
	}
    }
    
    //
    // Compute the TOS
    //
    switch (message_type) {
    case PIM_REGISTER:
	// If PIM Register, copy the TOS from the inner header
	// to the outer header. Strictly speaking, we need to do it
	// only for Registers with data (i.e., not for Null Registers),
	// but for simplicity we do it for Null Registers as well.
	switch (family()) {
	case AF_INET:
	{
	    struct ip ip4_header;
	    
	    BUFFER_COPYGET_DATA_OFFSET(&ip4_header, buffer, sizeof(uint32_t),
				       sizeof(ip4_header));
	    ip_tos = ip4_header.ip_tos;
	    break;
	}
#ifdef HAVE_IPV6
	case AF_INET6:
	{
	    struct ip6_hdr ip6_header;
	    
	    BUFFER_COPYGET_DATA_OFFSET(&ip6_header, buffer, sizeof(uint32_t),
				       sizeof(ip6_header));
	    // Get the Traffic Class
	    ip_tos = (ntohl(ip6_header.ip6_flow) >> 20) & 0xff;
	    break;
	}
#endif // HAVE_IPV6
	default:
	    XLOG_ASSERT(false);
	    return (XORP_ERROR);
	}
    default:
	break;
    }
    
    //
    // Prepare the PIM header
    //
    // TODO: XXX: PAVPAVPAV: use 'buffer = buffer_send_prepare()' ???
    // Point the buffer to the protocol header
    datalen = BUFFER_DATA_SIZE(buffer);
    BUFFER_RESET_TAIL(buffer);
    //
    pim_vt = PIM_MAKE_VT(proto_version(), message_type);
    BUFFER_PUT_OCTET(pim_vt, buffer);		// PIM version and message type
    BUFFER_PUT_OCTET(0, buffer);		// Reserved
    BUFFER_PUT_HOST_16(0, buffer);		// Zero the checksum field
    // Restore the buffer to include the data
    BUFFER_RESET_TAIL(buffer);
    BUFFER_PUT_SKIP(datalen, buffer);
    
    //
    // Compute the checksum
    //
    // XXX: The checksum for PIM_REGISTER excludes the encapsulated data packet
    switch (message_type) {
    case PIM_REGISTER:
	cksum = INET_CKSUM(BUFFER_DATA_HEAD(buffer), PIM_MINLEN);
	break;
    default:
	cksum = INET_CKSUM(BUFFER_DATA_HEAD(buffer), BUFFER_DATA_SIZE(buffer));
	break;
    }
    
    if (is_ipv6()) {
	// XXX: The checksum for IPv6 includes the IPv6 "pseudo-header"
	// as described in RFC 2460.
	struct ip6_pseudo_hdr {
	    struct in6_addr	ip6_src;	// Source address
	    struct in6_addr	ip6_dst;	// Destination address
	    uint32_t		ph_len;		// Upper-layer packet length
	    uint8_t		ph_zero[3];	// Zero
	    uint8_t		ph_next;	// Upper-layer protocol number
	} ip6_pseudo_header;	// TODO: may need __attribute__((__packed__))
	uint16_t cksum2;
	
	addr().copy_out(ip6_pseudo_header.ip6_src);
	dst.copy_out(ip6_pseudo_header.ip6_dst);
	ip6_pseudo_header.ph_len = htonl(BUFFER_DATA_SIZE(buffer));
	ip6_pseudo_header.ph_zero[0] = 0;
	ip6_pseudo_header.ph_zero[1] = 0;
	ip6_pseudo_header.ph_zero[2] = 0;
	ip6_pseudo_header.ph_next = IPPROTO_PIM;
	
	cksum2 = INET_CKSUM(&ip6_pseudo_header, sizeof(ip6_pseudo_header));
	cksum = INET_CKSUM_ADD(cksum, cksum2);
    }
    
    BUFFER_COPYPUT_INET_CKSUM(cksum, buffer, 2);	// XXX: the ckecksum
    
    XLOG_TRACE(pim_node().is_log_trace(), "TX %s from %s to %s",
	       PIMTYPE2ASCII(message_type),
	       cstring(addr()),
	       cstring(dst));
    
    //
    // Send the message
    //
    // XXX: if we use LAN-local unicast, the TTL of the unicast packets
    // probably should be MINTTL instead of DEFTTL. However, this will
    // complicate things a bit (we would need to check per message type),
    // and in general it shoudn't hurt even if we use unicast with TTL > MINTTL
    ret_value = pim_node().pim_send(vif_index(), addr(), dst,
				    dst.is_multicast() ? MINTTL : IPDEFTTL,
				    ip_tos,
				    dst.is_multicast() ? true : false,
				    buffer);
    
    //
    // Actions after the message is sent
    //
    if (ret_value >= 0) {
	switch (message_type) {
	case PIM_HELLO:
	    set_should_send_pim_hello(false);
	    break;
	default:
	    break;
	}
    }
    
    return (ret_value);
    
 buflen_error:
    XLOG_ASSERT(false);
    XLOG_ERROR("TX %s from %s to %s: "
	       "packet cannot fit into sending buffer",
	       PIMTYPE2ASCII(message_type),
	       cstring(addr()),
	       cstring(dst));
    return (XORP_ERROR);
    
 rcvlen_error:
    // XXX: this should not happen. The only way to jump here
    // is if we are trying to send a PIM Register message that did not
    // contain an IP header, but this is not a valid PIM Register message.
    XLOG_ASSERT(false);
    return (XORP_ERROR);
}

/**
 * PimVif::pim_recv:
 * @src: The message source address.
 * @dst: The message destination address.
 * @ip_ttl: The IP TTL of the message. If it has a negative value,
 * it should be ignored.
 * @ip_tos: The IP TOS of the message. If it has a negative value,
 * it should be ignored.
 * @router_alert_bool: True if the received IP packet had the ROUTER_ALERT
 * IP option set.
 * @buffer: The buffer with the received message.
 * 
 * Receive PIM message and pass it for processing.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimVif::pim_recv(const IPvX& src,
		 const IPvX& dst,
		 int ip_ttl,
		 int ip_tos,
		 bool router_alert_bool,
		 buffer_t *buffer)
{
    int ret_value = XORP_ERROR;
    
    if (! is_up())
	return (XORP_ERROR);
    
    //
    // TODO: if we are running in secure mode, then check ip_ttl and
    // @router_alert_bool (e.g. (ip_ttl == MINTTL) && (router_alert_bool))
    //
    
    ret_value = pim_process(src, dst, buffer);
    
    return (ret_value);
    
    UNUSED(ip_ttl);
    UNUSED(ip_tos);
    UNUSED(router_alert_bool);
}

/**
 * PimVif::pim_process:
 * @src: The message source address.
 * @dst: The message destination address.
 * @buffer: The buffer with the message.
 * 
 * Process PIM message and pass the control to the type-specific functions.
 * 
 * Return value: %XORP_OK on success, otherwise %XORP_ERROR.
 **/
int
PimVif::pim_process(const IPvX& src, const IPvX& dst, buffer_t *buffer)
{
    uint8_t pim_vt;
    uint8_t message_type, proto_version;
    PimNbr *pim_nbr;

    // Ignore my own PIM messages
    if (pim_node().vif_find_by_addr(src) != NULL)
	return (XORP_ERROR);
    
    //
    // Message length check.
    //
    if (BUFFER_DATA_SIZE(buffer) < PIM_MINLEN) {
	XLOG_WARNING("RX packet from %s to %s: "
		     "too short data field (%u bytes)",
		     cstring(src), cstring(dst),
		     (uint32_t)BUFFER_DATA_SIZE(buffer));
	return (XORP_ERROR);
    }
    
    //
    // Get the message type and PIM protocol version.
    // XXX: First we need the message type to verify correctly the checksum.
    //
    BUFFER_GET_OCTET(pim_vt, buffer);
    BUFFER_GET_SKIP_REVERSE(1, buffer);		// Rewind back
    message_type = PIM_VT_T(pim_vt);
    proto_version = PIM_VT_V(pim_vt);
    
    //
    // Checksum verification.
    //
    switch (message_type) {
    case PIM_REGISTER:
	if (!INET_CKSUM(BUFFER_DATA_HEAD(buffer), PIM_MINLEN))
	    break;
	//
	// XXX: Some non-spec compliant (the PC name for "buggy" :)
	// PIM-SM implementations compute the PIM_REGISTER
	// checksum over the whole packet instead of only the first 8 octets.
	// Hence, if the checksum fails over the first 8 octets, try over
	// the whole packet.
	//
	// FALLTHROUGH
    default:
	if (INET_CKSUM(BUFFER_DATA_HEAD(buffer), BUFFER_DATA_SIZE(buffer))) {
	    XLOG_WARNING("RX packet from %s to %s: "
			 "checksum error",
			 cstring(src), cstring(dst));
	    return (XORP_ERROR);
	}
	break;
    }
    
    //
    // Protocol version check.
    //
    // Note about protocol version checking (based on clarification/suggestion
    // from Mark Handley).
    // The expectation is that any protocol version increase would be
    // signalled in PIM Hello messages, and newer versions would be
    // required to fall back to the version understood by everybody,
    // or refuse to communicate with older versions (as they choose).
    // Hence, we drop everything other than a PIM Hello message
    // with version greather than the largest one we understand
    // (PIM_VERSION_MAX), but we log a warning. On the other hand,
    // we don't understand anything about versions smaller than
    // PIM_VERSION_MIN, hence we drop all messages with that version.
    if ((proto_version < PIM_VERSION_MIN)
	|| ((proto_version > PIM_VERSION_MAX)
	    && (message_type != PIM_HELLO))) {
	XLOG_WARNING("RX %s from %s to %s: "
		     "invalid PIM version: %d",
		     PIMTYPE2ASCII(message_type),
		     cstring(src), cstring(dst), proto_version);
	return (XORP_ERROR);
    }
    
#if 0	// TODO: do we need the TTL and Router Alert option checks?
    //
    // TTL (aka. Hop-limit in IPv6) and Router Alert option checks.
    //
    switch (message_type) {
    case PIM_HELLO:
    case PIM_JOIN_PRUNE:
    case PIM_ASSERT:
    case PIM_GRAFT:
    case PIM_GRAFT_ACK:
    case PIM_BOOTSTRAP:
	if (ip_ttl != 1) {
	    XLOG_WARNING("RX %s from %s to %s: "
			 "ip_ttl is %d instead of %d",
			 PIMTYPE2ASCII(message_type),
			 cstring(src), cstring(dst), ip_ttl, 1);
	    return (XORP_ERROR);
	}
	//
	// TODO: check for Router Alert option and ignore the message
	// if the option is missing and we are running in secure mode.
	//
	break;
    case PIM_REGISTER:
    case PIM_REGISTER_STOP:
    case PIM_CAND_RP_ADV:
	// Destination should be unicast. No TTL and RA check needed.
	break;
    default:
	break;
    }
#endif // 0/1

    //
    // Source address check.
    //
    if (! src.is_unicast()) {
	// Source address must always be unicast
	// The kernel should have checked that, but just in case
	XLOG_WARNING("RX %s from %s to %s: "
		     "source must be unicast",
		     PIMTYPE2ASCII(message_type),
		     cstring(src), cstring(dst));
	return (XORP_ERROR);
    }
    
    switch (message_type) {
    case PIM_HELLO:
    case PIM_JOIN_PRUNE:
    case PIM_ASSERT:
    case PIM_GRAFT:
    case PIM_GRAFT_ACK:
    case PIM_BOOTSTRAP:
	// Source address must be directly connected
	if (! is_directly_connected(src)) {
	    XLOG_WARNING("RX %s from %s to %s: "
			 "source must be directly connected",
			 PIMTYPE2ASCII(message_type),
			 cstring(src), cstring(dst));
	    return (XORP_ERROR);
	}
#ifdef HAVE_IPV6
#if 0
	// TODO: this check has to be fixed in case we use GRE tunnels
	if (is_ipv6()) {
	    struct in6_addr in6_addr;
	    src.copy_out(in6_addr);
	    if (! IN6_IS_ADDR_LINKLOCAL(&in6_addr)) {
		XLOG_WARNING("RX %s from %s to %s: "
			     "source is not a link-local address",
			     PIMTYPE2ASCII(message_type),
			     cstring(src), cstring(dst));
		return (XORP_ERROR);
	    }
	}
#endif // 0/1
#endif  // HAVE_IPV6
	break;
    case PIM_REGISTER:
    case PIM_REGISTER_STOP:
    case PIM_CAND_RP_ADV:
	// Source address can be anywhere
	// TODO: any source address check?
	break;
    default:
	break;
    }
    
    //
    // Destination address check.
    //
    switch (message_type) {
    case PIM_HELLO:
    case PIM_JOIN_PRUNE:
    case PIM_ASSERT:
    case PIM_GRAFT:
	// Destination must be multicast
	if (! dst.is_multicast()) {
	    XLOG_WARNING("RX %s from %s to %s: "
			 "destination must be multicast",
			 PIMTYPE2ASCII(message_type),
			 cstring(src), cstring(dst));
	    return (XORP_ERROR);
	}
#ifdef HAVE_IPV6
	if (is_ipv6()) {
	    //
	    // TODO: Multicast address scope check for IPv6
	    //
	}
#endif  // HAVE_IPV6
	if (dst != IPvX::PIM_ROUTERS(family())) {
	    XLOG_WARNING("RX %s from %s to %s: "
			 "destination must be ALL-PIM-ROUTERS multicast group",
			 PIMTYPE2ASCII(message_type),
			 cstring(src), cstring(dst));
	    return (XORP_ERROR);
	}
	break;
    case PIM_REGISTER:
    case PIM_REGISTER_STOP:
    case PIM_GRAFT_ACK:
    case PIM_CAND_RP_ADV:
	// Destination must be unicast
	if (! dst.is_unicast()) {
	    XLOG_WARNING("RX %s from %s to %s: "
			 "destination must be unicast",
			 PIMTYPE2ASCII(message_type),
			 cstring(src), cstring(dst));
	    return (XORP_ERROR);
	}
	break;
	
    case PIM_BOOTSTRAP:
	// Destination can be either unicast or multicast
	if (! (dst.is_unicast() || dst.is_multicast())) {
	    XLOG_WARNING("RX %s from %s to %s: "
			 "destination must be either unicast or multicast",
			 PIMTYPE2ASCII(message_type),
			 cstring(src), cstring(dst));
	    return (XORP_ERROR);
	}
#ifdef HAVE_IPV6
	if (dst.is_unicast()) {
	    // TODO: address check (if any)
	}
	if (dst.is_multicast()) {
	    if (dst != IPvX::PIM_ROUTERS(family())) {
		XLOG_WARNING("RX %s from %s to %s: "
			     "destination must be ALL-PIM-ROUTERS multicast group",
			     PIMTYPE2ASCII(message_type),
			     cstring(src), cstring(dst));
		return (XORP_ERROR);
	    }
	    
	    if (is_ipv6()) {
		//
		// TODO: Multicast address scope check for IPv6
		//
	    }
	}
#endif  // HAVE_IPV6
	break;
    default:
	break;
    }
    
    //
    // Message-specific checks.
    //
    switch (message_type) {
    case PIM_HELLO:
    case PIM_JOIN_PRUNE:
    case PIM_ASSERT:
	// PIM-SM and PIM-DM messages
	break;
    case PIM_REGISTER:
    case PIM_REGISTER_STOP:
    case PIM_BOOTSTRAP:
    case PIM_CAND_RP_ADV:
	// PIM-SM only messages
	if (proto_is_pimdm()) {
	    XLOG_WARNING("RX %s from %s to %s: "
			 "message type is PIM-SM specific",
			 PIMTYPE2ASCII(message_type),
			 cstring(src), cstring(dst));
	    return (XORP_ERROR);
	}
	break;
    case PIM_GRAFT:
    case PIM_GRAFT_ACK:
	// PIM-DM only messages
	if (proto_is_pimsm()) {
	    XLOG_WARNING("RX %s from %s to %s: "
			 "message type is PIM-DM specific",
			 PIMTYPE2ASCII(message_type),
			 cstring(src), cstring(dst));
	    return (XORP_ERROR);
	}
	break;
    default:
	XLOG_WARNING("RX %s from %s to %s: "
		     "message type (%d) is unknown",
		     PIMTYPE2ASCII(message_type),
		     cstring(src), cstring(dst), message_type);
	return (XORP_ERROR);
    }
    
    //
    // Origin router neighbor check.
    //
    pim_nbr = pim_nbr_find(src);
    switch (message_type) {
    case PIM_HELLO:
	// This could be a new neighbor
	break;
    case PIM_JOIN_PRUNE:
    case PIM_BOOTSTRAP:
    case PIM_ASSERT:
    case PIM_GRAFT:
    case PIM_GRAFT_ACK:
	// Those messages must be originated by a neighbor router
	if (((pim_nbr == NULL)
	     || ((pim_nbr != NULL) && (pim_nbr->is_nohello_neighbor())))
	    && accept_nohello_neighbors().get()) {
	    // We are configured to interoperate with neighbors that
	    // do not send Hello messages first.
	    // XXX: fake that we have received a Hello message with
	    // large enough Hello holdtime.
	    buffer_t *tmp_hello_buffer = BUFFER_MALLOC(BUF_SIZE_DEFAULT);
	    uint16_t tmp_default_holdtime
		= max(PIM_HELLO_HELLO_HOLDTIME_DEFAULT,
		      PIM_JOIN_PRUNE_HOLDTIME_DEFAULT);
	    bool is_nohello_neighbor = false;
	    if (pim_nbr == NULL)
		is_nohello_neighbor = true;
	    BUFFER_RESET(tmp_hello_buffer);
	    BUFFER_PUT_HOST_16(PIM_HELLO_HOLDTIME_OPTION, tmp_hello_buffer);
	    BUFFER_PUT_HOST_16(PIM_HELLO_HOLDTIME_LENGTH, tmp_hello_buffer);
	    BUFFER_PUT_HOST_16(tmp_default_holdtime, tmp_hello_buffer);
	    pim_hello_recv(pim_nbr, src, dst, tmp_hello_buffer, proto_version);
	    BUFFER_FREE(tmp_hello_buffer);
	    pim_nbr = pim_nbr_find(src);
	    if ((pim_nbr != NULL) && is_nohello_neighbor)
		pim_nbr->set_is_nohello_neighbor(is_nohello_neighbor);
	}
	if (pim_nbr == NULL) {
	    XLOG_WARNING("RX %s from %s to %s on vif%d(%s): "
			 "sender is not a PIM-neighbor router",
			 PIMTYPE2ASCII(message_type),
			 cstring(src), cstring(dst),
			 vif_index(),
			 name().c_str());
	    return (XORP_ERROR);
	}
	break;
    case PIM_REGISTER:
    case PIM_REGISTER_STOP:
    case PIM_CAND_RP_ADV:
	// Those messages may be originated by a remote router
	break;
    default:
	break;
    }
    
    XLOG_TRACE(pim_node().is_log_trace(), "RX %s from %s to %s",
	       PIMTYPE2ASCII(message_type),
	       cstring(src),
	       cstring(dst));
    
    /*
     * Process each message based on its type.
     */
    BUFFER_GET_SKIP(sizeof(struct pim), buffer);
    switch (message_type) {
    case PIM_HELLO:
	pim_hello_recv(pim_nbr, src, dst, buffer, proto_version);
	break;
    case PIM_REGISTER:
	pim_register_recv(pim_nbr, src, dst, buffer);
	break;
    case PIM_REGISTER_STOP:
	pim_register_stop_recv(pim_nbr, src, dst, buffer);
	break;
    case PIM_JOIN_PRUNE:
	pim_join_prune_recv(pim_nbr, src, dst, buffer, message_type);
	break;
    case PIM_BOOTSTRAP:
	pim_bootstrap_recv(pim_nbr, src, dst, buffer);
	break;
    case PIM_ASSERT:
	pim_assert_recv(pim_nbr, src, dst, buffer);
	break;
    case PIM_GRAFT:
	pim_graft_recv(pim_nbr, src, dst, buffer);
	break;
    case PIM_GRAFT_ACK:
	pim_graft_ack_recv(pim_nbr, src, dst, buffer);
	break;
    case PIM_CAND_RP_ADV:
	pim_cand_rp_adv_recv(pim_nbr, src, dst, buffer);
	break;
    default:
	break;
    }
    
    return (XORP_OK);
    
 rcvlen_error:    
    XLOG_ASSERT(false);
    XLOG_WARNING("RX %s packet from %s to %s: "
		 "some fields are too short",
		 module_name(),
		 cstring(src), cstring(dst));
    return (XORP_ERROR);
    
 buflen_error:
    XLOG_ASSERT(false);
    XLOG_WARNING("RX %s packet from %s to %s: "
		 "internal error",
		 module_name(),
		 cstring(src), cstring(dst));
    return (XORP_ERROR);
}

/**
 * PimVif::buffer_send_prepare:
 * @void: 
 * 
 * Reset and prepare the default buffer for sending data.
 * 
 * Return value: The prepared buffer.
 **/
buffer_t *
PimVif::buffer_send_prepare(void)
{
    return (buffer_send_prepare(_buffer_send));
}

/**
 * PimVif::buffer_send_prepare:
 * @buffer: The buffer to prepare.
 * 
 * Reset and prepare buffer @buffer for sendign data.
 * 
 * Return value: The prepared buffer.
 **/
buffer_t *
PimVif::buffer_send_prepare(buffer_t *buffer)
{
    BUFFER_RESET(buffer);
    BUFFER_PUT_SKIP_PIM_HEADER(buffer);
    
    return (buffer);
    
 buflen_error:
    XLOG_ASSERT(false);
    XLOG_ERROR("INTERNAL buffer_send_prepare() ERROR: buffer size too small");
    return (NULL);
}

/**
 * PimVif::pim_nbr_find:
 * @nbr_addr: The address of the neighbor to search for.
 * 
 * Find a PIM neighbor by its address.
 * 
 * Return value: The #PimNbr entry for the neighbor if found, otherwise %NULL.
 **/
PimNbr *
PimVif::pim_nbr_find(const IPvX& nbr_addr)
{
    list<PimNbr *>::iterator iter;
    for (iter = _pim_nbrs.begin(); iter != _pim_nbrs.end(); ++iter) {
	PimNbr *pim_nbr = *iter;
	if (nbr_addr == pim_nbr->addr())
	    return (pim_nbr);
    }
    
    return (NULL);
}

void
PimVif::add_pim_nbr(PimNbr *pim_nbr)
{
    _pim_nbrs.push_back(pim_nbr);
}

void
PimVif::delete_pim_nbr_from_nbr_list(PimNbr *pim_nbr)
{
    list<PimNbr *>::iterator iter;
    
    iter = find(_pim_nbrs.begin(), _pim_nbrs.end(), pim_nbr);
    if (iter != _pim_nbrs.end()) {
	XLOG_TRACE(pim_node().is_log_trace(), "Delete neighbor %s on vif %s",
		   cstring(pim_nbr->addr()), name().c_str());
	_pim_nbrs.erase(iter);
    }
}

int
PimVif::delete_pim_nbr(PimNbr *pim_nbr)
{
    delete_pim_nbr_from_nbr_list(pim_nbr);
    
    if (find(pim_node().processing_pim_nbr_list().begin(),
	     pim_node().processing_pim_nbr_list().end(),
	     pim_nbr) == pim_node().processing_pim_nbr_list().end()) {
	//
	// The PimNbr is not on the processing list, hence move it there
	//
	if (pim_nbr->pim_mre_rp_list().empty()
	    && pim_nbr->pim_mre_wc_list().empty()
	    && pim_nbr->pim_mre_sg_list().empty()
	    && pim_nbr->pim_mre_sg_rpt_list().empty()
	    && pim_nbr->processing_pim_mre_rp_list().empty()
	    && pim_nbr->processing_pim_mre_wc_list().empty()
	    && pim_nbr->processing_pim_mre_sg_list().empty()
	    && pim_nbr->processing_pim_mre_sg_rpt_list().empty()) {
	    delete pim_nbr;
	} else {
	    pim_node().processing_pim_nbr_list().push_back(pim_nbr);
	    pim_node().pim_mrt().add_task_pim_nbr_changed(Vif::VIF_INDEX_INVALID,
							  pim_nbr->addr());
	}
    }
    
    return (XORP_OK);
}

bool
PimVif::is_lan_delay_enabled() const
{
    list<PimNbr *>::const_iterator iter;
    for (iter = _pim_nbrs.begin(); iter != _pim_nbrs.end(); ++iter) {
	const PimNbr *pim_nbr = *iter;
	if (! pim_nbr->is_lan_prune_delay_present()) {
	    return (false);
	}
    }
    
    return (true);
}

const struct timeval&
PimVif::vif_propagation_delay() const
{
    static struct timeval timeval;
    uint16_t delay;
    
    // XXX: lan_delay is in milliseconds
    TIMEVAL_SET(&timeval,
		_lan_delay.get() / 1000,
		(_lan_delay.get() % 1000) * 1000);
    
    if (! is_lan_delay_enabled())
	return (timeval);
    
    delay = 0;
    list<PimNbr *>::const_iterator iter;
    for (iter = _pim_nbrs.begin(); iter != _pim_nbrs.end(); ++iter) {
	PimNbr *pim_nbr = *iter;
	if (pim_nbr->lan_delay() > delay)
	    delay = pim_nbr->lan_delay();
    }
    
    // XXX: delay is in milliseconds
    TIMEVAL_SET(&timeval, delay / 1000, (delay % 1000) * 1000);
    
    return (timeval);
}

const struct timeval&
PimVif::vif_override_interval() const
{
    static struct timeval timeval;
    uint16_t delay;
    
    // XXX: override_interval is in milliseconds
    TIMEVAL_SET(&timeval, _override_interval.get() / 1000,
		(_override_interval.get() % 1000) * 1000);
    
    if (! is_lan_delay_enabled())
	return (timeval);
    
    delay = 0;
    list<PimNbr *>::const_iterator iter;
    for (iter = _pim_nbrs.begin(); iter != _pim_nbrs.end(); ++iter) {
	PimNbr *pim_nbr = *iter;
	if (pim_nbr->override_interval() > delay)
	    delay = pim_nbr->override_interval();
    }
    
    // XXX: delay is in milliseconds
    TIMEVAL_SET(&timeval, delay / 1000, (delay % 1000) * 1000);
    
    return (timeval);
}

bool
PimVif::is_lan_suppression_state_enabled() const
{
    if (! is_lan_delay_enabled())
	return (true);
    
    list<PimNbr *>::const_iterator iter;
    for (iter = _pim_nbrs.begin(); iter != _pim_nbrs.end(); ++iter) {
	PimNbr *pim_nbr = *iter;
	if (! pim_nbr->is_tracking_support_disabled()) {
	    return (true);
	}
    }
    
    return (false);
}

//
// Compute the randomized 't_suppressed' interval:
// t_suppressed = rand(1.1 * t_periodic, 1.4 * t_periodic) when
//			Suppression_Enabled(I) is true, 0 otherwise
//
const struct timeval&
PimVif::upstream_join_timer_t_suppressed() const
{
    static struct timeval timeval;
    double random_factor
	= (PIM_JOIN_PRUNE_SUPPRESSION_TIMEOUT_RANDOM_FACTOR_MAX
	   - PIM_JOIN_PRUNE_SUPPRESSION_TIMEOUT_RANDOM_FACTOR_MIN) / 2;
    double base_ratio
	= (PIM_JOIN_PRUNE_SUPPRESSION_TIMEOUT_RANDOM_FACTOR_MAX
	   + PIM_JOIN_PRUNE_SUPPRESSION_TIMEOUT_RANDOM_FACTOR_MIN) / 2;
    
    if (is_lan_suppression_state_enabled()) {
	TIMEVAL_SET(&timeval, _join_prune_period.get(), 0);
	TIMEVAL_MUL(&timeval, base_ratio, &timeval);
	TIMEVAL_RANDOM(&timeval, random_factor);
    } else {
	TIMEVAL_CLEAR(&timeval);
    }
    
    return (timeval);
}

//
// Compute the randomized 't_override' interval value for Upstream Join Timer:
// t_override = rand(0, Override_Interval(I))
//
const struct timeval&
PimVif::upstream_join_timer_t_override() const
{
    static struct timeval timeval = vif_override_interval();
    
    // Randomize
    TIMEVAL_DIV(&timeval, 2, &timeval);
    TIMEVAL_RANDOM(&timeval, 1.0);
    
    return (timeval);
}

// Return the J/P Override Interval
const struct timeval&
PimVif::jp_override_interval() const
{
    static struct timeval timeval;
    struct timeval res1;
    struct timeval res2;
    
    res1 = vif_propagation_delay();
    res2 = vif_override_interval();
    TIMEVAL_ADD(&res1, &res2, &timeval);
    
    return (timeval);
}

/**
 * PimVif::i_am_dr:
 * void: Void.
 * 
 * Test if the protocol instance is a DR (Designated Router)
 * on a virtual interface.
 * 
 * Return value: True if the protocol instance is DR on a virtual interface,
 * otherwise false.
 **/
bool
PimVif::i_am_dr(void) const
{
    if (_proto_flags & PIM_VIF_DR)
	return (true);
    else
	return (false);
}

/**
 * PimVif::set_i_am_dr:
 * @v: If true, set the %PIM_VIF_DR flag, otherwise reset it.
 * 
 * Set/reset the %PIM_VIF_DR (Designated Router) flag on a virtual interface.
 **/
void
PimVif::set_i_am_dr(bool v)
{
    if (v) {
	_proto_flags |= PIM_VIF_DR;
    } else {
	_proto_flags &= ~PIM_VIF_DR;
    }
    pim_node().set_pim_vifs_dr(vif_index(), v);
}

void
PimVif::incr_usage_by_pim_mre_task()
{
    _usage_by_pim_mre_task++;
}

void
PimVif::decr_usage_by_pim_mre_task()
{
    XLOG_ASSERT(_usage_by_pim_mre_task > 0);
    _usage_by_pim_mre_task--;
    
    if (_usage_by_pim_mre_task == 0) {
	if (is_pending_down()) {
	    final_stop();
	}
    }
}

// TODO: temporary here. Should go to the Vif class after the Vif
// class starts using the ProtoUnit class
string
PimVif::flags_string() const
{
    string flags;
    
    if (is_up())
	flags += " UP";
    if (is_down())
	flags += " DOWN";
    if (is_pending_up())
	flags += " PENDING_UP";
    if (is_pending_down())
	flags += " PENDING_DOWN";
    if (is_ipv4())
	flags += " IPv4";
    if (is_ipv6())
	flags += " IPv6";
    if (is_enabled())
	flags += " ENABLED";
    if (is_disabled())
	flags += " DISABLED";
    if (is_underlying_vif_up())
	flags += " KERNEL_UP";
    else
	flags += " KERNEL_DOWN";
    
    return (flags);
}
