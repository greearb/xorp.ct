// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
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

// $XORP: xorp/ospf/peer.hh,v 1.153 2008/10/02 21:57:48 bms Exp $

#ifndef __OSPF_PEER_HH__
#define __OSPF_PEER_HH__

template <typename A> class Ospf;
template <typename A> class Peer;

#include "auth.hh"

/**
 * In OSPF terms this class represents an interface/link; interface is
 * too overloaded a term. The Peer class is also associated with an
 * area. In OSPFv2 there is a one-to-one correspondence. In OSPFv3 an
 * interface/link can belong to multiple areas.
 *
 * Responsibilities:
 *	1) Packet transmission; the outgoing queue lives here.
 *	2) Exchange Hello packets.
 *	3) Bring up adjacency, synchronize databases.
 *	4) Elect designated and backup router.
 */

template <typename A>
class PeerOut {
 public:

    PeerOut(Ospf<A>& ospf, const string interface, const string vif, 
	    OspfTypes::PeerID peerid, const A source,
	    OspfTypes::LinkType linktype, OspfTypes::AreaID area,
	    OspfTypes::AreaType area_type);

    ~PeerOut();

    /**
     * For debugging only printable rendition of this interface/vif.
     */
    string get_if_name() const { return _interface + "/" + _vif; }

    /**
     * If the source address matches the interface address return the
     * interface and vif.
     */
    bool match(A source, string& interface, string& vif);

    /**
     * Get Peer ID.
     */
    OspfTypes::PeerID get_peerid() const { return _peerid; }

    /**
     * Set the address of this interface/vif
     */
    bool set_interface_address(A interface_address) {
	_interface_address = interface_address;
	return true;
    }

    /**
     * Address of this interface/vif.
     *
     * @return interface/vif address.
     */
    A get_interface_address() const { return _interface_address; }

    /**
     * @return prefix length of this interface.
     */
    uint16_t get_interface_prefix_length() const;

    /**
     * @return mtu of this interface.
     */
    uint16_t get_interface_mtu() const {
	XLOG_ASSERT(0 != _interface_mtu);
	return _interface_mtu;
    }

    /**
     * The maximum size of an OSPF frame, the MTU minus the IP header.
     *
     * @return maximum frame size.
     */
    uint16_t get_frame_size() const;

    /**
     * Join multicast group on this interface/vif.
     */
    void join_multicast_group(A address) {
	_ospf.join_multicast_group(_interface, _vif, address);
    }

    /**
     * Leave multicast group on this interface/vif.
     */
    void leave_multicast_group(A address) {
	_ospf.leave_multicast_group(_interface, _vif, address);
    }

    /**
     * @return cost of this interface.
     */
    uint16_t get_interface_cost() const {
	return _interface_cost;
    }

    /**
     * @return InfTransDelay
     */
    uint16_t get_inftransdelay() const {
	return _inftransdelay;
    }

    /**
     * Get the list of areas associated with this peer.
     */
    bool get_areas(list<OspfTypes::AreaID>& areas) const;

    /**
     * Add another Area for this peer to be in, should only be allowed
     * for OSPFv3.
     */
    bool add_area(OspfTypes::AreaID area, OspfTypes::AreaType area_type);

    /**
     * Change the type of this area.
     */
    bool change_area_router_type(OspfTypes::AreaID area,
				 OspfTypes::AreaType area_type);

    /**
     * This area is being removed.
     *
     * @return true if this peer is no longer associated with any
     * areas. Allowing the caller to delete this peer.
     */
    bool remove_area(OspfTypes::AreaID area);

    /**
     * Add a neighbour to the peer.
     */
    bool add_neighbour(OspfTypes::AreaID area, A neighbour_address,
		       OspfTypes::RouterID);

    /**
     * Remove a neighbour from the peer.
     */
    bool remove_neighbour(OspfTypes::AreaID area, A neighbour_address,
			  OspfTypes::RouterID rid);
    
    /**
     * Set the state of this peer.
     */
    void set_state(bool state);

    /**
     * Get the state of this peer.
     */
    bool get_state() const {
	return _running;
    }

    /**
     * Set the link status. This is not only the link status but is
     * the interface/vif/address configured up.
     */
    void set_link_status(bool status, const char* dbg);

    /**
     * Dependent on the configured peer status and the link status
     * decide if the peer should be brought up or taken down.
     */
    void peer_change();

    /**
     * Used by external and internal entities to transmit packets.
     */
    bool transmit(typename Transmit<A>::TransmitRef tr);

    /**
     * Packets for this peer are received here.
     */
    bool receive(A dst, A src, Packet *packet) throw(BadPeer);

    /**
     * Send this LSA directly to the neighbour. Do not place on
     * retransmission list.
     *
     * @param area
     * @param nid
     * @param lsar
     *
     * @return true on success
     */
    bool send_lsa(OspfTypes::AreaID area, const OspfTypes::NeighbourID nid,
		  Lsa::LsaRef lsar);

    /**
     * Queue an LSA for transmission.
     *
     * @param peer the LSA arrived on.
     * @param nid the LSA arrived on.
     * @param lsar the lsa
     * @param multicast_on_peer Did this LSA get multicast on this peer.
     *
     * @return true on success.
     */
    bool queue_lsa(OspfTypes::PeerID peerid, OspfTypes::NeighbourID nid,
		   Lsa::LsaRef lsar, bool &multicast_on_peer) const;
    
    /**
     * Send (push) any queued LSAs.
     */
    bool push_lsas(const char* message);

    /**
     * Are any of the neighbours of this peer in the state exchange or
     * loading.
     *
     * @param area
     *
     * @return true if any of the neighbours are in state exchange or loading.
     */
    bool neighbours_exchange_or_loading(OspfTypes::AreaID area);

    /**
     * Is the state of the neighbour with the specified Router ID at
     * least 2-Way.
     *
     * @param area
     * @param rid Router ID
     * @param twoway if the neighbour is found true means the
     * neighbour is at least twoway.
     *
     * @return true if the neighbour is found.
     */
    bool neighbour_at_least_two_way(OspfTypes::AreaID area,
				    OspfTypes::RouterID rid, bool& twoway);

