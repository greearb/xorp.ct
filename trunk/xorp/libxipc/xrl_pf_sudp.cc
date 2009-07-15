// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
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



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "xrl_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/callback.hh"
#include "libxorp/exceptions.hh"

#include "libcomm/comm_api.h"

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif

#include <errno.h>

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <string>

#include "header.hh"
#include "xrl_error.hh"
#include "xrl_pf_sudp.hh"
#include "xrl_dispatcher.hh"
#include "xuid.hh"
#include "sockutil.hh"

// ----------------------------------------------------------------------------
// SUDP is "simple udp" - a minimalist and simple udp transport
// mechanism for XRLs.  It is intended as a placeholder to allow
// modules to start using XRL whilst we develop other mechanisms.
//
// Resolved names have protocol name "sudp" and specify addresses as
// "host/port"
//

// ----------------------------------------------------------------------------
// Constants

static char		SUDP_PROTOCOL_NAME[] = "sudp";
static string		SUDP_PROTOCOL = "sudp/1.0";

static const int	SUDP_REPLY_TIMEOUT_MS = 3000;
static const ssize_t	SUDP_RECV_BUFFER_BYTES = 32 * 1024;
static const ssize_t	SUDP_SEND_BUFFER_BYTES = SUDP_RECV_BUFFER_BYTES / 4;

const char* XrlPFSUDPSender::_protocol   = SUDP_PROTOCOL_NAME;
const char* XrlPFSUDPListener::_protocol = SUDP_PROTOCOL_NAME;

struct Request {
    XrlPFSender*		parent;		// Request creator
    XrlPFSender::SendCallback	cb;		// User Callback
    XUID			xuid;		// Unique Request ID
    XorpTimer			timeout;	// Timeout timer

    Request(XrlPFSender* p, const XrlPFSender::SendCallback& scb)
	: parent(p), cb(scb) {}
    bool operator==(const XUID& x) const { return xuid == x; }
};

// ----------------------------------------------------------------------------
// Utility Functions

static string
render_dispatch_header(const XUID& id, uint32_t content_bytes)
{
    HeaderWriter h;
    h.add("Protocol", SUDP_PROTOCOL);
    h.add("XUID", id.str());
    h.add("Content-Length", content_bytes);
    return h.str();
}

static bool
parse_dispatch_header(string hdr, XUID& id, uint32_t& content_bytes)
{
    try {
	HeaderReader h(hdr);
	string protocol, sid;
	h.get("Protocol", protocol);
	h.get("XUID", sid);
	h.get("Content-Length", content_bytes);
	id = XUID(sid);
	return (protocol == SUDP_PROTOCOL);
    } catch (const HeaderReader::InvalidString&) {
	debug_msg("header invalid\n");
    } catch (const HeaderReader::NotFound&) {
	debug_msg("header missing fields\n");
    }
    return false;
}

static string
xrlerror_to_status(const XrlError& e)
{
    string r = c_format("%d", e.error_code());
    if (e.note().size()) {
	r += " " + e.note();
    }
    return r;
}

static XrlError
status_to_xrlerror(const string& status)
{
    uint32_t error_code = 0;

    string::const_iterator si = status.begin();
    while (si != status.end()) {
	if (xorp_isdigit(*si) == false)
	    break;
	error_code *= 10;
	error_code += *si - '0';
	si++;
    }

    if (si == status.begin()) {
	XLOG_ERROR("Missing XrlError::errorcode value");
	return XrlError(INTERNAL_ERROR,	"corrupt xrl response");
    }

    if (si == status.end())
	return XrlErrorCode(error_code);

    si++;
    return XrlError(XrlErrorCode(error_code), string(si, status.end()));
}

static string
render_response(const XrlError& e, const XUID& id, uint32_t content_bytes)
{
    HeaderWriter h;
    h.add("Protocol", SUDP_PROTOCOL);
    h.add("XUID", id.str());
    h.add("Status", xrlerror_to_status(e));
    h.add("Content-Length", content_bytes);
    return h.str();
}

