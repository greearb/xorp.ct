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

// $XORP: xorp/libxipc/xrl_atom_encoding.hh,v 1.7 2003/03/16 08:20:31 pavlin Exp $

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
