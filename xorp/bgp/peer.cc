// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME
// #define SAVE_PACKETS
#define CHECK_TIME

#include "bgp_module.h"
#include "libxorp/xorp.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/timer.hh"
#include "libxorp/timespent.hh"

#include "libcomm/comm_api.h"

#include "libproto/packet.hh"

#include "peer.hh"
#include "bgp.hh"
#include "profile_vars.hh"

#define DEBUG_BGPPeer

uint32_t BGPPeer::_unique_id_allocator = UNIQUE_ID_START;

#if 0
inline void trap_callback(const XrlError& error, const char *comment);
#endif

BGPPeer::BGPPeer(LocalData *ld, BGPPeerData *pd, SocketClient *sock,
		 BGPMain *m) 
    : _unique_id(_unique_id_allocator++),
      _damping_peer_oscillations(true),
      _damp_peer_oscillations(m->eventloop(),
			      10,	/* restart threshold */
			      5 * 60,	/* time period */
			      2 * 60 	/* idle holdtime */)
{
    debug_msg("BGPPeer constructor called (1)\n");

    _localdata = ld;
    _peerdata = pd;
    _mainprocess = m;
    _state = STATEIDLE;
    _SocketClient = sock;
    _output_queue_was_busy = false;
    _handler = NULL;
    _peername = c_format("Peer-%s", peerdata()->iptuple().str().c_str());

    zero_stats();

    _current_state = _next_state = _activated = false;
}

BGPPeer::~BGPPeer()
{
    delete _SocketClient;
    delete _peerdata;
    list<AcceptSession *>::iterator i;
    for (i = _accept_attempt.begin(); i != _accept_attempt.end(); i++)
	delete (*i);
    _accept_attempt.clear();
}

void
BGPPeer::zero_stats()
{
    _in_updates = 0;
    _out_updates = 0;
    _in_total_messages = 0;
    _out_total_messages = 0;
    _last_error[0] = 0;
    _last_error[1] = 0;
    _established_transitions = 0;
    _mainprocess->eventloop().current_time(_established_time);
    _mainprocess->eventloop().current_time(_in_update_time);
}

void
BGPPeer::clear_last_error()
{
    _last_error[0] = 0;
    _last_error[1] = 0;
}

/*
 * This call is dispatched by the reader after making sure that
 * we have a packet with at least a BGP common header and whose
 * length matches the one in that header.
 * Our job now is to decode the message and dispatch it to the
 * state machine.
 */
bool
BGPPeer::get_message(BGPPacket::Status status, const uint8_t *buf,
		     size_t length, SocketClient *socket_client)
{
    XLOG_ASSERT(0 == socket_client || _SocketClient == socket_client);

    PROFILE(if (main()->profile().enabled(profile_message_in))
		main()->profile().log(profile_message_in,
				      c_format("message on %s len %u",
					       str().c_str(),
					       XORP_UINT_CAST(length))));
	
    TIMESPENT();

    switch (status) {
    case BGPPacket::GOOD_MESSAGE:
	break;

    case BGPPacket::ILLEGAL_MESSAGE_LENGTH:
	notify_peer_of_error(MSGHEADERERR, BADMESSLEN,
			     buf + BGPPacket::MARKER_SIZE, 2);
// 	event_tranfatal();
	TIMESPENT_CHECK();
	return false;

    case BGPPacket::CONNECTION_CLOSED:
 	// _SocketClient->disconnect();
 	event_closed();
	// XLOG_ASSERT(!is_connected());
	TIMESPENT_CHECK();
	return false;
    }

#ifdef	SAVE_PACKETS
    // XXX
    // This is a horrible hack to try and find a problem before the
    // 1.1 Release.
    // All packets will be stored in fname, in MRTD format, if they
    // arrived on an IPv4 peering. 
    // The route_btoa program can be used to print the packets.
    // TODO after the release:
    // 1) Move the structures into a header file in libxorp.
    // 2) Add an XRL to enable the saving of incoming packets on a
    // per peer basis.
    // 3) Handle IPv6 peerings.
    // 4) Save all the values that are required by mrt_update.
    // 5) Move all this code into a separate method.
    // 6) Keep the file open.
    // 7) Don't call gettimeofday directly, get the time from the eventloop.

#ifndef HOST_OS_WINDOWS
    string fname = "/tmp/bgpin.mrtd";
#else
    string fname = "C:\\BGPIN.MRTD";
#endif

    FILE *fp = fopen(fname.c_str(), "a");
    if(0 == fp)
	XLOG_FATAL("fopen of %s failed: %s", fname.c_str(),
		   strerror(errno));

    struct mrt_header {
	uint32_t time;
	uint16_t type;
	uint16_t subtype;
	uint32_t length;
    };

    struct mrt_update {
	uint16_t source_as;
	uint16_t dest_as;
	uint16_t ifindex;
	uint16_t af;
	uint32_t source_ip;
	uint32_t dest_ip;
    };

    TimeVal tv;
    TimerList::system_gettimeofday(&tv);
    
    mrt_header mrt_header;
    mrt_header.time = htonl(tv.sec());
    mrt_header.type = htons(16);
    mrt_header.subtype = htons(1);
    mrt_header.length = htonl(length + sizeof(mrt_update));
    
    if(fwrite(&mrt_header, sizeof(mrt_header), 1, fp) != 1)
	XLOG_FATAL("fwrite of %s failed: %s", fname.c_str(), strerror(errno));

    mrt_update update;
    memset(&update, 0, sizeof(update));
    update.af = htons(1);	/* IPv4 */

    string peer_addr = peerdata()->iptuple().get_peer_addr();
    try {
	update.source_as = htons(peerdata()->as().as());
	update.source_ip = IPv4(peer_addr.c_str()).addr();

	if(fwrite(&update, sizeof(update), 1, fp) != 1)
	    XLOG_FATAL("fwrite of %s failed: %s", fname.c_str(),
		       strerror(errno));

	if(fwrite(buf, length, 1, fp) != 1)
	    XLOG_FATAL("fwrite of %s failed: %s", fname.c_str(),
		       strerror(errno));
    } catch(InvalidFamily &e) {
	XLOG_ERROR("%s might not be an IPv4 address %s", peer_addr.c_str(),
		   e.str().c_str());
    }
	
    fclose(fp);
#endif

    _in_total_messages++;

    /*
    ** We should only have a good packet at this point.
    ** The buffer pointer must be non 0.
    */
    XLOG_ASSERT(0 != buf);

    const uint8_t* marker = buf + BGPPacket::MARKER_OFFSET;
    uint8_t type = extract_8(buf + BGPPacket::TYPE_OFFSET);
    try {

	/*
	** Check the Marker, total waste of time as it never contains
	** anything of interest.
	*/
	if (0 != memcmp(const_cast<uint8_t *>(&BGPPacket::Marker[0]),
			marker, BGPPacket::MARKER_SIZE)) {
	    xorp_throw(CorruptMessage,"Bad Marker", MSGHEADERERR, CONNNOTSYNC);
	}
	
	switch (type) {
	case MESSAGETYPEOPEN: {
	    debug_msg("OPEN Packet RECEIVED\n");
	    OpenPacket pac(buf, length);
	    PROFILE(XLOG_TRACE(main()->profile().enabled(trace_message_in),
			       "Peer %s: Receive: %s",
			       peerdata()->iptuple().str().c_str(),
			       cstring(pac)));
	    // All decode errors should throw a CorruptMessage.
	    debug_msg("%s", pac.str().c_str());
	    // want unified decode call. now need to get peerdata out.
	    _peerdata->dump_peer_data();
	    event_openmess(pac);
	    TIMESPENT_CHECK();
	    break;
	}
	case MESSAGETYPEKEEPALIVE: {
	    debug_msg("KEEPALIVE Packet RECEIVED %u\n",
		      XORP_UINT_CAST(length));
	    // Check that the length is correct or throw an exception
	    KeepAlivePacket pac(buf, length);
	    PROFILE(XLOG_TRACE(main()->profile().enabled(trace_message_in),
			       "Peer %s: Receive: %s",
			       peerdata()->iptuple().str().c_str(),
			       cstring(pac)));

	    // debug_msg("%s", pac.str().c_str());
	    event_keepmess();
	    TIMESPENT_CHECK();
	    break;
	}
	case MESSAGETYPEUPDATE: {
	    debug_msg("UPDATE Packet RECEIVED\n");
	    _in_updates++;
	    _mainprocess->eventloop().current_time(_in_update_time);
	    UpdatePacket pac(buf, length, _peerdata, _mainprocess, /*do checks*/true);

	    PROFILE(XLOG_TRACE(main()->profile().enabled(trace_message_in),
			       "Peer %s: Receive: %s",
			       peerdata()->iptuple().str().c_str(),
			       cstring(pac)));

	    // All decode errors should throw a CorruptMessage.
	    debug_msg("%s", pac.str().c_str());

	    event_recvupdate(pac);
	    TIMESPENT_CHECK();
	    if (TIMESPENT_OVERLIMIT()) {
		XLOG_WARNING("Processing packet took longer than %u second %s",
			     XORP_UINT_CAST(TIMESPENT_LIMIT),
			     pac.str().c_str());
	    }
	    break;
	}
	case MESSAGETYPENOTIFICATION: {
	    debug_msg("NOTIFICATION Packet RECEIVED\n");
	    NotificationPacket pac(buf, length);
	    
	    PROFILE(XLOG_TRACE(main()->profile().enabled(trace_message_in),
			       "Peer %s: Receive: %s",
			       peerdata()->iptuple().str().c_str(),
			       cstring(pac)));

	    // All decode errors should throw a CorruptMessage.
	    debug_msg("%s", pac.str().c_str());
	    event_recvnotify(pac);
	    TIMESPENT_CHECK();
	    break;
	}
	default:
	    /*
	    ** Send a notification to the peer. This is a bad message type.
	    */
	    XLOG_ERROR("%s Unknown packet type %d",
		       this->str().c_str(), type);
	    notify_peer_of_error(MSGHEADERERR, BADMESSTYPE,
				 buf + BGPPacket::TYPE_OFFSET, 1);
// 	    event_tranfatal();
	    TIMESPENT_CHECK();
	    return false;
	}
    } catch(CorruptMessage& c) {
	/*
	** This peer has sent us a bad message. Send a notification
	** and drop the the peering.
	*/
	XLOG_WARNING("%s %s %s", this->str().c_str(), c.where().c_str(),
		     c.why().c_str());
	notify_peer_of_error(c.error(), c.subcode(), c.data(), c.len());
// 	event_tranfatal();
	TIMESPENT_CHECK();
	return false;
    } catch (UnusableMessage& um) {
	// the packet wasn't usable for some reason, but also
	// wasn't so corrupt we need to send a notification -
	// this is a "silent" error.
	XLOG_WARNING("%s %s %s", this->str().c_str(), um.where().c_str(),
		     um.why().c_str());
    }

    TIMESPENT_CHECK();

    /*
    ** If we are still connected and supposed to be reading.
    */
    if (!is_connected() || !still_reading()) {
	TIMESPENT_CHECK();
	return false;
    }

    return true;
}

