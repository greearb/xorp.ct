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

#include "bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/callback.hh"
#include "libcomm/comm_api.h"

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "socket.hh"
#include "packet.hh"

/* **************** BGPSocket - PUBLIC METHODS *********************** */

Socket::Socket(const Iptuple& iptuple, EventLoop& e)
    : _iptuple(iptuple), _eventloop(e)
{
    debug_msg("Socket constructor called: %s\n", _iptuple.str().c_str());

    //    _eventloop = 0;

#ifdef	DEBUG_PEERNAME
    _remote_host = "unconnected socket";
#endif
}

void
Socket::create_listener()
{
    debug_msg("create_listener called\n");

    size_t len;
    const struct sockaddr *sin = get_local_socket(len);

    XLOG_ASSERT(!_s.is_valid());

    _s = comm_bind_tcp(sin, COMM_SOCK_NONBLOCKING, get_local_interface().c_str());
    if (!_s.is_valid()) {
	XLOG_ERROR("comm_bind_tcp failed");
    }
    else {
	debug_msg("create_listener, local_interface: %s", get_local_interface().c_str());

	if (comm_listen(_s, COMM_LISTEN_DEFAULT_BACKLOG) != XORP_OK) {
	    XLOG_ERROR("comm_listen failed");
	}
    }
}

/* **************** BGPSocket - PROTECTED METHODS *********************** */

void
Socket::close_socket()
{
    debug_msg("Close socket %s %s\n", get_remote_host(), _s.str().c_str());

    comm_sock_close(_s);
    _s.clear();
}

void
Socket::create_socket(const struct sockaddr *sin, int is_blocking)
{
    debug_msg("create_socket called\n");

    XLOG_ASSERT(!_s.is_valid());

    _s = comm_sock_open(sin->sa_family, SOCK_STREAM, 0, is_blocking);
    if (!_s.is_valid()) {
	XLOG_ERROR("comm_sock_open failed");
	return;
    }

    debug_msg("BGPSocket socket created (sock - %s)\n", _s.str().c_str());
}

void
Socket::init_sockaddr(string addr, uint16_t local_port,
		      struct sockaddr_storage& ss, size_t& len)
{
    debug_msg("addr %s port %u len = %u\n", addr.c_str(),
	      XORP_UINT_CAST(local_port), XORP_UINT_CAST(len));

    string port = c_format("%d", local_port);

    int error;
    struct addrinfo hints, *res0;
    // Need to provide a hint because we are providing a numeric port number.
    memset(&hints, 0, sizeof(hints));
#ifdef HOST_OS_WINDOWS
    hints.ai_family = PF_INET;
#else
    hints.ai_family = PF_UNSPEC;
#endif
    hints.ai_socktype = SOCK_STREAM;
    // addr must be numeric so this can't fail.
    if ((error = getaddrinfo(addr.c_str(), port.c_str(), &hints, &res0))) {
	XLOG_FATAL("getaddrinfo(%s,%s,...) failed: %s", addr.c_str(),
		   port.c_str(),
		   gai_strerror(error));
    }

    XLOG_ASSERT(res0->ai_addrlen <= sizeof(ss));
    memset(&ss, 0, sizeof(ss));
    memcpy(&ss, res0->ai_addr, res0->ai_addrlen);
    len = res0->ai_addrlen;

    freeaddrinfo(res0);
}	

/* **************** BGPSocket - PRIVATE METHODS *********************** */

/* **************** BGPSocketClient - PUBLIC METHODS *********************** */

SocketClient::SocketClient(const Iptuple& iptuple, EventLoop& e, bool md5sig)
    : Socket(iptuple, e), _md5sig(md5sig)
{
    debug_msg("SocketClient constructor called\n");
    _async_writer = 0;
    _async_reader = 0;
    _disconnecting = false;
    _connecting = false;
}

SocketClient::~SocketClient()
{
    async_remove();
    if( _connecting)
	connect_break();
}

void
SocketClient::disconnect()
{
    debug_msg("Disconnect\n");
    XLOG_ASSERT(get_sock().is_valid());

    /*
    ** If you see this assert then the disconnect code has been
    ** re-entered. The call graph is typically:
    **
    ** Socket::disconnect()
    ** Socket::async_remove()
    ** AsyncFileWriter::flush_buffers()
    ** 
    ** The call to flush_buffers() cause the completion methods to be
    ** run which in some cases call disconnect() again.
    ** The solution is to check with SocketClient::disconnecting()
    ** and just return if we are currently disconnecting. This is
    ** perfectly safe as the initial call to disconnect() should
    ** perform all the correct state transitions.
    **
    ** Look at "BGPPeer::send_notification_complete".
    */
    if (_disconnecting)
	return;
//     XLOG_ASSERT(!_disconnecting);

    _disconnecting = true;
    async_remove();
    close_socket();
    _disconnecting = false;
}

