// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2009 XORP, Inc.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License, Version 2, June
// 1991 as published by the Free Software Foundation. Redistribution
// and/or modification of this program under the terms of any other
// version of the GNU General Public License is not permitted.
// 
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. For more details,
// see the GNU General Public License, Version 2, a copy of which can be
// found in the XORP LICENSE.gpl file.
// 
// XORP Inc, 2953 Bunker Hill Lane, Suite 204, Santa Clara, CA 95054, USA;
// http://xorp.net



#include "ospf_module.h"

#include "libxorp/xorp.h"

#include "fletcher_checksum.hh"


/*
** return number modulo 255 most importantly convert negative numbers
** to a ones complement representation.
*/
inline
int32_t 
onecomp(int32_t a)
{
	int32_t res;

	res = a % 255;

	if(res <= 0)
		res = 255 + res;

	return res;
}

/*
** generate iso checksums.
*/
void
fletcher_checksum(uint8_t *bufp, size_t len, size_t off,
		  int32_t& x, int32_t& y)
{
        int32_t c0 = 0, c1 = 0;

	for(size_t i = 0; i < len; i++) {
		c0 = bufp[i] + c0;
		c1 = c1 + c0;
	}
	c0 %= 255;
	c1 %= 255;

	off += 1;	// C Arrays are from 0 not 1.
	x = onecomp(-c1 + (len - off) * c0);
	y = onecomp(c1 - (len - off + 1) * c0);
}
