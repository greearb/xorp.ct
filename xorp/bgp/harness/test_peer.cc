// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp/bgp_module.h"

#include "libxorp/xorp.h"
#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/status_codes.h"
#include "libxorp/xorpfd.hh"
#include "libxorp/eventloop.hh"

#ifdef HAVE_GETOPT_H
#include <getopt.h>
#endif

#include "libcomm/comm_api.h"

#include "libxipc/xrl_std_router.hh"

#include "xrl/interfaces/datain_xif.hh"

#include "bgp/socket.hh"

#include "bgppp.hh"
#include "test_peer.hh"


static const char SERVER[] = "test_peer";/* This servers name */
static const char SERVER_VERSION[] = "0.1";

#define	ZAP(s)	zap(s, #s)


/*
-------------------- XRL --------------------
*/

XrlTestPeerTarget::XrlTestPeerTarget(XrlRouter *r, TestPeer& test_peer,
				     bool trace)
    : XrlTestPeerTargetBase(r), _test_peer(test_peer), _trace(trace)
{
    debug_msg("\n");
}

XrlCmdError
XrlTestPeerTarget::common_0_1_get_target_name(string& name)
{
    debug_msg("\n");

    name = SERVER;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlTestPeerTarget::common_0_1_get_version(string& version)
{
    debug_msg("\n");

    version = SERVER_VERSION;
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlTestPeerTarget::common_0_1_get_status(// Output values,
					 uint32_t& status,
					 string& reason)
{
    //XXX placeholder only
    status = PROC_READY;
    reason = "Ready";
    return XrlCmdError::OKAY();
}

XrlCmdError
XrlTestPeerTarget::common_0_1_shutdown()
{
    string error_string;
    if(!_test_peer.terminate(error_string)) {
	return XrlCmdError::COMMAND_FAILED(error_string);
    }

    return XrlCmdError::OKAY();
}


XrlCmdError
XrlTestPeerTarget::test_peer_0_1_register(const string& coordinator,
					  const uint32_t& genid)
{
    debug_msg("\n");

    if(_trace)
	printf("register(%s,%u)\n", coordinator.c_str(),
	       XORP_UINT_CAST(genid));

    _test_peer.register_coordinator(coordinator);
    _test_peer.register_genid(genid);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlTestPeerTarget::test_peer_0_1_packetisation(const string& protocol)
{
    debug_msg("\n");

    if(_trace)
	printf("packetisation(%s)\n", protocol.c_str());

    if(!_test_peer.packetisation(protocol))
	return XrlCmdError::COMMAND_FAILED(c_format("Unsupported protocol %s",
					     protocol.c_str()));

    return XrlCmdError::OKAY();
}

/* test peer needs to know whether to assume ASPaths are 2-byte or
   4-byte encoded */

XrlCmdError
XrlTestPeerTarget::test_peer_0_1_use_4byte_asnums(const bool& use)
{
    debug_msg("\n");

    if(_trace)
	printf("use_4byte_asnums(%d)\n", use);

    _test_peer.use_4byte_asnums(use);

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlTestPeerTarget::test_peer_0_1_connect(const string&	host,
					 const uint32_t&  port)
{
    debug_msg("\n");

    if(_trace)
	printf("connect(%s,%u)\n", host.c_str(), XORP_UINT_CAST(port));

    string error_string;
    if(!_test_peer.connect(host, port, error_string)) {
	return XrlCmdError::COMMAND_FAILED(error_string);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlTestPeerTarget::test_peer_0_1_listen(const string& address,
					const uint32_t&	port)
{
    debug_msg("\n");

    if(_trace)
	printf("listen(%s,%u)\n", address.c_str(), XORP_UINT_CAST(port));

    string error_string;
    if(!_test_peer.listen(address, port, error_string)) {
	return XrlCmdError::COMMAND_FAILED(error_string);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlTestPeerTarget::test_peer_0_1_bind(const string& address,
				      const uint32_t& port)
{
    debug_msg("\n");

    if(_trace)
	printf("bind(%s,%u)\n", address.c_str(), XORP_UINT_CAST(port));

    string error_string;
    if(!_test_peer.bind(address, port, error_string)) {
	return XrlCmdError::COMMAND_FAILED(error_string);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlTestPeerTarget::test_peer_0_1_send(const vector<uint8_t>& data)
{
    debug_msg("\n");

    if(_trace && MESSAGETYPEOPEN == data[BGPPacket::COMMON_HEADER_LEN - 1]) {
	printf("send() - open\n");
    }

    string error_string;
    if(!_test_peer.send(data, error_string)) {
	return XrlCmdError::COMMAND_FAILED(error_string);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlTestPeerTarget::test_peer_0_1_disconnect()
{
    debug_msg("\n");

    if(_trace)
	printf("disconnect()\n");

    string error_string;
    if(!_test_peer.disconnect(error_string)) {
	return XrlCmdError::COMMAND_FAILED(error_string);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlTestPeerTarget::test_peer_0_1_reset()
{
    debug_msg("\n");

    if(_trace)
	printf("reset()\n");

    _test_peer.reset();

    return XrlCmdError::OKAY();
}

XrlCmdError
XrlTestPeerTarget::test_peer_0_1_terminate()
{
    debug_msg("\n");

    if(_trace)
	printf("terminate()\n");

    string error_string;
    if(!_test_peer.terminate(error_string)) {
	return XrlCmdError::COMMAND_FAILED(error_string);
    }

    return XrlCmdError::OKAY();
}

/*
-------------------- IMPLEMENTATION --------------------
*/
TestPeer::TestPeer(EventLoop& eventloop, XrlRouter& xrlrouter,
		   const char *server, bool verbose)
    : _eventloop(eventloop), _xrlrouter(xrlrouter), 
      _server(server),
      _verbose(verbose),
     _done(false),
      _async_writer(0),
      _bgp(false),
      _flying(0),
     _bgp_bytes(0)
{
    _localdata = new LocalData(eventloop);
    _localdata->set_as(AsNum(0));
    _peerdata = new BGPPeerData(*_localdata, Iptuple(), AsNum(0), IPv4(), 0);
    // we force IBGP, as this does fewer tests
    _peerdata->compute_peer_type();
}

TestPeer::~TestPeer()
{
    delete _peerdata;
    delete _localdata;
    delete _async_writer;
}

bool
TestPeer::done()
{
    return _done;
}

void
TestPeer::register_coordinator(const string& name)
{
    debug_msg("Test Peer: coordinator name = \"%s\"\n", name.c_str());

    _coordinator = name;
}

void
TestPeer::register_genid(const uint32_t& genid)
{
    debug_msg("Test Peer: genid = %u\n", XORP_UINT_CAST(genid));

    _genid = genid;
}

bool
TestPeer::packetisation(const string& protocol)
{
    debug_msg("\n");

    /*
    ** We only support "bgp".
    */
    if(protocol != "bgp") {
	return false;
    }

    _bgp = true;
    _bgp_bytes = 0;
    return true;
}

void
TestPeer::use_4byte_asnums(bool use)
{
    _localdata->set_use_4byte_asnums(use);

    // normally this is negotiated, but this is a test peer so we want
    // to be able to force the parsing of ASnums either way.
    _peerdata->set_use_4byte_asnums(use);
}

bool
TestPeer::connect(const string& host, const uint32_t& port,
		  string& error_string)
{
    debug_msg("\n");

    if (_listen.is_valid()) {
	error_string = "Peer is currently listening";
	return false;
    }

    if (_bind.is_valid()) {
	error_string = "Peer is currently bound";
	return false;
    }

    if (_s.is_valid()) {
	error_string = "Peer is already connected";
	return false;
    }

    struct sockaddr_storage peer;
    size_t len = sizeof(peer);
    try {
	Socket::init_sockaddr(host, port, peer, len);
    } catch(UnresolvableHost& e) {
	error_string = e.why();
	return false;
    }

    XorpFd s = comm_sock_open(peer.ss_family, SOCK_STREAM, 0,
			      COMM_SOCK_NONBLOCKING);
    if (!s.is_valid()) {
	error_string = c_format("comm_sock_open failed: %s\n",
				comm_get_last_error_str());
	return false;
    }

    // XXX
    int in_progress = 0;
    (void)comm_sock_connect(s, reinterpret_cast<struct sockaddr *>(&peer),
			    COMM_SOCK_NONBLOCKING, &in_progress);

   /*
   ** If there is a coordinator then add an I/O event callback.
   */
   if (0 != _coordinator.length() &&
       !_eventloop.add_ioevent_cb(s, IOT_READ,
				  callback(this, &TestPeer::receive),
				  XorpTask::PRIORITY_LOWEST)) {
	comm_sock_close(s);
	XLOG_WARNING("Failed to add socket %s to eventloop", s.str().c_str());
	return false;
    }

   /*
   ** Set up the async writer.
   */
    if (XORP_ERROR == comm_sock_set_blocking(s, COMM_SOCK_NONBLOCKING)) {
        XLOG_FATAL("Failed to go non-blocking: %s", comm_get_last_error_str());
    }
    delete _async_writer;
    _async_writer = new AsyncFileWriter(_eventloop, s, XorpTask::PRIORITY_LOWEST);

    _s = s;

    return true;
}

bool
TestPeer::listen(const string& host, const uint32_t& port,
		 string& error_string)
{
    debug_msg("\n");

    if (_listen.is_valid()) {
	error_string = "Peer is already listening";
	return false;
    }

    if (_bind.is_valid()) {
	error_string = "Peer is currently bound";
	return false;
    }

    if (_s.is_valid()) {
	error_string = "Peer is currently connected";
	return false;
    }

    struct sockaddr_storage local;
    size_t len = sizeof(local);
    try {
	Socket::init_sockaddr(host, port, local, len);
    } catch(UnresolvableHost& e) {
	error_string = e.why();
	return false;
    }

    XorpFd s = comm_bind_tcp(reinterpret_cast<struct sockaddr *>(&local),
			     COMM_SOCK_NONBLOCKING, NULL /* local-dev-name */);
    if (!s.is_valid()) {
	error_string = c_format("comm_bind_tcp() failed: %s\n",
				comm_get_last_error_str());
	return false;
    }

    if (comm_listen(s, COMM_LISTEN_DEFAULT_BACKLOG) != XORP_OK) {
	error_string = c_format("comm_listen() failed: %s\n",
				comm_get_last_error_str());
	return false;
    }

    if(!_eventloop.add_ioevent_cb(s, IOT_ACCEPT,
				  callback(this,
					   &TestPeer::connect_attempt),
				  XorpTask::PRIORITY_LOWEST)) {
	comm_sock_close(s);
	error_string = c_format("Failed to add socket %s to eventloop",
				s.str().c_str());
	return false;
    }
    _listen = s;

    return true;
}

bool
TestPeer::bind(const string& host, const uint32_t& port,
	       string& error_string)
{
    debug_msg("\n");

    if (_listen.is_valid()) {
	error_string = "Peer is currently listening";
	return false;
    }

    if (_bind.is_valid()) {
	error_string = "Peer is already bound";
	return false;
    }

    if (_s.is_valid()) {
	error_string = "Peer is currently connected";
	return false;
    }

    struct sockaddr_storage local;
    size_t len = sizeof(local);
    try {
	Socket::init_sockaddr(host, port, local, len);
    } catch(UnresolvableHost& e) {
	error_string = e.why();
	return false;
    }

    XorpFd s = comm_sock_open(local.ss_family, SOCK_STREAM, 0,
			      COMM_SOCK_NONBLOCKING);
    if (!s.is_valid()) {
	error_string = c_format("comm_sock_open failed: %s",
				comm_get_last_error_str());
	return false;
    }

    if (XORP_OK != comm_set_reuseaddr(s, 1)) {
	error_string = c_format("comm_set_reuseaddr failed: %s\n",
				comm_get_last_error_str());
	return false;
    }

    if (comm_sock_bind(s, reinterpret_cast<struct sockaddr *>(&local), NULL /* local-dev-name */)
	!= XORP_OK) {
	comm_sock_close(s);
	error_string = c_format("comm_sock_bind failed: %s",
				comm_get_last_error_str());
	return false;
    }

    _bind = s;

    return true;
}

bool
TestPeer::send(const vector<uint8_t>& data, string& error_string)
{
    debug_msg("len: %u\n", XORP_UINT_CAST(data.size()));
    if (!_s.is_valid()) {
	XLOG_WARNING("Not connected");
	error_string = "Not connected";
	return false;
    }

    size_t len = data.size();
    uint8_t *buf = new uint8_t[len];
    size_t i;
    for(i = 0; i < len; i++)
	buf[i] = data[i];
    _async_writer->add_buffer(buf, len,
			      callback(this, &TestPeer::send_complete));
    _async_writer->start();

    if(_verbose && _bgp)
	printf("Sending: %s", bgppp(buf, len, _peerdata).c_str());

    return true;
}

void
TestPeer::send_complete(AsyncFileWriter::Event ev, const uint8_t *buf,
			const size_t buf_bytes, const size_t offset)
{
    switch (ev) {
    case AsyncFileOperator::DATA:
	debug_msg("event: data\n");
	if (offset == buf_bytes)
	    delete[] buf;
	break;
    case AsyncFileOperator::FLUSHING:
	debug_msg("event: flushing\n");
	debug_msg("Freeing Buffer for sent packet: %p\n", buf);
	delete[] buf;
	break;
    case AsyncFileOperator::OS_ERROR:
	debug_msg("event: error\n");
	/* Don't free the message here we'll get it in the flush */
	XLOG_ERROR("Writing buffer failed: %s",  strerror(errno));
	break;
    case AsyncFileOperator::END_OF_FILE:
	XLOG_ERROR("End of File: %s",  strerror(errno));
	break;
    case AsyncFileOperator::WOULDBLOCK:
	// do nothing
	;
    }
}

bool
TestPeer::zap(XorpFd& fd, const char *msg)
{
    debug_msg("%s = %s\n", msg, fd.str().c_str());

    if (!fd.is_valid())
	return true;

    XorpFd tempfd = fd;
    fd.clear();

   _eventloop.remove_ioevent_cb(tempfd);
   debug_msg("Removing I/O event cb for fd = %s\n", tempfd.str().c_str());
   if (comm_sock_close(tempfd) == -1) {
	XLOG_WARNING("Close of %s failed: %s", msg, comm_get_last_error_str());
	return false;
   }

   return true;;
}

bool
TestPeer::disconnect(string& error_string)
{
    debug_msg("\n");

    if (!_s.is_valid()) {
	XLOG_WARNING("Not connected");
	error_string = "Not connected";
	return false;
    }

    delete _async_writer;
    _async_writer = 0;

    return ZAP(_s);
}


void
TestPeer::reset()
{
    debug_msg("\n");

    delete _async_writer;
    _async_writer = 0;

    ZAP(_s);
    ZAP(_listen);
    ZAP(_bind);
}

bool
TestPeer::terminate(string& /*error_string*/)
{
    debug_msg("\n");

    _done = true;

    return true;
}

/*
** Process incoming connection attempts.
*/
void
TestPeer::connect_attempt(XorpFd fd, IoEventType type)
{
    debug_msg("\n");

    if (type != IOT_ACCEPT) {
	XLOG_WARNING("Unexpected IoEventType %d", type);
	return;
    }

    if (_s.is_valid()) {
	XLOG_WARNING("Connection attempt while already connected");
	return;
    }

    XLOG_ASSERT(_listen == fd);
    _listen.clear();

    XorpFd connfd = comm_sock_accept(fd);
    debug_msg("Incoming connection attempt %s\n", connfd.str().c_str());

    /*
    ** We don't want any more connection attempts so remove ourselves
    ** from the eventloop and close the file descriptor.
    */
   _eventloop.remove_ioevent_cb(fd);
   debug_msg("Removing I/O event cb for fd = %s\n", fd.str().c_str());
   if (XORP_ERROR == comm_sock_close(fd))
	XLOG_WARNING("Close failed");

   /*
   ** If there is a coordinator then add an I/O event callback.
   */
   if(0 != _coordinator.length() &&
       !_eventloop.add_ioevent_cb(connfd, IOT_READ,
				  callback(this, &TestPeer::receive),
				  XorpTask::PRIORITY_LOWEST)) {
	comm_sock_close(connfd);
	XLOG_WARNING("Failed to add socket %s to eventloop",
			connfd.str().c_str());
	return;
    }

   /*
   ** Set up the async writer.
   */
    if (XORP_ERROR == comm_sock_set_blocking(connfd, COMM_SOCK_NONBLOCKING)) {
        XLOG_FATAL("Failed to go non-blocking: %s", comm_get_last_error_str());
    }
    delete _async_writer;
   _async_writer = new AsyncFileWriter(_eventloop, connfd, XorpTask::PRIORITY_LOWEST);

   _s = connfd;
}

/*
** Process incoming bytes
*/
void
TestPeer::receive(XorpFd fd, IoEventType type)
{
    debug_msg("\n");

    if (type != IOT_READ) {
	XLOG_WARNING("Unexpected IoEventType %d", type);
	return;
    }

    if (0 == _coordinator.length()) {
	XLOG_WARNING("No coordinator configured");
	return;
    }

    /*
    ** If requested perform BGP packetisation.
    */
    int len;
    if(!_bgp) {
	uint8_t buf[64000];
	if(-1 == (len = recv(fd, (char *)buf, sizeof(buf), 0))) {
	    string error = c_format("read error: %s", strerror(errno));
	    XLOG_WARNING("%s", error.c_str());
	    datain(ERROR, _bgp_buf, _bgp_bytes, error);
	    return;
	}
	datain(GOOD, _bgp_buf, _bgp_bytes);
	return;
    }

    /*
    ** Now doing BGP packetisation.
    */
    int get;
    if(_bgp_bytes < BGPPacket::COMMON_HEADER_LEN) {
	get = BGPPacket::COMMON_HEADER_LEN - _bgp_bytes;
    } else {
	uint16_t length = extract_16(_bgp_buf + BGPPacket::LENGTH_OFFSET);
	get = length - _bgp_bytes;
    }

    if (-1 == (len = recv(fd, (char *)&_bgp_buf[_bgp_bytes], get, 0))) {
	string error = c_format("read error: %s", strerror(errno));
	XLOG_WARNING("%s", error.c_str());
	datain(ERROR, _bgp_buf, _bgp_bytes, error);
	_bgp_bytes = 0;
	comm_sock_close(fd);
	_s.clear();
	_eventloop.remove_ioevent_cb(fd);
	return;
    }

    if (0 == len) {
	if(0 < _bgp_bytes)
	    datain(GOOD, _bgp_buf, _bgp_bytes);
	datain(CLOSED, 0, 0);
	_bgp_bytes = 0;
	comm_sock_close(fd);
	_s.clear();
	_eventloop.remove_ioevent_cb(fd);
	return;
    }

    _bgp_bytes += len;

    if (_bgp_bytes >= BGPPacket::COMMON_HEADER_LEN) {
	uint16_t length = extract_16(_bgp_buf + BGPPacket::LENGTH_OFFSET);
	if (length < BGPPacket::MINPACKETSIZE || length > sizeof(_bgp_buf)) {
	    string error = c_format("Illegal length value %d", length);
	    XLOG_ERROR("%s", error.c_str());
	    datain(ERROR, _bgp_buf, _bgp_bytes, error);
	    _bgp_bytes = 0;
	    return;
	}
	if(length == _bgp_bytes) {
	    datain(GOOD, _bgp_buf, _bgp_bytes);
	    _bgp_bytes = 0;
	}
    }
}

/*
** Send data received on the socket back to the coordinator
*/
void
TestPeer::datain(status st, uint8_t *ptr, size_t len, string error)
{
    debug_msg("status = %d len = %u error = %s\n", st, XORP_UINT_CAST(len),
	      error.c_str());

    if (_verbose) {
	switch(st) {
	case GOOD:
	    if(_bgp)
		printf("Received: %s", bgppp(ptr, len, _peerdata).c_str());
	    break;
	case CLOSED:
	    printf("Connection closed by peer\n");
	    break;
	case ERROR:
	    printf("Error on connection\n");
	    break;
	}
    }

    queue_data(st, ptr, len, error);
}

/*
** It is possible to overrun the XRL code by sending too many
** XRL's. Therefore queue the data that needs to be sent and only have
** a small number of outstanding XRL's at any time.
*/
void
TestPeer::queue_data(status st, uint8_t *ptr, size_t len, string error)
{
    Queued q;
    TimeVal tv;

    _eventloop.current_time(tv);
    q.secs = tv.sec();
    q.micro = tv.usec();
    q.st = st;
    q.len = len;
    for(size_t i = 0; i < len; i++)
	q.v.push_back(ptr[i]);
    q.error = error;
    _xrl_queue.push(q);

    sendit();
}

/*
** If there is any queued data and less than the allowed quota is in
** flight send some more.
*/
void
TestPeer::sendit()
{
    if(_flying >= FLYING_LIMIT)
	return;

    if(_xrl_queue.empty())
	return;

    Queued q = _xrl_queue.front();
    _xrl_queue.pop();

    XrlDatainV0p1Client datain(&_xrlrouter);

    debug_msg("%u\n", XORP_UINT_CAST(q.v.size()));
    XLOG_ASSERT(q.len == q.v.size());

    _flying++;

    if (q.len == 0) {
	//
	// XXX: Note that we don't consider it an error even if
	// "q.st == ERROR".
	// This is needed to preserve the original logic that considers
	// the case of a peer resetting immediately the TCP
	// connection not an error.
	//
	datain.send_closed(_coordinator.c_str(), _server, _genid,
			   callback(this, &TestPeer::xrl_callback));
	return;
    }

    if (q.st == ERROR) {
	datain.send_error(_coordinator.c_str(), _server, _genid, q.error,
			  callback(this, &TestPeer::xrl_callback));
	return;
    }

    datain.send_receive(_coordinator.c_str(), _server, _genid,
			GOOD == q.st ? true : false,
			q.secs, q.micro,
			q.v,
			callback(this, &TestPeer::xrl_callback));
}

void
TestPeer::xrl_callback(const XrlError& error)
{
    debug_msg("callback %s\n", error.str().c_str());

    if(XrlError::OKAY() != error) {
	if(XrlError::REPLY_TIMED_OUT() == error)
	    XLOG_ERROR("%s: Try reducing FLYING_LIMIT",  error.str().c_str());
	XLOG_ERROR("%s",  error.str().c_str());
    }
    _flying--;
    XLOG_ASSERT(_flying >= 0);
    sendit();
}

static void
usage(const char *name)
{
    fprintf(stderr,
	    "usage: %s [-h (finder host)] [-s server name] [-v][-t]\n", name);
    exit(-1);
}

int
main(int argc, char **argv)
{
    XorpUnexpectedHandler x(xorp_unexpected_handler);
    //
    // Initialize and start xlog
    //
    xlog_init(argv[0], NULL);
    xlog_set_verbose(XLOG_VERBOSE_HIGH);
    // XXX: verbosity of the error messages temporary increased
    xlog_level_set_verbose(XLOG_LEVEL_ERROR, XLOG_VERBOSE_HIGH);
    xlog_add_default_output();
    xlog_start();

    setvbuf(stdout, (char *)NULL, _IOLBF, 0);

    int c;
    string finder_host = FinderConstants::FINDER_DEFAULT_HOST().str();
    const char *server = SERVER;
    bool verbose = false;
    bool trace = false;

    while((c = getopt (argc, argv, "h:s:vt")) != EOF) {
	switch(c) {
	case 'h':
	    finder_host = optarg;
	    break;
	case 's':
	    server = optarg;
	    break;
	case 'v':
	    verbose = true;
	    break;
	case 't':
	    trace = true;
	    break;
	case '?':
	    usage(argv[0]);
	}
    }

    try {
	EventLoop eventloop;
	XrlStdRouter router(eventloop, server, finder_host.c_str());
	TestPeer test_peer(eventloop, router, server, verbose);
	XrlTestPeerTarget xrl_target(&router, test_peer, trace);

	wait_until_xrl_router_is_ready(eventloop, router);

	while(!test_peer.done()) {
	    eventloop.run();
	}

    } catch(...) {
	xorp_catch_standard_exceptions();
    }

    //
    // Gracefully stop and exit xlog
    //
    xlog_stop();
    xlog_exit();
    debug_msg("Bye!\n");
    return 0;
}
