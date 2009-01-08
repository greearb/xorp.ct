// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/contrib/olsr/face_manager.hh,v 1.3 2008/10/02 21:56:34 bms Exp $

#ifndef __OLSR_FACE_MANAGER_HH__
#define __OLSR_FACE_MANAGER_HH__

class Olsr;
class Message;
class Face;
class FaceManager;
class LogicalLink;
class Neighbor;
class Neighborhood;

/**
 * @short A callback used to process a specific type of OLSR
 *        protocol message.
 */
typedef XorpCallback3<bool,
		      Message*, const IPv4&, const IPv4&>::RefPtr
		     MessageReceiveCB;

/**
 * @short A member of the duplicate set.
 *
 * This is contained within a map keyed by its origin address.
 */
class DupeTuple {
public:
    DupeTuple(EventLoop& ev, FaceManager* parent, const IPv4& origin,
	      const uint16_t seqno, const TimeVal& vtime)
     : _ev(ev), _parent(parent),
       _origin(origin), _seqno(seqno), _is_forwarded(false)
    {
	update_timer(vtime);
    }

    /**
     * @return the origin of this duplicate set tuple.
     */
    inline IPv4 origin() const { return _origin; }

    /**
     * @return the sequence number of this duplicate set tuple.
     */
    inline uint16_t seqno() const { return _seqno; }

    /**
     * @return true if the message has previously been forwarded.
     */
    inline bool is_forwarded() const { return _is_forwarded; }

    /**
     * Set the forwarded flag for this message.
     *
     * @param is_forwarded the new value of _is_forwarded.
     */
    inline void set_is_forwarded(const bool is_forwarded) {
	_is_forwarded = is_forwarded;
    }

    /**
     * Determine if an interface has already received this message.
     *
     * @param faceid the ID of the interface to check.
     * @return true if this message has previously been received by faceid.
     */
    inline bool is_seen_by_face(const OlsrTypes::FaceID faceid) const {
	return _iface_list.count(faceid) > 0;
    }

    /**
     * Add an interface to the list of interfaces which have already
     * received this message.
     *
     * @param faceid the ID of the interface to add.
     */
    inline void set_seen_by_face(const OlsrTypes::FaceID faceid) {
	if (0 == _iface_list.count(faceid))
	    _iface_list.insert(faceid);
    }

    /**
     * Update the validity timer on this duplicate set entry.
     *
     * @param vtime relative validity time from now.
     */
    void update_timer(const TimeVal& vtime);

    /**
     * Callback method to: remove a duplicate set entry when it expires.
     */
    void event_dead();

private:
    EventLoop&	    _ev;
    FaceManager*    _parent;

    set<OlsrTypes::FaceID>    _iface_list;	// D_iface_list
    IPv4	    _origin;			// D_addr
    uint16_t	    _seqno;			// D_seq_num
    bool	    _is_forwarded;		// D_retransmitted
    XorpTimer	    _expiry_timer;		// D_time
};

/**
 * @short Class which manages all interface bindings
 * in the OLSR routing process.
 */
class FaceManager {
  public:
    FaceManager(Olsr& olsr, EventLoop& ev);
    ~FaceManager();

    MessageDecoder& message_decoder() { return _md; }

    Neighborhood* neighborhood() { return _nh; }
    void set_neighborhood(Neighborhood* nh) { _nh = nh; }

    /**
     * @return the number of interfaces known to FaceManager.
     */
    uint32_t get_face_count() const { return _faces.size(); }

    /**
     * Get a pointer to a Face given its ID.
     *
     * @param faceid the face ID.
     * @return pointer to the face.
     * @throw BadFace if the face ID cannot be found.
     */
    const Face* get_face_by_id(const OlsrTypes::FaceID faceid) const
	throw(BadFace);

    /**
     * Fill out a list of all face IDs.
     *
     * @param face_list the list to fill out.
     */
    void get_face_list(list<OlsrTypes::FaceID>& face_list) const;

    /**
     * @return the OLSR main address of this node.
     */
    IPv4 get_main_addr() const { return _main_addr; }

    /**
     * Attempt to set main address on node.
     *
     * TODO: Track address changes on interfaces.
     * TODO: Add a flag which does NOT allow the main address to
     * automatically change when interfaces change.
     *
     * This will fulfil the RFC requirement that the main address may never
     * change, but will effectively halt olsr when the main address is set
     * to an address we are not configured for.
     */
    bool set_main_addr(const IPv4& addr);

