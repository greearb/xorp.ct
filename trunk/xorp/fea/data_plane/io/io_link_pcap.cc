// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007-2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ident "$XORP: xorp/fea/data_plane/io/io_link_pcap.cc,v 1.14 2008/09/26 21:41:03 pavlin Exp $"

//
// I/O link raw communication support.
//
// The mechanism is pcap(3).
//

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/mac.hh"

#include "libproto/packet.hh"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_NET_IF_DL_H
#include <net/if_dl.h>
#endif

#include "fea/iftree.hh"

#include "io_link_pcap.hh"


#ifdef HAVE_PCAP

//
// Local constants definitions
//
#define IO_BUF_SIZE		(64*1024)  // I/O buffer(s) size

//
// Local structures/classes, typedefs and macros
//

//
// Local variables
//


IoLinkPcap::IoLinkPcap(FeaDataPlaneManager& fea_data_plane_manager,
		       const IfTree& iftree, const string& if_name,
		       const string& vif_name, uint16_t ether_type,
		       const string& filter_program)
    : IoLink(fea_data_plane_manager, iftree, if_name, vif_name, ether_type,
	     filter_program),
      _pcap(NULL),
      _datalink_type(-1),
      _databuf(NULL),
      _errbuf(NULL),
      _multicast_sock(-1)
{
}

IoLinkPcap::~IoLinkPcap()
{
    string error_msg;

    if (stop(error_msg) != XORP_OK) {
	XLOG_ERROR("Cannot stop the I/O Link raw pcap(3) mechanism: %s",
		   error_msg.c_str());
    }

    // Free the buffers
    if (_databuf != NULL)
	delete[] _databuf;
    if (_errbuf != NULL)
	delete[] _errbuf;
}

