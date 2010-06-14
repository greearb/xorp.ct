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

// $XORP: xorp/bgp/harness/test_peer.hh,v 1.20 2008/10/02 21:56:26 bms Exp $

#ifndef __BGP_HARNESS_TEST_PEER_HH__
#define __BGP_HARNESS_TEST_PEER_HH__

#include "xrl/targets/test_peer_base.hh"
#include "bgp/packet.hh"


class TestPeer {
public:
    TestPeer(EventLoop& eventloop, XrlRouter& xrlrouter, const char *server,
	     bool verbose);
    ~TestPeer();
    bool done();
    void register_coordinator(const string& name);
    void register_genid(const uint32_t& genid);
    bool packetisation(const string& protocol);
    void use_4byte_asnums(bool use);
    bool connect(const string& host, const uint32_t& port, 
		 string& error_string);
    bool listen(const string& host, const uint32_t& port,
		string& error_string);
    bool bind(const string& host, const uint32_t& port,
	      string& error_string);
    bool send(const vector<uint8_t>& data, string& error_string);
    void send_complete(AsyncFileWriter::Event ev, const uint8_t *buf,
		       const size_t buf_bytes, const size_t offset);
    bool zap(XorpFd &fd, const char *msg);
    bool disconnect(string& error_string);
    void reset();
    bool terminate(string& error_string);

protected:
    void connect_attempt(XorpFd fd, IoEventType type);
    void receive(XorpFd fd, IoEventType type);
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

    /* these are needed so we know how to decode ASnums (4-byte or 2-byte); */
    LocalData* _localdata;
    BGPPeerData* _peerdata;

    const char *_server;
    const bool _verbose;

    bool _done;
    XorpFd _s;
    XorpFd _listen;
    XorpFd _bind;
    AsyncFileWriter *_async_writer;
    string _coordinator;
    uint32_t _genid;

    bool _bgp;	// Perform BGP packetisation
    bool _use_4byte_asnums; // Assume AS numbers from peer are 4-byte.

    struct Queued {
	uint32_t secs;
	uint32_t micro;
	status st;
	size_t len;
	vector<uint8_t> v;
	string error;
    };
    queue <Queued> _xrl_queue;
    static const int FLYING_LIMIT = 100;	// XRL's allowed in flight at
						// one time.
    int _flying;

    size_t _bgp_bytes;
    uint8_t _bgp_buf[BGPPacket::MAXPACKETSIZE];	// Maximum allowed BGP message
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

    XrlCmdError common_0_1_startup() { return XrlCmdError::OKAY(); }

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
     *  Use 4byte asnums.
     */
    XrlCmdError test_peer_0_1_use_4byte_asnums(
	// Input values, 
	const bool& use);

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
     *  Bind the port but don't perform the listen or accept.
     *
     *  @param address local address.
     *
     *  @param port local port number.
     */
    XrlCmdError test_peer_0_1_bind(
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
