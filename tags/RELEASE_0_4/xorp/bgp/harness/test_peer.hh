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

// $XORP: xorp/bgp/harness/test_peer.hh,v 1.8 2003/06/26 19:41:48 atanu Exp $

#ifndef __BGP_HARNESS_TEST_PEER_HH__
#define __BGP_HARNESS_TEST_PEER_HH__

#include "xrl/targets/test_peer_base.hh"
#include "bgp/packet.hh"
#include <queue>

class TestPeer {
public:
    TestPeer(EventLoop& eventloop, XrlRouter& xrlrouter, const char *server,
	     bool verbose);
    ~TestPeer();
    bool done();
    void register_coordinator(const string& name);
    void register_genid(const uint32_t& genid);
    bool packetisation(const string& protocol);
    bool connect(const string& host, const uint32_t& port, 
		 string& error_string);
    bool listen(const string& host, const uint32_t& port, 
		 string& error_string);
    bool send(const vector<uint8_t>& data, string& error_string);
    void send_complete(AsyncFileWriter::Event ev, const uint8_t *buf,
		       const size_t buf_bytes, const size_t offset);
    bool zap(int &fd, const char *msg);
    bool disconnect(string& error_string);
    void reset();
    bool terminate(string& error_string);

protected:
    void connect_attempt(int fd, SelectorMask m);
    void receive(int fd, SelectorMask m);
    enum status {
	GOOD,
	CLOSED,
	ERROR
    };
    void datain(status st, uint8_t *ptr, size_t len, string error = "");
    void queue_data(status st, uint8_t *ptr, size_t len, string error);
    void sendit();
    void xrl_callback(const XrlError& error);

private:
    TestPeer(const TestPeer&);
    TestPeer& operator=(const TestPeer&);

private:
    EventLoop& _eventloop;
    XrlRouter& _xrlrouter;
    const char *_server;
    const bool _verbose;

    static const int UNCONNECTED = -1;
    bool _done;
    int _s;
    AsyncFileWriter *_async_writer;
    int _listen;
    string _coordinator;
    uint32_t _genid;

    bool _bgp;	// Perform BGP packetisation

    struct Queued {
	uint32_t secs;
	uint32_t micro;
	status st;
	size_t len;
	vector<uint8_t> v;
	string error;
    };
    queue <Queued> _xrl_queue;
    static const int FLYING_LIMIT = 100;// XRL's allowed in flight at
					 // one time.
    int _flying;

    size_t _bgp_bytes;
    uint8_t _bgp_buf[MAXPACKETSIZE]; // Maximum allowed BGP message
};

class XrlTestPeerTarget : XrlTestPeerTargetBase {
public:
    XrlTestPeerTarget(XrlRouter *r, TestPeer& test_peer, bool trace);

    /**
     *  Get name of Xrl Target
     */
    XrlCmdError common_0_1_get_target_name(
	// Output values, 
	string&	name);

    /**
     *  Get version string from Xrl Target
     */
    XrlCmdError common_0_1_get_version(
	// Output values, 
	string&	version);

    /**
     *  Get status from Xrl Target
     */
    XrlCmdError common_0_1_get_status(
				      // Output values,
				      uint32_t& status,
				      string&	reason);

    /**
     * shutdown target
     */
    XrlCmdError common_0_1_shutdown();

    /**
     *  Register for receiving packets and events.
     */
    XrlCmdError test_peer_0_1_register(
	// Input values, 
       const string&	coordinator,
       const uint32_t&	genid);

    /**
     *  Packetisation style.
     */
    XrlCmdError test_peer_0_1_packetisation(
	// Input values, 
	const string&	protocol);

    /**
     *  Make a tcp connection to the specified host and port.
     *  
     *  @param host name.
     *  
     *  @param port number.
     */
    XrlCmdError test_peer_0_1_connect(
	// Input values, 
	const string&	host, 
	const uint32_t&	port);

    /**
     *  Accept connections from the specified host on the specified port.
     *  
     *  @param host name.
     *  
     *  @param port number.
     */
    XrlCmdError test_peer_0_1_listen(
	// Input values, 
	const string&	address, 
	const uint32_t&	port);

    /**
     *  Send data to the peer.
     */
    XrlCmdError test_peer_0_1_send(
	// Input values, 
	const vector<uint8_t>&	data);

    /**
     *  Disconnect from the peer.
     */
    XrlCmdError test_peer_0_1_disconnect();

    /**
     *  Reset the peer.
     */
    XrlCmdError test_peer_0_1_reset();

    /**
     *  Terminate the test peer process.
     */
    XrlCmdError test_peer_0_1_terminate();

private:
    TestPeer& _test_peer;
    bool _trace;
};


#endif // __BGP_HARNESS_TEST_PEER_HH__
