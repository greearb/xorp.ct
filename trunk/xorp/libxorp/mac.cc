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

#ident "$XORP: xorp/libxorp/mac.cc,v 1.4 2003/03/10 23:20:33 hodson Exp $"

#include "xorp.h"
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
    if (EtherMac::valid(_srep))
	return;

    xorp_throw(InvalidString,
	       c_format("Unknown Mac representation: %s", _srep.c_str()));
}

/* ------------------------------------------------------------------------- */
/* EtherMac related methods */

EtherMac::EtherMac(const string& s) throw (InvalidString)
{
    set_rep(s);
    if (valid(_srep)) 
	return;
    xorp_throw(InvalidString,
	       c_format("Bad EtherMac representation: %s", _srep.c_str()));
}

EtherMac::EtherMac(const Mac& m) throw (BadMac)
{
    set_rep(m.str());
    if (valid(_srep))
	return;
    xorp_throw(BadMac,
	       c_format("Bad EtherMac representation: %s", _srep.c_str()));
}

EtherMac::EtherMac(const struct ether_addr& ea) throw (BadMac)
{
    // XXX: we need to const_cast the ether_ntoa() argument,
    // because in some OS (e.g., MacOS X 10.2.3) the ether_ntoa(3)
    // declaration is broken.
    const char* a = ether_ntoa(const_cast<struct ether_addr *>(&ea));
    if (a) {
	set_rep(a);
	return;
    }

    //
    // Nobody agrees on name of fields within ether_addr structure...
    // We probably don't care as this should never be reached.
    //
    static_assert(sizeof(ea) == 6);
    const uint8_t* s = reinterpret_cast<const uint8_t*>(&ea);
    xorp_throw(BadMac, c_format("%2x:%2x:%2x:%2x:%2x:%2x",
				s[0], s[1], s[2], s[3], s[4], s[5]));
}

bool
EtherMac::get_ether_addr(struct ether_addr& ea) const
{
    //
    // XXX: work-around because of broken ether_aton() declarations that
    // are missing the 'const' in the argument.
    //
    char buf[_srep.size() + 1];
    strncpy(buf, _srep.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    const struct ether_addr* p = ether_aton(buf);
    if (p) {
	memcpy(&ea, p, sizeof(ea));
	return true;
    }
    return false;
}

bool
EtherMac::valid(const string& s)
{
    //
    // XXX: work-around because of broken ether_aton() declarations that
    // are missing the 'const' in the argument.
    //
    char buf[s.size() + 1];
    strncpy(buf, s.c_str(), sizeof(buf) - 1);
    buf[sizeof(buf) - 1] = '\0';

    return ether_aton(buf) != 0;
}

