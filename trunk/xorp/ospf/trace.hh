// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2005 International Computer Science Institute
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

// $XORP: xorp/ospf/trace.hh,v 1.8 2005/11/04 07:50:22 atanu Exp $

#ifndef __OSPF_TRACE_HH__
#define __OSPF_TRACE_HH__

/**
 * All the trace variables in one place.
 */
struct Trace {
    Trace() : _input_errors(false),
	      _interface_events(false),
	      _neighbour_events(false),
	      _spt(false),
	      _import_policy(false),
	      _export_policy(false),
	      _virtual_link(false)
	      // Don't forget to add new variables to the all() method.
	{}

    /*
     * Set all flags
     */
    void all(bool val) {
	_input_errors = _interface_events = _neighbour_events = _spt = 
	    _import_policy = _export_policy = _virtual_link = val;
    }

    bool _input_errors;
    bool _interface_events;
    bool _neighbour_events;
    bool _spt;	/* Shortest Path Tree */
    bool _import_policy;
    bool _export_policy;
    bool _virtual_link;
};

#endif // __OSPF_TRACE_HH__
