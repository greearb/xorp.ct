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

#ident "$XORP: xorp/libxipc/xrl_pf_stcp.cc,v 1.27 2003/09/26 21:34:34 hodson Exp $"

#include "libxorp/xorp.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

#include <string>
#include <vector>
#include <map>
#include <algorithm>

#include "libxorp/debug.h"
#include "libxipc/xrl_module.h"
#include "libxorp/xlog.h"

#include "sockutil.hh"
#include "header.hh"

#include "xrl.hh"
#include "xrl_error.hh"
#include "xrl_pf_stcp.hh"
#include "xrl_pf_stcp_ph.hh"
#include "xrl_dispatcher.hh"

const char* XrlPFSTCPSender::_protocol   = "stcp";
const char* XrlPFSTCPListener::_protocol = "stcp";

static const uint32_t DEFAULT_SENDER_KEEPALIVE_MS = 10000;


// ----------------------------------------------------------------------------
// STCPRequestHandler - created by Listener to manage requests over a
// connection.  These are allocated on by XrlPFSTCPListener and delete
// themselves using a timeout timer when they have been quiescent for a while

class STCPRequestHandler {
public:
    STCPRequestHandler(XrlPFSTCPListener& parent, int fd) :
	_parent(parent), _fd(fd), _request(),
	_reader(parent.eventloop(), fd), _writer(parent.eventloop(), fd)
    {
	EventLoop& e = _parent.eventloop();
	_life_timer = e.new_oneoff_after_ms(QUIET_LIFE_MS,
					    callback(this,
						     &STCPRequestHandler::die,
						     "life timer expired",
						     true));
	prepare_for_request();
	debug_msg("STCPRequestHandler (%p) fd = %d\n", this, fd);
    }

    ~STCPRequestHandler()
    {
	_parent.remove_request_handler(this);
	_reader.stop();
	_writer.stop();
	close(_fd);
	debug_msg("~STCPRequestHandler (%p) fd = %d\n", this, _fd);
    }

    void dispatch_request(uint32_t seqno, const char* xrl_c_str);
    void prepare_for_request();
    void update_reader(AsyncFileReader::Event,
		       const uint8_t*,
		       size_t,
		       size_t);

    void start_writer();
    void update_writer(AsyncFileWriter::Event,
		       const uint8_t*,
		       size_t,
		       size_t);

    // Death related
    void postpone_death();
    void die(const char* reason, bool verbose = true);

private:
    XrlPFSTCPListener& _parent;
    int _fd;

    // Inbound request buffer
    vector<char> _request;
    size_t _request_seqno;
    size_t _request_payload_bytes;

    // Reader associated with buffer
    AsyncFileReader _reader;

    // Writer associated current response (head of responses).
    AsyncFileWriter _writer;

    typedef vector<uint8_t> ReplyPacket;
    list<ReplyPacket> _responses; // head is currently being written

    size_t _response_offset;	// byte position of response being written

    // STCPRequestHandlers delete themselves if quiescent for timeout period
    XorpTimer _life_timer;

    void parse_header(const uint8_t* buffer, size_t buffer_bytes);
    void parse_payload();

    static const int QUIET_LIFE_MS = 180 * 1000;	// 3 minutes
};

void
STCPRequestHandler::prepare_for_request()
{
    _request_payload_bytes = 0;
    _request_seqno = 0;

    if (_request.size() < sizeof(STCPPacketHeader)) {
	_request.resize(sizeof(STCPPacketHeader));
    }
    _reader.add_buffer((uint8_t*)&_request[0], sizeof(STCPPacketHeader),
		       callback(this, &STCPRequestHandler::update_reader));
    _reader.start();
}