static bool
parse_response(const char* buf,
	       XrlError& e,
	       XUID& xuid,
	       uint32_t& header_bytes,
	       uint32_t& content_bytes) {
    try {
	HeaderReader h(buf);

	string protocol;
	h.get("Protocol", protocol);
	if (protocol != SUDP_PROTOCOL) return false;

	string status;
	h.get("Status", status);
	e = status_to_xrlerror(status);

	string xuid_str;
	h.get("XUID", xuid_str);
	xuid = XUID(xuid_str);
	h.get("Content-Length", content_bytes);
	header_bytes = h.bytes_consumed();

	return true;
    } catch (const HeaderReader::InvalidString&) {
	debug_msg("Invalid string");
    } catch (const HeaderReader::NotFound&) {
	debug_msg("Not found");
    } catch (const XUID::InvalidString&) {
	debug_msg("Failed to restore XUID from string");
    }
    return false;
}

// ----------------------------------------------------------------------------
// XrlPFUDPSender

XorpFd XrlPFSUDPSender::sender_sock;
int XrlPFSUDPSender::instance_count;

typedef map<const XUID, Request> XuidRequestMap;
static XuidRequestMap requests_pending;

XrlPFSUDPSender::XrlPFSUDPSender(EventLoop& e, const char* address_slash_port)
    throw (XrlPFConstructorError)
    : XrlPFSender(e, address_slash_port)
{
    string addr;
    uint16_t port;

    if (split_address_slash_port(address_slash_port, addr, port) != true ||
	address_lookup(addr, _destination.sin_addr) != true) {
	xorp_throw(XrlPFConstructorError,
		   c_format("Bad destination: %s\n", address_slash_port));
    }
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
    _destination.sin_len = sizeof(struct sockaddr_in);
#endif
    _destination.sin_family = AF_INET;
    _destination.sin_port = htons(port);

    if (!sender_sock.is_valid()) {
	debug_msg("Creating master socket\n");
	sender_sock = comm_open_udp(AF_INET, COMM_SOCK_NONBLOCKING);
	if (sender_sock.is_valid()) {
	    if (comm_sock_set_sndbuf(sender_sock, SUDP_SEND_BUFFER_BYTES,
		SUDP_SEND_BUFFER_BYTES) < SUDP_SEND_BUFFER_BYTES)
	    {
		comm_close(sender_sock);
		sender_sock.clear();
		xorp_throw(XrlPFConstructorError,
			   c_format("Could not create master socket: "
				    "cannot set socket sending buffer to %d\n",
				    XORP_INT_CAST(SUDP_SEND_BUFFER_BYTES)));
	    }
	    _eventloop.add_ioevent_cb(sender_sock, IOT_READ,
				      callback(&XrlPFSUDPSender::recv));
	} else {
	    xorp_throw(XrlPFConstructorError,
		       c_format("Could not create master socket: %s.\n",
				comm_get_last_error_str()));
	}
    }
    instance_count++;
    debug_msg("Created XrlPFSUDPSender %s instance count %d sender_sock %s\n",
	      address_slash_port, instance_count, sender_sock.str().c_str());
}

XrlPFSUDPSender::~XrlPFSUDPSender()
{
    instance_count--;
    debug_msg("~XrlPFSUDPSender %p- instance count %d\n",
	      this, instance_count);

    if (instance_count == 0) {
	_eventloop.remove_ioevent_cb(sender_sock, IOT_READ);
	comm_close(sender_sock);
	sender_sock.clear();
    }

    // Delete requests associated with us, they cannot possibly be valid
    XuidRequestMap::iterator i = requests_pending.begin();
    while (i != requests_pending.end()) {
	if (i->second.parent == this) {
	    requests_pending.erase(i++);
	} else {
	    i++;
	}
    }
}

