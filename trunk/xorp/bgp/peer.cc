// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2005 International Computer Science Institute
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

#ident "$XORP: xorp/bgp/peer.cc,v 1.92 2005/04/10 06:23:15 atanu Exp $"

// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME
// #define SAVE_PACKETS
#define CHECK_TIME

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "bgp_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/timer.hh"
#include "xrl/interfaces/bgp_mib_traps_xif.hh"
#include "libxorp/timespent.hh"

#include "peer.hh"
#include "bgp.hh"
#include "profile_vars.hh"

#define DEBUG_BGPPeer

inline void trap_callback(const XrlError& error, const char *comment);

BGPPeer::BGPPeer(LocalData *ld, BGPPeerData *pd, SocketClient *sock,
		 BGPMain *m)
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

    _in_updates = 0;
    _out_updates = 0;
    _in_total_messages = 0;
    _out_total_messages = 0;
    _last_error[0] = 0;
    _last_error[1] = 0;
    _established_transitions = 0;
    _mainprocess->eventloop().current_time(_established_time);
    _mainprocess->eventloop().current_time(_in_update_time);

    _current_state = _next_state = _activated = false;
}

BGPPeer::~BGPPeer()
{
    delete _SocketClient;
    delete _peerdata;
}

/*
 * This call is dispatched by the reader after making sure that
 * we have a packet with at least a fixed_header and whose
 * length matches the one in the fixed_header.
 * Our job now is to decode the message and dispatch it to the
 * state machine.
 */
