// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2007 International Computer Science Institute
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

#ident "$XORP: xorp/fea/data_plane/io/io_link_pcap.cc,v 1.4 2007/06/27 18:54:24 pavlin Exp $"

//
// I/O link raw pcap(3)-based support.
//

#include "fea/fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/mac.hh"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#include "fea/iftree.hh"
#include "io_link_pcap.hh"


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


IoLinkPcap::IoLinkPcap(EventLoop& eventloop, const IfTree& iftree,
		       const string& if_name, const string& vif_name,
		       uint16_t ether_type, const string& filter_program)
    : _eventloop(eventloop),
      _iftree(iftree),
      _if_name(if_name),
      _vif_name(vif_name),
      _ether_type(ether_type),
      _filter_program(filter_program),
      _pcap(NULL),
      _datalink_type(-1),
      _databuf(NULL),
      _errbuf(NULL),
      _is_log_trace(false)
{
}

IoLinkPcap::~IoLinkPcap()
{
    string dummy_error_msg;

    stop(dummy_error_msg);

    // Free the buffers
    if (_databuf != NULL)
	delete[] _databuf;
    if (_errbuf != NULL)
	delete[] _errbuf;
}

int
IoLinkPcap::start(string& error_msg)
{
    return (open_pcap_access(error_msg));
}

