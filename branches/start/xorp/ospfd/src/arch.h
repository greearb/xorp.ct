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

/*
 * Architectural constants for the OSPF protocol. Defined in
 * Appendix B of the OSPF specification.
 * All time constants are in seconds.
 */

const byte OSPFv2 = 2;			// OSPF version number

const age_t LSRefreshTime = 1800;	// Max time between LSA updates
const age_t MinLSInterval = 5;		// Min time between updates
const age_t MaxAge = 3600;		// Maximum LS Age value
const age_t CheckAge = 300;		// Verify checksums this often
const age_t MaxAgeDiff = 900;		// Max LS age dispersion
const age_t DoNotAge = 0x8000;		// Bit set => don't age LSA
const age_t MinArrival = 1;		// Min time between LSA receptions

const uns32 LSInfinity = 0xffffffL;	// Metric value => unreachable
const uns32 DefaultDest = 0L;		// Default destination net
const uns32 DefaultMask	= 0L;		// mask for default route
const uns32 HostMask = 0xffffffffL;	// All ones host mask
const uns32 UnknownAddr	= 0L;		// Unknown address (0.0.0.0)

const seq_t InvalidLSSeq = 0x80000000L;	// Invalid LS sequence number
const seq_t InitLSSeq = 0x80000001L;	// Initial LS Sequence value
const seq_t MaxLSSeq = 0x7fffffffL;	// Maximum LS Sequence
const seq_t LSInvalid = 0x80000000L;	// Invalid LS Sequence value

const aid_t BACKBONE = 0;		// OSPF Backbone area
const InAddr AllSPFRouters = 0xe0000005; // 224.0.0.5
const InAddr AllDRouters = 0xe0000006;  // 224.0.0.6
