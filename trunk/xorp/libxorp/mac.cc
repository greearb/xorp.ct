// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/libxorp/mac.cc,v 1.27 2008/07/23 05:10:52 pavlin Exp $"

#include "libxorp/xorp.h"
#include "libxorp/ether_compat.h" 

#include "mac.hh"


Mac::Mac()
{
    memset(_addr, 0, sizeof(_addr));
}

Mac::Mac(const uint8_t* from_uint8)
{
    copy_in(from_uint8);
}

Mac::Mac(const char* from_cstring) throw (InvalidString)
{
    copy_in(from_cstring);
}

Mac::Mac(const struct ether_addr& from_ether_addr)
{
    copy_in(from_ether_addr);
}

Mac::Mac(const struct sockaddr& from_sockaddr)
{
    copy_in(from_sockaddr);
}

size_t
Mac::copy_out(uint8_t* to_uint8) const
{
    memcpy(to_uint8, _addr, sizeof(_addr));
    return (sizeof(_addr));
}

size_t
Mac::copy_out(struct ether_addr& to_ether_addr) const
{
    memcpy(&to_ether_addr, _addr, sizeof(_addr));
    return (sizeof(_addr));
}

size_t
Mac::copy_out(struct sockaddr& to_sockaddr) const
{
    memset(&to_sockaddr, 0, sizeof(to_sockaddr));

#ifdef  HAVE_STRUCT_SOCKADDR_SA_LEN
    to_sockaddr.sa_len = sizeof(to_sockaddr);
#endif
#ifdef AF_LINK
    to_sockaddr.sa_family = AF_LINK;
#else
    to_sockaddr.sa_family = AF_UNSPEC;
#endif

    uint8_t* sa_data = reinterpret_cast<uint8_t*>(to_sockaddr.sa_data);
    return (copy_out(sa_data));
}

size_t
Mac::copy_in(const uint8_t* from_uint8)
{
    memcpy(_addr, from_uint8, sizeof(_addr));
    return (sizeof(_addr));
}

size_t
Mac::copy_in(const struct ether_addr& from_ether_addr)
{
    memcpy(_addr, &from_ether_addr, sizeof(_addr));
    return (sizeof(_addr));
}

size_t
Mac::copy_in(const struct sockaddr& from_sockaddr)
{
    const uint8_t* sa_data = reinterpret_cast<const uint8_t*>(from_sockaddr.sa_data);
    return (copy_in(sa_data));
}

size_t
Mac::copy_in(const char* from_cstring) throw (InvalidString)
{
    if (from_cstring == NULL)
	xorp_throw(InvalidString, "Null value");

    //
    // XXX: We need to const_cast the ether_aton() argument, because
    // on some OS (e.g., MacOS X 10.2.3 ?) the ether_aton(3) declaration
    // is broken (missing "const" in the argument).
    //
    const struct ether_addr* ea = ether_aton(const_cast<char *>(from_cstring));
    if (ea == NULL)
	xorp_throw(InvalidString, c_format("Bad Mac \"%s\"", from_cstring));

    return (copy_in(*ea));
}

bool
Mac::operator<(const Mac& other) const
{
    size_t i;

    for (i = 0; i < (sizeof(_addr) - 1); i++) {
	// XXX: Loop ends intentionally one octet earlier
        if (_addr[i] != other._addr[i])
            break;
    }
    return (_addr[i] < other._addr[i]);
}

bool
Mac::operator==(const Mac& other) const
{
    return (memcmp(_addr, other._addr, sizeof(_addr)) == 0);
}

bool
Mac::operator!=(const Mac& other) const
{
    return (memcmp(_addr, other._addr, sizeof(_addr)) != 0);
}

string
Mac::str() const
{
    struct ether_addr ea;

    copy_out(ea);
    return (ether_ntoa(&ea));	// XXX: implicitly create string return object
}

bool
Mac::is_unicast() const
{
    return (! is_multicast());
}

bool
Mac::is_multicast() const
{
    return (_addr[0] & MULTICAST_BIT);
}

const Mac MacConstants::zero(Mac("00:00:00:00:00:00"));
const Mac MacConstants::all_ones(Mac("ff:ff:ff:ff:ff:ff"));
const Mac MacConstants::stp_multicast(Mac("01:80:c2:00:00:00"));
const Mac MacConstants::lldp_multicast(Mac("01:80:c2:00:00:0e"));
const Mac MacConstants::gmrp_multicast(Mac("01:80:c2:00:00:20"));
const Mac MacConstants::gvrp_multicast(Mac("01:80:c2:00:00:21"));
