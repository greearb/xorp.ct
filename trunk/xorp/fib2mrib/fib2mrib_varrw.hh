// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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

// $XORP: xorp/fib2mrib/fib2mrib_varrw.hh,v 1.8 2008/01/04 03:16:16 pavlin Exp $

#ifndef __FIB2MRIB_FIB2MRIB_VARRW_HH__
#define __FIB2MRIB_FIB2MRIB_VARRW_HH__

#include "policy/backend/single_varrw.hh"
#include "policy/common/element_factory.hh"
#include "fib2mrib_node.hh"

/**
 * @short Allows variables to be written and read from fib2mrib routes.
 */
class Fib2mribVarRW : public SingleVarRW {
public:
    enum {
	VAR_NETWORK4 = VAR_PROTOCOL,
	VAR_NEXTHOP4,
	VAR_NETWORK6,
	VAR_NEXTHOP6,
	VAR_METRIC
    };
    
    /**
     * @param route route to read/write values from.
     */
    Fib2mribVarRW(Fib2mribRoute& route);

    // SingleVarRW inteface:
    void     start_read();
    void     single_write(const Id& id, const Element& e);
    Element* single_read(const Id& id);   

private:
    Fib2mribRoute&	_route;
    ElementFactory	_ef;
    bool		_is_ipv4;
    bool		_is_ipv6;
};

#endif // __FIB2MRIB_FIB2MRIB_VARRW_HH__
