// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/ospf/trace.hh,v 1.21 2008/10/02 21:57:50 bms Exp $

#ifndef __OSPF_TRACE_HH__
#define __OSPF_TRACE_HH__

/**
 * All the trace variables in one place.
 */
struct Trace {
    Trace() : _input_errors(true),
	      _interface_events(false),
	      _neighbour_events(false),
	      _spt(false),
	      _import_policy(false),
	      _export_policy(false),
	      _virtual_link(false),
	      _find_interface_address(false),
	      _routes(false),
	      _retransmit(false),
	      _election(false),
	      _packets(false)
	      // Don't forget to add new variables to the all() method.
	{}

    /*
     * Set all flags
     */
    void all(bool val) {
	_input_errors = _interface_events = _neighbour_events = _spt = 
	    _import_policy = _export_policy = _virtual_link = 
	    _find_interface_address = _routes = _retransmit = _election = 
	    _packets = val;
    }

    bool _input_errors;
    bool _interface_events;
    bool _neighbour_events;
    bool _spt;	/* Shortest Path Tree */
    bool _import_policy;
    bool _export_policy;
    bool _virtual_link;
    bool _find_interface_address;
    bool _routes;	// add,replace,delete route.
    bool _retransmit;
    bool _election;	// DR and BDR election.
    bool _packets;	// Incoming and outgoing packets.
};

#endif // __OSPF_TRACE_HH__
