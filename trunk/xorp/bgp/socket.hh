// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/bgp/socket.hh,v 1.22 2008/01/04 03:15:27 pavlin Exp $

#ifndef __BGP_SOCKET_HH__
#define __BGP_SOCKET_HH__

// #define DEBUG_PEERNAME

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/exceptions.hh"
#include "libxorp/xorpfd.hh"
#include "libxorp/eventloop.hh"
#include "libxorp/asyncio.hh"
#include "libxorp/callback.hh"

#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif
#ifdef HAVE_SYS_UN_H
#include <sys/un.h>
#endif

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif

#include "packet.hh"
#include "iptuple.hh"


/* **************** BGPSocket *********************** */

class Socket {
public:
    static const int MAX_LISTEN_QUEUE = 5;

    Socket(const Iptuple& iptuple, EventLoop& e);

    /**
     * Given an address (IPv4 or IPv6) symbolic or numeric fill in the
     * provided structure.
     *
     * Note: This method is provided as a service to other code and is
     * no longer used by this class. Don't remove it as the test code
     * uses it.
     */
    static void init_sockaddr(string addr, uint16_t local_port,
			      struct sockaddr_storage& ss, size_t& len);

    //    void set_eventloop(EventLoop *evt) {_eventloop = evt;}
    EventLoop& eventloop() {return _eventloop;}

    XorpFd get_sock() { return _s; }

    void create_listener();

    /* Intended for debugging use only */
    void set_remote_host(const char *s) {_remote_host = s;}
    const char  *get_remote_host() {return _remote_host.c_str();}

protected:
    void set_sock(XorpFd s) { _s = s; }

    void close_socket();
    void create_socket(const struct sockaddr *sin, int is_blocking);

    const struct sockaddr *get_local_socket(size_t& len) {
	return _iptuple.get_local_socket(len);
    }
    string get_local_addr() {return _iptuple.get_local_addr();}
    uint16_t get_local_port() {return _iptuple.get_local_port();}

    const struct sockaddr *get_bind_socket(size_t& len) {
	return _iptuple.get_bind_socket(len);
    }

    const struct sockaddr *get_remote_socket(size_t& len) {
	return _iptuple.get_peer_socket(len);
    }
    string get_remote_addr() {return _iptuple.get_peer_addr();}
    uint16_t get_remote_port() {return _iptuple.get_peer_port();}
private:
    XorpFd _s;

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
    SocketClient(const Iptuple& iptuple, EventLoop& e, bool md5sig = false);
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
    void connected(XorpFd s);

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
    typedef XorpCallback4<bool,BGPPacket::Status,const uint8_t*,size_t,
			  SocketClient*>::RefPtr MessageCallback;

    /**
     * Set the callback for incoming data.
     */
    void set_callback(const MessageCallback& cb) {_callback = cb;}

    enum Event {
	DATA = AsyncFileWriter::DATA,
	FLUSHING = AsyncFileWriter::FLUSHING,
	ERROR = AsyncFileWriter::OS_ERROR
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
    void connect_socket(XorpFd sock, string raddr, uint16_t port,
			string laddr, ConnectCallback cb);
    void connect_socket_complete(XorpFd fd, IoEventType type,
				 ConnectCallback cb);
    void connect_socket_break();

    void async_add(XorpFd sock);
    void async_remove();
    void async_remove_reader();
    
    void read_from_server(XorpFd sock);
    void write_to_server(XorpFd sock);

    void send_message_complete(AsyncFileWriter::Event e,
			      const uint8_t* buf,
			      const size_t buf_bytes,
			      const size_t offset,
			      SendCompleteCallback cb);

    void async_read_start(size_t cnt = BGPPacket::COMMON_HEADER_LEN,
			  size_t ofset = 0);
    void async_read_message(AsyncFileWriter::Event ev,
			   const uint8_t *buf,
			   const size_t buf_bytes,
			   const size_t offset);

    MessageCallback _callback;
    AsyncFileWriter *_async_writer;
    AsyncFileReader *_async_reader;

    bool _disconnecting;
    bool _connecting;
    bool _md5sig;

    uint8_t _read_buf[BGPPacket::MAXPACKETSIZE]; // Maximum allowed BGP message
};

class SocketServer : public Socket {
public:
    SocketServer(EventLoop& e);
    SocketServer(const Iptuple& iptuple, EventLoop& e);
};

#endif // __BGP_SOCKET_HH__
