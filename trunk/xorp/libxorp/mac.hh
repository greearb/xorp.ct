// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2004 International Computer Science Institute
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

// $XORP: xorp/libxorp/mac.hh,v 1.3 2003/10/11 03:34:45 pavlin Exp $

#ifndef __LIBXORP_MAC_HH__
#define __LIBXORP_MAC_HH__

#include <string>

#include "config.h"
#include "ether_compat.h"
#include "exceptions.hh"

/**
 * @short MAC exceptions.
 *
 * The exception which is thrown when an illegal operation is
 * attempted on a MAC.
 */
class BadMac : public XorpReasonedException {
public:
    BadMac(const char* file, size_t line, const string init_why = "") 
	: XorpReasonedException("Incompatible Mac conversion",
				file, line, init_why) {}
};

/**
 * @short Generic container for any type of MAC.
 *
 * A class intended to carry any type of MAC. The assumption is
 * that all current and future MAC classes will have a printable
 * string representation and can be treated as opaque identifiers.
 */
class Mac {
public:
    /**
     * Default constructor
     */
    Mac() {}

    /**
     * Construct MAC address from string.  Mac address format must conform
     * to one of known types.
     *
     * @param s string representation of mac.
     * @throws InvalidString if s is not recognized Mac type.
     */
    Mac(const string& s) throw (InvalidString);

    /**
     * @return string representation of MAC address.
     */
    inline const string& str() const { return _srep; }

    /**
     * @return true if MAC address value is unset.
     */
    inline bool empty() const { return _srep.empty(); }

protected:
    inline void set_rep(const string& s) { _srep = s; }
    string _srep;
};

//
// EtherMac is really IEEE standard 6 octet address
//
class EtherMac : public Mac {
public:
    /**
     * Default constructor
     */
    EtherMac() : Mac() {}

    /**
     * Construct EtherMac from a string representation.
     *
     * @param s string representation of the form XX:XX:XX:XX:XX:XX where
     *          X represents a hex-digit.
     * @throws InvalidString if string passed does not match expected format.
     */
    EtherMac(const string& s) throw (InvalidString);

    /**
     * Construct EtherMac from Mac. 
     *
     * @param m Mac to construct EtherMac from.
     *
     * @throws BadMac if the Mac's string representation is not equivalent to 
     * the EtherMac's string representation.
     */
    EtherMac(const Mac& m) throw (BadMac);

    /**
     * Construct EtherMac from ether_addr.
     */
    EtherMac(const ether_addr& ea) throw (BadMac);
    
    /**
     * Convert to ether_addr representation.
     *
     * @param ea ether_addr to store representation.
     * @return true on success, false if underlying string is empty.
     */
    bool 
    get_ether_addr(struct ether_addr& ea) const;

    /**
     * Check whether string contains valid EtherMac representation.
     *
     * @param s potential EtherMac string representation.
     * @return true if s is valid, false otherwise.
     */
    static bool valid(const string& s);
};

inline bool
operator==(const Mac& m1, const Mac& m2)
{
    return m1.str() == m2.str();
}

inline bool
operator==(const EtherMac& m1, const EtherMac& m2)
{
    struct ether_addr ea1, ea2;

    if (m1.get_ether_addr(ea1) != true)
	return false;
    if (m2.get_ether_addr(ea2) != true)
	return false;
    return (memcmp(&ea1, &ea2, sizeof(ea1)) == 0);
}

#endif // __LIBXORP_MAC_HH__
