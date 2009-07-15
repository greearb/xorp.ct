// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

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



// #define DEBUG_LOGGING
// #define DEBUG_PRINT_FUNCTION_NAME

#include "bgp_module.h"
#include "route_table_policy_ex.hh"
#include "bgp_varrw_export.hh"

template <class A>
PolicyTableExport<A>::PolicyTableExport(const string& tablename, 
					const Safi& safi,
					BGPRouteTable<A>* parent,
					PolicyFilters& pfs,
					const string& neighbor,
					const A& self)
    : PolicyTable<A>(tablename, safi, parent, pfs, filter::EXPORT),
      _neighbor(neighbor)
{
    this->_parent = parent;

    //
    // XXX: Explicitly call PolicyTableExport::init_varrw(),
    // because the virtual PolicyTable::init_varrw() won't be overwritten
    // by PolicyTableExport::init_varrw() when called inside the
    // PolicyTable constructor.
    //
    init_varrw();
    this->_varrw->set_self(self);
}

template <class A>
void
PolicyTableExport<A>::init_varrw()
{
    if (this->_varrw)
	delete this->_varrw;
    this->_varrw = new BGPVarRWExport<A>(
                        filter::filter2str(PolicyTable<A>::_filter_type),
                        _neighbor);
}

template class PolicyTableExport<IPv4>;
template class PolicyTableExport<IPv6>;