    /**
     * @return the MID_INTERVAL protocol variable.
     */
    TimeVal get_mid_interval() const { return _mid_interval; }

    /**
     * Set the MID_INTERVAL protocol variable.
     * The timer will be rescheduled if it is running.
     *
     * @param interval the new MID_INTERVAL.
     */
    void set_mid_interval(const TimeVal& interval);

    /**
     * @return the MID_HOLD_TIME protocol variable.
     */
    TimeVal get_mid_hold_time() const { return _mid_interval * 3; }

    /**
     * @return the DUP_HOLD_TIME protocol variable.
     */
    TimeVal get_dup_hold_time() const { return _dup_hold_time; }

    /**
     * Set the hold time for duplicate messages.
     *
     * Note: This does not affect messages which are currently being tracked
     * in the duplicate set.
     *
     * @param dup_hold_time the new hold time for duplicate messages.
     */
    void set_dup_hold_time(const TimeVal& dup_hold_time);

    /**
     * Process a received datagram.
     *
     * @param interface the interface where the datagram was received.
     * @param vif the vif where the datagram was received.
     * @param dst the IPv4 destination address of the datagram.
     * @param dport the UDP destination port of the datagram.
     * @param src the IPv4 source address of the datagram.
     * @param sport the UDP source port of the datagram.
     * @param data the received datagram.
     * @param len the length of the received datagram.
     */
    void receive(const string& interface, const string& vif,
	const IPv4 & dst, const uint16_t & dport,
	const IPv4 & src, const uint16_t & sport,
	uint8_t* data, const uint32_t & len);

    /**
     * Transmit a datagram.
     *
     * @param interface the interface to transmit from.
     * @param vif the vif to transmit from.
     * @param dst the IPv4 destination address to send to.
     * @param dport the UDP destination port to send to.
     * @param src the IPv4 source address to transmit from.
     * @param sport the UDP source port to transmit from.
     * @param data the datagram to transmit.
     * @param len the length of the datagram to transmit.
     * @return true if the datagram was sent OK, otherwise false.
     */
    bool transmit(const string& interface, const string& vif,
	const IPv4 & dst, const uint16_t & dport,
	const IPv4 & src, const uint16_t & sport,
	uint8_t* data, const uint32_t & len);

    /**
     * Add a new interface.
     *
     * @param interface the name of the interface as known to the FEA.
     * @param vif the name of the vif as known to the FEA.
     * @return the ID of the new interface.
     * @throw BadFace if the interface could not be added.
     */
    OlsrTypes::FaceID create_face(const string& interface, const string& vif)
	throw(BadFace);

    /**
     * Clear the interface list.
     */
    void clear_faces();

    /**
     * Activate the OLSR binding to the given interface.
     *
     * This means recomputing address lists and choosing the primary
     * address on that interface; we do not yet support changing the
     * address used.
     *
     * We do not bring up any sockets until set_face_enabled().
     */
    bool activate_face(const string & interface, const string & vif);

    /**
     * Delete an interface.
     *
     * @param faceid the ID of the Face to delete.
     * @return true if the interface was deleted.
     */
    bool delete_face(OlsrTypes::FaceID faceid);

    /**
     * Recompute the list of protocol addresses used
     * by OLSR to send and receive protocol control traffic
     * on this Face.
     *
     * @param faceid the ID of the Face to recompute addresses on.
     * @return true if the address lists were recomputed successfully.
     *
     * This method works significantly differently than in OSPF.
     * In OLSR, we need to recompute the list of addresses to which
     * OLSR control traffic may originate from or be received on
     * whenever the configured addresses on an interface change.
     *
     * This relates only to the MID message; there is no strict
     * parallel to the network LSA in OSPF.
     */
    bool recompute_addresses_face(OlsrTypes::FaceID faceid);

    /**
     * Callback method to: Process a change of vif status from the FEA.
     *
     * @param interface the name of the affected interface.
     * @param vif the name of the affected vif.
     * @param state the new state of the vif.
     */
    void vif_status_change(const string& interface, const string& vif,
	bool state);

