// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

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

#ident "$XORP: xorp/libxorp/mac.cc,v 1.25 2008/05/01 03:30:05 pavlin Exp $"

#include <vector>

#include "libxorp_module.h"
#include "xorp.h"
#include "xlog.h"
#include "mac.hh"
#include "ether_compat.h" 

/* ------------------------------------------------------------------------- */
/* Base Mac methods */

Mac::Mac()
{
    *this = Mac::ZERO();
}

Mac::Mac(const uint8_t* from_uint8, size_t len) throw (BadMac)
{
    copy_in(from_uint8, len);
}

Mac::Mac(const string& from_string) throw (InvalidString)
{
    copy_in(from_string);
}

size_t
Mac::copy_out(uint8_t* to_uint8) const
{
    // ------------------------------------------------------------------------
    // I M P O R T A N T !
    //
    // Check all known MAC instance classes for whether string is valid
    // and use the corresponding copy_out() method.
    //
    // Add new MyMac::valid() and MyMac::normalize() methods here
    // ------------------------------------------------------------------------
    if (EtherMac::valid(_srep)) {
	EtherMac ether_mac(_srep);
	return (ether_mac.copy_out(to_uint8));
    }

    XLOG_UNREACHABLE();
    return (static_cast<size_t>(-1));
}

size_t
Mac::copy_in(const uint8_t* from_uint8, size_t len) throw (BadMac)
{
    size_t ret_value = static_cast<size_t>(-1);

    // ------------------------------------------------------------------------
    // I M P O R T A N T !
    //
    // Check all known MAC instance classes for whether address length is valid
    //
    // Add new address length checks here
    // ------------------------------------------------------------------------
    do {
	if (len == EtherMac::ADDR_BYTELEN) {
	    EtherMac ether_mac(from_uint8);
	    set_rep(ether_mac.str());
	    ret_value = len;
	    break;
	}

	xorp_throw(BadMac,
		   c_format("Unknown Mac representation: length = %u",
			    XORP_UINT_CAST(len)));
	return (static_cast<size_t>(-1));
    } while (false);

    return (ret_value);
}

size_t
Mac::copy_in(const string& from_string) throw (InvalidString)
{
    size_t ret_value = static_cast<size_t>(-1);

    // ------------------------------------------------------------------------
    // I M P O R T A N T !
    //
    // Check all known MAC instance classes for whether string is valid
    //
    // Add new MyMac::valid() methods here
    // ------------------------------------------------------------------------
    do {
	if (EtherMac::valid(from_string)) {
	    ret_value = EtherMac::ADDR_BYTELEN;
	    break;
	}
	xorp_throw(InvalidString,
		   c_format("Unknown Mac representation: %s",
			    from_string.c_str()));
	return (static_cast<size_t>(-1));
    } while (false);

    set_rep(from_string);
    return (ret_value);
}

string
Mac::normalized_str() const
{
    // ------------------------------------------------------------------------
    // I M P O R T A N T !
    //
    // Check all known MAC instance classes for whether string is valid
    // and return the corresponding normalized string.
    //
    // Add new MyMac::valid() and MyMac::normalize() methods here
    // ------------------------------------------------------------------------
    if (EtherMac::valid(_srep)) {
	return EtherMac::normalize(_srep);
    }

    XLOG_UNREACHABLE();
    return (_srep);
}

size_t
Mac::addr_bytelen() const
{
    // ------------------------------------------------------------------------
    // I M P O R T A N T !
    //
    // Check all known MAC instance classes for whether string is valid
    // and return the corresponding size.
    //
    // Add new MyMac::valid() and MyMac::addr_bytelen() methods here
    // ------------------------------------------------------------------------
    if (EtherMac::valid(_srep)) {
	return EtherMac::addr_bytelen();
    }

    XLOG_UNREACHABLE();
    return (0);
}

