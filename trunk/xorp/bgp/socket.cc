// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/bgp/socket.cc,v 1.18 2004/12/05 16:14:35 atanu Exp $"

// #define DEBUG_LOGGING 
// #define DEBUG_PRINT_FUNCTION_NAME 

#include <fcntl.h>

#include "bgp_module.h"

#include "config.h"
#include "libxorp/debug.h"
#include "libxorp/xorp.h"
#include "libxorp/xlog.h"
#include "libxorp/exceptions.hh"
#include "libxorp/callback.hh"
#include "libcomm/comm_api.h"

#include "socket.hh"
#include "packet.hh"


/* **************** BGPSocket - PUBLIC METHODS *********************** */

Socket::Socket(const Iptuple& iptuple, EventLoop& e)
    : _iptuple(iptuple), _eventloop(e)
{
    debug_msg("Socket constructor called: %s\n", _iptuple.str().c_str());

    //    _eventloop = 0;

    _s = UNCONNECTED;

#ifdef	DEBUG_PEERNAME
    _remote_host = "unconnected socket";
#endif
}

void
Socket::create_listener()
{
    debug_msg("create_listener called");

    size_t len;
    const struct sockaddr *sin = get_local_socket(len);

    create_socket(sin);

    int opt = 1;
    if(-1 == setsockopt(get_sock(),
			SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
	XLOG_FATAL("setsockopt failed: %s", strerror(errno));
    
    if(-1 == bind(get_sock(), sin, len))
	XLOG_FATAL("Bind failed: %s", strerror(errno));
}

/* **************** BGPSocket - PROTECTED METHODS *********************** */

void
Socket::close_socket()
{
    debug_msg("Close socket %s %d\n", get_remote_host(), _s);

    ::close(_s);
    _s = UNCONNECTED;
}

void
Socket::create_socket(const struct sockaddr *sin)
{
    debug_msg("create_socket called\n");

    XLOG_ASSERT(UNCONNECTED == _s);

    if(UNCONNECTED == (_s = ::socket (sin->sa_family, SOCK_STREAM, 0)))
	XLOG_FATAL("Socket call failed: %s", strerror(errno));

    debug_msg("BGPSocket socket created (sock - %d)\n", _s);
}

void
Socket::init_sockaddr(string addr, uint16_t local_port,
		      struct sockaddr *sin, size_t& len)
{
    debug_msg("addr %s port %d len = %u\n", addr.c_str(), local_port, len);

    string port = c_format("%d", local_port);

    int error;
    struct addrinfo hints, *res0;
    // Need to provide a hint because we are providing a numeric port number.
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = PF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    // addr must be numeric so this can't fail.
    if ((error = getaddrinfo(addr.c_str(), port.c_str(), &hints, &res0)))
	XLOG_FATAL("getaddrinfo(%s,%s,...) failed: %s", addr.c_str(),
		   port.c_str(),
		   gai_strerror(error));

    XLOG_ASSERT(res0->ai_addrlen <= len);
    memcpy(sin,res0->ai_addr, res0->ai_addrlen);

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
    XLOG_ASSERT(UNCONNECTED != get_sock());

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
    XLOG_ASSERT(!_disconnecting);

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

    XLOG_ASSERT(UNCONNECTED == get_sock());

    size_t len;
    create_socket(get_local_socket(len));

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
SocketClient::connected(int s)
{
#ifdef	DEBUG_PEERNAME
    char socket_buffer[SOCKET_BUFFER_SIZE];
    struct sockaddr *sin = reinterpret_cast<struct sockaddr *>(socket_buffer);
    socklen_t len = sizeof(socket_buffer);
    if (-1 == getpeername(s, sin, &len))
	XLOG_FATAL("getpeername failed: %s", strerror(errno));
    char hostname[1024];
    if (getnameinfo(sin, len, hostname, sizeof(hostname), 0, 0, 0))
	XLOG_FATAL("getnameinfo failed: %s", strerror(errno));
    set_remote_host(hostname);
#endif

    debug_msg("Connected to remote Peer %s\n", get_remote_host());

    XLOG_ASSERT(UNCONNECTED == get_sock());
    XLOG_ASSERT(!_connecting);
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
 * which is MINPACKETSIZE (i.e. the minimum size of a BGP message).
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
    debug_msg("async_read_message %d %u %u %s\n", ev, (uint32_t)buf_bytes,
	      (uint32_t)offset, get_remote_host());

    XLOG_ASSERT(_async_reader);

    switch (ev) {
    case AsyncFileReader::DATA:
	XLOG_ASSERT(offset <= buf_bytes);
	if (offset == buf_bytes) {		// message complete so far
	    const fixed_header *header =
		reinterpret_cast<const struct fixed_header *>(buf);
	    size_t fh_length = ntohs(header->length);

	    if (fh_length < MINPACKETSIZE || fh_length > sizeof(_read_buf)) {
		XLOG_ERROR("Illegal length value %u", (uint32_t)fh_length);
		if (!_callback->dispatch(BGPPacket::ILLEGAL_MESSAGE_LENGTH,
					buf, buf_bytes))
		    return;
	    }
	    /*
	     * Keep reading until we have the whole message.
	     */
	    if (buf_bytes == fh_length) {
		if (_callback->dispatch(BGPPacket::GOOD_MESSAGE,
				       buf, buf_bytes))
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
	    XLOG_WARNING("No outstanding reads");
	
	XLOG_ASSERT(!_async_reader ||
		    (_async_reader && _async_reader->buffers_remaining() > 0));
	break;

    case AsyncFileReader::FLUSHING:
	/*
	 * We are not using a dynamic buffer so don't worry.
	 */
	break;

    case AsyncFileReader::ERROR_CHECK_ERRNO:
	debug_msg("Read failed: %s\n", strerror(errno));
	_callback->dispatch(BGPPacket::CONNECTION_CLOSED, 0, 0);
	break;

    case AsyncFileReader::END_OF_FILE:
	debug_msg("End of file\n");
	_callback->dispatch(BGPPacket::CONNECTION_CLOSED, 0, 0);
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
    case AsyncFileWriter::ERROR_CHECK_ERRNO:
	//XXXX do something
	//probably need to do some more error handling here
	cb->dispatch(SocketClient::ERROR, buf);
	break;
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
    debug_msg("peer %s bytes = %u\n", get_remote_host(), (uint32_t)cnt);

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
SocketClient::output_queue_size() const {
    XLOG_ASSERT(_async_writer);

    return _async_writer->buffers_remaining();
}

/* **************** BGPSocketClient - PROTECTED METHODS *********************** */

/* **************** BGPSocketClient - PRIVATE METHODS *********************** */

void
SocketClient::connect_socket(int sock, string raddr, uint16_t port,
			     string laddr, ConnectCallback cb)
{
    debug_msg("laddr %s\n", laddr.c_str());

    size_t len;
    const struct sockaddr *local = get_bind_socket(len);

    /* Bind the local endpoint to the supplied address */
    if(-1 == ::bind(sock, local, len)) {
	XLOG_ERROR("Bind failed: %s", strerror(errno));

	/*
	** This endpoint is now screwed so shut it.
	*/
	close_socket();

	cb->dispatch(false);

	return;
    }

    const struct sockaddr *servername = get_remote_socket(len);

    if (!eventloop().
	add_selector(sock,
		    SEL_ALL,
		     callback(this,
			      &SocketClient::connect_socket_complete,
			      cb))) {
	XLOG_ERROR("Failed to add socket %d to eventloop", sock);
    }

    if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0) {
        XLOG_FATAL("Failed to go non-blocking: %s", strerror(errno));
    }

    // Given the file descriptor is now non-blocking we would expect a
    // in progress error.

    XLOG_ASSERT(!_connecting);
    _connecting = true;
    if (-1 == ::connect(sock, servername, len)) {
	if (EINPROGRESS == errno)
	    return;

	debug_msg("Connect failed: %s raddr %s port %d\n",
		  strerror(errno), raddr.c_str(), port);
//  	XLOG_ERROR("Connect failed: %s raddr %s port %d",
// 		   strerror(errno), inet_ntoa(raddr), ntohs(port));
	// If the connect really failed still drop into complete code
	// to tidy up. The completion code should be able to detect if
	// there is an error.
    }

    // Its possible that the connection completed immediately, this
    // happens in the loopback case.
    connect_socket_complete(sock, SEL_ALL, cb);
}

void
SocketClient::connect_socket_complete(int sock, SelectorMask m,
				      ConnectCallback cb)
{
    debug_msg("connect socket complete %d %d\n", sock, m);

    UNUSED(m);

    XLOG_ASSERT(_connecting);
    _connecting = false;

    XLOG_ASSERT(get_sock() == sock);

    eventloop().remove_selector(sock);

    char socket_buffer[SOCKET_BUFFER_SIZE];
    struct sockaddr *sin = reinterpret_cast<struct sockaddr *>(socket_buffer);
    socklen_t len = sizeof(socket_buffer);
    int error;

    /*
    ** Try a number of different methods to discover if the connection
    ** has succeeded.
    */

    // Did the connection succeed?
    if (read(sock, 0, 0) != 0) {
	debug_msg("connect failed (read) %d\n", sock);
	goto failed;
    }

    // Did the connection succeed?
    memset(sin, 0, len);
    len = sizeof(sin);
    if (-1 == getpeername(sock, sin, &len)) {
	debug_msg("connect failed (geetpeername) %d\n", sock);
	goto failed;
    }


#if	0
    // If we need to check the result of getpeername.
    char hostname[1024];
    if (getnameinfo(sin, len, hostname, sizeof(hostname), 0, 0,
		    NI_NUMERICHOST)) {
	debug_msg("getnameinfo failed: %s\n", strerror(errno)); 
	goto failed;
    }
#endif    
#if	0
    if (0 == sin.sin_port) {
	debug_msg("connect failed (sin_port) %d\n", sock);
	goto failed;
    }

    if (0 == sin.sin_addr.s_addr) {
	debug_msg("connect failed (sin_addr) %d\n", sock);
	goto failed;
    }
#endif

    // Did the connection succeed?
    len = sizeof(error);
    if (getsockopt(sock, SOL_SOCKET, SO_ERROR, &error, &len) < 0) {
	debug_msg("connect failed (getsockopt) %d\n", sock);
	goto failed;
    }

    debug_msg("connect suceeded %d\n", sock);

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
    XLOG_ASSERT(_connecting);
    _connecting = false;

    eventloop().remove_selector(get_sock());
    close_socket();
}

void 
SocketClient::async_add(int sock)
{
    if (fcntl(sock, F_SETFL, O_NONBLOCK) < 0) {
        XLOG_FATAL("Failed to go non-blocking: %s", strerror(errno));
    }

    XLOG_ASSERT(0 == _async_writer);
    _async_writer = new AsyncFileWriter(eventloop(), sock);

    XLOG_ASSERT(0 == _async_reader);
    _async_reader = new AsyncFileReader(eventloop(), sock);

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
	return UNCONNECTED;

    return get_sock() != UNCONNECTED;
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

void
SocketServer::listen()
{
    if(::listen(get_sock(), MAX_LISTEN_QUEUE) == -1) {
	debug_msg("listen failed\n");
	return ;
    }
}
