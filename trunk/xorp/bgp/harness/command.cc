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

#ident "$XORP: xorp/bgp/harness/command.cc,v 1.14 2003/08/05 22:14:45 atanu Exp $"

// #define DEBUG_LOGGING
#define DEBUG_PRINT_FUNCTION_NAME

#include "config.h"
#include "bgp/bgp_module.h"

#include "libxorp/debug.h"
#include "libxorp/xlog.h"

#include "libxorp/eventloop.hh"
#include "libxorp/callback.hh"
#include "libxipc/xrl_std_router.hh"
#include "xrl/interfaces/test_peer_xif.hh"
#include "coord.hh"
#include "command.hh"

Command::Command(EventLoop& eventloop, XrlStdRouter& xrlrouter)
    : _eventloop(eventloop),
      _xrlrouter(xrlrouter),
      _genid(0),
      _init_count(0)
    
{
    load_command_map();
}

void
Command::load_command_map()
{
    _commands.clear();

    _commands.insert(StringCommandMap::value_type("reset", &Command::reset));
    _commands.insert(StringCommandMap::value_type("target", &Command::target));
    _commands.insert(StringCommandMap::value_type("initialise",
						  &Command::initialise));
}

void
Command::command(const string& line) throw(InvalidString)
{
    debug_msg("command: %s\n", line.c_str());

    /*
    ** Split the line into words split on spaces.
    */
    vector<string> v;
    tokenize(line, v);

    if(v.empty())
	xorp_throw(InvalidString, "Empty command");

    StringCommandMap::iterator cur  = _commands.find(v[0].c_str());
   
    if(_commands.end() == cur)
	xorp_throw(InvalidString, c_format("Unknown command: %s",
					   v[0].c_str()));

    cur->second.dispatch(this, line, v);
}

void
Command::status(const string& peer, string& status)
{
    debug_msg("status: %s\n", peer.c_str());

    /*
    ** Are we in the peer table.
    */
    PeerList::iterator cur = _peers.begin();
    while(cur != _peers.end()) {
	switch(cur->is_this_you(peer)) {
	case Peer::YES_ITS_ME:
	    cur->status(status);
	    return;
	    break;
	case Peer::NO_ITS_NOT_ME:
	    break;
	case Peer::PLEASE_DELETE_ME:
	    _peers.erase(cur++);
	    continue;
	    break;
	}
	++cur;
    }
}

bool
Command::pending()
{
    debug_msg("pending\n");

//     if(_xrlrouter.pending())
// 	return true;

    /*
    ** Pending count is non zero when we are attempting to configure a
    ** new peer. In which case "_xrlrouter.pending()", must return
    ** true.
    */
    //    XLOG_ASSERT(_pending_count == 0);
    if(_init_count != 0) {
	XLOG_WARNING("_xrlrouter.pending() is false but _init_count is %d",
		     _init_count);
	return true;
    }

    for(PeerList::iterator cur = _peers.begin(); cur != _peers.end(); ++cur){
	if(cur->up() && cur->pending())
	    return true;
    }

    return false;
}

/*
** This is data that has been sent by the BGP process to the test_peer
** then to the coordinating process. Find which peer this data is
** destined for.
*/
void
Command::datain(const string&  peer,  const uint32_t& genid,
		const bool& status, const TimeVal& tv,
		const vector<uint8_t>&  data)
{
    debug_msg("peer: %s genid: %u status: %d secs: %lu micro: %lu data length: %u\n",
	      peer.c_str(), genid, status,
	      (unsigned long)tv.sec(), (unsigned long)tv.usec(),
	      (uint32_t)data.size());

    /*
    ** Are we in the peer table.
    */
    PeerList::iterator cur = _peers.begin();
    while(cur != _peers.end()) {
	switch(cur->is_this_you(peer, genid)) {
	case Peer::YES_ITS_ME:
	    cur->datain(status, tv, data);
	    return;
	    break;
	case Peer::NO_ITS_NOT_ME:
	    break;
	case Peer::PLEASE_DELETE_ME:
	    _peers.erase(cur++);
	    continue;
	    break;
	}
	++cur;
    }

    XLOG_WARNING("Data for a peer <%s,%u> that is not in our table",
		 peer.c_str(), genid);
}

void 
Command::datain_error(const string&  peer, const uint32_t& genid,
		      const string& reason)
{
    debug_msg("peer: %s genid: %u reason: %s\n", peer.c_str(), genid,
	      reason.c_str());
    /*
    ** Are we in the peer table.
    */
    PeerList::iterator cur = _peers.begin();
    while(cur != _peers.end()) {
	switch(cur->is_this_you(peer, genid)) {
	case Peer::YES_ITS_ME:
	    cur->datain_error(reason);
	    return;
	    break;
	case Peer::NO_ITS_NOT_ME:
	    break;
	case Peer::PLEASE_DELETE_ME:
	    _peers.erase(cur++);
	    continue;
	    break;
	}
	++cur;
    }

    XLOG_WARNING("Data for a peer <%s,%u> that is not in our table",
		 peer.c_str(), genid);
}

