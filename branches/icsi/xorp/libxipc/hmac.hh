// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001,2002 International Computer Science Institute
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

// $XORP: xorp/libxipc/hmac.hh,v 1.6 2002/12/09 18:29:04 hodson Exp $

#ifndef __LIBXORP_HMAC_HH_HH__
#define __LIBXORP_HMAC_HH_HH__

#include <string>
#include "hmac_md5.h"

class HMAC {
public:
    HMAC(const string& key) : _key(key) {}
    virtual ~HMAC() {}
    virtual size_t signature_size() const = 0;
    virtual const string signature(const string& message) const = 0;
    inline const string& key() const { return _key; }

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
    inline uint32_t d8tod32(uint8_t a, uint8_t b, uint8_t c, uint8_t d) const {
	return (a << 24) | (b << 16) | (c << 8) | d;
    }
    static const char* SIG;
    static const size_t SIG_SZ;
};

#endif // __LIBXORP_HMAC_HH_HH__


