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

// $XORP: xorp/bgp/harness/coord.hh,v 1.14 2008/10/02 21:56:26 bms Exp $

#ifndef __BGP_HARNESS_COORD_HH__
#define __BGP_HARNESS_COORD_HH__

#include "xrl/targets/coord_base.hh"
#include "command.hh"

class Coord {
public:
    Coord(EventLoop& eventloop, Command& command);
    void command(const string& command);
    void status(const string&	peer, string& status);
    bool pending();
    void datain(const string&  peer, const uint32_t& genid,
		const bool& status, const uint32_t& secs,
		const uint32_t& micro, const vector<uint8_t>&  data);
    void datain_error(const string&  peer, const uint32_t& genid,
		      const string& reason);
    void datain_closed(const string&  peer, const uint32_t& genid);
    bool done();
    void mark_done();

private:
    bool _done;
    EventLoop& _eventloop;
    Command& _command;
};

class XrlCoordTarget : XrlCoordTargetBase {
public:
    XrlCoordTarget(XrlRouter *r, Coord& coord);

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

    XrlCmdError common_0_1_startup() { return XrlCmdError::OKAY(); }

    /**
     * shutdown target
     */
    XrlCmdError common_0_1_shutdown();

    XrlCmdError coord_0_1_command(
	// Input values, 
        const string&	command);

    XrlCmdError coord_0_1_status(
	// Input values, 
	const string&	peer, 
	// Output values, 
	string&	status);

    XrlCmdError coord_0_1_pending(
	// Output values, 
	bool&	pending);

    XrlCmdError datain_0_1_receive(
	// Input values, 
	const string&	peer, 
	const uint32_t&	genid, 
	const bool&	status, 
	const uint32_t&	secs, 
	const uint32_t&	micro, 
	const vector<uint8_t>&	data);

    XrlCmdError datain_0_1_error(
	// Input values, 
	const string&	peer, 
	const uint32_t&	genid, 
	const string&	reason);

    XrlCmdError datain_0_1_closed(
	// Input values, 
    const string&	peer,
    const uint32_t&	genid);
private:
    Coord& _coord;
    int _incommand;
};

#endif // __BGP_HARNESS_COORD_HH__
