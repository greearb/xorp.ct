// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2011 XORP, Inc and Others
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

#ifndef __LIBXIPC_XRL_PF_STCP_HH__
#define __LIBXIPC_XRL_PF_STCP_HH__

#include "xrl_pf.hh"
#include "libxorp/asyncio.hh"
#include "libxorp/buffered_asyncio.hh"

class Xrl;

// ----------------------------------------------------------------------------
// XRL Protocol Family : Simplest TCP

class STCPRequestHandler;
class XrlPFSTCPSender;
class RequestState;

/**
 * @short Listener for XRL's transported by TCP.
 */
class XrlPFSTCPListener : public XrlPFListener {
public:
    XrlPFSTCPListener(EventLoop& e, XrlDispatcher* xr = 0, uint16_t port = 0)
	throw (XrlPFConstructorError);
    virtual ~XrlPFSTCPListener();

    virtual const char* address() const	 { return _address_slash_port.c_str(); }
    virtual const char* protocol() const { return _protocol; }

    void add_request_handler(STCPRequestHandler* h);
    void remove_request_handler(const STCPRequestHandler* h);
    void connect_hook(XorpFd fd, IoEventType type);
    bool response_pending() const;

    virtual string toString() const;

protected:
    XrlPFSTCPListener(EventLoop* e, XrlDispatcher* xr = 0);

    XorpFd	_sock;
    string	_address_slash_port;

private:
    list<STCPRequestHandler*>	_request_handlers;

    static const char*		_protocol;
};

/**
 * @short Sender of Xrls by TCP.
 */
class XrlPFSTCPSender : public XrlPFSender {
public:
    XrlPFSTCPSender(EventLoop& e, const char* address = 0,
	TimeVal keepalive_period = DEFAULT_SENDER_KEEPALIVE_PERIOD)
	throw (XrlPFConstructorError);
    XrlPFSTCPSender(EventLoop* e, const char* address = 0,
	TimeVal keepalive_period = DEFAULT_SENDER_KEEPALIVE_PERIOD);
    virtual ~XrlPFSTCPSender();

    bool send(const Xrl& 			x,
	      bool 				direct_call,
	      const XrlPFSender::SendCallback& 	cb);

    bool	        sends_pending() const;
    bool	        alive() const		    { return _sock.is_valid(); }
    virtual const char* protocol() const;
    static const char*  protocol_name()		    { return _protocol; }
    void	        set_keepalive_time(const TimeVal& time);
    const TimeVal&	keepalive_time() const	    { return _keepalive_time; }
    void	        batch_start();
    void	        batch_stop();

protected:
    void construct();

    XorpFd _sock;

private:
    void die(const char* reason, bool verbose = true);

    void update_writer(AsyncFileWriter::Event	e,
		       const uint8_t*		buffer,
		       size_t 			buffer_bytes,
		       size_t 			bytes_done);

    RequestState* find_request(uint32_t seqno);

    void read_event(BufferedAsyncReader*	reader,
		    BufferedAsyncReader::Event	ev,
		    uint8_t*			buffer,
		    size_t			buffer_bytes);

    void send_request(RequestState*);
    void dispose_request();

    void start_keepalives();
    void stop_keepalives();
    void defer_keepalives();
    bool send_keepalive();

public:
    static const TimeVal	 DEFAULT_SENDER_KEEPALIVE_PERIOD;

private:
    uint32_t 			 _uid;

    // Transmission related
    AsyncFileWriter*		  _writer;

    list<ref_ptr<RequestState> > _requests_waiting;	// All requests pending
    list<ref_ptr<RequestState> > _requests_sent;	// All requests pending

    uint32_t			 _current_seqno;
    size_t			 _active_bytes;
    size_t			 _active_requests;

    // Tunable timer variables
    TimeVal			_keepalive_time;

    // Reception related
    BufferedAsyncReader*	 _reader;
    vector<uint8_t>		 _reply;

    // Keepalive related
    XorpTimer			 _keepalive_timer;
    TimeVal			 _keepalive_last_fired;
    bool			 _keepalive_sent;

    // General stuff
    static const char*		 _protocol;
    static uint32_t		 _next_uid;
    bool			 _batching;
};

#endif // __LIBXIPC_XRL_PF_STCP_HH__