bool
XrlPFSUDPSender::send(const Xrl& 			x,
		      bool 				direct_call,
		      const XrlPFSender::SendCallback& 	cb)
{
    // Map request id to current object instance
    Request request(this, cb);
    assert(requests_pending.find(request.xuid) == requests_pending.end());

    pair<XuidRequestMap::iterator, bool> p =
	requests_pending.insert(XuidRequestMap::value_type(request.xuid,
							   request));
    if (p.second == false) {
	if (direct_call) {
	    return false;
	} else {
	    cb->dispatch(XrlError(SEND_FAILED, "Insufficient memory"), 0);
	    return true;
	}
    }

    // Prepare data
    string xrl = x.str();
    string header = render_dispatch_header(request.xuid,
					   static_cast<uint32_t>(xrl.size()));
    string msg = header + xrl;

    ssize_t msg_bytes = msg.size();
    if (msg_bytes > SUDP_SEND_BUFFER_BYTES) {
	debug_msg("Message sent larger than transport method designed");
    } else if (::sendto(sender_sock, msg.data(), msg.size(), 0,
		      (sockaddr*)&_destination,
#ifdef HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
		      _destination.sin_len
#else
		      sizeof(_destination)
#endif
		) != msg_bytes) {
	debug_msg("Write failed: %s\n", comm_get_last_error_str());
	requests_pending.erase(p.first);

	if (direct_call) {
	    return false;
	} else {
	    cb->dispatch(XrlError::SEND_FAILED(), 0);
	    return true;
	}
    }

    XuidRequestMap::iterator& xi = p.first;

    xi->second.timeout =
	_eventloop.new_oneoff_after_ms(SUDP_REPLY_TIMEOUT_MS,
	    callback(this, &XrlPFSUDPSender::timeout_hook, request.xuid));
    debug_msg("XrlPFSUDPSender::send (qsize %u)\n",
	      XORP_UINT_CAST(requests_pending.size()));
    return true;
}

bool
XrlPFSUDPSender::sends_pending() const
{
    XuidRequestMap::const_iterator i = requests_pending.begin();
    while (i != requests_pending.end()) {
	const XrlPFSender* parent = i->second.parent;
	if (parent == this)
	    return true;
	++i;
    }
    return false;
}

bool
XrlPFSUDPSender::alive() const
{
    return true;
}

const char*
XrlPFSUDPSender::protocol() const
{
    return _protocol;
}

void
XrlPFSUDPSender::timeout_hook(XUID xuid)
{
    map<const XUID, Request>::iterator i = requests_pending.find(xuid);
    assert (i != requests_pending.end());

    Request& r = i->second;
    SendCallback cb = r.cb;

    debug_msg("%p Erasing state for %s (timeout)\n",
	      this, r.xuid.str().c_str());

    requests_pending.erase(i);
    cb->dispatch(XrlError::REPLY_TIMED_OUT(), 0);
}

// ----------------------------------------------------------------------------
// XrlPFSUDPSender timer and io event hooks

