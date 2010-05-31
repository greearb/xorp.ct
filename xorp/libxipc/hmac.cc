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



#include "ipc_module.h"
#include "libxorp/xorp.h"
#include "libxorp/c_format.hh"
#include "hmac.hh"

const char*  HMACMD5::SIG = "HMAC/MD5 0x%08x%08x%08x%08x";
const size_t HMACMD5::SIG_SZ = c_format(HMACMD5::SIG, 0, 0, 0, 0).size();

const string
HMACMD5::signature(const string& message) const
{
    uint8_t d[16];	// digest
    hmac_md5((const uint8_t*)message.c_str(), message.size(),
	     (const uint8_t*)_key.c_str(), _key.size(), d);

    uint32_t d32[4];
    for (int i = 0; i < 16; i += 4) {
	d32[i/4] = d8tod32(d[i], d[i + 1], d[i + 2], d[i + 3]);
    }
    return c_format(SIG, d32[0], d32[1], d32[2], d32[3]);
}