uint32_t
Mac::addr_bitlen() const
{
    // ------------------------------------------------------------------------
    // I M P O R T A N T !
    //
    // Check all known MAC instance classes for whether string is valid
    // and return the corresponding size.
    //
    // Add new MyMac::valid() and MyMac::addr_bitlen() methods here
    // ------------------------------------------------------------------------
    if (EtherMac::valid(_srep)) {
	return EtherMac::addr_bitlen();
    }

    XLOG_UNREACHABLE();
    return (0);
}

bool
Mac::is_zero() const
{
    return (*this == ZERO());
}

bool
Mac::is_multicast() const
{
    // ------------------------------------------------------------------------
    // I M P O R T A N T !
    //
    // Check all known MAC instance classes for whether string is valid
    //
    // Add new MyMac::valid() methods here
    // ------------------------------------------------------------------------
    if (EtherMac::valid(_srep)) {
	EtherMac ether_mac(_srep);
	return (ether_mac.is_multicast());
    }

    return (false);
}


/* ------------------------------------------------------------------------- */
/* EtherMac related methods */

EtherMac::EtherMac(const uint8_t* from_uint8)
{
    copy_in(from_uint8);
}

EtherMac::EtherMac(const string& from_string) throw (InvalidString)
{
    if (valid(from_string)) {
	set_rep(from_string);
	return;
    }

    xorp_throw(InvalidString,
	       c_format("Bad EtherMac representation: %s",
			from_string.c_str()));
}

EtherMac::EtherMac(const Mac& from_mac) throw (BadMac)
{
    copy_in(from_mac);
}

EtherMac::EtherMac(const struct ether_addr& from_ether_addr) throw (BadMac)
{
    static_assert(sizeof(from_ether_addr) == ADDR_BYTELEN);

    if (copy_in(from_ether_addr) != ADDR_BYTELEN) {
	//
	// Nobody agrees on name of fields within ether_addr structure...
	// We probably don't care as this should never be reached.
	//
	const uint8_t* s = reinterpret_cast<const uint8_t*>(&from_ether_addr);
	xorp_throw(BadMac, c_format("%2x:%2x:%2x:%2x:%2x:%2x",
				    s[0], s[1], s[2], s[3], s[4], s[5]));
    }
}

EtherMac::EtherMac(const struct sockaddr& from_sockaddr) throw (BadMac)
{
    if (copy_in(from_sockaddr) != ADDR_BYTELEN) {
	const uint8_t* s = reinterpret_cast<const uint8_t*>(from_sockaddr.sa_data);
	xorp_throw(BadMac, c_format("%2x:%2x:%2x:%2x:%2x:%2x",
				    s[0], s[1], s[2], s[3], s[4], s[5]));
    }
}

size_t
EtherMac::copy_out(uint8_t* to_uint8) const
{
    struct ether_addr ether_addr;

    static_assert(sizeof(ether_addr) == ADDR_BYTELEN);

    if (copy_out(ether_addr) != sizeof(ether_addr))
	return (static_cast<size_t>(-1));

    memcpy(to_uint8, &ether_addr, sizeof(ether_addr));

    return (ADDR_BYTELEN);
}

size_t
EtherMac::copy_in(const uint8_t* from_uint8)
{
    struct ether_addr ether_addr;

    static_assert(sizeof(ether_addr) == ADDR_BYTELEN);
    memcpy(&ether_addr, from_uint8, sizeof(ether_addr));

    return (copy_in(ether_addr));
}

size_t
EtherMac::copy_out(struct ether_addr& to_ether_addr) const
{
    static_assert(sizeof(ether_addr) == ADDR_BYTELEN);

    //
    // XXX: work-around because of broken ether_aton() declarations that
    // are missing the 'const' in the argument.
    //
    vector<char> buf(_srep.size() + 1);
    strncpy(&buf[0], _srep.c_str(), buf.size() - 1);
    buf[buf.size() - 1] = '\0';

    const struct ether_addr* ep = ether_aton(&buf[0]);
    if (ep != NULL) {
	memcpy(&to_ether_addr, ep, sizeof(to_ether_addr));
	return (sizeof(to_ether_addr));
    }

    return (static_cast<size_t>(-1));
}

