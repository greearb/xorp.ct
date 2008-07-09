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

#ident "$XORP: xorp/bgp/route_table_policy_ex.cc,v 1.5 2007/02/16 22:45:18 pavlin Exp $"

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
					const string& neighbor)
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
}

template <class A>
void
PolicyTableExport<A>::init_varrw()
{
    this->_varrw = new BGPVarRWExport<A>(
                        filter::filter2str(PolicyTable<A>::_filter_type),
                        _neighbor);
}

template class PolicyTableExport<IPv4>;
template class PolicyTableExport<IPv6>;
