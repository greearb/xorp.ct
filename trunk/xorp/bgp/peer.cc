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

#ident "$XORP: xorp/bgp/peer.cc,v 1.15 2003/01/26 17:03:18 rizzo Exp $"

// #define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "config.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "peer.hh"
#include "main.hh"

#define DEBUG_BGPPeer

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

    _in_updates = 0;
    _out_updates = 0;
    _in_total_messages = 0;
    _out_total_messages = 0;
    _last_error[0] = 0;
    _last_error[1] = 0;
    _established_transitions = 0;
    gettimeofday(&_established_time, NULL);
    gettimeofday(&_in_update_time, NULL);
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
    switch (status) {
    case BGPPacket::GOOD_MESSAGE:
	break;

    case BGPPacket::ILLEGAL_MESSAGE_LENGTH:
	notify_peer_of_error(MSGHEADERERR, BADMESSLEN);
	event_tranfatal();
	return false;

    case BGPPacket::CONNECTION_CLOSED:
 	// _SocketClient->disconnect();
 	event_closed();
	// XLOG_ASSERT(!is_connected());
	return false;
    }

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
	    // All decode errors should throw a CorruptMessage.
	    debug_msg(pac.str().c_str());
	    // want unified decode call. now need to get peerdata out.
	    _peerdata->dump_peer_data();
	    event_openmess(&pac);
	    break;
	}
	case MESSAGETYPEKEEPALIVE: {
	    debug_msg("KEEPALIVE Packet RECEIVED %u\n", (uint32_t)length);
	    // Check that the length is correct or throw an exception
	    KeepAlivePacket pac(buf, length);
	    // debug_msg(pac.str().c_str());
	    event_keepmess();
	    break;
	}
	case MESSAGETYPEUPDATE: {
	    debug_msg("UPDATE Packet RECEIVED\n");
	    _in_updates++;
	    gettimeofday(&_in_update_time, NULL);
	    UpdatePacket pac(buf, length);
	    // All decode errors should throw a CorruptMessage.
	    debug_msg(pac.str().c_str());
	    event_recvupdate(&pac);
	    break;
	}
	case MESSAGETYPENOTIFICATION: {
	    debug_msg("NOTIFICATION Packet RECEIVED\n");
	    try {
		NotificationPacket pac(buf, length);
		// All decode errors should throw an InvalidPacket
		_last_error[0] = pac.error_code();	// used for the MIB(?)
		_last_error[1] = pac.error_subcode();
		debug_msg(pac.str().c_str());
		event_recvnotify();
	    } catch (InvalidPacket& err) {
		XLOG_WARNING("Received Invalid Notification Packet\n");
		// We received a bad notification packet.  We don't
		// want to send a notification in response to a
		// notification, so we need to treat this as if it were
		// a good notification and tear the connection down.
		// Pretend we received a CEASE, which is really the
		// catchall error code.
		// XXX we should rather use a 'bad notification' error code?
		NotificationPacket pac(CEASE);
		_last_error[0] = pac.error_code();	// used for the MIB(?)
		_last_error[1] = pac.error_subcode();
		event_recvnotify();
	    }
	    break;
	}
	default:
	    /*
	    ** Send a notification to the peer. This is a bad message type.
	    */
	    XLOG_ERROR("Unknown packet type %d", header->type);
	    notify_peer_of_error(MSGHEADERERR, BADMESSTYPE);
	    event_tranfatal();
	    return false;
	}
    } catch(CorruptMessage c) {
	/*
	** This peer had sent us a bad message. Send a notification
	** and drop the the peering.
	*/
	XLOG_WARNING("From peer %s: %s", peerdata()->iptuple().str().c_str(),
		     c.why().c_str());
	notify_peer_of_error(c.error(), c.subcode(), c.data(), c.len());
	event_tranfatal();

	return false;
    }

    /*
    ** If we are still connected and supposed to be reading.
    */
    if (!is_connected() || !still_reading())
	return false;

    return true;
}

