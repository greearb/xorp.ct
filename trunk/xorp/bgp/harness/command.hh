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

// $XORP: xorp/bgp/harness/command.hh,v 1.7 2002/12/09 18:28:52 hodson Exp $

#ifndef __BGP_HARNESS_COMMAND_HH__
#define __BGP_HARNESS_COMMAND_HH__

#include "peer.hh"

/*
** All the commands to the coordinating process come via this class.
** The commands are delivered either via XRL's or from a file.
*/

class Command {
public:
    Command(XrlRouter& xrlrouter);

    /*
    ** Load command map.
    */
    void load_command_map();

    /*
    ** Accept a command for the coordinating process.
    */
    void command(const string& line) throw(InvalidString);

    /*
    ** Test to see if there are any uncompleted commands.
    */
    bool pending();

    /*
    ** Data from the test peers.
    */
    void datain(const string&  peer, const bool& status, const timeval& tv,
		const vector<uint8_t>&  data);
    void datain_error(const string&  peer, const string& reason);
    void datain_closed(const string&  peer);
   
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
    void initialise_callback(const XrlError& error, const string peername);
private:
    XrlRouter& _xrlrouter;

    /*
    ** Supported commands.
    */
    typedef void (Command::* PMEM)(const string& line,
				   const vector<string>& v);
    map<const string, PMEM> _commands;
    map<const string, Peer> _peers;

    string _target_hostname;
    string _target_port;
};

#endif // __BGP_HARNESS_COMMAND_HH__
