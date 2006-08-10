// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-

// Copyright (c) 2001-2006 International Computer Science Institute
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

// $XORP$


#ifndef __LIBPROTO_PACKET_HH__
#define __LIBPROTO_PACKET_HH__


#include "libxorp/xorp.h"


/**
 * Extract an 8-bit number from the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in network order.
 *
 * @param ptr the buffer with the data.
 * @return an 8-bit number from the beginning of a buffer.
 */
inline
uint8_t
extract_8(const uint8_t *ptr)
{
    uint8_t val;

    val = ptr[0];

    return val;
}

/**
 * Embed an 8-bit number into the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in network order.
 *
 * @param ptr the buffer to store the data.
 * @param val the 8-bit value to embed into the beginning of the buffer.
 */
inline
void
embed_8(uint8_t *ptr, uint8_t val)
{
    ptr[0] = val;
}

/**
 * Extract a 16-bit number from the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in network order.
 *
 * @param ptr the buffer with the data.
 * @return a 16-bit number from the beginning of a buffer.
 */
inline
uint16_t
extract_16(const uint8_t *ptr)
{
    uint16_t val;

    val = ptr[0];
    val <<= 8;
    val |= ptr[1];

    return val;
}

/**
 * Embed a 16-bit number into the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in network order.
 *
 * @param ptr the buffer to store the data.
 * @param val the 16-bit value to embed into the beginning of the buffer.
 */
inline
void
embed_16(uint8_t *ptr, uint16_t val)
{
    ptr[0] = (val >> 8) & 0xff;
    ptr[1] = val & 0xff;
}

/**
 * Extract a 24-bit number from the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in network order.
 *
 * @param ptr the buffer with the data.
 * @return a 24-bit number from the beginning of a buffer.
 */
inline
uint32_t
extract_24(const uint8_t *ptr)
{
    uint32_t val;

    val = ptr[0];
    val <<= 8;
    val |= ptr[1];
    val <<= 8;
    val |= ptr[2];

    return val;
}

/**
 * Embed a 24-bit number into the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in network order.
 *
 * @param ptr the buffer to store the data.
 * @param val the 24-bit value to embed into the beginning of the buffer.
 */
inline
void
embed_24(uint8_t *ptr, uint32_t val)
{
    ptr[0] = (val >> 16) & 0xff;
    ptr[1] = (val >> 8) & 0xff;
    ptr[2] = val & 0xff;
}

/**
 * Extract a 32-bit number from the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in network order.
 *
 * @param ptr the buffer with the data.
 * @return a 32-bit number from the beginning of a buffer.
 */
inline
uint32_t
extract_32(const uint8_t *ptr)
{
    uint32_t val;

    val = ptr[0];
    val <<= 8;
    val |= ptr[1];
    val <<= 8;
    val |= ptr[2];
    val <<= 8;
    val |= ptr[3];

    return val;
}

/**
 * Embed a 32-bit number into the beginning of a buffer.
 *
 * Note that the integers in the buffer are stored in network order.
 *
 * @param ptr the buffer to store the data.
 * @param val the 32-bit value to embed into the beginning of the buffer.
 */
inline
void
embed_32(uint8_t *ptr, uint32_t val)
{
    ptr[0] = (val >> 24) & 0xff;
    ptr[1] = (val >> 16) & 0xff;
    ptr[2] = (val >> 8) & 0xff;
    ptr[3] = val & 0xff;
}

#endif // __LIBPROTO_PACKET_HH__