PeerOutputState
BGPPeer::send_message(const BGPPacket& p)
{
    debug_msg("Packet sent of type %d\n", p.type());

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
    debug_msg("Buffer for sent packet is %x\n", (uint)buf);


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
	/*drop through to next case*/
    case SocketClient::FLUSHING:
	debug_msg("event: flushing\n");
	debug_msg("Freeing Buffer for sent packet: %x\n", (uint)buf);
	delete[] buf;
	break;
    case SocketClient::ERROR:
	debug_msg("event: error\n");
	/* Don't free the message here we'll get it in the flush */
	XLOG_ERROR("Writing buffer failed: %s",  strerror(errno));
	// _SocketClient->disconnect();
	event_closed();
	// XLOG_ASSERT(!is_connected());
    }
}

void
BGPPeer::send_notification(const NotificationPacket *p, bool error)
{
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
    const uint8_t *buf = (const uint8_t *)p->encode(ccnt);
    debug_msg("Buffer for sent packet is %x\n", (uint)buf);

    /*
    ** This write is async. So we can't free the data now,
    ** we will deal with it in the complete routine.  */
    bool ret =_SocketClient->send_message(buf, ccnt,
	       callback(this, &BGPPeer::send_notification_complete, error));

    if (!ret) {
	delete[] buf;
	return;
    }

    start_stopped_timer();
}

void
BGPPeer::send_notification_complete(SocketClient::Event ev,
				    const uint8_t* buf, bool error)
{
    switch (ev) {
    case SocketClient::DATA:
	debug_msg("Notification sent\n");
	XLOG_ASSERT(STATESTOPPED == _state);
	delete[] buf;
	set_state(STATEIDLE, error);
	break;
    case SocketClient::FLUSHING:
	delete[] buf;
	break;
    case SocketClient::ERROR:
	debug_msg("Notification not sent\n");
	XLOG_ASSERT(STATESTOPPED == _state);
	set_state(STATEIDLE, error);
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
    XLOG_WARNING("Unable to send Notification so taking peer to idle");

    /*
    ** If the original notification was not an error such as sending a
    ** CEASE. If we arrived here due to a timeout, something has gone
    ** wrong so unconditionally set the error to true.
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
    switch(_state) {

    case STATESTOPPED:
	flush_transmit_queue();		// ensure callback can't happen
	set_state(STATEIDLE);	// go through STATEIDLE to clear resources
	// fallthrough now to process the start event
    case STATEIDLE:
	// Initalise resources
	start_connect_retry_timer();
	set_state(STATECONNECT);
	if (connect_to_peer())
	    event_open();		// Event = EVENTBGPTRANOPEN
	else
	    event_openfail();		// Event = EVENTBGPCONNOPENFAIL
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
BGPPeer::event_stop()			// EVENTBGPSTOP
{ 
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
	// Send Notification Message with error code of CEASE.
	send_notification(NotificationPacket(CEASE), false);
	set_state(STATESTOPPED);
	break;

    case STATESTOPPED:
	// a second stop will cause us to give up on sending the CEASE
	flush_transmit_queue();		// ensure callback can't happen
	set_state(STATEIDLE);
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
BGPPeer::event_open()			// EVENTBGPTRANOPEN
{ 
    switch(_state) {
    case STATEOPENSENT:
    case STATEOPENCONFIRM:
    case STATEESTABLISHED:
    case STATESTOPPED:
    case STATEIDLE:
	XLOG_FATAL("can't get EVENTBGPTRANOPEN in state %s",
		pretty_print_state(_state) );
	break;

    case STATECONNECT:
    case STATEACTIVE: {
	OpenPacket *open_packet =
	    new OpenPacket(_localdata->get_as_num(),
		    _localdata->get_id(),
		    _peerdata->get_configured_hold_time());

	list <const BGPParameter*>::const_iterator
	    pi = _peerdata->parameter_sent_list().begin();
	while(pi != _peerdata->parameter_sent_list().end()) {
	    open_packet->add_parameter( (const BGPParameter *)*pi );
	    pi++;
	}

	send_message(*open_packet);

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
    switch(_state) {
    case STATEIDLE:
	break;

    case STATECONNECT:
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
    switch(_state) {
    case STATEIDLE:
    case STATEACTIVE:
    case STATEOPENSENT:
    case STATEOPENCONFIRM:
    case STATEESTABLISHED:
    case STATESTOPPED:
	XLOG_FATAL("can't get EVENTBGPCONNOPENFAIL in state %s",
		pretty_print_state(_state) );
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
    switch(_state) {
    case STATEIDLE:
    case STATESTOPPED:
	break;

    case STATECONNECT:
    case STATEACTIVE:
	start_connect_retry_timer();
	if (_state == STATEACTIVE) {
	    // Move to the correct state before the open transition.
	    set_state(STATECONNECT);
	}
	// Initiate a transport Connection
	if (connect_to_peer())
	    event_open();		// Event = EVENTBGPTRANOPEN
	else
	    event_openfail();		// Event = EVENTBGPCONNOPENFAIL
	break;

    /*
     * if these happen, we failed to properly cancel a timer (?)
     */
    case STATEOPENSENT:
    case STATEOPENCONFIRM:
    case STATEESTABLISHED:
	// Send Notification Message with error code of FSM error.
	// XXX this needs to be revised.
	XLOG_WARNING("FSM received EVENTCONNTIMEEXP in state %s",
	    pretty_print_state(_state));
	send_notification(NotificationPacket(FSMERROR));
	set_state(STATESTOPPED, true);
	break;
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
    case STATEESTABLISHED:
	// Send Notification Message with error code of Hold Timer expired.
	send_notification(NotificationPacket(HOLDTIMEEXP));
	set_state(STATESTOPPED, true);
	break;
    }
}