PeerOutputState
BGPPeer::send_message(const BGPPacket& p)
{
    debug_msg("%s", p.str().c_str());

    PROFILE(XLOG_TRACE(main()->profile().enabled(trace_message_out),
		       "Peer %s: Send: %s",
		       peerdata()->iptuple().str().c_str(),
		       cstring(p)));

    uint8_t packet_type = p.type();

    if ( packet_type != MESSAGETYPEOPEN &&
	 packet_type != MESSAGETYPEUPDATE &&
	 packet_type != MESSAGETYPENOTIFICATION &&
	 packet_type != MESSAGETYPEKEEPALIVE) {
	xorp_throw(InvalidPacket,
		   c_format("Unknown packet type %d\n", packet_type));

    }

    _out_total_messages++;
    if (packet_type == MESSAGETYPEUPDATE)
	_out_updates++;

    uint8_t *buf = new uint8_t[BGPPacket::MAXPACKETSIZE];
    size_t ccnt = BGPPacket::MAXPACKETSIZE;

    /*
    ** This buffer is dynamically allocated and should be freed.
    */
    XLOG_ASSERT(p.encode(buf, ccnt, _peerdata));
    debug_msg("Buffer for sent packet is %p\n", buf);


    /*
    ** This write is async. So we can't free the data now,
    ** we will deal with it in the complete routine.
    */
    bool ret = _SocketClient->send_message(buf, ccnt,
			       callback(this,&BGPPeer::send_message_complete));

    if (ret == false)
	delete[] buf;
    if (ret) {
	int size = _SocketClient->output_queue_size();
	UNUSED(size);
	debug_msg("Output queue size is %d\n", size);
	if (_SocketClient->output_queue_busy()) {
	    _output_queue_was_busy = true;
	    return PEER_OUTPUT_BUSY;
	} else
	    return PEER_OUTPUT_OK;
    } else
	return PEER_OUTPUT_FAIL;
}

void
BGPPeer::send_message_complete(SocketClient::Event ev, const uint8_t *buf)
{
    TIMESPENT();

    debug_msg("Packet sent, queue size now %d\n",
	    _SocketClient->output_queue_size());

    switch (ev) {
    case SocketClient::DATA:
	debug_msg("event: data\n");
	if (_output_queue_was_busy &&
	    (_SocketClient->output_queue_busy() == false)) {
	    debug_msg("Peer: output no longer busy\n");
	    _output_queue_was_busy = false;
	    if (_handler != NULL)
		_handler->output_no_longer_busy();
	}
	TIMESPENT_CHECK();
	/* fall through */
    case SocketClient::FLUSHING:
	debug_msg("event: flushing\n");
	debug_msg("Freeing Buffer for sent packet: %p\n", buf);
	delete[] buf;
	TIMESPENT_CHECK();
	break;
    case SocketClient::ERROR:
	// The most likely cause of an error is that the peer closed
	// the connection.
	debug_msg("event: error\n");
	/* Don't free the message here we'll get it in the flush */
	// XLOG_ERROR("Writing buffer failed: %s",  strerror(errno));
	// _SocketClient->disconnect();
	event_closed();
	// XLOG_ASSERT(!is_connected());
	TIMESPENT_CHECK();
    }
}

void
BGPPeer::send_notification(const NotificationPacket& p, bool restart,
			   bool automatic)
{
    debug_msg("%s", p.str().c_str());

    XLOG_INFO("Sending: %s", cstring(p));

    PROFILE(XLOG_TRACE(main()->profile().enabled(trace_message_out),
		       "Peer %s: Send: %s",
		       peerdata()->iptuple().str().c_str(),
		       cstring(p)));

    /*
    ** We need to deal with NOTIFICATION differently from other packets -
    ** NOTIFICATION is the last packet we send on a connection, and because
    ** we're using async sends, we need to chain the rest of the
    ** cleanup on the send complete callback.
    */

    /*
     * First we need to clear the transmit queue - we no longer care
     * about anything that's in it, and we want to make sure that the
     * transmit complete callbacks can no longer happen.
     */
    flush_transmit_queue();

    /*
    ** Don't read anything more on this connection.
    */
    stop_reader();

    /*
     * This buffer is dynamically allocated and should be freed.
     */
    size_t ccnt = BGPPacket::MAXPACKETSIZE;
    uint8_t *buf = new uint8_t[BGPPacket::MAXPACKETSIZE];

    XLOG_ASSERT(p.encode(buf, ccnt, _peerdata));
    debug_msg("Buffer for sent packet is %p\n", buf);

    /*
    ** This write is async. So we can't free the data now,
    ** we will deal with it in the complete routine.  */
    bool ret =_SocketClient->send_message(buf, ccnt,
	       callback(this, &BGPPeer::send_notification_complete,
			restart, automatic));

    if (!ret) {
	delete[] buf;
	return;
    }

}

void
BGPPeer::send_notification_complete(SocketClient::Event ev,
				    const uint8_t* buf, bool restart,
				    bool automatic)
{
    TIMESPENT();

    switch (ev) {
    case SocketClient::DATA:
	debug_msg("Notification sent\n");
	XLOG_ASSERT(STATESTOPPED == _state);
	delete[] buf;
	set_state(STATEIDLE, restart);
	break;
    case SocketClient::FLUSHING:
	delete[] buf;
	break;
    case SocketClient::ERROR:
	debug_msg("Notification not sent\n");
	XLOG_ASSERT(STATESTOPPED == _state);
	set_state(STATEIDLE, restart, automatic);
	/* Don't free the message here we'll get it in the flush */
	break;
    }

    /* there doesn't appear to be anything we can do at this point if
       status indicates something went wrong */
}

/*
** Timer callback
*/
void
BGPPeer::hook_stopped()
{
    XLOG_ASSERT(STATESTOPPED == _state);
    XLOG_WARNING("%s Unable to send Notification so taking peer to idle",
		 this->str().c_str());

    /*
    ** If the original notification was not an error such as sending a
    ** CEASE. If we arrived here due to a timeout, something has gone
    ** wrong so unconditionally set the restart to true.
    */
    set_state(STATEIDLE);
}

/*
 * Here begins the BGP state machine.
 * It is split into multiple functions, each one dealing with a
 * specific event.
 */

/*
 * STATESTOPPED is the state we're in after we've queued a
 * CEASE notification, but before it's actually been sent.  We
 * ignore packet events, but a further STOP event or error
 * will cause is to stop waiting for the CEASE to be sent, and
 * a START event will cause us to transition to idle, and then
 * process the START event as normal
 */

/*
 * start event. No data associated with the event.
 */
void
BGPPeer::event_start()			// EVENTBGPSTART
{ 
    TIMESPENT();

    // Compute the type of this peering.
    const_cast<BGPPeerData*>(peerdata())->compute_peer_type();

    switch(_state) {

    case STATESTOPPED:
	flush_transmit_queue();		// ensure callback can't happen
	set_state(STATEIDLE, false);// go through STATEIDLE to clear resources
	// fallthrough now to process the start event
	/* fall through */
    case STATEIDLE:
	// Initalise resources
	start_connect_retry_timer();
	set_state(STATECONNECT);
	connect_to_peer(callback(this, &BGPPeer::connect_to_peer_complete));
	break;

    // in all other cases, remain in the same state
    case STATECONNECT:
    case STATEACTIVE:
    case STATEOPENSENT:
    case STATEOPENCONFIRM:
    case STATEESTABLISHED:
	break;
    }
}

/*
 * stop event. No data associated with the event.
 */
void
BGPPeer::event_stop(bool restart, bool automatic)	// EVENTBGPSTOP
{
    TIMESPENT();

    switch(_state) {
    case STATEIDLE:
	break;

    case STATECONNECT:
	_SocketClient->connect_break();
	clear_connect_retry_timer();
	/*FALLTHROUGH*/

    case STATEACTIVE:
	set_state(STATEIDLE, restart, automatic);
	break;

    case STATEOPENSENT:
    case STATEOPENCONFIRM:
    case STATEESTABLISHED: {
	// Send Notification Message with error code of CEASE.
	NotificationPacket np(CEASE);
	send_notification(np, restart, automatic);
	set_state(STATESTOPPED, restart, automatic);
	break;
    }
    case STATESTOPPED:
	// a second stop will cause us to give up on sending the CEASE
	flush_transmit_queue();		// ensure callback can't happen
	set_state(STATEIDLE, restart, automatic);
	break;
    }
}

/*
 * EVENTBGPTRANOPEN event. No data associated with the event.
 * We only call this when a connect() or an accept() succeeded,
 * and we have a valid socket.
 * The only states from which we enter here are STATECONNECT and STATEACTIVE
 */
void
BGPPeer::event_open()	// EVENTBGPTRANOPEN
{ 
    TIMESPENT();

    switch(_state) {
    case STATEOPENSENT:
    case STATEOPENCONFIRM:
    case STATEESTABLISHED:
    case STATESTOPPED:
    case STATEIDLE:
	XLOG_FATAL("%s can't get EVENTBGPTRANOPEN in state %s",
		   this->str().c_str(),
		   pretty_print_state(_state));
	break;

    case STATECONNECT:
    case STATEACTIVE: {
	if (0 != _peerdata->get_delay_open_time()) {
	    start_delay_open_timer();
	    clear_connect_retry_timer();
	    return;
	}

	OpenPacket open_packet(_peerdata->my_AS_number(),
			       _localdata->get_id(),
			       _peerdata->get_configured_hold_time());

#if	0
	ParameterList::const_iterator
	    pi = _peerdata->parameter_sent_list().begin();
	while(pi != _peerdata->parameter_sent_list().end()) {
	    if ((*pi)->send())
		open_packet.add_parameter(*pi);
	    pi++;
	}
#endif
	generate_open_message(open_packet);
	send_message(open_packet);

	clear_connect_retry_timer();
	if ((_state == STATEACTIVE) || (_state == STATECONNECT)) {
	    // Start Holdtimer - four minutes recommended in spec.
	    _peerdata->set_hold_duration(4 * 60);
	    start_hold_timer();
	}
	// Change state to OpenSent
	set_state(STATEOPENSENT);
	break;
    }
    }
}

