// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/contrib/olsr/face.hh,v 1.2 2008/07/23 05:09:51 pavlin Exp $

#ifndef __OLSR_FACE_HH__
#define __OLSR_FACE_HH__

class FaceManager;
class Packet;
class Link;
class Message;
class MessageDecoder;
class Neighborhood;
class Olsr;
class XorpTimer;

/**
 * @short Per-interface protocol statistics.
 */
class FaceCounters {
public:
    FaceCounters()
     : _bad_packets(0),
       _bad_messages(0),
       _duplicates(0),
       _messages_from_self(0),
       _unknown_messages(0),
       _forwarded(0)
    {}

    inline size_t bad_packets() const { return _bad_packets; }
    inline void incr_bad_packets() { _bad_packets++; }

    inline size_t bad_messages() const { return _bad_messages; }
    inline void incr_bad_messages() { _bad_messages++; }

    inline size_t messages_from_self() const { return _messages_from_self; }
    inline void incr_messages_from_self() { _messages_from_self++; }

    inline size_t unknown_messages() const { return _unknown_messages; }
    inline void incr_unknown_messages() { _unknown_messages++; }

    inline size_t duplicates() const { return _duplicates; }
    inline void incr_duplicates() { _duplicates++; }

    inline size_t forwarded() const { return _forwarded; }
    inline void incr_forwarded() { _forwarded++; }

private:
    /**
     * The number of bad packets received on this Face.
     */
    uint32_t		_bad_packets;

    /**
     * The number of bad messages received on this Face.
     */
    uint32_t		_bad_messages;

    /**
     * The number of messages received on this Face which were
     * already processed according to the duplicate set.
     */
    uint32_t		_duplicates;

    /**
     * The number of messages received on this Face which contained
     * our main address as the origin.
     */
    uint32_t		_messages_from_self;

    /**
     * The number of messages of unknown type received on this Face.
     */
    uint32_t		_unknown_messages;

    /**
     * The number of messages received on this interface which
     * have been forwarded to other nodes.
     */
    uint32_t		_forwarded;
};

/**
 * @short An OLSR interface.
 *
 * There is one Face per physical if/vif which OLSR is
 * configured to run on.
 *
 * Whilst an IPv4 interface may have multiple addresses,
 * OLSR uses IPv4 addresses as primary keys in the
 * protocol. Therefore it is necessary to track the first
 * configured address on each vif, and use that as the
 * Face's address. This happens in the absence of any
 * IPv4 link-scope addresses as the system does not
 * have any concept of such things. This may be considered
 * the same limitation as IGMP has.
 *
 * The analogue in XORP's OSPF implementation is class Peer.
 * Unlike OSPF, we do not split Face in the same way as Peer/PeerOut
 * are split, as there is no concept of Areas in version 1 of OLSR.
 *
 * A face is associated with Links to each Neighbor.
 * An instance of Olsr must have at least one Face configured to run Olsr,
 * otherwise, it isn't an Olsr router.
 * If no Faces exist, OLSR is not active.
 * If only one Face exists, only a subset of the full OLSR protocol is used.
 */
class Face {
  public:
    Face(Olsr& olsr, FaceManager& fm, Neighborhood* nh,
	 MessageDecoder& md,
	 const string& interface, const string& vif,
	 OlsrTypes::FaceID id);

    /**
     * @return the internal ID of this OLSR interface.
     */
    inline OlsrTypes::FaceID id() const { return _id; }

    /**
     * @return the name of this OLSR interface as known to the FEA.
     */
    string interface() const { return _interface; }

    /**
     * @return the name of this OLSR interface's vif as known to the FEA.
     */
    string vif() const { return _vif; }

    /**
     * @return the protocol counters for this OLSR interface.
     */
    FaceCounters& counters() {
	return _counters;
    }

    /**
     * @return the protocol counters for this OLSR interface.
     */
    const FaceCounters& counters() const {
	return _counters;
    }

    /**
     * @return the MTU of this interface.
     */
    inline uint32_t mtu() const { return _mtu; }

    /**
     * Set the MTU of this interface.
     */
    inline void set_mtu(const uint32_t mtu) { _mtu = mtu; }

    /**
     * @return the next available sequence number for a Packet to be
     *         sent from this OLSR interface.
     */
    inline uint32_t get_pkt_seqno() { return _next_pkt_seqno++; }

