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

#ident "$XORP: xorp/bgp/harness/test_peer.cc,v 1.12 2003/06/19 00:46:09 hodson Exp $"

// #define DEBUG_LOGGING 
#define DEBUG_PRINT_FUNCTION_NAME 

#include "bgp/bgp_module.h"
#include "config.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"
#include "libxorp/status_codes.h"

#include "libxorp/eventloop.hh"
#include "libxipc/xrl_std_router.hh"
#include "bgp/socket.hh"

#include "xrl/interfaces/datain_xif.hh"
#include "bgppp.hh"
#include "test_peer.hh"

static const char SERVER[] = "test_peer";/* This servers name */
static const char SERVER_VERSION[] = "0.1";

#define	ZAP(s)	zap(s, #s)


/*
-------------------- XRL --------------------
*/

XrlTestPeerTarget::XrlTestPeerTarget(XrlRouter *r, TestPeer& test_peer)
    : XrlTestPeerTargetBase(r), _test_peer(test_peer)
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
XrlTestPeerTarget::test_peer_0_1_register(const string& coordinator)
{
    debug_msg("\n");

    _test_peer.register_coordinator(coordinator);

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlTestPeerTarget::test_peer_0_1_packetisation(const string& protocol)
{
    debug_msg("\n");

    if(!_test_peer.packetisation(protocol)) 
	return XrlCmdError::COMMAND_FAILED(c_format("Unsupported protocol %s",
					     protocol.c_str()));

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlTestPeerTarget::test_peer_0_1_connect(const string&	host,
					 const uint32_t&  port)
{
    debug_msg("\n");

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

    string error_string;
    if(!_test_peer.listen(address, port, error_string)) {
	return XrlCmdError::COMMAND_FAILED(error_string);
    }

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlTestPeerTarget::test_peer_0_1_send(const vector<uint8_t>& data)
{
    debug_msg("\n");

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

    _test_peer.reset();

    return XrlCmdError::OKAY();
}

XrlCmdError 
XrlTestPeerTarget::test_peer_0_1_terminate()
{
    debug_msg("\n");

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
    : _eventloop(eventloop), _xrlrouter(xrlrouter), _server(server),
      _verbose(verbose),
     _done(false),  
      _s(UNCONNECTED), _async_writer(0), _listen(UNCONNECTED), 
      _bgp(false),
      _flying(0),
     _bgp_bytes(0)
{
}

TestPeer::~TestPeer()
{
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

bool
TestPeer::connect(const string& host, const uint32_t& port,
		  string& error_string)
{
    debug_msg("\n");

    if(UNCONNECTED != _listen) {
	error_string = "Peer is currently listening";
	return false;
    }

    if(UNCONNECTED != _s) {
	error_string = "Peer is already connected";
	return false;
    }

    int s = ::socket(PF_INET, SOCK_STREAM, 0);
    if(-1 == s) {
	error_string = c_format("socket call failed: %s", strerror(errno));
	return false;
    }

    struct sockaddr_in peer;
    try {
	Socket::init_sockaddr(&peer,
			      Iptuple::get_addr(host.c_str()),
			      htons(port));
    } catch(UnresolvableHost e) {
	::close(s);
	error_string = e.why();
	return false;
    }

    if(-1 == ::connect(s, reinterpret_cast<struct sockaddr *>(&peer),
		       sizeof(peer))) {
	::close(s);
	return false;
    }

   /*
   ** If there is a coordinator then add a selector.
   */
   if(0 != _coordinator.length() &&
       !_eventloop.add_selector(s, SEL_RD,
				callback(this, &TestPeer::receive))) {
	::close(s);
	XLOG_WARNING("Failed to add socket %d to eventloop", s);
	return false;
    }
   
   /*
   ** Set up the async writer.
   */
    if(fcntl(s, F_SETFL, O_NONBLOCK) < 0) {
        XLOG_FATAL("Failed to go non-blocking: %s", strerror(errno));
    }
    delete _async_writer;
   _async_writer = new AsyncFileWriter(_eventloop, s);

    _s = s;

    return true;
}

bool
TestPeer::listen(const string& host, const uint32_t& port,
		 string& error_string)
{
    debug_msg("\n");

    if(UNCONNECTED != _listen) {
	error_string = "Peer is already listening";
	return false;
    }

    if(UNCONNECTED != _s) {
	error_string = "Peer is already connected";
	return false;
    }

    int s = ::socket(PF_INET, SOCK_STREAM, 0);
    if(-1 == s) {
	error_string = c_format("socket call failed: %s", strerror(errno));
	return false;
    }

    struct sockaddr_in local;
    try {
	Socket::init_sockaddr(&local,
			      Iptuple::get_addr(host.c_str()),
			      htons(port));
    } catch(UnresolvableHost e) {
	::close(s);
	error_string = e.why();
	return false;
    } 

    int opt = 1;
    if(-1 == ::setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
	error_string = c_format("setsockopt failed: %s\n", strerror(errno));
	return false;
    }

    if(-1 == ::bind(s, reinterpret_cast<struct sockaddr *>(&local),
		  sizeof(local))) {
	::close(s);
	error_string = c_format("Bind failed: %s", strerror(errno));
	return false;
    }
    
    if(-1 == ::listen(s, 1)) {
	::close(s);
	error_string = c_format("Bind failed: %s", strerror(errno));
	return false;
    }

    if(!_eventloop.add_selector(s, SEL_RD,
				callback(this,
					 &TestPeer::connect_attempt))) {
	::close(s);
	error_string = c_format("Failed to add socket %d to eventloop", s);
	return false;
    }
    _listen = s;
    
    return true;
}

bool
TestPeer::send(const vector<uint8_t>& data, string& error_string)
{
    debug_msg("len: %u\n", (uint32_t)data.size());
    if(UNCONNECTED == _s) {
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
	printf("Sending: %s", bgppp(buf, len).c_str());

    return true;
}

void
TestPeer::send_complete(AsyncFileWriter::Event ev, const uint8_t *buf,
			const size_t buf_bytes, const size_t offset)
{
    switch(ev) {
    case AsyncFileOperator::DATA:
	debug_msg("event: data\n");
	if (offset == buf_bytes)
	    delete[] buf;
	break;
    case AsyncFileOperator::FLUSHING:
	debug_msg("event: flushing\n");
	debug_msg("Freeing Buffer for sent packet: %x\n", (uint)buf);
	delete[] buf;
	break;
    case AsyncFileOperator::ERROR_CHECK_ERRNO:
	debug_msg("event: error\n");
	/* Don't free the message here we'll get it in the flush */
	XLOG_ERROR("Writing buffer failed: %s",  strerror(errno));
    case AsyncFileOperator::END_OF_FILE:
	XLOG_ERROR("End of File: %s",  strerror(errno));
    }
}

bool
TestPeer::zap(int& fd, const char *msg)
{
    debug_msg("%s = %d\n", msg, fd);
    if(UNCONNECTED == fd)
	return true;
    
    int tempfd = fd;
    fd = UNCONNECTED;

   _eventloop.remove_selector(tempfd);
   debug_msg("Removing selector for fd = %d\n", tempfd);
   if(-1 == ::close(tempfd)) {
	XLOG_WARNING("Close of %s failed %s", msg, strerror(errno));
	return false;
   }

   return true;;
}

bool
TestPeer::disconnect(string& error_string)
{
    debug_msg("\n");

    if(UNCONNECTED == _s) {
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
TestPeer::connect_attempt(int fd, SelectorMask m)
{
    debug_msg("\n");

    if(SEL_RD != m) {
	XLOG_WARNING("Unexpected SelectorMask %#x", m);
	return;
    }

    if(UNCONNECTED != _s) {
	XLOG_WARNING("Connection attempt while already connected");
	return;
    }

    XLOG_ASSERT(_listen == fd);
    _listen = UNCONNECTED;

    struct sockaddr_in cliaddr;
    socklen_t clilen = sizeof(cliaddr);
    int connfd = accept(fd, reinterpret_cast<struct sockaddr *>(&cliaddr),
			&clilen);

    debug_msg("Incoming connection attempt %d\n", connfd);

    /*
    ** We don't want any more connection attempts so remove ourselves
    ** from the eventloop and close the file descriptor.
    */
   _eventloop.remove_selector(fd);
   debug_msg("Removing selector for fd = %d\n", fd);
   if(-1 == ::close(fd))
	XLOG_WARNING("Close failed");

   /*
   ** If there is a coordinator then add a selector.
   */
   if(0 != _coordinator.length() &&
       !_eventloop.add_selector(connfd, SEL_RD,
				callback(this, &TestPeer::receive))) {
	::close(connfd);
	XLOG_WARNING("Failed to add socket %d to eventloop", connfd);
	return;
    }
   
   /*
   ** Set up the async writer.
   */
    if(fcntl(connfd, F_SETFL, O_NONBLOCK) < 0) {
        XLOG_FATAL("Failed to go non-blocking: %s", strerror(errno));
    }
    delete _async_writer;
   _async_writer = new AsyncFileWriter(_eventloop, connfd);

   _s = connfd;
}

/*
** Process incoming bytes
*/
void
TestPeer::receive(int fd, SelectorMask m)
{
    debug_msg("\n");

    if(SEL_RD != m) {
	XLOG_WARNING("Unexpected SelectorMask %#x", m);
	return;
    }

    if(0 == _coordinator.length()) {
	XLOG_WARNING("No coordinator configured");
	return;
    }

    /*
    ** If requested perform BGP packetisation.
    */
    int len;
    if(!_bgp) {
	uint8_t buf[64000];
	if(-1 == (len = read(fd, buf, sizeof(buf)))) {
	    string error = c_format("read error: %s", strerror(errno));
	    XLOG_WARNING(error.c_str());
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
    if(_bgp_bytes < BGP_COMMON_HEADER_LEN) {
	get = BGP_COMMON_HEADER_LEN - _bgp_bytes;
    } else {
	const fixed_header *header =
	    reinterpret_cast<const struct fixed_header *>(_bgp_buf);
	u_short length = ntohs(header->length);
	get = length - _bgp_bytes;
    }

    if(-1 == (len = read(fd, &_bgp_buf[_bgp_bytes], get))) {
	string error = c_format("read error: %s", strerror(errno));
	XLOG_WARNING(error.c_str());
	datain(ERROR, _bgp_buf, _bgp_bytes, error);
	_bgp_bytes = 0;
	::close(fd);
	_s = UNCONNECTED;
	_eventloop.remove_selector(fd);
	return;
    }

    if(0 == len) {
	if(0 < _bgp_bytes)
	    datain(GOOD, _bgp_buf, _bgp_bytes);
	datain(CLOSED, 0, 0);
	_bgp_bytes = 0;
	::close(fd);
	_s = UNCONNECTED;
	_eventloop.remove_selector(fd);
	return;
    }
    
    _bgp_bytes += len;

    if(_bgp_bytes >= BGP_COMMON_HEADER_LEN) {
	const fixed_header *header =
	    reinterpret_cast<const struct fixed_header *>(_bgp_buf);
	u_short length = ntohs(header->length);
	if(length < MINPACKETSIZE || length > sizeof(_bgp_buf)) {
	    string error = c_format("Illegal length value %d", length);
	    XLOG_ERROR(error.c_str());
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
    debug_msg("status = %d len = %u error = %s\n", st, (uint32_t)len,
	      error.c_str());

    if(_verbose) {
	switch(st) {
	case GOOD:
	    if(_bgp)
		printf("Received: %s", bgppp(ptr, len).c_str());
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
    
    debug_msg("%u\n", (uint32_t)q.v.size());
    XLOG_ASSERT(q.len == q.v.size());

    _flying++;

    switch(q.len) {
    case 0:
	datain.send_closed(_coordinator.c_str(), _server,
			   callback(this, &TestPeer::xrl_callback));
	break;
    case -1:
	datain.send_error(_coordinator.c_str(), _server, q.error,
			  callback(this, &TestPeer::xrl_callback));
	break;
    default:
	datain.send_receive(_coordinator.c_str(), _server,
			    GOOD == q.st ? true : false,
			    q.secs, q.micro,
			    q.v,
			    callback(this, &TestPeer::xrl_callback));
	break;
    }
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

void
usage(char *name)
{
    fprintf(stderr,
	    "usage: %s [-h (finder host)] [-s server name] [-v]\n", name);
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

    int c;
    const char *finder_host = "localhost";
    const char *server = SERVER;
    bool verbose = false;

    while((c = getopt (argc, argv, "h:s:v")) != EOF) {
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
	case '?':
	    usage(argv[0]);
	}
    }
   
    try {
	EventLoop eventloop;
	XrlStdRouter router(eventloop, server, finder_host);
	TestPeer test_peer(eventloop, router, server, verbose);
	XrlTestPeerTarget xrl_target(&router, test_peer);

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