void
XrlPFSUDPSender::recv(XorpFd fd, IoEventType type)
{
    assert(fd == sender_sock);
    assert(type == IOT_READ);

    char buf[SUDP_RECV_BUFFER_BYTES + 1];

    ssize_t read_bytes = ::recvfrom(sender_sock, buf, SUDP_RECV_BUFFER_BYTES,
				    0, NULL, NULL);

    if (read_bytes < 0) {
	debug_msg("recvfrom failed: %s\n", comm_get_last_error_str());
	return;
    }
    buf[read_bytes] = '\0';

    XrlError	err;
    XUID 	xuid;
    uint32_t 	content_bytes = 0;
    uint32_t	header_bytes = 0;

    if (parse_response(buf, err, xuid, header_bytes, content_bytes) != true) {
	debug_msg("response header parsing failed\n");
	return;
    } else if (content_bytes + header_bytes != (uint32_t)read_bytes) {
	debug_msg("header and data bytes != read_bytes (%u + %u != %d\n",
		  XORP_UINT_CAST(header_bytes),
		  XORP_UINT_CAST(content_bytes),
		  XORP_INT_CAST(read_bytes));
    }

    debug_msg("Received %s\n", xuid.str().c_str());
    map<const XUID, Request>::iterator i = requests_pending.find(xuid);
    if (i == requests_pending.end()) {
	XLOG_WARNING("XRL Protocol Family SUDP: response arrived for XRL "
		     "that appears to have timed out.");
	return;
    }

    // Unschedule timer
    i->second.timeout.unschedule();

    // Copy out state we'd like to use from request before deleting it.
    SendCallback cb = i->second.cb;

    debug_msg("Erasing state for %s (answered)\n",
	      i->second.xuid.str().c_str());
    requests_pending.erase(i);

    try {
	XrlArgs response(buf + header_bytes);
	cb->dispatch(err, &response);
    } catch (const InvalidString&) {
	debug_msg("Corrupt response: "
		  "header_bytes %u content_bytes %u\n\t\"%s\"\n",
		  XORP_UINT_CAST(header_bytes),
		  XORP_UINT_CAST(content_bytes),
		  buf + header_bytes);
	XrlError xe(INTERNAL_ERROR, "corrupt xrl response");
	cb->dispatch(xe, 0);
	return;
    }
}

// ----------------------------------------------------------------------------
// XrlPFUDPListener

XrlPFSUDPListener::XrlPFSUDPListener(EventLoop& e, XrlDispatcher* xr)
    throw (XrlPFConstructorError)
    : XrlPFListener(e, xr)
{
    debug_msg("XrlPFSUDPListener\n");

    in_addr myaddr = get_preferred_ipv4_addr();

    _sock = comm_bind_udp4(&myaddr, 0, COMM_SOCK_NONBLOCKING);
    if (!_sock.is_valid()) {
	xorp_throw(XrlPFConstructorError,
		   c_format("Could not allocate listening IP socket: %s.",
			    comm_get_last_error_str()));
    }

    // XXX: We don't check return values here.
    (void)comm_sock_set_sndbuf(_sock, SO_SND_BUF_SIZE_MAX,
			       SO_SND_BUF_SIZE_MIN);
    (void)comm_sock_set_rcvbuf(_sock, SO_RCV_BUF_SIZE_MAX,
			       SO_RCV_BUF_SIZE_MIN);

    string addr;
    uint16_t port;
    if (get_local_socket_details(_sock, addr, port) == false) {
	comm_close(_sock);
	xorp_throw(XrlPFConstructorError,
		   c_format("Could not get local socket details."));
    }
    _addr = address_slash_port(addr, port);

    _eventloop.add_ioevent_cb(_sock, IOT_READ,
			      callback(this, &XrlPFSUDPListener::recv));
}

XrlPFSUDPListener::~XrlPFSUDPListener()
{
    _eventloop.remove_ioevent_cb(_sock, IOT_READ);
    comm_close(_sock);
}

void
XrlPFSUDPListener::recv(XorpFd fd, IoEventType type)
{
    static char rbuf[SUDP_RECV_BUFFER_BYTES + 1];

    assert(fd == _sock);
    assert(type == IOT_READ);

    debug_msg("recv()\n");

    struct sockaddr_storage sockfrom;
    socklen_t sockfrom_bytes = sizeof(sockfrom);

    ssize_t rbuf_bytes = ::recvfrom(_sock, rbuf, sizeof(rbuf) / sizeof(rbuf[0]),
				    0, (sockaddr*)&sockfrom, &sockfrom_bytes);
    if (rbuf_bytes < 0) {
	int err = comm_get_last_error();
	if (err == EWOULDBLOCK) {
	    return;
	} else {
	    debug_msg("recvfrom failed: %s\n", comm_get_error_str(err));
	    return;
	}
    }

    if (rbuf_bytes > SUDP_RECV_BUFFER_BYTES) {
	debug_msg("Packet too large (%d > %d) bytes\n",
		  XORP_INT_CAST(rbuf_bytes),
		  XORP_INT_CAST(SUDP_RECV_BUFFER_BYTES));
	return;
    }
    rbuf[rbuf_bytes] = '\0';

    debug_msg("XXX %s\n", rbuf);

    uint32_t content_bytes;
    XrlArgs response;
    XrlError	e;
    XUID	xuid;
    if (parse_dispatch_header(rbuf, xuid, content_bytes) == true) {
	e = dispatch_command(rbuf + rbuf_bytes - content_bytes, response);
	debug_msg("response \"%s\"\n", response.str().c_str());
	send_reply(&sockfrom, sockfrom_bytes, e, xuid, &response);
    } else {
	// XXX log busted header.
    }
}

