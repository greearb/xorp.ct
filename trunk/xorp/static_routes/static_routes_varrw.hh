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

#ifndef __STATIC_ROUTES_STATIC_ROUTES_VARRW_HH__
#define __STATIC_ROUTES_STATIC_ROUTES_VARRW_HH__

#include "policy/backend/single_varrw.hh"
#include "policy/common/element_factory.hh"
#include "static_routes_node.hh"

/**
 * @short Allows variables to be written and read from static routes.
 */
class StaticRoutesVarRW : public SingleVarRW {
public:
    /**
     * @param route route to read/write values from.
     */
    StaticRoutesVarRW(StaticRoute& route);

    // SingleVarRW inteface: 
    void single_start();
    void single_write(const string& id, const Element& e);
    void single_end();
   

private:
    StaticRoute& _route;
    ElementFactory _ef;
    bool _v4;
};

#endif // __STATIC_ROUTES_STATIC_ROUTES_VARRW_HH__
