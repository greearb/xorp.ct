// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

// $XORP: xorp/bgp/peer.hh,v 1.52 2008/10/02 21:56:17 bms Exp $

#ifndef __BGP_PEER_HH__
#define __BGP_PEER_HH__

#include <sys/types.h>

#include "socket.hh"
#include "local_data.hh"
#include "peer_data.hh"

enum FSMState {
    STATEIDLE = 1,
    STATECONNECT = 2,
    STATEACTIVE = 3,
    STATEOPENSENT = 4,
    STATEOPENCONFIRM = 5,
    STATEESTABLISHED = 6,
    STATESTOPPED = 7	// This state is not in the protocol specification. 
			// After sending a notify it is necessary to
			// close the connection. Data transmission/reception 
			// is asynchronous, but the close is currently
			// synchronous. Thus the stopped state allows
			// us to wait for the notify to be sent to TCP,
			// before closing the connection.
};

enum FSMEvent {
    EVENTBGPSTART = 1,
    EVENTBGPSTOP = 2,
    EVENTBGPTRANOPEN = 3,
    EVENTBGPTRANCLOSED = 4,
    EVENTBGPCONNOPENFAIL = 5,
    EVENTBGPTRANFATALERR = 6,
    EVENTCONNTIMEEXP = 7,
    EVENTHOLDTIMEEXP = 8,
    EVENTKEEPALIVEEXP = 9,
    EVENTRECOPENMESS = 10,
    EVENTRECKEEPALIVEMESS = 11,
    EVENTRECUPDATEMESS = 12,
    EVENTRECNOTMESS = 13
};

enum PeerOutputState {
    PEER_OUTPUT_OK = 1,
    PEER_OUTPUT_BUSY = 2,
    PEER_OUTPUT_FAIL = 3
};

#define OPENMSGOK 0
#define UPDATEMSGOK 0

const uint32_t RIB_IPC_HANDLER_UNIQUE_ID = 0;
const uint32_t AGGR_HANDLER_UNIQUE_ID = 1;
const uint32_t UNIQUE_ID_START = AGGR_HANDLER_UNIQUE_ID + 1;

class BGPMain;
class PeerHandler;
class AcceptSession;

/**
 * Manage the damping of peer oscillations.
 */
class DampPeerOscillations {
 public:
    DampPeerOscillations(EventLoop& eventloop, uint32_t restart_threshold,
			 uint32_t time_period, uint32_t idle_holdtime);

    /**
     * The session has been restarted.
     */
    void restart();

    /**
     * @return the idle holdtime. 
     */
    uint32_t idle_holdtime() const;

    /**
     * Reset the state, possibly due to a manual restart. 
     */
    void reset();
 private:
    EventLoop& _eventloop;		// Reference to the eventloop.
    const uint32_t _restart_threshold;	// Number of restart after
					// which the idle holdtime
					// will increase.
    const uint32_t _time_period;	// Period in seconds over
					// which to sample errors.
    const uint32_t _idle_holdtime;	// Holdtime in seconds to use once the
					// error threshold is passed.

    uint32_t _restart_counter;		// Count number of restarts in
					// last time quantum.

    XorpTimer _zero_restart;		// Zero the restart counter.

    /**
     * Used by the timer to zero the error count.
     */
    void zero_restart_count();
};

class BGPPeer {
public:
    BGPPeer(LocalData *ld, BGPPeerData *pd, SocketClient *sock, BGPMain *m);
    virtual ~BGPPeer();

    /**
     * Get this peers unique ID.
     */
    uint32_t get_unique_id() const	{ return _unique_id; }

    /**
     * Zero all the stats counters.
     */
    void zero_stats();

    /**
     * Clear the last error.
     */
    void clear_last_error();

    /**
     * Replace the old peerdata with a new copy. It is the
     * responsiblity of the caller to free the memory.
     */
    BGPPeerData *swap_peerdata(BGPPeerData *pd) {
	BGPPeerData *tmp = _peerdata;
	_peerdata = pd;

	return tmp;
    }

    void connected(XorpFd s);
    void remove_accept_attempt(AcceptSession *conn);
    SocketClient *swap_sockets(SocketClient *new_sock);
    XorpFd get_sock();

    /**
     * state machine handlers for the various BGP events
     */
    void event_start();			// EVENTBGPSTART
    void event_stop(bool restart=false, bool automatic = false);// EVENTBGPSTOP
    void event_open();			// EVENTBGPTRANOPEN
    void event_open(const XorpFd sock);	// EVENTBGPTRANOPEN
    void event_closed();		// EVENTBGPTRANCLOSED
    void event_openfail();		// EVENTBGPCONNOPENFAIL
    void event_tranfatal();		// EVENTBGPTRANFATALERR
    void event_connexp();		// EVENTCONNTIMEEXP
    void event_holdexp();		// EVENTHOLDTIMEEXP
    void event_keepexp();		// EVENTKEEPALIVEEXP
    void event_delay_open_exp();	// Event 12: DelayOpenTimer_Expires
    void event_idle_hold_exp();		// Event 13: IdleHoldTimer_Expires
    void event_openmess(const OpenPacket& p);	// EVENTRECOPENMESS
    void event_keepmess();			// EVENTRECKEEPALIVEMESS
    void event_recvupdate(UpdatePacket& p); 	// EVENTRECUPDATEMESS
    void event_recvnotify(const NotificationPacket& p); // EVENTRECNOTMESS

