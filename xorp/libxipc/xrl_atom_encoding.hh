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

// $XORP: xorp/libxipc/xrl_atom_encoding.hh,v 1.14 2008/10/02 21:57:24 bms Exp $

#ifndef __LIBXIPC_XRL_ATOM_ENCODING_HH__
#define __LIBXIPC_XRL_ATOM_ENCODING_HH__

#include <string>
#include <vector>

/**
 * Encode the string representation of an XrlAtom value into a value
 * suitable for integrating into a spaceless Xrl representation.  This
 * is essentially URL encoding though a reduced subset of
 * non-alphanumeric characters are escaped, ie only those that would
 * otherwise interfere with Xrl parsing.
 */
string
xrlatom_encode_value(const char* val, size_t val_bytes);

/**
 * Encode the string representation of an XrlAtom value into a value
 * suitable for integrating into a spaceless Xrl representation.  This
 * is essentially URL encoding though a reduced subset of
 * non-alphanumeric characters are escaped, ie only those that would
 * otherwise interfere with Xrl parsing.
 */
inline string
xrlatom_encode_value(const string& val)
{
    return xrlatom_encode_value(val.c_str(), val.size());
}

/**
 * Encode string representation of a binary data type XrlAtom value into
 * a value suitable for integrating into a spaceless Xrl representation.
 */
inline string
xrlatom_encode_value(const vector<uint8_t>& v)
{
    const uint8_t* start = &v[0];
    return xrlatom_encode_value(reinterpret_cast<const char*>(start),
				v.size());
}

/**
 * Decode escaped XrlAtom representation.
 *
 * @return -1 on success, or the index of the character causing the
 * decode failure in the string "in".
 */
ssize_t
xrlatom_decode_value(const char* in, size_t in_bytes, string& out);

/**
 * Decode escaped XrlAtom representation of XrlAtom binary data type.
 *
 * @return -1 on success, or the index of the character causing the
 * decode failure in the string "in".
 */
ssize_t
xrlatom_decode_value(const char* in, size_t in_bytes, vector<uint8_t>& out);

#endif // __LIBXIPC_XRL_ATOM_ENCODING_HH__