/*
 * EVENTKEEPALIVEEXP event.
 * A keepalive timer expired.
 */
void
BGPPeer::event_keepexp()			// EVENTKEEPALIVEEXP
{ 
    switch(_state) {
    case STATEIDLE:
    case STATESTOPPED:
    case STATECONNECT:
    case STATEACTIVE:
    case STATEOPENSENT:
	XLOG_FATAL("FSM received EVENTKEEPALIVEEXP in state %s",
	    pretty_print_state(_state));
	break;

    case STATEOPENCONFIRM:
    case STATEESTABLISHED:
	start_keepalive_timer();
	send_message(KeepAlivePacket());
	break;
    }
}

/*
 * EVENTRECOPENMESS event.
 * We receive an open packet.
 */
void
BGPPeer::event_openmess(const OpenPacket* p)		// EVENTRECOPENMESS
{
    switch(_state) {
    case STATEIDLE:
    case STATECONNECT:
    case STATEACTIVE:
	XLOG_FATAL("FSM received EVENTRECOPENMESS in state %s",
	    pretty_print_state(_state));
	break;

    case STATEOPENSENT:
	// Process OPEN MESSAGE
	try {
	    check_open_packet(p);
	    // extract open msg data into peerdata.
	    send_message(KeepAlivePacket());

	    // set negotiated hold time as the holdtime
	    // if negotiated hold time is zero don't start either
	    // keepalive or hold timer
	    if ( _peerdata->get_hold_duration() > 0) {
		// start timers
		debug_msg("Starting timers\n");
		start_keepalive_timer();
		start_hold_timer();
	    }
	    // if AS number is the same as the local AS number set
	    // connection as internal otherwise set as external
	    if ( _localdata->get_as_num() == _peerdata->get_as_num() )
		_peerdata->set_internal_peer(true);
	    else
		_peerdata->set_internal_peer(false);

	    // add in parameters and capabilities

	    list <BGPParameter*>::const_iterator pi =
		p->parameter_list().begin();
	    while (pi != p->parameter_list().end()) {
		_peerdata->add_recv_parameter( (BGPParameter *)*pi );
		pi++;
	    }
	    set_state(STATEOPENCONFIRM);
	} catch(CorruptMessage& mess) {
	    send_notification(
		NotificationPacket(mess.error(), mess.subcode()));
	    set_state(STATESTOPPED, true);
	}
	break;

    case STATEOPENCONFIRM:
    case STATEESTABLISHED:
	// Send Notification - FSM error
	XLOG_WARNING("FSM received EVENTRECOPENMESS in state %s",
	    pretty_print_state(_state));
	send_notification(NotificationPacket(FSMERROR));
	set_state(STATESTOPPED, true);
	break;

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
    switch(_state) {
    case STATEIDLE:
    case STATECONNECT:
    case STATEACTIVE:
	XLOG_FATAL("FSM received EVENTRECKEEPALIVEMESS in state %s",
	    pretty_print_state(_state));
	break;

    case STATESTOPPED:
	break;

    case STATEOPENSENT:
	// Send Notification Message with error code of FSM error.
	XLOG_WARNING("FSM received EVENTRECKEEPALIVEMESS in state %s",
	    pretty_print_state(_state));
	send_notification(NotificationPacket(FSMERROR));
	set_state(STATESTOPPED, true);
	break;

    case STATEOPENCONFIRM:	// this is what we were waiting for.
	set_state(STATEESTABLISHED);
	break;

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
BGPPeer::event_recvupdate(const UpdatePacket *p) // EVENTRECUPDATEMESS
{ 
    switch(_state) {
    case STATEIDLE:
    case STATECONNECT:
    case STATEACTIVE:
	XLOG_FATAL("FSM received EVENTRECUPDATEMESS in state %s",
	    pretty_print_state(_state));
	break;

    case STATEOPENSENT:
    case STATEOPENCONFIRM:
	// Send Notification Message with error code of FSM error.
	XLOG_WARNING("FSM received EVENTRECUPDATEMESS in state %s",
	    pretty_print_state(_state));
	send_notification(NotificationPacket(FSMERROR));
	set_state(STATESTOPPED, true);
	break;

    case STATEESTABLISHED: {
	NotificationPacket *notification = check_update_packet(p);
	if ( notification != 0 ) {	// bad message
	    // Send Notification Message
	    send_notification(notification);
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
// 	    printf("rewrite\n");
	    list<PathAttribute*> l = p->pathattribute_list();
	    list<PathAttribute*>::iterator i;
	    for (i = l.begin(); i != l.end(); i++) {
		if (NEXT_HOP == (*i)->type()) {
		    delete (*i);
		    (*i) = new NextHopAttribute<IPv4>(next_hop);
		    break;
		}
	    }
// 	    printf("New packet %s\n", p->str().c_str());
	}
	_handler->process_update_packet(p);

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
BGPPeer::event_recvnotify()			// EVENTRECNOTMESS
{ 
    switch(_state) {
    case STATEIDLE:
    case STATECONNECT:
    case STATEACTIVE:
	XLOG_FATAL("FSM received EVENTRECNOTMESS in state %s",
	    pretty_print_state(_state));
	break;

    case STATEOPENCONFIRM:
    case STATEESTABLISHED:
	set_state(STATEIDLE, true);
	break;

    case STATEOPENSENT:
	// Send Notification Message with error code of FSM error.
	XLOG_WARNING("FSM Error 1 event EVENTRECUPDATEMESS");
	send_notification(NotificationPacket(FSMERROR));
	set_state(STATESTOPPED, true);
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
	XLOG_FATAL("Attempt to send invalid error code %d subcode %d",
		   error, subcode);
    }

    /*
     * An error has occurred. If we still have a connection to the
     * peer send a notification of the error.
     */
    if (is_connected()) {
	send_notification(NotificationPacket(error, subcode, data, len));
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
	_SocketClient->connected(sock);
	event_open();
    } else {
	debug_msg("rejected\n");
	if (-1 == ::close(sock)) {
	    XLOG_WARNING("Close of incoming connection failed: %s",
			 strerror(errno));
	}
    }
}

void
BGPPeer::check_open_packet(const OpenPacket *p) throw(CorruptMessage)
{
    if (p->Version() != BGPVERSION)
	xorp_throw(CorruptMessage,
		   c_format("Unsupported BGPVERSION %d", p->Version()),
		   OPENMSGERROR, UNSUPVERNUM);
    if (p->AutonomousSystemNumber() != _peerdata->get_as_num()) {
	debug_msg("**** Peer has %s, should have %s ****\n",
		  p->AutonomousSystemNumber().str().c_str(),
		  _peerdata->get_as_num().str().c_str());
	xorp_throw(CorruptMessage,
		   c_format("Wrong AS %s expecting %s",
		      p->AutonomousSystemNumber().str().c_str(),
		      _peerdata->get_as_num().str().c_str()),
		   OPENMSGERROR, BADASPEER);
    }

    // XXX What do we check for a bad BGP ID?
    _peerdata->set_id(p->BGPIdentifier());

    // put received parameters into the peer data.
    _peerdata->clone_parameters(  p->parameter_list() );
    // check the received parameters
    if (_peerdata->unsupported_parameters() == true)
	xorp_throw(CorruptMessage,
		   c_format("Unsupported parameters"),
		   OPENMSGERROR, UNSUPOPTPAR);

    // XXX Need to add auth check here

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

    _peerdata->dump_peer_data();
    debug_msg("check_open_packet says it's OK with us\n");
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
    ** First check for the mandatory fields:
    ** ORIGIN
    ** AS_PATH
    ** NEXT_HOP
    */

    // if the path attributes length is 0 then no network
    // layer reachability information should be present.

    if ( p->pathattribute_list().size()==0 ) {
	if ( p->nlri_list().size()!=0 )	{
	    debug_msg("Zero length path attribute list and "
		      "non-zero NLRI list\n");
	    return new NotificationPacket(UPDATEMSGERR, MALATTRLIST);
	}
    } else {
	// if we have path attributes, check that they are valid.
	bool local_pref;
	local_pref = false;

	list<PathAttribute*> l = p->pathattribute_list();
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
					(*i)->data(), (*i)->size());
		}
	    }
	}

	if (origin_attr == NULL) {
	    debug_msg("Missing ORIGIN\n");
	    uint8_t data = ORIGIN;
	    return new NotificationPacket(UPDATEMSGERR, MISSWATTR, &data, 1);
	}

	// The AS Path attribute is mandatory
	if (as_path_attr == NULL) {
	    debug_msg("Missing AS_PATH\n");
	    uint8_t data = AS_PATH;
	    return new NotificationPacket(UPDATEMSGERR, MISSWATTR, &data, 1);
	}

	if (!ibgp()) {
	    // If this is an EBGP peering, the AS Path MUST NOT be empty
	    if (as_path_attr->as_path().path_length() == 0)
		return new NotificationPacket(UPDATEMSGERR, MALASPATH);

	    // If this is an EBGP peering, the AS Path MUST start
	    // with the AS number of the peer.
	    AsNum my_asnum(peerdata()->get_as_num());
	    if (as_path_attr->as_path().first_asnum() != my_asnum)
		return new NotificationPacket(UPDATEMSGERR, MALASPATH);
	}

	// The NEXT_HOP attribute is mandatory
	if (next_hop_attr == NULL) {
	    debug_msg("Missing NEXT_HOP\n");
	    uint8_t data = NEXT_HOP;
	    return new NotificationPacket(UPDATEMSGERR, MISSWATTR, &data, 1);
	}

	// XXX need also to check that the nexthop address is not
	// one of our addresses, and with single hop EBGP that the
	// nexthop address shares a subnet with us or is the
	// address of our peer
	// if so, we'd discard the route, but not send a notification

	if (ibgp() && !local_pref)
	    XLOG_WARNING("Update packet from ibgp peer %s: no LOCAL_PREF",
			 peerdata()->iptuple().str().c_str());

	if (!ibgp() && local_pref)
	    XLOG_WARNING("Update packet from ebgp peer %s: with LOCAL_PREF",
			 peerdata()->iptuple().str().c_str());

	// XXX Check Network layer reachability fields

    }

    // XXX Check withdrawn routes are correct.

    return 0;	// all correct.
}

bool
BGPPeer::established()
{
    debug_msg("stage2 initialisation %s\n",
	      peerdata()->iptuple().str().c_str());

    if (_localdata == NULL) {
	XLOG_ERROR("No _localdata");
	return false;
    }

    if (_handler == NULL) {
	// plumb peer into plumbing
	string peername = "Peer-" + peerdata()->iptuple().str();
	debug_msg("Peer is called >%s<\n", peername.c_str());
	_handler = new PeerHandler(peername, this,
				   _mainprocess->plumbing());
    } else {
	_handler->peering_came_up();
    }

    _in_updates = 0;
    _out_updates = 0;
    _in_total_messages = 0;
    _out_total_messages = 0;
    _established_transitions++;
    gettimeofday(&_established_time, NULL);
    gettimeofday(&_in_update_time, NULL);
    return true;
}

void
BGPPeer::connected(int sock)
{
    if (!_SocketClient)
	XLOG_FATAL("No socket structure");

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
//     if(is_connected()) {
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
    debug_msg("Start Connect Retry timer after %d ms\n",
	      _peerdata->get_retry_duration());

    _timer_connect_retry = _mainprocess->get_eventloop()->
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
    debug_msg("restart Connect Retry timer after %d ms\n",
	      _peerdata->get_retry_duration());

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
    debug_msg("Holdtimer started %d\n", duration);

    if (duration != 0)
	_timer_hold_time = _mainprocess->get_eventloop()->
	    new_oneoff_after_ms(duration,
	    callback(this, &BGPPeer::event_holdexp));
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
    debug_msg("KeepAlive timer started with duration %d %ld\n",
	      duration, time(0));

    if (duration > 0)
	_timer_keep_alive = _mainprocess->get_eventloop()->
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
    debug_msg("Stopped timer started with duration %d ms %ld\n", delay,
	      time(0));

    _timer_stopped = _mainprocess->get_eventloop()->
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
    debug_msg("BGPPeer::release_resources()\n");

    if (_handler != NULL && _handler->peering_is_up()) {
	_handler->peering_went_down();
    } else {
	XLOG_ERROR("PeerHandler was 0?");
    }

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

    gettimeofday(&_established_time, NULL);
    return true;
}

string
BGPPeer::str()
{
    return c_format("Peer %s is on fd is %d\n",
		    peerdata()->iptuple().str().c_str(),
		    get_sock());
}

const char *
BGPPeer::pretty_print_state(FSMState s)
{
    const char *str;

    switch (s) {
    case STATEIDLE:
	str = "IDLE(1)";
	break;
    case STATECONNECT:
	str = "CONNECT(2)";
	break;
    case STATEACTIVE:
	str = "ACTIVE(3)";
	break;
    case STATEOPENSENT:
	str = "OPENSENT(4)";
	break;
    case STATEOPENCONFIRM:
	str = "OPENCONFIRM(5)";
	break;
    case STATEESTABLISHED:
	str = "ESTABLISHED(6)";
	break;
    case STATESTOPPED:
	str = "STOPPED(7)";
	break;
    }

    return str;
}

void
BGPPeer::set_state(FSMState s, bool error)
{
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
	    if (error) {
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
BGPPeer::send_netreachability(const NetLayerReachability &n)
{
    debug_msg("send_netreachability called\n");
    UpdatePacket bup;
    bup.add_nlri(n);
    return send_message(bup);
}

uint32_t
BGPPeer::get_established_time() const
{
    struct timeval now;
    gettimeofday(&now, NULL);
    return now.tv_sec - _established_time.tv_sec;
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
    struct timeval now;
    gettimeofday(&now, NULL);
    in_update_elapsed = now.tv_sec - _in_update_time.tv_sec;
}