    /**
     * Neighbour's source address.
     *
     * @param area
     * @param rid Router ID
     * @param interface_id Interface ID.
     * @param neighbour_address set if neighbour is found.
     *
     * @return true if the neighbour is found.
     */
    bool get_neighbour_address(OspfTypes::AreaID area,
			       OspfTypes::RouterID rid,
			       uint32_t interface_id,
			       A& neighbour_address);

    /**
     * Is this LSA on this neighbours link state request list.
     * @param nid
     *
     * @return true if it is.
     */
    bool on_link_state_request_list(OspfTypes::AreaID area,
				    const OspfTypes::NeighbourID nid,
				    Lsa::LsaRef lsar);
    
    /**
     * Generate a BadLSReq event.
     *
     * @param area
     * @param nid
     *
     */
    bool event_bad_link_state_request(OspfTypes::AreaID area,
				      const OspfTypes::NeighbourID nid);

    /**
     * Are any of neighbours of this peer a virtual link endpoint.
     *
     * @return true if any are.
     */
    bool virtual_link_endpoint(OspfTypes::AreaID area);

    /**
     * @return the link type.
     */
    OspfTypes::LinkType get_linktype() const { return _linktype; }

    // Configure the peering.

    /**
     * The router ID is about to change.
     */
    void router_id_changing();

    /**
     * Set the interface ID OSPFv3 only.
     */
    bool set_interface_id(uint32_t interface_id);

    /**
     * Get the interface ID OSPFv3 only.
     */
    uint32_t get_interface_id() const { return _interface_id; }
	
    /**
     * Return a list of the fully adjacent routers.
     */
    bool get_attached_routers(OspfTypes::AreaID area,
			      list<RouterInfo>& routes);

    /**
     * Set a network to advertise OSPFv3 only.
     */
    bool add_advertise_net(OspfTypes::AreaID area, A addr, uint32_t prefix);

    /**
     * Remove all the networks that are being advertised OSPFv3 only.
     */
    bool remove_all_nets(OspfTypes::AreaID area);

    /**
     * Calls to add_advertise_net() and remove_all_nets() must be
     * followed by a call to update nets to force a new Link-LSA to be
     * sent out OSPFv3 only.
     */
    bool update_nets(OspfTypes::AreaID area);

    /**
     * Set the hello interval in seconds.
     */
    bool set_hello_interval(OspfTypes::AreaID area, uint16_t hello_interval);

    /**
     * Set options.
     */
    bool set_options(OspfTypes::AreaID area, uint32_t options);

    /**
     * Set router priority.
     */
    bool set_router_priority(OspfTypes::AreaID area, uint8_t priority);

    /**
     * Set the router dead interval in seconds.
     */
    bool set_router_dead_interval(OspfTypes::AreaID area,
				  uint32_t router_dead_interval);

    /**
     * Set a simple password authentication key.
     *
     * Note that the current authentication handler is replaced with
     * a simple password authentication handler.
     *
     * @param area the area ID.
     * @param password the password to set.
     * @param the error message (if error).
     * @return true on success, otherwise false.
     */
    bool set_simple_authentication_key(OspfTypes::AreaID area,
				       const string& password,
				       string& error_msg);

    /**
     * Delete a simple password authentication key.
     *
     * Note that after the deletion the simple password authentication
     * handler is replaced with a Null authentication handler.
     *
     * @param area the area ID.
     * @param the error message (if error).
     * @return true on success, otherwise false.
     */
    bool delete_simple_authentication_key(OspfTypes::AreaID area,
					  string& error_msg);

    /**
     * Set an MD5 authentication key.
     *
     * Note that the current authentication handler is replaced with
     * an MD5 authentication handler.
     *
     * @param area the area ID.
     * @param key_id unique ID associated with key.
     * @param password phrase used for MD5 digest computation.
     * @param start_timeval start time when key becomes valid.
     * @param end_timeval end time when key becomes invalid.
     * @param max_time_drift the maximum time drift among all routers.
     * @param the error message (if error).
     * @return true on success, otherwise false.
     */
    bool set_md5_authentication_key(OspfTypes::AreaID area, uint8_t key_id,
				    const string& password,
				    const TimeVal& start_timeval,
				    const TimeVal& end_timeval,
				    const TimeVal& max_time_drift,
				    string& error_msg);

    /**
     * Delete an MD5 authentication key.
     *
     * Note that after the deletion if there are no more valid MD5 keys,
     * the MD5 authentication handler is replaced with a Null authentication
     * handler.
     *
     * @param area the area ID.
     * @param key_id the ID of the key to delete.
     * @param the error message (if error).
     * @return true on success, otherwise false.
     */
    bool delete_md5_authentication_key(OspfTypes::AreaID area, uint8_t key_id,
				       string& error_msg);

    /**
     * Toggle the passive status of an interface.
     */
    bool set_passive(OspfTypes::AreaID area, bool passive, bool host);

    /**
     * Set the interface cost.
     */
    bool set_interface_cost(uint16_t interface_cost);

    /**
     * Set RxmtInterval.
     */
    bool set_retransmit_interval(OspfTypes::AreaID area,
				 uint16_t retransmit_interval);

    /**
     * Set InfTransDelay.
     */
    bool set_inftransdelay(uint16_t inftransdelay) {
	_inftransdelay = inftransdelay;
	return true;
    }

    /**
     *  Get a list of all the neighbours.
     */
    bool get_neighbour_list(list<OspfTypes::NeighbourID>& neighbours) const;

    /**
     * Get state information about this neighbour.
     *
     * @param nid neighbour information is being request about.
     * @param ninfo if neighbour is found its information.
     *
     */
    bool get_neighbour_info(OspfTypes::NeighbourID nid,
			    NeighbourInfo& ninfo) const;

    /**
     * Get the set of addresses that should be advertised OSPFv3 only.
     */
    set<AddressInfo<A> >& get_address_info(OspfTypes::AreaID area);