void
STCPRequestHandler::parse_header(const uint8_t* buffer, size_t bytes_done)
{
    if (bytes_done < sizeof(STCPPacketHeader)) {
	debug_msg("Incoming with small header %u < %u\n",
		  (uint32_t)bytes_done, (uint32_t)sizeof(STCPPacketHeader));
	return;
    }
    assert(bytes_done == sizeof(STCPPacketHeader));

    const STCPPacketHeader* sph = reinterpret_cast<const STCPPacketHeader*>
	(buffer);

    if (!sph->is_valid()) {
	die("bad header");
	return;
    }

    if (sph->type() == STCP_PT_HELO) {
	debug_msg("got keepalive\n");
	prepare_for_request(); // no further processing required
	return;
    } else if (sph->type() != STCP_PT_REQUEST) {
	die("Bad packet type");
	return;
    }

    _request_seqno = sph->seqno();
    _request_payload_bytes = sph->payload_bytes();

    if (_request_payload_bytes >= _request.size()) {
	_request.resize(_request_payload_bytes + 1);
    }
    _reader.add_buffer((uint8_t*)&_request[0],
		       _request_payload_bytes,
		       callback(this, &STCPRequestHandler::update_reader));
    _reader.start();
    return;
}

void
STCPRequestHandler::parse_payload()
{
    assert(_request.size() >= _request_payload_bytes + 1);

    // Prepare to cast request as a C string
    _request[_request_payload_bytes] = 0;

    // dispatch request and buffer result
    dispatch_request(_request_seqno, (const char*)&_request[0]);

    // get ready for next request
    prepare_for_request();
}

void
STCPRequestHandler::update_reader(AsyncFileReader::Event ev,
				  const uint8_t*	 buffer,
				  size_t 		 /* buffer_bytes */,
				  size_t		 bytes_done)
{
    postpone_death();

    if (ev == AsyncFileReader::FLUSHING)
	return; // this code predate flushing event

    if (ev == AsyncFileReader::ERROR_CHECK_ERRNO) {
	debug_msg("Read failed (errno = %d): %s\n",
		  errno, strerror(errno));
	if (errno == EAGAIN) {
	    _reader.resume(); // error cause reader to stop
	} else {
	    debug_msg("Death due to read error\n");
	    die("read error");
	}
	return;
    }

    if (ev == AsyncFileReader::END_OF_FILE) {
	die("end of file", false);
	return;
    }

    assert(ev == AsyncFileReader::DATA);
    debug_msg("update_reader: %u done, expecting %u\n",
	      (uint32_t)bytes_done,
	      (_request_payload_bytes) ?
	      (uint32_t)_request_payload_bytes
	      : (uint32_t)sizeof(STCPPacketHeader));

    if (_request_payload_bytes == 0) {
	parse_header(buffer, bytes_done);
    } else if (bytes_done == _request_payload_bytes) {
	parse_payload();
    }
}

void
STCPRequestHandler::dispatch_request(uint32_t seqno, const char* xrl_c_str)
{
    XrlError e;
    XrlArgs response;

    const XrlDispatcher* d = _parent.dispatcher();
    assert(d != 0);

    try {
	Xrl xrl(xrl_c_str);
	e = d->dispatch_xrl(xrl.command(), xrl.args(), response);
    } catch (const InvalidString&) {
	e = XrlError(XrlError::INTERNAL_ERROR().error_code(), "corrupt xrl");
    }
    debug_msg("Response count %u\n", (uint32_t)_responses.size());

    _responses.push_back(ReplyPacket());
    ReplyPacket& r = _responses.back();

    string xrl_data = response.str();
    r.resize(sizeof(STCPPacketHeader) + e.note().size() + xrl_data.size());

    // We cast first few bytes of allocated block to packet header to
    // avoid a copy after constructing.  This works because
    // STCPPacketHeader uses a network order representation.
    STCPPacketHeader* sph = reinterpret_cast<STCPPacketHeader*>(&r[0]);
    sph->initialize(seqno, STCP_PT_RESPONSE, e, xrl_data.size());

    if (e.note().size()) {
	memcpy(&r[0] + sizeof(STCPPacketHeader),
	       e.note().c_str(), e.note().size());
    }

    if (xrl_data.size()) {
	memcpy(&r[0] + sizeof(STCPPacketHeader) + e.note().size(),
	       xrl_data.c_str(), xrl_data.size());
    }

    debug_msg("about to start_writer (%d)\n", _responses.size() != 0);
    start_writer();
}

