// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/contrib/olsr/trace.hh,v 1.1 2008/04/24 15:19:56 bms Exp $

#ifndef __OLSR_TRACE_HH__
#define __OLSR_TRACE_HH__

/**
 * @short Trace control variables.
 */
struct Trace {
    /**
     * Construct a new Trace instance.
     *
     * Don't forget to add new variables to the all() method.
     */
    inline Trace() :
	_input_errors(true),
	_packets(false),
	_mpr_selection(false),
	_interface_events(false),
	_neighbor_events(false),
	_spt(false),
	_routes(false),
	_import_policy(false),
	_export_policy(false)
    {}

    /**
     * Set all tracing options to @param value
     */
    inline void all(bool value) {
	_input_errors =
	_packets =
	_mpr_selection =
	_interface_events =
	_neighbor_events =
	_spt =
	_routes =
	_import_policy =
	_export_policy =
	    value;
    }

    /**
     * @short true if tracing is enabled for message input errors.
     */
    bool	_input_errors;

    /**
     * @short true if tracing is enabled for packet I/O.
     */
    bool	_packets;

    /**
     * @short true if tracing is enabled for MPR selection.
     */
    bool	_mpr_selection;

    /**
     * @short true if tracing is enabled for interface events.
     */
    bool	_interface_events;

    /**
     * @short true if tracing is enabled for neighbor events.
     */
    bool	_neighbor_events;

    /**
     * @short true if tracing is enabled for Shortest Path Tree operations.
     */
    bool	_spt;

    /**
     * @short true if tracing is enabled for route add/replace/delete
     * operations.
     */
    bool	_routes;

    /**
     * @short true if tracing is enabled for import policy filters.
     */
    bool	_import_policy;

    /**
     * @short true if tracing is enabled for export policy filters.
     */
    bool	_export_policy;
};

#endif // __OLSR_TRACE_HH__
