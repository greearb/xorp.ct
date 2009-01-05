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

#ident "$XORP: xorp/bgp/bgp_varrw_export.cc,v 1.11 2008/12/16 23:55:41 mjh Exp $"

#include "bgp_module.h"

#include "libxorp/xorp.h"

#include "bgp_varrw_export.hh"


template <class A>
BGPVarRWExport<A>::BGPVarRWExport(const string& name,
				  const string& neighbor)
    : BGPVarRW<A>(name), _neighbor(neighbor)
{
}

template <class A>
Element*
BGPVarRWExport<A>::read_neighbor()
{
    return BGPVarRW<A>::_ef.create(ElemIPv4::id, _neighbor.c_str());
}

template class BGPVarRWExport<IPv4>;
template class BGPVarRWExport<IPv6>;