void
STCPRequestHandler::start_writer()
{
    debug_msg("start_writer (%d)\n", _responses.size() != 0);
    if (_responses.empty() == false) {
	assert(_writer.running() == false);
	ReplyPacket& r = _responses.front();
	_response_offset = 0;
	_writer.add_buffer(&r[0], r.size(),
			   callback(this, &STCPRequestHandler::update_writer));
	_writer.start();
    }
}

void
STCPRequestHandler::update_writer(AsyncFileWriter::Event ev,
				  const uint8_t*	 /* buffer */,
				  size_t		 /* buffer_bytes */,
				  size_t		 bytes_done) {
    postpone_death();

    if (ev == AsyncFileWriter::FLUSHING)
	return;	// code pre-dates FLUSHING event

    debug_msg("Writer offset %u\n", (uint32_t)bytes_done);

    if (ev == AsyncFileWriter::ERROR_CHECK_ERRNO && errno != EAGAIN) {
	debug_msg("Read failed: %s\n", strerror(errno));
	die("read failed");
	return;
    }

    list<ReplyPacket>::iterator ri = _responses.begin();
    if (ri->size() == bytes_done) {
	debug_msg("Packet completed -> %u bytes written.\n",
		  (uint32_t)ri->size());
	// erase old head
	_responses.erase(ri);
	_response_offset = 0;
	// restart writer if necessary
	start_writer();
    }
}

// Methods and constants relating to death
void
STCPRequestHandler::postpone_death()
{
    _life_timer.schedule_after_ms(QUIET_LIFE_MS);
}

void
STCPRequestHandler::die(const char *reason, bool verbose)
{
    debug_msg(reason);
    if (verbose)
	XLOG_ERROR("STCPRequestHandler died: %s", reason);
    delete this;
}


// ----------------------------------------------------------------------------
// Simple TCP Listener - creates TCPRequestHandlers for each incoming
// connection.

XrlPFSTCPListener::XrlPFSTCPListener(EventLoop&	    e,
				     XrlDispatcher* x,
				     uint16_t	    port)
    throw (XrlPFConstructorError)
    : XrlPFListener(e, x), _fd(-1), _address_slash_port()
{

    if ((_fd = create_listening_ip_socket(TCP, port)) < 1) {
	xorp_throw(XrlPFConstructorError, strerror(errno));
    }

    string addr;
    if (get_local_socket_details(_fd, addr, port) == false) {
        close(_fd);
        xorp_throw(XrlPFConstructorError, strerror(errno));
    }

    if (fcntl(_fd, F_SETFL, O_NONBLOCK) < 0) {
	debug_msg("failed to go non-blocking\n");
        close(_fd);
        xorp_throw(XrlPFConstructorError, strerror(errno));
    }

    _address_slash_port = address_slash_port(addr, port);
    _eventloop.add_selector(_fd, SEL_RD,
			     callback(this, &XrlPFSTCPListener::connect_hook));
}

XrlPFSTCPListener::~XrlPFSTCPListener()
{
    while (_request_handlers.empty() == false) {
	delete _request_handlers.front();
	// nb destructor for STCPRequestHandler triggers removal of node
	// from list
    }
    _eventloop.remove_selector(_fd);
    close(_fd);
}

void
XrlPFSTCPListener::connect_hook(int fd, SelectorMask /* m */)
{
    struct sockaddr_in a;
    socklen_t alen = sizeof(a);

    int cfd = accept(fd, (sockaddr*)&a, &alen);
    if (cfd < 0) {
	debug_msg("accept() failed: %s\n", strerror(errno));
	return;
    }
    fcntl(cfd, F_SETFL, fcntl(fd, F_GETFL) | O_NONBLOCK);
    add_request_handler(new STCPRequestHandler(*this, cfd));
}

void
XrlPFSTCPListener::add_request_handler(STCPRequestHandler* h)
{
    // assert handler is not already in list
    assert(find(_request_handlers.begin(), _request_handlers.end(), h)
	   == _request_handlers.end());
    _request_handlers.push_back(h);
}

