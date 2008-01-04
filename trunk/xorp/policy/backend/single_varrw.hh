// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

// $XORP: xorp/policy/backend/single_varrw.hh,v 1.11 2007/02/16 22:47:00 pavlin Exp $

#ifndef __POLICY_BACKEND_SINGLE_VARRW_HH__
#define __POLICY_BACKEND_SINGLE_VARRW_HH__

#include "policy/common/varrw.hh"
#include "policy/common/policy_utils.hh"
#include "policy/common/element_base.hh"
#include <list>

/**
 * @short An interface to VarRW which deals with memory management.
 *
 * Read and writes are cached, so they are done only once on the route, not
 * matter how many time the filter requests or writes the variable.
 *
 * Because of this caching, the SingleVarRW is usuable only once. After it has
 * done its work once, it has to be re-created.
 */
class SingleVarRW : public VarRW {
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
    Element* _trash[16];
    unsigned _trashc;

    // Map that caches element read/write's 
    const Element* _elems[VAR_MAX];

    // variable id's that changed
    bool _modified[VAR_MAX];

    bool _did_first_read;

    // not impl
    SingleVarRW(const SingleVarRW&);
    SingleVarRW& operator=(const SingleVarRW&);
};

#endif // __POLICY_BACKEND_SINGLE_VARRW_HH__
