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

/* Definitions of commonly used OSPF-specific
 * data types
 */

typedef uns32 rtid_t;		// Router ID
typedef uns32 lsid_t;		// Link State ID
typedef uns32 aid_t;		// Area IDs
typedef	uns32 autyp_t;		// Authentication type
				// Fields in link state header
typedef	uns16 age_t;		// LS age, 16-bit unsigned
typedef	int32 seq_t;		// LS Sequence Number, 32-bit signed
typedef	uns16 xsum_t;		// LS Checksum, 16-bit unsigned