/*
 * EVENTBGPTRANCLOSED event. No data associated with the event.
 */
void
BGPPeer::event_closed()			// EVENTBGPTRANCLOSED
{ 
    TIMESPENT();

    switch(_state) {
    case STATEIDLE:
	break;

    case STATECONNECT:
	if (_SocketClient->is_connected())
	    _SocketClient->connect_break();
	clear_connect_retry_timer();
	set_state(STATEIDLE);
	break;

    case STATEACTIVE:
	set_state(STATEIDLE);
	break;

    case STATEOPENSENT:
	// Close Connection
	_SocketClient->disconnect();
	// Restart ConnectRetry Timer
	restart_connect_retry_timer();
	set_state(STATEACTIVE);
	break;

    case STATEOPENCONFIRM:
    case STATEESTABLISHED:
	set_state(STATEIDLE);
	break;

    case STATESTOPPED:
	flush_transmit_queue();		// ensure callback can't happen
	set_state(STATEIDLE);
	break;
    }
}

/*
 * EVENTBGPCONNOPENFAIL event. No data associated with the event.
 * We can only be in STATECONNECT
 */
void
BGPPeer::event_openfail()			// EVENTBGPCONNOPENFAIL
{ 
    TIMESPENT();

    switch(_state) {
    case STATEIDLE:
    case STATEACTIVE:
    case STATEOPENSENT:
    case STATEOPENCONFIRM:
    case STATEESTABLISHED:
    case STATESTOPPED:
	XLOG_FATAL("%s can't get EVENTBGPCONNOPENFAIL in state %s",
		   this->str().c_str(),
		   pretty_print_state(_state));
	break;

    case STATECONNECT:
	if (_peerdata->get_delay_open_time() == 0) {
	    //
	    // If the DelayOpenTimer is not running, we have to go first
	    // through STATEIDLE.
	    // XXX: Instead of checking the configured DelayOpenTime value,
	    // we should check whether the corresponding timer is indeed
	    // not running. However, there is no dedicated DelayOpenTimer
	    // (the _idle_hold timer is reused instead), hence we check
	    // the corresponding DelayOpenTime configured value.
	    //
	    set_state(STATEIDLE, false);
	}
	restart_connect_retry_timer();
	set_state(STATEACTIVE);		// Continue to listen for a connection
	break;
    }
}

/*
 * EVENTBGPTRANFATALERR event. No data associated with the event.
 */
void
BGPPeer::event_tranfatal()			// EVENTBGPTRANFATALERR
{ 
    TIMESPENT();

    switch(_state) {
    case STATEIDLE:
	break;

    case STATECONNECT:
    case STATEACTIVE:
	set_state(STATEIDLE);
	break;

    case STATEOPENSENT:
    case STATEOPENCONFIRM:
    case STATEESTABLISHED:
	set_state(STATEIDLE);
	break;

    case STATESTOPPED:
	// ensure callback can't happen
	flush_transmit_queue();
	set_state(STATEIDLE);
	break;
    }
}

/*
 * EVENTCONNTIMEEXP event. No data associated with the event.
 * This is a timer hook called from a connection_retry_timer expiring
 * (it is a oneoff timer)
 */
void
BGPPeer::event_connexp()			// EVENTCONNTIMEEXP
{
    TIMESPENT();

    switch(_state) {
    case STATEIDLE:
    case STATESTOPPED:
	break;

    case STATECONNECT:
	restart_connect_retry_timer();
	_SocketClient->connect_break();	
	connect_to_peer(callback(this, &BGPPeer::connect_to_peer_complete));
	break;

    case STATEACTIVE:
	restart_connect_retry_timer();
	set_state(STATECONNECT);
	connect_to_peer(callback(this, &BGPPeer::connect_to_peer_complete));
	break;

    /*
     * if these happen, we failed to properly cancel a timer (?)
     */
    case STATEOPENSENT:
    case STATEOPENCONFIRM:
    case STATEESTABLISHED: {
	// Send Notification Message with error code of FSM error.
	// XXX this needs to be revised.
	XLOG_WARNING("%s FSM received EVENTCONNTIMEEXP in state %s",
		     this->str().c_str(),
		     pretty_print_state(_state));
	NotificationPacket np(FSMERROR);
	send_notification(np);
	set_state(STATESTOPPED, true);
	break;
    }
    }
}

/*
 * EVENTHOLDTIMEEXP event. No data associated with the event.
 * Only called by a hold_timer (oneoff) expiring.
 * If it goes off, the connection is over so we do not start the timer again.
 */
void
BGPPeer::event_holdexp()			// EVENTHOLDTIMEEXP
{ 
    TIMESPENT();

    switch(_state) {
    case STATEIDLE:
    case STATESTOPPED:
	break;

    case STATECONNECT:
    case STATEACTIVE:
	set_state(STATEIDLE);
	break;

    case STATEOPENSENT:
    case STATEOPENCONFIRM:
    case STATEESTABLISHED: {
	// Send Notification Message with error code of Hold Timer expired.
	NotificationPacket np(HOLDTIMEEXP);
	send_notification(np);
	set_state(STATESTOPPED);
	break;
    }
    }
}

/*
 * EVENTKEEPALIVEEXP event.
 * A keepalive timer expired.
 */
void
BGPPeer::event_keepexp()			// EVENTKEEPALIVEEXP
{ 
    TIMESPENT();

    switch(_state) {
    case STATEIDLE:
    case STATESTOPPED:
    case STATECONNECT:
    case STATEACTIVE:
    case STATEOPENSENT:
	XLOG_FATAL("%s FSM received EVENTKEEPALIVEEXP in state %s",
		   this->str().c_str(),
		   pretty_print_state(_state));
	break;

    case STATEOPENCONFIRM:
    case STATEESTABLISHED:
	start_keepalive_timer();
	KeepAlivePacket kp;
	send_message(kp);
	break;
    }
}

void
BGPPeer::event_delay_open_exp()
{ 
    TIMESPENT();
    
    switch(_state) {
    case STATEIDLE:
    case STATESTOPPED:
    case STATEOPENSENT:
    case STATEESTABLISHED: {
	XLOG_WARNING("%s FSM received EVENTRECOPENMESS in state %s",
		     this->str().c_str(),
		     pretty_print_state(_state));
	NotificationPacket np(FSMERROR);
	send_notification(np);
	set_state(STATESTOPPED);
    }
	break;
    case STATECONNECT:
    case STATEACTIVE:
    case STATEOPENCONFIRM: {
	OpenPacket open_packet(_peerdata->my_AS_number(),
			       _localdata->get_id(),
			       _peerdata->get_configured_hold_time());
	generate_open_message(open_packet);
	send_message(open_packet);

	if ((_state == STATEACTIVE) || (_state == STATECONNECT)) {
	    // Start Holdtimer - four minutes recommended in spec.
	    _peerdata->set_hold_duration(4 * 60);
	    start_hold_timer();
	}
	// Change state to OpenSent
	set_state(STATEOPENSENT);
    }
	break;
    }
}

void
BGPPeer::event_idle_hold_exp()
{ 
    TIMESPENT();

    XLOG_ASSERT(state() == STATEIDLE);
    event_start();
}

/*
 * EVENTRECOPENMESS event.
 * We receive an open packet.
 */
void
BGPPeer::event_openmess(const OpenPacket& p)		// EVENTRECOPENMESS
{
    TIMESPENT();

    switch(_state) {
    case STATECONNECT:
    case STATEACTIVE: {
	// The only way to get here is due to a delayed open.
	// Kill the delay open timer and send the open packet.
	clear_delay_open_timer();
	OpenPacket open_packet(_peerdata->my_AS_number(),
			       _localdata->get_id(),
			       _peerdata->get_configured_hold_time());
	generate_open_message(open_packet);
	send_message(open_packet);
    }
	/* FALLTHROUGH */
    case STATEOPENSENT:
	// Process OPEN MESSAGE
	try {
	    check_open_packet(&p);
	    // We liked the open packet continue, trying to setup session.
	    KeepAlivePacket kp;
	    send_message(kp);

	    // start timers
	    debug_msg("Starting timers\n");
	    clear_all_timers();
	    start_keepalive_timer();
	    start_hold_timer();

	    // Save the parameters from the open packet.
	    _peerdata->save_parameters(p.parameter_list());

	    // Compare 
	    _peerdata->open_negotiation();

	    set_state(STATEOPENCONFIRM);
	} catch(CorruptMessage& c) {
	    XLOG_WARNING("%s %s", this->str().c_str(), c.why().c_str());
	    notify_peer_of_error(c.error(), c.subcode(), c.data(), c.len());
	}
	break;

    case STATEIDLE:
    case STATEOPENCONFIRM:
    case STATEESTABLISHED: {
	// Send Notification - FSM error
	XLOG_WARNING("%s FSM received EVENTRECOPENMESS in state %s",
		     this->str().c_str(),
		     pretty_print_state(_state));
	notify_peer_of_error(FSMERROR);
	break;
    }
    case STATESTOPPED:
	break;
    }
}

/*
 * EVENTRECKEEPALIVEMESS event.
 * We received a keepalive message.
 */
void
BGPPeer::event_keepmess()			// EVENTRECKEEPALIVEMESS
{ 
    TIMESPENT();

    switch(_state) {
    case STATEIDLE:
    case STATECONNECT:
    case STATEACTIVE:
	XLOG_FATAL("%s FSM received EVENTRECKEEPALIVEMESS in state %s",
		   this->str().c_str(),
		   pretty_print_state(_state));
	break;

    case STATESTOPPED:
	break;

    case STATEOPENSENT: {
	// Send Notification Message with error code of FSM error.
	XLOG_WARNING("%s FSM received EVENTRECKEEPALIVEMESS in state %s",
		     this->str().c_str(),
		     pretty_print_state(_state));
	NotificationPacket np(FSMERROR);
	send_notification(np);
	set_state(STATESTOPPED);
	break;
    }
    case STATEOPENCONFIRM:	// this is what we were waiting for.
	set_state(STATEESTABLISHED);
	/* FALLTHROUGH */

    case STATEESTABLISHED:	// this is a legitimate message.
	restart_hold_timer();
	break;
    }
}