void
SocketClient::connect(ConnectCallback cb)
{
    debug_msg("SocketClient connecting to remote Peer %s\n",
	      get_remote_host());

    // Assert that socket doesn't already exist, as we are about to create it.
    // XXX: This paranoid assertion existed to catch socket recycling
    // issues on the Windows platform; commented out for now.
    //XLOG_ASSERT(!get_sock().is_valid());

    size_t len;
    create_socket(get_local_socket(len), COMM_SOCK_BLOCKING);

    // bindtodevice?
    debug_msg("SocketClient::connect, local_interface: %s", get_local_interface().c_str());
    if (get_local_interface().size()) {
	comm_set_bindtodevice(get_sock(), get_local_interface().c_str());
    }

    if (_md5sig)
 	comm_set_tcpmd5(get_sock(), _md5sig);

    return connect_socket(get_sock(), get_remote_addr(), get_remote_port(),
			  get_local_addr(), cb);
}

void
SocketClient::connect_break()
{
    connect_socket_break();
}

void
SocketClient::connected(XorpFd s)
{
#ifdef	DEBUG_PEERNAME
    sockaddr_storage ss;
    struct sockaddr *sin = reinterpret_cast<struct sockaddr *>(ss);
    socklen_t len = sizeof(ss);
    if (-1 == getpeername(s, sin, &len))
	XLOG_FATAL("getpeername failed: %s", strerror(errno));
    char hostname[1024];
    int error;
    if (error = getnameinfo(sin, len, hostname, sizeof(hostname), 0, 0, 0))
	XLOG_FATAL("getnameinfo failed: %s", gai_strerror(error));
    set_remote_host(hostname);
#endif

    debug_msg("Connected to remote Peer %s\n", get_remote_host());

    XLOG_ASSERT(!get_sock().is_valid());
    XLOG_ASSERT(!_connecting);
    // Clean up any old reader/writers we might have around.
    // This can happen if you kill network connection between two peers
    // for 10 seconds and then re-start it.
    async_remove();
    set_sock(s);
    async_add(s);
}

void
SocketClient::flush_transmit_queue() 
{
    if (_async_writer)
	_async_writer->flush_buffers();
}

void
SocketClient::async_read_start(size_t cnt, size_t offset)
{
    debug_msg("start reading %s\n", get_remote_host());

    XLOG_ASSERT(_async_reader);

    _async_reader->
	add_buffer_with_offset(_read_buf, 
			       cnt,
			       offset,
			       callback(this,
					&SocketClient::async_read_message));
    _async_reader->start();
}

/*
 * Handler for reading incoming data on a BGP connection.
 *
 * When a packet first comes in, we read the default amount of data,
 * which is BGPPacket::MINPACKETSIZE (i.e. the minimum size of a BGP message).
 * This callback is then invoked a first time, and it can check the
 * actual message length. If more bytes are needed, we call again
 * async_read_start with the desired length (instructing it to skip
 * whatever we already read).
 * Once the packet is complete, we invoke the packet decoder with dispatch()
 */