    /**
     * Enable receiving packets on this interface.
     */
    void start_receiving_packets();

    /**
     * Disable receiving packets on this interface.
     */
    void stop_receiving_packets();

 private:
    Ospf<A>& _ospf;			// Reference to the controlling class.

    const string _interface;	   	// The interface and vif this peer is
    const string _vif;			// responsible for.
    const OspfTypes::PeerID _peerid;	// The peers ID.
    uint32_t _interface_id;		// Inferface ID OSPFv3 only.
    A _interface_address;		// Interface address.
    uint16_t _interface_prefix_length;	// Interface prefix length
    uint16_t _interface_mtu;		// MTU of this interface.
    uint16_t _interface_cost;		// Cost of this interface.
    uint16_t _inftransdelay;		// InfTransDelay.

    const OspfTypes::LinkType _linktype;	// Type of this link.

					//  Areas being served.
    map<OspfTypes::AreaID, Peer<A> *>  _areas; 

    // In order for the peer to be up and running the peer has to be
    // configured up and the link also has to be up.

    bool _running;			// True if the peer is up and running
    bool _link_status;			// True if the link is up,
					// cable connected and
					// interface/vif/address
					// configured up.
    bool _status;			// True if the peer has been
					// configured up.

    set<AddressInfo<A> > _dummy;	// If get_address_info fails
					// return a reference to this
					// dummy entry 
    bool _receiving;			// Are we currently enabled
					// for receiving packets.

    /**
     * If this IPv4 then set the mask in the hello packet.
     */
    void set_mask(Peer<A> *peer);

    // In order to maintain the requirement for an interpacket gap,
    // all outgoing packets are appended to this queue. Then they are
    // read off the queue and transmitted at the interpacket gap rate.
    queue<typename Transmit<A>::TransmitRef> _transmit_queue;	

    bool bring_up_peering();
    void take_down_peering();

    /**
     * Are all the peers in a passive state.
     */
    bool get_passive();
};

template <typename A> class Neighbour;

/**
 * A peer represents a single area and is bound to a PeerOut.
 */
template <typename A>
class Peer {
 public:
    /**
     * Interface as defined by OSPF not XORP.
     */
    enum InterfaceState {
	Down,
	Loopback,
	Waiting,
	Point2Point,
	DR_other,
	Backup,
	DR,
    };

    Peer(Ospf<A>& ospf, PeerOut<A>& peerout, OspfTypes::AreaID area_id,
	 OspfTypes::AreaType area_type)
	: _ospf(ospf), _peerout(peerout), _area_id(area_id),
	  _area_type(area_type), _go_called(false),
	  _enabled(false), _passive(false), _passive_host(false),
	  _auth_handler(_ospf.get_eventloop()),
	  _interface_state(Down),
	  _hello_packet(ospf.get_version())
    {
	_hello_packet.set_area_id(area_id);

	// Some defaults taken from the Juniper manual. These values
	// should be overriden by the values in the templates files.
	// For testing set some useful values
	_hello_packet.set_hello_interval(10);
	_hello_packet.set_router_priority(128);

	// RFC 2328 Appendix C.3 Router Interface Parameters
	_hello_packet.
	    set_router_dead_interval(4 * _hello_packet.get_hello_interval());
	_rxmt_interval = 5;

	init();
    }

    ~Peer() {
	typename list<Neighbour<A> *>::iterator n;
	for (n = _neighbours.begin(); n != _neighbours.end(); n++)
	    delete (*n);
	_neighbours.clear();

	shutdown();
    }

    /**
     * Should be invoked just once.
     */
    bool init() {
	bool status = true;
	switch(_ospf.get_version()) {
	    case OspfTypes::V2:
		break;
	    case OspfTypes::V3:
		status = initV3();
		break;
	}
	return status;
    }

    /**
     * Will only execute if go() has been called. Can be called multiple
     * times paired with go().
     */
    bool shutdown() {
	bool status = true;
	if (!_go_called)
	    return status;
	else
	    _go_called = false;
	switch(_ospf.get_version()) {
	    case OspfTypes::V2:
		break;
	    case OspfTypes::V3:
		status = shutdownV3();
		break;
	}
	return status;
    }

    /**
     * Called once the peer is configured and enabled. Can be called
     * multiple times paired with shutdown().
     */
    bool go() {
	bool status = true;
	XLOG_ASSERT(!_go_called);
	_go_called = true;
	switch(_ospf.get_version()) {
	    case OspfTypes::V2:
		break;
	    case OspfTypes::V3:
		status = goV3();
		break;
	}
	return status;
    }

    /**
     * OSPFv3 specific initialisation.
     */
    bool initV3();

    /**
     * OSPFv3 specific go
     */
    bool goV3();

    /**
     * OSPFv3 specific shutdown
     */
    bool shutdownV3();

    /*
     * OSPFv3 set all the fields for the peers Link-LSA.
     */
    void populate_link_lsa();

    /**
     * If the source address matches a global address return true.
     */
    bool match(IPv6 source) const;

    /**
     * For debugging only printable rendition of this interface/vif.
     */
    string get_if_name() const { return _peerout.get_if_name(); }

    /**
     * Get Peer ID.
     *
     */
    OspfTypes::PeerID get_peerid() const { return _peerout.get_peerid(); }

    /**
     * Address of this interface/vif.
     *
     * @return interface/vif address.
     */
    A get_interface_address() const { 
	return _peerout.get_interface_address();
    }
    
    /**
     * @return prefix length of this interface.
     */
    uint16_t get_interface_prefix_length() const {
	return _peerout.get_interface_prefix_length();
    }

#if	0
    /**
     * Address of the p2p neighbour.
     *
     * @return p2p neighbour address.
     */
    A get_p2p_neighbour_address() const {
	XLOG_ASSERT(OspfTypes::PointToPoint == get_linktype());
	XLOG_ASSERT(1 == _neighbours.size());

	// When an P2P interface is configured a single neighbour will
	// exist. Fetch the address from the neighbour structure.
	typename list<Neighbour<A> *>::const_iterator ni = _neighbours.begin();
	XLOG_ASSERT(ni != _neighbours.end());

	return (*ni)->get_neighbour_address();
    }
#endif

