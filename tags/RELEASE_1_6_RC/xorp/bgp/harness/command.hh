// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/bgp/harness/command.hh,v 1.17 2008/07/23 05:09:43 pavlin Exp $

#ifndef __BGP_HARNESS_COMMAND_HH__
#define __BGP_HARNESS_COMMAND_HH__

#include "libxorp/ref_ptr.hh"
#include "libxorp/tokenize.hh"
#include "peer.hh"

class TimeVal;

/*
** All the commands to the coordinating process come via this class.
** The commands are delivered either via XRL's or from a file.
*/

class Command {
public:
    Command(EventLoop& eventloop, XrlStdRouter& xrlrouter);

    /*
    ** Load command map.
    */
    void load_command_map();

    /*
    ** Accept a command for the coordinating process.
    */
    void command(const string& line) throw(InvalidString);

    /*
    ** Show the status of the peers.
    */
    void status(const string& peer, string& status);

    /*
    ** Test to see if there are any uncompleted commands.
    */
    bool pending();

    /*
    ** Data from the test peers.
    */
    void datain(const string&  peer,  const uint32_t& genid,
		const bool& status, const TimeVal& tv,
		const vector<uint8_t>&  data);
    void datain_error(const string&  peer,  const uint32_t& genid,
		      const string& reason);
    void datain_closed(const string&  peer,  const uint32_t& genid);
   
    /*
    ** All commands to peers sent through here.
    */
    void peer(const string& line, const vector<string>& v) 
	throw(InvalidString);

    void reset(const string& line, const vector<string>& v);
    void target(const string& line, const vector<string>& v) 
	throw(InvalidString);
    void initialise(const string& line, const vector<string>& v) 
	throw(InvalidString);
    void initialise_callback(const XrlError& error, string peername);
private:
    EventLoop& _eventloop;
    XrlStdRouter& _xrlrouter;
    uint32_t _genid;

    uint32_t _init_count;	// Number of initialisations with
				// test_peers currently in progress.

    /*
    ** Supported commands.
    */
    typedef void (Command::* MEMFUN)(const string& line,
				     const vector<string>& v);

    struct PCmd {
	PCmd(const Command::MEMFUN& mem_fun) : _mem_fun(mem_fun)
	{}

	void dispatch(Command* c, const string& line, const vector<string>&v)
	{
	    (c->*_mem_fun)(line, v);
	}
	Command::MEMFUN _mem_fun;
    };

    typedef map<const string, PCmd> StringCommandMap;
    StringCommandMap _commands;

    typedef list<Peer> PeerList;
    PeerList _peers;

    string _target_hostname;
    string _target_port;
};

#endif // __BGP_HARNESS_COMMAND_HH__