bool
BGPPeer::get_message(BGPPacket::Status status, const uint8_t *buf,
		     size_t length)
{
    if (main()->profile().enabled(profile_message_in))
	main()->profile().log(profile_message_in,
			      c_format("message on %s len %u",
				       str().c_str(),
				       XORP_UINT_CAST(length)));
	
    TIMESPENT();

    switch (status) {
    case BGPPacket::GOOD_MESSAGE:
	break;

    case BGPPacket::ILLEGAL_MESSAGE_LENGTH:
	notify_peer_of_error(MSGHEADERERR, BADMESSLEN);
	event_tranfatal();
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

    string fname = "/tmp/bgpin.mrtd";

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

    timeval now;
    gettimeofday(&now, 0);
    TimeVal tv(now);
    
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

    const fixed_header *header =
	reinterpret_cast<const struct fixed_header *>(buf);

    /* XXX
    ** Put the marker authentication code here.
    */
    try {
	switch (header->type) {
	case MESSAGETYPEOPEN: {
	    debug_msg("OPEN Packet RECEIVED\n");
	    OpenPacket pac(buf, length);
	    XLOG_TRACE(main()->profile().enabled(trace_message_in),
		       cstring(pac));
	    // All decode errors should throw a CorruptMessage.
	    debug_msg(pac.str().c_str());
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
	    XLOG_TRACE(main()->profile().enabled(trace_message_in),
		       cstring(pac));
	    // debug_msg(pac.str().c_str());
	    event_keepmess();
	    TIMESPENT_CHECK();
	    break;
	}
	case MESSAGETYPEUPDATE: {
	    debug_msg("UPDATE Packet RECEIVED\n");
	    _in_updates++;
	    _mainprocess->eventloop().current_time(_in_update_time);
	    UpdatePacket pac(buf, length);
	    XLOG_TRACE(main()->profile().enabled(trace_message_in),
		       cstring(pac));
	    // All decode errors should throw a CorruptMessage.
	    debug_msg(pac.str().c_str());
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
	    try {
		NotificationPacket pac(buf, length);
		XLOG_TRACE(main()->profile().enabled(trace_message_in),
			   cstring(pac));
		// All decode errors should throw an InvalidPacket
		debug_msg(pac.str().c_str());
		event_recvnotify(pac);
	    } catch (InvalidPacket& err) {
		XLOG_WARNING("%s Received Invalid Notification Packet",
			     this->str().c_str());
		// We received a bad notification packet.  We don't
		// want to send a notification in response to a
		// notification, so we need to treat this as if it were
		// a good notification and tear the connection down.
		// Pretend we received a CEASE, which is really the
		// catchall error code.
		// XXX we should rather use a 'bad notification' error code?
		NotificationPacket pac(CEASE);
		event_recvnotify(pac);
	    }
	    TIMESPENT_CHECK();
	    break;
	}
	default:
	    /*
	    ** Send a notification to the peer. This is a bad message type.
	    */
	    XLOG_ERROR("%s Unknown packet type %d",
		       this->str().c_str(), header->type);
	    notify_peer_of_error(MSGHEADERERR, BADMESSTYPE);
	    event_tranfatal();
	    TIMESPENT_CHECK();
	    return false;
	}
    } catch(CorruptMessage c) {
	/*
	** This peer has sent us a bad message. Send a notification
	** and drop the the peering.
	*/
	XLOG_WARNING("%s %s", this->str().c_str(),c.why().c_str());
	notify_peer_of_error(c.error(), c.subcode(), c.data(), c.len());
	event_tranfatal();
	TIMESPENT_CHECK();
	return false;
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
    debug_msg(p.str().c_str());

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

    const uint8_t *buf;
    size_t ccnt;

    /*
    ** This buffer is dynamically allocated and should be freed.
    */
    buf = (const uint8_t *)p.encode(ccnt);
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
	/*drop through to next case*/
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
BGPPeer::send_notification(const NotificationPacket& p, bool restart)
{
    debug_msg(p.str().c_str());

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

    size_t ccnt;
    /*
     * This buffer is dynamically allocated and should be freed.
     */
    const uint8_t *buf = (const uint8_t *)p.encode(ccnt);
    debug_msg("Buffer for sent packet is %p\n", buf);

    /*
    ** This write is async. So we can't free the data now,
    ** we will deal with it in the complete routine.  */
    bool ret =_SocketClient->send_message(buf, ccnt,
	       callback(this, &BGPPeer::send_notification_complete, restart));

    if (!ret) {
	delete[] buf;
	return;
    }

}

void
BGPPeer::send_notification_complete(SocketClient::Event ev,
				    const uint8_t* buf, bool restart)
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
	set_state(STATEIDLE, restart);
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
    set_state(STATEIDLE, true);
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

    switch(_state) {

    case STATESTOPPED:
	flush_transmit_queue();		// ensure callback can't happen
	set_state(STATEIDLE);	// go through STATEIDLE to clear resources
	// fallthrough now to process the start event
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
BGPPeer::event_stop(bool restart)	// EVENTBGPSTOP
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
	set_state(STATEIDLE, restart);
	break;

    case STATEOPENSENT:
    case STATEOPENCONFIRM:
    case STATEESTABLISHED: {
	// Send Notification Message with error code of CEASE.
	NotificationPacket np(CEASE);
	send_notification(np, restart);
	set_state(STATESTOPPED, restart);
	break;
    }
    case STATESTOPPED:
	// a second stop will cause us to give up on sending the CEASE
	flush_transmit_queue();		// ensure callback can't happen
	set_state(STATEIDLE, restart);
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
	OpenPacket open_packet(_localdata->as(),
			       _localdata->id(),
			       _peerdata->get_configured_hold_time());

	ParameterList::const_iterator
	    pi = _peerdata->parameter_sent_list().begin();
	while(pi != _peerdata->parameter_sent_list().end()) {
	    if ((*pi)->send())
		open_packet.add_parameter(*pi);
	    pi++;
	}

	send_message(open_packet);

	clear_connect_retry_timer();
	if (_state == STATEACTIVE) {
	    // Start Holdtimer - four minutes recommended in spec.
	    _peerdata->set_hold_duration(4 * 60 * 1000);
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
	_SocketClient->connect_break();
	clear_connect_retry_timer();
	/*FALLTHROUGH*/

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
	set_state(STATEIDLE, true);
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
	set_state(STATEIDLE, true);	// retry later...
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
	connect_to_peer(callback(this, &BGPPeer:: connect_to_peer_complete));
	break;

    case STATEACTIVE:
	restart_connect_retry_timer();
	set_state(STATECONNECT);
	connect_to_peer(callback(this, &BGPPeer:: connect_to_peer_complete));
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
	set_state(STATESTOPPED, true);
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

/*
 * EVENTRECOPENMESS event.
 * We receive an open packet.
 */
void
BGPPeer::event_openmess(const OpenPacket& p)		// EVENTRECOPENMESS
{
    TIMESPENT();

    switch(_state) {
    case STATEIDLE:
    case STATECONNECT:
    case STATEACTIVE:
	XLOG_FATAL("%s FSM received EVENTRECOPENMESS in state %s",
		   this->str().c_str(),
		   pretty_print_state(_state));
	break;

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

	    // if AS number is the same as the local AS number set
	    // connection as internal otherwise set as external
	    if ( _localdata->as() == _peerdata->as() )
		_peerdata->set_internal_peer(true);
	    else
		_peerdata->set_internal_peer(false);

	    // Save the parameters from the open packet.
	    _peerdata->save_parameters(p.parameter_list());

	    // Compare 
	    _peerdata->open_negotiation();

	    set_state(STATEOPENCONFIRM);
	} catch(CorruptMessage& mess) {
	    NotificationPacket np(mess.error(), mess.subcode());
	    send_notification(np);
	    set_state(STATESTOPPED, true);
	}
	break;

    case STATEOPENCONFIRM:
    case STATEESTABLISHED: {
	// Send Notification - FSM error
	XLOG_WARNING("%s FSM received EVENTRECOPENMESS in state %s",
		     this->str().c_str(),
		     pretty_print_state(_state));
	NotificationPacket np(FSMERROR);
	send_notification(np);
	set_state(STATESTOPPED, true);
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
	set_state(STATESTOPPED, true);
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
BGPPeer::event_recvupdate(const UpdatePacket& p) // EVENTRECUPDATEMESS
{ 
    TIMESPENT();

    switch(_state) {
    case STATEIDLE:
    case STATECONNECT:
    case STATEACTIVE:
	XLOG_FATAL("%s FSM received EVENTRECUPDATEMESS in state %s",
		   this->str().c_str(),
		   pretty_print_state(_state));
	break;

    case STATEOPENSENT:
    case STATEOPENCONFIRM: {
	// Send Notification Message with error code of FSM error.
	XLOG_WARNING("%s FSM received EVENTRECUPDATEMESS in state %s",
		     this->str().c_str(),
		     pretty_print_state(_state));
	NotificationPacket np(FSMERROR);
	send_notification(np);
	set_state(STATESTOPPED, true);
	break;
    }
    case STATEESTABLISHED: {
	NotificationPacket *notification = check_update_packet(&p);
	if ( notification != 0 ) {	// bad message
	    // Send Notification Message
	    send_notification(*notification);
	    delete notification;
	    set_state(STATESTOPPED, true);
	    break;
	}
	// ok the packet is correct.
	restart_hold_timer();
	// process the packet...
	debug_msg("Process the packet!!!\n");
	XLOG_ASSERT(_handler);
	// XXX - Temporary hack until we get programmable filters.
	const IPv4 next_hop = peerdata()->get_next_hop_rewrite();
	if (!next_hop.is_zero()) {
	    PathAttributeList<IPv4>& l = const_cast<
		PathAttributeList<IPv4>&>(p.pa_list());
	    PathAttributeList<IPv4>::iterator i;
	    for (i = l.begin(); i != l.end(); i++) {
		if (NEXT_HOP == (*i)->type()) {
 		    delete (*i);
		    (*i) = new NextHopAttribute<IPv4>(next_hop);
		    break;
		}
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

    XLOG_WARNING("%s in state %s received %s",
		 this->str().c_str(),
		 pretty_print_state(_state),
		 p.str().c_str());

    _last_error[0] = p.error_code();
    _last_error[1] = p.error_subcode();

    switch(_state) {
    case STATEIDLE:
    case STATECONNECT:
    case STATEACTIVE:
	XLOG_FATAL("%s FSM received EVENTRECNOTMESS in state %s",
		   this->str().c_str(),
		   pretty_print_state(_state));
	break;

    case STATEOPENSENT:
    case STATEOPENCONFIRM:
    case STATEESTABLISHED:
	set_state(STATEIDLE, true);
	break;

    case STATESTOPPED:
	break;
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
	set_state(STATESTOPPED, true);
	return;
    }
}

/*
 * Handle incoming connection events.
 *
 * The argment is the incoming socket descriptor, which
 * must be used or closed by this method.
 */
void
BGPPeer::event_open(const int sock)
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
		     
	if (-1 == ::close(sock)) {
	    XLOG_WARNING("%s Close of incoming connection failed: %s",
			 this->str().c_str(),
			 strerror(errno));
	}
    }
}

void
BGPPeer::check_open_packet(const OpenPacket *p) throw(CorruptMessage)
{
    if (p->Version() != BGPVERSION) {
	uint8_t data[2];
	*reinterpret_cast<uint16_t *>(&data[0]) = htons(BGPVERSION);
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

    uint32_t hold_ms = hold_secs * 1000;
    _peerdata->set_hold_duration(hold_ms);
    _peerdata->set_keepalive_duration(hold_ms / 3);

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

NotificationPacket *
BGPPeer::check_update_packet(const UpdatePacket *p)
{
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

    // if the path attributes list is empty then no network
    // layer reachability information should be present.

    if ( p->pa_list().empty() ) {
	if ( p->nlri_list().empty() == false )	{
	    debug_msg("Empty path attribute list and "
		      "non-empty NLRI list\n");
	    return new NotificationPacket(UPDATEMSGERR, MALATTRLIST);
	}
    } else {
	// if we have path attributes, check that they are valid.
	bool local_pref = false;

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
	    default:
		if ((*i)->well_known()) {
		    // unrecognized well_known attribute.
		    return new NotificationPacket(UPDATEMSGERR, UNRECOGWATTR,
					(*i)->data(), (*i)->wire_size());
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
	    }
	}

	// XXX need also to check that the nexthop address is not
	// one of our addresses, and with single hop EBGP that the
	// nexthop address shares a subnet with us or is the
	// address of our peer
	// if so, we'd discard the route, but not send a notification

#if	0
	if (ibgp() && !local_pref)
	    XLOG_WARNING("%s Update packet from ibgp with no LOCAL_PREF",
			 this->str().c_str());

	if (!ibgp() && local_pref)
	    XLOG_WARNING("%s Update packet from ebgp with LOCAL_PREF",
			 this->str().c_str());
#endif
	// XXX Check Network layer reachability fields

    }

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

    _in_updates = 0;
    _out_updates = 0;
    _in_total_messages = 0;
    _out_total_messages = 0;
    _established_transitions++;
    _mainprocess->eventloop().current_time(_established_time);
    _mainprocess->eventloop().current_time(_in_update_time);
    return true;
}

void
BGPPeer::connected(int sock)
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
    event_open(sock);
}

int
BGPPeer::get_sock()
{
    if (_SocketClient != NULL)
	return _SocketClient->get_sock();
    else
	return -1;
}

void
BGPPeer::clear_all_timers()
{
    clear_connect_retry_timer();
    clear_hold_timer();
    clear_keepalive_timer();
    clear_stopped_timer();
}

void
BGPPeer::start_connect_retry_timer()
{
    debug_msg("Start Connect Retry timer after %u ms\n",
	      XORP_UINT_CAST(_peerdata->get_retry_duration()));

    _timer_connect_retry = _mainprocess->eventloop().
	new_oneoff_after_ms(_peerdata->get_retry_duration(),
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
    debug_msg("restart Connect Retry timer after %u ms\n",
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
	/* Add another half a second to give the remote keepalive a chance */
	duration += 500;
	debug_msg("Holdtimer started %u\n", XORP_UINT_CAST(duration));
	_timer_hold_time = _mainprocess->eventloop().
	    new_oneoff_after_ms(duration,
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
    debug_msg("KeepAlive timer started with duration %u\n",
	      XORP_UINT_CAST(duration));

    if (duration > 0)
	_timer_keep_alive = _mainprocess->eventloop().
	    new_oneoff_after_ms(duration,
		callback(this, &BGPPeer::event_keepexp));
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
    const int delay = 10 * 1000;
    debug_msg("Stopped timer started with duration %d ms\n", delay);

    _timer_stopped = _mainprocess->eventloop().
	new_oneoff_after_ms(delay, callback(this, &BGPPeer::hook_stopped));
}

void
BGPPeer::clear_stopped_timer()
{
    debug_msg("Stopped timer cleared\n");

    _timer_stopped.unschedule();
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
BGPPeer::set_state(FSMState s, bool restart)
{
    TIMESPENT();

    debug_msg("Peer %s: Previous state: %s Current state: %s\n",
	    peerdata()->iptuple().str().c_str(),
	    pretty_print_state(_state),
	    pretty_print_state(s));
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
		// re-start timer = 60;
		// or exp growth see section 8

		/* XXX
		** Connection has been blown away try to
		** reconnect.
		*/
		event_start(); // XXX ouch, recursive call into state machine
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
}

inline
void
trap_callback(const XrlError& error, const char *comment)
{
    debug_msg("trap_callback %s %s\n", comment, error.str().c_str());
    if (XrlError::OKAY() != error) {
	XLOG_WARNING("trap_callback: %s %s", comment, error.str().c_str());
    }
}

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
    string this_remote_ip = peerdata()->iptuple().get_peer_addr();
    string other_remote_ip = peer.peerdata()->iptuple().get_peer_addr();
    return (this_remote_ip >= other_remote_ip);     
}
