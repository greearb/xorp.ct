// vim:set sts=4 ts=8:

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP$

#ifndef __POLICY_BACKEND_VARRW_HH__
#define __POLICY_BACKEND_VARRW_HH__

#include "element.hh"
#include <string>

using std::string;

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
    virtual ~VarRW() {}
   
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
    virtual const Element& read(const string& id) = 0;

    /**
     * Write a variable to a route.
     *
     * VarRW does not own Element, so it must not delete it.
     *
     * @param id Identifier of variable that must be written to.
     * @param e Value that must be written to the variable.
     *
     */
    virtual void write(const string& id, const Element& e) = 0;

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
    virtual void sync() = 0;
};

#endif // __POLICY_BACKEND_VARRW_HH__
