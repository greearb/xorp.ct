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

// $XORP: xorp/libxipc/xrl_pf_stcp.hh,v 1.19 2004/06/10 22:41:12 hodson Exp $

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
class STCPPacketHeader;
class RequestState;

/**
 * @short Listener for XRL's transported by TCP.
 */
class XrlPFSTCPListener : public XrlPFListener {
public:
    XrlPFSTCPListener(EventLoop& e, XrlDispatcher* xr = 0, uint16_t port = 0)
	throw (XrlPFConstructorError);
    ~XrlPFSTCPListener();

    const char* address() const	{ return _address_slash_port.c_str(); }
    const char* protocol() const { return _protocol; }

    void add_request_handler(STCPRequestHandler* h);
    void remove_request_handler(const STCPRequestHandler* h);

    bool response_pending() const;

private:
    void connect_hook(int fd, SelectorMask m);

    int	_fd;
    string _address_slash_port;
    list<STCPRequestHandler*> _request_handlers;

    static const char* _protocol;
    static const uint32_t _timeout_period;
};

/**
 * @short Sender of Xrls by TCP.
 */
class XrlPFSTCPSender : public XrlPFSender {
public:
    XrlPFSTCPSender(EventLoop& e, const char* address = 0)
	throw (XrlPFConstructorError);
    virtual ~XrlPFSTCPSender();

    bool send(const Xrl& 			x,
	      bool 				direct_call,
	      const XrlPFSender::SendCallback& 	cb);

    bool sends_pending() const;

    inline bool alive() const 				{ return _fd > 0; }

    const char* protocol() const;

    inline static const char* protocol_name()		{ return _protocol; }

    void set_keepalive_ms(uint32_t t);

    inline uint32_t keepalive_ms() const	{ return _keepalive_ms; }

private:
    void die(const char* reason);

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

private:
    uint32_t 			 _uid;
    int				 _fd;

    // Transmission related
    AsyncFileWriter*		  _writer;

    list<ref_ptr<RequestState> > _requests_waiting;	// All requests pending
    list<ref_ptr<RequestState> > _requests_sent;	// All requests pending

    uint32_t			 _current_seqno;
    size_t			 _active_bytes;
    size_t			 _active_requests;

    // Tunable timer variables
    uint32_t			 _keepalive_ms;

    // Reception related
    BufferedAsyncReader*	 _reader;
    vector<uint8_t>		 _reply;
    const STCPPacketHeader*	 _sph;

    // Keepalive related
    XorpTimer			 _keepalive_timer;
    bool			 _keepalive_sent;

    // General stuff
    static const char*		 _protocol;
    static uint32_t		 _next_uid;
};

#endif // __LIBXIPC_XRL_PF_STCP_HH__