    /**
     * Callback method to: Process a change of address status from the FEA.
     *
     * @param interface the name of the affected interface.
     * @param vif the name of the affected vif.
     * @param addr the affected IPv4 interface address.
     * @param state the new state of the address.
     */
    void address_status_change(const string& interface, const string& vif,
	IPv4 addr, bool state);

    /**
     * Get the ID of an interface, given its names as known to the FEA.
     *
     * @param interface the name of the interface to look up.
     * @param vif the name of the vif to look up.
     * @return the ID of the face.
     * @throw BadFace if the interface was not found.
     */
    OlsrTypes::FaceID get_faceid(const string& interface, const string& vif)
	throw(BadFace);

    /**
     * Get the FEA names of an interface, given its OLSR interface ID.
     *
     * @param faceid the ID of the interface to look up.
     * @param interface the name of the interface if found.
     * @param vif the name of the vif if found.
     * @return true if the interface was found, otherwise false.
     */
    bool get_interface_vif_by_faceid(OlsrTypes::FaceID faceid,
	string & interface, string & vif);

    /**
     * Set the cost of an interface.
     * Used by shortest path calculation.
     *
     * @param faceid the ID of the interface to set cost for.
     * @param cost the interface cost to set.
     * @return true if the interface cost was set, otherwise false.
     */
    bool get_interface_cost(OlsrTypes::FaceID faceid, int& cost);

    /**
     * Enable the OLSR binding on the given interface.
     *
     * This method is responsible for realizing the
     * socket needed by the Face.
     * If more than one interface is enabled, this method MAY
     * start the MID protocol t imer.
     */
    bool set_face_enabled(OlsrTypes::FaceID faceid, bool enabled);

    /**
     * Get the "administratively up" status of an OLSR interface.
     *
     * @param faceid the ID of Face to query enabled state for.
     * @return true if faceid is enabled.
     */
    bool get_face_enabled(OlsrTypes::FaceID faceid);

    /**
     * Set the cost of an OLSR interface.
     *
     * @param faceid the ID of Face to set cost for.
     * @param cost the cost to set.
     * @return true if the cost was set successfully, otherwise false.
     */
    bool set_interface_cost(OlsrTypes::FaceID faceid, const int cost);

    /**
     * Set the IPv4 local address of an interface.
     *
     * @param faceid the ID of Face to set local address for.
     * @param local_addr the local address to set.
     * @return true if the local address was set successfully,
     *              otherwise false.
     */
    bool set_local_addr(OlsrTypes::FaceID faceid, const IPv4& local_addr);

    /**
     * Get the IPv4 local address of an interface.
     *
     * @param faceid the ID of Face to get local address for.
     * @param local_addr the local address, if interface was found.
     * @return true if the local address was retrieved successfully,
     *              otherwise false.
     */
    bool get_local_addr(OlsrTypes::FaceID faceid, IPv4& local_addr);

    /**
     * Set the UDP local port of an interface.
     *
     * @param faceid the ID of Face to set local port for.
     * @param local_port the local port to set.
     * @return true if the local port was set successfully,
     *              otherwise false.
     */
    bool set_local_port(OlsrTypes::FaceID faceid, const uint16_t local_port);

    /**
     * Get the UDP local port of an interface.
     *
     * Uses [] so can't be declared const.
     *
     * @param faceid the ID of Face to get local port for.
     * @param local_port the UDP local port, if interface was found.
     * @return true if the local port was retrieved successfully,
     *              otherwise false.
     */
    bool get_local_port(OlsrTypes::FaceID faceid, uint16_t& local_port);

    /**
     * Set the IPv4 all-nodes address of an interface.
     *
     * @param faceid the ID of Face to set all-nodes address for.
     * @param all_nodes_addr the all-nodes address to set.
     * @return true if the all-nodes address was set successfully,
     *              otherwise false.
     */
    bool set_all_nodes_addr(OlsrTypes::FaceID faceid,
			    const IPv4& all_nodes_addr);

    /**
     * Get the IPv4 all-nodes address of an interface.
     *
     * Uses [] so can't be declared const.
     *
     * @param faceid the ID of Face to get all-nodes address for.
     * @param all_nodes_addr output variable which will contain the
     *                       all-nodes address.
     * @return true if the all-nodes address was retrieved successfully,
     *              otherwise false.
     */
    bool get_all_nodes_addr(OlsrTypes::FaceID faceid,
			    IPv4& all_nodes_addr);