size_t
EtherMac::copy_in(const struct ether_addr& from_ether_addr)
{
    //
    // XXX: we need to const_cast the ether_ntoa() argument,
    // because on some OS (e.g., MacOS X 10.2.3) the ether_ntoa(3)
    // declaration is broken.
    //
    const char* ap = ether_ntoa(const_cast<struct ether_addr *>(&from_ether_addr));
    if (ap == NULL)
	return (static_cast<size_t>(-1));

    set_rep(ap);
    return (ADDR_BYTELEN);
}

size_t
EtherMac::copy_out(struct sockaddr& to_sockaddr) const
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
EtherMac::copy_in(const struct sockaddr& from_sockaddr)
{
    const uint8_t* sa_data = reinterpret_cast<const uint8_t*>(from_sockaddr.sa_data);
    return (copy_in(sa_data));
}

size_t
EtherMac::copy_out(Mac& to_mac) const
{
    try {
	to_mac.copy_in(str());
	return (ADDR_BYTELEN);
    } catch (const BadMac& e) {
	// XXX: shouldn't happen
	XLOG_FATAL("Invalid EtherMac address: %s", str().c_str());
    }

    return (static_cast<size_t>(-1));
}

size_t
EtherMac::copy_in(const Mac& from_mac) throw (BadMac)
{
    string s = from_mac.str();

    if (valid(s)) {
	set_rep(s);
	return (ADDR_BYTELEN);
    }

    xorp_throw(BadMac,
	       c_format("Bad EtherMac representation: %s", s.c_str()));
    return (static_cast<size_t>(-1));
}

bool
EtherMac::valid(const string& s)
{
    //
    // XXX: work-around because of broken ether_aton() declarations that
    // are missing the 'const' in the argument.
    //
    vector<char> buf(s.size() + 1);
    strncpy(&buf[0], s.c_str(), buf.size() - 1);
    buf[buf.size() - 1] = '\0';

    return (ether_aton(&buf[0]) != NULL);
}

string
EtherMac::normalize(const string& s) throw (InvalidString)
{
    //
    // XXX: work-around because of broken ether_aton() declarations that
    // are missing the 'const' in the argument.
    //
    vector<char> buf(s.size() + 1);
    strncpy(&buf[0], s.c_str(), buf.size() - 1);
    buf[buf.size() - 1] = '\0';

    //
    // Convert the string with an EtherMAC address into
    // an "struct ether_addr", and then back to a string.
    // Thus, the string address representation is normalized
    // to the system's internal preference. Example:
    // "00:00:00:00:00:00" -> "0:0:0:0:0:0"
    //
    struct ether_addr* ep;
    ep = ether_aton(&buf[0]);
    if (ep == NULL) {
	xorp_throw(InvalidString,
		   c_format("Bad EtherMac representation: %s", s.c_str()));
    }
    char* ap = ether_ntoa(ep);
    if (ap == NULL) {
	xorp_throw(InvalidString,
		   c_format("Internal error: bad EtherMac representation: %s",
			    s.c_str()));
    }
    return (string(ap));
}

bool
EtherMac::is_multicast() const
{
    uint8_t addr[ADDR_BYTELEN];

    copy_out(addr);

    return (addr[0] & MULTICAST_BIT);
}

const Mac MacConstants::zero(Mac("00:00:00:00:00:00"));
const Mac MacConstants::all_ones(Mac("ff:ff:ff:ff:ff:ff"));
const Mac MacConstants::stp_multicast(Mac("01:80:c2:00:00:00"));
const Mac MacConstants::lldp_multicast(Mac("01:80:c2:00:00:0e"));
const Mac MacConstants::gmrp_multicast(Mac("01:80:c2:00:00:20"));
const Mac MacConstants::gvrp_multicast(Mac("01:80:c2:00:00:21"));
