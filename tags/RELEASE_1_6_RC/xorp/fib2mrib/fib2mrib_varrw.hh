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

// $XORP: xorp/fib2mrib/fib2mrib_varrw.hh,v 1.9 2008/07/23 05:10:36 pavlin Exp $

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
