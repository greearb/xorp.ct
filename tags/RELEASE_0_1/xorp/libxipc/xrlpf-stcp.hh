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

// $XORP: xorp/libxipc/xrlpf-stcp.hh,v 1.13 2002/12/09 18:29:09 hodson Exp $

#ifndef __XRLPF_STCP_HH__
#define __XRLPF_STCP_HH__

#include "xrlpf.hh"
#include "libxorp/asyncio.hh"

// ----------------------------------------------------------------------------
// XRL Protocol Family : Simplest TCP 

class STCPRequestHandler;
class XrlPFSTCPSender;
class STCPPacketHeader;

struct RequestState {
    XrlPFSTCPSender*	parent;
    uint32_t		seqno;
    Xrl			xrl;
    XorpTimer		timeout;
    XrlPFSender::SendCallback	callback;
    void*		userdata;

    RequestState(XrlPFSTCPSender* p, uint32_t sno, const Xrl& x, 
		 XrlPFSender::SendCallback cb, void* thunk)
	: parent(p), seqno(sno), xrl(x), callback(cb), userdata(thunk) {}
    RequestState() {}
    bool has_seqno(uint32_t n) const { return seqno == n; }
};



class XrlPFSTCPListener : public XrlPFListener {
public:
    XrlPFSTCPListener(EventLoop& e, XrlCmdMap* m = 0, int port = 0) 
	throw (XrlPFConstructorError);
    ~XrlPFSTCPListener();

    const char* address() const	{ return _address_slash_port.c_str(); }
    const char* protocol() const { return _protocol; }

    void add_request_handler(STCPRequestHandler* h);
    void remove_request_handler(const STCPRequestHandler* h);
private:
    static void connect_hook(int fd, SelectorMask m, void* thunked_listener);

    int	_fd;
    string _address_slash_port;
    list<STCPRequestHandler*> _request_handlers;

    static const char* _protocol;
    static const uint32_t _timeout_period;
};



class XrlPFSTCPSender : public XrlPFSender {
public:
    XrlPFSTCPSender(EventLoop& e, const char* address = NULL) 
	throw (XrlPFConstructorError);
    virtual ~XrlPFSTCPSender();

    void send(const Xrl& x, SendCallback cb, void *cookie);
    bool sends_pending() const 			{ return true; }
    bool alive() const 				{ return _fd > 0; }

    static const char* protocol() 		{ return _protocol; }

    // Response timeout control
    void set_timeout_ms(uint32_t t)		{ _timeout_ms = t; }
    uint32_t timeout_ms() const 		{ return _timeout_ms; }

    // Keepalive interval
    void set_keepalive_ms(uint32_t t);
    uint32_t keepalive_ms() const		{ return _keepalive_ms; }

private:
    void send_first_request();
    void die(const char* reason);
    void prepare_packet(const XrlPFSender::Request& r);

    int _fd;

    // Transmission related
    AsyncFileWriter* _writer;

    list<RequestState> _requests_pending;	// All requests pending
    list<RequestState> _requests_sent;		// All requests sent

    vector<uint8_t>	_request_packet;
    uint32_t		_current_seqno;

    void update_writer(AsyncFileWriter::Event	e,
		       const uint8_t*		buffer, 
		       size_t 			buffer_bytes,
		       size_t 			bytes_done);

    RequestState* find_request(uint32_t seqno);
    void postpone_timeout(uint32_t seqno);
    void timeout_request(uint32_t seqno);
    static void request_timed_out(void *thunked_request);

    // Tunable timer variables
    uint32_t _timeout_ms;
    uint32_t _keepalive_ms;

    // Reception related
    AsyncFileReader*	    _reader;
    vector<uint8_t>	    _reply;
    const STCPPacketHeader* _sph;

    void prepare_for_reply_header();

    void recv_data(AsyncFileReader::Event	ev, 
		   const uint8_t*		buffer,
		   size_t			buffer_bytes,
		   size_t			offset);

    void dispatch_reply();

    // Keepalive related
    XorpTimer		_keepalive_timer;
    vector<uint8_t>	_keepalive_packet;
    bool		_keepalive_in_progress;

    inline void start_keepalives();
    inline void stop_keepalives();
    inline void postpone_keepalive();
    inline bool send_keepalive();
    void confirm_keepalive(AsyncFileOperator::Event	e,
			   const uint8_t*		buffer,
			   size_t			buffer_bytes,
			   size_t			offset);

    // General stuff
    static const char* _protocol;
};

#endif // __XRLPF_STCP_HH__