    void generate_open_message(OpenPacket& open);
    void notify_peer_of_error(const int error,
			      const int subcode = UNSPECIFIED,
			      const uint8_t*data = 0,
			      const size_t len = 0);

    FSMState state()			{ return _state; }
    static const char *pretty_print_state(FSMState s);

    /**
     * If jitter is globally enabled apply it to the time provided otherwise
     * just return the input time.
     */
    TimeVal jitter(const TimeVal& t);

    void clear_all_timers();
    void start_connect_retry_timer();
    void clear_connect_retry_timer();
    void restart_connect_retry_timer();

    void start_keepalive_timer();
    void clear_keepalive_timer();

    void start_hold_timer();
    void clear_hold_timer();
    void restart_hold_timer();

    void start_stopped_timer();
    void clear_stopped_timer();

    void start_idle_hold_timer();
    void clear_idle_hold_timer();
    /**
     * @return true if the idle hold timer is running.
     */
    bool running_idle_hold_timer() const;

    void start_delay_open_timer();
    void clear_delay_open_timer();

    bool get_message(BGPPacket::Status status, const uint8_t *buf, size_t len,
		     SocketClient *socket_client);
    PeerOutputState send_message(const BGPPacket& p);
    void send_message_complete(SocketClient::Event, const uint8_t *buf);

    string str() const			{ return _peername; }
    bool is_connected() const		{ return _SocketClient->is_connected(); }
    bool still_reading() const		{ return _SocketClient->still_reading(); }
    LocalData* localdata()              { return _localdata; }
    IPv4 id() const		        { return _localdata->get_id(); }
    BGPMain* main() const		{ return _mainprocess; }
    const BGPPeerData* peerdata() const	{ return _peerdata; }
    bool ibgp() const			{ return peerdata()->ibgp(); }
    bool use_4byte_asnums() const { 
	return _peerdata->use_4byte_asnums(); 
    }
    bool we_use_4byte_asnums() const { 
	return _localdata->use_4byte_asnums(); 
    }

    /**
     * send the netreachability message, return send result.
     */
    bool send_netreachability(const BGPUpdateAttrib &n);
    /*
    ** Virtual so that it can be subclassed in the plumbing test code.
    */
    virtual PeerOutputState send_update_message(const UpdatePacket& p);

    uint32_t get_established_transitions() const {
	return _established_transitions;
    }
    uint32_t get_established_time() const;
    void get_msg_stats(uint32_t& in_updates, 
		       uint32_t& out_updates, 
		       uint32_t& in_msgs, 
		       uint32_t& out_msgs, 
		       uint16_t& last_error, 
		       uint32_t& in_update_elapsed) const;
protected:
private:
    LocalData* _localdata;

    /**
     * For the processing in decision every peer requires a unique ID.
     */
    static uint32_t _unique_id_allocator;
    const uint32_t _unique_id;

    friend class BGPPeerList;

    void connect_to_peer(SocketClient::ConnectCallback cb) {
	XLOG_ASSERT(_SocketClient);
	_SocketClient->connect(cb);
    }
    void connect_to_peer_complete(bool success) {
	if (success)
	    event_open();		// Event = EVENTBGPTRANOPEN
	else
	    event_openfail();		// Event = EVENTBGPCONNOPENFAIL
    }

    void send_notification(const NotificationPacket& p, bool restart = true,
			   bool automatic = true);
    void send_notification_complete(SocketClient::Event, const uint8_t *buf,
				    bool restart, bool automatic);
    void flush_transmit_queue()		{ _SocketClient->flush_transmit_queue(); }
    void stop_reader()			{ _SocketClient->stop_reader(); }

    SocketClient *_SocketClient;
    bool _output_queue_was_busy;
    FSMState _state;

    BGPPeerData* _peerdata;
    BGPMain* _mainprocess;
    PeerHandler *_handler;
    list<AcceptSession *> _accept_attempt;
    string _peername;

    XorpTimer _timer_connect_retry;
    XorpTimer _timer_hold_time;
    XorpTimer _timer_keep_alive;
    XorpTimer _idle_hold;
    XorpTimer _delay_open;

    // counters needed for the BGP MIB
    uint32_t _in_updates;
    uint32_t _out_updates;
    uint32_t _in_total_messages;
    uint32_t _out_total_messages;
    uint8_t _last_error[2];
    uint32_t _established_transitions;
    TimeVal _established_time;
    TimeVal _in_update_time;

    /**
     * This timer is to break us out of the stopped state.
     */
    XorpTimer _timer_stopped;
    void hook_stopped();

    void check_open_packet(const OpenPacket *p) throw (CorruptMessage);
    NotificationPacket* check_update_packet(const UpdatePacket *p,
					    bool& good_nexthop);