void
SocketClient::async_read_message(AsyncFileWriter::Event ev,
		const uint8_t *buf,	// the base of the buffer
		const size_t buf_bytes,	// desired message size
		const size_t offset)	// where we got so far (next free byte)
{
    debug_msg("async_read_message %d %u %u %s\n", ev,
	      XORP_UINT_CAST(buf_bytes), XORP_UINT_CAST(offset),
	      get_remote_host());

    XLOG_ASSERT(_async_reader);

    switch (ev) {
    case AsyncFileReader::DATA:
	XLOG_ASSERT(offset <= buf_bytes);
	if (offset == buf_bytes) {		// message complete so far
	    size_t fh_length = extract_16(buf + BGPPacket::LENGTH_OFFSET);

	    if (fh_length < BGPPacket::MINPACKETSIZE
		|| fh_length > sizeof(_read_buf)) {
		XLOG_ERROR("Illegal length value %u",
			   XORP_UINT_CAST(fh_length));
		if (!_callback->dispatch(BGPPacket::ILLEGAL_MESSAGE_LENGTH,
					 buf, buf_bytes, this))
		    return;
	    }
	    /*
	     * Keep reading until we have the whole message.
	     */
	    if (buf_bytes == fh_length) {
		if (_callback->dispatch(BGPPacket::GOOD_MESSAGE,
					buf, buf_bytes, this))
		    async_read_start();		// ready for next message
	    } else {				// read rest of the message
		async_read_start(fh_length, buf_bytes);
	    }
	}
	/*
	** At this point if we have a valid _async_reader then it should
	** have buffers into which we expect data.
	*/
	if (_async_reader && 0 == _async_reader->buffers_remaining())
	    XLOG_WARNING("No outstanding reads %s socket %p async_reader %p",
			 is_connected() ? "connected" : "not connected",
			 this, _async_reader);
	
	XLOG_ASSERT(!_async_reader ||
		    (_async_reader &&
		     _async_reader->buffers_remaining() > 0));
	break;

    case AsyncFileReader::WOULDBLOCK:
    case AsyncFileReader::FLUSHING:
	/*
	 * We are not using a dynamic buffer so don't worry.
	 */
	break;

    case AsyncFileReader::OS_ERROR:
	debug_msg("Read failed: %d\n", _async_reader->error());
	_callback->dispatch(BGPPacket::CONNECTION_CLOSED, 0, 0, this);
	break;

    case AsyncFileReader::END_OF_FILE:
	debug_msg("End of file\n");
	_callback->dispatch(BGPPacket::CONNECTION_CLOSED, 0, 0, this);
	break;
    }
}

void
SocketClient::send_message_complete(AsyncFileWriter::Event ev,
				   const uint8_t *buf,
				   const size_t buf_bytes,
				   const size_t offset,
				   SendCompleteCallback cb)
{
    debug_msg("event %d %s\n", ev, get_remote_host());

    switch (ev) {
    case AsyncFileWriter::DATA:
	if (offset == buf_bytes) {
	    debug_msg("Message sent\n");
	    cb->dispatch(SocketClient::DATA, buf);
	}
	XLOG_ASSERT(offset <= buf_bytes);
	break;
    case AsyncFileWriter::FLUSHING:
	cb->dispatch(SocketClient::FLUSHING, buf);
	break;
    case AsyncFileWriter::OS_ERROR:
	//XXXX do something
	//probably need to do some more error handling here
	cb->dispatch(SocketClient::ERROR, buf);
	break;
    case AsyncFileWriter::WOULDBLOCK:
    case AsyncFileWriter::END_OF_FILE:
	// Can't possibly happen.
	break;
    }
}

bool
SocketClient::send_message(const uint8_t* buf,
			   const size_t	cnt,
			   SendCompleteCallback cb)
{
    debug_msg("peer %s bytes = %u\n", get_remote_host(),
	      XORP_UINT_CAST(cnt));

    if(!is_connected()) {
#ifdef	DEBUG_LOGGING
	XLOG_WARNING("sending message to %s, not connected!!!",
		   get_remote_host());
#else
	XLOG_WARNING("sending message to %s, not connected!!!",
		     get_remote_addr().c_str());
#endif
	return false;
    }

    XLOG_ASSERT(_async_writer);

    _async_writer->add_buffer(buf, cnt,
			      callback(this,
				       &SocketClient::send_message_complete,
				       cb));
    _async_writer->start();

    return true;
}

bool 
SocketClient::output_queue_busy() const 
{
    //20 is a fairly arbitrary soft limit on how many buffers we want
    //in the output queue before we start to push back
    XLOG_ASSERT(_async_writer);

    if (_async_writer->buffers_remaining() > 20)
	return true;
    else
	return false;
}

int
SocketClient::output_queue_size() const 
{
    XLOG_ASSERT(_async_writer);

    return _async_writer->buffers_remaining();
}

/* **************** BGPSocketClient - PROTECTED METHODS *********************** */

/* **************** BGPSocketClient - PRIVATE METHODS *********************** */