/*
 * EVENTRECUPDATEMESS event.
 * We received an update message.
 */
void
BGPPeer::event_recvupdate(UpdatePacket& p) // EVENTRECUPDATEMESS
{ 
    TIMESPENT();

    switch(_state) {
    case STATEIDLE:
    case STATECONNECT:
    case STATEACTIVE: {
	XLOG_WARNING("%s FSM received EVENTRECUPDATEMESS in state %s",
		     this->str().c_str(),
		     pretty_print_state(_state));
	NotificationPacket np(FSMERROR);
	send_notification(np);
	set_state(STATESTOPPED);
    }
	break;

    case STATEOPENSENT:
    case STATEOPENCONFIRM: {
	// Send Notification Message with error code of FSM error.
	XLOG_WARNING("%s FSM received EVENTRECUPDATEMESS in state %s",
		     this->str().c_str(),
		     pretty_print_state(_state));
	NotificationPacket np(FSMERROR);
	send_notification(np);
	set_state(STATESTOPPED);
	break;
    }
    case STATEESTABLISHED: {
	restart_hold_timer();

	// Check that the prefix limit if set will not be exceeded.
	ConfigVar<uint32_t> &prefix_limit =
	    const_cast<BGPPeerData *>(peerdata())->get_prefix_limit();
	if (prefix_limit.get_enabled()) {
	    if ((_handler->get_prefix_count() + p.nlri_list().size())
		> prefix_limit.get_var()) {
		NotificationPacket np(CEASE);
		send_notification(np);
		set_state(STATESTOPPED);
		break;
	    }
	}

	// process the packet...
	debug_msg("Process the packet!!!\n");
	XLOG_ASSERT(_handler);
	// XXX - Temporary hack until we get programmable filters.
	const IPv4 next_hop = peerdata()->get_next_hop_rewrite();
	if (!next_hop.is_zero()) {
	    FPAList4Ref l = p.pa_list();
	    if (l->nexthop_att()) {
		l->replace_nexthop(next_hop);
	    }
	}
	_handler->process_update_packet(&p);

	break;
    }

    case STATESTOPPED:
	break;
    }
}

/*
 * EVENTRECNOTMESS event.
 * We received a notify message. Currently we ignore the payload.
 */
void
BGPPeer::event_recvnotify(const NotificationPacket& p)	// EVENTRECNOTMESS
{ 
    TIMESPENT();

    XLOG_INFO("%s in state %s received %s",
	      this->str().c_str(),
	      pretty_print_state(_state),
	      p.str().c_str());

    _last_error[0] = p.error_code();
    _last_error[1] = p.error_subcode();

    switch(_state) {
    case STATEIDLE:
	XLOG_FATAL("%s FSM received EVENTRECNOTMESS in state %s",
		   this->str().c_str(),
		   pretty_print_state(_state));
	break;

    case STATECONNECT:
    case STATEACTIVE:
    case STATEOPENSENT:
    case STATEOPENCONFIRM:
    case STATEESTABLISHED:
	set_state(STATEIDLE);
	break;

    case STATESTOPPED:
	break;
    }
}

void
BGPPeer::generate_open_message(OpenPacket& open)
{
    uint8_t last_error_code = _last_error[0];
    uint8_t last_error_subcode = _last_error[1];
    bool ignore_cap_optional_parameters = false;
    ParameterList::const_iterator pi;

    if ((last_error_code == OPENMSGERROR)
	&& (last_error_subcode == UNSUPOPTPAR)) {
	//
	// XXX: If the last error was Unsupported Optional Parameter, then
	// don't include the Capabilities Optional Parameter.
	//
	ignore_cap_optional_parameters = true;
    }

    for (pi = _peerdata->parameter_sent_list().begin();
	 pi != _peerdata->parameter_sent_list().end();
	 ++pi) {
	if (ignore_cap_optional_parameters) {
	    // XXX: ignore the Capabilities Optional Parameters.
	    const ref_ptr<const BGPParameter>& ref_par = *pi;
	    const BGPParameter& par = *ref_par;
	    const BGPCapParameter* cap_par;
	    cap_par = dynamic_cast<const BGPCapParameter*>(&par);
	    if (cap_par != NULL)
		continue;
	}
	open.add_parameter(*pi);
    }
}

/*
** A BGP Fatal error has ocurred. Try and send a notification.
*/
void
BGPPeer::notify_peer_of_error(const int error, const int subcode,
		const uint8_t *data, const size_t len)
{
    if (!NotificationPacket::validate_error_code(error, subcode)) {
	XLOG_WARNING("%s Attempt to send invalid error code %d subcode %d",
		     this->str().c_str(),
		     error, subcode); // XXX make it fatal ?
    }

    /*
     * An error has occurred. If we still have a connection to the
     * peer send a notification of the error.
     */
    if (is_connected()) {
	NotificationPacket np(error, subcode, data, len);
	send_notification(np);
	set_state(STATESTOPPED);
	return;
    }

    // The peer is no longer connected make sure we get to idle.
    event_tranfatal();
}

/*
 * Handle incoming connection events.
 *
 * The argment is the incoming socket descriptor, which
 * must be used or closed by this method.
 */
void
BGPPeer::event_open(const XorpFd sock)
{
    debug_msg("Connection attempt: State %d ", _state);
    if (_state == STATECONNECT || _state == STATEACTIVE) {
	debug_msg("accepted\n");
	if (_state == STATECONNECT)
	    _SocketClient->connect_break();
	_SocketClient->connected(sock);
	event_open();
    } else {
	debug_msg("rejected\n");
	XLOG_INFO("%s rejecting connection: current state %s",
		  this->str().c_str(),
		  pretty_print_state(_state));
	comm_sock_close(sock);
    }
}

void
BGPPeer::check_open_packet(const OpenPacket *p) throw(CorruptMessage)
{
    if (p->Version() != BGPVERSION) {
	static uint8_t data[2];
	embed_16(data, BGPVERSION);
	xorp_throw(CorruptMessage,
		   c_format("Unsupported BGPVERSION %d", p->Version()),
		   OPENMSGERROR, UNSUPVERNUM, &data[0], sizeof(data));
    }

    if (p->as() != _peerdata->as()) {
	debug_msg("**** Peer has %s, should have %s ****\n",
		  p->as().str().c_str(),
		  _peerdata->as().str().c_str());
	xorp_throw(CorruptMessage,
		   c_format("Wrong AS %s expecting %s",
			    p->as().str().c_str(),
			    _peerdata->as().str().c_str()),
		   OPENMSGERROR, BADASPEER);
    }

    // Must be a valid unicast IP host address.
    if (!p->id().is_unicast() || p->id().is_zero()) {
	xorp_throw(CorruptMessage,
		   c_format("Not a valid unicast IP host address %s",
			    p->id().str().c_str()),
		   OPENMSGERROR, BADBGPIDENT);
    }

    // This has to be a valid IPv4 address.
    _peerdata->set_id(p->id());

    // put received parameters into the peer data.
//     _peerdata->clone_parameters(  p->parameter_list() );
    // check the received parameters
#if	0
    if (_peerdata->unsupported_parameters() == true)
	xorp_throw(CorruptMessage,
		   c_format("Unsupported parameters"),
		   OPENMSGERROR, UNSUPOPTPAR);
#endif
    /*
     * Set the holdtime and keepalive times.
     *
     * Internally we store the values in milliseconds. The value sent
     * on the wire is in seconds. First check that we have been
     * offered a legal value. If the value is legal convert it to
     * milliseconds.  Compare the holdtime in the packet against our
     * configured value and choose the lowest value.
     *
     * The spec suggests using a keepalive time of a third the hold
     * duration.
     */
    uint16_t hold_secs = p->HoldTime();
    if (hold_secs == 1 || hold_secs == 2)
	xorp_throw(CorruptMessage,
		   c_format("Illegal holdtime value %d secs", hold_secs),
		   OPENMSGERROR, UNACCEPTHOLDTIME);

    if (_peerdata->get_configured_hold_time() < hold_secs)
	hold_secs = _peerdata->get_configured_hold_time();

    _peerdata->set_hold_duration(hold_secs);
    _peerdata->set_keepalive_duration(hold_secs / 3);

    /*
    ** Any unrecognised optional parameters would already have caused
    ** any exception to be thrown in the open packet decoder.
    */

    _peerdata->dump_peer_data();
    debug_msg("check_open_packet says it's OK with us\n");
}

#define	REMOVE_UNNEGOTIATED_NLRI
inline
bool
check_multiprotocol_nlri(const UpdatePacket *, const BGPUpdateAttribList& pa,
			 bool neg)
{
    if (!pa.empty() && !neg) {
#ifdef	REMOVE_UNNEGOTIATED_NLRI
	// We didn't advertise this capability, so strip this sucker out.
	const_cast<BGPUpdateAttribList *>(&pa)->clear();
#else
	// We didn't advertise this capability, drop peering.
 	return false;
#endif
    }

    return true;
}

#if 0
inline
bool
check_multiprotocol_nlri(const UpdatePacket *p, PathAttribute *pa, bool neg)
{
    if (pa && !neg) {
#ifdef	REMOVE_UNNEGOTIATED_NLRI
	// We didn't advertise this capability, so strip this sucker out.
	PathAttributeList<IPv4>& tmp = 
	    const_cast<PathAttributeList<IPv4>&>(p->pa_list());
	tmp.remove_attribute_by_pointer(pa);
#else
	// We didn't advertise this capability, drop peering.
 	return false;
#endif
    }

    return true;
}
#endif

