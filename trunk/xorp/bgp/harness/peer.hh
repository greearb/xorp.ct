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

// $XORP: xorp/bgp/harness/peer.hh,v 1.6 2003/06/20 18:55:57 hodson Exp $

#ifndef __BGP_HARNESS_PEER_HH__
#define __BGP_HARNESS_PEER_HH__

#include "trie.hh"
#include "libxorp/callback.hh"

class EventLoop;
class TimeVal;

class Peer {
public:

    Peer(EventLoop&    eventloop,
	 IPv4	       finder_address,
	 uint16_t      finder_port,
	 const string& coordinator_name,
	 const string& peer_name,
	 const string& target_hostname,
	 const string& target_port);

    ~Peer();
    
    void status(string& status);

    bool pending();

    void listen(const string& line, const vector<string>& words)
	throw(InvalidString);

    void connect(const string& line, const vector<string>& words)
	throw(InvalidString);

    void disconnect(const string& line, const vector<string>& words)
	throw(InvalidString);

    void establish(const string& line, const vector<string>& words)
	throw(InvalidString);

    void send(const string& line, const vector<string>& words)
	throw(InvalidString);

    void send_packet(const string& line, const vector<string>& words)
	throw(InvalidString);

    void send_dump(const string& line, const vector<string>& words)
	throw(InvalidString);

    void trie(const string& line, const vector<string>& words)
	throw(InvalidString);

    void expect(const string& line, const vector<string>& words)
	throw(InvalidString);

    void assertX(const string& line, const vector<string>& words)
	throw(InvalidString);

    void dump(const string& line, const vector<string>& words)
	throw(InvalidString);

    void check_expect(BGPPacket *rec);

    void xrl_callback(const XrlError& error, const char *comment);

    void datain(const bool& status, const TimeVal& tv,
		const vector<uint8_t>&  data);
    void datain_error(const string& reason);
    void datain_closed();

    const BGPPacket *packet(const string& line, const vector<string>& words,
			    int index)
	const throw(InvalidString);
protected:
    typedef XorpCallback1<void, const XrlError&>::RefPtr SMCB;
    SMCB _smcb;
    void send_message(const uint8_t *buf, const size_t len, SMCB cb);
    void send_dump_callback(const XrlError& error, FILE *fp,
			    const char *comment);
    void send_open();

private:
    // Not implemented
    Peer(const Peer&);
    Peer& operator=(const Peer&);
    
private:
    EventLoop&   _eventloop;
    XrlStdRouter _xrlrouter;

    string _coordinator;
    string _peername;
    string _target_hostname;
    string _target_port;

    bool _session;	// We are attempting to form a BGP session.
    bool _passive;	// We are passively trying to create a session.
    bool _keepalive;	// If true echo all received keepalives.
    bool _established;	// True if we believe a session has been formed.

    AsNum _as;		// Our AS number.
    int _holdtime;	// The holdtime sent in the BGP open message.
    IPv4 _id;		// The ID sent in the open message.

    Trie _trie_recv;	// Update messages received.
    Trie _trie_sent;	// Update messages sent.

    typedef XorpCallback3<void, const uint8_t*, size_t, TimeVal>::RefPtr Dumper;
    Dumper _traffic_recv;
    Dumper _traffic_sent;
    
    struct expect {
	expect() : _ok(true), _bad(0) {
	}
	~expect() {
	    list<const BGPPacket *>::iterator i;
	    for(i = _list.begin(); i != _list.end(); i++)
		delete *i;
	    delete _bad;
	}
	
	list<const BGPPacket *> _list; 	// Messages that we are expecting.
	bool _ok;			// True while expected messages arrive.
	const BGPPacket *_bad;	       	// The mismatched packet.
    } _expect;
};

#endif // __BGP_HARNESS_PEER_HH__