void
Command::datain_closed(const string&  peer, const uint32_t& genid)
{
    debug_msg("peer: %s genid: %u \n", peer.c_str(), genid);
    /*
    ** Are we in the peer table.
    */
    PeerList::iterator cur = _peers.begin();
    while(cur != _peers.end()) {
	switch(cur->is_this_you(peer, genid)) {
	case Peer::YES_ITS_ME:
	    cur->datain_closed();
	    return;
	    break;
	case Peer::NO_ITS_NOT_ME:
	    break;
	case Peer::PLEASE_DELETE_ME:
	    _peers.erase(cur++);
	    continue;
	    break;
	}
	++cur;
    }

    XLOG_WARNING("Data for a peer <%s,%u> that is not in our table",
		 peer.c_str(), genid);
}

/*
** All commands to peers are channelled through here.
*/
void
Command::peer(const string& line, const vector<string>& words)
    throw(InvalidString)
{
    debug_msg("peer: %s\n", line.c_str());

    if(1 == words.size())
	xorp_throw(InvalidString, 
		   c_format("Insufficient arguments: %s",  line.c_str()));

    /*
    ** This pointer must be valid. If we are in the command table
    ** there must be an entry in the peer table.
    */
    PeerList::iterator p = _peers.begin();
    for(p = _peers.begin(); p != _peers.end(); ++p){
	if(Peer::YES_ITS_ME == p->is_this_you(words[0]))
	    break;
    }
    
    XLOG_ASSERT(_peers.end() != p);

    const string command = words[1];
    if("connect" == command) {
	p->connect(line, words);
    } else if("disconnect" == command) {
	p->disconnect(line, words);
    } else if("establish" == command) {
	p->establish(line, words);
    } else if ("send" == command) {
	p->send(line, words);
    } else if ("trie" == command) {
	p->trie(line, words);
    } else if ("expect" == command) {
	p->expect(line, words);
    } else if ("assert" == command) {
	p->assertX(line, words);
    } else if ("dump" == command) {
	p->dump(line, words);
    } else {
	xorp_throw(InvalidString, 
		   c_format("Unrecognized command: %s",  command.c_str()));
    }
}

/*
** This command takes no arguments:
** reset.
**
** Reset all the state in the coordinating process. All scripts should
** start with this command.
*/
void
Command::reset(const string& /*line*/, const vector<string>& /*v*/)
{
    debug_msg("reset:\n");

    load_command_map();

    _target_hostname = "";
    _target_port = "";

    /*
    ** Shutdown all the peers that are currently running.
    */
    for(PeerList::iterator cur = _peers.begin(); cur != _peers.end(); ++cur){
	if(cur->up())
	    cur->shutdown();
    }

    _init_count = 0;
}

/*
** This command takes two arguments:
** target hostname port.
**
** The host and port number of the BGP process that is being tested.
*/
void
Command::target(const string& line, const vector<string>& v)
    throw(InvalidString)
{
    debug_msg("target: %s\n", line.c_str());

    if(3 != v.size())
	xorp_throw(InvalidString, 
		   c_format("\"target hostname port\" expected got \"%s\"", 
			    line.c_str()));

    _target_hostname = v[1];
    _target_port = v[2];
}

/*
** This command takes two arguments:
** initialise attach/create peername
**
** Attach to or create test peers.
*/
void
Command::initialise(const string& line, const vector<string>& v)
    throw(InvalidString)
{
    debug_msg("target: %s\n", line.c_str());

    if(3 != v.size())
	xorp_throw(InvalidString, 
		   c_format("\"initialise attach/create peer\" expected got \"%s\"", 
			    line.c_str()));

    
    const char *peername = v[2].c_str();

    /*
    ** Make sure that this peer doesn't already exist.
    */


    PeerList::iterator cur = _peers.begin();
    for(; cur != _peers.end(); ++cur)
	if(Peer::YES_ITS_ME == cur->is_this_you(peername))
	    break;

    if(cur != _peers.end())
	 debug_msg("Peer name %s\n", peername);

    if(_peers.end() != cur)
	xorp_throw(InvalidString, c_format("This peer already exists: %s",
					   peername));

    /*
    ** This peer name will be added to the command name map, verify
    ** that we don't already have a command with this name.
    */
    StringCommandMap::iterator com  = _commands.find(peername);
    if(_commands.end() != com)
	xorp_throw(InvalidString, c_format("Peername command clash: %s",
					   peername));

    if(v[1] == "attach") {
	/* FINE */
    } else if(v[1] == "create") {
	XLOG_FATAL("initialise create not currently supported");
    } else
	xorp_throw(InvalidString,
		   c_format("Only attach/create allowed not: %s",
					   v[3].c_str()));

    /*
    ** If we got here we are attaching.
    */
    
    /* Attach to and reset the peer. */
    debug_msg("About to attach\n");
    XrlTestPeerV0p1Client test_peer(&_xrlrouter);
    test_peer.send_reset(peername,
			 callback(this, &Command::initialise_callback, 
				  string(peername)));

    _init_count++;
}

void
Command::initialise_callback(const XrlError& error, string peername)
{
    debug_msg("callback: %s\n", peername.c_str());
    _init_count--;

    if(XrlError::OKAY() != error) {
	XLOG_ERROR("%s: Failed to initialise peer %s", error.str().c_str(),
		   peername.c_str());
	return;
    }

    /* Add to the peer structure */
    _peers.push_back(Peer(&_eventloop, &_xrlrouter, peername, _genid++,
		       _target_hostname, _target_port));

    /* Add to the command structure */
    _commands.insert(StringCommandMap::value_type(peername, &Command::peer));
}