NotificationPacket *
BGPPeer::check_update_packet(const UpdatePacket *p, bool& good_nexthop)
{
    UNUSED(p);
    UNUSED(good_nexthop);
    // return codes
    // 1 - Malformed Attribute List
    // 2 - Unrecognized Well-known Attribute
    // 3 - Missing Well-known Attribute
    // 4 - Attribute Flags Error
    // 5 - Attribute Length Error
    // 6 - Invalid ORIGIN Attribute
    // 8 - Invalid NEXT-HOP Attribute
    // 9 - Optional Attribute Error
    // 10 - Invalid Network Field
    // 11 - Malformed AS_PATH

    /*
    ** The maximum message size is 4096 bytes. We don't need an
    ** explicit check here as the packet construction routines will
    ** already have generated an error if the maximum size had been
    ** exceeded.
    */

#if 0
    /*
    ** Check for multiprotocol parameters that haven't been "negotiated".
    **
    ** There are two questions that seem to ambiguous in the BGP specs.
    **
    ** 1) How do we deal with a multiprotocol attribute that hasn't
    **    been "negotiated".
    **	  a) Strip out the offending attribute.
    **	  b) Send a notify and drop the peering.
    ** 2) In order to accept a multiprotol attribute. I.E to consider
    **    it to have been "negotiated", what is the criteria.
    **	  a) Is it sufficient for this speaker to have announced the
    **    capability.
    **	  b) Do both speakers in a session need to announce a capability.
    **
    ** For the time being we are going with 1) (a) and 2 (a).
    */

    good_nexthop = true;

//     BGPPeerData::Direction dir = BGPPeerData::NEGOTIATED;
    BGPPeerData::Direction dir = BGPPeerData::SENT;
    bool bad_nlri = false;
#define STOP_IPV4_UNICAST
#ifdef	STOP_IPV4_UNICAST
    if (!check_multiprotocol_nlri(p, p->nlri_list(),
				  peerdata()->
				  multiprotocol<IPv4>(SAFI_UNICAST, dir)))
	bad_nlri = true;
    if (!check_multiprotocol_nlri(p, p->wr_list(),
				  peerdata()->
				  multiprotocol<IPv4>(SAFI_UNICAST, dir)))
	bad_nlri = true;
#endif
    if (!check_multiprotocol_nlri(p, p->mpreach<IPv4>(SAFI_MULTICAST),
				  peerdata()->
				  multiprotocol<IPv4>(SAFI_MULTICAST, dir)))
	bad_nlri = true;
    if (!check_multiprotocol_nlri(p, p->mpunreach<IPv4>(SAFI_MULTICAST),
				  peerdata()->
				  multiprotocol<IPv4>(SAFI_MULTICAST, dir)))
	bad_nlri = true;
    if (!check_multiprotocol_nlri(p, p->mpreach<IPv6>(SAFI_UNICAST),
				  peerdata()->
				  multiprotocol<IPv6>(SAFI_UNICAST, dir)))
	bad_nlri = true;
    if (!check_multiprotocol_nlri(p, p->mpunreach<IPv6>(SAFI_UNICAST),
				  peerdata()->
				  multiprotocol<IPv6>(SAFI_UNICAST, dir)))
	bad_nlri = true;
    if (!check_multiprotocol_nlri(p, p->mpreach<IPv6>(SAFI_MULTICAST),
				  peerdata()->
				  multiprotocol<IPv6>(SAFI_MULTICAST, dir)))
	bad_nlri = true;
    if (!check_multiprotocol_nlri(p, p->mpunreach<IPv6>(SAFI_MULTICAST),
				  peerdata()->
				  multiprotocol<IPv6>(SAFI_MULTICAST, dir)))
	bad_nlri = true;
#ifndef	REMOVE_UNNEGOTIATED_NLRI
    if (bad_nlri)
	return new NotificationPacket(UPDATEMSGERR, OPTATTR);
#endif

#endif
    // if the path attributes list is empty then no network
    // layer reachability information should be present.

#if 0
    if ( p->pa_list().empty() ) {
	if ( p->nlri_list().empty() == false )	{
	    debug_msg("Empty path attribute list and "
		      "non-empty NLRI list\n");
	    return new NotificationPacket(UPDATEMSGERR, MALATTRLIST);
	}
    } else {
	// if we have path attributes, check that they are valid.
	bool local_pref = false;
	bool route_reflector = false;

	list<PathAttribute*> l = p->pa_list();
	list<PathAttribute*>::const_iterator i;
	ASPathAttribute* as_path_attr = NULL;
	OriginAttribute* origin_attr = NULL;
	NextHopAttribute<IPv4>* next_hop_attr = NULL;
	set <PathAttType> prev_attrs;
	for (i = l.begin(); i != l.end(); i++) {
	    // check there are no duplicate attributes
	    if (prev_attrs.find((*i)->type()) != prev_attrs.end())
		return new NotificationPacket(UPDATEMSGERR, MALATTRLIST);
	    prev_attrs.insert((*i)->type());

	    switch ((*i)->type()) {
	    case ORIGIN:
		origin_attr = (OriginAttribute*)(*i);
		break;
	    case AS_PATH:
		as_path_attr = (ASPathAttribute*)(*i);
		break;
	    case NEXT_HOP:
		next_hop_attr = (NextHopAttribute<IPv4>*)(*i);
		break;
	    case LOCAL_PREF:
		local_pref = true;
		break;
	    case MED:
		break;
	    case ATOMIC_AGGREGATE:
		break;
	    case AGGREGATOR:
		break;
	    case COMMUNITY:
		break;
	    case ORIGINATOR_ID:
		/* FALLTHROUGH */
	    case CLUSTER_LIST:
		route_reflector = true;
		break;
	    default:
		if ((*i)->well_known()) {
		    // unrecognized well_known attribute.
		    uint8_t buf[8192];
		    size_t wire_size = 8192;
		    (*i)->encode(buf, wire_size, _peerdata);
		    return new NotificationPacket(UPDATEMSGERR, UNRECOGWATTR,
						  buf, wire_size);
		}
	    }
	}

	/*
	** If a NLRI attribute is present check for the following
	** mandatory fields: 
	** ORIGIN
	** AS_PATH
	** NEXT_HOP
	*/
	if ( p->nlri_list().empty() == false ||
	     p->mpreach<IPv4>(SAFI_MULTICAST) ||
	     p->mpreach<IPv6>(SAFI_UNICAST)   ||
	     p->mpreach<IPv6>(SAFI_MULTICAST)) {
	    // The ORIGIN Path attribute is mandatory 
	    if (origin_attr == NULL) {
		debug_msg("Missing ORIGIN\n");
		uint8_t data = ORIGIN;
		return new 
		    NotificationPacket(UPDATEMSGERR, MISSWATTR, &data, 1);
	    }

	    // The AS Path attribute is mandatory
	    if (as_path_attr == NULL) {
		debug_msg("Missing AS_PATH\n");
		uint8_t data = AS_PATH;
		return new 
		    NotificationPacket(UPDATEMSGERR, MISSWATTR, &data, 1);
	    }

	    // The NEXT_HOP attribute is mandatory for IPv4 unicast.
	    // For multiprotocol NLRI its in the path attribute.
	    if (p->nlri_list().empty() == false && next_hop_attr == NULL) {
		debug_msg("Missing NEXT_HOP\n");
		uint8_t data = NEXT_HOP;
		return new 
		    NotificationPacket(UPDATEMSGERR, MISSWATTR, &data, 1);
	    }

	    // In a multiprotocol NLRI message there is always a nexthop,
	    // just check that its not zero.
	    if (p->mpreach<IPv4>(SAFI_MULTICAST) && 
		p->mpreach<IPv4>(SAFI_MULTICAST)->nexthop() == IPv4::ZERO()) {
		uint8_t data = NEXT_HOP;
		return new 
		    NotificationPacket(UPDATEMSGERR, MISSWATTR, &data, 1);
	    }

	    if (p->mpreach<IPv6>(SAFI_UNICAST) && 
		p->mpreach<IPv6>(SAFI_UNICAST)->nexthop() == IPv6::ZERO()) {
		uint8_t data = NEXT_HOP;
		return new 
		    NotificationPacket(UPDATEMSGERR, MISSWATTR, &data, 1);
	    }

	    if (p->mpreach<IPv6>(SAFI_MULTICAST) && 
		p->mpreach<IPv6>(SAFI_MULTICAST)->nexthop() == IPv6::ZERO()) {
		uint8_t data = NEXT_HOP;
		return new 
		    NotificationPacket(UPDATEMSGERR, MISSWATTR, &data, 1);
	    }
	}

	// If we got this far and as_path_attr is not set this is a
	// multiprotocol withdraw.
	if (as_path_attr != NULL) {
	    if (!ibgp()) {
		// If this is an EBGP peering, the AS Path MUST NOT be empty
		if (as_path_attr->as_path().path_length() == 0)
		    return new NotificationPacket(UPDATEMSGERR, MALASPATH);

		// If this is an EBGP peering, the AS Path MUST start
		// with the AS number of the peer.
		AsNum my_asnum(peerdata()->as());
		if (as_path_attr->as_path().first_asnum() != my_asnum)
		    return new NotificationPacket(UPDATEMSGERR, MALASPATH);

		// If this is an EBGP peering and a route reflector
		// attribute has been received then generate an error.
		if (route_reflector)
		    return new NotificationPacket(UPDATEMSGERR, MALATTRLIST);
	    }
	    // Receiving confederation path segments when the router
	    // is not configured for confederations is an error. 
	    if (!_peerdata->confederation() &&
		as_path_attr->as_path().contains_confed_segments())
		    return new NotificationPacket(UPDATEMSGERR, MALASPATH);
	}

	// If an update message is received that contains a nexthop
	// that belongs to this router then discard the update, don't
	// send a notification.

	if (next_hop_attr != NULL) {
	    if (_mainprocess->interface_address4(next_hop_attr->nexthop())) {
		XLOG_ERROR("Nexthop in update belongs to this router:\n %s",
			   cstring(*p));
		good_nexthop = false;
		return 0;
	    }
	}

	if (p->mpreach<IPv4>(SAFI_MULTICAST)) {
	    if (_mainprocess->
		interface_address4(p->mpreach<IPv4>(SAFI_MULTICAST)->
				   nexthop())) {
		XLOG_ERROR("Nexthop in update belongs to this router:\n %s",
			   cstring(*p));
		good_nexthop = false;
		return 0;
	    }
	}

	if (p->mpreach<IPv6>(SAFI_UNICAST)) {
	    if (_mainprocess->
		interface_address6(p->mpreach<IPv6>(SAFI_UNICAST)->
				   nexthop())) {
		XLOG_ERROR("Nexthop in update belongs to this router:\n %s",
			   cstring(*p));
		good_nexthop = false;
		return 0;
	    }
	}

	if (p->mpreach<IPv6>(SAFI_MULTICAST)) {
	    if (_mainprocess->
		interface_address6(p->mpreach<IPv6>(SAFI_MULTICAST)->
				   nexthop())) {
		XLOG_ERROR("Nexthop in update belongs to this router:\n %s",
			   cstring(*p));
		good_nexthop = false;
		return 0;
	    }
	}

	// XXX
	// In our implementation BGP is multihop by default. If we
	// ever switch to single hop (default in other
	// implementations) then we need to make sure that the
	// provided nexthop falls into a common subnet.
	
	if (ibgp() && !local_pref)
	    XLOG_WARNING("%s Update packet from ibgp with no LOCAL_PREF",
			 this->str().c_str());

	if (!ibgp() && local_pref)
	    XLOG_WARNING("%s Update packet from ebgp with LOCAL_PREF",
			 this->str().c_str());
	// A semantically incorrect NLRI generates an error message
	// for the log and is ignored, it does *not* generate a
	// notification message and drop the peering. The semantic
	// check is therefore performed in the peer handler where the
	// update packet is being turned into individual messages
	// where the offending NLRIs can be dropped.
    }
#endif

    // XXX Check withdrawn routes are correct.

    return 0;	// all correct.
}

