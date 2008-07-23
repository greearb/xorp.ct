// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/ospf/fletcher_checksum.hh,v 1.4 2008/01/04 03:16:55 pavlin Exp $

#ifndef __OSPF_FLETCHER_CHECKSUM_HH__
#define __OSPF_FLETCHER_CHECKSUM_HH__

/**
 * Compute a fletcher checksum
 *
 * @param bufp pointer to start of the buffer.
 * @param length of the buffer.
 * @param off Offset into the buffer where the checksum is placed.
 * @param x output value checksum
 * @param y output value checksum
 */
void fletcher_checksum(uint8_t *bufp, size_t len,
		       size_t off, int32_t& x, int32_t& y);

#endif // __OSPF_FLETCHER_CHECKSUM_HH__