    /**
     * @return mtu of this interface.
     */
    uint16_t get_interface_mtu() const {
	return _peerout.get_interface_mtu();
    }

    /**
     * The maximum size of an OSPF frame, the MTU minus the IP
     * header. Also include any bytes that the authentication scheme
     * may use.
     *
     * @return maximum frame size.
     */
    uint16_t get_frame_size() const {
	return _peerout.get_frame_size() - _auth_handler.additional_payload();
    }

    /**
     * @return InfTransDelay
     */
    uint16_t get_inftransdelay() const {
	return _peerout.get_inftransdelay();
    }

    /**
     * Used by external and internal entities to transmit packets.
     */
    bool transmit(typename Transmit<A>::TransmitRef tr) {
	return _peerout.transmit(tr);
    }

    /**
     * Add neighbour
     */
    bool add_neighbour(A neighbour_address, OspfTypes::RouterID rid);

    /**
     * Remove neighbour
     */
    bool remove_neighbour(A neighbour_address, OspfTypes::RouterID rid);

    /**
     * Address belongs to this router used for destination address validation.
     */
    bool belongs(A addr) const;

    /**
     * Packets for this peer are received here.
     */
    bool receive(A dst, A src, Packet *packet);

    /**
     * Used to test if an lsa should be accepted for this
     * peer/neighbour. Specifically to deal with the case that
     * AS-External-LSAs should not be sent on virtual links.
     */
    bool accept_lsa(Lsa::LsaRef lsar) const;

    /**
     * Send this LSA directly to the neighbour. Do not place on
     * retransmission list.
     *
     * @param nid
     * @param lsar
     *
     * @return true on success
     */
    bool send_lsa(const OspfTypes::NeighbourID nid, Lsa::LsaRef lsar) const;

    /**
     * Queue an LSA for transmission.
     *
     * @param peer the LSA arrived on.
     * @param nid the LSA arrived on.
     * @param lsar the lsa
     * @param multicast_on_peer Did this LSA get multicast on this peer.
     *
     * @return true on success.
     */
    bool queue_lsa(OspfTypes::PeerID peerid, OspfTypes::NeighbourID nid,
		   Lsa::LsaRef lsar, bool& multicast_on_peer) const;

    /**
     * Send (push) any queued LSAs.
     */
    bool push_lsas(const char* message);

    /*
     * Should we be computing the DR and BDR on this peer?
     * Another way of phrasing this is, is the linktype BROADCAST or NBMA?
     */
    bool do_dr_or_bdr() const;

    /**
     * @return true if this router is the DR.
     */
    bool is_DR() const;

    /**
     * @return true if this router is the BDR.
     */
    bool is_BDR() const;

    /**
     * @return true if this router is the DR or BDR.
     */
    bool is_DR_or_BDR() const;

    /**
     * Are any of the neighbours of this peer in the state exchange or
     * loading.
     *
     * @return true if any of the neighbours are in state exchange or loading.
     */
    bool neighbours_exchange_or_loading() const;

    /**
     * Get the state of the neighbour with the specified router ID.
     *
     * @param rid Router ID
     * @param twoway if the neighbour is found true means the
     * neighbour is at least twoway.
     *
     * @return true if the neighbour is found.
     */
    bool neighbour_at_least_two_way(OspfTypes::RouterID rid, bool& twoway);

    /**
     * Neighbour's source address.
     *
     * @param rid Router ID
     * @param interface_id Interface ID.
     * @param neighbour_address set if neighbour is found.
     *
     * @return true if the neighbour is found.
     */
    bool get_neighbour_address(OspfTypes::RouterID rid, uint32_t interface_id,
			       A& neighbour_address);

    /**
     * Is this LSA on this neighbours link state request list.
     * @param nid
     *
     * @return true if it is.
     */
    bool on_link_state_request_list(const OspfTypes::NeighbourID nid,
				    Lsa::LsaRef lsar) const;

    /**
     * Generate a BadLSReq event.
     *
     * @param nid
     *
     */
    bool event_bad_link_state_request(const OspfTypes::NeighbourID nid) const;

    /**
     * Are any of neighbours of this peer a virtual link endpoint.
     *
     * @return true if any are.
     */
    bool virtual_link_endpoint() const;

    /**
     * Send direct ACKs
     *
     * @param nid the neighbour that the LSAs that are being acked
     * arrived on.
     * @param ack list of acks to send.
     */
    void send_direct_acks(OspfTypes::NeighbourID nid,
			 list<Lsa_header>& ack);

    /**
     * Send delayed ACKs
     *
     * @param nid the neighbour that the LSAs that are being acked
     * arrived on.
     * @param ack list of acks to send.
     */
    void send_delayed_acks(OspfTypes::NeighbourID nid,
			  list<Lsa_header>& ack);

    /*
     * Find neighbour that this address or router ID is associated
     * with. If the linktype is Virtual Link or PointToPoint the
     * router ID is used otherwise the src address is used.
     *
     * @param src address of neighbour.
     * @param rid router ID of neighbour
     *
     * @return neighbour or 0 if no match.
     */
    Neighbour<A> *find_neighbour(A src, OspfTypes::RouterID rid);

    /**
     * @return true if this routers neighbour is the DR or BDR.
     */
    bool is_neighbour_DR_or_BDR(OspfTypes::NeighbourID nid) const;

    /**
     * Process a hello packet.
     */
    bool process_hello_packet(A dst, A src, HelloPacket *hello);

    /**
     * Process a data description packet.
     */
    bool process_data_description_packet(A dst, A src,
					 DataDescriptionPacket *dd);

    /**
     * Process a link state request packet.
     */
    bool process_link_state_request_packet(A dst, A src,
					   LinkStateRequestPacket *lsrp);

    /**
     * Process a link state update packet.
     */
    bool process_link_state_update_packet(A dst, A src,
					   LinkStateUpdatePacket *lsup);