bool
BGPPeer::established()
{
    debug_msg("%s\n", this->str().c_str());

    if (_localdata == NULL) {
	XLOG_ERROR("No _localdata");
	return false;
    }

    if (_handler == NULL) {
	// plumb peer into plumbing
	string peername = "Peer-" + peerdata()->iptuple().str();
	debug_msg("Peer is called >%s<\n", peername.c_str());
	_handler = new PeerHandler(peername, this,
				   _mainprocess->plumbing_unicast(),
				   _mainprocess->plumbing_multicast());
    } else {
	_handler->peering_came_up();
    }

//     _in_updates = 0;
//     _out_updates = 0;
//     _in_total_messages = 0;
//     _out_total_messages = 0;
    _established_transitions++;
    _mainprocess->eventloop().current_time(_established_time);
    _mainprocess->eventloop().current_time(_in_update_time);
    return true;
}

void
BGPPeer::connected(XorpFd sock)
{
    if (!_SocketClient)
	XLOG_FATAL("%s No socket structure", this->str().c_str());

    /*
    ** simultaneous open
    */
    if (_SocketClient->get_sock() == sock) {
	debug_msg("Simultaneous open\n");
	return;
    }

    /*
    ** A connection attempt comes in. We already have a valid socket
    ** so just close the connection attempt and continue. Don't bother
    ** the state machine.
    **
    ** XXX
    ** Need to check the spec to see if the incoming connection should
    ** override the existing connection. If we do make this change
    ** then it may be necessary to deal with the STATESTOPPED
    ** explictly.
    */
//     if (is_connected()) {
// 	debug_msg("Already connected dropping this connection\n");
// 	::close(sock);
// 	return;
//     }
//     event_open(sock);

    AcceptSession *connect_attempt = new AcceptSession(*this, sock);
    _accept_attempt.push_back(connect_attempt);
    connect_attempt->start();
}

void
BGPPeer::remove_accept_attempt(AcceptSession *conn)
{
    list<AcceptSession *>::iterator i;
    for (i = _accept_attempt.begin(); i != _accept_attempt.end(); i++)
	if (conn == (*i)) {
	    delete (*i);
	    _accept_attempt.erase(i);
	    return;
	}

    XLOG_UNREACHABLE();
}

SocketClient *
BGPPeer::swap_sockets(SocketClient *new_sock)
{
    debug_msg("%s Wiring up the accept socket %s\n", str().c_str(),
	      pretty_print_state(_state));
    XLOG_ASSERT(_state == STATEACTIVE ||
		_state == STATECONNECT ||
		_state == STATEOPENSENT ||
		_state == STATEOPENCONFIRM);

    SocketClient *old_sock = _SocketClient;
    _SocketClient = new_sock;
    _SocketClient->set_callback(callback(this, &BGPPeer::get_message));
    set_state(STATEACTIVE);
    event_open();

    return old_sock;
}

XorpFd
BGPPeer::get_sock()
{
    if (_SocketClient != NULL)
	return _SocketClient->get_sock();
    else {
	XorpFd invalidfd;
	return invalidfd;
    }
}

TimeVal
BGPPeer::jitter(const TimeVal& t)
{
    if (!_localdata->get_jitter())
	return t;

    // Uniformly distributed between 0.75 and 1.0
    return random_uniform(TimeVal(t.get_double() * 0.75), t);
}

void
BGPPeer::clear_all_timers()
{
    clear_connect_retry_timer();
    clear_hold_timer();
    clear_keepalive_timer();
    clear_stopped_timer();
    clear_delay_open_timer();
    clear_idle_hold_timer();
}

void
BGPPeer::start_connect_retry_timer()
{
    debug_msg("Start Connect Retry timer after %u s\n",
	      XORP_UINT_CAST(_peerdata->get_retry_duration()));

    _timer_connect_retry = _mainprocess->eventloop().
	new_oneoff_after(jitter(TimeVal(_peerdata->get_retry_duration(), 0)),
			 callback(this, &BGPPeer::event_connexp));
}

void
BGPPeer::clear_connect_retry_timer()
{
    debug_msg("Unschedule Connect Retry timer\n");

    _timer_connect_retry.unschedule();
}

void
BGPPeer::restart_connect_retry_timer()
{
    debug_msg("restart Connect Retry timer after %u s\n",
	      XORP_UINT_CAST(_peerdata->get_retry_duration()));

    clear_connect_retry_timer();
    start_connect_retry_timer();
}

/*
 * For some reason we need to restart the hold_timer, but we only do so
 * if we have negotiated the use of keepalives with the other side
 * (i.e. a non-zero value).
 */
void
BGPPeer::start_hold_timer()
{
    uint32_t duration = _peerdata->get_hold_duration();

    if (duration != 0) {
	/* Add another second to give the remote keepalive a chance */
	duration += 1;
	debug_msg("Holdtimer started %u s\n", XORP_UINT_CAST(duration));
	_timer_hold_time = _mainprocess->eventloop().
	    new_oneoff_after(TimeVal(duration, 0),
	    callback(this, &BGPPeer::event_holdexp));
    }
}

void
BGPPeer::clear_hold_timer()
{
    debug_msg("Holdtimer cleared\n");

    _timer_hold_time.unschedule();
}

void
BGPPeer::restart_hold_timer()
{
    debug_msg("Holdtimer restarted\n");
    clear_hold_timer();
    start_hold_timer();
}

void
BGPPeer::start_keepalive_timer()
{
    uint32_t duration = _peerdata->get_keepalive_duration();
    debug_msg("KeepAlive timer started with duration %u s\n",
	      XORP_UINT_CAST(duration));

    if (duration > 0) {
	TimeVal delay = jitter(TimeVal(duration, 0));
	// A keepalive must not be sent more frequently that once a second.
	delay = delay < TimeVal(1, 0) ? TimeVal(1, 0) : delay;
	_timer_keep_alive = _mainprocess->eventloop().
	    new_oneoff_after(delay, callback(this, &BGPPeer::event_keepexp));
    }
}

void
BGPPeer::clear_keepalive_timer()
{
    debug_msg("KeepAlive timer cleared\n");

    _timer_keep_alive.unschedule();
}

void
BGPPeer::start_stopped_timer()
{
    /* XXX - Only allow 10 seconds in the stopped state */
    const int delay = 10;
    debug_msg("Stopped timer started with duration %d s\n", delay);

    _timer_stopped = _mainprocess->eventloop().
	new_oneoff_after(TimeVal(delay, 0),
			 callback(this, &BGPPeer::hook_stopped));
}

void
BGPPeer::clear_stopped_timer()
{
    debug_msg("Stopped timer cleared\n");

    _timer_stopped.unschedule();
}

void 
BGPPeer::start_idle_hold_timer()
{
    if (!_damping_peer_oscillations)
	return;

    _idle_hold = _mainprocess->eventloop().
	new_oneoff_after(TimeVal(_damp_peer_oscillations.idle_holdtime(), 0),
			 callback(this, &BGPPeer::event_idle_hold_exp));
    
}

void 
BGPPeer::clear_idle_hold_timer()
{
    if (!_damping_peer_oscillations)
	return;

    _idle_hold.unschedule();
}

bool
BGPPeer::running_idle_hold_timer() const
{
    if (!_damping_peer_oscillations)
	return false;

    return _idle_hold.scheduled();
}

void 
BGPPeer::start_delay_open_timer()
{
    _idle_hold = _mainprocess->eventloop().
	new_oneoff_after(TimeVal(_peerdata->get_delay_open_time(), 0),
			 callback(this, &BGPPeer::event_delay_open_exp));
    
}

void 
BGPPeer::clear_delay_open_timer()
{
    _delay_open.unschedule();
}

bool
BGPPeer::release_resources()
{
    TIMESPENT();

    debug_msg("BGPPeer::release_resources()\n");

    if (_handler != NULL && _handler->peering_is_up())
	_handler->peering_went_down();

    TIMESPENT_CHECK();

    /*
    ** Only if we are connected call the disconnect.
    */
    if (is_connected())
	_SocketClient->disconnect();

    // clear the counters.
    _in_updates = 0;
    _out_updates = 0;
    _in_total_messages = 0;
    _out_total_messages = 0;

    _mainprocess->eventloop().current_time(_established_time);
    return true;
}

#if	0
string
BGPPeer::str() const
{
    return c_format("Peer(%d)-%s",
		    get_sock(),
		    peerdata()->iptuple().str().c_str());
}
#endif

const char *
BGPPeer::pretty_print_state(FSMState s)
{
    switch (s) {
    case STATEIDLE:
	return "IDLE(1)";
    case STATECONNECT:
	return "CONNECT(2)";
    case STATEACTIVE:
	return "ACTIVE(3)";
    case STATEOPENSENT:
	return "OPENSENT(4)";
    case STATEOPENCONFIRM:
	return "OPENCONFIRM(5)";
    case STATEESTABLISHED:
	return "ESTABLISHED(6)";
    case STATESTOPPED:
	return "STOPPED(7)";
    }
    return "ERROR";
}

