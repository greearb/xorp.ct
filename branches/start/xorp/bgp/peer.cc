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

#ident "$XORP: xorp/bgp/peer.cc,v 1.64 2002/12/09 18:28:44 hodson Exp $"

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
    _ConnectionState = STATEIDLE;
    _SocketClient = sock;
    _output_queue_was_busy = false;
    _handler = NULL;
}

BGPPeer::~BGPPeer()
{
    if (_SocketClient)
	delete _SocketClient;
    delete _peerdata;
}

bool
BGPPeer::get_message(BGPPacket::Status status, const uint8_t *buf,
			 size_t length)
{
    switch (status) {
    case BGPPacket::GOOD_MESSAGE:
// action(EVENTBGPTRANFATALERR, MSGHEADERERR, BADMESSLEN);
// return false;
	break;
    case BGPPacket::ILLEGAL_MESSAGE_LENGTH:
	action(EVENTBGPTRANFATALERR, MSGHEADERERR, BADMESSLEN);
	return false;
    case BGPPacket::CONNECTION_CLOSED:
 	// _SocketClient->disconnect();
 	action(EVENTBGPTRANCLOSED);
	// XLOG_ASSERT(!is_connected());
	return false;
    }

    /*
    ** We should only have a good packet at this point.
    ** The buffer pointer must be non 0.
    */
    XLOG_ASSERT(0 != buf);

    const fixed_header *header =
	reinterpret_cast<const struct fixed_header *>(buf);
    struct fixed_header fh = *header;
    fh._length = ntohs(header->_length);

    /* XXX
    ** Put the marker authentication code here.
    */
    try {
	switch (fh._type) {
	case MESSAGETYPEOPEN: {
	    debug_msg("OPEN Packet RECEIVED\n");
	    OpenPacket pac(buf, length);
	    // All decode errors should throw a CorruptMessage.
	    pac.decode();
	    debug_msg(pac.str().c_str());
	    // want unified decode call. now need to get peerdata out.
	    _peerdata->dump_peer_data();
	    action(EVENTRECOPENMESS, &pac);
	    break;
	}
	case MESSAGETYPEKEEPALIVE: {
	    debug_msg("KEEPALIVE Packet RECEIVED %d\n", length);
	    // Decoding a KeepAlivePacket cannot fail
	    KeepAlivePacket pac(buf, length);
	    pac.decode();
	    debug_msg(pac.str().c_str());
	    action(EVENTRECKEEPALIVEMESS, &pac);
	    break;
	}
	case MESSAGETYPEUPDATE: {
	    debug_msg("UPDATE Packet RECEIVED\n");
	    UpdatePacket pac(buf, length);
	    // All decode errors should throw a CorruptMessage.
	    pac.decode();
	    debug_msg(pac.str().c_str());
	    _mainprocess->add_update(_peerdata, &pac);
	    action(EVENTRECUPDATEMESS, &pac);
	    break;
	}
	case MESSAGETYPENOTIFICATION: {
	    debug_msg("NOTIFICATION Packet RECEIVED\n");
	    try {
		NotificationPacket pac(buf, length);
		// All decode errors should throw an InvalidPacket
		pac.decode();
		debug_msg(pac.str().c_str());
		action(EVENTRECNOTMESS, &pac);
	    } catch (InvalidPacket& err) {
		XLOG_WARNING("Received Invalid Notification Packet\n");
		// We received a bad notification packet.  We don't
		// want to send a notification in response to a
		// notification, so we need to treat this as if it were
		// a good notification and tear the connection down.
		// Pretend we received a CEASE, which is really the
		// catchall error code.
		NotificationPacket pac(CEASE);
		action(EVENTRECNOTMESS, &pac);
	    }
	    break;
	}
	default:
	    /*
	    ** Send a notification to the peer. This is a bad message type.
	    */
	    XLOG_ERROR("Unknown packet type %d", header->_type);
	    action(EVENTBGPTRANFATALERR, MSGHEADERERR, BADMESSTYPE);
	    return false;
	}
    } catch(CorruptMessage c) {
	/*
	** This peer had sent us a bad message. Send a notification
	** and drop the the peering.
	*/
	XLOG_WARNING("From peer %s: %s", peerdata()->iptuple().str().c_str(),
		     c.why().c_str());
	action(EVENTBGPTRANFATALERR, c.error(), c.subcode(), c.data(),
		   c.len());

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
    debug_msg("Packet sent of type %d\n", p.get_type());

    uint8_t packet_type = p.get_type();

    if ( packet_type != MESSAGETYPEOPEN &&
	 packet_type != MESSAGETYPEUPDATE &&
	 packet_type != MESSAGETYPENOTIFICATION &&
	 packet_type != MESSAGETYPEKEEPALIVE) {
	xorp_throw(InvalidPacket,
		   c_format("Unknown packet type %d\n", packet_type));

    }

    const uint8_t *buf;
    int cnt;

    /*
    ** This buffer is dynamically allocated and should be freed.
    */
    buf = (const uint8_t *)p.encode(cnt);
    debug_msg("Buffer for sent packet is %x\n", (uint)buf);

    size_t ccnt = cnt;

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
	action(EVENTBGPTRANCLOSED);
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

    int cnt;
    /*
    ** This buffer is dynamically allocated and should be freed.  */
    const uint8_t *buf = (const uint8_t *)p->encode(cnt);
    debug_msg("Buffer for sent packet is %x\n", (uint)buf);
    size_t ccnt = cnt;

    /*
    ** This write is async. So we can't free the data now,
    ** we will deal with it in the complete routine.  */
    bool ret =_SocketClient->send_message(buf, ccnt,
			       callback(this,
					&BGPPeer::send_notification_complete,
					error));

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
	XLOG_ASSERT(STATESTOPPED == _ConnectionState);
	delete[] buf;
	set_state(STATEIDLE, error);
	break;
    case SocketClient::FLUSHING:
	delete[] buf;
	break;
    case SocketClient::ERROR:
	debug_msg("Notification not sent\n");
	XLOG_ASSERT(STATESTOPPED == _ConnectionState);
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
    XLOG_ASSERT(STATESTOPPED == _ConnectionState);
    XLOG_WARNING("Unable to send Notification so taking peer to idle");

    /*
    ** If the original notification was not an error such as sending a
    ** CEASE. If we arrived here due to a timeout, something has gone
    ** wrong so unconditionally set the error to true.
    */
    set_state(STATEIDLE, true);
}

void
BGPPeer::flush_transmit_queue()
{
    _SocketClient->flush_transmit_queue();
}

void
BGPPeer::stop_reader()
{
    _SocketClient->stop_reader();
}

bool
BGPPeer::connect_to_peer()
{
    return _SocketClient->connect();
}

FSMState
BGPPeer::get_ConnectionState()
{
    return _ConnectionState;
}

void
BGPPeer::set_ConnectionState(FSMState s)
{
    _ConnectionState = s;
}

/*
** A BGP Fatal error has ocurred. Try and send a notification.
*/
void
BGPPeer::action(FSMEvent Event,
		const int error, const int subcode,
		const uint8_t *data, const size_t len)
{
    if (EVENTBGPTRANFATALERR != Event)
	XLOG_FATAL("Only allowed event is EVENTBGPTRANFATALERR got %d",
		   Event);

    if (!NotificationPacket::validate_error_code(error, subcode)) {
	XLOG_FATAL("Attempt to send invalid error code %d subcode %d",
		   error, subcode);
    }

    /*
    ** An error has occurred. If we still have a connection to the
    ** peer send a notification of the error.
    */
    if (is_connected()) {
	if (0 == data || 0 == len)
	    send_notification(NotificationPacket(error, subcode));
	else
	    send_notification(NotificationPacket(data, len, error,
						    subcode));
	set_state(STATESTOPPED, true);

	return;
    }

    state_machine(Event);
}

/*
** Handle incoming connection events.
**
** This method should only ever be called with a FSMEvent of
** EVENTBGPTRANOPEN. The redundant first argument is here for
** consistency with other calls to action.
**
** The second argment sock is the incoming socket descriptor. Which
** must be used or closed by this method.
*/
void
BGPPeer::action(FSMEvent ConnectionEvent, const int sock)
{
    if (EVENTBGPTRANOPEN != ConnectionEvent)
	XLOG_FATAL("Only allowed event is EVENTBGPTRANOPEN got %d",
		   ConnectionEvent);

    bool accept = false;

    switch (_ConnectionState) {
    case STATEIDLE:
	accept = false;
	break;
    case STATECONNECT:
	accept = true;
	break;
    case STATEACTIVE:
	accept = true;
	break;
    case STATEOPENSENT:
	accept = false;
	break;
    case STATEOPENCONFIRM:
	accept = false;
	break;
    case STATEESTABLISHED:
	accept = false;
	break;
    case STATESTOPPED:
	/*
	** We are state stopped because we are waiting for a
	** notification packet to be sent. In the meantime our peer
	** has made a connection request. So drive the state machine
	** through the idle state which should dump all state and the
	** connection.
	*/
	set_state(STATEIDLE);
	accept = true;
	break;
    }

    debug_msg("Connection attempt: State %d ", _ConnectionState);
    if (accept) {
	debug_msg("accepted\n");
	_SocketClient->connected(sock);
    } else {
	debug_msg("rejected\n");
	if (-1 == ::close(sock)) {
	    XLOG_WARNING("Close of incoming connection failed: %s",
			 strerror(errno));
	}
    }

    state_machine(ConnectionEvent);
}

void
BGPPeer::action(FSMEvent ConnectionEvent, const BGPPacket *p)
{
    state_machine(ConnectionEvent, p);
}

void
BGPPeer::state_machine(FSMEvent ConnectionEvent, const BGPPacket *p)
{
    uint8_t res;

    if (p != NULL)
	debug_msg("BGPPeer STATE %d EVENT %d and %d\n",
		  _ConnectionState, ConnectionEvent, p->get_type());

    switch (_ConnectionState) {
/******************************************/
    case STATEIDLE:
	debug_msg("State Idle\n");
	switch (ConnectionEvent) {
	case EVENTBGPSTART:
	    // Initalise resources
	    if ( initialise_stage1() ) {
		set_state(STATECONNECT);
		// Event = EVENTBGPTRANOPEN
		state_machine(EVENTBGPTRANOPEN, NULL);
	    } else {
 		set_state(STATECONNECT);
		// Event = EVENTBGPCONNOPENFAIL
		state_machine(EVENTBGPCONNOPENFAIL, NULL);
	    }
	    break;
	default:
	    break;
	}
	break;
/******************************************/
    case STATESTOPPED:
	/*
	 * State Stopped is the state we're in after we've queued a
	 * CEASE notification, but before it's actually been sent.  We
	 * ignore packet events, but a further STOP event or error
	 * will cause is to stop waiting for the CEASE to be sent, and
	 * a START event will cause us to transition to idle, and then
	 * process the START event as normal */
	debug_msg("State Stopped\n");
	switch (ConnectionEvent) {
	case EVENTBGPSTART:
	    // ensure callback can't happen
	    flush_transmit_queue();

	    // go through STATEIDLE to clear resources
	    set_state(STATEIDLE);

	    // now process the start event
	    state_machine(EVENTBGPSTART, p);
	    break;
	case EVENTBGPSTOP:
	    // a second stop will cause us to give up on sending the CEASE
	case EVENTBGPTRANCLOSED:
	case EVENTBGPTRANFATALERR:
	    // ensure callback can't happen
	    flush_transmit_queue();

	    set_state(STATEIDLE);
	    break;
	case EVENTBGPTRANOPEN:
	    /*
	    ** The only way that we should be able to get here is if a
	    ** connection comes in from our peer. However if a
	    ** connection already exists we *won't* be called.
	    **
	    ** THIS CAN NEVER HAPPEN.
	    */
	    XLOG_FATAL(
	         "Connection accepted while waiting for cease transmission");
	default: {
	    // we don't care about any other events
	    }
	}
	break;
/******************************************/
    case STATECONNECT:
	debug_msg("State Connect\n");
	switch (ConnectionEvent) {
	case EVENTBGPSTART:
	    break;
	case EVENTBGPTRANOPEN:
	    // Complete initalisation
	    if ( initialise_stage2() ) {
		// Send Open Packet
		debug_msg("Open packet sent\n");

		OpenPacket *open_packet = new OpenPacket(_localdata->get_as_num(), _localdata->get_id(), _peerdata->get_configured_hold_time());

		list <const BGPParameter*>::const_iterator pi = _peerdata->parameter_sent_list().begin();
		while(pi != _peerdata->parameter_sent_list().end())
                    {
			open_packet->add_parameter( (const BGPParameter *)*pi );
			pi++;
                    }

		send_message(*open_packet);

		// Clear ConnectRetry Timer
		clear_connect_retry_timer();
		set_state(STATEOPENSENT);
	    } else {
		// exit and throw exeception that can't iniatlise.
	    }

	    break;
	case EVENTBGPCONNOPENFAIL:
	    // Restart ConnectRetry Timer
	    restart_connect_retry_timer();
	    // Continue to listen for a connection
	    set_state(STATEACTIVE);
	    break;
	case EVENTCONNTIMEEXP:
	    // Restart ConnectRetry Timer
	    restart_connect_retry_timer();
	    // Initiate a transport Connection
	    if (connect_to_peer()) {
		    // Event = EVENTBGPTRANOPEN
		    state_machine(EVENTBGPTRANOPEN, NULL);
	    } else {
		// Event = EVENTBGPCONNOPENFAIL
		state_machine(EVENTBGPCONNOPENFAIL, NULL);
	    }
	default:
	    set_state(STATEIDLE);
	    break;
	}
	break;
/******************************************/
    case STATEACTIVE:
	debug_msg("State Active\n");
	switch (ConnectionEvent) {
	case EVENTBGPSTART:
	    // none - stay in state STATEACTIVE
	    break;
	case EVENTBGPTRANOPEN:
	    // Complete initalisation
	    if ( initialise_stage2() ) {
		// Send Open Packet
		debug_msg("Open packet sent\n");

	        OpenPacket *open_packet = new OpenPacket(_localdata->get_as_num(), _localdata->get_id(), _peerdata->get_configured_hold_time());

		list <const BGPParameter*>::const_iterator pi = _peerdata->parameter_sent_list().begin();
		while(pi != _peerdata->parameter_sent_list().end())
                    {
			open_packet->add_parameter( (const BGPParameter *)*pi );
			pi++;
                    }

		send_message(*open_packet);

		// Clear ConnectRetry Timer
		clear_connect_retry_timer();
		// Start Holdtimer
		start_hold_timer();
		// Change state to OpenSent
		set_state(STATEOPENSENT);
	    } else {
		// XXX
		// exit and throw exeception that can't iniatlise.
	    }
	    break;
	case EVENTBGPCONNOPENFAIL:
	    // This case is for rejecting anon-peers, or being rejected by a peer.
	    // TODO - sort this.

	    // Close Connection
	    _SocketClient->disconnect();
	    // Restart ConnectRetry Timer
	    restart_connect_retry_timer();
	    // set_event(STATEACTIVE);
	    break;
	case EVENTCONNTIMEEXP:
	    // Restart ConnectRetry Timer
	    restart_connect_retry_timer();
	    // Move to the correct state before the open transition.
 	    set_state(STATECONNECT);
	    // Initiate a transport Connection
	    if (connect_to_peer()) {
		// Event = EVENTBGPTRANOPEN
		state_machine(EVENTBGPTRANOPEN, NULL);
	    } else {
		// Event = EVENTBGPCONNOPENFAIL
		state_machine(EVENTBGPCONNOPENFAIL, NULL);
	    }
	    break;

	default:
	    // XXX code was previously unreachable
	    set_state(STATEIDLE);
	}
	break;
/******************************************/
    case STATEOPENSENT:
	debug_msg("State Open Sent\n");
	switch (ConnectionEvent) {

	case EVENTBGPSTART:
	    // none - stay in same state OPENSENT
	    break;

	case EVENTBGPTRANCLOSED:
	    // Close Connection
	    _SocketClient->disconnect();
	    // Restart ConnectRetry Timer
	    restart_connect_retry_timer();
	    set_state(STATEACTIVE);
	    break;

	case EVENTBGPTRANFATALERR:
	    set_state(STATEIDLE, true);
	    break;

	case EVENTRECOPENMESS:
	    // Process OPEN MESSAGE
	    res = check_open_packet((const OpenPacket*)p);
	    if (res == OPENMSGOK) {
		// extact open msg data into peerdata.

		// send keepalive message
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
		// THIS SHOULD BE PUT BACK IN IF STATEMENT ABOVE,
		// SINCE PEER DATA IS NOT BEING SET TIMERS NOT BEING
		// STARTED
		start_keepalive_timer();
		// if AS number is the same as the local AS number set
		// connection as internal otherwise set as external
		if ( _localdata->get_as_num() == _peerdata->get_as_num() )
		    _peerdata->set_internal_peer(true);
		else
		    _peerdata->set_internal_peer(false);

		// add in parameters and capabilities

		list <BGPParameter*>::const_iterator pi =
		    ((const OpenPacket*)p)->parameter_list().begin();
		 while(pi != ((const OpenPacket*)p)->parameter_list().end())
                    {
			_peerdata->add_recv_parameter( (BGPParameter *)*pi );
			pi++;
                    }


		set_state(STATEOPENCONFIRM);
	    } else {
		// Send Notification Message
		send_notification(NotificationPacket(OPENMSGERROR, res));
		set_state(STATESTOPPED, true);
	    }
	    break;

	case EVENTHOLDTIMEEXP:
	    // Send Notification Message with error code of Hold Timer expired.
	    send_notification(NotificationPacket(HOLDTIMEEXP));
	    set_state(STATESTOPPED, true);
	    break;

	case EVENTBGPSTOP:
	    // Send Notification Message with error code of CEASE.
	    send_notification(NotificationPacket(CEASE), false);
	    set_state(STATESTOPPED, false);
	    break;

	case EVENTRECNOTMESS:
	    set_state(STATEIDLE, true);
	    break;

	case EVENTBGPTRANOPEN:
	    // The action routine has already closed the incoming
	    // socket. So just ignore this connection attempt.
	    break;

	default:
 	    // Send Notification Message with error code of FSM error.
 	    XLOG_WARNING("FSM Error 1 event %d", ConnectionEvent);
 	    send_notification(NotificationPacket(FSMERROR));
 	    set_state(STATESTOPPED, true);
	}
	break;
/******************************************/
    case STATEOPENCONFIRM:
	// Waiting for a KEEPALIVE or NOTIFICATION method
	debug_msg("State Open Confirm\n");

	switch (ConnectionEvent) {
	case EVENTBGPSTART:
	    // none - Stay in same state OPENCONFIRM
	    break;

	case EVENTBGPTRANCLOSED:
	    set_state(STATEIDLE, true);
	    break;

	case EVENTBGPTRANFATALERR:
	    set_state(STATEIDLE, true);
	    break;

	case EVENTKEEPALIVEEXP:
	    // Restart KeepAlive Timer
	    restart_keepalive_timer();
	    // Send KeepAlive packet
	    if ( check_send_keepalive() )
		send_message(KeepAlivePacket());
	    set_state(STATEOPENCONFIRM);
	    break;

	case EVENTRECKEEPALIVEMESS:
	    set_state(STATEESTABLISHED);
	    break;

	case EVENTRECNOTMESS:
	    set_state(STATEIDLE, true);
	    break;

	case EVENTHOLDTIMEEXP:
	    // Send Notification that notification timer expred
	    send_notification(NotificationPacket(HOLDTIMEEXP));
	    set_state(STATESTOPPED, true);
	    break;

	case EVENTBGPSTOP:
	    // Send Notification that notification - cease
	    send_notification(NotificationPacket(CEASE), false);
	    set_state(STATESTOPPED, false);
	    break;

	default:
	    // Send Notification - FSM error
 	    XLOG_WARNING("FSM Error 2 event %d", ConnectionEvent);
	    send_notification(NotificationPacket(FSMERROR));
	    set_state(STATESTOPPED, true);
	}
	break;
/******************************************/
    case STATEESTABLISHED:
	debug_msg("State Established STATE %d\n", ConnectionEvent);

	switch (ConnectionEvent) {
	case EVENTBGPSTART:
	    break;

	case EVENTBGPSTOP:
	    // Send Notification that notification - cease
	    send_notification(NotificationPacket(CEASE), false);
	    set_state(STATESTOPPED, false);
	    break;

	case EVENTBGPTRANCLOSED:
	    set_state(STATEIDLE, true);
	    break;

	case EVENTBGPTRANFATALERR:
	    set_state(STATEIDLE, true);
	    break;

	case EVENTKEEPALIVEEXP:
	    // Restart Keep Alive Timer
	    restart_keepalive_timer();
	    // Send Keep Alive message
	    // if ( check_send_keepalive() )
	    send_message(KeepAlivePacket());
	    break;

	case EVENTHOLDTIMEEXP:
	    // send notifcation message with error code hold timer expired.
	    send_notification(NotificationPacket(HOLDTIMEEXP));
	    set_state(STATESTOPPED, true);
	    break;

	case EVENTRECKEEPALIVEMESS:
	    // Restart Hold Timer - if negotiated hold time is zero
	    // don't start hold timer
	    if ( _peerdata->get_hold_duration() > 0) {
		// start timers
		restart_hold_timer();
	    } else {
		clear_hold_timer();
	    }
	    set_state(STATEESTABLISHED);
	    break;

	case EVENTRECUPDATEMESS: {
	    NotificationPacket *notification;
	    notification
		= check_update_packet(dynamic_cast<const UpdatePacket *>(p));
	    if ( notification == NULL ) {
		// Message ok, update with data
		if ( _peerdata->get_hold_duration() > 0) {
		    // start timer
		    restart_hold_timer();
		} else
		    clear_hold_timer();
		// process the packet...
		debug_msg("Process the packet!!!\n");
		if (_handler != NULL)
		    _handler->
			process_update_packet(dynamic_cast
					      <const UpdatePacket *>(p));
	    } else {
		// Send Notification Message
		send_notification(notification);
		delete notification;
		set_state(STATESTOPPED, true);
	    }
	    break;
	}
	case EVENTRECNOTMESS:
	    set_state(STATEIDLE, true);
	    break;

	default:
	    // Send Notification - FSM error
 	    XLOG_WARNING("FSM Error 3 event %d", ConnectionEvent);
	    send_notification(NotificationPacket(FSMERROR));
	    set_state(STATESTOPPED, true);
	}
	break;

    default:
	debug_msg("Other illegal state\n");
    }
}

bool
BGPPeer::check_send_keepalive()
{
    // do checks as per section 4.4 of draft-ietf-idr-bgp4-12.txt
    return true;
}

uint8_t
BGPPeer::check_open_packet(const OpenPacket *p)
{
    // return codes
    // 1 - Unsupported version number
    // 2 - Bad Peer AS
    // 3 - Bad BGP identifier
    // 4 - Unsupported Optional Parameter
    // 5 - Authentication failed
    // 6 - Unacceptable Hold Time

    if (p->Version() != BGPVERSION)
	return UNSUPVERNUM;

    if (p->AutonomousSystemNumber() != _peerdata->get_as_num()) {
	debug_msg("**** Peer has %s, should have %s ****\n",
		  p->AutonomousSystemNumber().str().c_str(),
		  _peerdata->get_as_num().str().c_str());
	return BADASPEER;
    }

    // XXX What do we check for a bad BGP ID?
    _peerdata->set_id(p->BGPIdentifier());

    // put received parameters into the peer data.
    _peerdata->clone_parameters(  p->parameter_list() );
    // check the receieved parameters
    if (_peerdata->unsupported_parameters() == true)
	return UNSUPOPTPAR;

    // XXX Need to add auth check here

    /*
    ** Set the holdtime and keepalive times.
    **
    ** Internally we store the values in milliseconds. The value sent
    ** on the wire is in seconds. First check that we have been
    ** offered a legal value. If the value is legal convert it to
    ** milliseconds.  Compare the holdtime in the packet against our
    ** configured value and choose the lowest value.
    **
    ** The spec suggests using a keepalive time of a third the hold
    ** duration.
    */
    try {
	check_hold_duration(p->HoldTime());
    } catch(CorruptMessage& mess) {
	return UNACCEPTHOLDTIME;
    }

    uint16_t hold_secs = p->HoldTime() <
	_peerdata->get_configured_hold_time() ?
	p->HoldTime() : _peerdata->get_configured_hold_time();

    uint32_t hold_ms = hold_secs * 1000;
    _peerdata->set_hold_duration(hold_ms);
    _peerdata->set_keepalive_duration(hold_ms / 3);

    _peerdata->dump_peer_data();
    debug_msg("check_open_packet says it's OK with us\n");

    return OPENMSGOK;
}

void
BGPPeer::check_hold_duration(int32_t hold_secs)
    throw(CorruptMessage)
{
    if (0xffff < hold_secs || 0 > hold_secs ||
       1 == hold_secs || 2 == hold_secs)
	xorp_throw(CorruptMessage,
		   c_format("Illegal holdtime value %d secs", hold_secs),
		   OPENMSGERROR, UNACCEPTHOLDTIME);
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
		    return new NotificationPacket((*i)->get_data(),
						  UPDATEMSGERR,
						  UNRECOGWATTR,
						  (*i)->get_size());
		}
	    }
	}

	if (origin_attr == NULL) {
	    debug_msg("Missing ORIGIN\n");
	    uint8_t data = ORIGIN;
	    return new NotificationPacket(&data, UPDATEMSGERR, MISSWATTR, 1);
	}

	// The AS Path attribute is mandatory
	if (as_path_attr == NULL) {
	    debug_msg("Missing AS_PATH\n");
	    uint8_t data = AS_PATH;
	    return new NotificationPacket(&data, UPDATEMSGERR, MISSWATTR, 1);
	}

	if (!ibgp()) {
	    // If this is an EBGP peering, the AS Path MUST NOT be empty
	    if (as_path_attr->as_path().get_path_length() == 0)
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
	    return new NotificationPacket(&data, UPDATEMSGERR,
					  MISSWATTR, 1);
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

	// XXXX Check Network layer reachability fields

    }

    // XXXX Check withdrawn routes are correct.

    return 0;
}

