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

// $XORP: xorp/bgp/socket.hh,v 1.39 2002/12/09 18:28:49 hodson Exp $

#ifndef __BGP_SOCKET_HH__
#define __BGP_SOCKET_HH__

// #define DEBUG_PEERNAME

#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netdb.h>
#include <signal.h>
#include "libxorp/debug.h"
#include "libxorp/exceptions.hh"
#include "libxorp/selector.hh"
#include "libxorp/asyncio.hh"
#include "libxorp/callback.hh"
#include "packet.hh"
#include "iptuple.hh"

/* **************** BGPSocket *********************** */

#define MAX_LISTEN_QUEUE 5

class Socket {
public:
    Socket(int s);// For testing
    Socket(const Iptuple& iptuple);

    static void init_sockaddr(struct sockaddr_in *name, struct in_addr addr,
			      uint16_t port);

    void set_eventloop(EventLoop *evt) {_eventloop = evt;}
    EventLoop *get_eventloop() {return _eventloop;}

    int get_sock() { return _s;}
#ifdef	DEPRECATED
    bool compare_remote_addr(struct in_addr addr) {
	return addr.s_addr == get_remote_addr().s_addr;
    }
#endif

    void create_listener();

    /* Intended for debugging use only */
    void set_remote_host(const char *s) {_remote_host = s;}
    const char  *get_remote_host() {return _remote_host.c_str();}

protected:
    static const int UNCONNECTED = -1;

    void set_sock(int s) {_s = s;}

    void close_socket();
    void create_socket();

    struct in_addr get_local_addr() {return _iptuple.get_local_addr();}
    uint16_t get_local_port() {return _iptuple.get_local_port();}

    struct in_addr get_remote_addr() {return _iptuple.get_peer_addr();}
    uint16_t get_remote_port() {return _iptuple.get_peer_port();}
private:
    int _s;

    /*
    ** All in network byte order.
    */
    const Iptuple _iptuple;

    EventLoop *_eventloop;

    /*
    ** Remote host. For debugging only
    */
    string _remote_host;
};

class SocketClient : public Socket {
public:
    SocketClient(const Iptuple& iptuple);
    SocketClient(int s);
    ~SocketClient();

    bool connect();
    void connected(int s);
    void flush_transmit_queue();
    void stop_reader() {async_remove_reader();}
    void disconnect();
    bool disconnecting() {return _disconnecting;}

    typedef XorpCallback3<bool,BGPPacket::Status,const uint8_t*,size_t>::RefPtr
    MessageCallback;
    void set_callback(const MessageCallback& cb) {_callback = cb;}

    enum Event {
	DATA = AsyncFileWriter::DATA,
	FLUSHING = AsyncFileWriter::FLUSHING,
	ERROR = AsyncFileWriter::ERROR_CHECK_ERRNO
    };
    typedef XorpCallback2<void,Event,const uint8_t*>::RefPtr
    SendCompleteCallback;

    void send_message_complete(AsyncFileWriter::Event e,
			      const uint8_t* buf,
			      const size_t buf_bytes,
			      const size_t offset,
			      SendCompleteCallback cb);
			      
    bool send_message(const uint8_t*	  buf,
		     size_t		  cnt, 
		     SendCompleteCallback cb);

    bool output_queue_busy() const;
    int output_queue_size() const;

    void async_read_start(size_t cnt = BGP_COMMON_HEADER_LEN,size_t ofset = 0);
    void async_read_message(AsyncFileWriter::Event ev,
			   const uint8_t *buf,
			   const size_t buf_bytes,
			   const size_t offset);

    bool is_connected();
    bool still_reading();
protected:
private:
    bool connect_socket(int sock, struct in_addr raddr, uint16_t port,
			struct in_addr laddr);
    void async_add(int sock);
    void async_remove();
    void async_remove_reader();
    
    void read_from_server(int sock);
    void write_to_server(int sock);
    bool valid_marker(uint8_t* marker);
    bool valid_length(uint16_t length);
    bool valid_type(uint8_t type);

    MessageCallback _callback;
    AsyncFileWriter *_async_writer;
    AsyncFileReader *_async_reader;

    bool _disconnecting;

    uint8_t _read_buf[MAXPACKETSIZE]; // Maximum allowed BGP message
};

class SocketServer : public Socket {
public:
    SocketServer();
    SocketServer(const Iptuple& iptuple);
    void listen();
};

#endif // __BGP_SOCKET_HH__
