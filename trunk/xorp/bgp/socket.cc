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

#ident "$XORP: xorp/bgp/socket.cc,v 1.3 2003/01/21 18:54:27 rizzo Exp $"

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

#include "socket.hh"
#include "packet.hh"


/* **************** BGPSocket - PUBLIC METHODS *********************** */

Socket::Socket(const Iptuple& iptuple)
    : _iptuple(iptuple)
{
    debug_msg("Socket constructor called: %s\n", _iptuple.str().c_str());

    _eventloop = 0;

    _s = UNCONNECTED;

#ifdef	DEBUG_PEERNAME
    _remote_host = "unconnected socket";
#endif
}

Socket::Socket(int s) : _s(s)
{
    _eventloop = 0;
}

void Socket::create_listener()
{
    struct sockaddr_in servername;
    debug_msg("create_listener called");
    create_socket();
    init_sockaddr(&servername, get_local_addr(), get_local_port());

    int opt = 1;
    if(-1 == setsockopt(get_sock(),
			SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
	XLOG_FATAL("setsockopt failed: %s", strerror(errno));
    
    if(-1 == bind(get_sock(), (struct sockaddr *)&servername,
		  sizeof(servername)))
	XLOG_FATAL("Bind failed: %s", strerror(errno));
}

/* **************** BGPSocket - PROTECTED METHODS *********************** */

void Socket::close_socket()
{
    debug_msg("Close socket %s %d\n", get_remote_host(), _s);

    ::close(_s);
    _s = UNCONNECTED;
}

void Socket::create_socket()
{
    debug_msg("create_socket called\n");

    XLOG_ASSERT(UNCONNECTED == _s);

    if(UNCONNECTED == (_s = ::socket (PF_INET, SOCK_STREAM, 0)))
	XLOG_FATAL("Socket call failed: %s", strerror(errno));

    debug_msg("BGPSocket socket created (sock - %d)\n", _s);
}

void Socket::init_sockaddr(struct sockaddr_in *name, 
			       struct in_addr addr,
			       uint16_t port)
{
    debug_msg("addr %s port %d len = %u\n", inet_ntoa(addr), ntohs(port),
	      (uint32_t)sizeof(*name));

    memset(name, 0, sizeof(*name));
#ifdef	HAVE_SIN_LEN
    name->sin_len = sizeof(*name);
#endif
    name->sin_family = AF_INET;
    name->sin_addr = addr;
    name->sin_port = port;
}	

/* **************** BGPSocket - PRIVATE METHODS *********************** */

/* **************** BGPSocketClient - PUBLIC METHODS *********************** */

SocketClient::SocketClient(const Iptuple& iptuple)
    : Socket(iptuple)
{
    debug_msg("SocketClient constructor called\n");
    _async_writer = 0;
    _async_reader = 0;
    _disconnecting = false;
}

SocketClient::SocketClient(int s) : Socket(s)
{
    _async_writer = 0;
    _async_reader = 0;
    _disconnecting = false;
}

SocketClient::~SocketClient()
{
    async_remove();
}

void SocketClient::disconnect()
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

bool SocketClient::connect()
{
    debug_msg("SocketClient connecting to remote Peer %s\n",
	      get_remote_host());

    XLOG_ASSERT(UNCONNECTED == get_sock());

    create_socket();
    return connect_socket(get_sock(), get_remote_addr(), get_remote_port(),
			  get_local_addr());
}

void SocketClient::connected(int s)
{
#ifdef	DEBUG_PEERNAME
    struct sockaddr_in sin;
    socklen_t len = sizeof(sin);
    if(1 == getpeername(s, reinterpret_cast<struct sockaddr *>(&sin), &len))
	XLOG_FATAL("getpeername failed: %s", strerror(errno));
    set_remote_host(inet_ntoa(sin.sin_addr));
	
#endif
    debug_msg("Connected to remote Peer %s\n", get_remote_host());

    XLOG_ASSERT(UNCONNECTED == get_sock());
    set_sock(s);
    async_add(s);
}

void SocketClient::flush_transmit_queue() 
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
SocketClient::send_message(const uint8_t*	    buf,
			       const size_t	    cnt,
			       SendCompleteCallback cb)
{
    debug_msg("peer %s bytes = %u\n", get_remote_host(), (uint32_t)cnt);

    if(!is_connected()) {
#ifdef	DEBUG_LOGGING
	XLOG_WARNING("sending message to %s, not connected!!!",
		   get_remote_host());
#else
	XLOG_WARNING("sending message to %s, not connected!!!",
		     inet_ntoa(get_remote_addr()));
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
SocketClient::output_queue_busy() const {
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

bool
SocketClient::connect_socket(int sock, struct in_addr raddr, uint16_t port,
			     struct in_addr laddr)
{
    struct sockaddr_in local;
    debug_msg("laddr %s\n", inet_ntoa(laddr));
    init_sockaddr(&local, laddr, 0);

    /* Bind the local endpoint to the supplied address */
    if(-1 == ::bind(sock, reinterpret_cast<struct sockaddr *>(&local),
		   sizeof(local))) {
	XLOG_ERROR("Bind failed: %s", strerror(errno));

	/*
	** This endpoint is now screwed so shut it.
	*/
	close_socket();

	return false;
    }

    struct sockaddr_in servername;
    debug_msg("raddr %s port %d\n", inet_ntoa(raddr), ntohs(port));
    init_sockaddr(&servername, raddr, port);

    if(-1 == ::connect(sock, reinterpret_cast<struct sockaddr *>(&servername), 
		      sizeof(servername))) {

	XLOG_ERROR("Connect failed: %s", strerror(errno));

	/*
	** This endpoint is now screwed so shut it.
	*/
	close_socket();

	return false;
    }
    debug_msg("socket connected (fd %d)\n", sock);
    async_add(sock);

    return true;
}

void 
SocketClient::async_add(int sock)
{
    if(fcntl(sock, F_SETFL, O_NONBLOCK) < 0) {
        XLOG_FATAL("Failed to go non-blocking: %s", strerror(errno));
    }

    XLOG_ASSERT(0 == _async_writer);
    _async_writer = new AsyncFileWriter(*get_eventloop(), sock);

    XLOG_ASSERT(0 == _async_reader);
    _async_reader = new AsyncFileReader(*get_eventloop(), sock);

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
    if(_async_reader) {
	_async_reader->stop();
	_async_reader->flush_buffers();
	delete _async_reader;
	_async_reader = 0;
    }
}

bool SocketClient::valid_marker(uint8_t* m)
{
    m = 0; //temp assignment
    return true;
}

bool SocketClient::valid_length(uint16_t l)
{
    l = 0;//temp assignment
    return true;
}

bool SocketClient::valid_type(uint8_t t)
{
    t = 0;//temp assignment
    return true;
}

bool SocketClient::is_connected()
{
    return get_sock() != UNCONNECTED;
}	

bool SocketClient::still_reading()
{
    return _async_reader;
}

/* **************** BGPSocketServer *********************** */

SocketServer::SocketServer(const Iptuple& iptuple) :
    Socket(iptuple)
{
    debug_msg("SocketServer constructor called\n");	
}

void SocketServer::listen()
{
    if(::listen(get_sock(), MAX_LISTEN_QUEUE) == -1) {
	debug_msg("listen failed\n");
	return ;
    }
}
