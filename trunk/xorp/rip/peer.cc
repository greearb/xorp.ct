// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2003 International Computer Science Institute
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

#ident "$XORP: xorp/rip/peer.cc,v 1.1 2003/07/09 00:11:02 hodson Exp $"

#include "peer.hh"
#include "port.hh"

template<typename A>
uint32_t
Peer<A>::expiry_secs() const
{
    return port().constants().expiry_secs();
}

template<typename A>
uint32_t
Peer<A>::deletion_secs() const
{
    return port().constants().deletion_secs();
}


// ----------------------------------------------------------------------------
// Instantiations

#ifdef INSTANTIATE_IPV4
template class Peer<IPv4>;
#endif

#ifdef INSTANTIATE_IPV6
template class Peer<IPv6>;
#endif