    /**
     * @return true if this OLSR interface is administratively up.
     */
    inline bool enabled() const {
      return _enabled;
    }

    /**
     * @return the "all nodes" address configured for this OLSR interface.
     */
    inline IPv4 all_nodes_addr() const {
      return _all_nodes_addr;
    }

    /**
     * @return the "all nodes" UDP port configured for this OLSR interface.
     */
    inline uint16_t all_nodes_port() const {
      return _all_nodes_port;
    }

    /**
     * @return the source address configured for this OLSR interface.
     */
    inline IPv4 local_addr() const {
      return _local_addr;
    }

    /**
     * @return the UDP source port configured for this OLSR interface.
     */
    inline uint16_t local_port() const {
      return _local_port;
    }

    /**
     * @return the default cost of transiting this OLSR interface.
     * Used by the shortest path algorithm.
     */
    inline int cost() const {
      return _cost;
    }

    /**
     * Set the "administratively up" state of an OLSR interface.
     *
     * @param value true if this interface is enabled for OLSR,
     *              otherwise false.
     */
    void set_enabled(bool value);

    /**
     * Set the "all nodes" address for this OLSR interface.
     *
     * @param all_nodes_addr the "all nodes" address to set.
     */
    inline void set_all_nodes_addr(const IPv4& all_nodes_addr) {
	_all_nodes_addr = all_nodes_addr;
    }

    /**
     * Set the "all nodes" UDP port for this OLSR interface.
     *
     * @param all_nodes_port the "all nodes" port to set.
     */
    inline void set_all_nodes_port(const uint16_t all_nodes_port) {
	_all_nodes_port = all_nodes_port;
    }

    /**
     * Set the local address for this OLSR interface.
     *
     * @param local_addr the local address to set.
     */
    inline void set_local_addr(const IPv4& local_addr) {
	_local_addr = local_addr;
    }

    /**
     * Set the UDP local port for this OLSR interface.
     *
     * @param local_port the local port to set.
     */
    inline void set_local_port(const uint16_t local_port) {
	_local_port = local_port;
    }

    /**
     * Set the shortest path tree cost for this OLSR interface.
     *
     * @param cost the cost to set.
     */
    inline void set_cost(int cost) {
	_cost = cost;
    }

    /**
     * Transmit a datagram on this Face.
     *
     * @param data the datagram to be sent.
     * @param len the length of the datagram to be sent.
     * @return true if the datagram was successfully transmitted.
     */
    bool transmit(uint8_t* data, const uint32_t& len);

    /**
     * Originate a HELLO message on this interface.
     *
     * The message thus originated will be filled out appropriately,
     * containing the relevant OLSR protocol state as seen from this Face.
     *
     * TODO: Perform MTU-based message segmentation.
     */
    void originate_hello();

  private:
    Olsr&		_olsr;
    FaceManager&	_fm;
    Neighborhood*	_nh;
    MessageDecoder&	_md;

    /**
     * @short Counters for this Face.
     */
    FaceCounters	_counters;

    /**
     * @short A unique identifier for this Face.
     */
    OlsrTypes::FaceID	_id;

    /**
     * @short true if this Face is enabled for routing.
     */
    bool		_enabled;

    /**
     * @short The name of the interface with which this Face is associated.
     */
    string		_interface;

    /**
     * @short The name of the Vif with which this Face is associated.
     */
    string		_vif;

    /**
     * @short The MTU for this interface.
     */
    uint32_t		_mtu;

    /**
     * @short Local address to use for transmitting OLSR
     * protocol messages in UDP.
     */
    IPv4		_local_addr;

    /**
     * @short Local port to use for transmitting
     * OLSR protocol messages.
     */
    uint16_t		_local_port;

    /**
     * @short The "all nodes" address to use on this Face
     * for sending protocol messages.
     */
    IPv4		_all_nodes_addr;

    /**
     * @short The "all nodes" port to use on this Face
     * for sending protocol messages.
     */
    uint16_t		_all_nodes_port;

    /**
     * @short A fixed cost for the interface, used in route calculation.
     */
    int			_cost;

    /**
     * @short The sequence number of the next packet sent from this Face.
     */
    uint32_t		_next_pkt_seqno;
};

#endif // __OLSR_FACE_HH__
