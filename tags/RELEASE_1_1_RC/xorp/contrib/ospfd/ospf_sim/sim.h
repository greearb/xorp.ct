/*
 *   OSPFD routing simulator
 *   Copyright (C) 1999 by John T. Moy
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

const int TICKS_PER_SECOND = 20; // Simulated time granularity
const int LINK_DELAY = 10;	// Simulated link delay (milliseconds)

/* Packet types exchanged between the simulation
 * controller and the individual ospfd simulations.
 */

enum {
    // From controller to simulated ospfds
    SIM_FIRST_TICK = 1,	// First time tick
    SIM_TICK,		// Simulated time tick
    SIM_CONFIG,		// Config messages
    SIM_CONFIG_DEL,	// Config deletion messages
    SIM_ADDRMAP,	// Address to group mappings
    SIM_SHUTDOWN,	// Router shutdown request
    SIM_START_PING,	// Start ping session
    SIM_STOP_PING,	// Stop ping session
    SIM_START_TR,	// Start traceroute session
    SIM_STOP_TR,	// Stop traceroute session
    SIM_ADD_MEMBER,	// Add membership on segment
    SIM_DEL_MEMBER,	// Delete membership on segment
    SIM_START_MTRACE,	// Start multicast traceroute session
    SIM_RESTART,	// Restart router
    SIM_RESTART_HITLESS,// Hitless restart of router

    // Responses from ospfds
    SIM_HELLO = 100,	// Initial identification
    SIM_TICK_RESPONSE,  // Tick response
    SIM_LOGMSG,		// Logging message
    SIM_ECHO_REPLY,	// Echo reply
    SIM_ICMP_ERROR,	// ICMP error received
    SIM_TRACEROUTE_TTL,	// Traceroute TTL
    SIM_TRACEROUTE_TMO,	// Traceroute timeout
    SIM_TRACEROUTE_DONE,// Traceroute complete
    SIM_PRINT_SESSION,	// Print line in session window
};

/* Tick message contains the simulators version of
 * time, so that all the simulated routers can agree.
 */

struct TickBody {
    uns32 tick;
};

/* Body of the address map message is an array of the
 * following structures which map unicast IP addresses
 * into ports listening for the address.
 */

struct AddrMap {
  InAddr addr;		// Router interface address
  uns32 port;		// Port listening for address
  InAddr home;		// Router ID of owning router
};

/* Body of the Hello message carries the OSPF router ID
 * of the simulated router.
 */

struct SimHello {
    InAddr rtrid;
    uns16 myport;
};

/* Body of the Start ping command carries the destination and TTL
 * of the ping session. The ID of the session is carried in the
 * message subtype. If src is non-zero, it will be used as the
 * source of ping packets, otherwise the router will chose the
 * source itself.
 */

struct PingStartMsg {
    InAddr src;
    InAddr dest;
    byte ttl;
};

/* Body of the Hitless Restart message contains the length
 * of the hitless restart period in seconds.
 */

struct HitlessRestartMsg {
    uns16 period;
};

/* Tick responses carry the #LSAs and checksum for 
 * AS-externals and each area, so that the controller
 * can tell whether the routers' databases are
 * sychronized.
 */

struct DBStats {
    uns32 n_exlsas;	// # AS-external-LSAs in database
    uns32 ex_dbxsum;	// AS-external-LSA checksum
    InAddr area_id;
    uns32 n_lsas;	// # LSAs in database
    uns32 dbxsum;	// Area database checksum
};

/* Body of the Echo reply response.
 * The ID of the session is carried in the
 * message subtype.
 */

struct EchoReplyMsg {
    InAddr src;
    uns32 msd;
    uns16 icmp_seq;
    byte ttl;
};

/* Body of the ICMP error message.
 * The ID of the session is carried in the
 * message subtype.
 */

struct IcmpErrMsg {
    InAddr src;
    uns32 msd;
    byte type;
    byte code;
    uns16 icmp_seq;
};

/* Traceroute informs us of the current TTL
 * that it is probing.
 */

struct TrTtlMsg {
    int ttl;
};

/* Messages associated with the add and delete group
 * membership commands.
 */

struct GroupMsg {
    int phyint;
    InAddr group;
};