bool
BGPPeer::initialise_stage1()
{
    // Create Routing Table entries.

    // Create Client Socket

    // Accept connection from this peer.
    // XXX - Not sure what this is here for - Atanu.
    _mainprocess->accept_connection_from(_peerdata);

    // Start connection retry timer

    start_connect_retry_timer();

    return connect_to_peer();
}

bool BGPPeer::initialise_stage2()
{
    return true;
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
    action(EVENTBGPTRANOPEN, sock);
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
			    callback(hook_connect_retry, this));
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

void
BGPPeer::start_hold_timer()
{
    uint32_t duration = _peerdata->get_hold_duration();
    debug_msg("Holdtimer started %d\n", duration);

    if (0 == duration)
	return;

    _timer_hold_time = _mainprocess->get_eventloop()->
	new_oneoff_after_ms(duration, callback(hook_hold_time, this));
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

    if (0 == duration)
	return;

    _timer_keep_alive = _mainprocess->get_eventloop()->
	new_oneoff_after_ms(duration, callback(hook_keep_alive, this));
}

void
BGPPeer::clear_keepalive_timer()
{
    debug_msg("KeepAlive timer cleared\n");

    _timer_keep_alive.unschedule();
}

void
BGPPeer::restart_keepalive_timer()
{
    debug_msg("KeepAlive timer restarted\n");

    clear_keepalive_timer();
    start_keepalive_timer();
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

    return true;
}

