// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/ospf/peer.hh,v 1.93 2005/09/09 00:13:07 atanu Exp $

#ifndef __OSPF_PEER_HH__
#define __OSPF_PEER_HH__

template <typename A> class Ospf;
template <typename A> class Peer;

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
	    PeerID peerid,
	    const A source, const uint16_t prefix_length,
	    const uint16_t interface_mtu,
	    OspfTypes::LinkType linktype, OspfTypes::AreaID area,
	    OspfTypes::AreaType area_type);

    ~PeerOut();

    /**
     * For debugging only printable rendition of this interface/vif.
     */
    string get_if_name() const { return _interface + "/" + _vif; }

    /**
     * Get Peer ID.
     *
     */
    PeerID get_peerid() const { return _peerid; }

    /**
     * Address of this interface/vif.
     *
     * @return interface/vif address.
     */
    A get_interface_address() const { return _interface_address; }

    /**
     * @return prefix length of this interface.
     */
    uint16_t get_interface_prefix_length() const {
	return _interface_prefix_length;
    }

    /**
     * @return mtu of this interface.
     */
    uint16_t get_interface_mtu() const {
	return _interface_mtu;
    }

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
     * Add another Area for this peer to be in, should only be allowed
     * for OSPFv3.
     */
    bool add_area(OspfTypes::AreaID area, OspfTypes::AreaType area_type);

    /**
     * This area is being removed.
     *
     * @return true if this peer is no longer associated with any
     * areas. Allowing the caller to delete this peer.
     */
    bool remove_area(OspfTypes::AreaID area);

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
     * Used by external and internal entities to transmit packets.
     */
    bool transmit(typename Transmit<A>::TransmitRef tr);

    /**
     * Packets for this peer are received here.
     */
    bool receive(A dst, A src, Packet *packet) throw(BadPeer);

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
    bool queue_lsa(PeerID peerid, OspfTypes::NeighbourID nid,
		   Lsa::LsaRef lsar, bool &multicast_on_peer) const;
    
    /**
     * Send (push) any queued LSAs.
     */
    bool push_lsas();

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
     * Set the interface ID OSPFv3 only.
     */
    bool set_interface_id(OspfTypes::AreaID area, uint32_t interface_id);

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
     * Set the interface cost.
     */
    bool set_interface_cost(uint16_t interface_cost) {
	_interface_cost = interface_cost;
	return true;
    }

    /**
     * Set InfTransDelay
     */
    bool set_inftransdelay(uint16_t inftransdelay) {
	_inftransdelay = inftransdelay;
	return true;
    }

 private:
    Ospf<A>& _ospf;			// Reference to the controlling class.

    const string _interface;	   	// The interface and vif this peer is
    const string _vif;			// responsible for.
    const PeerID _peerid;		// The peers ID.
    const A _interface_address;		// Interface address.
    const uint16_t _interface_prefix_length;	// Interface prefix length
    const uint16_t _interface_mtu;	// MTU of this interface.
    uint16_t _interface_cost;		// Cost of this interface.
    uint16_t _inftransdelay;		// InfTransDelay.

    const OspfTypes::LinkType _linktype;	// Type of this link.

					//  Areas being served.
    map<OspfTypes::AreaID, Peer<A> *>  _areas; 

    bool _running;			// True if the peer is up and running

    /**
     * If this IPv4 then set the mask in the hello packet.
     */
    void set_mask(Peer<A> *peer);

    // In order to maintain the requirement for an interpacket gap,
    // all outgoing packets are appended to this queue. Then they are
    // read off the queue and transmitted at the interpacket gap rate.
    queue<typename Transmit<A>::TransmitRef> _transmit_queue;	

    void bring_up_peering();
    void take_down_peering();
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
	  _area_type(area_type), _interface_state(Down),
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
    }

    ~Peer() {
	typename list<Neighbour<A> *>::iterator n;
	for (n = _neighbours.begin(); n != _neighbours.end(); n++)
	    delete (*n);
	_neighbours.clear();
    }

    /**
     * For debugging only printable rendition of this interface/vif.
     */
    string get_if_name() const { return _peerout.get_if_name(); }

    /**
     * Get Peer ID.
     *
     */
    PeerID get_peerid() const { return _peerout.get_peerid(); }

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

    /**
     * Adress of the p2p neighbour.
     *
     * @return p2p neighbour address.
     */
    A get_p2p_neighbour_address() const {
	XLOG_ASSERT(OspfTypes::PointToPoint == get_linktype());
	XLOG_UNFINISHED();
	// When an P2P interface is configured a single neighbour will
	// exist. Fetch the address from the neighbour structure.
	return 0;
    }

    /**
     * @return mtu of this interface.
     */
    uint16_t get_interface_mtu() const {
	return _peerout.get_interface_mtu();
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
     * Packets for this peer are received here.
     */
    bool receive(A dst, A src, Packet *packet);

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
    bool queue_lsa(PeerID peerid, OspfTypes::NeighbourID nid,
		   Lsa::LsaRef lsar, bool& multicast_on_peer) const;

    /**
     * Send (push) any queued LSAs.
     */
    bool push_lsas();

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
     * Find neighbour that this packet should be associated with.
     *
     * @return neighbour or 0 if no match.
     */
    Neighbour<A> *find_neighbour(A src, Packet *packet);

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

 private:
    Ospf<A>& _ospf;			// Reference to the controlling class.
    PeerOut<A>& _peerout;		// Reference to PeerOut class.
    const OspfTypes::AreaID _area_id;	// Area that is being represented.
    const OspfTypes::AreaType _area_type;// NORMAL or STUB or NSSA.

    XorpTimer _hello_timer;		// Timer used to fire hello messages.
    XorpTimer _wait_timer;		// Wait to discover other DRs.
    XorpTimer _event_timer;		// Defer event timer.

    uint32_t _rxmt_interval;		// The number of seconds
					// between transmission for:
					// LSAs, DDs and LSRPs.

    InterfaceState _interface_state;

    list<Neighbour<A> *> _neighbours;	// List of discovered neighbours.

    HelloPacket _hello_packet;		// Packet that is sent by this peer.

    list<RouterLink> _router_links;	// Router links for this peer

    /**
     * Possible DR or BDR candidates.
     */
    struct Candidate {
	Candidate(OspfTypes::RouterID router_id, OspfTypes::RouterID dr,
		  OspfTypes::RouterID bdr, uint8_t router_priority) 
	    : _router_id(router_id), _dr(dr), _bdr(bdr), 
	      _router_priority(router_priority)
	{}

	OspfTypes::RouterID _router_id;	// Candidate's ID
	OspfTypes::RouterID _dr;	// Designated router.
	OspfTypes::RouterID _bdr;	// Backup Designated router.
	uint8_t  _router_priority;	// Router Priority.
    };

    list<string> _scheduled_events;	// List of deferred events.

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
     * Return a list of the fully adjacent routers.
     */
    void get_attached_routers(list<OspfTypes::RouterID>& routers);

    void start_hello_timer();

    void start_wait_timer();

    bool send_hello_packet();
    
    OspfTypes::RouterID
    backup_designated_router(list<Candidate>& candidates) const;
    OspfTypes::RouterID
    designated_router(list<Candidate>& candidates) const;

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
	  _data_description_packet(ospf.get_version()),
	  _rxmt_wrapper(0)
    {
	// No neigbhour should ever have this ID.
	XLOG_ASSERT(OspfTypes::ALLNEIGHBOURS != neighbourid);

	TimeVal t;
	_ospf.get_eventloop().current_time(t);
	// If we are debugging numbers starting from 0 are easier to
	// deal with.
#ifdef	DEBUG_LOGGING
	_data_description_packet.set_dd_seqno(0);
#else
	_data_description_packet.set_dd_seqno(t.sec());
#endif
    }

    ~Neighbour() {
	delete _hello_packet;
	delete _rxmt_wrapper;
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
    OspfTypes::RouterID get_router_id() const { return _router_id; }

    /**
     * Neighbours source address.
     */
    A get_neighbour_address() const { return _neighbour_address; }

    /**
     * @return the value that should be used for DR or BDR for this neighbour
     * In OSPFv2 its the source address of the interface.
     * In OSPFv3 its the router ID.
     */
    OspfTypes::RouterID get_candidate_id() const {
	return Peer<A>::get_candidate_id(_neighbour_address, _router_id);
    }

    /**
     * Get the state of this neighbour.
     */
    State get_state() const { return _state; }

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
     * Queue an LSA for transmission.
     *
     * @param peer the LSA arrived on.
     * @param nid the LSA arrived on.
     * @param lsar the lsa
     * @param multicast_on_peer Did this LSA get multicast on this peer.
     *
     * @return true on success.
     */
    bool queue_lsa(PeerID peerid, OspfTypes::NeighbourID nid,
		   Lsa::LsaRef lsar, bool& multicast_on_peer);

    /**
     * Send (push) any queued LSAs.
     */
    bool push_lsas();

    /**
     * Is this LSA on this neighbours link state request list.
     *
     * @return true if it is.
     */
    bool on_link_state_request_list(Lsa::LsaRef lsar) const;

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

    XorpTimer _rxmt_timer;		// Retransmit timer.
    RxmtWrapper *_rxmt_wrapper;		// Wrapper to retransmiter.

    DataBaseHandle _database_handle;	// Handle to the Link State Database.
    list<Lsa_header> _ls_request_list;	// Link state request list.

    list<Lsa::LsaRef> _lsa_queue;	// Queue of LSAs waiting to be sent.
    list<Lsa::LsaRef> _lsa_rxmt;	// Unacknowledged LSAs
					// awaiting retransmission.
    XorpTimer _inactivity_timer;	// Inactivity timer.

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
     * @param RxmtCallback method to be called ever retransmit interval.
     * @param immediate don't wait for the retransmit interval send
     * one now.
     * @param comment to track the callbacks
     */
    void start_rxmt_timer(RxmtCallback, bool immediate, const char *comment);

    /**
     * Stop the retransmit timer.
     */
    void stop_rxmt_timer(const char *comment);

    /**
     * restart transmitter.
     */
    void restart_retransmitter();

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
     */
    void start_sending_data_description_packets(const char *event_name);

    /**
     * Send link state request packet.
     */
    bool send_link_state_request_packet(LinkStateRequestPacket& lsrp);

    /**
     * Send link state update packet.
     */
    bool send_link_state_update_packet(LinkStateUpdatePacket& lsup);

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
