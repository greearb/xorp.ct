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

// $XORP: xorp/bgp/peer.hh,v 1.1.1.1 2002/12/11 23:55:49 hodson Exp $

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
    STATESTOPPED = 7
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

    FSMState get_ConnectionState();
    void set_ConnectionState(FSMState s);

    void connected(int s);
    int get_sock();

    bool same_sockaddr(struct sockaddr_in *sinp);

    void action(FSMEvent e, const int sock);
    void action(FSMEvent ConnectionEvent, const int error, const int subcode,
		const uint8_t*data = 0, const size_t len = 0);
    void action(FSMEvent e, const BGPPacket *p = NULL);

    void clear_all_timers();
    void start_connect_retry_timer();
    void clear_connect_retry_timer();
    void restart_connect_retry_timer();
    void start_keepalive_timer();
    void clear_keepalive_timer();
    void restart_keepalive_timer();
    void start_hold_timer();
    void clear_hold_timer();
    void restart_hold_timer();
    void start_stopped_timer();
    void clear_stopped_timer();

    bool get_message(BGPPacket::Status status, const uint8_t *buf, size_t len);
    PeerOutputState send_message(const BGPPacket& p);
    void send_message_complete(SocketClient::Event, const uint8_t *buf);

    static void check_hold_duration(int32_t hold_secs) throw(CorruptMessage);

    string str();
    bool is_connected();
    bool still_reading();
    LocalData* _localdata;
    BGPMain* main() { return _mainprocess; }
    const BGPPeerData* peerdata() { return _peerdata; }
    bool ibgp() { return peerdata()->get_internal_peer(); }
    bool send_netreachability(const NetLayerReachability &n);
    /*
    ** Virtual so that it can be subclassed in the plumbing test code.
    */
    virtual PeerOutputState send_update_message(const UpdatePacket& p);
protected:
private:
    bool connect_to_peer();
    void state_machine(FSMEvent e, const BGPPacket *p = NULL);
    void send_notification(const NotificationPacket& p, bool error = true) {
	send_notification(&p, error);
    }
    void send_notification(const NotificationPacket *p, bool error = true);
    void send_notification_complete(SocketClient::Event, const uint8_t *buf,
				    bool error);
    void flush_transmit_queue();
    void stop_reader();

    SocketClient *_SocketClient;
    bool _output_queue_was_busy;
    FSMState _ConnectionState;
    FSMEvent _ConnectionEvent;

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
    struct timeval _established_time;
    struct timeval _in_update_time;

    /**
     * This timer is to break us out of the stopped state.
     */
    XorpTimer _timer_stopped;
    void hook_stopped();
#ifdef	LATER
    XorpTimer _timer_min_as_orig_interval;
    XorpTimer _timer_min_route_adv_interval;
#endif

    bool check_send_keepalive();
    uint8_t check_open_packet(const OpenPacket *p);
    NotificationPacket* check_update_packet(const UpdatePacket *p);

    bool initialise_stage1();
    bool initialise_stage2();
    /**
     * Called the first time that we go to the established state.
     */
    bool established();

    bool release_resources();

    void set_state(FSMState s, bool error = false);
};

void hook_connect_retry(BGPPeer *);
void hook_hold_time(BGPPeer *);
void hook_keep_alive(BGPPeer *);
void hook_min_as_orig_interval(BGPPeer *);
void hook_min_route_adv_interval(BGPPeer *);

#endif // __BGP_PEER_HH__
