// -*- c-basic-offset: 4; tab-width: 8; indent-tabs-mode: t -*-
// vim:set sts=4 ts=8:

// Copyright (c) 2001-2008 XORP, Inc.
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

// $XORP: xorp/ospf/fletcher_checksum.hh,v 1.5 2008/07/23 05:11:08 pavlin Exp $

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
