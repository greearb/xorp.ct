// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2008 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License, Version
// 2.1, June 1999 as published by the Free Software Foundation.
// Redistribution and/or modification of this program under the terms of
// any other version of the GNU Lesser General Public License is not
// permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU Lesser General Public License, Version 2.1, a copy of
// which can be found in the XORP LICENSE.lgpl file.
// 
// XORP, Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net

// $XORP: xorp/libxipc/xuid.hh,v 1.9 2008/07/23 05:10:47 pavlin Exp $

#ifndef __LIBXIPC_XUID_HH__
#define __LIBXIPC_XUID_HH__

#include <string>

class XUID {
public:
    class InvalidString {};
    XUID(const string&) throw (class InvalidString);

    // an XUID can be explicitly constructed
    XUID() { initialize(); }

    // Or a block of memory can be cast as an XUID and initialized.
    void initialize();

    bool operator==(const XUID&) const;
    bool operator<(const XUID&) const;

    string str() const;
private:
    uint32_t _data[4];		// Internal representation is network ordered
};

#endif // __LIBXIPC_XUID_HH__
