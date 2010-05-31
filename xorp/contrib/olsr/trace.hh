// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8 sw=4:

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

// $XORP: xorp/contrib/olsr/trace.hh,v 1.3 2008/10/02 21:56:36 bms Exp $

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
