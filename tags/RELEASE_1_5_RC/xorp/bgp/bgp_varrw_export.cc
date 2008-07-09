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

#ident "$XORP: xorp/bgp/bgp_varrw_export.cc,v 1.6 2007/02/16 22:45:11 pavlin Exp $"

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