void
SocketClient::connect_socket(XorpFd sock, string raddr, uint16_t port,
			     string laddr, ConnectCallback cb)
{
    debug_msg("raddt %s port %d laddr %s\n", raddr.c_str(), port,
	      laddr.c_str());

    size_t len;
    const struct sockaddr *local = get_bind_socket(len);

    /* Bind the local endpoint to the supplied address */
    if (XORP_ERROR == comm_sock_bind(sock, local, get_local_interface().c_str())) {

	/*
	** This endpoint is now screwed so shut it.
	*/
	close_socket();

	cb->dispatch(false);

	return;
    }

    debug_msg("SocketClient::connect_socket, local_interface: %s", get_local_interface().c_str());

    const struct sockaddr *servername = get_remote_socket(len);

    if (!eventloop().
	add_ioevent_cb(sock,
		    IOT_CONNECT,
		     callback(this,
			      &SocketClient::connect_socket_complete,
			      cb))) {
	XLOG_ERROR("Failed to add socket %s to eventloop", sock.str().c_str());
    }

    const int blocking = 0;

    if (XORP_ERROR == comm_sock_set_blocking(sock, blocking))
	XLOG_FATAL("Failed to go non-blocking");

    // Given the file descriptor is now non-blocking we would expect a
    // in progress error.

    XLOG_ASSERT(!_connecting);
    _connecting = true;
    int in_progress = 0;
    if (XORP_ERROR == comm_sock_connect(sock, servername, blocking,
					&in_progress)) {
	if (in_progress) {
	    debug_msg("connect failed in progress %s\n",
		      in_progress ? "yes" : "no");
	    return;
	}
    }

    // 1) If an error apart from in_progress occured call the completion
    // method which will tidy up.
    //
    // 2) A connection may have been made already, this happens in the
    // loopback case, again the completion method should deal with this.

    connect_socket_complete(sock, IOT_CONNECT, cb);
}

void
SocketClient::connect_socket_complete(XorpFd sock, IoEventType type,
				      ConnectCallback cb)
{
    int soerror;
    int is_connected = 0;
    socklen_t len = sizeof(soerror);

    debug_msg("connect socket complete %s %d\n", sock.str().c_str(), type);

    UNUSED(type);

    XLOG_ASSERT(_connecting);
    _connecting = false;

    XLOG_ASSERT(get_sock() == sock);

    eventloop().remove_ioevent_cb(sock);

    // Did the connection succeed?
    if (comm_sock_is_connected(sock, &is_connected) != XORP_OK) {
	debug_msg("connect failed (comm_sock_is_connected: %s) %s\n",
		  comm_get_last_error_str(), sock.str().c_str());
	goto failed;
    }
    if (is_connected == 0) {
	debug_msg("connect failed (comm_sock_is_connected: false) %s\n",
		  sock.str().c_str());
	goto failed;
    }

    // Check for a pending socket error if one exists.
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, XORP_SOCKOPT_CAST(&soerror),
		   &len) != 0) {
	debug_msg("connect failed (getsockopt) %s\n", sock.str().c_str());
	goto failed;
    }

    debug_msg("connect suceeded %s\n", sock.str().c_str());

    // Clean up any old reader/writers we might have around.
    // Not sure exactly how this one was triggered.
    async_remove();
    async_add(sock);
    cb->dispatch(true);
    return;

 failed:
//  	XLOG_ERROR("Connect failed: raddr %s port %d",
// 		   inet_ntoa(get_remote_addr()), ntohs(get_remote_port()));

	close_socket();
	cb->dispatch(false);
}

void
SocketClient::connect_socket_break()
{
    _connecting = false;

    eventloop().remove_ioevent_cb(get_sock());
    close_socket();
}

void 
SocketClient::async_add(XorpFd sock)
{
    if (XORP_ERROR == comm_sock_set_blocking(sock, COMM_SOCK_NONBLOCKING))
	XLOG_FATAL("Failed to go non-blocking");

    XLOG_ASSERT(0 == _async_writer);
    _async_writer = new AsyncFileWriter(eventloop(), sock);

    XLOG_ASSERT(0 == _async_reader);
    //
    // XXX: We use lowest priority for the file reader so that we don't
    // generally receive data in faster than we can send it back out.
    // Also, the priority is lower than the tasks' background priority
    // to avoid being overloaded by high volume data from the peers.
    //
    _async_reader = new AsyncFileReader(eventloop(), sock,
					XorpTask::PRIORITY_BACKGROUND);

    async_read_start();
}

void 
SocketClient::async_remove()
{
    if(_async_writer) {
	_async_writer->stop();
	_async_writer->flush_buffers();
	delete _async_writer;
	_async_writer = 0;
    }
    async_remove_reader();
}

void 
SocketClient::async_remove_reader()
{
    if (_async_reader) {
	_async_reader->stop();
	_async_reader->flush_buffers();
	delete _async_reader;
	_async_reader = 0;
    }
}

bool 
SocketClient::is_connected()
{
    if (_connecting)
	return false;

    return get_sock().is_valid();
}	

bool
SocketClient::still_reading()
{
    return _async_reader;
}

/* **************** BGPSocketServer *********************** */

SocketServer::SocketServer(const Iptuple& iptuple, EventLoop& e) :
    Socket(iptuple, e)
{
    debug_msg("SocketServer constructor called\n");	
}