void
XrlPFSTCPListener::remove_request_handler(const STCPRequestHandler* rh)
{
    list<STCPRequestHandler*>::iterator i;
    i = find(_request_handlers.begin(), _request_handlers.end(), rh);
    assert(i != _request_handlers.end());
    _request_handlers.erase(i);
}

bool
XrlPFSTCPListener::response_pending() const
{
    return _request_handlers.empty() == false;
}


/**
 * @short Sender state for tracking Xrl's forwarded by TCP.
 *
 * At some point this class should be re-factored to hold rendered Xrl
 * in wire format rather than as an Xrl since the existing scheme
 * requires additional copy operations.
 */
struct RequestState {
public:
    typedef XrlPFSender::SendCallback Callback;

public:
    RequestState(XrlPFSTCPSender* p,
		 uint32_t	  sn,
		 const Xrl&	  x,
		 const Callback&  cb)
	: _p(p), _sn(sn), _x(x), _cb(cb)
    {
	debug_msg("RequestState (%p - seqno %u)\n", this, sn);
	debug_msg("RequestState Xrl = %s\n", x.str().c_str());
    }

    inline bool		    has_seqno(uint32_t n) const { return _sn == n; }
    inline XrlPFSTCPSender* parent() const		{ return _p; }
    inline uint32_t	    seqno() const		{ return _sn; }
    inline Xrl&		    xrl() 			{ return _x; }
    inline Callback&	    cb() 			{ return _cb; }

    ~RequestState()
    {
	debug_msg("~RequestState (%p - seqno %u)\n", this, seqno());
    }

private:
    RequestState(const RequestState&);			// Not implemented
    RequestState& operator=(const RequestState&);	// Not implemented

private:
    XrlPFSTCPSender*	_p;				// parent
    uint32_t		_sn;				// sequence number
    Xrl			_x;
    Callback		_cb;
};


// ----------------------------------------------------------------------------
// Xrl Simple TCP protocol family sender -> -> -> XRLPFSTCPSender

XrlPFSTCPSender::XrlPFSTCPSender(EventLoop& e, const char* addr_slash_port)
    throw (XrlPFConstructorError)
    : XrlPFSender(e, addr_slash_port), _fd(-1),
      _keepalive_ms(DEFAULT_SENDER_KEEPALIVE_MS)
{
    _fd = create_connected_ip_socket(TCP, addr_slash_port);
    debug_msg("stcp sender (%p) fd = %d\n", this, _fd);
    if (_fd <= 0) {
	debug_msg("failed to connect to %s\n", addr_slash_port);
	xorp_throw(XrlPFConstructorError,
		   c_format("Could not connect to %s\n", addr_slash_port));
    }

    if (fcntl(_fd, F_SETFL, fcntl(_fd, F_GETFL) | O_NONBLOCK) < 0) {
	debug_msg("failed to go non-blocking.\n");
        close(_fd);
	xorp_throw(XrlPFConstructorError,
		   c_format("Failed to set fd non-blocking: %s\n",
			    strerror(errno)));
    }

    _reader = new AsyncFileReader(e, _fd);
    if (0 == _reader) {
        xorp_throw(XrlPFConstructorError, "Could not allocate reader.");
    }

    _writer = new AsyncFileWriter(e, _fd);
    if (0 == _writer) {
	delete _reader;
        xorp_throw(XrlPFConstructorError, "Could not allocate writer.");
    }

    prepare_for_reply_header();
    start_keepalives();
}

XrlPFSTCPSender::~XrlPFSTCPSender()
{
    delete _reader;
    _reader = 0;
    delete _writer;
    _writer = 0;
    _fd = -1;
    debug_msg("~XrlPFSTCPSender (%p)\n", this);
}