int
IoLinkPcap::start(string& error_msg)
{
    if (_is_running)
	return (XORP_OK);

    //
    // Open the multicast L2 join socket
    //
    XLOG_ASSERT(_multicast_sock < 0);
    _multicast_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (_multicast_sock < 0) {
	error_msg = c_format("Error opening multicast L2 join socket: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }

    if (open_pcap_access(error_msg) != XORP_OK) {
	close(_multicast_sock);
	_multicast_sock = -1;
	return (XORP_ERROR);
    }

    _is_running = true;

    return (XORP_OK);
}

int
IoLinkPcap::stop(string& error_msg)
{
    if (! _is_running)
	return (XORP_OK);

    if (close_pcap_access(error_msg) != XORP_OK)
	return (XORP_ERROR);

    //
    // Close the multicast L2 join socket
    //
    XLOG_ASSERT(_multicast_sock >= 0);
    if (close(_multicast_sock) < 0) {
	error_msg = c_format("Error closing multicast L2 join socket: %s",
			     strerror(errno));
	return (XORP_ERROR);
    }
    _multicast_sock = -1;

    _is_running = false;

    return (XORP_OK);
}

int
IoLinkPcap::join_multicast_group(const Mac& group, string& error_msg)
{
    return (join_leave_multicast_group(true, group, error_msg));
}

int
IoLinkPcap::leave_multicast_group(const Mac& group, string& error_msg)
{
    return (join_leave_multicast_group(false, group, error_msg));
}

int
IoLinkPcap::open_pcap_access(string& error_msg)
{
    string dummy_error_msg;

    if (_packet_fd.is_valid())
	return (XORP_OK);

    // Allocate the buffers
    if (_databuf == NULL)
	_databuf = new uint8_t[IO_BUF_SIZE];
    if (_errbuf == NULL)
	_errbuf = new char[PCAP_ERRBUF_SIZE];

    //
    // Open the pcap descriptor
    //
    _errbuf[0] = '\0';
    _pcap = pcap_open_live(vif_name().c_str(), IO_BUF_SIZE, 0, 1, _errbuf);
    if (_pcap == NULL) {
	error_msg = c_format("Cannot open interface %s vif %s "
			     "for pcap access: %s",
			     if_name().c_str(), vif_name().c_str(), _errbuf);
	close_pcap_access(dummy_error_msg);
	return (XORP_ERROR);
    }
    if (_errbuf[0] != '\0') {
	XLOG_WARNING("Warning opening interface %s vif %s for pcap access: %s",
		     if_name().c_str(), vif_name().c_str(), _errbuf);
    }

    //
    // Get the data link type
    //
    _datalink_type = pcap_datalink(_pcap);
    if (_datalink_type < 0) {
	error_msg = c_format("Cannot get the pcap data link type for "
			     "interface %s vif %s: %s",
			     if_name().c_str(), vif_name().c_str(),
			     pcap_geterr(_pcap));
	close_pcap_access(dummy_error_msg);
	return (XORP_ERROR);
    }
    switch (_datalink_type) {
    case DLT_EN10MB:		// Ethernet (10Mb, 100Mb, 1000Mb, and up)
	break;			// XXX: data link type recognized

#if 0
	//
	// XXX: List of some of the data link types that might be supported
	// in the future.
	//
    case DLT_NULL:		// BSD loopback encapsulation
    case DLT_IEEE802:		// IEEE 802.5 Token Ring
    case DLT_ARCNET:		// ARCNET
    case DLT_SLIP:		// SLIP
    case DLT_PPP:		// PPP
    case DLT_FDDI:		// FDDI
    case DLT_ATM_RFC1483:	// RFC 1483 LLC/SNAP-encapsulated ATM
    case DLT_RAW:		// Raw IP
    case DLT_PPP_SERIAL:	// PPP in HDLC-like framing
    case DLT_PPP_ETHER:		// PPPoE
    case DLT_C_HDLC:		// Cisco PPP with HDLC framing
    case DLT_IEEE802_11:	// IEEE 802.11 wireless LAN
    case DLT_FRELAY:		// Frame Relay
    case DLT_LOOP:		// OpenBSD loopback encapsulation
    case DLT_LINUX_SLL:		// Linux "cooked" capture encapsulation
    case DLT_LTALK:		// Apple LocalTalk
    case DLT_PFLOG:		// OpenBSD pflog
    case DLT_PRISM_HEADER:	// Prism monitor mode info + 802.11 header
    case DLT_IP_OVER_FC:	// RFC 2625 IP-over-Fibre Channel
    case DLT_SUNATM:		// SunATM devices
    case DLT_IEEE802_11_RADIO:	// Link-layer information + 802.11 header
    case DLT_ARCNET_LINUX:	// Linux ARCNET
    case DLT_LINUX_IRDA:	// Linux IrDA
#endif // 0
    default:
	error_msg = c_format("Data link type %d (%s) on interface %s vif %s "
			     "is not supported",
			     _datalink_type,
			     pcap_datalink_val_to_name(_datalink_type),
			     if_name().c_str(), vif_name().c_str());
	close_pcap_access(dummy_error_msg);
	return (XORP_ERROR);
    }
    
    //
    // Put the pcap descriptor in non-blocking mode
    //
    if (pcap_setnonblock(_pcap, 1, _errbuf) != 0) {
	error_msg = c_format("Cannot set interface %s vif %s in pcap "
			     "non-blocking mode: %s",
			     if_name().c_str(), vif_name().c_str(),
			     pcap_geterr(_pcap));
	close_pcap_access(dummy_error_msg);
	return (XORP_ERROR);
    }

    //
    // Put the pcap descriptor in mode that avoids loops when receiving packets
    //
    pcap_breakloop(_pcap);

    //
    // Obtain a select()-able file descriptor
    //
    _packet_fd = pcap_get_selectable_fd(_pcap);
    if (! _packet_fd.is_valid()) {
	error_msg = c_format("Cannot get selectable file descriptor for "
			     "pcap access for interface %s vif %s: %s",
			     if_name().c_str(), vif_name().c_str(),
			     pcap_geterr(_pcap));
	close_pcap_access(dummy_error_msg);
	return (XORP_ERROR);
    }

    //
    // Set the pcap filter
    //
    // XXX: The filter is logical AND of the EtherType/DSAP with the
    // user's optional filter program.
    //
    string pcap_filter_program;
    if (ether_type() > 0) {
	if (ether_type() < ETHERNET_LENGTH_TYPE_THRESHOLD) {
	    // A filter using the DSAP in IEEE 802.2 LLC frame
	    pcap_filter_program = c_format("(ether[%u] = %u)",
					   ETHERNET_HEADER_SIZE,
					   ether_type());
	} else {
	    // A filter using the EtherType
	    pcap_filter_program = c_format("(ether proto %u)", ether_type());
	}
    }
    if (! filter_program().empty()) {
	if (! pcap_filter_program.empty())
	    pcap_filter_program += " and ";
	pcap_filter_program += c_format("(%s)", filter_program().c_str());
    }

    //
    // XXX: We can't use pcap_filter_program.c_str() as an argument
    // to pcap_compile(), because the pcap_compile() specificiation
    // of the argument is not const-ified.
    //
    vector<char> program_buf(pcap_filter_program.size() + 1);
    strncpy(&program_buf[0], pcap_filter_program.c_str(),
	    program_buf.size() - 1);
    program_buf[program_buf.size() - 1] = '\0';
    char* program_c_str = &program_buf[0];
    struct bpf_program bpf_program;
    if (pcap_compile(_pcap, &bpf_program, program_c_str, 1, 0) < 0) {
	error_msg = c_format("Cannot compile pcap program '%s': %s",
			     program_c_str, pcap_geterr(_pcap));
	close_pcap_access(dummy_error_msg);
	return (XORP_ERROR);
    }
    if (pcap_setfilter(_pcap, &bpf_program) != 0) {
	pcap_freecode(&bpf_program);
	error_msg = c_format("Cannot set the pcap filter for "
			     "interface %s vif %s for program '%s': %s",
			     if_name().c_str(), vif_name().c_str(),
			     program_c_str, pcap_geterr(_pcap));
	close_pcap_access(dummy_error_msg);
	return (XORP_ERROR);
    }
    pcap_freecode(&bpf_program);

    //
    // Set the direction of the traffic to capture.
    // The default is PCAP_D_INOUT (if supported by the system).
    //
    //
#ifdef PCAP_D_INOUT
    if (pcap_setdirection(_pcap, PCAP_D_INOUT) != 0) {
	error_msg = c_format("Cannot set the pcap packet capture "
			     "direction for interface %s vif %s: %s",
			     if_name().c_str(), vif_name().c_str(),
			     pcap_geterr(_pcap));
	close_pcap_access(dummy_error_msg);
	return (XORP_ERROR);
    }
#endif // PCAP_D_INOUT

    //
    // Assign a method to read from this descriptor
    //
    if (eventloop().add_ioevent_cb(_packet_fd, IOT_READ,
				   callback(this, &IoLinkPcap::ioevent_read_cb))
	== false) {
	error_msg = c_format("Cannot add a pcap file descriptor to the set of "
			     "sockets to read from in the event loop");
	close_pcap_access(dummy_error_msg);
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

int
IoLinkPcap::close_pcap_access(string& error_msg)
{
    error_msg = "";

    //
    // Close the descriptors
    //
    if (_packet_fd.is_valid()) {
	// Remove it just in case, even though it may not be select()-ed
	eventloop().remove_ioevent_cb(_packet_fd);
	_packet_fd.clear();
    }
    if (_pcap != NULL) {
	pcap_close(_pcap);
	_pcap = NULL;
    }

    return (XORP_OK);
}

int
IoLinkPcap::join_leave_multicast_group(bool is_join, const Mac& group,
				       string& error_msg)
{
    const IfTreeVif* vifp;

    // Find the vif
    vifp = iftree().find_vif(if_name(), vif_name());
    if (vifp == NULL) {
	error_msg = c_format("%s multicast group %s failed: "
			     "interface %s vif %s not found",
			     (is_join)? "Joining" : "Leaving",
			     cstring(group),
			     if_name().c_str(),
			     vif_name().c_str());
	return (XORP_ERROR);
    }

#if 0	// TODO: enable or disable the enabled() check?
    if (! vifp->enabled()) {
	error_msg = c_format("Cannot %s group %s on interface %s vif %s: "
			     "interface/vif is DOWN",
			     (is_join)? "join" : "leave",
			     cstring(group),
			     if_name().c_str(),
			     vif_name().c_str());
	return (XORP_ERROR);
    }
#endif // 0/1


    //
    // Use ioctl(SIOCADDMULTI, struct ifreq) to add L2 multicast membership.
    // Use ioctl(SIOCDELMULTI, struct ifreq) to delete L2 multicast membership.
    //
    //
    // On Linux we need to use ifreq.ifr_hwaddr with sa_family of AF_UNSPEC.
    //
    // On FreeBSD and DragonFlyBSD we need to use ifreq.ifr_addr with sa_family
    // of AF_LINK and sockaddr_dl aligned address storage.
    // On NetBSD and OpenBSD we need to use ifreq.ifr_addr with sa_family
    // of AF_UNSPEC.
    //
    uint8_t buffer[sizeof(struct ifreq) + sizeof(struct sockaddr_storage)];
    struct ifreq* ifreq_p = reinterpret_cast<struct ifreq *>(&buffer[0]);
    struct sockaddr* sa = NULL;

    memset(buffer, 0, sizeof(buffer));
    strlcpy(ifreq_p->ifr_name, vif_name().c_str(), sizeof(ifreq_p->ifr_name));
#ifdef HAVE_STRUCT_IFREQ_IFR_HWADDR
    sa = &ifreq_p->ifr_hwaddr;
#else
    sa = &ifreq_p->ifr_addr;
#endif

    //
    // Prepare the MAC address
    //
    switch (_datalink_type) {
    case DLT_EN10MB:		// Ethernet (10Mb, 100Mb, 1000Mb, and up)
    {
#if defined(HOST_OS_DRAGONFLYBSD) || defined(HOST_OS_FREEBSD)
	{
	    struct sockaddr_dl* sdl;
	    struct ether_addr ether_addr;

	    group.copy_out(ether_addr);
	    sdl = reinterpret_cast<struct sockaddr_dl *>(sa);
	    sdl->sdl_len = sizeof(*sdl);
	    sdl->sdl_family = AF_LINK;
	    sdl->sdl_alen = sizeof(ether_addr);
	    memcpy(LLADDR(sdl), &ether_addr, sizeof(ether_addr));
	}
#else
	group.copy_out(*sa);
#endif
	break;
    }

    default:
	// XXX: The data link type on the interface is not supported
	error_msg = c_format("Cannot %s group %s on interface %s vif %s: "
			     "data link type %d (%s) is not supported",
			     (is_join)? "join" : "leave",
			     cstring(group),
			     if_name().c_str(), vif_name().c_str(),
			     _datalink_type,
			     pcap_datalink_val_to_name(_datalink_type));
	return (XORP_ERROR);
    }

    //
    // XXX: NetBSD and OpenBSD have AF_LINK defined, but they insist
    // on the sockaddr's sa_family being set to AF_UNSPEC.
    // Verified on NetBSD-3.1 and OpenBSD-4.1.
    //
#if defined(HOST_OS_NETBSD) || defined(HOST_OS_OPENBSD)
    sa->sa_family = AF_UNSPEC;
#endif

    int request = (is_join)? SIOCADDMULTI : SIOCDELMULTI;
    if (ioctl(_multicast_sock, request, ifreq_p) < 0) {
	error_msg = c_format("Cannot %s group %s on interface %s vif %s: %s",
			     (is_join)? "join" : "leave",
			     cstring(group),
			     if_name().c_str(), vif_name().c_str(),
			     strerror(errno));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

void
IoLinkPcap::ioevent_read_cb(XorpFd fd, IoEventType type)
{
    UNUSED(fd);
    UNUSED(type);

    recv_data();
}

void
IoLinkPcap::recv_data()
{
    size_t	payload_size = 0;
    size_t	payload_offset = 0;
    Mac		src_address;
    Mac		dst_address;
    uint16_t	ether_type = 0;
    struct pcap_pkthdr pcap_pkthdr;
    const u_char* packet;

    // Receive a packet
    packet = pcap_next(_pcap, &pcap_pkthdr);
    if (packet == NULL) {
	XLOG_TRACE(is_log_trace(), "No packet");
	// XXX: No need to have the task that tries to read again the data
	_recv_data_task.unschedule();
	return;				// OK
    }

    //
    // XXX: Schedule a task to read again the data in case there are more
    // packets queued for receiving.
    //
    _recv_data_task = eventloop().new_oneoff_task(
	callback(this, &IoLinkPcap::recv_data));

    //
    // Decode the link-layer header
    //
    switch (_datalink_type) {
    case DLT_EN10MB:		// Ethernet (10Mb, 100Mb, 1000Mb, and up)
    {
	const uint8_t* ptr = packet;

	// Test the received packet size
	if (pcap_pkthdr.caplen < ETHERNET_MIN_FRAME_SIZE) {
	    XLOG_WARNING("Received packet on interface %s vif %s: "
			 "data is too short "
			 "(captured %u expecting at least %u octets)",
			 if_name().c_str(),
			 vif_name().c_str(),
			 XORP_UINT_CAST(pcap_pkthdr.caplen),
			 XORP_UINT_CAST(ETHERNET_MIN_FRAME_SIZE));
	    return;			// Error
	}

	// Extract the MAC destination and source address
	dst_address.copy_in(ptr);
	ptr += Mac::ADDR_BYTELEN;
	src_address.copy_in(ptr);
	ptr += Mac::ADDR_BYTELEN;

	//
	// Extract the EtherType
	//
	// XXX: The EtherType field could be either type or length
	// for Ethernet II (DIX) and IEEE 802.2 LLC frames correspondingly.
	//
	ether_type = extract_16(ptr);
	ptr += sizeof(uint16_t);

	if (ether_type < ETHERNET_LENGTH_TYPE_THRESHOLD) {
	    //
	    // This is a IEEE 802.2 LLC frame
	    //

	    //
	    // XXX: Return the DSAP from the LLC header as an EtherType value.
	    // Note that there is no colusion, because the DSAP values are
	    // in the range [0, 255], while the EtherType values are in
	    // the range [1536, 65535].
	    //
	    uint8_t dsap = extract_8(ptr);
	    ether_type = dsap;

	    //
	    // XXX: Don't move the ptr, because we only peek into the LLC
	    // header to get the DSAP value, but keep the header as
	    // part of the payload to the user.
	    //
	}

	// Calculate the payload offset and size
	payload_offset = ptr - packet;
	payload_size = pcap_pkthdr.caplen - payload_offset;
	break;
    }

    default:
	// XXX: The data link type on the interface is not supported
	return;			// Error
    }

    //
    // Various checks
    //
    if (pcap_pkthdr.caplen < pcap_pkthdr.len) {
	XLOG_WARNING("Received packet from %s to %s on interface %s vif %s: "
		     "data is too short (captured %u expecting %u octets)",
		     src_address.str().c_str(),
		     dst_address.str().c_str(),
		     if_name().c_str(),
		     vif_name().c_str(),
		     XORP_UINT_CAST(pcap_pkthdr.caplen),
		     XORP_UINT_CAST(pcap_pkthdr.len));
	return;			// Error
    }

    XLOG_TRACE(is_log_trace(),
	       "Received link-level packet: "
	       "src = %s dst = %s EtherType = 0x%x len = %u",
	       src_address.str().c_str(),
	       dst_address.str().c_str(),
	       ether_type,
	       XORP_UINT_CAST(pcap_pkthdr.caplen));

    // Process the result
    vector<uint8_t> payload(payload_size);
    memcpy(&payload[0], packet + payload_offset, payload_size);
    recv_packet(src_address, dst_address, ether_type, payload);

    return;			// OK
}

int
IoLinkPcap::send_packet(const Mac& src_address,
			const Mac& dst_address,
			uint16_t ether_type,
			const vector<uint8_t>& payload,
			string& error_msg)
{
    size_t packet_size = 0;
    const IfTreeInterface* ifp = NULL;
    const IfTreeVif* vifp = NULL;

    //
    // Test whether the interface/vif is enabled
    //
    ifp = iftree().find_interface(if_name());
    if (ifp == NULL) {
	error_msg = c_format("No interface %s", if_name().c_str());
	return (XORP_ERROR);
    }
    vifp = ifp->find_vif(vif_name());
    if (vifp == NULL) {
	error_msg = c_format("No interface %s vif %s",
			     if_name().c_str(), vif_name().c_str());
	return (XORP_ERROR);
    }
    if (! ifp->enabled()) {
	error_msg = c_format("Interface %s is down",
			     ifp->ifname().c_str());
	return (XORP_ERROR);
    }
    if (! vifp->enabled()) {
	error_msg = c_format("Interface %s vif %s is down",
			     ifp->ifname().c_str(),
			     vifp->vifname().c_str());
	return (XORP_ERROR);
    }

    //
    // Prepare the packet for transmission
    //
    switch (_datalink_type) {
    case DLT_EN10MB:		// Ethernet (10Mb, 100Mb, 1000Mb, and up)
    {
	uint8_t* ptr = _databuf;

	//
	// Prepare the Ethernet header.
	//
	dst_address.copy_out(ptr);
	ptr += Mac::ADDR_BYTELEN;
	src_address.copy_out(ptr);
	ptr += Mac::ADDR_BYTELEN;

	//
	// The EtherType
	//
	// XXX: The EtherType field could be either type or length
	// for Ethernet II (DIX) and IEEE 802.2 LLC frames correspondingly.
	//
	if (ether_type < ETHERNET_LENGTH_TYPE_THRESHOLD) {
	    // A IEEE 802.2 LLC frame, hence embed the length of the payload
	    embed_16(ptr, payload.size());
	    ptr += sizeof(uint16_t);
	} else {
	    // An Ethernet II (DIX frame), hence embed the EtherType itself
	    embed_16(ptr, ether_type);
	    ptr += sizeof(uint16_t);
	}

	//
	// Calculate and test the packet size
	//
	packet_size = (ptr - _databuf) + payload.size();
	if (packet_size > IO_BUF_SIZE) {
	    error_msg = c_format("Sending packet from %s to %s EtherType %u"
				 "on interface %s vif %s failed: "
				 "too much data: %u octets (max = %u)",
				 src_address.str().c_str(),
				 dst_address.str().c_str(),
				 ether_type,
				 if_name().c_str(),
				 vif_name().c_str(),
				 XORP_UINT_CAST(packet_size),
				 XORP_UINT_CAST(IO_BUF_SIZE));
	    return (XORP_ERROR);
	}

	//
	// Copy the payload to the data buffer
	//
	memcpy(ptr, &payload[0], payload.size());

	break;
    }

    default:
	error_msg = c_format("Data link type %d (%s) on interface %s vif %s "
			     "is not supported",
			     _datalink_type,
			     pcap_datalink_val_to_name(_datalink_type),
			     if_name().c_str(), vif_name().c_str());
	return (XORP_ERROR);
    }

    //
    // Transmit the packet
    //
    if (pcap_sendpacket(_pcap, _databuf, packet_size) != 0) {
	error_msg = c_format("Sending packet from %s to %s EtherType %u"
			     "on interface %s vif %s failed: %s",
			     src_address.str().c_str(),
			     dst_address.str().c_str(),
			     ether_type,
			     if_name().c_str(),
			     vif_name().c_str(),
			     pcap_geterr(_pcap));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

#endif // HAVE_PCAP