int
IoLinkPcap::stop(string& error_msg)
{
    return (close_pcap_access(error_msg));
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

#ifndef HAVE_PCAP

int
IoLinkPcap::open_pcap_access(string& error_msg)
{
    error_msg = c_format("IoLinkPcap::open_pcap_access() failed: "
			 "The I/O link raw pcap(3)-based mechanism is not "
			 "supported");
    return (XORP_ERROR);
}

int
IoLinkPcap::close_pcap_access(string& error_msg)
{
    error_msg = c_format("IoLinkPcap::close_pcap_access() failed: "
			 "The I/O link raw pcap(3)-based mechanism is not "
			 "supported");
    return (XORP_ERROR);
}

int
IoLinkPcap::join_leave_multicast_group(bool is_join, const Mac& group,
				       string& error_msg)
{
    UNUSED(is_join);
    UNUSED(group);

    error_msg = c_format("IoLinkPcap::join_leave_multicast_group() failed: "
			 "The I/O link raw pcap(3)-based mechanism is not "
			 "supported");
    return (XORP_ERROR);
}

int
IoLinkPcap::send_packet(const Mac& src_address,
			const Mac& dst_address,
			uint16_t ether_type,
			const vector<uint8_t>& payload,
			string& error_msg)
{
    UNUSED(src_address);
    UNUSED(dst_address);
    UNUSED(ether_type);
    UNUSED(payload);

    error_msg = c_format("IoLinkPcap::send_packet() failed: "
			 "The I/O link raw pcap(3)-based mechanism is not "
			 "supported");
    return (XORP_ERROR);
}

#else // HAVE_PCAP

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
    _pcap = pcap_open_live(_vif_name.c_str(), IO_BUF_SIZE, 0, 1, _errbuf);
    if (_pcap == NULL) {
	error_msg = c_format("Cannot open interface %s vif %s "
			     "for pcap access: %s",
			     _if_name.c_str(), _vif_name.c_str(), _errbuf);
	close_pcap_access(dummy_error_msg);
	return (XORP_ERROR);
    }
    if (_errbuf[0] != '\0') {
	XLOG_WARNING("Warning opening interface %s vif %s for pcap access: %s",
		     _if_name.c_str(), _vif_name.c_str(), _errbuf);
    }

    //
    // Get the data link type
    //
    _datalink_type = pcap_datalink(_pcap);
    if (_datalink_type < 0) {
	error_msg = c_format("Cannot get the pcap data link type for "
			     "interface %s vif %s: %s",
			     _if_name.c_str(), _vif_name.c_str(),
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
			     _if_name.c_str(), _vif_name.c_str());
	close_pcap_access(dummy_error_msg);
	return (XORP_ERROR);
    }
    
    //
    // Put the pcap descriptor in non-blocking mode
    //
    if (pcap_setnonblock(_pcap, 1, _errbuf) != 0) {
	error_msg = c_format("Cannot set interface %s vif %s in pcap "
			     "non-blocking mode: %s",
			     _if_name.c_str(), _vif_name.c_str(),
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
			     _if_name.c_str(), _vif_name.c_str(),
			     pcap_geterr(_pcap));
	close_pcap_access(dummy_error_msg);
	return (XORP_ERROR);
    }

    //
    // Set the pcap filter
    //
    string filter_program = _filter_program;
    if (_ether_type > 0) {
	// Logical AND the EtherType with the filter program
	filter_program = c_format("(ether proto %u) and (%s)",
				  _ether_type, _filter_program.c_str());
    }
    //
    // XXX: We can't use filter_program.c_str() as an argument
    // to pcap_compile(), because the pcap_compile() specificiation
    // of the argument is not const-ified.
    //
    vector<char> program_buf(filter_program.size() + 1);
    strncpy(&program_buf[0], filter_program.c_str(), program_buf.size() - 1);
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
			     _if_name.c_str(), _vif_name.c_str(),
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
			     _if_name.c_str(), _vif_name.c_str(),
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
    vifp = _iftree.find_vif(_if_name, _vif_name);
    if (vifp == NULL) {
	error_msg = c_format("%s multicast group %s failed: "
			     "interface %s vif %s not found",
			     (is_join)? "Joining" : "Leaving",
			     cstring(group),
			     _if_name.c_str(),
			     _vif_name.c_str());
	return (XORP_ERROR);
    }

#if 0	// TODO: enable or disable the enabled() check?
    if (! vifp->enabled()) {
	error_msg = c_format("Cannot %s group %s on interface %s vif %s: "
			     "interface/vif is DOWN",
			     (is_join)? "join" : "leave",
			     cstring(group),
			     _if_name.c_str(),
			     _vif_name.c_str());
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
    // of AF_LINK.
    // On NetBSD and OpenBSD we need to use ifreq.ifr_addr with sa_family
    // of AF_UNSPEC.
    //
    struct ifreq ifreq;
    struct sockaddr* sa = NULL;
    memset(&ifreq, 0, sizeof(ifreq));
    strncpy(ifreq.ifr_name, _vif_name.c_str(), sizeof(ifreq.ifr_name) - 1);
#ifdef HAVE_STRUCT_IFREQ_IFR_HWADDR
    sa = &ifreq.ifr_hwaddr;
#else
    sa = &ifreq.ifr_addr;
#endif

    //
    // Prepare the MAC address
    //
    switch (_datalink_type) {
    case DLT_EN10MB:		// Ethernet (10Mb, 100Mb, 1000Mb, and up)
    {
	EtherMac group_ether;

	try {
	    group_ether.copy_in(group);
	} catch (BadMac& e) {
	    error_msg = c_format("Invalid Ethernet group address: %s",
				 e.str().c_str());
	    return (XORP_ERROR);
	}
	group_ether.copy_out(*sa);
	break;
    }

    default:
	// XXX: The data link type on the interface is not supported
	error_msg = c_format("Cannot %s group %s on interface %s vif %s: "
			     "data link type %d (%s) is not supported",
			     (is_join)? "join" : "leave",
			     cstring(group),
			     _if_name.c_str(), _vif_name.c_str(),
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
    if (ioctl(_packet_fd, request, &ifreq) < 0) {
	error_msg = c_format("Cannot %s group %s on interface %s vif %s: %s",
			     (is_join)? "join" : "leave",
			     cstring(group),
			     _if_name.c_str(), _vif_name.c_str(),
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
	const struct ether_header* ether_header_p;
	struct ether_addr ether_addr_src, ether_addr_dst;

	// Test the received packet size
	if (pcap_pkthdr.caplen < sizeof(*ether_header_p)) {
	    XLOG_WARNING("Received packet on interface %s vif %s: "
			 "data is too short "
			 "(captured %u expecting at least %u octets)",
			 _if_name.c_str(),
			 _vif_name.c_str(),
			 XORP_UINT_CAST(pcap_pkthdr.caplen),
			 XORP_UINT_CAST(sizeof(*ether_header_p)));
	    return;			// Error
	}

	ether_header_p = (const struct ether_header *)packet;

	// Extract the MAC source and destination address
	memcpy(&ether_addr_src, ether_header_p->ether_shost,
	       EtherMac::ADDR_BYTELEN);
	memcpy(&ether_addr_dst, ether_header_p->ether_dhost,
	       EtherMac::ADDR_BYTELEN);
	EtherMac ether_src(ether_addr_src);
	EtherMac ether_dst(ether_addr_dst);
	src_address.copy_in(ether_src.str());
	dst_address.copy_in(ether_dst.str());

	// Extract the EtherType
	ether_type = ntohs(ether_header_p->ether_type);

	// Calcuate the payload offset and size
	payload_offset = sizeof(*ether_header_p);
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
		     _if_name.c_str(),
		     _vif_name.c_str(),
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
    process_recv_data(src_address, dst_address, ether_type, payload);

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
    ifp = _iftree.find_interface(_if_name);
    if (ifp == NULL) {
	error_msg = c_format("No interface %s", _if_name.c_str());
	return (XORP_ERROR);
    }
    vifp = ifp->find_vif(_vif_name);
    if (vifp == NULL) {
	error_msg = c_format("No interface %s vif %s",
			     _if_name.c_str(), _vif_name.c_str());
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
	struct ether_header ether_header;
	EtherMac src_ether, dst_ether;

	//
	// Prepare the Ethernet header
	//
	try {
	    src_ether.copy_in(src_address);
	} catch (BadMac& e) {
	    error_msg = c_format("Invalid Ethernet source address: %s",
				 e.str().c_str());
	    return (XORP_ERROR);
	}
	try {
	    dst_ether.copy_in(dst_address);
	} catch (BadMac& e) {
	    error_msg = c_format("Invalid Ethernet destination address: %s",
				 e.str().c_str());
	    return (XORP_ERROR);
	}
	src_ether.copy_out(reinterpret_cast<uint8_t *>(ether_header.ether_shost));
	dst_ether.copy_out(reinterpret_cast<uint8_t *>(ether_header.ether_dhost));
	ether_header.ether_type = htons(ether_type);

	//
	// Calculate and test the packet size
	//
	packet_size = sizeof(ether_header) + payload.size();
	if (packet_size > IO_BUF_SIZE) {
	    error_msg = c_format("Sending packet from %s to %s EtherType %u"
				 "on interface %s vif %s failed: "
				 "too much data: %u octets (max = %u)",
				 src_address.str().c_str(),
				 dst_address.str().c_str(),
				 ether_type,
				 _if_name.c_str(),
				 _vif_name.c_str(),
				 XORP_UINT_CAST(packet_size),
				 XORP_UINT_CAST(IO_BUF_SIZE));
	    return (XORP_ERROR);
	}

	//
	// Copy the Ethernet header and the payload to the data buffer
	//
	memcpy(_databuf, &ether_header, sizeof(ether_header));
	memcpy(_databuf + sizeof(ether_header), &payload[0], payload.size());

	break;
    }

    default:
	error_msg = c_format("Data link type %d (%s) on interface %s vif %s "
			     "is not supported",
			     _datalink_type,
			     pcap_datalink_val_to_name(_datalink_type),
			     _if_name.c_str(), _vif_name.c_str());
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
			     _if_name.c_str(),
			     _vif_name.c_str(),
			     pcap_geterr(_pcap));
	return (XORP_ERROR);
    }

    return (XORP_OK);
}

#endif // HAVE_PCAP
