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

/* Class that all configurable classes inherit from.
 * This allows us to use a generic mechanism for adding
 * and deleting configuration items.
 */

class ConfigItem {
    ConfigItem *prev;
    ConfigItem *next;
protected:
    int	updated;
public:
    ConfigItem();
    virtual ~ConfigItem();
    virtual void clear_config() = 0;
    friend class OSPF;
};

// Configuration message formats

/* Global parameters set in the OSPF software.
 * Router ID is given when the OSPF process is first initialized.
 * Admin status is revoked by calling ospf class destructor.
 * TOS routing is not supported.
 * The demand circuit extensions are always enabled.
 */

struct CfgGen {
    int	lsdb_limit;	// Maximum number of AS-external-LSAs
    int	mospf_enabled;	// Running MOSPF?
    int	inter_area_mc;	// Inter-area multicast forwarder?
    int	ovfl_int;	// Time to exit overflow state
    int	new_flood_rate; // # self-orig per second
    uns16 max_rxmt_window;// # back-to-back retransmissions
    byte max_dds;	// # simultaneous DB exchanges
    byte host_mode;	// Don't forward data packets?
    int log_priority;	// Logging message priority
    int32 refresh_rate;	// Rate to refresh DoNotAge LSAs
    uns32 PPAdjLimit;	// Max # p-p adjacencies to neighbor
    int random_refresh;	// Should we spread out LSA refreshes?

    void set_defaults();
};

/* OSPF area configuration.
 */

struct CfgArea {
    aid_t area_id;	// OSPF area ID

    int	stub;		// stub area?
    uns32 dflt_cost;	// Cost of imported default (stubs)
    int	import_summs;	// Import summary routes? (stubs)
};

/* Configuration of OSPF area ranges.
 */

struct CfgRnge {
    InAddr net;		// Network and
    InMask mask; 	// mask
    aid_t area_id;	// Associated area

    int	no_adv;		// Suppress advertisement?
};

/* Configuration of advertised hosts.
 */

struct CfgHost {
    InAddr net;		// Network and
    InMask mask; 	// mask

    aid_t area_id;	// Area host belongs to
    uns16 cost;		// Cost to advertise
};

/* Configured interface parameters
 */

struct CfgIfc {
    InAddr address;	// IP address
    int	phyint;		// Physical interface

    InMask mask; 	// Interface address mask
    uns16 mtu;		// Max IP datagram in bytes
    uns16 IfIndex;	// MIB-II IfIndex
    aid_t area_id;	// Attached area
    int	IfType;		// Type of interface (NBMA, etc.)
    byte dr_pri;	// Designated router priority
    byte xmt_dly;	// Transit delay (seconds)
    byte rxmt_int;	// Retransmission interval (seconds)
    uns16 hello_int;	// Hello interval (seconds)
    uns16 if_cost;	// Interface cost
    uns32 dead_int;	// Router dead interval (seconds)
    uns32 poll_int;	// Poll interval
    int	auth_type;	// Authentication type
    byte auth_key[8];	// Authentication key
    int	mc_fwd;		// Multicast forwarding enabled?
    int	demand;		// On Demand interface?
    int passive;	// Don't send control packets?
    int igmp;		// IGMP enabled?
};


/* Enumeration of interface types.
 */
enum {
    IFT_BROADCAST = 1,
    IFT_PP,
    IFT_NBMA,
    IFT_P2MP,
    IFT_VL,
    IFT_LOOPBK,
    };

/* Values for interface's multicast forwarding field,
 * CfgIfc::mc_fwd and SpfIfc::if_mcfwd.
 */

enum {
    IF_MCFWD_BLOCKED = 0,
    IF_MCFWD_MC = 1,
    IF_MCFWD_UNI = 2,
};

/* Configuration of virtual links.
 */

struct CfgVL {
    aid_t transit_area;	// Transit area
    rtid_t nbr_id; 	// Other endpoint's router ID

    byte xmt_dly;	// Transit delay (seconds)
    byte rxmt_int;	// Retransmission interval (seconds)
    uns16 hello_int;	// Hello interval (seconds)
    uns32 dead_int;	// Router dead interval (seconds)
    int	auth_type;	// Authentication type
    byte auth_key[8];	// Authentication key
};

/* Neighbor configuration.
 * Only used for neighbors on NBMA and Point-to-MultiPoint
 * interfaces, which cannot be dynamically discovered.
 */

struct CfgNbr {
    InAddr nbr_addr;	// Neighbor's IP address

    int	dr_eligible;	// Eligible to become Designated Router?
};

/* Indication of an external route to import into
 * OSPF
 */

struct CfgExRt {
    InAddr net;		// Network
    InMask mask; 	// and mask

    int	type2:1,	// external type, 1 or 2
	mc:1,		// Multicast source?
        direct:1,	// Directly attached subnet?
	noadv:1;	// Don't advertise in AS-external-LSAs
    uns32 cost;		// cost to advertise
    int	phyint;		// Outgoing physical interface		
    InAddr gw;		// Next hop gateway
    uns32 tag;		// Tag to advertise
};

/* Key to add or delete from a given interface
 */

struct CfgAuKey {
    InAddr address;	// IP address
    int	phyint;		// Physical interface
    byte key_id; 	// Key ID

    byte auth_key[16];	// Authentication key

    // Transmission usage parameters
    SPFtime start_generate;		
    SPFtime stop_generate;
    bool    stop_generate_specified;

    // Reception usage parameters
    SPFtime start_accept;	
    SPFtime stop_accept;
    bool    stop_accept_specified;
};


// Encoding of configuration types
enum {
    CfgType_Gen = 1,	// Global config items
    CfgType_Area, 	// Area configuration
    CfgType_Range, 	// Area Aggregate
    CfgType_Host, 	// Loopback addresses
    CfgType_Ifc, 	// Interface configuration
    CfgType_VL,		// Virtual link configuration
    CfgType_Nbr, 	// NBMA, p-m-p neighbor configuration
    CfgType_Route, 	// Import external route
    CfgType_Key, 	// Interface authentication key
    CfgType_Start, 	// Start complete config pass
    CfgType_Done, 	// End configuration pass
};

// Encoding of status field
enum {
    ADD_ITEM,
    DELETE_ITEM,
};