    /**
     * Process a link state acknowledgement packet.
     */
    bool
    process_link_state_acknowledgement_packet(A dst, A src,
					      LinkStateAcknowledgementPacket
					      *lsap);

    /**
     * Start the protocol machinery running
     */
    void start();

    /**
     * Stop the protocol machinery running
     */
    void stop();

    /**
     * Change the type of this area.
     */
    void change_area_router_type(OspfTypes::AreaType area_type);

    /**
     * Event: InterfaceUP
     */
    void event_interface_up();

    /**
     * Event: WaitTimer
     */
    void event_wait_timer();

    /**
     * Event: BackupSeen
     */
    void event_backup_seen();

    /**
     * Event: NeighborChange
     */
    void event_neighbour_change();

    /**
     * Event: LoopInd
     */
    void event_loop_ind();

    /**
     * Event: UnLoopInd
     */
    void event_unloop_ind();

    /**
     * Event: InterfaceDown
     */
    void event_interface_down();

    /**
     * Schedule an event, used by the neighbours to schedule an
     * interface event.
     */
    void schedule_event(const char *);

    /**
     * Run all the deferred events, callback method.
     */
    void process_scheduled_events();

    /**
     * Get the area router.
     */
    AreaRouter<A> *get_area_router() {
	AreaRouter<A> *area_router = 
	    _ospf.get_peer_manager().get_area_router(get_area_id());
	XLOG_ASSERT(area_router);
	return area_router;
    }

    /**
     * @return the value that should be used for DR or BDR.
     * In OSPFv2 its the source address of the interface.
     * In OSPFv3 its the router ID.
     */
    static OspfTypes::RouterID get_candidate_id(A, OspfTypes::RouterID);

    /**
     * @return the value that should be used for DR or BDR for this router
     * In OSPFv2 its the source address of the interface.
     * In OSPFv3 its the router ID.
     * A dummy argument is used to force an IPv4 and an IPv6 instance
     * of this method to be generated. Isn't C++ cool?
     */
    OspfTypes::RouterID get_candidate_id(A = A::ZERO()) const;

    InterfaceState get_state() const { return _interface_state; }

    /**
     * @return the link type.
     */
    OspfTypes::LinkType get_linktype() const {
	return _peerout.get_linktype();
    }

    /**
     * Return the authentication handler.
     */
    Auth& get_auth_handler() { return _auth_handler; }

#if	0
    /**
     * @return the options field that is placed in some of outgoing
     * packets.
     */
    uint32_t send_options();
#endif

    /**
     * Fill in the common header parts of the packet.
     */
    void populate_common_header(Packet& packet);

    /**
     * Pretty print the interface state.
     */
    static string pp_interface_state(InterfaceState is);

    /**
     * @return the Area ID.
     */
    OspfTypes::AreaID get_area_id() const { return _area_id; }

    /**
     * @return the Area Type.
     */
    OspfTypes::AreaType get_area_type() const { return _area_type; }

    /**
     * @return the Area Type.
     */
    void set_area_type(OspfTypes::AreaType area_type) {
	_area_type = area_type;
    }

    /**
     * The router ID is about to change.
     */
    void router_id_changing();

    /**
     * Set the network mask OSPFv2 only.
     */
    bool set_network_mask(uint32_t network_mask);

    /**
     * Set the network mask OSPFv2 only.
     */
    uint32_t get_network_mask() const;

    /**
     * Set the interface ID OSPFv3 only.
     */
    bool set_interface_id(uint32_t interface_id);

    /**
     * Get the interface ID OSPFv3 only.
     */
    uint32_t get_interface_id() const;

    /**
     * Set a network to advertise OSPFv3 only.
     */
    bool add_advertise_net(A addr, uint32_t prefix);

    /**
     * Remove all the networks that are being advertised OSPFv3 only.
     */
    bool remove_all_nets();

    /**
     * Calls to add_advertise_net() and remove_all_nets() must be
     * followed by a call to update nets to force a new Link-LSA to be
     * sent out OSPFv3 only.
     */
    bool update_nets();

    /**
     * Set the hello interval in seconds.
     */
    bool set_hello_interval(uint16_t hello_interval);

    /**
     * Set options.
     */
    bool set_options(uint32_t options);

    /**
     * Get options.
     */
    uint32_t get_options() const;

    /**
     * Set router priority.
     */
    bool set_router_priority(uint8_t priority);

    /**
     * Set the router dead interval in seconds.
     */
    bool set_router_dead_interval(uint32_t router_dead_interval);

    /**
     * Get the router dead interval in seconds.
     */
    uint32_t get_router_dead_interval() const;

    /**
     * Set a simple password authentication key.
     *
     * Note that the current authentication handler is replaced with
     * a simple password authentication handler.
     *
     * @param password the password to set.
     * @param the error message (if error).
     * @return true on success, otherwise false.
     */
    bool set_simple_authentication_key(const string& password,
				       string& error_msg);

    /**
     * Delete a simple password authentication key.
     *
     * Note that after the deletion the simple password authentication
     * handler is replaced with a Null authentication handler.
     *
     * @param the error message (if error).
     * @return true on success, otherwise false.
     */
    bool delete_simple_authentication_key(string& error_msg);

    /**
     * Set an MD5 authentication key.
     *
     * Note that the current authentication handler is replaced with
     * an MD5 authentication handler.
     *
     * @param key_id unique ID associated with key.
     * @param password phrase used for MD5 digest computation.
     * @param start_timeval start time when key becomes valid.
     * @param end_timeval end time when key becomes invalid.
     * @param max_time_drift the maximum time drift among all routers.
     * @param the error message (if error).
     * @return true on success, otherwise false.
     */
    bool set_md5_authentication_key(uint8_t key_id, const string& password,
				    const TimeVal& start_timeval,
				    const TimeVal& end_timeval,
				    const TimeVal& max_time_drift,
				    string& error_msg);

    /**
     * Delete an MD5 authentication key.
     *
     * Note that after the deletion if there are no more valid MD5 keys,
     * the MD5 authentication handler is replaced with a Null authentication
     * handler.
     *
     * @param key_id the ID of the key to delete.
     * @param the error message (if error).
     * @return true on success, otherwise false.
     */
    bool delete_md5_authentication_key(uint8_t key_id, string& error_msg);