    /**
     * Set the UDP all-nodes port of an interface.
     *
     * @param faceid the ID of Face to set the all-nodes port for.
     * @param all_nodes_port the all-nodes port to set.
     * @return true if the all-nodes port was set successfully,
     *              otherwise false.
     */
    bool set_all_nodes_port(OlsrTypes::FaceID faceid,
			    const uint16_t all_nodes_port);

    /**
     * Get the UDP all-nodes port of an interface.
     *
     * @param faceid the ID of Face to get the all-nodes port for.
     * @param all_nodes_port output variable which will contain the
     *                       all-nodes port.
     * @return true if the all-nodes port was retrieved successfully,
     *              otherwise false.
     */
    bool get_all_nodes_port(OlsrTypes::FaceID faceid,
			    uint16_t& all_nodes_port);

    /**
     * Flood a message on all interfaces, performing appropriate
     * fragmentation for each interface.
     *
     * @param message The message to flood, will be deleted by this method.
     */
    bool flood_message(Message* message);

    /**
     * @return the next sequence number to use for a transmitted Message.
     */
    uint16_t get_msg_seqno() { return _next_msg_seqno++; }

    /**
     * Stop all timers (HELLO and MID).
     */
    void stop_all_timers();

    void start_hello_timer();
    void stop_hello_timer();
    void restart_hello_timer();

    /**
     * Reschedule the HELLO protocol timer, if its interval changed.
     */
    void reschedule_hello_timer();

    /**
     * Reschedule the HELLO protocol timer to fire as soon as possible.
     */
    void reschedule_immediate_hello_timer();

    /**
     * Broadcast a HELLO message on each enabled interface.
     */
    bool event_send_hello();

    void start_mid_timer();
    void stop_mid_timer();
    void restart_mid_timer();

    /**
     * Reschedule the MID protocol timer, if its interval changed.
     */
    void reschedule_mid_timer();

    /**
     * Reschedule the MID protocol timer to fire as soon as possible.
     */
    void reschedule_immediate_mid_timer();

    /**
     * Callback method to: Send a MID message on each enabled interface.
     */
    bool event_send_mid();

    /**
     * Set the HELLO_INTERVAL protocol variable.
     *
     * The HELLO timer will be rescheduled if running.
     */
    void set_hello_interval(const TimeVal& interval);

    /**
     * @return the HELLO_INTERVAL protocol variable.
     */
    TimeVal get_hello_interval() { return _hello_interval; }

    /**
     * @return the MAX_JITTER protocol variable.
     */
    TimeVal get_max_jitter() { return _hello_interval / 4; }

    /**
     * @return the number of interfaces enabled for OLSR.
     */
    inline uint32_t get_enabled_face_count() const {
	return _enabled_face_count;
    }

    /**
     * Register an OLSR protocol message handler.
     *
     * MessageReceiveCBs are invoked in reverse order to which they
     * have been registered. The FaceManager always registers a handler
     * for unknown message types first, so that such messages will be
     * forwarded, even if XORP has no handler for it. Each handler
     * thus registered is given an opportunity to claim the message
     * as its own, by returning true.
     *
     * C++ dynamic casts are used at runtime by each MessageReceiveCB
     * to determine if it should consume the message.
     *
     * @param cb the message receive callback function to register.
     */
    void add_message_cb(MessageReceiveCB cb);

    /**
     * Deregister an OLSR protocol message handler.
     *
     * @param cb the message receive callback function to deregister.
     * @return true if cb was deregistered, otherwise false.
     */
    bool delete_message_cb(MessageReceiveCB cb);

    /**
     * Callback method to: forward a Message of unknown type to
     * the rest of the OLSR topology.
     */
    bool event_receive_unknown(Message* msg, const IPv4& remote_addr,
	const IPv4& local_addr);

    /**
     * Implement the Default Forwarding Algorithm (Section 3.4.1).
     *
     * @param remote_addr the interface address of the node which sent or
     *                    forwarded this message to us.
     *                    Note: This may not be the origin if the message
     *                    has previously been forwarded.
     * @param msg the message itself.
     * @return true if the message will be forwarded to other nodes.
     */
    bool forward_message(const IPv4& remote_addr, Message* msg);