void
XrlPFSTCPSender::die(const char* reason)
{
    debug_msg("Sender dying (fd = %d) reason: %s\n", _fd, reason);

    XLOG_ASSERT(_fd > 0);

    // XLOG_ERROR("XrlPFSTCPSender died: %s", reason);

    stop_keepalives();

    _reader->flush_buffers();
    delete _reader;
    _reader = 0;

    _writer->flush_buffers();
    delete _writer;
    _writer = 0;

    close(_fd);
    _fd = -1;

    // Detach all callbacks before attempting to invoke them.
    // Otherwise destructor may get called when we're still going through
    // the lists of callbacks.
    list<ref_ptr<RequestState> > tmp_sent;
    tmp_sent.splice(tmp_sent.begin(), _requests_sent);

    list<ref_ptr<RequestState> > tmp_pending;
    tmp_pending.splice(tmp_pending.begin(), _requests_pending);

    debug_msg("Sent requests outstanding = %d\n", (int)tmp_sent.size());
    while (tmp_sent.empty() == false) {
	ref_ptr<RequestState>& rp = tmp_sent.front();
	if (rp->cb().is_empty() == false)
	    rp->cb()->dispatch(XrlError::SEND_FAILED(), 0);
	tmp_sent.pop_front();
    }

    debug_msg("Pending requests outstanding= %d\n", (int)tmp_pending.size());
    while (tmp_pending.empty() == false) {
	ref_ptr<RequestState>& rp = tmp_pending.front();
	if (rp->cb().is_empty() == false)
	    rp->cb()->dispatch(XrlError::SEND_FAILED(), 0);
	tmp_pending.pop_front();
    }
}

void
XrlPFSTCPSender::send(const Xrl& x, const XrlPFSender::SendCallback& cb)
{
    if (_fd <= 0) {
	debug_msg("Attempted send when socket is dead!\n");
	cb->dispatch(XrlError(SEND_FAILED, "socket dead"), 0);
	return;
    }

    bool push = (_requests_pending.empty() == true &&
		 _keepalive_in_progress == false);
    _requests_pending.push_back(
				new RequestState(this, _current_seqno++, x, cb)
				);
    if (push) {
	send_first_request();
    } else {
	assert(_writer->running() == true);
    }
}

bool
XrlPFSTCPSender::sends_pending() const
{
    bool queues_empty = _requests_sent.empty() && _requests_pending.empty();
    return !queues_empty;
}

RequestState*
XrlPFSTCPSender::find_request(uint32_t seqno)
{
    list<ref_ptr<RequestState> >::iterator i;
    for (i = _requests_sent.begin(); i != _requests_sent.end(); ++i) {
	ref_ptr<RequestState>& rp = *i;
	if (rp->has_seqno(seqno))
	    return rp.get();
    }

    for (i = _requests_pending.begin(); i != _requests_pending.end(); ++i) {
	ref_ptr<RequestState>& rp = *i;
	if (rp->has_seqno(seqno))
	    return rp.get();
    }

    abort();
    return 0;
}

void
XrlPFSTCPSender::send_first_request()
{
    assert(_writer);
    assert(_writer->running() == false);

    debug_msg("%s\n", __PRETTY_FUNCTION__);
    // Render data as ascii
    list< ref_ptr<RequestState> >::iterator i = _requests_pending.begin();
    ref_ptr<RequestState>& r = *i;
    string xrl_ascii = r->xrl().str();

    // Size outgoing packet accordingly
    size_t packet_size = sizeof(STCPPacketHeader) + xrl_ascii.size();
    _request_packet.resize(packet_size);

    // Configure header
    STCPPacketHeader* sph =
	reinterpret_cast<STCPPacketHeader*>(&_request_packet[0]);
    sph->initialize(r->seqno(), STCP_PT_REQUEST, XrlError::OKAY(),
		    xrl_ascii.size());

    // Copy-in payload
    memcpy(&_request_packet[sizeof(STCPPacketHeader)],
	   xrl_ascii.data(), xrl_ascii.size());

    // Kick off writing
    _writer->add_buffer(&_request_packet[0], _request_packet.size(),
			callback(this, &XrlPFSTCPSender::update_writer));
    _writer->start();
    assert(_writer->running());
}