    /**
     * Toggle the passive status of an interface.
     */
    bool set_passive(bool passive, bool host);

    /**
     * If all peers are in state passive then return passive.
     */
    bool get_passive() const;

    /**
     * Set RxmtInterval.
     */
    bool set_rxmt_interval(uint32_t rxmt_interval);

    /**
     * Get RxmtInterval.
     */
    uint32_t get_rxmt_interval();
    
    /**
     * Get the designated router.
     */
    OspfTypes::RouterID get_designated_router() const;

    /**
     * Get the backup designated router.
     */
    OspfTypes::RouterID get_backup_designated_router() const;
    
    /**
     * Get the interface ID of the designated router.
     * OSPFv3 only.
     */
    uint32_t get_designated_router_interface_id(A = A::ZERO()) const;

    /**
     * Compute the current router link.
     *
     * Typically called after a state transition.
     */
    void update_router_links();

    /**
     * Used to notify the peer that a neighbour has become fully
     * adjacent or a neighbour is no longer fully adjacent. Used to
     * trigger the generation or withdrawal of a network-LSA. Should
     * only be called if the interface is in state DR.
     *
     * @param up true if the adjacency became full, false otherwise.
     */
    void adjacency_change(bool up);

    /**
     *  Get a list of all the neighbours.
     */
    bool get_neighbour_list(list<OspfTypes::NeighbourID>& neighbours) const;

    /**
     * Get state information about this neighbour.
     *
     * @param nid neighbour information is being request about.
     * @param ninfo if neighbour is found its information.
     *
     */
    bool get_neighbour_info(OspfTypes::NeighbourID nid,
			    NeighbourInfo& ninfo) const;

    /**
     * Get this list of addresses that should be advertised OSPFv3 only.
     */
    set<AddressInfo<A> >& get_address_info();

    /**
     * Return a list of the fully adjacent routers.
     */
    bool get_attached_routers(list<RouterInfo>& routers);

 private:
    Ospf<A>& _ospf;			// Reference to the controlling class.
    PeerOut<A>& _peerout;		// Reference to PeerOut class.
    const OspfTypes::AreaID _area_id;	// Area that is being represented.
    OspfTypes::AreaType _area_type;	// NORMAL or STUB or NSSA.
    bool _go_called;			// True if go() has been called.
    bool _enabled;			// True if the interface is enabled.
    bool _passive;			// True if the interface is in loopback
    bool _passive_host;			// True for host, False for
					// network when in loopback i.e.
					// (_passive == True).

    Auth _auth_handler;			// The authentication handler.

    XorpTimer _hello_timer;		// Timer used to fire hello messages.
    XorpTimer _wait_timer;		// Wait to discover other DRs.
    XorpTimer _event_timer;		// Defer event timer.

    uint32_t _rxmt_interval;		// The number of seconds
					// between transmission for:
					// LSAs, DDs and LSRPs.

    InterfaceState _interface_state;

    list<Neighbour<A> *> _neighbours;	// List of discovered neighbours.

    HelloPacket _hello_packet;		// Packet that is sent by this peer.

    Lsa::LsaRef _link_lsa;		// This interfaces OSPFv3 Link-LSA. 

    list<RouterLink> _router_links;	// Router links for this peer

    /**
     * Possible DR or BDR candidates.
     */
    struct Candidate {
	Candidate(OspfTypes::RouterID candidate_id,
		  OspfTypes::RouterID router_id, OspfTypes::RouterID dr,
		  OspfTypes::RouterID bdr, uint8_t router_priority) 
	    : _candidate_id(candidate_id), _router_id(router_id), _dr(dr),
	      _bdr(bdr), _router_priority(router_priority)
	{}

	// OSPFv2 the candidate ID is the interface address.
	// OSPFv3 the candidate ID is the Router ID.
	OspfTypes::RouterID _candidate_id;// Candidate's ID
	OspfTypes::RouterID _router_id;	// Router ID
	OspfTypes::RouterID _dr;	// Designated router.
	OspfTypes::RouterID _bdr;	// Backup Designated router.
	uint8_t  _router_priority;	// Router Priority.

	string str() const {
	    return c_format("CID %s RID %s DR %s BDR %s PRI %d",
			    pr_id(_candidate_id).c_str(),
			    pr_id(_router_id).c_str(),
			    pr_id(_dr).c_str(),
			    pr_id(_bdr).c_str(),
			    _router_priority);
	}
    };

    list<string> _scheduled_events;	// List of deferred events.

    set<AddressInfo<A> > _address_info;	// Set of addresses that have
					// been configured with this
					// peer. Only the peer manager
					// should access this data
					// structure. 

    /**
     * Change state, use this not set_state when changing states.
     */
    void change_state(InterfaceState state);

    /**
     * Set the state of this peer.
     */
    void set_state(InterfaceState state) {_interface_state = state; }

    /**
     * Set the designated router.
     */
    bool set_designated_router(OspfTypes::RouterID dr);

    /**
     * Set the backup designated router.
     */
    bool set_backup_designated_router(OspfTypes::RouterID dr);

    /**
     * Called when this peer becomes the designated router or
     * this peer was the designated router.
     *
     * @param yes true if the peer became the DR false if it is no
     * longer the DR.
     */
    void designated_router_changed(bool yes);

    /**
     * Unconditionally start the hello timer running.
     */
    void start_hello_timer();

    /**
     * Unconditionally stop the hello timer running.
     */
    void stop_hello_timer();

    /**
     * If the hello timer is already running, then stop and start the
     * timer. Required when the timer value is changed interactively.
     */
    void restart_hello_timer();

    void start_wait_timer();
    void stop_wait_timer();

    bool send_hello_packet();
    
    OspfTypes::RouterID
    backup_designated_router(list<Candidate>& candidates) const;
    OspfTypes::RouterID
    designated_router(list<Candidate>& candidates,
		      OspfTypes::RouterID backup_designated_router) const;