const XrlError
XrlPFSUDPListener::dispatch_command(const char* rbuf, XrlArgs& reply)
{
    const XrlDispatcher* d = dispatcher();
    assert(d != 0);

    try {
	Xrl xrl(rbuf);
	const string& command = xrl.command();
	const XrlArgs& args = xrl.args();
	return d->dispatch_xrl(command, args, reply);
    } catch (InvalidString& e) {
	debug_msg("Invalid string - failed to dispatch %s\n", rbuf);
    }
    return XrlError(INTERNAL_ERROR, "corrupt xrl");
}

void
XrlPFSUDPListener::send_reply(struct sockaddr_storage*	ss,
			      socklen_t			ss_len,
			      const XrlError&		e,
			      const XUID&		xuid,
			      const XrlArgs* 	reply_args)
{
#ifdef XRLPFSUDPPARANOIA
    static XUID last("00000000-00000000-00000000-00000000");
    assert(last != xuid);
    assert(last < xuid);
    last = xuid;
#endif

    UNUSED(ss_len);	// XXX: ss_len is used only under certain conditions

    string reply;
    if (reply_args != 0) {
	reply = reply_args->str();
    }
    const string header = render_response(e, xuid,
					  static_cast<uint32_t>(reply.size()));

    struct iovec v[2];
    v[0].iov_base = const_cast<char*>(header.c_str());
    v[0].iov_len = header.size();
    v[1].iov_base = const_cast<char*>(reply.c_str());
    v[1].iov_len = reply.size();

    ssize_t v_bytes = v[0].iov_len + v[1].iov_len;

    if (v_bytes > SUDP_SEND_BUFFER_BYTES) {
	XLOG_ERROR("Failed to send reply: message too large %d (max %d)",
		   XORP_INT_CAST(v_bytes),
		   XORP_INT_CAST(SUDP_SEND_BUFFER_BYTES));
	return;
    }

    int err = 0;
    bool is_error = false;
#ifdef HOST_OS_WINDOWS
    if (SOCKET_ERROR == WSASendTo(_sock, (LPWSABUF)v, 2, (LPDWORD)&v_bytes,
				  0, (sockaddr*)ss, ss_len, NULL, NULL)) {
	err = WSAGetLastError();
	is_error = true;
    }

#else // ! HOST_OS_WINDOWS
    msghdr m;
    memset(&m, 0, sizeof(m));
    m.msg_name = (caddr_t)ss;
#ifdef HAVE_STRUCT_SOCKADDR_STORAGE_SS_LEN
    m.msg_namelen = ss->ss_len;
#else
    m.msg_namelen = ss_len;
#endif
    m.msg_iov = v;
    m.msg_iovlen = sizeof(v) / sizeof(v[0]);

    if (v_bytes != sendmsg(_sock, &m, 0)) {
	err = errno;
	is_error = true;
    }
#endif // ! HOST_OS_WINDOWS

    if (is_error) {
	XLOG_ERROR("Failed to send reply (%d): %s",
		   err, comm_get_error_str(err));
    }
}

bool
XrlPFSUDPListener::response_pending() const
{
    // Responses are immediate for UDP
    return false;
}