    /**
     * Check if a message is a previously seen duplicate.
     *
     * @return true if the message is a duplicate and should neither be
     * processed or forwarded.
     */
    bool is_duplicate_message(const Message* msg) const;

    /**
     * Check if a message should be forwarded, according to the default
     * forwarding rules.
     *
     * @param msg the message to check.
     * @return true if the message SHOULD NOT be forwarded, false if it is
     * OK to forward the message.
     */
    bool is_forwarded_message(const Message* msg) const;

    /**
     * Get a pointer to a tuple in the duplicate set for a message, given
     * its origin address and sequence number.
     *
     * Avoid throwing an exception as this path may be called
     * very frequently when forwarding or flooding messages.
     * This however means we return a pointer.
     *
     * @param origin_addr the protocol address of the message origin.
     * @param seqno the sequence number of the message.
     * @return the pointer to the duplicate set entry, or 0 if none exists.
     */
    DupeTuple* get_dupetuple(const IPv4& origin_addr,
			     const uint16_t seqno) const;

    /**
     * Update or create an entry in the duplicate message set.
     *
     * Given a message, update or create its duplicate set tuple in order
     * that we can detect if other nodes in the OLSR domain loop a message
     * back to us.
     * OLSR uses histogram based message loop detection, as the shortest
     * path tree may be out of phase with the real network topology.
     *
     * @param msg the message itself.
     * @param is_forwarded true if the message will be forwarded.
     */
    void update_dupetuple(const Message* msg, const bool is_forwarded);

    /**
     * Delete a duplicate set entry when it expires.
     */
    void event_dupetuple_expired(const IPv4& origin, const uint16_t seqno);

    /**
     * Clear the duplicate set.
    */
    void clear_dupetuples();

    /**
     * @return true if the given address belongs to any
     * locally configured interface.
     */
    bool is_local_addr(const IPv4& addr);

    /**
     * Get the statistics for the given face.
     *
     * @param ifname the name of the interface to retrieve stats for.
     * @param vifname the name of the vif to retrieve stats for.
     * @param stats output; reference to an empty FaceCounters object.
     * @return true if the stats were retrieved, otherwise false.
     */
    bool get_face_stats(const string& ifname, const string& vifname,
			FaceCounters& stats);

  private:
    Olsr&		_olsr;
    EventLoop&		_eventloop;
    MessageDecoder	_md;
    Neighborhood*	_nh;

    OlsrTypes::FaceID	_next_faceid;
    uint32_t		_enabled_face_count;

    /**
     * @short The next available message sequence number.
     */
    uint16_t		_next_msg_seqno;

    /**
     * @short primary protocol address of this node;
     * used as 'origin' for OLSR control messages.
     */
    IPv4		_main_addr;

    /**
     * @short Vector of message handlers.
     */
    vector<MessageReceiveCB>		_handlers;

    /**
     * @short Map interface/vif to OlsrTypes::FaceID.
     */
    map<string, OlsrTypes::FaceID>	_faceid_map;

    /**
     * @short Map FaceID to Face.
     */
    map<OlsrTypes::FaceID, Face*>	_faces;

    /**
     * Maintain state about messages we may have already processed
     * and/or forwarded. HELLO messages are specifically excluded.
     * Section 3.4: Duplicate Tuple.
     */
    typedef multimap<IPv4, DupeTuple*>	DupeTupleMap;
    DupeTupleMap			_duplicate_set;

   /**
     * @short The HELLO_INTERVAL protocol control variable.
     * RFC 3626 Section 18.2.
     */
    TimeVal	_hello_interval;
    XorpTimer	_hello_timer;

    /**
     * @short The MID_INTERVAL protocol control variable.
     * RFC 3626 Section 18.2.
     */
    TimeVal	_mid_interval;
    XorpTimer	_mid_timer;

    /**
     * @short The DUP_HOLD_TIME protocol control variable.
     * Section 3.4.1: Default Forwarding Algorithm.
     */
    TimeVal	_dup_hold_time;

    /**
     * @short true if a MID message should be queued as soon as
     * the set of configured interfaces changes.
     */
    bool	_is_early_mid_enabled;
};

#endif // __OLSR_FACE_MANAGER_HH__
