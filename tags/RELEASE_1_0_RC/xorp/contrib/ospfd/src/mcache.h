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

#ifndef MCACHE_H
#define MCACHE_H

/* The definition of a multicast cache entry.
 * These are calculated and stored by MOSPF, and
 * also downloaded into the platform's kernel.
 * Cache entries are calculated for each combination
 * of multicast source and group destination.
 * Multiple upstream interfaces are possible when
 * multiple point-to-point links connect a pair
 * of routers; MOSPF is unable to distinguish between
 * the multiple lines, so cannot apply tiebreakers.
 */

/* Downstream interfaces carry the number of hops
 * to the nearest group member, which can be used to
 *
 * Downstream neighbors are used instead of interfaces
 * when the interface is non-broadcast, or when multicast
 * forwarding is configured to used data-link unicast.
 * Neighbos are indicated by nbr_addr != 0.
 */

struct DownStr {
    int phyint;
    InAddr nbr_addr;
    byte ttl;
};

/* The cache entry itself.
 */

struct MCache {
    int n_upstream;
    int *up_phys;
    int n_downstream;
    DownStr *down_str;
    InMask mask;

    MCache();
    ~MCache();
    bool valid_incoming(int in_phyint);
    bool valid_outgoing(int phyint, InAddr nbr_addr, byte &ttl);
};

#endif
