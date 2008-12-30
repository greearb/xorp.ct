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

#ident "$XORP: xorp/fea/io_link.cc,v 1.5 2008/10/26 23:25:07 pavlin Exp $"

#include "fea_module.h"

#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/debug.h"
#include "libxorp/mac.hh"

#include "libproto/packet.hh"

#include "fea/iftree.hh"

#include "io_link.hh"


//
// FEA I/O link raw communication base class implementation.
//

IoLink::IoLink(FeaDataPlaneManager& fea_data_plane_manager,
	       const IfTree& iftree, const string& if_name,
	       const string& vif_name, uint16_t ether_type,
	       const string& filter_program)
    : _is_running(false),
      _io_link_manager(fea_data_plane_manager.io_link_manager()),
      _fea_data_plane_manager(fea_data_plane_manager),
      _eventloop(fea_data_plane_manager.eventloop()),
      _iftree(iftree),
      _if_name(if_name),
      _vif_name(vif_name),
      _ether_type(ether_type),
      _filter_program(filter_program),
      _io_link_receiver(NULL),
      _is_log_trace(false)
{
}

IoLink::~IoLink()
{
}

void
IoLink::register_io_link_receiver(IoLinkReceiver* io_link_receiver)
{
    _io_link_receiver = io_link_receiver;
}

void
IoLink::unregister_io_link_receiver()
{
    _io_link_receiver = NULL;
}

void
IoLink::recv_packet(const Mac&		src_address,
		    const Mac&		dst_address,
		    uint16_t		ether_type,
		    const vector<uint8_t>& payload)
{
    if (_io_link_receiver == NULL) {
	// XXX: should happen only during transient setup stage
	return;
    }

    XLOG_TRACE(is_log_trace(),
	       "Received link-level packet: "
	       "src = %s dst = %s EtherType = 0x%x payload length = %u",
	       src_address.str().c_str(),
	       dst_address.str().c_str(),
	       ether_type,
	       XORP_UINT_CAST(payload.size()));

    _io_link_receiver->recv_packet(src_address, dst_address, ether_type,
				   payload);
}

void
IoLink::recv_ethernet_packet(const uint8_t* packet, size_t packet_size)
{
    Mac		src_address;
    Mac		dst_address;
    uint16_t	ether_type = 0;
    size_t	payload_size = 0;
    size_t	payload_offset = 0;
    const uint8_t* ptr = packet;

    // Test the received packet size
    if (packet_size < ETHERNET_MIN_FRAME_SIZE) {
	XLOG_WARNING("Received packet on interface %s vif %s: "
		     "packet is too short "
		     "(captured %u expecting at least %u octets)",
		     if_name().c_str(),
		     vif_name().c_str(),
		     XORP_UINT_CAST(packet_size),
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
    payload_size = packet_size - payload_offset;

    // Process the result
    vector<uint8_t> payload(payload_size);
    memcpy(&payload[0], packet + payload_offset, payload_size);
    recv_packet(src_address, dst_address, ether_type, payload);
}

int
IoLink::prepare_ethernet_packet(const Mac& src_address,
				const Mac& dst_address,
				uint16_t ether_type,
				const vector<uint8_t>& payload,
				vector<uint8_t>& packet,
				string& error_msg)
{
    size_t packet_size = 0;
    size_t pad_size = 0;
    uint8_t* ptr;
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
    // Prepare the packet
    //
    packet.resize(L2_MAX_PACKET_SIZE);
    ptr = &packet[0];

    //
    // Prepare the Ethernet header
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
    packet_size = (ptr - &packet[0]) + payload.size();
    if (ether_type < ETHERNET_LENGTH_TYPE_THRESHOLD) {
	// A IEEE 802.2 LLC frame, hence add padding if necessary
	if (packet_size < ETHERNET_MIN_FRAME_SIZE)
	    pad_size = ETHERNET_MIN_FRAME_SIZE - packet_size;
    }
    if (packet_size > packet.size()) {
	error_msg = c_format("Sending packet from %s to %s EtherType %u"
			     "on interface %s vif %s failed: "
			     "too much data: %u octets (max = %u)",
			     src_address.str().c_str(),
			     dst_address.str().c_str(),
			     ether_type,
			     if_name().c_str(),
			     vif_name().c_str(),
			     XORP_UINT_CAST(packet_size),
			     XORP_UINT_CAST(packet.size()));
	return (XORP_ERROR);
    }

    //
    // Copy the payload to the data buffer
    //
    packet.resize(packet_size + pad_size);
    memcpy(ptr, &payload[0], payload.size());
    ptr += payload.size();
    // Add the pad if necessary
    if (pad_size > 0) {
	memset(ptr, 0, pad_size);
	ptr += pad_size;
    }

    return (XORP_OK);
}