    void compute_designated_router_and_backup_designated_router();

    /**
     * Compute the current router link for OSPFv2
     *
     * Typically called after a state transition.
     */
    void update_router_linksV2(list<RouterLink>& router_links);

    /**
     * Compute the current router link for OSPFv3
     *
     * Typically called after a state transition.
     */
    void update_router_linksV3(list<RouterLink>& router_links);

    /**
     * Remove neighbour state.
     */
    void remove_neighbour_state();

    /**
     * Stop all timers.
     */
    void tear_down_state();
};

class RxmtWrapper;

/**
 * Neighbour specific information.
 */
template <typename A>
class Neighbour {
 public:
    /**
     * The ordering is important (used in the DR and BDR election).
     */
    enum State {
	Down = 1,
	Attempt = 2,
	Init = 3,
	TwoWay = 4,
	ExStart = 5,
	Exchange = 6,
	Loading = 7,
	Full = 8
    };

    static const uint32_t TIMERS = 2;	// Number of timers

    /**
     * Index for timers.
     */
    enum Timers {
	INITIAL = 0,	// Database exchanges.
	FULL = 1,	// LSA requests and retransmissions.
    };

    typedef XorpCallback0<bool>::RefPtr RxmtCallback;

    /**
     * We start in Init not Down state as typically this class is
     * created on demand when a hello packet arrives.
     */
    Neighbour(Ospf<A>& ospf, Peer<A>& peer, OspfTypes::RouterID router_id,
	      A neighbour_address, OspfTypes::NeighbourID neighbourid,
	      OspfTypes::LinkType linktype, 
	      State state = Init)
	: _ospf(ospf), _peer(peer), _router_id(router_id),
	  _neighbour_address(neighbour_address),
	  _neighbourid(neighbourid),
	  _linktype(linktype),
	  _state(state), _hello_packet(0),
	  _last_dd(ospf.get_version()),
	  _data_description_packet(ospf.get_version())
    {
	// No neighbour should ever have this ID.
	XLOG_ASSERT(OspfTypes::ALLNEIGHBOURS != neighbourid);
	
	for (uint32_t i = 0; i < TIMERS; i++)
	    _rxmt_wrapper[i] = 0;

	TimeVal t;
	_ospf.get_eventloop().current_time(t);
	// If we are debugging numbers starting from 0 are easier to
	// deal with.
#ifdef	DEBUG_LOGGING
	_data_description_packet.set_dd_seqno(0);
#else
	_data_description_packet.set_dd_seqno(t.sec());
#endif
	_creation_time = t;
    }

    ~Neighbour() {
	delete _hello_packet;
	for (uint32_t i = 0; i < TIMERS; i++)
	    delete _rxmt_wrapper[i];
    }

    /**
     * Get neighbour ID our internal ID for each neighbour.
     */
    OspfTypes::NeighbourID get_neighbour_id() const {
	return _neighbourid;
    }

    /**
     * Neighbours router ID.
     */
    OspfTypes::RouterID get_router_id() const { 
	if (_hello_packet)
	    return _hello_packet->get_router_id();
	return _router_id;
    }

    /**
     * Neighbour's source address.
     */
    A get_neighbour_address() const { return _neighbour_address; }

    /**
     * @return the value that should be used for DR or BDR for this neighbour
     * In OSPFv2 its the source address of the interface.
     * In OSPFv3 its the router ID.
     */
    OspfTypes::RouterID get_candidate_id() const {
	return Peer<A>::get_candidate_id(_neighbour_address, get_router_id());
    }

    /**
     * Get the state of this neighbour.
     */
    State get_state() const { return _state; }

    /**
     * Return the authentication handler.
     */
    Auth& get_auth_handler() { return _peer.get_auth_handler(); }

    /**
     * @return true if this routers neighbour is the DR.
     */
    bool is_neighbour_DR() const;

    /**
     * @return true if this routers neighbour is the DR or BDR.
     */
    bool is_neighbour_DR_or_BDR() const;

    /**
     * Should this neighbour be announced in hello packet.
     *
     * @return true if it should.
     */
    bool announce_in_hello_packet() const {
	return _hello_packet;
    }
	
    /**
     * Get a copy of the last hello packet that was received.
     */
    HelloPacket *get_hello_packet() { return _hello_packet; }

    /**
     * Get a copy of the last hello packet that was received.
     */
    HelloPacket *get_hello_packet() const { return _hello_packet; }

    void event_hello_received(HelloPacket *hello);

    void data_description_received(DataDescriptionPacket *dd);

    void link_state_request_received(LinkStateRequestPacket *lsrp);

    void link_state_update_received(LinkStateUpdatePacket *lsup);

    void link_state_acknowledgement_received(LinkStateAcknowledgementPacket
					     *lsap);

    /**
     * Send this LSA directly to the neighbour. Do not place on
     * retransmission list.
     *
     * @param lsar
     *
     * @return true on success
     */
    bool send_lsa(Lsa::LsaRef lsar);

    /**
     * Queue an LSA for transmission.
     *
     * @param peer the LSA arrived on.
     * @param nid the LSA arrived on.
     * @param lsar the lsa
     * @param multicast_on_peer Did this LSA get multicast on this peer.
     *
     * @return true on success.
     */
    bool queue_lsa(OspfTypes::PeerID peerid, OspfTypes::NeighbourID nid,
		   Lsa::LsaRef lsar, bool& multicast_on_peer);

    /**
     * Send (push) any queued LSAs.
     */
    bool push_lsas(const char* message);

    /**
     * Is this LSA on this neighbours link state request list.
     *
     * @return true if it is.
     */
    bool on_link_state_request_list(Lsa::LsaRef lsar) const;

    /**
     * @return the link type.
     */
    OspfTypes::LinkType get_linktype() const { return _linktype; }

    /**
     * Send acknowledgement.
     *
     * @param ack list of acknowledgements.
     * @param direct if true send directly to the neighbour.
     * @param multicast_on_peer set to true if the ack was
     * multicast. Only if direct is false is it possible for the
     * packet to be multicast.
     *
     * @return true if an acknowledgement is sent.
     */
    bool send_ack(list<Lsa_header>& ack, bool direct, bool& multicast_on_peer);