string
BGPPeer::str()
{
    return c_format("Peer %s is on fd is %d\n",
		    peerdata()->iptuple().str().c_str(),
		    get_sock());
}

bool
BGPPeer::is_connected()
{
    return _SocketClient->is_connected();
}

bool
BGPPeer::still_reading()
{
    return _SocketClient->still_reading();
}


/* ********************** Private Methods ************************* */
inline const char *
pretty_print_state(FSMState s)
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
	    pretty_print_state(_ConnectionState),
	    pretty_print_state(s));
    FSMState previous_state = _ConnectionState;
    _ConnectionState = s;

    if (previous_state == STATESTOPPED && _ConnectionState != STATESTOPPED)
	clear_stopped_timer();

    switch (_ConnectionState) {
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
		action(EVENTBGPSTART);
	    }

	    if (previous_state == STATECONNECT) {
	    }
	    if (previous_state == STATEACTIVE) {
	    }
	    if (previous_state == STATEOPENSENT) {
	    }
	    if (previous_state == STATEOPENCONFIRM) {
	    }
	    if (previous_state == STATEESTABLISHED) {
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

/* ********************** Timer Hook Methods ********************** */

void
hook_connect_retry(BGPPeer *b)
{
    debug_msg("Hook Connect Retry called\n");

    b->action(EVENTCONNTIMEEXP);

    b->start_connect_retry_timer();
}


void
hook_hold_time(BGPPeer *b)
{
    debug_msg("Hook Hold Time called\n");

    b->action(EVENTHOLDTIMEEXP);

    b->start_hold_timer();
}

void
hook_keep_alive(BGPPeer *b)
{
    debug_msg("Hook KeepAlive called\n");

    b->action(EVENTKEEPALIVEEXP);

    b->start_keepalive_timer();
}

#ifdef	LATER
void hook_min_as_orig_interval(BGPPeer */*b*/)
{
    debug_msg("Hook min AS orig interval called \n");

}

void hook_min_route_adv_interval(BGPPeer */*b*/)
{
    debug_msg("Hook route adv interval called \n");
}
#endif

PeerOutputState
BGPPeer::send_update_message(const UpdatePacket& p)
{
    PeerOutputState queue_state;
    debug_msg("send_update_message called\n");
    assert(STATEESTABLISHED == _ConnectionState);
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


