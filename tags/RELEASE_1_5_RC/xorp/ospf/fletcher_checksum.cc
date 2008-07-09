// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 International Computer Science Institute
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

#ident "$XORP: xorp/ospf/fletcher_checksum.cc,v 1.6 2007/02/16 22:46:41 pavlin Exp $"

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