void
BGPPeer::set_state(FSMState s, bool restart, bool automatic)
{
    TIMESPENT();

    //XLOG_INFO("Peer %s: Previous state: %s Current state: %s\n",
    //      peerdata()->iptuple().str().c_str(),
    //      pretty_print_state(_state),
    //      pretty_print_state(s));
    PROFILE(XLOG_TRACE(main()->profile().enabled(trace_state_change),
		       "Peer %s: Previous state: %s Current state: %s\n",
		       peerdata()->iptuple().str().c_str(),
		       pretty_print_state(_state),
		       pretty_print_state(s)));

    FSMState previous_state = _state;
    _state = s;

    if (previous_state == STATESTOPPED && _state != STATESTOPPED)
	clear_stopped_timer();

    switch (_state) {
    case STATEIDLE:
	if (previous_state != STATEIDLE) {
	    // default actions
	    clear_all_timers();
	    // Release resources - which includes a disconnect
	    release_resources();
	    if (restart) {
		if (automatic) {
		    automatic_restart();
		    start_idle_hold_timer();
		} else {
		    /* XXX
		    ** Connection has been blown away try to
		    ** reconnect.
		    */
		    event_start(); // XXX ouch, recursive call into
				   // state machine 
		}
	    }
	}
	break;
    case STATESTOPPED:
	if (previous_state != STATESTOPPED) {
	    clear_all_timers();
	    start_stopped_timer();
	}
	if (previous_state == STATEESTABLISHED) {
	    // We'll have an active peerhandler, so we need to inactivate it.
	    XLOG_ASSERT(0 != _handler);
	    _handler->stop();
	}
	break;
    case STATECONNECT:
    case STATEACTIVE:
    case STATEOPENSENT:
    case STATEOPENCONFIRM:
	break;
    case STATEESTABLISHED:
	if (STATEESTABLISHED != previous_state)
	    established();
	break;
    }

#if 0
    /*
    ** If there is a BGP MIB target running send it traps when a state
    ** transition takes place.
    */
    BGPMain *m  = _mainprocess;
    if (m->do_snmp_trap()) {
	 XrlBgpMibTrapsV0p1Client snmp(m->get_router());
	 string last_error = NotificationPacket::pretty_print_error_code(
						       _last_error[0],
						       _last_error[1]);
	 if (STATEESTABLISHED == _state && STATEESTABLISHED != previous_state){
	    snmp.send_send_bgp_established_trap(m->bgp_mib_name().c_str(),
						last_error,_state,
						callback(trap_callback,
							 "established"));
	} else if (_state < previous_state) {
	    snmp.send_send_bgp_backward_transition_trap(
						  m->bgp_mib_name().c_str(), 
						   last_error, _state,
						   callback(trap_callback,
							    "backward"));
	}
    }
#endif
}

#if 0
inline
void
trap_callback(const XrlError& error, const char *comment)
{
    debug_msg("trap_callback %s %s\n", comment, error.str().c_str());
    if (XrlError::OKAY() != error) {
	XLOG_WARNING("trap_callback: %s %s", comment, error.str().c_str());
    }
}
#endif

PeerOutputState
BGPPeer::send_update_message(const UpdatePacket& p)
{
    PeerOutputState queue_state;
    debug_msg("send_update_message called\n");
    assert(STATEESTABLISHED == _state);
    queue_state = send_message(p);
    debug_msg("send_update_message: queue is state %d\n", queue_state);
    return queue_state;
}

bool
BGPPeer::send_netreachability(const BGPUpdateAttrib &n)
{
    debug_msg("send_netreachability called\n");
    UpdatePacket bup;
    bup.add_nlri(n);
    return send_message(bup);
}

uint32_t
BGPPeer::get_established_time() const
{
    TimeVal now;
    _mainprocess->eventloop().current_time(now);
    return now.sec() - _established_time.sec();
}

void 
BGPPeer::get_msg_stats(uint32_t& in_updates, 
		       uint32_t& out_updates, 
		       uint32_t& in_msgs, 
		       uint32_t& out_msgs, 
		       uint16_t& last_error, 
		       uint32_t& in_update_elapsed) const 
{
    in_updates = _in_updates;
    out_updates = _out_updates;
    in_msgs = _in_total_messages;
    out_msgs = _out_total_messages;
    memcpy(&last_error, _last_error, 2);
    TimeVal now;
    _mainprocess->eventloop().current_time(now);
    in_update_elapsed = now.sec() - _in_update_time.sec();
}

bool 
BGPPeer::remote_ip_ge_than(const BGPPeer& peer)
{
    IPvX this_remote_ip(peerdata()->iptuple().get_peer_addr().c_str());
    IPvX other_remote_ip(peer.peerdata()->iptuple().get_peer_addr().c_str());

    return (this_remote_ip >= other_remote_ip);     
}

void
BGPPeer::automatic_restart()
{
    if (!_damping_peer_oscillations)
	return;

    _damp_peer_oscillations.restart();
}

// Second FSM.

AcceptSession::AcceptSession(BGPPeer& peer, XorpFd sock)
    : _peer(peer), _sock(sock), _accept_messages(true)
{

    const BGPPeerData *pd = peer.peerdata();

    bool md5sig = !pd->get_md5_password().empty();

    _socket_client = new SocketClient(pd->iptuple(),
				     peer.main()->eventloop(), md5sig);


    _socket_client->set_callback(callback(this,
					  &AcceptSession::get_message_accept));
}

AcceptSession::~AcceptSession()
{
    debug_msg("Socket %p\n", _socket_client);

    XLOG_ASSERT(BAD_XORPFD == _sock);
    XLOG_ASSERT(!is_connected());
    XLOG_ASSERT(!_open_wait.scheduled());
    delete _socket_client;
    _socket_client = 0;
}

void
AcceptSession::start()
{
    debug_msg("%s %s\n", str().c_str(),  _peer.pretty_print_state(state()));

    uint32_t hold_duration;

    // Note this is the state of the main FSM.
    switch(state()) {
    case STATEIDLE:
	// Drop this connection, we are in idle.
	debug_msg("rejected\n");
	XLOG_INFO("%s rejecting connection: current state %s %s",
		  str().c_str(),
		  _peer.pretty_print_state(state()),
		  running_idle_hold_timer() ? "holdtimer running" : "");
	comm_sock_close(_sock);
	_sock = BAD_XORPFD;
	remove();
	break;
    case STATEOPENSENT:
	// Note we are not going to send anything.
	// Wait for an open message from the peer so that the ID's can
	// be compared to determine which connection to accept.
	// Start a timer in case the open never arrives.
	hold_duration = _peer.peerdata()->get_hold_duration();
	if (0 == hold_duration) {
	    hold_duration = 4 * 60;
	    XLOG_WARNING("Connection collision hold duration is 0 "
			 "setting to %d seconds", hold_duration);
	}
	_open_wait = main()->
	    eventloop().
	    new_oneoff_after(TimeVal(hold_duration, 0),
			     callback(this,
				      &AcceptSession::no_open_received));
	_socket_client->connected(_sock);
	_sock = BAD_XORPFD;
	break;
    case STATEOPENCONFIRM:
	// Check the IDs to see what should be done.
	collision();
	break;
    case STATEESTABLISHED:
	// Send a cease and shutdown this attempt.
	cease();
	break;
    case STATECONNECT:
    case STATEACTIVE:
    case STATESTOPPED:
	// Accept this connection attempt.
	_socket_client->set_callback(callback(_peer, &BGPPeer::get_message));
	_peer.event_open(_sock);
	_sock = BAD_XORPFD;
	remove();
	break;
    }
}

void
AcceptSession::no_open_received()
{
    debug_msg("\n");
    cease();
}

void
AcceptSession::remove()
{
    _peer.remove_accept_attempt(this);
}

void
AcceptSession::send_notification_accept(const NotificationPacket& np)
{
    // Don't process any more incoming messages
    ignore_message();

    if (BAD_XORPFD != _sock) {
	_socket_client->connected(_sock);
	_sock = BAD_XORPFD;
    }
    _socket_client->stop_reader();

    // This buffer is dynamically allocated and needs to be freed.
    size_t ccnt = BGPPacket::MAXPACKETSIZE;
    uint8_t *buf = new uint8_t[BGPPacket::MAXPACKETSIZE];

    XLOG_ASSERT(np.encode(buf, ccnt, _peer.peerdata()));
    debug_msg("Buffer for sent packet is %p\n", buf);

    XLOG_INFO("Sending: %s", cstring(np));

    PROFILE(XLOG_TRACE(main()->profile().enabled(trace_message_out),
		       "Peer %s: Send: %s",
		       peerdata()->iptuple().str().c_str(),
		       cstring(np)));
    
    // Free the buffer in the completion routine.
    bool ret =_socket_client->send_message(buf, ccnt,
	       callback(this, &AcceptSession::send_notification_cb));

    if (!ret) {
	delete[] buf;
	remove();
	return;
    }
}

void
AcceptSession::send_notification_cb(SocketClient::Event ev, const uint8_t* buf)
{
    switch (ev) {
    case SocketClient::DATA:
	debug_msg("Notification sent\n");
	delete[] buf;
	break;
    case SocketClient::FLUSHING:
	delete[] buf;
	break;
    case SocketClient::ERROR:
	debug_msg("Notification not sent\n");
	/* Don't free the message here we'll get it in the flush */
	break;
    }

    _socket_client->disconnect();
    
    remove();
}

void
AcceptSession::cease()
{
    NotificationPacket np(CEASE);
    send_notification_accept(np);
}

void
AcceptSession::collision()
{
    IPv4 id = _peer.id();
    IPv4 peerid = _peer.peerdata()->id();

    debug_msg("%s router id %s peerid %s\n", str().c_str(), cstring(id),
	      cstring(peerid));

    // This is a comparison of the IDs in the main state machine not
    // this one. The BGP that has the largest ID gets to make the
    // connection.

    // Dump the other connection and take this one.
    if (peerid > id) {
	debug_msg("%s Accepting connect attempt\n", str().c_str());
	swap_sockets();
    } else {
	debug_msg("%s Dropping connect attempt\n", str().c_str());
    }

    cease();
}

void
AcceptSession::event_openmess_accept(const OpenPacket& p)
{
    debug_msg("%s %s\n", str().c_str(), cstring(p));

    // While waiting for the open message the state of the main FSM
    // may have changed.

    bool compare = false;

    switch(state()) {
    case STATEIDLE:
	// The peer was taken to idle while we waited for the open message.
	debug_msg("rejected\n");
	XLOG_INFO("%s rejecting connection: current state %s",
		  str().c_str(),
		  _peer.pretty_print_state(state()));
	_socket_client->disconnect();
	remove();
	break;
    case STATEACTIVE:
	// The main FSM is now waiting for a connection so provide
	// this one, it doesn't have a connection so there is no need
	// to send a cease().
	swap_sockets(p);
	remove();
	break;
    case STATECONNECT:
    case STATEOPENSENT:
	// We have an open packet on this session that should sort
	// things out.
	compare = true;
	break;
    case STATEOPENCONFIRM:
	// If the IDs used in both sessions are the same we don't need
	// to check the other open packet.
	compare = true;
	break;
    case STATEESTABLISHED:
	// Send a cease and shutdown this attempt.
	cease();
	break;
    case STATESTOPPED:
	// The socket in the main FSM is now shut. So swap in this socket.
	swap_sockets(p);
	XLOG_ASSERT(BAD_XORPFD == _socket_client->get_sock());
	remove();// remove() not cease()
	break;
    }

    if (!compare)
	return;

    IPv4 id = _peer.id();
    IPv4 peerid = p.id();
    if (peerid > id) {
	debug_msg("%s Accepting connect attempt\n", str().c_str());
	swap_sockets(p);
    } else {
	debug_msg("%s Dropping connect attempt\n", str().c_str());
    }

    XLOG_ASSERT(BAD_XORPFD == _sock);

    cease();
}

