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

// $XORP: xorp/libxipc/xrl_pf_stcp.hh,v 1.18 2003/10/02 18:55:05 hodson Exp $

#ifndef __LIBXIPC_XRL_PF_STCP_HH__
#define __LIBXIPC_XRL_PF_STCP_HH__

#include "xrl_pf.hh"
#include "libxorp/asyncio.hh"

class Xrl;

// ----------------------------------------------------------------------------
// XRL Protocol Family : Simplest TCP

class STCPRequestHandler;
class XrlPFSTCPSender;
class STCPPacketHeader;

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

class RequestState;

/**
 * @short Sender of Xrls by TCP.
 */
class XrlPFSTCPSender : public XrlPFSender {
public:
    XrlPFSTCPSender(EventLoop& e, const char* address = 0)
	throw (XrlPFConstructorError);
    virtual ~XrlPFSTCPSender();

    void send(const Xrl& x, const XrlPFSender::SendCallback& cb);

    bool sends_pending() const;

    inline bool alive() const 				{ return _fd > 0; }

    const char* protocol() const;

    inline static const char* protocol_name()		{ return _protocol; }

    void set_keepalive_ms(uint32_t t);

    inline uint32_t keepalive_ms() const	{ return _keepalive_ms; }

private:
    void send_first_request();
    void die(const char* reason);

    void update_writer(AsyncFileWriter::Event	e,
		       const uint8_t*		buffer,
		       size_t 			buffer_bytes,
		       size_t 			bytes_done);

    RequestState* find_request(uint32_t seqno);

    void prepare_for_reply_header();

    void recv_data(AsyncFileReader::Event	ev,
		   const uint8_t*		buffer,
		   size_t			buffer_bytes,
		   size_t			offset);

    void dispatch_reply();

    inline void start_keepalives();
    inline void stop_keepalives();
    inline void postpone_keepalive();
    inline bool send_keepalive();
    void confirm_keepalive(AsyncFileOperator::Event	e,
			   const uint8_t*		buffer,
			   size_t			buffer_bytes,
			   size_t			offset);

private:
    uint32_t 			 _uid;
    int				 _fd;

    // Transmission related
    AsyncFileWriter*		  _writer;

    list<ref_ptr<RequestState> > _requests_pending;	// All requests pending
    list<ref_ptr<RequestState> > _requests_sent;	// All requests sent

    vector<uint8_t>		 _request_packet;
    uint32_t			 _current_seqno;

    // Tunable timer variables
    uint32_t			 _keepalive_ms;

    // Reception related
    AsyncFileReader*	    	 _reader;
    vector<uint8_t>		 _reply;
    const STCPPacketHeader*	 _sph;

    // Keepalive related
    XorpTimer			 _keepalive_timer;
    vector<uint8_t>		 _keepalive_packet;
    bool			 _keepalive_in_progress;

    // General stuff
    static const char*		 _protocol;
    static uint32_t		 _next_uid;
};

#endif // __LIBXIPC_XRL_PF_STCP_HH__
