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

// $XORP: xorp/bgp/socket.hh,v 1.5 2003/12/11 03:04:36 atanu Exp $

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
    Socket(const Iptuple& iptuple, EventLoop& e);

    static void init_sockaddr(struct sockaddr_in *name, struct in_addr addr,
			      uint16_t port);

    //    void set_eventloop(EventLoop *evt) {_eventloop = evt;}
    EventLoop& eventloop() {return _eventloop;}

    int get_sock() { return _s;}

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

    EventLoop& _eventloop;

    /*
    ** Remote host. For debugging only
    */
    string _remote_host;
};

class SocketClient : public Socket {
public:
    /**
     * @param iptuple specification of the connection endpoints.
     */
    SocketClient(const Iptuple& iptuple, EventLoop& e);
    ~SocketClient();

    /**
     * Callback for connection attempts.
     */
    typedef XorpCallback1<void, bool>::RefPtr ConnectCallback;

    /**
     * Asynchronously connect.
     *
     * @param cb is called when connection suceeds or fails.
     */
    void connect(ConnectCallback cb);

    /**
     * Break asynchronous connect attempt.
     *
     * Each instance of the this class has a single connection
     * associated with it. If while we are attemping to connect to a
     * peer (using connect()), the peer connects to us. It is
     * necessary to stop the outgoing connect.
     */
    void connect_break();

    /**
     * The peer has initiated the connection so form an association.
     *
     * @param s incoming socket file descriptor
     */
    void connected(int s);

    /**
     * Throw away all the data that is queued to be sent on this socket.
     */
    void flush_transmit_queue();

    /**
     * Stop reading data on this socket.
     */
    void stop_reader() {async_remove_reader();}

    /**
     * Disconnect this socket.
     */
    void disconnect();

    /**
     * @return Are we currrent disconecting.
     */
    bool disconnecting() {return _disconnecting;}

    /**
     * Callback for incoming data.
     *
     * @param const uint8_t* pointer to data.
     * @param size_t length of data.
     */
    typedef XorpCallback3<bool,BGPPacket::Status,const uint8_t*,size_t>::RefPtr
    MessageCallback;

    /**
     * Set the callback for incoming data.
     */
    void set_callback(const MessageCallback& cb) {_callback = cb;}

    enum Event {
	DATA = AsyncFileWriter::DATA,
	FLUSHING = AsyncFileWriter::FLUSHING,
	ERROR = AsyncFileWriter::ERROR_CHECK_ERRNO
    };

    /**
     * Callback for data transmission.
     *
     * @param Event status of the send.
     * @param const uint8_t* pointer to the transmitted data. Allows
     * caller to free the data.
     */
    typedef XorpCallback2<void,Event,const uint8_t*>::RefPtr
    SendCompleteCallback;
			      
    /**
     * Asynchronously Send a message.
     *
     * @param buf pointer to data buffer.
     * @param cnt length of data buffer.
     * @param cb notification of success or failure.
     *
     * @return true if the message is accepted.
     *
     */
    bool send_message(const uint8_t* buf,
		      size_t cnt, 
		      SendCompleteCallback cb);

    /**
     * Flow control signal. 
     *
     * Data will be queued for transmission until a resource such as
     * memory is exceeded. A correctly written client should stop
     * sending messages if the output queue is not draining.
     *
     * @return true if its time to stop sending messages.
     */
    bool output_queue_busy() const;

    /**
     * @return number of messages in the output queue.
     */
    int output_queue_size() const;

    /**
     * @return true if a session exists.
     */
    bool is_connected();

    /**
     * @return true if we are still reading.
     */
    bool still_reading();
protected:
private:
    void connect_socket(int sock, struct in_addr raddr, uint16_t port,
			struct in_addr laddr, ConnectCallback cb);
    void connect_socket_complete(int fd, SelectorMask m, ConnectCallback cb);
    void connect_socket_break();

    void async_add(int sock);
    void async_remove();
    void async_remove_reader();
    
    void read_from_server(int sock);
    void write_to_server(int sock);

    void send_message_complete(AsyncFileWriter::Event e,
			      const uint8_t* buf,
			      const size_t buf_bytes,
			      const size_t offset,
			      SendCompleteCallback cb);

    void async_read_start(size_t cnt = BGP_COMMON_HEADER_LEN,size_t ofset = 0);
    void async_read_message(AsyncFileWriter::Event ev,
			   const uint8_t *buf,
			   const size_t buf_bytes,
			   const size_t offset);

    MessageCallback _callback;
    AsyncFileWriter *_async_writer;
    AsyncFileReader *_async_reader;

    bool _disconnecting;
    bool _connecting;

    uint8_t _read_buf[MAXPACKETSIZE]; // Maximum allowed BGP message
};

class SocketServer : public Socket {
public:
    SocketServer(EventLoop& e);
    SocketServer(const Iptuple& iptuple, EventLoop& e);
    void listen();
};

#endif // __BGP_SOCKET_HH__