void
XrlPFSTCPSender::update_writer(AsyncFileWriter::Event	e,
			       const uint8_t*		/* buffer */,
			       size_t			buffer_bytes,
			       size_t			bytes_done)
{
    debug_msg("bytes done %u / %u\n", (uint32_t)bytes_done,
	      (uint32_t)buffer_bytes);
    assert(_keepalive_in_progress == false);
    if (e == AsyncFileWriter::FLUSHING)
	return; // Code predates FLUSHING

    if (e != AsyncFileWriter::DATA) {
	debug_msg("Write failed: %s\n", strerror(errno));
	die("write failed");
    }

    if (bytes_done != _request_packet.size()) {
	return;
    }

    list<ref_ptr<RequestState> >::iterator rs = _requests_pending.begin();
    // Request has been sent. Move request from head of pend queue
    // to tail of sent queue where it will wait for a reply
    _requests_sent.splice(_requests_sent.end(), _requests_pending, rs);
    if (_requests_pending.empty() == false) {
	send_first_request();
    }
}

void
XrlPFSTCPSender::dispatch_reply()
{
    uint32_t seqno = _sph->seqno();
    assert(_sph->is_valid());

    // Packet format is Header + optional Error note + xrl_data as textual info

    char* data = reinterpret_cast<char*>(&_reply[0] +
					 sizeof(STCPPacketHeader));

    // We reserved an additional one byte for null termination
    data[_sph->payload_bytes()] = 0;

    XrlError rcv_err;
    if (_sph->error_note_bytes()) {
	rcv_err = XrlError(XrlErrorCode(_sph->error_code()),
			   string(data, _sph->error_note_bytes()));
	data += _sph->error_note_bytes();
    } else {
	rcv_err = XrlError(XrlErrorCode(_sph->error_code()));
    }

    const char* xrl_data = "";
    if (_sph->xrl_data_bytes()) {
	xrl_data = data;
    }
 
    list<ref_ptr<RequestState> >::iterator rs;
    for (rs = _requests_sent.begin(); rs != _requests_sent.end(); ++rs) {
	ref_ptr<RequestState>& rp = *rs;
	if (rp->has_seqno(seqno))
	    break;
    }

    if (rs == _requests_sent.end()) {
	RequestState* r = find_request(seqno);
	if (r != 0)
	    abort();
    }
    assert(rs != _requests_sent.end());

    XrlPFSender::SendCallback& cb = rs->get()->cb();
    try {
	XrlArgs response(xrl_data);
	cb->dispatch(rcv_err, &response);
    } catch (InvalidString& ) {
	XrlError xe (XrlError::INTERNAL_ERROR().error_code(),
		    "corrupt xrl response");
	cb->dispatch(xe, 0);
	debug_msg("Corrupt response: %s\n", xrl_data);
    }

    // Tidy up receive state
    _requests_sent.erase(rs);

    // Prepare for next reply
    prepare_for_reply_header();
}

void
XrlPFSTCPSender::recv_data(AsyncFileReader::Event e,
			   const uint8_t*	    buffer,
			   size_t		    /* buffer_bytes */,
			   size_t		    offset)
{
    assert(buffer == &_reply[0]);
    assert(_reply.size() >= sizeof(STCPPacketHeader));

    switch (e) {
    case AsyncFileReader::FLUSHING:
	return; // Code pre-dates FLUSHING event, dont care about it

    case AsyncFileReader::END_OF_FILE:
	die("reached end of file.  Far end probably closed pipe.");
	return;

    case AsyncFileReader::ERROR_CHECK_ERRNO:
	die(c_format("read error - %s", strerror(errno)).c_str());
	return;

    case AsyncFileReader::DATA:
	break;
    }

    if (offset < sizeof(STCPPacketHeader)) {
	// XXX Hopefully never reached.  Just wait until next
	// chunk arrives before doing any more processing...
	debug_msg("got small header...%u bytes\n", (uint32_t)offset);
	assert(_reader->running());
	return;
    }

    assert(_sph == 0 || offset >= sizeof(STCPPacketHeader));
    if (_sph == 0) {
	// Awaiting header - this looks like it
	_sph = reinterpret_cast<const STCPPacketHeader*>(&_reply[0]);
	if (_sph->is_valid() == false) {
	    debug_msg("Invalid packet header (%d type 0x%02x)\n",
		      _sph->is_valid(), _sph->type());
	    die("invalid packet header");
	    return;
	} else if (_sph->type() != STCP_PT_RESPONSE) {
	    die("unexpected packet type - not a response");
	    return;
	}
	// Dimension to be 1 char longer so we can null terminate data portion
	// later and cast it to a C-string.
	_reply.resize(sizeof(STCPPacketHeader) + _sph->payload_bytes() + 1);
	_sph = reinterpret_cast<const STCPPacketHeader*>(&_reply[0]);

	if (_sph->payload_bytes()) {
	    _reader->add_buffer_with_offset(
			&_reply[0], _reply.size() - 1,
			sizeof(STCPPacketHeader),
			callback(this, &XrlPFSTCPSender::recv_data)
			);
	    _reader->start();
	}
    }
    postpone_keepalive();

    if (offset == sizeof(STCPPacketHeader) + _sph->payload_bytes()) {
	dispatch_reply();
    }
}