    void event_kill_neighbour();
    void event_adj_ok();
    void event_bad_link_state_request();

    /**
     * Pretty print the neighbour state.
     */
    static string pp_state(State is);

    /**
     * Get state information about this neighbour.
     *
     * @param ninfo if neighbour is found its information.
     *
     */
    bool get_neighbour_info(NeighbourInfo& ninfo) const;

    string str() {
	return "Address: " + _neighbour_address.str() +
	    "RouterID: " + pr_id(get_router_id());
    }

    static OspfTypes::NeighbourID _ticket;	// Allocator for NeighbourID's
 private:
    Ospf<A>& _ospf;			// Reference to the controlling class.
    Peer<A>& _peer;			// Reference to Peer class.
    const OspfTypes::RouterID _router_id;// Neighbour's RouterID.
    const A _neighbour_address;		// Neighbour's address.
    const OspfTypes::NeighbourID _neighbourid;	// The neighbours ID.
    const OspfTypes::LinkType _linktype;	// Type of this link.

    State _state;			// State of this neighbour.
    HelloPacket *_hello_packet;		// Last hello packet received
					// from this neighbour.
    DataDescriptionPacket _last_dd;	// Saved state from Last DDP received.
					// The DDP this neighbour sends.
    DataDescriptionPacket _data_description_packet;
    bool _all_headers_sent;		// Tracking database transmssion

    XorpTimer _rxmt_timer[TIMERS];	// Retransmit timers.
    RxmtWrapper *_rxmt_wrapper[TIMERS];	// Wrappers to retransmiter.

    DataBaseHandle _database_handle;	// Handle to the Link State Database.
    list<Lsa_header> _ls_request_list;	// Link state request list.

    list<Lsa::LsaRef> _lsa_queue;	// Queue of LSAs waiting to be sent.
    list<Lsa::LsaRef> _lsa_rxmt;	// Unacknowledged LSAs
					// awaiting retransmission.
    XorpTimer _inactivity_timer;	// Inactivity timer.

    TimeVal _creation_time;		// Creation time.
    TimeVal _adjacency_time;		// Adjacency time.

    /**
     * Get the area router.
     */
    AreaRouter<A> *get_area_router() {return _peer.get_area_router(); }
      
    /**
     * Change state, use this not set_state when changing states.
     */
    void change_state(State state);

    /**
     * Set the state of this neighbour.
     */
    void set_state(State state) {_state = state; }

    /**
     * @return true if an adjacency should be established with this neighbour
     */
    bool establish_adjacency_p() const;

    /**
     * @return true if this router is the DR.
     */
    bool is_DR() const;

    /**
     * @return true if this router is the BDR.
     */
    bool is_BDR() const;

    /**
     * @return true if this router is the DR or BDR.
     */
    bool is_DR_or_BDR() const;

    /**
     * Start the inactivity timer.
     * Used to track Hello packets from the neighbour.
     */
    void start_inactivity_timer();

    /**
     * Stop the inactivity timer.
     * Used to track Hello packets from the neighbour.
     */
    void stop_inactivity_timer();

    /**
     * Start the retransmit timer.
     *
     * @param index defining which timer to use.
     * @param RxmtCallback method to be called ever retransmit interval.
     * @param immediate don't wait for the retransmit interval send
     * one now.
     * @param comment to track the callbacks
     */
    void start_rxmt_timer(uint32_t index, RxmtCallback, bool immediate, 
			  const char *comment);

    /**
     * Stop the retransmit timer.
     * @param index defining which timer to use.
     *
     */
    void stop_rxmt_timer(uint32_t index, const char *comment);

    /**
     * restart transmitter.
     */
    void restart_retransmitter(const char* comment);

    /**
     * Stop the transmitter.
     */
//     void stop_retransmitter();

    /**
     * Retransmit link state request and link state update packets.
     *
     * @return true if there are more retransmissions to perform.
     */
    bool retransmitter();

    /**
     * Build database description packet.
     */
    void build_data_description_packet();

    /**
     * Send database description packet.
     */
    bool send_data_description_packet();

    /**
     * Start sending data description packets.
     * Should only be called in state ExStart.
     *
     * @param event_name for debugging.
     * @param immediate if true send the packet immediately, if false
     * wait the retransmit interval.
     */
    void start_sending_data_description_packets(const char *event_name,
						bool immediate = true);

    /**
     * Extract the list of LSA headers for future requests from the
     * neighbour.
     *
     * @return false if an unknown LS type is encountered or if an
     * AS-External-LSA appears in a non-normal area, otherwise true.
     */
    bool extract_lsa_headers(DataDescriptionPacket *dd);

    /**
     * Send link state request packet.
     */
    bool send_link_state_request_packet(LinkStateRequestPacket& lsrp);

    /**
     * Send link state update packet.
     * @param direct if true send directly to the neighbour.
     */
    bool send_link_state_update_packet(LinkStateUpdatePacket& lsup,
				       bool direct = false);

    /**
     * Send link state ack packet.
     * @param direct if true send directly to the neighbour.
     * @param multicast_on_peer set to true if the packet is multicast
     * false otherwise.
     */
    bool send_link_state_ack_packet(LinkStateAcknowledgementPacket& lsap,
				    bool direct,
				    bool& multicast_on_peer);

    /**
     * The state has just dropped so pull out any state associated
     * with a higher state. 
     *
     * @param previous_state
     */
    void tear_down_state(State previous_state);

    void event_1_way_received();
    void event_2_way_received();
    void event_negotiation_done();
    void event_sequence_number_mismatch();
    void event_exchange_done();
    void event_loading_done();
    void event_inactivity_timer();

    /**
     * Common code for:
     * Sequence Number Mismatch and Bad Link State Request.
     */
    void event_SequenceNumberMismatch_or_BadLSReq(const char *event_name);
};


#endif // __OSPF_PEER_HH__
