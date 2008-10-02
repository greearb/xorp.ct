// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/policy/common/varrw.hh,v 1.18 2008/08/06 08:27:12 abittau Exp $

#ifndef __POLICY_BACKEND_VARRW_HH__
#define __POLICY_BACKEND_VARRW_HH__

#include "element_base.hh"
#include <string>
#include <sstream>

/**
 * @short Interface used by policy filters to execute a policy on a route.
 *
 * It deals with reading and writing field/variables/attributes of a route [such
 * as nexthop, metric and so on].
 *
 * A routing protocol must implement this interface in order to support policy
 * filtering.
 *
 */
class VarRW {
public:
    typedef int Id;
    enum {
        VAR_TRACE	= 0,
        VAR_POLICYTAGS,
        VAR_FILTER_IM,
        VAR_FILTER_SM,
        VAR_FILTER_EX,
	VAR_TAG		= 5,
        VAR_PROTOCOL	= 10, // protocol specific vars start here
        VAR_MAX		= 32  // must be last
    };

    VarRW();
    virtual ~VarRW();

    /**
     * Read a variable from a route [such as nexthop].
     *
     * If the protocol doesn't support the requested variable, and exception
     * should be thrown.
     *
     * If the variable is not present in the current route, then an ElemNull
     * should be returned [for example if ipv6 is requested on a v4 route].
     *
     * VarRW is responsible for deleting the object read [it owns it]. However
     * care must be taken not to delete objects that were obtained by write()
     * even though we pass them to read() later.
     *
     * @return Element requested, or ElemNull of element is not available.
     * @param id The variable that is being requested [such as metric].
     *
     */
    virtual const Element& read(const Id& id) = 0;

    /**
     * Write a variable to a route.
     *
     * VarRW does not own Element, so it must not delete it.
     *
     * @param id Identifier of variable that must be written to.
     * @param e Value that must be written to the variable.
     *
     */
    virtual void write(const Id& id, const Element& e) = 0;

    /**
     * VarRW must perform all pending writes to the route now.
     *
     * This is usefull in scenarios where VarRW decides to cache read and writes
     * and perform the actual writes at the end [i.e. it stores pointers to
     * elements].
     *
     * All pointers to elements [by write] may become invalid after a sync.
     *
     */
    virtual void sync();

    /**
     * Enable/disable generating trace strings / output.
     *
     */
    void enable_trace(bool on);

    /**
     * Support for tracing reads.  Executor will call this.
     * This call will then call read()
     *
     * @param id variable to read.
     * @return variable desired.
     */
    const Element& read_trace(const Id& id); 

    /**
     * Support for tracing writes.  Executor will call this.
     * This will then call write()
     *
     * @ param id variable to write to.
     * @param e value to write.
     */
    void write_trace(const Id& id, const Element& e);

    /**
     * Obtain the final trace value.  Should be called after executing the
     * policy in case it changes.
     *
     * @return trace value.
     *
     */
    uint32_t trace();

    /**
     * Obtain the actual trace from the varrw.
     *
     * @return string representation of what was read and written.
     */
    string tracelog();

    /**
     * Obtain any VarRW specific traces.
     *
     * @return string representation of specific VarRW traces.
     */
    virtual string more_tracelog();

    void reset_trace();

private:
    bool	    _do_trace;
    uint32_t	    _trace;
    ostringstream   _tracelog;
};

#endif // __POLICY_BACKEND_VARRW_HH__
