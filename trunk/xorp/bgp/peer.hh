// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

// $XORP: xorp/bgp/peer.hh,v 1.15 2004/05/13 18:49:17 atanu Exp $

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

class BGPMain;
class PeerHandler;

class BGPPeer {
public:
    BGPPeer(LocalData *ld, BGPPeerData *pd, SocketClient *sock, BGPMain *m);
    virtual ~BGPPeer();

    void connected(int s);
    int get_sock();

    bool same_sockaddr(struct sockaddr_in *sinp);

    /**
     * state machine handlers for the various BGP events
     */
    void event_start();			// EVENTBGPSTART
    void event_stop();			// EVENTBGPSTOP
    void event_open();			// EVENTBGPTRANOPEN
    void event_open(const int sock);	// EVENTBGPTRANOPEN
    void event_closed();		// EVENTBGPTRANCLOSED
    void event_openfail();		// EVENTBGPCONNOPENFAIL
    void event_tranfatal();		// EVENTBGPTRANFATALERR
    void event_connexp();		// EVENTCONNTIMEEXP
    void event_holdexp();		// EVENTHOLDTIMEEXP
    void event_keepexp();		// EVENTKEEPALIVEEXP
    void event_openmess(const OpenPacket& p);	// EVENTRECOPENMESS
    void event_keepmess();			// EVENTRECKEEPALIVEMESS
    void event_recvupdate(const UpdatePacket& p); 	// EVENTRECUPDATEMESS
    void event_recvnotify(const NotificationPacket& p); // EVENTRECNOTMESS

    void notify_peer_of_error(const int error, const int subcode,
		const uint8_t*data = 0, const size_t len = 0);

    FSMState state()			{ return _state; }
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

    bool get_message(BGPPacket::Status status, const uint8_t *buf, size_t len);
    PeerOutputState send_message(const BGPPacket& p);
    void send_message_complete(SocketClient::Event, const uint8_t *buf);

    string str();
    bool is_connected() const		{ return _SocketClient->is_connected(); }
    bool still_reading() const		{ return _SocketClient->still_reading(); }
    LocalData* _localdata;
    BGPMain* main()			{ return _mainprocess; }
    const BGPPeerData* peerdata() const	{ return _peerdata; }
    bool ibgp() const			{ return peerdata()->get_internal_peer(); }

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

    friend class BGPPeerList;

    void connect_to_peer(SocketClient::ConnectCallback cb) {
	_SocketClient->connect(cb);
    }
    void connect_to_peer_complete(bool success) {
	if (success)
	    event_open();		// Event = EVENTBGPTRANOPEN
	else
	    event_openfail();		// Event = EVENTBGPCONNOPENFAIL
    }

#if 0
    void send_notification(const NotificationPacket& p, bool error = true) {
	send_notification(&p, error);
    }
    void send_notification(const NotificationPacket *p, bool error = true);
#endif
    void send_notification(const NotificationPacket& p, bool error = true);
    void send_notification_complete(SocketClient::Event, const uint8_t *buf,
				    bool error);
    void flush_transmit_queue()		{ _SocketClient->flush_transmit_queue(); }
    void stop_reader()			{ _SocketClient->stop_reader(); }

    SocketClient *_SocketClient;
    bool _output_queue_was_busy;
    FSMState _state;

    BGPPeerData* _peerdata;
    BGPMain* _mainprocess;
    PeerHandler *_handler;

    XorpTimer _timer_connect_retry;
    XorpTimer _timer_hold_time;
    XorpTimer _timer_keep_alive;

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
    NotificationPacket* check_update_packet(const UpdatePacket *p);

    /**
     * Called the first time that we go to the established state.
     */
    bool established();

    bool release_resources();

    /**
     * move to the desired state, plus does some additional
     * work to clean up existing state and possibly retrying to
     * open/connect if error = true
     */
    void set_state(FSMState s, bool error = false);
    static const char *pretty_print_state(FSMState s);
    bool remote_ip_ge_than(const BGPPeer& peer);
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

#endif // __BGP_PEER_HH__
