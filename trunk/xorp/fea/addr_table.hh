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

// $XORP: xorp/devnotes/template.hh,v 1.2 2003/01/16 19:08:48 mjh Exp $

#ifndef __FEA_ADDR_TABLE_HH__
#define __FEA_ADDR_TABLE_HH__

#include <list>
#include <set>
#include <string>

#include "libxorp/ipv4.hh"
#include "libxorp/ipv6.hh"

/**
 * @short Class for observer for events in AddressTableBase instances.
 *
 * Classes derived from AddressTableBase are used by the
 * XrlSocketServer to know which interface addresses are valid, and to
 * be notified of status changes.
 */
class AddressTableEventObserver {
public:
    virtual ~AddressTableEventObserver() = 0;

    /**
     * Address invalid event.
     *
     * @param addr address invalidated.
     */
    virtual void invalidate_address(const IPv4& addr, const string& why) = 0;

    /**
     * Address invalid event.
     *
     * @param addr address invalidated.
     */
    virtual void invalidate_address(const IPv6& addr, const string& why) = 0;
};

/**
 * Base class for Address Table.
 *
 * The address table stores a list of valid addresses.  It is intended
 * for use by the XrlSocketServer.
 */
class AddressTableBase {
public:
    virtual ~AddressTableBase();

    /**
     * Enquire whether given IPv4 address is valid.  Validity is
     * defined by being enabled and on an interface.
     */
    virtual bool address_valid(const IPv4& addr) const = 0;

    /**
     * Enquire whether given IPv6 address is valid.  Validity is
     * defined by being enabled and on an interface.
     */
    virtual bool address_valid(const IPv6& addr) const = 0;

    /**
     * Add observer for AddressTable events.
     */
    void add_observer(AddressTableEventObserver* o);

    /**
     * Remove observer for AddressTable events.
     */
    void remove_observer(AddressTableEventObserver* o);

protected:
    void invalidate_address(const IPv4& addr, const string& why);
    void invalidate_address(const IPv6& addr, const string& why);

protected:
    typedef list<AddressTableEventObserver*> ObserverList;
    ObserverList _ol;
};

#endif // __FEA_ADDR_TABLE_HH__
