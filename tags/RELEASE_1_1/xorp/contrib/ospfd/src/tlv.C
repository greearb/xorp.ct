/*
 *   OSPFD routing daemon
 *   Copyright (C) 2001 by John T. Moy
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

/* This file contains the routines used to build and parse
 * TLVs within the body of OSPF LSAs.
 */

#include "machdep.h"
#include "spftype.h"
#include "spfparam.h"
#include "ip.h"
#include "arch.h"
#include "lshdr.h"
#include "tlv.h"

/* Return the length of the TLV, including header and
 * padding, given the length of the value within the
 * body.
 */

int TLVbuf::tlen(int vlen)

{
    int len;
    len = 4*((vlen + 3)/4);
    len += sizeof(TLV);
    return(len);
}

/* Reserve space in the TLV buffer for a new TLV,
 * given the length of its value. Reallocate
 * the current buffer, copying its contents, if necessary.
 */

TLV *TLVbuf::reserve(int vlen)

{
    int len;
    byte *end;
    byte *old;
    TLV *fill;
    int used;

    len = tlen(vlen);
    end = buf + blen;
    if (buf == 0) {
        buf = new byte[len];
	current = (TLV *)buf;
	blen = len;
    }
    else if ((((byte *)current) + len) > end) {
        old = buf;
	used = ((byte *)current) - buf;
	buf = new byte[len + used];
	memcpy(buf, old, used);
	blen = len + used;
	current = (TLV *)(buf + used);
	delete old;
    }
    // Update current length
    fill = current;
    memset(fill, 0, len);
    current = (TLV *) (((byte *)fill) + len);
    return(fill);
}

/* Constructor for a buffer of TLVs that is to be
 * parsed.
 */

TLVbuf::TLVbuf(byte *body, int len)

{
    buf = body;
    blen = len;
    current = 0;	// Indicates no TLV has been parsed
}

/* Get the type of the next TLV in the buffer. Subsequent
 * calls to other methods (like get_int()) will be used to
 * get the value out of the TLV.
 * A return of false indicates that there are no more TLVs
 * available.
 */

bool TLVbuf::next_tlv(int & type)

{
    byte *end;
    TLV *next;

    if (current == 0)
        next = (TLV *) buf;
    else
        next = (TLV *) (((byte *)current) + tlen(ntoh16(current->length)));

    // Check to see the entire TLV is cont ained with the buffer
    end = buf + blen;
    if (end < (((byte *)next) + sizeof(TLV)) ||
	end < (((byte *)next) + tlen(ntoh16(next->length))))
        return(false);

    type = ntoh16(next->type);
    current = next;
    return(true);
}

/* Check to see that the value within the current TLV is 4 bytes, and if
 * so, return it as an integer.
 */

bool TLVbuf::get_int(int32 & val)

{
    int32 *ptr;
    if (!current || ntoh16(current->length) != sizeof(int32))
        return(false);
    ptr = (int32 *)(current + 1);
    val = ntoh32(*ptr);
    return(true);
}

/* Check to see that the value within the current TLV is 2 bytes, and if
 * so, return it as an unsigned short.
 */

bool TLVbuf::get_short(uns16 & val)

{
    uns16 *ptr;
    if (!current || ntoh16(current->length) != sizeof(uns16))
        return(false);
    ptr = (uns16 *)(current + 1);
    val = ntoh16(*ptr);
    return(true);
}

/* Check to see that the value within the current TLV is 1 byte, and if
 * so, return it.
 */

bool TLVbuf::get_byte(byte & val)

{
    byte *ptr;
    if (!current || ntoh16(current->length) != sizeof(byte))
        return(false);
    ptr = (byte *)(current + 1);
    val = *ptr;
    return(true);
}

/* If there us a current TLV, return a pointer to its body and
 * indicate its length. A return of 0 indicates that there
 * was no TLV to parse.
 */

char *TLVbuf::get_string(int & len)

{
    if (!current)
        return(0);
    len = ntoh16(current->length);
    return((char *)(current + 1));
}

/* Routines used to build an LSA body consisting of TLVs.
 */

/* When TLVbuf originally constructed, it has no buffer
 * associated with it. As TLVs are added, reserve() will allocate
 * appropriate buffer space.
 */

TLVbuf::TLVbuf()

{
    buf = 0;
    blen = 0;
    current = 0;
}

/* To re-use a TLV transmit buffer, simply set current to the
 * begiining of the buffer. When building the packet, current
 * is always pointing to the end of the TLVs that are current
 * in the buffer.
 */

void TLVbuf::reset()

{
    current = (TLV *) buf;
}

/* Put a TLV with a 4-byte integer value into the buffer.
 */

void TLVbuf::put_int(int type, int32 val)

{
    TLV *fill;
    int32 *ptr;

    fill = reserve(sizeof(val));
    fill->type = hton16(type);
    fill->length = hton16(sizeof(val));
    ptr = (int32 *)(fill + 1);
    *ptr = hton32(val);
}

/* Put a TLV with a 2-byte integer value into the buffer.
 */

void TLVbuf::put_short(int type, uns16 val)

{
    TLV *fill;
    uns16 *ptr;

    fill = reserve(sizeof(val));
    fill->type = hton16(type);
    fill->length = hton16(sizeof(val));
    ptr = (uns16 *)(fill + 1);
    *ptr = hton16(val);
}

/* Put a TLV with a one byte value into the buffer.
 */

void TLVbuf::put_byte(int type, byte val)

{
    TLV *fill;
    byte *ptr;

    fill = reserve(sizeof(val));
    fill->type = hton16(type);
    fill->length = hton16(sizeof(val));
    ptr = (byte *)(fill + 1);
    *ptr = val;
}

/* Put a TLV with a string value into the buffer.
 */

void TLVbuf::put_string(int type, char *str, int len)

{
    TLV *fill;
    byte *ptr;

    fill = reserve(len);
    fill->type = hton16(type);
    fill->length = hton16(len);
    ptr = (byte *)(fill + 1);
    memcpy(ptr, str, len);
}
