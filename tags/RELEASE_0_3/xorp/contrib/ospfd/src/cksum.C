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

#include "machdep.h"
#include "spftype.h"
#include "ip.h"
#include "arch.h"
#include "lshdr.h"

/* Calculate the fletcher checksum of a message, given
 * its length an the offset of the checksum field.
 * Uses the algorithm from RFC 1008. MODX is chosen to be the
 * length of the smallest block that can be checksummed without
 * overrunning a signed integer.
 */

uns16 fletcher(byte *message, int mlen, int offset)

{
    byte *ptr;
    byte *end;
    int c0; // Checksum high byte
    int c1; // Checksum low byte
    uns16 cksum;	// Concatenated checksum
    int	iq;	// Adjust for message placement, high byte
    int	ir;	// low byte

    // Set checksum field to zero
    if (offset) {
	message[offset-1] = 0;
	message[offset] = 0;
    }

    // Initialize checksum fields
    c0 = 0;
    c1 = 0;
    ptr = message;
    end = message + mlen;

    // Accumulate checksum
    while (ptr < end) {
	byte	*stop;
	stop = ptr + MODX;
	if (stop > end)
	    stop = end;
	for (; ptr < stop; ptr++) {
	    c0 += *ptr;
	    c1 += c0;
	}
	// Ones complement arithmetic
	c0 = c0 % 255;
	c1 = c1 % 255;
    }

    // Form 16-bit result
    cksum = (c1 << 8) + c0;

    // Calculate and insert checksum field
    if (offset) {
	iq = ((mlen - offset)*c0 - c1) % 255;
	if (iq <= 0)
	    iq += 255;
	ir = (510 - c0 - iq);
	if (ir > 255)
	    ir -= 255;
	message[offset-1] = iq;
	message[offset] = ir;
    }

    return(cksum);
}

/* Verify an LSA's checksum.
 */

bool LShdr::verify_cksum()

{
    byte *message;
    int	mlen;

    message = (byte *) &ls_opts;
    mlen = ntoh16(ls_length) - sizeof(age_t);
    return (fletcher(message, mlen, 0) == (uns16) 0);
}

/* Generate an LSA's checksum.
 */

void LShdr::generate_cksum()

{
    byte *message;
    int	mlen;
    int	offset;

    message = (byte *) &ls_opts;
    mlen = ntoh16(ls_length) - sizeof(age_t);
    offset = (int) (((byte *)&ls_xsum) - message) + 1;
    (void) fletcher(message, mlen, offset);
}
