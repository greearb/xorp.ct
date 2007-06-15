// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2007 International Computer Science Institute
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

#ident "$XORP: xorp/libxorp/mac.cc,v 1.13 2007/02/16 22:46:20 pavlin Exp $"

#include <vector>

#include "libxorp_module.h"
#include "xorp.h"
#include "xlog.h"
#include "mac.hh"
#include "ether_compat.h" 

/* ------------------------------------------------------------------------- */
/* Base Mac methods */

Mac::Mac(const string& s) throw (InvalidString)
    : _srep(s)
{
    // Empty string valid, nothing to do.
    if (_srep.empty())
	return;

    // ------------------------------------------------------------------------
    // I M P O R T A N T !
    //
    // Check all known MAC instance classes for whether string is valid
    //
    // Add new MyMac::valid methods here
    // ------------------------------------------------------------------------
    if (EtherMac::valid(_srep)) {
	return;
    }

    xorp_throw(InvalidString,
	       c_format("Unknown Mac representation: %s", s.c_str()));
}

string
Mac::normalized_str() const
{
    if (_srep.empty())
	return _srep;	// XXX: the empty string is valid, so just return it

    // ------------------------------------------------------------------------
    // I M P O R T A N T !
    //
    // Check all known MAC instance classes for whether string is valid
    // and return the corresponding normalized string.
    //
    // Add new MyMac::valid and MyMac::normalize() methods here
    // ------------------------------------------------------------------------
    if (EtherMac::valid(_srep)) {
	return EtherMac::normalize(_srep);
    }

    XLOG_UNREACHABLE();
    return (_srep);
}

/* ------------------------------------------------------------------------- */
/* EtherMac related methods */

EtherMac::EtherMac(const string& s) throw (InvalidString)
{
    if (valid(s)) {
	set_rep(s);
	return;
    }

    xorp_throw(InvalidString,
	       c_format("Bad EtherMac representation: %s", s.c_str()));
}

EtherMac::EtherMac(const Mac& m) throw (BadMac)
{
    string s = m.str();

    if (valid(s)) {
	set_rep(s);
	return;
    }

    xorp_throw(BadMac,
	       c_format("Bad EtherMac representation: %s", s.c_str()));
}

EtherMac::EtherMac(const struct ether_addr& ether_addr) throw (BadMac)
{
    static_assert(sizeof(ether_addr) == ADDR_BYTELEN);

    if (copy_in(ether_addr) != ADDR_BYTELEN) {
	//
	// Nobody agrees on name of fields within ether_addr structure...
	// We probably don't care as this should never be reached.
	//
	const uint8_t* s = reinterpret_cast<const uint8_t*>(&ether_addr);
	xorp_throw(BadMac, c_format("%2x:%2x:%2x:%2x:%2x:%2x",
				    s[0], s[1], s[2], s[3], s[4], s[5]));
    }
}

size_t
EtherMac::copy_out(struct ether_addr& to_ether_addr) const
{
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

    return (0);
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
	return (0);

    set_rep(ap);
    return (ADDR_BYTELEN);
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
