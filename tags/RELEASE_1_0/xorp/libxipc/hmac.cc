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

#ident "$XORP: xorp/libxipc/hmac.cc,v 1.3 2003/03/10 23:20:24 hodson Exp $"

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