void
AcceptSession::swap_sockets()
{
    debug_msg("%s\n", str().c_str());

    if (BAD_XORPFD != _sock) {
	_socket_client->connected(_sock);
	_sock = BAD_XORPFD;
    }
    _socket_client = _peer.swap_sockets(_socket_client);
    _socket_client->
	set_callback(callback(this, &AcceptSession::get_message_accept));
}

void
AcceptSession::swap_sockets(const OpenPacket& p)
{
    swap_sockets();

    // Either call open message in the main FSM or go through
    // get_message. Calling get_message ensures all the counters
    // and profiling information is correct.
    // If profiling is enabled the open message will be seen twice.
#ifdef	NO_STATS
    _peer.event_openmess(p);
#else
    // This buffer is dynamically allocated and needs to be freed.
    size_t ccnt = BGPPacket::MAXPACKETSIZE;
    uint8_t *buf = new uint8_t[BGPPacket::MAXPACKETSIZE];
    XLOG_ASSERT(p.encode(buf, ccnt, NULL));
    _peer.get_message(BGPPacket::GOOD_MESSAGE, buf, ccnt, 0);
    delete[] buf;
#endif
}

void
AcceptSession::notify_peer_of_error_accept(const int error,
					   const int subcode,
					   const uint8_t *data,
					   const size_t len)
{
    if (!NotificationPacket::validate_error_code(error, subcode)) {
	XLOG_WARNING("%s Attempt to send invalid error code %d subcode %d",
		     this->str().c_str(),
		     error, subcode);
    }

    if (is_connected()) {
	NotificationPacket np(error, subcode, data, len);
	send_notification_accept(np);
	return;
    }
}

void
AcceptSession::event_tranfatal_accept()
{
}

void
AcceptSession::event_closed_accept()
{
    // The socket is closed so out of here.
    _socket_client->disconnect();
    remove();
}

void
AcceptSession::event_keepmess_accept()
{
    // A keepalive is unexpected send a cease and out of here.
    cease();
}

void
AcceptSession::event_recvupdate_accept(const UpdatePacket& /*p*/)
{
    // An update is unexpected send a cease and out of here.
    cease();
}

void
AcceptSession::event_recvnotify_accept(const NotificationPacket& /*p*/)
{
    // Can't send a notify in response to a notify just shut the
    // connection and out of here.
    _socket_client->disconnect();
    remove();
}

bool
AcceptSession::get_message_accept(BGPPacket::Status status,
				  const uint8_t *buf,
				  size_t length,
				  SocketClient *socket_client)
{
    XLOG_ASSERT(socket_client == _socket_client);

    // An open is expected but any packet will break us out of this state.
    _open_wait.clear();

    if (!accept_message()) {
	debug_msg("Disregarding messages %#x %u", status,
		  XORP_UINT_CAST(length));
	return true;
    }

    TIMESPENT();

    switch (status) {
    case BGPPacket::GOOD_MESSAGE:
	break;

    case BGPPacket::ILLEGAL_MESSAGE_LENGTH:
	notify_peer_of_error_accept(MSGHEADERERR, BADMESSLEN,
				    buf + BGPPacket::MARKER_SIZE, 2);
// 	event_tranfatal_accept();
	TIMESPENT_CHECK();
	debug_msg("Returning false\n");
	return false;

    case BGPPacket::CONNECTION_CLOSED:
  	event_closed_accept();
	TIMESPENT_CHECK();
	debug_msg("Returning false\n");
	return false;
    }

    /*
    ** We should only have a good packet at this point.
    ** The buffer pointer must be non 0.
    */
    XLOG_ASSERT(0 != buf);

    const uint8_t* marker = buf + BGPPacket::MARKER_OFFSET;
    uint8_t type = extract_8(buf + BGPPacket::TYPE_OFFSET);
    try {
	/*
	** Check the Marker, total waste of time as it never contains
	** anything of interest.
	*/
	if (0 != memcmp(const_cast<uint8_t *>(&BGPPacket::Marker[0]),
			marker, BGPPacket::MARKER_SIZE)) {
	    xorp_throw(CorruptMessage,"Bad Marker", MSGHEADERERR, CONNNOTSYNC);
	}
	
	switch (type) {
	case MESSAGETYPEOPEN: {
	    debug_msg("OPEN Packet RECEIVED\n");
	    OpenPacket pac(buf, length);
 	    PROFILE(XLOG_TRACE(main()->profile().enabled(trace_message_in),
			       "Peer %s: Receive: %s",
			       peerdata()->iptuple().str().c_str(),
			       cstring(pac)));

	    // All decode errors should throw a CorruptMessage.
	    debug_msg("%s", pac.str().c_str());
	    // want unified decode call. now need to get peerdata out.
// 	    _peerdata->dump_peer_data();
	    event_openmess_accept(pac);
	    TIMESPENT_CHECK();
	    break;
	}
	case MESSAGETYPEKEEPALIVE: {
	    debug_msg("KEEPALIVE Packet RECEIVED %u\n",
		      XORP_UINT_CAST(length));
	    // Check that the length is correct or throw an exception
	    KeepAlivePacket pac(buf, length);

 	    PROFILE(XLOG_TRACE(main()->profile().enabled(trace_message_in),
			       "Peer %s: Receive: %s",
			       peerdata()->iptuple().str().c_str(),
			       cstring(pac)));

	    // debug_msg(pac.str().c_str());
	    event_keepmess_accept();
	    TIMESPENT_CHECK();
	    break;
	}
	case MESSAGETYPEUPDATE: {
	    debug_msg("UPDATE Packet RECEIVED\n");
// 	    _in_updates++;
// 	    main()->eventloop().current_time(_in_update_time);
	    UpdatePacket pac(buf, length, _peer.peerdata(), 
			     _peer.main(), /*do checks*/true);

 	    PROFILE(XLOG_TRACE(main()->profile().enabled(trace_message_in),
			       "Peer %s: Receive: %s",
			       peerdata()->iptuple().str().c_str(),
			       cstring(pac)));

	    // All decode errors should throw a CorruptMessage.
	    debug_msg("%s", pac.str().c_str());
	    event_recvupdate_accept(pac);
	    TIMESPENT_CHECK();
	    if (TIMESPENT_OVERLIMIT()) {
		XLOG_WARNING("Processing packet took longer than %u second %s",
			     XORP_UINT_CAST(TIMESPENT_LIMIT),
			     pac.str().c_str());
	    }
	    break;
	}
	case MESSAGETYPENOTIFICATION: {
	    debug_msg("NOTIFICATION Packet RECEIVED\n");
	    NotificationPacket pac(buf, length);
	    
	    PROFILE(XLOG_TRACE(main()->profile().enabled(trace_message_in),
			       "Peer %s: Receive: %s",
			       peerdata()->iptuple().str().c_str(),
			       cstring(pac)));

	    // All decode errors should throw a CorruptMessage.
	    debug_msg("%s", pac.str().c_str());
	    event_recvnotify_accept(pac);
	    TIMESPENT_CHECK();
	    break;
	}
	default:
	    /*
	    ** Send a notification to the peer. This is a bad message type.
	    */
	    XLOG_ERROR("%s Unknown packet type %d",
		       this->str().c_str(), type);
	    notify_peer_of_error_accept(MSGHEADERERR, BADMESSTYPE,
					buf + BGPPacket::TYPE_OFFSET, 1);
// 	    event_tranfatal_accept();
	    TIMESPENT_CHECK();
	    debug_msg("Returning false\n");
	    return false;
	}
    } catch(CorruptMessage& c) {
	/*
	** This peer has sent us a bad message. Send a notification
	** and drop the the peering.
	*/
	XLOG_WARNING("%s %s %s", this->str().c_str(), c.where().c_str(),
		     c.why().c_str());
	notify_peer_of_error_accept(c.error(), c.subcode(), c.data(), c.len());
// 	event_tranfatal_accept();
	TIMESPENT_CHECK();
	debug_msg("Returning false\n");
	return false;
    } catch (UnusableMessage& um) {
	// the packet wasn't usable for some reason, but also
	// wasn't so corrupt we need to send a notification -
	// this is a "silent" error.
	XLOG_WARNING("%s %s %s", this->str().c_str(), um.where().c_str(),
		     um.why().c_str());
    }

    TIMESPENT_CHECK();

    /*
    ** If we are still connected and supposed to be reading.
    */
    if (!socket_client->is_connected() || !socket_client->still_reading()) {
	TIMESPENT_CHECK();
	debug_msg("Returning false %s %s socket %p\n",
		  is_connected() ? "connected" : "not connected",
		  still_reading() ? "reading" : "not reading",
		  _socket_client);
	return false;
    }

    return true;
}

DampPeerOscillations::DampPeerOscillations(EventLoop& eventloop,
					   uint32_t restart_threshold,
					   uint32_t time_period,
					   uint32_t idle_holdtime) 
    : _eventloop(eventloop),
      _restart_threshold(restart_threshold),
      _time_period(time_period),
      _idle_holdtime(idle_holdtime),
      _restart_counter(0)
{
}

void
DampPeerOscillations::restart()
{
    if (0 == _restart_counter++) {
	_zero_restart = _eventloop.
	    new_oneoff_after(TimeVal(_time_period, 0),
			     callback(this,
				      &DampPeerOscillations::
				      zero_restart_count));
	
    }
}

void
DampPeerOscillations::reset()
{
    _zero_restart.unschedule();
    zero_restart_count();
}

void
DampPeerOscillations::zero_restart_count()
{
    _restart_counter = 0;
}

uint32_t
DampPeerOscillations::idle_holdtime() const
{
    return _restart_counter < _restart_threshold ? 0 : _idle_holdtime;
}
