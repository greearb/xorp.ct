// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
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

// $XORP: xorp/libxipc/hmac.hh,v 1.12 2008/10/02 21:57:21 bms Exp $

#ifndef __LIBXIPC_HMAC_HH__
#define __LIBXIPC_HMAC_HH__


#include "hmac_md5.h"

class HMAC {
public:
    HMAC(const string& key) : _key(key) {}
    virtual ~HMAC() {}
    virtual size_t signature_size() const = 0;
    virtual const string signature(const string& message) const = 0;
    const string& key() const { return _key; }

    virtual HMAC* clone() const = 0;
protected:

    const string _key;
};

class HMACMD5 : public HMAC {
public:
    HMACMD5(const string& key) : HMAC(key) {}
    ~HMACMD5() {}
    size_t signature_size() const { return SIG_SZ; }
    const string signature(const string& message) const;

    virtual HMAC* clone() const {
	return new HMACMD5(key());
    }
protected:
    uint32_t d8tod32(uint8_t a, uint8_t b, uint8_t c, uint8_t d) const {
	return (a << 24) | (b << 16) | (c << 8) | d;
    }
    static const char* SIG;
    static const size_t SIG_SZ;
};

#endif // __LIBXIPC_HMAC_HH__
