/*
 *   OSPFD routing daemon
 *   Copyright (C) 1998 by John T. Moy
 *   
 *   This program is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU General Public License
 *   as published by the Free Software Foundation; either version 2
 *   of the License, or (at your option) any later version.
 *   
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *   
 *   You should have received a copy of the GNU General Public License
 *   along with this program; if not, write to the Free Software
 *   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdarg.h>
#include <string.h>
#include <time.h>

/* 
 * Machine-dependent and compiler-dependent constants and macros.
 * Defined here for a Intel architecture, and a compiler whose
 * "ints" are 32-bits and "shorts" 16.
 */

// Definition of fixed-length types

typedef unsigned char byte;	// Byte (8-bit unsigned)
typedef	unsigned short uns16;	// 16-bit unsigned
typedef	unsigned uns32;		// 32-bit unsigned
typedef char int8;		// 8-bit signed
typedef	short int16;		// 16-bit signed
typedef	int int32;		// 32-bit signed

typedef unsigned long word;	// Generic pointer

// Constant for fletcher checksum calculation
const int MODX = 4102;

// Machine-dependencies in the socket interface
typedef unsigned int socklen;	// length arg to accept(), etc.

// Convert fields between network and machine byte-order
// This involves byte-swapping on an Intel architecture

// Convert a 32-bit unsigned quantity from network to host order

inline uns32 ntoh32(uns32 value)

{
    uns32 swval;

    swval = (value << 24);
    swval |= (value << 8) & 0x00ff0000L;
    swval |= (value >> 8) & 0x0000ff00L;
    swval |= (value >> 24);
    return(swval);
}

// Convert a 32-bit quantity from host to network order

inline uns32 hton32(uns32 value)

{
    return(ntoh32(value));
}

// Convert a 16-bit unsigned quantity from network to host order

inline uns16 ntoh16(uns16 value)

{
    uns16 swval;

    swval = (value << 8);
    swval |= (value >> 8) & 0xff;
    return(swval);
}

// Convert a 16-bit quantity from host to network order

inline uns16 hton16(uns16 value)

{
    return(ntoh16(value));
}

// External definitions
//extern "C" char *strptime(char *buf, const char *format, const struct tm *tm);
