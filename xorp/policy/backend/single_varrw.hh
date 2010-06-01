// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2010 XORP, Inc and Others
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

// $XORP: xorp/policy/backend/single_varrw.hh,v 1.15 2008/10/02 21:58:05 bms Exp $

#ifndef __POLICY_BACKEND_SINGLE_VARRW_HH__
#define __POLICY_BACKEND_SINGLE_VARRW_HH__

#include <list>



#include "policy/common/varrw.hh"
#include "policy/common/policy_utils.hh"
#include "policy/common/element_base.hh"
#include "policytags.hh"

/**
 * @short An interface to VarRW which deals with memory management.
 *
 * Read and writes are cached, so they are done only once on the route, not
 * matter how many time the filter requests or writes the variable.
 *
 * Because of this caching, the SingleVarRW is usuable only once. After it has
 * done its work once, it has to be re-created.
 */
class SingleVarRW :
    public NONCOPYABLE,
    public VarRW
{
public:
    /**
     * @short Exception thrown on error, such as reading unsupported variable.
     */
    class SingleVarRWErr : public PolicyException {
    public:
	SingleVarRWErr(const char* file, size_t line, 
		       const string& init_why = "")   
            : PolicyException("SingleVarRWErr", file, line, init_why) {} 
    };

    SingleVarRW();
    virtual ~SingleVarRW();

    /**
     * Implementation of VarRW read.
     *
     * @return variable requested.
     * @param id identifier of variable to be read.
     */
    const Element& read(const Id& id);

    /**
     * Implementation of VarRW write.
     *
     * @param id identifier of variable to be written to.
     * @param e value of variable to be written to.
     */
    void write(const Id& id, const Element& e);

    /**
     * Implementation of VarRW sync.
     *
     * Writes are performed now, as cached Element* pointers may become invalid
     * afterwards.a
     *
     * trash is also emptied upon completion.
     */
    void sync();

    // XXX: be smart: register callback for element writing
    /**
     * Register a variable for read access with SingleVarRW.
     * SingleVarRW owns the element, so derived classes do not need to worry
     * about deleting objects.
     *
     * All supported variables must be registered, even the ones not present in
     * the current route. For example v6 nexthops must be set to ElemNull on v4
     * routes. [assuming the protocol itself supports v6].
     *
     * @param id identifier of variable that may be read.
     * @param e value of variable.
     */
    void initialize(const Id& id, Element* e);
    
    void initialize(PolicyTags& pt);

    /**
     * If any reads are performed, this is a marker which informs the derived
     * class that reads will now start.
     */
    virtual void start_read() {} 

    /**
     * If any writes were performed, this is a marker which informs the derived
     * class that writes will start.
     */
    virtual void start_write() {}

    /**
     * Write of a variable. The write MUST be performed now, as the element
     * pointer may become invalid after this call. Also, a single write will be
     * called for each modified element.
     *
     * @param id identifier of variable to be written to.
     * @param e value of variable.
     */
    virtual void single_write(const Id& id, const Element& e) = 0;

    /**
     * Read of a variable.  The VarRW needs to read a particular element.  This
     * may return NULL indicating ElemNull---i.e. variable not present in THIS
     * route.
     *
     * @return variable requested.
     * @param id the id of the variable.
     */
    virtual Element* single_read(const Id& id) = 0;

    /**
     * Marks the end of writes in case there were any modified fields.
     */
    virtual void end_write() {}

private:
    Element*	    _trash[16];
    unsigned	    _trashc;
    const Element*  _elems[VAR_MAX];    // Map that caches element read/writes 
    bool	    _modified[VAR_MAX]; // variable id's that changed
    bool	    _did_first_read;
    PolicyTags*	    _pt;
};

#endif // __POLICY_BACKEND_SINGLE_VARRW_HH__