    /**
     * Called the first time that we go to the established state.
     */
    bool established();

    bool release_resources();

    /**
     * move to the desired state, plus does some additional
     * work to clean up existing state and possibly retrying to
     * open/connect if restart = true
     *
     * @param restart if true and this is a transition to idle restart
     * the connection.
     * @param automatic if the transition is to idle and automatic restart has
     * been request. This is not a manual restart.
     */
    void set_state(FSMState s, bool restart = true, bool automatic = true);
    bool remote_ip_ge_than(const BGPPeer& peer);

    bool _damping_peer_oscillations;	// True if Damp Peer
					// Oscillations is enabled.
    DampPeerOscillations _damp_peer_oscillations;
    
    /**
     * Called every time there is an automatic restart. Used for
     * tracking peer damp oscillations.
     */
    void automatic_restart();
    
private:
    friend class BGPMain;

    bool _current_state;
    void set_current_peer_state(bool state) {_current_state = state;}
    bool get_current_peer_state() {return _current_state;}

    bool _next_state;
    void set_next_peer_state(bool state) {_next_state = state;}
    bool get_next_peer_state() {return _next_state;}

    bool _activated;
    void set_activate_state(bool state) {_activated = state;}
    bool get_activate_state() {return _activated;}
};

/*
 * All incoming TCP connection attempts are handled through this class.
 * The BGPPeer class handles outgoing connection attempts.
 *
 * Under normal circumstances only one connection attempt will be
 * taking place. When both BGP processes at either end of a session
 * attempt to make a connection at the same time there may be a
 * connection collision in this case it is necessary to hold two TCP
 * connections until an open message is seen by the peer to decide
 * which session should be selected. If a connection collision is
 * detected this class does *not* send an open message, it waits for
 * the peers open message. It should be noted that the BGPPeer class
 * is not aware that a connection collision condition exists, hence it
 * does not check the IDs. The assumptions are that:
 * 1) The IDs will be identical in both open messages so it's only
 *    necessary to check one of the two open messages.
 * 2) A BGP process will actually send a open message after making a
 *    connection. 
 *
 * This class could be used to get rid of the XORP invented STOPPED
 * state in state machine.
 */
class AcceptSession {
 public:
     AcceptSession(BGPPeer& peer, XorpFd sock);

     ~AcceptSession();

     /**
      * Start the FSM.
      */
     void start();

     /**
      * Timeout routine that is called if no messages are seen from the
      * peer. Ideally an open message should be seen from the peer.
      */
     void no_open_received();

     /**
      * This FSM has done its job signal to the peer to remove this
      * class. This should be the last method to be called in any methods.
      */
     void remove();

     /**
      * Send a notification.
      */
     void send_notification_accept(const NotificationPacket& np);

     /**
      * Notification callback.
      */
     void send_notification_cb(SocketClient::Event ev, const uint8_t* buf);

     /**
      * Send a cease.
      */
     void cease();

     /**
      * The main FSM is in state OPENCONFIRM so both IDs are available
      * to resolve the collision.
      */
     void collision();

     /**
      * An open message has just been received on the accept socket
      * decide and keep the winner.
      */
     void event_openmess_accept(const OpenPacket& p);

     /**
      * Swap the socket in this class with the one in the main FSM.
      */
     void swap_sockets();

     /**
      * Replace this socket with the one in the main FSM and feed in
      * an open packet.
      */
     void swap_sockets(const OpenPacket& p);


     void notify_peer_of_error_accept(const int error,
				      const int subcode = UNSPECIFIED,
				      const uint8_t*data = 0,
				      const size_t len = 0);

     void event_tranfatal_accept();

    /**
     * Called if the TCP connection is closed.
     */ 
     void event_closed_accept();

    /**
     * Called if a keepalive message is seen.
     */ 
     void event_keepmess_accept();

    /**
     * Called if a update message is seen.
     */ 
     void event_recvupdate_accept(const UpdatePacket& p);

    /**
     * Called if a notify message is seen.
     */ 
     void event_recvnotify_accept(const NotificationPacket& p);

    /**
     * Handle incoming messages.
     */ 
     bool get_message_accept(BGPPacket::Status status, const uint8_t *buf,
			     size_t len, SocketClient *socket_client);

     bool is_connected() { return _socket_client->is_connected(); }

     bool still_reading() { return _socket_client->still_reading(); }

     void ignore_message() { _accept_messages = false; }

     bool accept_message() const { return _accept_messages; }

     string str() {
	 return _peer.str();
     }

 private:
     BGPPeer& _peer;
     XorpFd _sock;
     SocketClient *_socket_client;
     bool _accept_messages;

     XorpTimer _open_wait;	// Wait for an open message from the peer.

     BGPMain *main()			{ return _peer.main(); }
     FSMState state()			{ return _peer.state(); }
     const BGPPeerData* peerdata() const{ return _peer.peerdata(); }
     bool running_idle_hold_timer() const {
	return _peer.running_idle_hold_timer();
     } 
};

#endif // __BGP_PEER_HH__