void
XrlPFSTCPSender::prepare_for_reply_header()
{
    assert(_reader->running() == false);

    _sph = 0;
    if (_reply.size() < sizeof(STCPPacketHeader))
	_reply.resize(sizeof(STCPPacketHeader));
    _reader->add_buffer(&_reply[0], sizeof(STCPPacketHeader),
			callback(this, &XrlPFSTCPSender::recv_data));
    _reader->start();
}

const char*
XrlPFSTCPSender::protocol() const
{
    return _protocol;
}

// ----------------------------------------------------------------------------
// Sender keepalive related

void
XrlPFSTCPSender::set_keepalive_ms(uint32_t t)
{
    _keepalive_ms = t;
    start_keepalives();
}

inline void
XrlPFSTCPSender::start_keepalives()
{
    _keepalive_timer = _eventloop.new_periodic(_keepalive_ms,
						callback(this, &XrlPFSTCPSender::send_keepalive));
    _keepalive_packet.resize(sizeof(STCPPacketHeader));
    _keepalive_in_progress = false;
}

inline void
XrlPFSTCPSender::postpone_keepalive()
{
    _keepalive_timer.schedule_after_ms(_keepalive_ms);
}

inline void
XrlPFSTCPSender::stop_keepalives()
{
    _keepalive_timer.unschedule();
}

inline bool
XrlPFSTCPSender::send_keepalive()
{
    if (_writer->buffers_remaining() != 0) {
	// no-go in middle of a transaction.
	postpone_keepalive();
	return true;
    }

    assert(_keepalive_packet.size() == sizeof(STCPPacketHeader));

    STCPPacketHeader* sph =
	reinterpret_cast<STCPPacketHeader*>(&_keepalive_packet[0]);
    sph->initialize(_current_seqno++, STCP_PT_HELO, XrlError::OKAY(), 0);

    _writer->add_buffer(&_keepalive_packet[0], _keepalive_packet.size(),
			callback(this, &XrlPFSTCPSender::confirm_keepalive));
    _writer->start();

    _keepalive_in_progress = true;
    return true;
}

void
XrlPFSTCPSender::confirm_keepalive(AsyncFileWriter::Event e,
				   const uint8_t*	  /* buffer */,
				   size_t		  /* buffer_bytes */,
				   size_t		  bytes_done)
{
    assert(bytes_done == 0 || bytes_done == sizeof(STCPPacketHeader));

    _keepalive_in_progress = false;

    switch (e) {
    case AsyncFileWriter::FLUSHING:
	return; // code pre-dates FLUSHING

    case AsyncFileWriter::ERROR_CHECK_ERRNO:
    case AsyncFileWriter::END_OF_FILE:
	if (errno == EAGAIN) {
	    _writer->resume();
	    return;
	}
	die("Keepalive failed");
	return;

    case AsyncFileWriter::DATA:
	// Keepalive succeeded - kick off request dispatch if waiting.
	debug_msg("Keepalive success\n");
	assert(_writer->buffers_remaining() == 0);
	assert(_writer->running() == false);
	if (_requests_pending.empty() == false) {
	    send_first_request();
	}
    }
}
